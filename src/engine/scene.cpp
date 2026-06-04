#include <algorithm>

#include "scene.h"
#include "engine.h"

void collectRenderCommands(
    const Scene& scene,
    ObjectID id,
    const glm::mat4& parent,
    std::vector<RenderCommand>& out);
glm::mat4 transformToMatrix(const Transform& t);

std::vector<RenderCommand> Scene::buildRenderList() {
    std::vector<RenderCommand> out;
    collectRenderCommands(*this, rootId, glm::mat4(1.0f), out);
    return out;
}

void collectRenderCommands(
    const Scene& scene,
    ObjectID id,
    const glm::mat4& parent,
    std::vector<RenderCommand>& out)
{
    const Object& obj = scene.get(id);

    glm::mat4 local = transformToMatrix(obj.transform);
    glm::mat4 world = parent * local;

    if (obj.model) {
        for (const auto& part : obj.model->getParts()) {
            RenderCommand cmd;
            cmd.mesh = &part.mesh;
            cmd.material = &part.material;
            cmd.model = world;
            out.push_back(cmd);
        }
    }

    for (auto child : obj.children) {
        collectRenderCommands(scene, child, world, out);
    }
}

glm::mat4 transformToMatrix(const Transform& t) {
    glm::mat4 m(1.0f);

    m = glm::translate(m, t.position);

    m = glm::rotate(m, glm::radians(t.rotation.x),
                    glm::vec3(1,0,0));
    m = glm::rotate(m, glm::radians(t.rotation.y),
                    glm::vec3(0,1,0));
    m = glm::rotate(m, glm::radians(t.rotation.z),
                    glm::vec3(0,0,1));

    m = glm::scale(m, t.scale);

    return m;
}

void Scene::update() {
    updateScripts(getRoot());
}

void Scene::updateScripts(ObjectID id) {
    Object& obj = get(id);

    for (auto& script : obj.scripts) {
        script->update();
    }

    for (ObjectID child : obj.children) {
        updateScripts(child);
    }
}

void Scene::init() {
    initScripts(getRoot());
}

void Scene::initScripts(ObjectID id) {
    Object& obj = get(id);

    for (auto& script : obj.scripts) {
        script->init();
    }

    for (ObjectID child : obj.children) {
        initScripts(child);
    }
}

void Scene::reparent(ObjectID childId, ObjectID newParentId) {
    ObjectID oldParentId = objects[childId].parent;

    auto& siblings = objects[oldParentId].children;

    siblings.erase(
        std::remove(
            siblings.begin(),
            siblings.end(),
            childId),
        siblings.end());

    objects[childId].parent = newParentId;
    objects[newParentId].children.push_back(childId);
}

ObjectID Scene::createObject() {
    ObjectID id = createObjectInternal();
    objects[id].setID(id);
    objects[id].parent = rootId;
    objects[rootId].children.push_back(id);
    return id;
}

ObjectID Scene::createObjectInternal() {
    ObjectID id = static_cast<ObjectID>(objects.size());
    objects.emplace_back();
    return id;
}
