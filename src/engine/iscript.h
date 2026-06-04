#pragma once

#include "defines.h"

class IScript {
public:
    ObjectID object;

    IScript(){}
    virtual ~IScript(){}
    virtual void init() = 0;
    virtual void update() = 0;
};
