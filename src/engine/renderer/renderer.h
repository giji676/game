#pragma once

#include <glm/glm.hpp>

#include "engine/asset_manager/material.h"
#include "engine/asset_manager/mesh.h"
#include "engine/asset_manager/object.h"

typedef struct {
    Mesh* mesh;
    Material* material;
    glm::mat4 model;
} RenderCommand;

class Renderer {
public:
    void draw(const Object &object);
};
