#pragma once

#include "defines.h"

class IBehaviour {
public:
    ObjectID object = INVALID_OBJECT;

    virtual ~IBehaviour() = default;

    virtual void init() {}
    virtual void update() {}
    virtual void lateUpdate() {}
};
