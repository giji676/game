#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "engine/shader.h"
// #include "engine/texture.h"
// #include "engine/model.h"

class AssetManager {
public:
    void init();

    Shader& loadShader(const std::string& name,
                       const std::string& vsPath,
                       const std::string& fsPath);

    Shader& getShader(const std::string& name);

    // Texture& loadTexture(const std::string& name,
    //                      const std::string& path);
    //
    // Texture& getTexture(const std::string& name);
    //
    // Model& loadModel(const std::string& name,
    //                  const std::string& path);
    //
    // Model& getModel(const std::string& name);

private:
    std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;
    // std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
    // std::unordered_map<std::string, std::unique_ptr<Model>> models;
};
