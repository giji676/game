#pragma once

#include <glad/glad.h>
#include <string>

class Texture {
public:
    GLuint id;

    std::string type;
    std::string path;
};
