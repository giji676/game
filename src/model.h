#include <cstring>
#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

#include "shader.h"
#include "mesh.h"
#include "gj_model/gj_model.h"
#include "gj_image/gj_image.h"

using namespace std;


unsigned int TextureFromFile(const char *path, const string &directory);

class Model
{
public:
    Model(char *path) {
        loadModel(path);
    }
    void Draw(Shader &shader) {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

private:
    // model data
    vector<Texture> textures_loaded;
    vector<Mesh> meshes;
    string directory;
    void loadModel(string path) {
        struct gjModel *gjModel = gj_model_load(path.c_str());
        if (!gjModel) {
            cout << "ERROR::gj-model::" << endl;
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));
        processNode(&gjModel->root, gjModel);
    }
    void processNode(struct gjNode *node, struct gjModel *model) {
        for (int i = 0; i < node->meshCount; i++) {
            int meshIndex = node->meshIndices[i];
            struct gjMesh *mesh = &model->meshes[meshIndex];
            meshes.push_back(processMesh(mesh, model));
        }
        for (int i = 0; i < node->childCount; i++) {
            processNode(&node->children[i], model);
        }
    }

    Mesh processMesh(struct gjMesh *mesh, struct gjModel *model) {
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

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

        Mesh result(vertices, indices, textures);
        result.diffuseFallback = diffuseFallback;
        result.specularFallback = specularFallback;

        return result;
    }

    void loadMaterialTextures(
        const char *filename,
        string typeName,
        vector<Texture> *textures) {

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
};

unsigned int TextureFromFile(const char *path, const string &directory) {
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, channels = 0;
    unsigned char *data = gj_image_load(filename.c_str(), &width, &height, &channels);

    if (data) {
        GLenum format;
        int textureEdgeConfig = 0;
        if      (channels == 1) { format = GL_RED;  textureEdgeConfig = GL_REPEAT; }
        else if (channels == 3) { format = GL_RGB;  textureEdgeConfig = GL_REPEAT; }
        else if (channels == 4) { format = GL_RGBA; textureEdgeConfig = GL_CLAMP_TO_EDGE; }
        else {
            std::cout << "Unsupported number of channels: " << channels << std::endl;
            gj_image_free(data);
            return 0;
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureEdgeConfig);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureEdgeConfig);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        gj_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        std::cout << "gj_image error: " << gj_get_last_error() << std::endl;
        gj_image_free(data);
        return 0;
    }

    return textureID;
}
