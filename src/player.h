#pragma once

#include <glm/glm.hpp>

class Player {
public:
    glm::vec3 pos;
    glm::vec3 velocity;
    bool isGrounded;
    float speed = 1.0f;
    float jumpForce = 3.0f;
private:
};
