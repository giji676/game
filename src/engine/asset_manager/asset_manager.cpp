#include "asset_manager.h"

#include "shader.h"
#include "model.h"
#include "texture.h"

Shader& AssetManager::loadShader(const std::string& name,
                                 const std::string& vsPath,
                                 const std::string& fsPath)
{
    auto it = shaders.find(name);
    if (it != shaders.end())
        return *it->second;

    auto shader = std::make_unique<Shader>(vsPath.c_str(), fsPath.c_str());
    shaders.emplace(name, std::move(shader));

    return *shaders.at(name);
}

Shader& AssetManager::getShader(const std::string& name)
{
    return *shaders.at(name);
}

Texture& AssetManager::loadTexture(const std::string& name,
                                   const std::string& fullPath,
                                   const std::string& type)
{
    auto it = textures.find(name);
    if (it != textures.end()) {
        return *it->second;
    }

    auto tex = std::make_unique<Texture>(fullPath.c_str(), type.c_str());
    textures.emplace(name, std::move(tex));

    return *textures.at(name);
}

Texture& AssetManager::getTexture(const std::string& name)
{
    return *textures.at(name);
}

Model& AssetManager::loadModel(const std::string& name,
                               const std::string& path)
{
    auto it = models.find(name);
    if (it != models.end()) {
        return *it->second;
    }

    auto model = std::make_unique<Model>(path.c_str(), this);
    models.emplace(name, std::move(model));

    return *models.at(name);
}

Model& AssetManager::getModel(const std::string& name)
{
    return *models.at(name);
}

Font& AssetManager::loadFont(const std::string& name,
                               const std::string& path)
{
    auto it = fonts.find(name);
    if (it != fonts.end()) {
        return *it->second;
    }

    auto font = std::make_unique<Font>(path.c_str());
    fonts.emplace(name, std::move(font));

    return *fonts.at(name);
}

Font& AssetManager::getFont(const std::string& name)
{
    return *fonts.at(name);
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

    auto it = materials.find(key);
    if (it != materials.end())
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

    materials.emplace(key, std::move(mat));
    return *materials.at(key);
}
