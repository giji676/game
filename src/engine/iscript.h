#pragma once

#include "defines.h"

class IScript {
public:
    ObjectID parentObject;

    IScript(){}
    virtual ~IScript(){}
    virtual void init() = 0;
    virtual void update(float dt) = 0;
};
