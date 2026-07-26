#pragma once

#include <glm/glm.hpp>

#include "engine/defines.h"

class UI;

// Resolves style-driven positions and sizes for the whole element tree.
// Percentages resolve against the parent's resolved box, and the root box is
// the viewport, so styled elements follow the window as it resizes.
void layoutTree(UI& ui, UIElementID rootId, glm::vec2 viewportSize);
