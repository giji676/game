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

enum class TextAlignV {
    Top,
    Center,
    Bottom,
};

class Font {
public:
    GLuint VAO, VBO;
    FontMetrics metrics;
    float referenceSize = 48.f;

    std::map<char, Character> characters;

    Font(const char* fontPath);

    void draw(
        const std::string& text,
        const glm::vec2 position,
        const glm::vec2 size,
        const glm::vec3& color,
        const glm::mat4& projection,
        Shader& shader);

    glm::vec2 measure(const std::string& text, float size) const;

    float scaleFor(float size) const { return size / referenceSize; }
    float lineHeightAt(float size) const { return metrics.lineHeight * scaleFor(size); }
    float descenderAt(float size) const { return metrics.descender * scaleFor(size); }
    float ascenderAt(float size) const { return metrics.ascender * scaleFor(size); }

    glm::vec2 baselineInRect(
        glm::vec2 rectPos,
        glm::vec2 rectSize,
        glm::vec2 padding,
        float size,
        TextAlignV align = TextAlignV::Center) const;

    struct CaretRect {
        glm::vec2 position;
        glm::vec2 size;
    };
    CaretRect caretAt(glm::vec2 baseline, float xOffset, float size, float width = 2.f) const;

    void loadFont(const char* fontPath);

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&&) = delete;
    Font& operator=(Font&&) = delete;

private:
    FT_Face face;
    void setupFontGPU();
};
