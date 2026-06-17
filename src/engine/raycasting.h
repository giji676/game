#pragma once

#include <cfloat>
#include <glm/glm.hpp>
#include <optional>
#include "defines.h"

#define INVALID_OBJECT_ID 0

struct triangle3 {
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;
};

struct RaycastHit {
    ObjectID object = INVALID_OBJECT_ID;
    float distance = FLT_MAX;
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    float t = FLT_MAX;
};

struct TriangleHit {
    float t;
    glm::vec3 position;
    glm::vec3 normal;
};

class Raycasting {
public:
    RaycastHit castRay();

    void checkIntersect(
        ObjectID id,
        const glm::mat4& parent,
        RaycastHit& bestHit);

    static bool testSphereIntersection(
        const Ray& ray,
        const glm::vec3& sphereCenter,
        float radius,
        float& intersectionDistance);

    static std::optional<TriangleHit> testTriangleIntersection(
        const Ray& ray,
        const triangle3& triangle);
};
