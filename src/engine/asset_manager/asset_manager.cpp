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

