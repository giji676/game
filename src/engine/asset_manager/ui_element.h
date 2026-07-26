#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "engine/defines.h"
#include "engine/asset_manager/ui_widget.h"
#include "engine/ui/style.h"

struct Transform2D {
    glm::vec2 position;
    glm::vec2 size;
    // Normalized point on the element (0 = bottom-left, 1 = top-right) used with
    // inset positioning. For top/right insets, anchor 0 means the far edge sits
    // on the inset line (CSS default); for bottom/left, anchor 0 means the
    // near edge sits on the inset line. Use 0.5 to center on both axes.
    glm::vec2 anchor = {0.f, 0.f};
    float fontSize = 0.f;
};

class UIElement {
public:
    Transform2D transform;
    Style style;
    bool visible = true;
    UIElementID parent = 0;
    std::vector<UIElementID> children;
    std::unique_ptr<UIWidget> widget;

    // Scroll state (containers with overflow: Scroll).
    glm::vec2 scrollOffset = {0.f, 0.f};
    glm::vec2 scrollContentSize = {0.f, 0.f};
    glm::vec2 scrollViewportSize = {0.f, 0.f};

    template <typename T, typename... Args>
    T& addWidget(Args&&... args) {
        widget = std::make_unique<T>(std::forward<Args>(args)...);
        return *static_cast<T*>(widget.get());
    }

    UIElementID getID() const { return ID; }
    void setID(UIElementID id) { ID = id; }
private:
    UIElementID ID;
};
