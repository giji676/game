#include <algorithm>

#include "scene.h"
#include "engine.h"

glm::mat4 transformToMatrix(const Transform& t);

void Scene::render() {
    Engine& engine = Engine::instance();
    Camera& camera = *engine.getActiveCamera();

    glm::mat4 view = glm::lookAt(camera.pos, camera.pos + camera.front, camera.up);
    glm::mat4 projection = glm::perspective(
        glm::radians(45.f),
        (float)engine.app.width() / (float)engine.app.height(),
        0.1f, 100.0f);

    glm::mat4 identity(1.0f);

    recurseRender(getRoot(), identity);
    Engine::instance().renderer.render(view, projection);
}

void Scene::recurseRender(
    const ObjectID objId,
    const glm::mat4& parentMatrix)
{
    Object& obj = Engine::instance().scene.get(objId);

    glm::mat4 localMatrix =
        transformToMatrix(obj.transform);

    glm::mat4 worldMatrix =
        parentMatrix * localMatrix;

    Engine::instance().renderer.submit(obj, worldMatrix);

    for (const auto& child : obj.children) {
        recurseRender(child, worldMatrix);
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
