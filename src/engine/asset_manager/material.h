#pragma once

#include <vector>

#include "shader.h"
#include "texture.h"

class Material {
public:
    uint32_t id = 0;

    Shader* shader;

    std::vector<Texture*> textures;

    glm::vec3 diffuseFallback{1.f};
    glm::vec3 specularFallback{0.f};
    // MTL `d` dissolve / opacity. 1 = fully opaque.
    float opacity = 1.f;

    void bind() const;
    bool usesTransparency() const;
};
