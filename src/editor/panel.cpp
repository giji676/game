#include "editor/panel.h"

#include <algorithm>
#include <cmath>

#include "engine/asset_manager/widgets.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "engine/utils/geometry.h"

void EditorPanel::build(
        UI& ui,
        UIElementID parent,
        const std::string& title,
        glm::vec4 rect)
{
    ui_ = &ui;
    title_ = title;
    rect_ = rect;

    rootId_ = ui.createElement();
    ui.reparent(rootId_, parent);
    UIElement& root = ui.get(rootId_);
    root.visible = false;
    root.style.position = PositionMode::Absolute;
    root.style.display = Display::Flex;
    root.style.flexDirection = FlexDirection::Column;
    root.style.alignItems = AlignItems::Stretch;
    root.style.overflow = Overflow::Hidden;

    auto& bg = root.addWidget<Rect>();
    bg.color = {0.12f, 0.12f, 0.14f, 0.96f};
    bg.borderWidth = 1.f;
    bg.borderColor = {0.3f, 0.3f, 0.35f, 1.f};

    titleBarId_ = ui.createElement();
    ui.reparent(titleBarId_, rootId_);
    UIElement& titleBar = ui.get(titleBarId_);
    titleBar.style.position = PositionMode::Relative;
    titleBar.style.height = Length::px(kTitleBarH);
    titleBar.style.flexGrow = 0.f;
    titleBar.style.display = Display::Flex;
    titleBar.style.flexDirection = FlexDirection::Row;
    titleBar.style.alignItems = AlignItems::Center;
    titleBar.style.padding.left = Length::px(10.f);
    titleBar.style.padding.right = Length::px(10.f);

    auto& titleBg = titleBar.addWidget<Rect>();
    titleBg.color = {0.16f, 0.16f, 0.19f, 1.f};

    titleLabelId_ = ui.label(
            {0.f, 0.f},
            {0.f, 16.f},
            {0.85f, 0.85f, 0.9f, 1.f},
            title_);
    ui.reparent(titleLabelId_, titleBarId_);
    ui.get(titleLabelId_).style.position = PositionMode::Relative;
    ui.get(titleLabelId_).style.height = Length::px(20.f);

    contentId_ = ui.createElement();
    ui.reparent(contentId_, rootId_);
    UIElement& content = ui.get(contentId_);
    content.style.position = PositionMode::Relative;
    content.style.flexGrow = 1.f;
    content.style.flexBasis = Length::px(0.f);
    content.style.display = Display::Block;
    content.style.padding.left = Length::px(8.f);
    content.style.padding.right = Length::px(8.f);
    content.style.padding.top = Length::px(8.f);
    content.style.padding.bottom = Length::px(8.f);
    content.style.overflow = Overflow::Scroll;

    sync();
}

void EditorPanel::setVisible(bool visible) {
    if (!isBuilt())
        return;
    ui_->get(rootId_).visible = visible;
}

void EditorPanel::setRect(glm::vec4 rect) {
    rect_ = rect;
    sync();
}

void EditorPanel::sync() {
    if (!isBuilt())
        return;

    UIElement& root = ui_->get(rootId_);
    root.style.inset.left = Length::px(rect_.x);
    root.style.inset.bottom = Length::px(rect_.y);
    root.style.width = Length::px(rect_.z);
    root.style.height = Length::px(rect_.w);
}

void EditorPanel::clampToWindow(glm::vec2 windowSize) {
    const float winW = std::max(1.f, windowSize.x);
    const float winH = std::max(1.f, windowSize.y);

    float x = rect_.x;
    float y = rect_.y;
    float w = rect_.z;
    float h = rect_.w;

    if (x < 0.f) {
        w += x;
        x = 0.f;
    }
    if (y < 0.f) {
        h += y;
        y = 0.f;
    }
    if (x + w > winW)
        w = winW - x;
    if (y + h > winH)
        h = winH - y;

    w = std::max(kMinW, std::min(w, winW));
    h = std::max(kMinH, std::min(h, winH));

    if (x + w > winW)
        x = winW - w;
    if (y + h > winH)
        y = winH - h;

    x = clampf(x, 0.f, std::max(0.f, winW - w));
    y = clampf(y, 0.f, std::max(0.f, winH - h));

    rect_ = {x, y, w, h};
}

