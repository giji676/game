#pragma once

#include "engine/iscript.h"
#include "engine/engine.h"
#include "engine/object_ref.h"
#include "engine/world.h"

class Test : public IScript {
public:
    EntityRef target;

    Test() {
        INSPECT(target);
    }

    void init() override {}
    const char* typeName() const override { return "Test"; }

    void update() override {
        World& world = ENGINE().world;
        Entity e = target.id;
        if (!world.isValid(e))
            e = entity;
        if (!world.isValid(e) || !world.has<Transform_>(e))
            return;

        Transform_& t = world.get<Transform_>(e);
        t.rotation.y += 45.f * DT();
    }
};
