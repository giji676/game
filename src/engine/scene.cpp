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
            // Still recurse children - child objects may be visible
            // even if parent is culled.
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
                (uint64_t)(cmd.material->usesTransparency() ? 1 : 0) << 63 |
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

void Scene::update(bool runBehaviours) {
    if (runBehaviours) {
        updateScripts(getRoot());
        updateComponents(getRoot());
    }

    PROFILE_SCOPE("updateWorldTransforms");
    updateWorldTransforms(rootId, glm::mat4(1.0f));

    // Readers (camera, etc.) pull world pose after hierarchy update.
    lateUpdateComponents(getRoot());
    lateUpdateScripts(getRoot());
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

void Scene::updateComponents(ObjectID id) {
    Object& obj = get(id);

    for (auto& component : obj.components) {
        component->update();
    }

    for (ObjectID child : obj.children) {
        updateComponents(child);
    }
}

void Scene::lateUpdateScripts(ObjectID id) {
    Object& obj = get(id);

    for (auto& script : obj.scripts) {
        script->lateUpdate();
    }

    for (ObjectID child : obj.children) {
        lateUpdateScripts(child);
    }
}

void Scene::lateUpdateComponents(ObjectID id) {
    Object& obj = get(id);

    for (auto& component : obj.components) {
        component->lateUpdate();
    }

    for (ObjectID child : obj.children) {
        lateUpdateComponents(child);
    }
}

void Scene::init() {
    initBehaviours(getRoot());
}

void Scene::initBehaviours(ObjectID id) {
    Object& obj = get(id);

    for (auto& component : obj.components) {
        component->init();
    }
    for (auto& script : obj.scripts) {
        script->init();
    }

    for (ObjectID child : obj.children) {
        initBehaviours(child);
    }
}

void Scene::reparent(ObjectID childId, ObjectID newParentId, int index) {
    if (childId == INVALID_OBJECT || newParentId == INVALID_OBJECT)
        return;
    if (childId == newParentId)
        return;
    if (childId == rootId)
        return;
    if (isDescendant(childId, newParentId))
        return;

    ObjectID oldParentId = objects[childId].parent;
    auto& oldSiblings = objects[oldParentId].children;
    auto oldIt = std::find(oldSiblings.begin(), oldSiblings.end(), childId);
    if (oldIt == oldSiblings.end())
        return;

    const int oldIndex = static_cast<int>(oldIt - oldSiblings.begin());
    oldSiblings.erase(oldIt);

    auto& newSiblings = objects[newParentId].children;
    if (oldParentId == newParentId && oldIndex < index)
        --index;

    if (index < 0 || index > static_cast<int>(newSiblings.size()))
        index = static_cast<int>(newSiblings.size());

    newSiblings.insert(newSiblings.begin() + index, childId);
    objects[childId].parent = newParentId;
}

bool Scene::isDescendant(ObjectID ancestorId, ObjectID id) const {
    if (ancestorId == INVALID_OBJECT || id == INVALID_OBJECT)
        return false;
    ObjectID current = id;
    while (current != rootId && current != INVALID_OBJECT) {
        if (current == ancestorId)
            return true;
        current = objects[current].parent;
        if (current == objects[current].parent && current == rootId)
            break;
    }
    return false;
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
