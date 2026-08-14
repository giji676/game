#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "engine/defines.h"
#include "engine/icomponent.h"
#include "engine/iscript.h"
#include "engine/asset_manager/transform.h"
#include "model.h"

class Engine;

class Object {
public:
    Model* model = nullptr;
    Transform transform;
    glm::mat4 worldMatrix{1.f};
    glm::mat4 worldInvMatrix{1.f};

    ObjectID parent = 0;
    std::vector<ObjectID> children;

    std::vector<std::unique_ptr<IScript>> scripts;
    std::vector<std::unique_ptr<IComponent>> components;
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
        static_assert(std::is_base_of<IScript, T>::value,
                      "T must inherit IScript");
        auto script = std::make_unique<T>(std::forward<Args>(args)...);
        script->object = getID();
        T& ref = *script;
        scripts.push_back(std::move(script));
        return ref;
    }

    template <typename T, typename... Args>
    T& addComponent(Args&&... args) {
        static_assert(std::is_base_of<IComponent, T>::value,
                      "T must inherit IComponent");
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        component->object = getID();
        T& ref = *component;
        components.push_back(std::move(component));
        return ref;
    }

    template <typename T>
    T* getComponent() {
        static_assert(std::is_base_of<IComponent, T>::value,
                      "T must inherit IComponent");
        for (auto& component : components) {
            if (T* typed = dynamic_cast<T*>(component.get()))
                return typed;
        }
        return nullptr;
    }

    template <typename T>
    const T* getComponent() const {
        static_assert(std::is_base_of<IComponent, T>::value,
                      "T must inherit IComponent");
        for (const auto& component : components) {
            if (const T* typed = dynamic_cast<const T*>(component.get()))
                return typed;
        }
        return nullptr;
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
