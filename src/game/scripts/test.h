#pragma once

#include "engine/iscript.h"
#include "engine/engine.h"
#include "engine/asset_manager/object.h"
#include "engine/object_ref.h"

class Test : public IScript {
public:
    ObjectRef target;

    Test() {
        INSPECT(target);
    }

    void init() override { }
    const char* typeName() const override { return "Test"; }
    void update() override {
        Scene& scene = Engine::instance().scene;
        Object* obj = target.get(scene);
        if (!obj)
            obj = &scene.get(object);
        obj->transform.rotate({0, 45.f * Engine::instance().app.deltaTime, 0});
    }
};
