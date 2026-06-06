#include <algorithm>
#include "raycasting.h"
#include "engine.h"
#include "engine/asset_manager/model.h"
#include "glm/geometric.hpp"

RaycastHit Raycasting::castRay() {
    RaycastHit hit;

    checkIntersect(
        Engine::instance().scene.getRoot(),
        glm::mat4(1.0f),
        hit
    );

    return hit;
}

void Raycasting::checkIntersect(
    ObjectID id,
    const glm::mat4& parent,
    RaycastHit& bestHit)
{
    Engine& engine = Engine::instance();
    const Object& obj = engine.scene.get(id);

    glm::vec3 rayOrigin = engine.getActiveCamera()->pos;
    glm::vec3 rayDirection = glm::normalize(engine.getActiveCamera()->front);
    glm::mat4 world = parent * obj.transform.localMatrix();

    Bounds bounds = obj.getBounds();
    glm::vec3 size = bounds.size;
    if (size != glm::vec3(0.0f)) {
        glm::vec3 worldScale(
            glm::length(glm::vec3(world[0])),
            glm::length(glm::vec3(world[1])),
            glm::length(glm::vec3(world[2]))
        );

        float radius =
            std::max({
                size.x * worldScale.x,
                size.y * worldScale.y,
                size.z * worldScale.z
            }) * 0.5f;

        glm::vec3 center =
            glm::vec3(world * glm::vec4(bounds.center, 1.0f));

        float distance;

        if (testRaySphereIntersection(
            rayOrigin,
            rayDirection,
            center,
            radius,
            distance))
        {
            if (distance < bestHit.distance) {
                bestHit.distance = distance;
                bestHit.object = obj.getID();
            }
        }
    }

    for (ObjectID child : obj.children) {
        checkIntersect(child, world, bestHit);
    }
}

bool Raycasting::testRaySphereIntersection(
    const glm::vec3& rayOrigin,
    const glm::vec3& rayDirection,
    const glm::vec3& sphereCenter,
    float radius,
    float& intersectionDistance)
{
    glm::vec3 oc = rayOrigin - sphereCenter;

    float a = glm::dot(rayDirection, rayDirection);
    float b = 2.0f * glm::dot(oc, rayDirection);
    float c = glm::dot(oc, oc) - radius * radius;

    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0.0f) {
        return false;
    }

    float sqrtD = std::sqrt(discriminant);

    float t1 = (-b - sqrtD) / (2.0f * a);
    float t2 = (-b + sqrtD) / (2.0f * a);

    if (t1 >= 0.0f) {
        intersectionDistance = t1;
        return true;
    }

    if (t2 >= 0.0f) {
        intersectionDistance = t2;
        return true;
    }

    return false;
}
