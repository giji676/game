#pragma once
#include <glm/glm.hpp>
#include "engine/asset_manager/model.h"

struct Plane {
    glm::vec3 normal;
    float distance;

    float distanceTo(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }
};

struct Frustum {
    Plane planes[6]; // left, right, bottom, top, near, far

    // extract from view-projection matrix
    static Frustum fromMatrix(const glm::mat4& vp) {
        Frustum f;

        // left
        f.planes[0].normal.x = vp[0][3] + vp[0][0];
        f.planes[0].normal.y = vp[1][3] + vp[1][0];
        f.planes[0].normal.z = vp[2][3] + vp[2][0];
        f.planes[0].distance = vp[3][3] + vp[3][0];

        // right
        f.planes[1].normal.x = vp[0][3] - vp[0][0];
        f.planes[1].normal.y = vp[1][3] - vp[1][0];
        f.planes[1].normal.z = vp[2][3] - vp[2][0];
        f.planes[1].distance = vp[3][3] - vp[3][0];

        // bottom
        f.planes[2].normal.x = vp[0][3] + vp[0][1];
        f.planes[2].normal.y = vp[1][3] + vp[1][1];
        f.planes[2].normal.z = vp[2][3] + vp[2][1];
        f.planes[2].distance = vp[3][3] + vp[3][1];

        // top
        f.planes[3].normal.x = vp[0][3] - vp[0][1];
        f.planes[3].normal.y = vp[1][3] - vp[1][1];
        f.planes[3].normal.z = vp[2][3] - vp[2][1];
        f.planes[3].distance = vp[3][3] - vp[3][1];

        // near
        f.planes[4].normal.x = vp[0][3] + vp[0][2];
        f.planes[4].normal.y = vp[1][3] + vp[1][2];
        f.planes[4].normal.z = vp[2][3] + vp[2][2];
        f.planes[4].distance = vp[3][3] + vp[3][2];

        // far
        f.planes[5].normal.x = vp[0][3] - vp[0][2];
        f.planes[5].normal.y = vp[1][3] - vp[1][2];
        f.planes[5].normal.z = vp[2][3] - vp[2][2];
        f.planes[5].distance = vp[3][3] - vp[3][2];

        // normalize planes
        for (auto& p : f.planes) {
            float len = glm::length(p.normal);
            p.normal   /= len;
            p.distance /= len;
        }

        return f;
    }

    // test world-space AABB against frustum
    bool intersectsAABB(const glm::vec3& min, const glm::vec3& max) const {
        for (const Plane& plane : planes) {
            // positive vertex — the corner most in the direction of the plane normal
            glm::vec3 positive = {
                plane.normal.x >= 0 ? max.x : min.x,
                plane.normal.y >= 0 ? max.y : min.y,
                plane.normal.z >= 0 ? max.z : min.z,
            };

            if (plane.distanceTo(positive) < 0)
                return false; // fully outside this plane
        }
        return true;
    }
};

inline void transformAABB(
    const Bounds& local,
    const glm::mat4& world,
    glm::vec3& outMin,
    glm::vec3& outMax)
{
    // transform all 8 corners and refit
    outMin = glm::vec3(FLT_MAX);
    outMax = glm::vec3(-FLT_MAX);

    glm::vec3 corners[8] = {
        {local.min.x, local.min.y, local.min.z},
        {local.max.x, local.min.y, local.min.z},
        {local.min.x, local.max.y, local.min.z},
        {local.max.x, local.max.y, local.min.z},
        {local.min.x, local.min.y, local.max.z},
        {local.max.x, local.min.y, local.max.z},
        {local.min.x, local.max.y, local.max.z},
        {local.max.x, local.max.y, local.max.z},
    };

    for (const auto& c : corners) {
        glm::vec3 wc = glm::vec3(world * glm::vec4(c, 1.0f));
        outMin = glm::min(outMin, wc);
        outMax = glm::max(outMax, wc);
    }
}
