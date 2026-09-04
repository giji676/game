#pragma once

class World;

struct ISystem {
    virtual ~ISystem() = default;
    virtual void update(World& w, float dt) = 0;
};
