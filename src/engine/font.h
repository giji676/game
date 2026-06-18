#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/glm.hpp>
#include <map>

struct Character {
    unsigned int textureID;
    glm::ivec2 size;
    glm::ivec2 bearing;
    unsigned int advance;
};

class Font {
public:
    unsigned int VAO, VBO;

    void loadFont(const char* fontPath);
    std::map<char, Character> characters;
    void setupFontGPU();


private:
    FT_Face face;
};
