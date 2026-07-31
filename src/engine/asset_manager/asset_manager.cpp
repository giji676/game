#include "asset_manager.h"

#include "shader.h"
#include "model.h"
#include "texture.h"

#include "gj_image/gj_image.h"

#include <thread>

// enque function for loading multiple assets in parallel
// once all assets are loaded a signal is emitted to notify 
// the engine that the assets are ready

Texture& AssetManager::enqueImageLoad(const std::string& name,
        const std::string& fullPath,
        const std::string& type)
{
    if (auto it = textures_.find(name); it != textures_.end())
        return *it->second;
    if (auto it = enquedTextures_.find(name); it != enquedTextures_.end())
        return *it->second;

    auto tex = std::make_unique<Texture>(fullPath, type, imageVFlip_);
    auto [it, inserted] = enquedTextures_.emplace(name, std::move(tex));
    return *it->second;
}

void AssetManager::processEnqueuedImageLoads()
{
    // Workers flip CPU-side from Texture::flipY; keep the global flag off so
    // concurrent gj_image_load calls do not race on it.
    gj_vflip_image(0);

    std::vector<std::thread> threads;
    threads.reserve(enquedTextures_.size());
    std::vector<Texture*> uploaded;

    // Move textures into the main map and load image bytes off the GL thread.
    for (auto& [name, tex] : enquedTextures_) {
        if (!tex)
            continue;

        auto [it, inserted] = textures_.emplace(name, std::move(tex));
        if (!inserted || !it->second)
            continue;

        Texture* texture = it->second.get();
        uploaded.push_back(texture);
        threads.emplace_back(&Texture::loadImage, texture);
    }

    for (auto& t : threads)
        t.join();

    // GL upload must happen on the main/context thread after CPU loads finish.
    for (Texture* texture : uploaded)
        texture->setupImageGPU();

    enquedTextures_.clear();
}

Shader& AssetManager::loadShader(const std::string& name,
                                 const std::string& vsPath,
                                 const std::string& fsPath)
{
    auto it = shaders_.find(name);
    if (it != shaders_.end())
        return *it->second;

    auto shader = std::make_unique<Shader>(vsPath.c_str(), fsPath.c_str());
    shaders_.emplace(name, std::move(shader));

    return *shaders_.at(name);
}

Shader& AssetManager::getShader(const std::string& name)
{
    return *shaders_.at(name);
}

Texture& AssetManager::loadTexture(const std::string& name,
                                   const std::string& fullPath,
                                   const std::string& type)
{
    auto it = textures_.find(name);
    if (it != textures_.end()) {
        return *it->second;
    }

    auto tex = std::make_unique<Texture>(fullPath, type, imageVFlip_);
    gj_vflip_image(0);
    tex->loadImage();
    tex->setupImageGPU();
    textures_.emplace(name, std::move(tex));

    return *textures_.at(name);
}

Texture& AssetManager::getTexture(const std::string& name)
{
    return *textures_.at(name);
}

Model& AssetManager::loadModel(const std::string& name,
                               const std::string& path)
{
    auto it = models_.find(name);
    if (it != models_.end()) {
        return *it->second;
    }

    auto model = std::make_unique<Model>(path.c_str(), this);
    models_.emplace(name, std::move(model));

    return *models_.at(name);
}

Model& AssetManager::getModel(const std::string& name)
{
    return *models_.at(name);
}

Font& AssetManager::loadFont(const std::string& name,
                               const std::string& path)
{
    auto it = fonts_.find(name);
    if (it != fonts_.end()) {
        return *it->second;
    }

    auto font = std::make_unique<Font>(path.c_str());
    fonts_.emplace(name, std::move(font));

    return *fonts_.at(name);
}

Font& AssetManager::getFont(const std::string& name)
{
    return *fonts_.at(name);
}

Material& AssetManager::getOrCreateMaterial(
    Shader* shader,
    Texture* diffuse,
    Texture* specular,
    Texture* alpha,
    glm::vec3 diffuseFallback,
    glm::vec3 specularFallback,
    float opacity)
{
    // create a key from the combination
    std::string key = std::to_string((uint64_t)shader)
                    + "_" + std::to_string((uint64_t)diffuse)
                    + "_" + std::to_string((uint64_t)specular)
                    + "_" + std::to_string((uint64_t)alpha)
                    + "_" + std::to_string(opacity);

    auto it = materials_.find(key);
    if (it != materials_.end())
        return *it->second;

    auto mat = std::make_unique<Material>();
    mat->shader = shader;
    mat->diffuseFallback = diffuseFallback;
    mat->specularFallback = specularFallback;
    mat->opacity = opacity;
    mat->id = allocateMaterialId();
    if (diffuse) mat->textures.push_back(diffuse);
    if (specular) mat->textures.push_back(specular);
    if (alpha) mat->textures.push_back(alpha);

    materials_.emplace(key, std::move(mat));
    return *materials_.at(key);
}
