#pragma once

#include <glad/glad.h>
#include <string>

/*
* Texture(...) call sets up the object, with the id of the OpenGl texture object.
* But the image data is not loaded into memory yet.
* loadImage() can then be called in parallel context to load the image data into memory.
* setupImageGPU() should then be called in non-parallel context to upload the image data
 */

class Texture {
public:
    Texture() = default;
    explicit Texture(
        const std::string& fullPath,
        const std::string type,
        bool flipY = false)
    {
        genTextureId();
        this->fullPath = fullPath;
        this->type = type;
        this->flipY = flipY;
    }

    bool loadImage();
    /*
     * Should only be called in non-paralel context.
     * Image data should already be loaded in memory.
     */
    void setupImageGPU();

    GLuint id = 0;
    std::string type;
    std::string fullPath;
    bool flipY = false;

private:
    unsigned char* data_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
    void genTextureId();
    void flipImageVertically();
};
