#pragma once

#include "engine/renderer/ui_renderer.h"

class UIElement;

class UIWidget {
public:
    virtual ~UIWidget() = default;

    virtual void update(
            const UIElement& e,
            const glm::vec2& pos) {}

    virtual void buildCommands(
        const UIElement& element,
        std::vector<UIRenderCommand>& out) const = 0;
};
