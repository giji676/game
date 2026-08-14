#pragma once

#include "defines.h"
#include "inspectable.h"

class IBehaviour : public Inspectable {
public:
    ObjectID object = INVALID_OBJECT;
    bool enabled = true;

    IBehaviour() {
        INSPECT(enabled);
    }

    virtual ~IBehaviour() = default;

    virtual const char* typeName() const { return "Behaviour"; }

    virtual void init() {}
    virtual void update() {}
    virtual void lateUpdate() {}
};
