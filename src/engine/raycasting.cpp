#include <algorithm>
#include "raycasting.h"
#include "engine.h"
#include "engine/asset_manager/model.h"
#include "engine/profilers/profile_scope.h"
#include "glm/geometric.hpp"

RaycastHit Raycasting::castRay() {
    PROFILE_SCOPE("Raycasting::castRay");
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

    Ray ray = Ray {
        .origin = engine.getActiveCamera()->pos,
        .direction = glm::normalize(engine.getActiveCamera()->front),
    };

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

        if (testSphereIntersection(
            ray,
            center,
            radius,
            distance))
        {
            // TODO: stop doing inverse (too expensive)
            // cache world/local mats instead
            glm::mat4 invWorld = glm::inverse(world);
            Ray localRay;
            localRay.origin = glm::vec3(invWorld * glm::vec4(ray.origin, 1.0f));
            localRay.direction = glm::normalize(
                    glm::vec3(invWorld * glm::vec4(ray.direction, 0.0f))
                    );

            obj.model->bvh.intersectBVH(
                    localRay,
                    obj.model->bvh.rootNodeIdx
                    );

            if (localRay.t < bestHit.distance) {
                bestHit.distance = localRay.t;
                bestHit.object = obj.getID();
            }
        }
    }

    for (ObjectID child : obj.children) {
        checkIntersect(child, world, bestHit);
    }
}

bool Raycasting::testSphereIntersection(
        const Ray& ray,
        const glm::vec3& sphereCenter,
        float radius,
        float& intersectionDistance)
{
    glm::vec3 oc = ray.origin - sphereCenter;

    float a = glm::dot(ray.direction, ray.direction);
    float b = 2.0f * glm::dot(oc, ray.direction);
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

std::optional<TriangleHit> Raycasting::testTriangleIntersection(
        const Ray& ray,
        const triangle3& triangle)
{
    // From https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
    constexpr float epsilon = std::numeric_limits<float>::epsilon();

    glm::vec3 edge1 = triangle.b - triangle.a;
    glm::vec3 edge2 = triangle.c - triangle.a;

    // Backface culling, assuming CCW-wound triangles.
    const glm::vec3 normal = cross(edge1, edge2); // No need to normalize
    if (dot(normal, ray.direction) > 0) return {};

    glm::vec3 ray_cross_e2 = cross(ray.direction, edge2);
    float det = dot(edge1, ray_cross_e2);

    if (abs(det) < epsilon) return {}; // Ray is parallel to triangle

    float inv_det = 1.0 / det;
    glm::vec3 s = ray.origin - triangle.a;
    float u = inv_det * dot(s, ray_cross_e2);

    if (u < -epsilon || u - 1 > epsilon) return {}; // Ray passes outside edge2's bounds

    glm::vec3 s_cross_e1 = cross(s, edge1);
    float v = inv_det * dot(ray.direction, s_cross_e1);

    if (v < -epsilon || u + v - 1 > epsilon) return {}; // Ray passes outside edge1's bounds

    // The ray line intersects with the triangle.
    // We compute t to find where on the ray the intersection is.
    float t = inv_det * dot(edge2, s_cross_e1);

    if (t > epsilon) // Ray intersection
        return TriangleHit{t, ray.origin + ray.direction * t, normal};
    else // This means that there is a line intersection but not a ray intersection.
        return {};
}
