#include "ui_renderer.h"
#include "engine/asset_manager/shader.h"
#include <algorithm>
#include <iostream>

namespace {
bool hasBaseScissor(const glm::vec4& base) {
    return base.z > 0.f && base.w > 0.f;
}

void applyScissor(glm::vec2 pos, glm::vec2 size) {
    glScissor(
        static_cast<GLint>(pos.x),
        static_cast<GLint>(pos.y),
        static_cast<GLsizei>(size.x),
        static_cast<GLsizei>(size.y));
}

void setClipRect(
    const UIRenderCommand& cmd,
    glm::vec2 clipOrigin,
    const glm::vec4& base,
    glm::vec2 clipScale)
{
    if (!cmd.clip) return;

    glm::vec2 pos = cmd.clipPos * clipScale + clipOrigin;
    glm::vec2 size = cmd.clipSize * clipScale;

    if (hasBaseScissor(base)) {
        const float x0 = std::max(pos.x, base.x);
        const float y0 = std::max(pos.y, base.y);
        const float x1 = std::min(pos.x + size.x, base.x + base.z);
        const float y1 = std::min(pos.y + size.y, base.y + base.w);
        pos = {x0, y0};
        size = {std::max(0.f, x1 - x0), std::max(0.f, y1 - y0)};
    } else {
        glEnable(GL_SCISSOR_TEST);
    }

    applyScissor(pos, size);
}

// Restores the surrounding scissor rather than disabling the test, which would
// otherwise release the confinement of the whole pass.
void clearClipRect(const UIRenderCommand& cmd, const glm::vec4& base) {
    if (!cmd.clip) return;
    if (hasBaseScissor(base))
        applyScissor({base.x, base.y}, {base.z, base.w});
    else
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
    Shader& shader,
    glm::vec2 clipOrigin,
    glm::vec4 baseScissor,
    glm::vec2 clipScale)
{
    setClipRect(cmd, clipOrigin, baseScissor, clipScale);
    cmd.font->draw(
        cmd.text,
        cmd.position,
        cmd.size,
        cmd.color,
        projection,
        shader
    );
    clearClipRect(cmd, baseScissor);
}

void UIRenderer::render(
    const std::vector<UIRenderCommand>& cmds,
    const glm::mat4& projection,
    Shader& textShader,
    Shader& rectShader,
    glm::vec2 clipOrigin,
    glm::vec4 baseScissor,
    glm::vec2 clipScale)
{
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const bool confined = hasBaseScissor(baseScissor);
    if (confined) {
        glEnable(GL_SCISSOR_TEST);
        applyScissor({baseScissor.x, baseScissor.y},
                     {baseScissor.z, baseScissor.w});
    }

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
                setClipRect(cmd, clipOrigin, baseScissor, clipScale);
                flushRectBatch({ toRectInstance(cmd) }, projection, rectShader);
                clearClipRect(cmd, baseScissor);
            } else {
                batch.push_back({
                    cmd.position, cmd.size, cmd.color,
                    cmd.borderColor, cmd.cornerRadii, cmd.borderWidth
                });
                if (batch.size() == MAX_INSTANCES) flush();
            }
        } else if (cmd.type == UICmdType::Text) {
            flush();
            drawText(cmd, projection, textShader, clipOrigin, baseScissor, clipScale);
        }
    }
    flush();

    if (confined)
        glDisable(GL_SCISSOR_TEST);
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
