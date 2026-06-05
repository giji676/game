#include <cfloat>

#include "model.h"

Bounds Model::getBounds() {
    glm::vec3 min(FLT_MAX);
    glm::vec3 max(-FLT_MAX);

    for (const SubMesh& part : parts) {
        for (const Vertex& v : part.mesh.vertices) {
            min = glm::min(min, v.Position);
            max = glm::max(max, v.Position);
        }
    }

    Bounds b;
    b.min = min;
    b.max = max;
    b.center = (min + max) * 0.5f;
    b.size = max - min;

    return b;
}
void Model::draw() {
    for (SubMesh& part : parts) {
        part.material.bind();
        part.mesh.draw();
    }
}

void Model::loadModel(std::string path) {
    struct gjModel *gjModel = gj_model_load(path.c_str());
    if (!gjModel) {
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));
    processNode(&gjModel->root, gjModel);
}

void Model::processNode(struct gjNode *node, struct gjModel *model) {
    for (int i = 0; i < node->meshCount; i++) {
        int meshIndex = node->meshIndices[i];
        struct gjMesh *mesh = &model->meshes[meshIndex];
        parts.push_back(processMesh(mesh, model));
    }
    for (int i = 0; i < node->childCount; i++) {
        processNode(&node->children[i], model);
    }
}

SubMesh Model::processMesh(struct gjMesh *mesh, struct gjModel *model) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    glm::vec3 diffuseFallback(1.0f);
    glm::vec3 specularFallback(0.0f);
    for (int i = 0; i < mesh->vertexCount; i++) {
        Vertex vertex;
        glm::vec3 vector;
        vector.x = mesh->positions[i * 3 + 0];
        vector.y = mesh->positions[i * 3 + 1];
        vector.z = mesh->positions[i * 3 + 2];
        vertex.Position = vector;
        vector.x = mesh->normals[i * 3 + 0];
        vector.y = mesh->normals[i * 3 + 1];
        vector.z = mesh->normals[i * 3 + 2];
        vertex.Normal = vector;
        if (mesh->texcoords) {
            glm::vec2 vec;
            vec.x = mesh->texcoords[i * 2 + 0];
            vec.y = mesh->texcoords[i * 2 + 1];
            vertex.TexCoords = vec;
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }
        vertices.push_back(vertex);
    }
    for (int i = 0; i < mesh->indexCount; i++) {
        indices.push_back(mesh->indices[i]);
    }
    Texture* diffuseMap = nullptr;
    Texture* specularMap = nullptr;
    if (mesh->materialIndex >= 0) {
        struct gjMaterial material = model->materials[mesh->materialIndex];
        Texture* dtex = &assetManager->loadTexture(
            material.diffuseMap,   // name (can be full path)
            directory + "/" + material.diffuseMap,
            "diffuseMap"
        );
        diffuseMap = dtex;
        Texture* stex = &assetManager->loadTexture(
            material.specularMap,
            directory + "/" + material.specularMap,
            "specularMap"
        );
        specularMap = stex;
        diffuseFallback = glm::vec3(
            material.diffuse[0],
            material.diffuse[1],
            material.diffuse[2]
        );

        specularFallback = glm::vec3(
            material.specular[0],
            material.specular[1],
            material.specular[2]
        );
    } else {
        struct gjMaterial material = model->materials[0];
        Texture* dtex = &assetManager->loadTexture(
            material.diffuseMap,
            directory + "/" + material.diffuseMap,
            "diffuseMap"
        );
        diffuseMap = dtex;
        Texture* stex = &assetManager->loadTexture(
            material.specularMap,
            directory + "/" + material.specularMap,
            "specularMap"
        );
        specularMap = stex;
        diffuseFallback = glm::vec3(
            material.diffuse[0],
            material.diffuse[1],
            material.diffuse[2]
        );

        specularFallback = glm::vec3(
            material.specular[0],
            material.specular[1],
            material.specular[2]
        );
    }

    Mesh myMesh = Mesh(vertices, indices);
    myMesh.id = assetManager->allocateMeshId();
    Material material;
    material.diffuseFallback = diffuseFallback;
    material.specularFallback = specularFallback;
    material.shader = &assetManager->getShader("textured_mat");
    material.id = assetManager->allocateMaterialId();

    if (diffuseMap)
        material.textures.push_back(diffuseMap);

    if (specularMap)
        material.textures.push_back(specularMap);

    SubMesh subMesh = {
        .mesh = myMesh,
        .material = material,
    };

    return subMesh;
}

std::vector<SubMesh>& Model::getParts() {
    return parts;
}
