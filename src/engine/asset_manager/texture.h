#pragma once

#include <glad/glad.h>
#include <iostream>
#include <string>

#include "gj_image/gj_image.h"

inline unsigned int TextureFromFile(const char *fullPath);

class Texture {
public:
    Texture() = default;
    explicit Texture(
        const std::string& fullPath,
        const std::string type)
    {
        id = TextureFromFile(fullPath.c_str());
        this->fullPath = fullPath;
        this->type = type;
    }

    GLuint id = 0;
    std::string type;
    std::string fullPath;
};

inline unsigned int TextureFromFile(const char *fullPath) {
    std::string filename = std::string(fullPath);

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
        std::cout << "Texture failed to load at path: " << fullPath << std::endl;
        std::cout << "gj_image error: " << gj_get_last_error() << std::endl;
        gj_image_free(data);
        return 0;
    }

    return textureID;
}
