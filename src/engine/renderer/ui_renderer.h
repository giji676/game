#pragma once

#include "engine/asset_manager/font.h"
#include "engine/asset_manager/texture.h"
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

    Font* font = nullptr;
    Texture* texture = nullptr;
    std::string text;
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
    GLuint quadVAO = 0;
    GLuint quadVBO = 0;

    void initQuad();
};
