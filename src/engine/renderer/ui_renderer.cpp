#include "ui_renderer.h"
#include "engine/asset_manager/shader.h"
#include "engine/engine.h"
#include "engine/profilers/profile_scope.h"

void UIRenderer::render(const glm::mat4& projection) {
    PROFILE_SCOPE("UIRenderer::render");
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float x = 50;
    float y = 50;
    float scale = 1;
    std::string text = "WAZZAAAAAAPPP";
    glm::vec3 color = {1, 1, 0};
    Shader& s = Engine::instance().assets.getShader("glyph");
    Font& font = Engine::instance().font;

    // activate corresponding render state
    s.use();
    s.setMat4("projection", projection);
    s.setVec3("textColor", color);
    s.setInt("text", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(font.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, font.VBO);
    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = font.characters[*c];
        float xpos = x + ch.bearing.x * scale;
        float ypos = y - (ch.size.y - ch.bearing.y) * scale;
        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos, ypos + h, 0.0f, 0.0f },
            { xpos, ypos, 0.0f, 1.0f },
            { xpos + w, ypos, 1.0f, 1.0f },
            { xpos, ypos + h, 0.0f, 0.0f },
            { xpos + w, ypos, 1.0f, 1.0f },
            { xpos + w, ypos + h, 1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, font.VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // advance cursors for next glyph (advance is 1/64 pixels)
        x += (ch.advance >> 6) * scale; // bitshift by 6 (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
};
