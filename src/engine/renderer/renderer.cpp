#include "renderer.h"

void Renderer::submit(const Object& object,
                      const glm::mat4& modelMatrix) {
    if (!object.model)
        return;

    for (const SubMesh& part : object.model->getParts()) {
        RenderCommand cmd;
        cmd.mesh = &part.mesh;
        cmd.material = &part.material;
        cmd.model = modelMatrix;
        queue.push_back(cmd);
    }
}

void Renderer::render(const glm::mat4& view,
                      const glm::mat4& projection)
{
    // TODO: sort by shader, material, etc. to minimize state changes
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

    queue.clear();
}
