#pragma once

#include "engine/defines.h"

class Object;
class Scene;

// Unity-style serialized references. The editor stores an ObjectID; scripts
// resolve it through Scene when they need the live object / component / script.
struct ObjectRef {
    ObjectID id = INVALID_OBJECT;

    template <typename SceneT>
    auto get(SceneT& scene) const -> decltype(&scene.get(id)) {
        if (id == INVALID_OBJECT || !scene.valid(id))
            return nullptr;
        return &scene.get(id);
    }
};

template <typename T>
struct ComponentRef {
    ObjectID id = INVALID_OBJECT;

    template <typename SceneT>
    auto get(SceneT& scene) const -> decltype(scene.get(id).template getComponent<T>()) {
        if (id == INVALID_OBJECT || !scene.valid(id))
            return nullptr;
        return scene.get(id).template getComponent<T>();
    }
};

template <typename T>
struct ScriptRef {
    ObjectID id = INVALID_OBJECT;

    template <typename SceneT>
    auto get(SceneT& scene) const -> decltype(scene.get(id).template getScript<T>()) {
        if (id == INVALID_OBJECT || !scene.valid(id))
            return nullptr;
        return scene.get(id).template getScript<T>();
    }
};
