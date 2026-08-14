#pragma once

#include "ibehaviour.h"

class IScript : public IBehaviour {
public:
    virtual ~IScript() = default;
    const char* typeName() const override { return "Script"; }
};
