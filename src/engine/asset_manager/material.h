#pragma once

#include <vector>

#include "shader.h"
#include "texture.h"

class Material {
public:
    Shader* shader;

    std::vector<Texture> textures;

    glm::vec3 diffuseFallback{1.f};
    glm::vec3 specularFallback{0.f};

    void bind() const;
};
