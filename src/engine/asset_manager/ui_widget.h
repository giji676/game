#pragma once

#include "engine/renderer/ui_renderer.h"
#include "engine/input.h"

class UIElement;

class UIWidget {
public:
    virtual ~UIWidget() = default;

    virtual void tick(const UIElement& e, float dt) {}

    virtual void updateInput(
            const UIElement& e,
            const glm::vec2& mouse) {}

    virtual void buildCommands(
        const UIElement& element,
        std::vector<UIRenderCommand>& out) const = 0;

    virtual void onFocusGained(void) {}
    virtual void onFocusLost(void) {}
    virtual void onTextInput(const std::string& text) {}
    virtual void onKeyInput(Key key) {}
    virtual void resetInteraction() {}
};
