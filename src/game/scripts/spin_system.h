#pragma once

#include "engine/isystem.h"
#include "engine/world.h"

struct SpinSystem : ISystem {
    void update(World& w, float dt) override {
        for (uint32_t idx = 0; idx < w.slots.size(); ++idx) {
            if (!w.slots[idx].alive)
                continue;

            const Entity e = {idx, w.slots[idx].gen};
            if (!w.has<Object_>(e) || !w.has<Transform_>(e) || !w.has<Tag_>(e))
                continue;

            if (!hasTag(w.get<Tag_>(e), w.tagRegistry.byName("spin")))
                continue;

            Transform_& transform = w.get<Transform_>(e);
            transform.rotation.y += dt * 45.f;
        }
    }
};
