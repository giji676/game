#include "renderer.h"

void Renderer::submit(const Object& object,
                      const glm::mat4& modelMatrix) {
    for (const SubMesh& part : object.model->getParts()) {
        RenderCommand cmd;
        cmd.mesh = &part.mesh;
        cmd.material = &part.material;
        cmd.model = modelMatrix;
        queue.push_back(cmd);
    }
}

void Renderer::render(const glm::mat4& view,
                      const glm::mat4& projection) {
    for (const RenderCommand& cmd : queue) {
        Shader* shader = cmd.material->shader;

        shader->use();

        cmd.material->bind();

        shader->setMat4("model", cmd.model);
        shader->setMat4("view", view);
        shader->setMat4("projection", projection);

        cmd.mesh->draw();
    }

    queue.clear();
}
