#pragma once

#include <algorithm>
#include <glm/glm.hpp>

inline float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(v, hi));
}

inline bool pointInRect(
        const glm::vec2& point,
        const glm::vec2& rectPos,
        const glm::vec2& rectSize)
{
    return point.x >= rectPos.x &&
           point.x <= rectPos.x + rectSize.x &&
           point.y >= rectPos.y &&
           point.y <= rectPos.y + rectSize.y;
}
