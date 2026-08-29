#pragma once

#include <cstdint>
#include <glm/glm.hpp>

#define INVALID_UI_ELEMENT -1
#define INVALID_OBJECT -1

using ObjectID = int64_t;
using UIElementID = int64_t;

constexpr float PI = 3.14159265358979323846f;

enum class GizmoSpace {
    Local,
    World,
};

struct Triangle {
    glm::vec3 v0, v1, v2;
    glm::vec3 centroid;
};