PanelDrag EditorPanel::hitTest(glm::vec2 mouse) const {
    const float x = rect_.x;
    const float y = rect_.y;
    const float w = rect_.z;
    const float h = rect_.w;
    const float hs = kResizeHandle;

    if (!pointInRect(mouse, {x - hs, y - hs}, {w + hs * 2.f, h + hs * 2.f}))
        return PanelDrag::None;

    const bool left   = mouse.x <= x + hs;
    const bool right  = mouse.x >= x + w - hs;
    const bool bottom = mouse.y <= y + hs;
    const bool top    = mouse.y >= y + h - hs;

    if (bottom && left)  return PanelDrag::BottomLeft;
    if (bottom && right) return PanelDrag::BottomRight;
    if (top && left)     return PanelDrag::TopLeft;
    if (top && right)    return PanelDrag::TopRight;
    if (left)            return PanelDrag::Left;
    if (right)           return PanelDrag::Right;
    if (bottom)          return PanelDrag::Bottom;
    if (top)             return PanelDrag::Top;

    // Move only from the title bar so content stays clickable later.
    const float titleBottom = y + h - kTitleBarH;
    if (pointInRect(mouse, {x, titleBottom}, {w, kTitleBarH}))
        return PanelDrag::Move;

    return PanelDrag::None;
}

glm::vec4 EditorPanel::applyDrag(
        PanelDrag drag,
        glm::vec4 start,
        glm::vec2 delta)
{
    float x = start.x;
    float y = start.y;
    float w = start.z;
    float h = start.w;

    switch (drag) {
        case PanelDrag::Move:
            x += delta.x;
            y += delta.y;
            break;
        case PanelDrag::Left:
            x += delta.x;
            w -= delta.x;
            break;
        case PanelDrag::Right:
            w += delta.x;
            break;
        case PanelDrag::Bottom:
            y += delta.y;
            h -= delta.y;
            break;
        case PanelDrag::Top:
            h += delta.y;
            break;
        case PanelDrag::BottomLeft:
            x += delta.x;
            w -= delta.x;
            y += delta.y;
            h -= delta.y;
            break;
        case PanelDrag::BottomRight:
            w += delta.x;
            y += delta.y;
            h -= delta.y;
            break;
        case PanelDrag::TopLeft:
            x += delta.x;
            w -= delta.x;
            h += delta.y;
            break;
        case PanelDrag::TopRight:
            w += delta.x;
            h += delta.y;
            break;
        default:
            break;
    }

    if (w < kMinW) {
        if (drag == PanelDrag::Left ||
            drag == PanelDrag::BottomLeft ||
            drag == PanelDrag::TopLeft) {
            x -= kMinW - w;
        }
        w = kMinW;
    }

    if (h < kMinH) {
        if (drag == PanelDrag::Bottom ||
            drag == PanelDrag::BottomLeft ||
            drag == PanelDrag::BottomRight) {
            y -= kMinH - h;
        }
        h = kMinH;
    }

    return {x, y, w, h};
}

void EditorPanel::update(Input& input, glm::vec2 windowSize) {
    if (!isBuilt())
        return;

    const glm::vec2 mouse = input.mousePosition();

    if (input.pressed(MouseAction::Left)) {
        drag_ = hitTest(mouse);
        if (drag_ != PanelDrag::None) {
            dragStartMouse_ = mouse;
            dragStartRect_ = rect_;
        }
    }

    if (input.down(MouseAction::Left) && drag_ != PanelDrag::None) {
        rect_ = applyDrag(drag_, dragStartRect_, mouse - dragStartMouse_);
        clampToWindow(windowSize);
        sync();
    }

    if (input.released(MouseAction::Left))
        drag_ = PanelDrag::None;

    clampToWindow(windowSize);
    sync();
}
