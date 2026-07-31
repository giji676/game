#include "texture.h"

#include "gj_image/gj_image.h"
#include <cstring>
#include <iostream>
#include <vector>

void Texture::genTextureId()
{
    glGenTextures(1, &id);
}

void Texture::flipImageVertically()
{
    if (!data_ || width_ <= 0 || height_ <= 1 || channels_ <= 0)
        return;

    const int stride = width_ * channels_;
    std::vector<unsigned char> row(static_cast<size_t>(stride));
    for (int y = 0; y < height_ / 2; ++y) {
        unsigned char* top = data_ + y * stride;
        unsigned char* bot = data_ + (height_ - 1 - y) * stride;
        std::memcpy(row.data(), top, static_cast<size_t>(stride));
        std::memcpy(top, bot, static_cast<size_t>(stride));
        std::memcpy(bot, row.data(), static_cast<size_t>(stride));
    }
}

bool Texture::loadImage()
{
    // Workers must not touch the global gj_vflip_image flag. Decode as-is,
    // then flip CPU-side when this texture requested it at enqueue time.
    data_ = gj_image_load(fullPath.c_str(), &width_, &height_, &channels_);
    if (!data_) {
        std::cout << "Failed to load texture: " << fullPath << std::endl;
        std::cout << "gj_image error: " << gj_get_last_error() << std::endl;
        return false;
    }

    if (flipY)
        flipImageVertically();

    return true;
}

void Texture::setupImageGPU()
{
    if (!data_) {
        std::cout << "Texture failed to load at path: " << fullPath << std::endl;
        std::cout << "gj_image error: " << gj_get_last_error() << std::endl;
        return;
    }

    GLenum format;
    int textureEdgeConfig = 0;
    if      (channels_ == 1) { format = GL_RED;  textureEdgeConfig = GL_REPEAT; }
    else if (channels_ == 3) { format = GL_RGB;  textureEdgeConfig = GL_REPEAT; }
    else if (channels_ == 4) { format = GL_RGBA; textureEdgeConfig = GL_CLAMP_TO_EDGE; }
    else {
        std::cout << "Unsupported number of channels: " << channels_ << std::endl;
        gj_image_free(data_);
        data_ = nullptr;
        return;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width_, height_, 0, format, GL_UNSIGNED_BYTE, data_);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureEdgeConfig);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureEdgeConfig);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    gj_image_free(data_);
    data_ = nullptr;
}
