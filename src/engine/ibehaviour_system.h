#pragma once

#include "engine/world.h"

struct IBehaviourSystem {
    void update(World& w) {
        for (uint32_t idx : w.entitiesWith<IBehaviour_>()) {
            if (!w.slots[idx].alive)
                continue;

            const Entity e = {idx, w.slots[idx].gen};
            IBehaviour_& ib = w.get<IBehaviour_>(e);

            for (auto& component : ib.components) {
                if (component->enabled)
                    component->update();
            }
            for (auto& script : ib.scripts) {
                if (script->enabled)
                    script->update();
            }
        }
    }

    void lateUpdate(World& w) {
        for (uint32_t idx : w.entitiesWith<IBehaviour_>()) {
            if (!w.slots[idx].alive)
                continue;

            const Entity e = {idx, w.slots[idx].gen};
            IBehaviour_& ib = w.get<IBehaviour_>(e);

            for (auto& component : ib.components) {
                if (component->enabled)
                    component->lateUpdate();
            }
            for (auto& script : ib.scripts) {
                if (script->enabled)
                    script->lateUpdate();
            }
        }
    }
};
