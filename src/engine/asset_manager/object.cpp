#include "object.h"
#include "engine/engine.h"

void Object::markChildrenDirty(Object& obj) {
    obj.transform.dirty = true;
    for (auto child : obj.children) {
        markChildrenDirty(ENGINE().scene.get(child));
    }
}

