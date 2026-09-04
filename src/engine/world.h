#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

#include "engine/asset_manager/model.h"
#include "engine/component_pool.h"
#include "engine/frustrum.h"
#include "engine/isystem.h"
#include "engine/renderer/renderer.h"
#include "glm/common.hpp"

struct Entity {
    uint32_t idx = UINT32_MAX;
    uint32_t gen = 0;

    static Entity invalid() { return {UINT32_MAX, 0}; }
};

struct EntitySlot {
    bool alive = false;
    uint32_t gen = 0;
    uint64_t comp_mask = 0;
};

struct Transform_ {
    glm::vec3 position{0.f};
    glm::vec3 rotation{0.f};
    glm::vec3 scale{1.f};

    glm::mat4 worldMatrix{1.f};
    glm::mat4 worldInvMatrix{1.f};
};

struct Object_ {
    std::string name;
    Model* model = nullptr;
    bool debug = false;
};

struct Hierarchy_ {
    Entity parent;
    std::vector<Entity> children;
};

struct TagRegistry {
    std::unordered_map<std::string, uint32_t> byName_;
    std::vector<std::string> byId;

    uint32_t intern(const std::string& name);
    uint32_t byName(const std::string& name) const;
    const std::string& name(uint32_t id) const;
    bool hasName(const std::string& name) const;
    bool isValidId(uint32_t id) const;
};

struct Tag_ {
    std::vector<uint32_t> ids;
};

class World {
public:
    std::vector<EntitySlot> slots;
    std::vector<uint32_t> freeIndices;
    uint32_t nextIndex = 0;

    bool isValid(const Entity& e) const;
    Entity create();
    Entity create(Entity parent);
    void destroy(Entity& e);

    template <typename T>
    T& get(const Entity& e);

    template <typename T>
    const T& get(const Entity& e) const;

    template <typename T>
    bool has(const Entity& e) const;

    template <typename T>
    T& add(const Entity& e);

    void update(float dt);
    void init();
    void collectRenderCommands(const Frustum& frustum, std::vector<RenderCommand>& out);

    void registerSystem(std::unique_ptr<ISystem> system);

    size_t lastRenderListSize = 0;

    Entity root = Entity::invalid();
    TagRegistry tagRegistry;

private:
    template <typename T>
    friend struct ComponentTraits;

    ComponentPool<Transform_> transforms_;
    ComponentPool<Object_> objects_;
    ComponentPool<Hierarchy_> hierarchies_;
    ComponentPool<Tag_> tags_;

    std::vector<std::unique_ptr<ISystem>> systems_;

    void ensureSlotCapacity(uint32_t idx);
    void clearEntityComponents(uint32_t idx);
};

template <typename T>
struct ComponentTraits {
    static constexpr uint64_t mask = 0;
    static ComponentPool<T>& pool(World& w);
};

#define WORLD_REGISTER_COMPONENT(Type, Member, Mask)                    \
    template <>                                                         \
    struct ComponentTraits<Type> {                                      \
        static constexpr uint64_t mask = Mask;                          \
        static ComponentPool<Type>& pool(World& w) { return w.Member; } \
    };

WORLD_REGISTER_COMPONENT(Transform_, transforms_, (1ull << 0))
WORLD_REGISTER_COMPONENT(Object_, objects_, (1ull << 1))
WORLD_REGISTER_COMPONENT(Hierarchy_, hierarchies_, (1ull << 2))
WORLD_REGISTER_COMPONENT(Tag_, tags_, (1ull << 3))

template <typename T>
T& World::get(const Entity& e) {
    assert(isValid(e));
    return ComponentTraits<T>::pool(*this).at(e.idx);
}

template <typename T>
const T& World::get(const Entity& e) const {
    assert(isValid(e));
    return ComponentTraits<T>::pool(*this).at(e.idx);
}

template <typename T>
bool World::has(const Entity& e) const {
    if (!isValid(e))
        return false;
    if (ComponentTraits<T>::mask == 0)
        return false;
    return (slots[e.idx].comp_mask & ComponentTraits<T>::mask) != 0;
}

template <typename T>
T& World::add(const Entity& e) {
    assert(isValid(e));
    slots[e.idx].comp_mask |= ComponentTraits<T>::mask;
    ComponentTraits<T>::pool(*this).ensure(e.idx);
    return ComponentTraits<T>::pool(*this).at(e.idx);
}

bool hasTag(const Tag_& t, uint32_t id);
void addTag(Tag_& t, uint32_t id);
void removeTag(Tag_& t, uint32_t id);
