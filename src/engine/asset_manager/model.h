#pragma once

#include <cstring>
#include <glad/glad.h>

#include "gj_model/gj_model.h"
#include "material.h"
#include "mesh.h"
#include "asset_manager.h"

struct Bounds {
    glm::vec3 min;
    glm::vec3 max;
    glm::vec3 center;
    glm::vec3 size;
};

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
    std::vector<SubMesh>& getParts();
    // TODO: cache size during model load (should be static not changing)
    Bounds getBounds();

private:
    std::vector<SubMesh> parts;
    std::string directory;
    AssetManager *assetManager;

    void loadModel(std::string path);
    void processNode(struct gjNode *node, struct gjModel *model);
    SubMesh processMesh(struct gjMesh *mesh, struct gjModel *model);
};

