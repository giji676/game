#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "engine/defines.h"
#include "engine/iscript.h"
#include "engine/asset_manager/transform.h"
#include "model.h"

class Engine;

class Object {
public:
    Model* model = nullptr;
    Transform transform;
    glm::mat4 worldMatrix;
    glm::mat4 worldInvMatrix;

    ObjectID parent = 0;
    std::vector<ObjectID> children;

    std::vector<std::unique_ptr<IScript>> scripts;
    bool debug = false;
    std::string name;

    void markChildrenDirty(Object& obj);

    void updateWorld(const glm::mat4& world) {
        worldMatrix = world;
        worldInvMatrix = glm::inverse(world);
    }

    Bounds getBounds() const {
        if (model) return model->getBounds();

        return Bounds{
            .min = glm::vec3(0.0f),
            .max = glm::vec3(0.0f),
            .center = glm::vec3(0.0f),
            .size = glm::vec3(0.0f),
        };
    }
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
