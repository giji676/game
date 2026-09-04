#include "world.h"

#include <algorithm>

#include "engine/render_system.h"
#include "engine/transfrom_system.h"

uint32_t TagRegistry::byName(const std::string& name) const {
    const auto it = byName_.find(name);
    assert(it != byName_.end());
    return it->second;
}

uint32_t TagRegistry::intern(const std::string& name) {
    const auto it = byName_.find(name);
    if (it != byName_.end())
        return it->second;

    const uint32_t id = static_cast<uint32_t>(byId.size());
    byName_.emplace(name, id);
    byId.push_back(name);
    return id;
}

const std::string& TagRegistry::name(uint32_t id) const {
    assert(id < byId.size());
    return byId[id];
}

bool TagRegistry::hasName(const std::string& name) const {
    return byName_.find(name) != byName_.end();
}

bool TagRegistry::isValidId(uint32_t id) const {
    return id < byId.size();
}

bool hasTag(const Tag_& t, uint32_t id) {
    return std::find(t.ids.begin(), t.ids.end(), id) != t.ids.end();
}

void addTag(Tag_& t, uint32_t id) {
    if (!hasTag(t, id))
        t.ids.push_back(id);
}

void removeTag(Tag_& t, uint32_t id) {
    auto it = std::find(t.ids.begin(), t.ids.end(), id);
    if (it != t.ids.end())
        t.ids.erase(it);
}

void World::init() {
    root = create(Entity::invalid());
    Object_& rootObj = add<Object_>(root);
    rootObj.name = "Scene";
}

void World::registerSystem(std::unique_ptr<ISystem> system) {
    systems_.push_back(std::move(system));
}

void World::update(float dt) {
    for (auto& system : systems_)
        system->update(*this, dt);

    TransformSystem ts;
    ts.update(*this);
}

void World::collectRenderCommands(
    const Frustum& frustum,
    std::vector<RenderCommand>& out)
{
    RenderSystem rs;
    rs.lastSize = lastRenderListSize;
    rs.update(*this, frustum);
    lastRenderListSize = rs.lastSize;
    out.insert(out.end(), rs.commands.begin(), rs.commands.end());
}

void World::ensureSlotCapacity(uint32_t idx) {
    const size_t need = static_cast<size_t>(idx) + 1;
    if (slots.size() < need)
        slots.resize(need);

    transforms_.ensure(idx);
}

Entity World::create() {
    return create(Entity::invalid());
}

Entity World::create(Entity parent) {
    uint32_t idx;

    if (freeIndices.empty())
        idx = nextIndex++;
    else {
        idx = freeIndices.back();
        freeIndices.pop_back();
    }

    ensureSlotCapacity(idx);

    transforms_.reset(idx);

    Entity e = {
        .idx = idx,
        .gen = slots[idx].gen,
    };

    slots[idx].alive = true;
    slots[idx].comp_mask = ComponentTraits<Transform_>::mask;

    Hierarchy_& h = add<Hierarchy_>(e);
    h.children.clear();

    // Creating the scene root: no parent, do not touch any parent list.
    if (!isValid(parent) && !isValid(root)) {
        h.parent = Entity::invalid();
        return e;
    }

    // Default parent is the scene root.
    if (!isValid(parent))
        parent = root;

    h.parent = parent;

    Hierarchy_& parentHierarchy = add<Hierarchy_>(parent);
    parentHierarchy.children.push_back(e);

    return e;
}

void World::clearEntityComponents(uint32_t idx) {
    transforms_.reset(idx);
    objects_.reset(idx);
    hierarchies_.reset(idx);
    tags_.reset(idx);
}

void World::destroy(Entity& e) {
    if (!isValid(e))
        return;

    if (isValid(root) && e.idx == root.idx)
        return;

    const uint32_t idx = e.idx;

    if (has<Hierarchy_>(e)) {
        Hierarchy_& h = get<Hierarchy_>(e);
        if (isValid(h.parent) && has<Hierarchy_>(h.parent)) {
            Hierarchy_& parentH = get<Hierarchy_>(h.parent);
            auto it = std::find_if(parentH.children.begin(), parentH.children.end(),
                [&](const Entity& child) { return child.idx == idx; });
            if (it != parentH.children.end())
                parentH.children.erase(it);
        }

        // Copy first: recursive destroy unlinks children from this parent.
        std::vector<Entity> children = h.children;
        h.children.clear();
        for (Entity child : children)
            destroy(child);
    }

    clearEntityComponents(idx);

    slots[idx].alive = false;
    slots[idx].comp_mask = 0;
    slots[idx].gen++;
    freeIndices.push_back(idx);

    e = Entity::invalid();
}

bool World::isValid(const Entity& e) const {
    return e.idx != UINT32_MAX &&
        e.idx < slots.size() &&
        e.gen == slots[e.idx].gen &&
        slots[e.idx].alive;
}
