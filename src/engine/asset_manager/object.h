#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "engine/defines.h"
#include "engine/iscript.h"
#include "engine/asset_manager/transform.h"
#include "model.h"

class Object {
public:
    Model* model = nullptr;
    Transform transform;

    ObjectID parent = 0;
    std::vector<ObjectID> children;

    std::vector<std::unique_ptr<IScript>> scripts;

    template <typename T, typename... Args>
    T& addScript(Args&&... args) {
        auto script = std::make_unique<T>(std::forward<Args>(args)...);
        script->object = getID();

        scripts.push_back(std::move(script));
        return *script;
    }

    Object() = default;

    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    Object(Object&&) = default;
    Object& operator=(Object&&) = default;

    ObjectID getID() const { return ID; }
    void setID(ObjectID id) { ID = id; }

private:
    ObjectID ID;
};
