#include "engine/asset_manager.h"
#include "engine/shader.h"

Shader& AssetManager::loadShader(const std::string& name,
                                 const std::string& vsPath,
                                 const std::string& fsPath) {
    auto shader = std::make_unique<Shader>(vsPath.c_str(), fsPath.c_str());
    shaders[name] = std::move(shader);
    return *shaders[name];
}

Shader& AssetManager::getShader(const std::string& name) {
    return *shaders.at(name);
}
