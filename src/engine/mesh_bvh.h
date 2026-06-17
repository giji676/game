#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "raycasting.h"

// https://jacco.ompf2.com/2022/04/13/How-to-build-a-bvh-part-1-basics/

class Model;

struct MeshBVHNode {
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;

    uint32_t leftChild;
    uint32_t rightChild;

    uint32_t firstPrim;
    uint32_t primCount;
};

struct Triangle {
    glm::vec3 v0, v1, v2;
    glm::vec3 centroid;
};

class MeshBVH {
public:
    std::vector<MeshBVHNode> nodes;

    uint32_t rootNodeIdx = 0;
    uint32_t nodesUsed = 1;

    void build(Model& model);
    void intersectBVH(Ray& ray, uint32_t nodeIdx);

private:
    std::vector<Triangle> triangles;

    void updateNodeBounds(uint32_t nodeIdx);
    void subdivide(uint32_t nodeIdx);

    void _intersectBVH(Ray& ray, uint32_t nodeIdx);
    static bool intersectAABB(
        const Ray& ray,
        const glm::vec3& bmin,
        const glm::vec3& bmax);
};
