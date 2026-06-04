#pragma once

#include "engine/asset_manager/object.h"
#include "renderer/renderer.h"

class Scene {
public:
    Scene() { rootId = createObjectInternal(); }

    void init();
    void update();
    std::vector<RenderCommand> buildRenderList();

    ObjectID createObject();

    void reparent(ObjectID childId, ObjectID newParentId);

    Object& get(ObjectID id) { return objects[id]; }
    const Object& get(ObjectID id) const { return objects[id]; }

    ObjectID getRoot() const { return rootId; }
    Object& root() { return objects[rootId]; }
    const Object& root() const { return objects[rootId]; }

private:
    std::vector<Object> objects;
    ObjectID rootId = 0;

    ObjectID createObjectInternal();
    void updateScripts(ObjectID id);
    void initScripts(ObjectID id);
    void recurseRender(
        const ObjectID objId,
        const glm::mat4& parentMatrix);
};
