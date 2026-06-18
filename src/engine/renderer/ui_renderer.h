#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class UIRenderer {
public:
    void render(const glm::mat4& projection);
};
