#include <algorithm>

#include "scene.h"

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
    objects[id].parent = rootId;
    objects[rootId].children.push_back(id);
    return id;
}

ObjectID Scene::createObjectInternal() {
    ObjectID id = static_cast<ObjectID>(objects.size());
    objects.emplace_back();
    return id;
}
