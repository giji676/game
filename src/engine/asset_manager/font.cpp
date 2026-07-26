#include "font.h"
#include <algorithm>
#include <iostream>
#include <glad/glad.h>

glm::vec2 Font::measure(const std::string& text, float size) const {
    float scale = scaleFor(size);

    float width = 0.f;
    for (char c : text) {
        const Character& ch = characters.at(c);
        width += (ch.advance >> 6) * scale;
    }

    return { width, size };
}

size_t Font::caretIndexAt(const std::string& text, float x, float size) const {
    if (text.empty())
        return 0;

    float scale = scaleFor(size);
    float width = 0.f;

    for (size_t i = 0; i < text.size(); ++i) {
        const Character& ch = characters.at(text[i]);
        float advance = (ch.advance >> 6) * scale;
        if (x < width + advance * 0.5f)
            return i;
        width += advance;
    }

    return text.size();
}

glm::vec2 Font::baselineInRect(
    glm::vec2 rectPos,
    glm::vec2 rectSize,
    glm::vec2 padding,
    float size,
    TextAlignV align) const
{
    float innerHeight = rectSize.y - padding.y * 2.f;
    float lineH = lineHeightAt(size);
    float descender = descenderAt(size);

    float y;
    switch (align) {
        case TextAlignV::Bottom:
            y = rectPos.y + padding.y + descender;
            break;
        case TextAlignV::Top:
            y = rectPos.y + rectSize.y - padding.y - ascenderAt(size);
            break;
        case TextAlignV::Center:
        default:
            y = rectPos.y + padding.y + (innerHeight - lineH) * 0.5f + descender;
            break;
    }

    return { rectPos.x + padding.x, y };
}

float Font::horizontalScrollOffset(
    const std::string& text,
    size_t caretPos,
    float innerWidth,
    float size,
    float currentScroll) const
{
    float textWidth = measure(text, size).x;
    if (textWidth <= innerWidth)
        return 0.f;

    float maxScroll = textWidth - innerWidth;
    float caretX = measure(text.substr(0, caretPos), size).x;

    float scroll = currentScroll;
    if (caretX < scroll)
        scroll = caretX;
    else if (caretX > scroll + innerWidth)
        scroll = caretX - innerWidth;

    return std::clamp(scroll, 0.f, maxScroll);
}

Font::CaretRect Font::caretAt(
    glm::vec2 baseline,
    float xOffset,
    float size,
    float width) const
{
    return {
        { baseline.x + xOffset, baseline.y - descenderAt(size) },
        { width, lineHeightAt(size) },
    };
}

void Font::draw(
    const std::string& text,
    const glm::vec2 position,
    const glm::vec2 size,
    const glm::vec3& color,
    const glm::mat4& projection,
    Shader& shader)
{
    shader.use();
    shader.setMat4("projection", projection);
    shader.setVec3("textColor", color);
    shader.setInt("text", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    float scale = size.y / referenceSize;
    float x = position.x;

    for (char c : text) {
        const Character& ch = characters[c];

        float xpos = x + ch.bearing.x * scale;
        float ypos = position.y - (ch.size.y - ch.bearing.y) * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos,     ypos,     0.0f, 1.0f },
            { xpos + w, ypos,     1.0f, 1.0f },

            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos + w, ypos,     1.0f, 1.0f },
            { xpos + w, ypos + h, 1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.textureID);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (ch.advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Font::Font(const char* fontPath) {
    loadFont(fontPath);
}

void Font::loadFont(const char* fontPath) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" <<
            std::endl;

    if (FT_New_Face(ft, fontPath, 0, &face))
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

    FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(referenceSize));

    metrics.ascender   =  (face->size->metrics.ascender  >> 6);
    metrics.descender  = -(face->size->metrics.descender >> 6);
    metrics.lineHeight =  (face->size->metrics.height    >> 6);

    setupFontGPU();

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void Font::setupFontGPU() {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char c = 0; c < 128; c++) {
        // load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
                );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        characters.insert(std::pair<char, Character>(c, character));
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6*4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
