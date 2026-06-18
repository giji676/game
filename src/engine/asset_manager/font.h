#pragma once

#include "engine/asset_manager/shader.h"
#include <ft2build.h>
#include <string>
#include FT_FREETYPE_H
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <map>

struct Character {
    unsigned int textureID;
    glm::ivec2 size;
    glm::ivec2 bearing;
    unsigned int advance;
};

class Font {
public:
    GLuint VAO, VBO;

    Font(const char* fontPath);

    void draw(
        const std::string& text,
        float x,
        float y,
        float scale,
        const glm::vec3& color,
        const glm::mat4& projection,
        Shader& shader);
    void loadFont(const char* fontPath);
    std::map<char, Character> characters;

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&&) = delete;
    Font& operator=(Font&&) = delete;

private:
    FT_Face face;
    void setupFontGPU();
};
