#include "world.h"

#include "engine/render_system.h"
#include "engine/transfrom_system.h"

void World::init() {
    root = create(Entity::invalid());
    Object_& rootObj = add<Object_>(root);
    rootObj.name = "Scene";
}

void World::update(float dt) {
    (void)dt;
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

void World::destroy(Entity& e) {
    if (!isValid(e))
        return;

    if (isValid(root) && e.idx == root.idx)
        return;

    if (has<Hierarchy_>(e)) {
        Hierarchy_& h = get<Hierarchy_>(e);
        if (isValid(h.parent) && has<Hierarchy_>(h.parent)) {
            Hierarchy_& parentH = get<Hierarchy_>(h.parent);
            auto it = std::find_if(parentH.children.begin(), parentH.children.end(),
                [&](const Entity& child) { return child.idx == e.idx; });
            if (it != parentH.children.end())
                parentH.children.erase(it);
        }
        for (const Entity& child : h.children)
            destroy(const_cast<Entity&>(child));
    }

    slots[e.idx].alive = false;
    slots[e.idx].comp_mask = 0;
    slots[e.idx].gen++;
    freeIndices.push_back(e.idx);
}

bool World::isValid(const Entity& e) const {
    return e.idx != UINT32_MAX &&
        e.idx < slots.size() &&
        e.gen == slots[e.idx].gen &&
        slots[e.idx].alive;
}
