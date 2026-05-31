#pragma once

#include <cstring>
#include <glad/glad.h>
#include <iostream>

#include "gj_model/gj_model.h"
#include "gj_image/gj_image.h"
#include "material.h"
#include "mesh.h"

unsigned int TextureFromFile(const char *path, const std::string &directory);

typedef struct {
    Mesh mesh;
    Material material;
} SubMesh;

class Model {
public:
    Model(char *path) {
        loadModel(path);
    }
    void draw();

private:
    // model data
    std::vector<SubMesh> parts;

    std::vector<Texture> textures_loaded;
    std::string directory;

    void loadModel(std::string path);
    void processNode(struct gjNode *node, struct gjModel *model);
    SubMesh processMesh(struct gjMesh *mesh, struct gjModel *model);
    void loadMaterialTextures(
        const char *filename,
        std::string typeName,
        std::vector<Texture> *textures);
};

inline unsigned int TextureFromFile(const char *path, const std::string &directory) {
    std::string filename = std::string(path);
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
