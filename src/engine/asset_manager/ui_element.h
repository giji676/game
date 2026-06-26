#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "engine/defines.h"
#include "engine/asset_manager/ui_widget.h"

struct Transform2D {
    glm::vec2 position;
    glm::vec2 size;
    glm::vec2 anchor = {0.f, 0.f};
};

class UIElement {
public:
    Transform2D transform;
    bool visible = true;
    UIElementID parent = 0;
    std::vector<UIElementID> children;
    std::unique_ptr<UIWidget> widget;

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
