#include "model.h"

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
    if (mesh->materialIndex >= 0) {
        struct gjMaterial material = model->materials[mesh->materialIndex];
        loadMaterialTextures(material.diffuseMap, "texture_diffuse", &textures);
        loadMaterialTextures(material.specularMap, "texture_specular", &textures);
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
        loadMaterialTextures(material.diffuseMap, "texture_diffuse", &textures);
        loadMaterialTextures(material.specularMap, "texture_specular", &textures);
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
    Material material;
    material.diffuseFallback = diffuseFallback;
    material.specularFallback = specularFallback;

    SubMesh subMesh = {
        .mesh = myMesh,
        .material = material,
    };

    return subMesh;
}

void Model::loadMaterialTextures(
    const char *filename,
    std::string typeName,
    std::vector<Texture> *textures) {

    if (!filename || filename[0] == '\0') return;

    for (unsigned int j = 0; j < textures_loaded.size(); j++) {
        if (std::strcmp(textures_loaded[j].path.c_str(), filename) == 0) {
            textures->push_back(textures_loaded[j]);
            return;
        }
    }
    Texture texture;
    texture.id = TextureFromFile(filename, directory);
    texture.type = typeName;
    texture.path = filename;
    textures->push_back(texture);
    textures_loaded.push_back(texture);
}
