#include "ui_renderer.h"
#include "engine/asset_manager/shader.h"

void UIRenderer::drawText(
    const UIRenderCommand& cmd,
    const glm::mat4& projection,
    Shader& shader)
{
    cmd.font->draw(
        cmd.text,
        cmd.position,
        cmd.size,
        cmd.color,
        projection,
        shader
    );
}

void UIRenderer::drawRect(
    const UIRenderCommand& cmd,
    const glm::mat4& projection,
    Shader& shader)
{
    shader.use();
    shader.setMat4("uProjection", projection);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(cmd.position, 0.0f));
    model = glm::scale(model, glm::vec3(cmd.size, 1.0f));

    shader.setMat4("uModel", model);
    shader.setVec4("uColor", cmd.color);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void UIRenderer::render(
    const std::vector<UIRenderCommand>& cmds,
    const glm::mat4& projection,
    Shader& textShader,
    Shader& rectShader)
{
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto& cmd : cmds) {
        switch (cmd.type) {
            case UICmdType::Text:
                drawText(cmd, projection, textShader);
                break;
            case UICmdType::Rect:
                drawRect(cmd, projection, rectShader);
                break;
            default:
                break;
        }
    }
}

void UIRenderer::init() {
    initQuad();
}

void UIRenderer::initQuad() {
    float vertices[] = {
        // pos
        0.f, 0.f,
        1.f, 0.f,
        1.f, 1.f,
        0.f, 1.f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}
