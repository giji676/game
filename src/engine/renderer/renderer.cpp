#include "renderer.h"

void Renderer::submit(const Object& object) {
    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, object.transform.position);
    model = glm::rotate(model, glm::radians(object.transform.rotation.x), glm::vec3(1,0,0));
    model = glm::rotate(model, glm::radians(object.transform.rotation.y), glm::vec3(0,1,0));
    model = glm::rotate(model, glm::radians(object.transform.rotation.z), glm::vec3(0,0,1));
    model = glm::scale(model, object.transform.scale);

    for (const SubMesh& part : object.model->getParts()) {
        RenderCommand cmd;
        cmd.mesh = &part.mesh;
        cmd.material = &part.material;
        cmd.model = model;
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
