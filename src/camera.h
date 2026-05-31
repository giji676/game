#pragma once

#include "glm/ext/vector_float3.hpp"

class GJ_Camera {
public:
    glm::vec3 pos = glm::vec3(0.0f, 1.0f, 3.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);

    float yaw = -90.f;
    float pitch;
private:
};
