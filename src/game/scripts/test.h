#pragma once

#include "engine/iscript.h"
#include "engine/asset_manager/object.h"

class Test : public IScript {
public:
    void init() override { }
    void update(float dt) override {
        // Object& obj = scene.get(parentObject);
        // obj->transform.position.y += 0.5f * dt;
    }
};
