#include <algorithm>

#include "renderer.h"
#include "engine/profilers/profile_scope.h"

void Renderer::render(std::vector<RenderCommand>& queue,
                      const glm::mat4& view,
                      const glm::mat4& projection)
{
    PROFILE_SCOPE("Renderer::render");
    std::sort(queue.begin(), queue.end(),
              [](const RenderCommand& a, const RenderCommand& b) {
              return a.sortKey < b.sortKey;
              });

    Shader* currentShader = nullptr;
    const Material* currentMaterial = nullptr;

    for (const RenderCommand& cmd : queue) {
        if (cmd.material->shader != currentShader) {
            currentShader = cmd.material->shader;

            currentShader->use();
            currentShader->setMat4("view", view);
            currentShader->setMat4("projection", projection);

            currentMaterial = nullptr;
        }

        if (cmd.material != currentMaterial) {
            currentMaterial = cmd.material;

            currentMaterial->bind();

            currentShader->setVec3(
                "diffuseFallback",
                currentMaterial->diffuseFallback);

            currentShader->setVec3(
                "specularFallback",
                currentMaterial->specularFallback);

            for (unsigned int i = 0; i < currentMaterial->textures.size(); i++) {
                const std::string& type =
                    currentMaterial->textures[i]->type;

                currentShader->setInt(
                    type + std::to_string(i + 1),
                    i);
            }
        }

        currentShader->setMat4("model", cmd.model);

        cmd.mesh->draw();
    }
}
