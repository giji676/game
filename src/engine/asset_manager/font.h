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

struct FontMetrics {
    float ascender;
    float descender;
    float lineHeight;
};

class Font {
public:
    GLuint VAO, VBO;
    FontMetrics metrics;

    std::map<char, Character> characters;

    Font(const char* fontPath);

    void draw(
        const std::string& text,
        const glm::vec2 position,
        const glm::vec2 size,
        const glm::vec3& color,
        const glm::mat4& projection,
        Shader& shader);

    glm::vec2 measure(const std::string& text, float height);
    void loadFont(const char* fontPath);

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&&) = delete;
    Font& operator=(Font&&) = delete;

private:
    FT_Face face;
    void setupFontGPU();
};
