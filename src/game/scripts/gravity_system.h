#pragma once

#include "engine/isystem.h"
#include "engine/world.h"
#include "game/components/gravity.h"

struct GravitySystem : ISystem {
    void update(World& w, float dt) override {
        for (uint32_t idx = 0; idx < w.slots.size(); ++idx) {
            if (!w.slots[idx].alive)
                continue;

            const Entity e = {idx, w.slots[idx].gen};
            if (!w.has<Gravity_>(e) || !w.has<Transform_>(e))
                continue;

            Gravity_& gravity = w.get<Gravity_>(e);
            Transform_& transform = w.get<Transform_>(e);

            gravity.velocity += gravity.acceleration * dt;
            transform.position += gravity.velocity * dt;
        }
    }
};
