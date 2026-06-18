#include "ui_renderer.h"
#include "engine/asset_manager/shader.h"

void drawText(
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
void UIRenderer::render(
    const std::vector<UIRenderCommand>& cmds,
    const glm::mat4& projection,
    Shader& textShader)
{
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto& cmd : cmds) {
        switch (cmd.type) {
            case UICmdType::Text:
                drawText(cmd, projection, textShader);
                break;
            default:
                break;
        }
    }
}
