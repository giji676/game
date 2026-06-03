#pragma once

#include <glm/glm.hpp>

#include "engine/asset_manager/material.h"
#include "engine/asset_manager/mesh.h"
#include "engine/asset_manager/object.h"

typedef struct {
    const Mesh* mesh;
    const Material* material;
    glm::mat4 model;
} RenderCommand;

class Renderer {
public:
    void submit(const Object& object, const glm::mat4& worldMatrix);
    void render(const glm::mat4& view, const glm::mat4& projection);

private:
    std::vector<RenderCommand> queue;
};
