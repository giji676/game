#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "shader.h"
#include "texture.h"

class Model;

class AssetManager {
public:
    void init();

    Shader& loadShader(const std::string& name,
                       const std::string& vsPath,
                       const std::string& fsPath);

    Shader& getShader(const std::string& name);

    Texture& loadTexture(const std::string& name,
                         const std::string& fullPath,
                         const std::string& type);

    Texture& getTexture(const std::string& name);

    Model& loadModel(const std::string& name,
                     const std::string& path);

    Model& getModel(const std::string& name);

    uint32_t allocateMaterialId() { return nextMaterialId++; }
    uint32_t allocateMeshId() { return nextMeshId++; }

private:
    std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
    std::unordered_map<std::string, std::unique_ptr<Model>> models;

    uint32_t nextMaterialId = 1;
    uint32_t nextMeshId = 1;
};
