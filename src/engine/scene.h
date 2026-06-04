#pragma once

#include "engine/asset_manager/object.h"

class Scene {
public:
    Scene() { rootId = createObjectInternal(); }

    ObjectID getRoot() const { return rootId; }
    Object& root() { return objects[rootId]; }
    const Object& root() const { return objects[rootId]; }

    ObjectID createObject();

    void reparent(ObjectID childId, ObjectID newParentId);

    Object& get(ObjectID id) { return objects[id]; }
    const Object& get(ObjectID id) const { return objects[id]; }

private:
    std::vector<Object> objects;
    ObjectID rootId = 0;

    ObjectID createObjectInternal();
};
