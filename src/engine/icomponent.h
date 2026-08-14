#pragma once

#include "ibehaviour.h"

class IComponent : public IBehaviour {
public:
    virtual ~IComponent() = default;
};
