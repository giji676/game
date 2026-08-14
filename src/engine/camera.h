#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/icomponent.h"

class Camera : public IComponent {
public:
    glm::vec3 pos = glm::vec3(0.f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    void lateUpdate() override;

    const char* typeName() const override { return "Camera"; }

    glm::mat4 view() const {
        return glm::lookAt(pos, pos + front, up);
    }
};
