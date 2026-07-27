#pragma once

#include "engine/asset_manager/font.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

enum class UICmdType {
    Rect,
    Image,
    Text
};

struct UIRenderCommand {
    UICmdType type;

    glm::vec2 position;
    glm::vec2 size;
    glm::vec4 color;

    glm::vec4 cornerRadii = {0,0,0,0}; // TL, TR, BR, BL
    float borderWidth = 0.f;
    glm::vec4 borderColor = {0,0,0,0};

    Font* font = nullptr;
    std::string text;

    bool clip = false;
    glm::vec2 clipPos;
    glm::vec2 clipSize;
};

struct RectInstance {
    glm::vec2 pos;
    glm::vec2 size;
    glm::vec4 color;
    glm::vec4 borderColor;
    glm::vec4 cornerRadii;
    float borderWidth;
    // float _pad[3]; // keep 16-byte alignment for std140-ish layout if you ever UBO this
};

class UIRenderer {
public:
    void init();

    // clipOrigin shifts surface-local clip rects into window space.
    // baseScissor (x, y, w, h, window space) confines the whole pass; a zero
    // width or height means unconfined.
    void render(
        const std::vector<UIRenderCommand>& cmds,
        const glm::mat4& projection,
        Shader& textShader,
        Shader& rectShader,
        glm::vec2 clipOrigin = {0.f, 0.f},
        glm::vec4 baseScissor = {0.f, 0.f, 0.f, 0.f},
        glm::vec2 clipScale = {1.f, 1.f});

    void drawText(
        const UIRenderCommand& cmd,
        const glm::mat4& projection,
        Shader& shader,
        glm::vec2 clipOrigin,
        glm::vec4 baseScissor,
        glm::vec2 clipScale);

private:
    GLuint quadVAO = 0, quadVBO = 0, instanceVBO = 0;
    static constexpr size_t MAX_INSTANCES = 4096;

    void initQuad();
    void flushRectBatch(const std::vector<RectInstance>& batch,
                         const glm::mat4& projection,
                         Shader& shader);
};
