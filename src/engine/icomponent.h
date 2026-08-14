#pragma once

#include "ibehaviour.h"

class IComponent : public IBehaviour {
public:
    virtual ~IComponent() = default;
    const char* typeName() const override { return "Component"; }
};
