#pragma once

#include "engine/iscript.h"
#include "engine/engine.h"
#include "engine/asset_manager/object.h"

class Test : public IScript {
public:
    void init() override { }
    void update() override {
        Object& obj = Engine::instance().scene.get(object);
        obj.transform.rotation.y += 45.f * Engine::instance().app.deltaTime;
    }
};
