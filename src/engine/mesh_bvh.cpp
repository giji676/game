#include "engine/mesh_bvh.h"
#include "engine/engine.h"
#include "engine/profilers/profile_scope.h"
#include "engine/raycasting.h"
#include "asset_manager/model.h"
#include <algorithm>

void MeshBVH::build(Model& model) {
    triangles.clear();
    nodes.clear();

    nodesUsed = 1;
    rootNodeIdx = 0;

    for (const auto& sm : model.getParts()) {
        const auto& mesh = sm.mesh;

        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            uint32_t i0 = mesh.indices[i + 0];
            uint32_t i1 = mesh.indices[i + 1];
            uint32_t i2 = mesh.indices[i + 2];

            const glm::vec3& v0 = mesh.vertices[i0].Position;
            const glm::vec3& v1 = mesh.vertices[i1].Position;
            const glm::vec3& v2 = mesh.vertices[i2].Position;

            Triangle t;
            t.v0 = v0;
            t.v1 = v1;
            t.v2 = v2;
            t.centroid = (v0 + v1 + v2) * 0.3333f;

            triangles.push_back(t);
        }
    }

    nodes.resize(triangles.size() * 2 - 1);

    MeshBVHNode& root = nodes[rootNodeIdx];
    root.leftChild = root.rightChild = 0;
    root.firstPrim = 0;
    root.primCount = triangles.size();

    updateNodeBounds(rootNodeIdx);
    subdivide(rootNodeIdx);
}

void MeshBVH::updateNodeBounds(uint32_t nodeIdx) {
    MeshBVHNode& node = nodes[nodeIdx];

    node.aabbMin = glm::vec3(1e30f);
    node.aabbMax = glm::vec3(-1e30f);

    for (uint32_t i = 0; i < node.primCount; i++) {
        const Triangle& tri = triangles[node.firstPrim + i];

        glm::vec3 triMin = glm::min(tri.v0, glm::min(tri.v1, tri.v2));
        glm::vec3 triMax = glm::max(tri.v0, glm::max(tri.v1, tri.v2));

        node.aabbMin = glm::min(node.aabbMin, triMin);
        node.aabbMax = glm::max(node.aabbMax, triMax);
    }
}

void MeshBVH::subdivide(uint32_t nodeIdx) {
    MeshBVHNode& node = nodes[nodeIdx];

    if (node.primCount <= 2)
        return;

    glm::vec3 extent = node.aabbMax - node.aabbMin;

    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;

    std::nth_element(
            triangles.begin() + node.firstPrim,
            triangles.begin() + node.firstPrim + node.primCount / 2,
            triangles.begin() + node.firstPrim + node.primCount,
            [&](const Triangle& a, const Triangle& b)
            {
            return a.centroid[axis] < b.centroid[axis];
            }
            );

    float splitPos =
        triangles[node.firstPrim + node.primCount / 2].centroid[axis];

    uint32_t i = node.firstPrim;
    uint32_t j = node.firstPrim + node.primCount - 1;

    while (i <= j) {
        if (triangles[i].centroid[axis] < splitPos)
            i++;
        else
            std::swap(triangles[i], triangles[j--]);
    }

    uint32_t leftCount = i - node.firstPrim;

    if (leftCount == 0 || leftCount == node.primCount)
        return;

    uint32_t leftChild = nodesUsed++;
    uint32_t rightChild = nodesUsed++;

    nodes[leftChild].firstPrim = node.firstPrim;
    nodes[leftChild].primCount = leftCount;

    nodes[rightChild].firstPrim = i;
    nodes[rightChild].primCount = node.primCount - leftCount;

    node.leftChild = leftChild;
    node.rightChild = rightChild;

    nodes[leftChild].leftChild = nodes[leftChild].rightChild = 0;
    nodes[rightChild].leftChild = nodes[rightChild].rightChild = 0;

    node.primCount = 0;

    updateNodeBounds(leftChild);
    updateNodeBounds(rightChild);

    subdivide(leftChild);
    subdivide(rightChild);
}

void MeshBVH::intersectBVH(Ray& ray, uint32_t nodeIdx, const glm::mat4& world) {
    PROFILE_SCOPE("MeshBVH::intersectBVH");
    _intersectBVH(ray, nodeIdx, world);
}

void MeshBVH::_intersectBVH(Ray& ray, uint32_t nodeIdx, const glm::mat4& world) {
    const MeshBVHNode& node = nodes[nodeIdx];
    if (!intersectAABB(ray, node.aabbMin, node.aabbMax, world))
        return;

    Engine::instance().debugRenderer.aabb(
        world,
        node.aabbMin,
        node.aabbMax,
        {1,0,0}
    );

    if (node.primCount > 0) {
        PROFILE_SCOPE("TriangleIntersection");
        for (uint32_t i = 0; i < node.primCount; i++) {
            const Triangle& tri = triangles[node.firstPrim + i];

            // TODO: top using triangle3, use Triangle instaed
            // so no need to construct a new triangle3
            triangle3 tri3{
                tri.v0,
                    tri.v1,
                    tri.v2
            };
            auto hit = Raycasting::testTriangleIntersection(ray, tri3);

            if (hit && hit->t < ray.t)
                ray.t = hit->t;
        }
        return;
    }

    _intersectBVH(ray, node.leftChild, world);
    _intersectBVH(ray, node.rightChild, world);
}

bool MeshBVH::intersectAABB(
        const Ray& ray,
        const glm::vec3& bmin,
        const glm::vec3& bmax,
        const glm::mat4& world)
{
    PROFILE_SCOPE("MeshBVH::intersectAABB");
    glm::vec3 invD = 1.0f / ray.direction;

    glm::vec3 t0 = (bmin - ray.origin) * invD;
    glm::vec3 t1 = (bmax - ray.origin) * invD;

    glm::vec3 tmin3 = glm::min(t0, t1);
    glm::vec3 tmax3 = glm::max(t0, t1);

    float tmin = glm::max(glm::max(tmin3.x, tmin3.y), tmin3.z);
    float tmax = glm::min(glm::min(tmax3.x, tmax3.y), tmax3.z);

    return (tmax >= glm::max(tmin, 0.0f)) && (tmin <= ray.t);
}
