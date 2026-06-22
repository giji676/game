#pragma once

#include <glm/glm.hpp>

#include "engine/asset_manager/material.h"
#include "engine/asset_manager/mesh.h"
#include "engine/asset_manager/mesh_registry.h"

struct RenderCommand {
    const Mesh* mesh;
    const Material* material;
    glm::mat4 model;
    uint64_t sortKey;
    const MeshAllocation* allocation;
};

struct DrawElementsIndirectCommand {
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t  baseVertex;
    uint32_t baseInstance;
};

class Renderer {
public:
    void init(MeshRegistry* registry);
    void render(
        std::vector<RenderCommand>& queue,
        const glm::mat4& view,
        const glm::mat4& projection);
    void cleanup();

private:
    MeshRegistry* meshRegistry = nullptr;
    // GLuint transformSSBO = 0;
    // GLuint indirectBuffer = 0;
    std::vector<glm::mat4> transformData;
    std::vector<DrawElementsIndirectCommand> commands;
    std::vector<uint32_t> sortedIndices;

    static const uint32_t MAX_OBJECTS = 100000;
    static const uint32_t NUM_BUFFERS = 2;
    
    GLuint transformSSBO[NUM_BUFFERS];
    GLuint indirectBuffer[NUM_BUFFERS];
    uint32_t currentBuffer = 0;
};
