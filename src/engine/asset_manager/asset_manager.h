#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "engine/asset_manager/material.h"
#include "font.h"
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

    Font& loadFont(const std::string& name,
                     const std::string& path);

    Font& getFont(const std::string& name);

    // Multithreaded
    // Captured by enqueImageLoad / loadTexture at call time (per-texture).
    void setImageVFlip(bool flip) { imageVFlip_ = flip; }
    bool getImageVFlip() const { return imageVFlip_; }

    Texture& enqueImageLoad(const std::string& name,
                        const std::string& fullPath,
                        const std::string& type);

    void processEnqueuedImageLoads();
    //

    Material& getOrCreateMaterial(
            Shader* shader,
            Texture* diffuse,
            Texture* specular,
            Texture* alpha,
            glm::vec3 diffuseFallback,
            glm::vec3 specularFallback,
            float opacity = 1.f);

    uint32_t allocateMaterialId() { return nextMaterialId_++; }
    uint32_t allocateMeshId() { return nextMeshId_++; }
    uint32_t allocateFontId() { return nextFontId_++; }

private:
    std::unordered_map<std::string, std::unique_ptr<Shader>> shaders_;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures_;
    std::unordered_map<std::string, std::unique_ptr<Material>> materials_;
    std::unordered_map<std::string, std::unique_ptr<Model>> models_;
    std::unordered_map<std::string, std::unique_ptr<Font>> fonts_;

    std::unordered_map<std::string, std::unique_ptr<Texture>> enquedTextures_;

    bool imageVFlip_ = false;

    uint32_t nextMaterialId_ = 1;
    uint32_t nextMeshId_ = 1;
    uint32_t nextFontId_ = 1;
};
