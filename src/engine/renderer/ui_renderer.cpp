#include "ui_renderer.h"
#include "engine/asset_manager/shader.h"
#include "engine/engine.h"
#include "engine/profilers/profile_scope.h"

void UIRenderer::render(const glm::mat4& projection) {
    PROFILE_SCOPE("UIRenderer::render");

    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Font& font = Engine::instance().assets.getFont("InterVariable");
    Shader& shader = Engine::instance().assets.getShader("glyph");

    font.draw(
        "WAZZAAAAAAPPP",
        50.0f,
        50.0f,
        1.0f,
        {1, 1, 0},
        projection,
        shader
    );
}
