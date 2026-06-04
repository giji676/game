#pragma once

#include "engine/iscript.h"
#include "engine/engine.h"
#include "engine/asset_manager/object.h"

class Test : public IScript {
public:
    void init() override { }
    void update() override {
        Object& obj = Engine::instance().scene.get(parentObject);
        obj.transform.position.y += 0.5f * Engine::instance().app.deltaTime;
    }
};
