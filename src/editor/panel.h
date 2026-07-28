#pragma once

#include <glm/glm.hpp>
#include <string>

#include "engine/defines.h"

class UI;
class Input;

// Shared drag/resize mode for floating editor chrome. Snap targets can hook into
// applyDrag / clamp later without changing the interaction model.
enum class PanelDrag {
    None,
    Move,
    Left,
    Right,
    Bottom,
    Top,
    BottomLeft,
    BottomRight,
    TopLeft,
    TopRight,
};

// Floating editor panel shell: title bar + empty content region.
// Hierarchy / Inspector / etc. will parent their widgets into contentId().
class EditorPanel {
public:
    EditorPanel() = default;

    void build(
        UI& ui,
        UIElementID parent,
        const std::string& title,
        glm::vec4 rect);

    void update(Input& input, glm::vec2 windowSize);
    void setVisible(bool visible);
    void setRect(glm::vec4 rect);
    void setDockedMode(bool docked) { dockedMode_ = docked; }
    void sync();

    bool isDragging() const { return drag_ != PanelDrag::None; }
    bool isMoving() const { return drag_ == PanelDrag::Move; }
    bool isResizing() const { return drag_ != PanelDrag::None && drag_ != PanelDrag::Move; }
    PanelDrag dragMode() const { return drag_; }
    bool isBuilt() const { return rootId_ != INVALID_UI_ELEMENT; }
    glm::vec4 rect() const { return rect_; }
    UIElementID rootId() const { return rootId_; }
    UIElementID contentId() const { return contentId_; }
    UIElementID titleBarId() const { return titleBarId_; }
    const std::string& title() const { return title_; }

    static constexpr float kTitleBarH = 28.f;
    static constexpr float kMinW = 180.f;
    static constexpr float kMinH = 120.f;
    static constexpr float kResizeHandle = 8.f;

private:
    UI* ui_ = nullptr;
    std::string title_;
    glm::vec4 rect_ = {0.f, 0.f, 280.f, 360.f};

    UIElementID rootId_ = INVALID_UI_ELEMENT;
    UIElementID titleBarId_ = INVALID_UI_ELEMENT;
    UIElementID titleLabelId_ = INVALID_UI_ELEMENT;
    UIElementID contentId_ = INVALID_UI_ELEMENT;

    PanelDrag drag_ = PanelDrag::None;
    glm::vec2 dragStartMouse_ = {0.f, 0.f};
    glm::vec4 dragStartRect_ = {0.f, 0.f, 0.f, 0.f};
    bool dockedMode_ = false;

    void clampToWindow(glm::vec2 windowSize);
    PanelDrag hitTest(glm::vec2 mouse) const;
    static glm::vec4 applyDrag(PanelDrag drag, glm::vec4 start, glm::vec2 delta);
};
