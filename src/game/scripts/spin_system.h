#pragma once

#include "engine/isystem.h"
#include "engine/world.h"

struct SpinSystem : ISystem {
    void update(World& w, float dt) override {
        if (!w.tagRegistry.hasName("spin"))
            return;

        const uint32_t spinTag = w.tagRegistry.byName("spin");
        for (uint32_t idx : w.tagRegistry.entities(spinTag)) {
            if (!w.slots[idx].alive)
                continue;

            const Entity e = {idx, w.slots[idx].gen};
            if (!w.has<Transform_>(e))
                continue;

            Transform_& transform = w.get<Transform_>(e);
            transform.rotation.y += dt * 45.f;
        }
    }
};
