#pragma once

#include "engine/asset_manager/object.h"
#include "engine/frustrum.h"
#include "renderer/renderer.h"

class Scene {
public:
    Scene();

    void init();
    void update();
    void buildRenderList(
            std::vector<RenderCommand>& out,
            const Frustum& frustum);

    ObjectID createObject();

    void reparent(ObjectID childId, ObjectID newParentId, int index = -1);
    bool isDescendant(ObjectID ancestorId, ObjectID id) const;

    Object& get(ObjectID id) { return objects[id]; }
    const Object& get(ObjectID id) const { return objects[id]; }

    ObjectID getRoot() const { return rootId; }
    Object& root() { return objects[rootId]; }
    const Object& root() const { return objects[rootId]; }

private:
    std::vector<Object> objects;
    ObjectID rootId = 0;
    size_t lastRenderListSize = 0;

    ObjectID createObjectInternal();
    void updateScripts(ObjectID id);
    void initScripts(ObjectID id);
    void recurseRender(
        const ObjectID objId,
        const glm::mat4& parentMatrix);
    void updateWorldTransforms(
        const ObjectID objId,
        const glm::mat4& parentWorld);
};
