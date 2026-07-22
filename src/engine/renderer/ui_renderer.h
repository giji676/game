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
    void render(
        const std::vector<UIRenderCommand>& cmds,
        const glm::mat4& projection,
        Shader& textShader,
        Shader& rectShader);

    void drawText(
        const UIRenderCommand& cmd,
        const glm::mat4& projection,
        Shader& shader);

    void drawRect(
        const UIRenderCommand& cmd,
        const glm::mat4& projection,
        Shader& shader);

private:
    GLuint quadVAO = 0, quadVBO = 0, instanceVBO = 0;
    static constexpr size_t MAX_INSTANCES = 4096;

    void initQuad();
    void flushRectBatch(const std::vector<RectInstance>& batch,
                         const glm::mat4& projection,
                         Shader& shader);
};
