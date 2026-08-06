#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 pos = glm::vec3(0.f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);

    float yaw = -90.f;
    float pitch;

    glm::mat4 view() const {
        return glm::lookAt(pos, pos + front, up);
    }
};
