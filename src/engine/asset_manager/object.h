#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "engine/defines.h"
#include "engine/iscript.h"
#include "model.h"

typedef struct Transform {
    glm::vec3 position{0.f};
    glm::vec3 rotation{0.f};
    glm::vec3 scale{1.f};
} Transform;

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
        script->parentObject = getID();
        scripts.push_back(std::move(script));
        return *script;
    }

    Object() = default;

    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    Object(Object&&) = default;
    Object& operator=(Object&&) = default;

    ObjectID getID() const { return ID; }

private:
    ObjectID ID;
};
