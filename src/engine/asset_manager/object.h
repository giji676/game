#pragma once

#include <glm/glm.hpp>

#include "model.h"

typedef struct Transform {
    glm::vec3 position{0.f};
    glm::vec3 rotation{0.f};
    glm::vec3 scale{1.f};
} Transform;

class Object {
public:
    Model *model;
    Transform transform;
};
