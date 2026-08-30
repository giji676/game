#include "world.h"

#include "engine/engine.h"
#include "engine/scene.h"

void World::init() {}

void World::update(float dt) {
    Scene& scene = ENGINE().scene;

    for (uint32_t idx = 0; idx < slots.size(); ++idx) {
        if (!slots[idx].alive)
            continue;

        const Entity e = {idx, slots[idx].gen};
        if (!has<Transform_>(e))
            continue;

        Transform_& transform = get<Transform_>(e);
        transform.rotation.y += 45.f * dt;
    }
}

void World::ensureSlotCapacity(uint32_t idx) {
    const size_t need = static_cast<size_t>(idx) + 1;
    if (slots.size() < need)
        slots.resize(need);

    transforms_.ensure(idx);
}

Entity World::create() {
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

    return e;
}

void World::destroy(Entity& e) {
    if (!isValid(e))
        return;

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
