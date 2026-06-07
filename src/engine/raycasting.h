#pragma once

#include <glm/glm.hpp>
#include <optional>
#include "defines.h"

#define INVALID_OBJECT_ID 0

typedef struct {
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;
} triangle3;

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

    bool testSphereIntersection(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDirection,
        const glm::vec3& sphereCenter,
        float radius,
        float& intersectionDistance);

    std::optional<glm::vec3> testTriangleIntersection(const glm::vec3& ray_origin,
                                                      const glm::vec3 &ray_vector,
                                                      const triangle3& triangle);
};
