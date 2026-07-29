#include <algorithm>
#include <string>

#include "scene.h"
#include "engine/profilers/profile_scope.h"
#include "engine/renderer/debug.h"
#include "engine/engine.h"

void collectRenderCommands(
    const Scene& scene,
    ObjectID id,
    const glm::mat4& parent,
    std::vector<RenderCommand>& out,
    const Frustum& frustum);

Scene::Scene() {
    rootId = createObjectInternal();
    objects[rootId].setID(rootId);
    objects[rootId].parent = rootId;
    objects[rootId].name = "Scene";
}

void Scene::buildRenderList(
        std::vector<RenderCommand>& out,
        const Frustum& frustum)
{
    out.reserve(lastRenderListSize); // reserve based on last frame
    PROFILE_SCOPE("collectRenderCommands");
    collectRenderCommands(*this, rootId, glm::mat4(1.0f), out, frustum);
    lastRenderListSize = out.size(); // remember for next frame
}

void collectRenderCommands(
    const Scene& scene,
    ObjectID id,
    const glm::mat4& parent,
    std::vector<RenderCommand>& out,
    const Frustum& frustum)
{
    const Object& obj = scene.get(id);
    glm::mat4 world = obj.worldMatrix;

    if (obj.model) {
        Bounds local = obj.getBounds();
        glm::vec3 wMin, wMax;
        transformAABB(local, world, wMin, wMax);

        if (!frustum.intersectsAABB(wMin, wMax)) {
            // still recurse children — child objects may be visible
            // even if parent is culled
            for (ObjectID child : obj.children)
                collectRenderCommands(scene, child, world, out, frustum);
            return;
        }
    }

    if (obj.debug) {
        DebugRenderer& debug = Engine::instance().debugRenderer;
        debug.axis(world, 2.5f);
        Bounds bounds = obj.getBounds();
        debug.box(
            world * glm::translate(glm::mat4(1.f), bounds.center),
            bounds.size, {1.f, 1.f, 1.f});
    }

    if (obj.model) {
        for (const auto& part : obj.model->getParts()) {
            RenderCommand cmd;
            cmd.mesh = &part.mesh;
            cmd.material = part.material;
            cmd.model = world;
            cmd.sortKey =
                (uint64_t)cmd.material->shader->ID << 48 |
                (uint64_t)cmd.material->id       << 32 |
                (uint64_t)cmd.mesh->id           << 16;
            cmd.allocation = &Engine::instance().meshRegistry.getAllocation(&part.mesh);
            out.push_back(cmd);
        }
    }

    for (auto child : obj.children) {
        collectRenderCommands(scene, child, world, out, frustum);
    }
}

void Scene::update() {
    updateScripts(getRoot());

    PROFILE_SCOPE("updateWorldTransforms");
    updateWorldTransforms(rootId, glm::mat4(1.0f));
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
    objects[id].name = "Object " + std::to_string(id);
    objects[rootId].children.push_back(id);
    return id;
}

ObjectID Scene::createObjectInternal() {
    ObjectID id = static_cast<ObjectID>(objects.size());
    objects.emplace_back();
    return id;
}

void Scene::updateWorldTransforms(ObjectID id, const glm::mat4& parentWorld) {
    Object& obj = objects[id];

    glm::mat4 local = obj.transform.localMatrix();
    obj.updateWorld(parentWorld * local);

    for (auto child : obj.children)
        updateWorldTransforms(child, obj.worldMatrix);
}
