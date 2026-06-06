#pragma once

#include <glm/glm.hpp>
#include "defines.h"

#define INVALID_OBJECT_ID 0

struct RaycastHit {
    ObjectID object = INVALID_OBJECT_ID;
    float distance = FLT_MAX;
};

class Raycasting {
public:
    RaycastHit castRay();

    void checkIntersect(
        ObjectID id,
        const glm::mat4& parent,
        RaycastHit& bestHit);

    bool testRaySphereIntersection(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDirection,
        const glm::vec3& sphereCenter,
        float radius,
        float& intersectionDistance);
};
