#include "ui_renderer.h"
#include "engine/asset_manager/shader.h"
#include <iostream>

namespace {
void setClipRect(const UIRenderCommand& cmd) {
    if (!cmd.clip) return;
    glEnable(GL_SCISSOR_TEST);
    glScissor(
        static_cast<GLint>(cmd.clipPos.x),
        static_cast<GLint>(cmd.clipPos.y),
        static_cast<GLsizei>(cmd.clipSize.x),
        static_cast<GLsizei>(cmd.clipSize.y));
}

void clearClipRect(const UIRenderCommand& cmd) {
    if (!cmd.clip) return;
    glDisable(GL_SCISSOR_TEST);
}

RectInstance toRectInstance(const UIRenderCommand& cmd) {
    return {
        cmd.position, cmd.size, cmd.color,
        cmd.borderColor, cmd.cornerRadii, cmd.borderWidth
    };
}
} // namespace

void UIRenderer::drawText(
    const UIRenderCommand& cmd,
    const glm::mat4& projection,
    Shader& shader)
{
    setClipRect(cmd);
    cmd.font->draw(
        cmd.text,
        cmd.position,
        cmd.size,
        cmd.color,
        projection,
        shader
    );
    clearClipRect(cmd);
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

    std::vector<RectInstance> batch;
    batch.reserve(MAX_INSTANCES);

    auto flush = [&]() {
        if (batch.empty()) return;
        flushRectBatch(batch, projection, rectShader);
        batch.clear();
    };

    for (const auto& cmd : cmds) {
        if (cmd.type == UICmdType::Rect) {
            if (cmd.clip) {
                flush();
                setClipRect(cmd);
                flushRectBatch({ toRectInstance(cmd) }, projection, rectShader);
                clearClipRect(cmd);
            } else {
                batch.push_back({
                    cmd.position, cmd.size, cmd.color,
                    cmd.borderColor, cmd.cornerRadii, cmd.borderWidth
                });
                if (batch.size() == MAX_INSTANCES) flush();
            }
        } else if (cmd.type == UICmdType::Text) {
            flush();
            drawText(cmd, projection, textShader);
        }
    }
    flush();
}

void UIRenderer::flushRectBatch(
    const std::vector<RectInstance>& batch,
    const glm::mat4& projection,
    Shader& shader)
{
    shader.use();
    shader.setMat4("uProjection", projection);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, batch.size() * sizeof(RectInstance), batch.data());

    glBindVertexArray(quadVAO);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, (GLsizei)batch.size());
    glBindVertexArray(0);
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
    glGenBuffers(1, &instanceVBO);

    glBindVertexArray(quadVAO);

    // per-vertex (unit quad corners)
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);

    // per-instance
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * sizeof(RectInstance), nullptr, GL_DYNAMIC_DRAW);

    auto setAttr = [](GLuint idx, GLint count, size_t offset) {
        glEnableVertexAttribArray(idx);
        glVertexAttribPointer(idx, count, GL_FLOAT, GL_FALSE,
                               sizeof(RectInstance), (void*)offset);
        glVertexAttribDivisor(idx, 1);
    };
    setAttr(1, 2, offsetof(RectInstance, pos));
    setAttr(2, 2, offsetof(RectInstance, size));
    setAttr(3, 4, offsetof(RectInstance, color));
    setAttr(4, 4, offsetof(RectInstance, borderColor));
    setAttr(5, 4, offsetof(RectInstance, cornerRadii));
    setAttr(6, 1, offsetof(RectInstance, borderWidth));

    glBindVertexArray(0);
}
