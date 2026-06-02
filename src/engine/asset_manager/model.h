#pragma once

#include <cstring>
#include <glad/glad.h>

#include "gj_model/gj_model.h"
#include "material.h"
#include "mesh.h"
#include "asset_manager.h"

typedef struct {
    Mesh mesh;
    Material material;
} SubMesh;

class Model {
public:
    Model(const char *path, AssetManager *assetManager) {
        this->assetManager = assetManager;
        loadModel(path);
    }
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    void draw();
    const std::vector<SubMesh>& getParts() const;

private:
    std::vector<SubMesh> parts;
    std::string directory;
    AssetManager *assetManager;

    void loadModel(std::string path);
    void processNode(struct gjNode *node, struct gjModel *model);
    SubMesh processMesh(struct gjMesh *mesh, struct gjModel *model);
};

