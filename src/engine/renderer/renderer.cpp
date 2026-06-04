#include <algorithm>

#include "renderer.h"

void Renderer::render(std::vector<RenderCommand>& queue,
                      const glm::mat4& view,
                      const glm::mat4& projection)
{
    std::sort(queue.begin(), queue.end(),
              [](const RenderCommand& a, const RenderCommand& b) {
              return a.sortKey < b.sortKey;
              });

    for (const RenderCommand& cmd : queue) {
        Shader* shader = cmd.material->shader;

        shader->use();

        shader->setMat4("model", cmd.model);
        shader->setMat4("view", view);
        shader->setMat4("projection", projection);

        cmd.material->bind();

        for (unsigned int i = 0; i < cmd.material->textures.size(); i++) {
            const std::string& type = cmd.material->textures[i]->type;
            shader->setInt(type + std::to_string(i + 1), i);
        }

        shader->setVec3("diffuseFallback", cmd.material->diffuseFallback);
        shader->setVec3("specularFallback", cmd.material->specularFallback);

        cmd.mesh->draw();
    }
}
