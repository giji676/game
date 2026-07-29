#include "editor/editor.h"

#include <algorithm>
#include <cmath>

#include "engine/asset_manager/widgets.h"
#include "engine/engine.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "engine/utils/geometry.h"

namespace {

constexpr float kDefaultViewportScale = 0.6f;
constexpr float kMinViewportW = 192.f;
constexpr float kMinViewportH = 108.f;
constexpr float kResizeHandle = 10.f;

} // namespace

Editor::Editor(Engine& engine)
    : engine_(engine)
{}

void Editor::init() {
    buildShell();
    syncVisibility();
}

void Editor::shutdown() {
    open_ = false;
}

void Editor::update() {
    Input& input = engine_.input;

    if (input.pressed(Action::ToggleEditor))
        toggleOpen();

    if (!open_)
        return;

    const float winW = static_cast<float>(engine_.app.width());
    const float winH = static_cast<float>(engine_.app.height());

    // Reflow the dock tree whenever the OS window size changes.
    if (winW != lastWindowSize_.x || winH != lastWindowSize_.y) {
        lastWindowSize_ = {winW, winH};
        layout_.relayout({0.f, 0.f, winW, winH});
        applyLayoutRects();
    }

    for (EditorPanel* panel : panels_)
        panel->update(input, {winW, winH});

    if (open_)
        hierarchyView_.update(engine_.scene);

    syncResize();
    syncDocking();

    viewportRect_ = viewportPanel_.rect();
}

void Editor::setOpen(bool open) {
    if (open_ == open)
        return;

    open_ = open;

    if (open_) {
        wasPausedBeforeOpen_ = engine_.isPaused();
        playState_ = EditorPlayState::Edit;
        engine_.setPaused(true);
        if (!viewportInitialized_) {
            resetViewportDefault();
            viewportInitialized_ = true;
        } else {
            clampViewportToWindow();
        }
    } else {
        viewportDrag_ = ViewportDrag::None;
        engine_.setPaused(wasPausedBeforeOpen_);
        if (!wasPausedBeforeOpen_)
            engine_.gameUi.onUnpause();
        samplePanelB_.setVisible(false);
        samplePanelC_.setVisible(false);
        hierarchyPanel_.setVisible(false);
    }

    syncVisibility();
}

void Editor::toggleOpen() {
    setOpen(!open_);
}

void Editor::buildShell() {
    UI& ui = engine_.editorUi;

    rootId_ = ui.createElement();
    UIElement& root = ui.get(rootId_);
    root.visible = false;
    root.style.inset.left = Length::px(0.f);
    root.style.inset.right = Length::px(0.f);
    root.style.inset.top = Length::px(0.f);
    root.style.inset.bottom = Length::px(0.f);
    root.style.display = Display::Block;

    placeholderLabelId_ = ui.label(
            {0.f, 0.f},
            {0.f, 20.f},
            {0.85f, 0.85f, 0.9f, 1.f},
            "Editor - F3 to close  |  drag edges to resize, drag center to move");
    ui.reparent(placeholderLabelId_, rootId_);
    ui.get(placeholderLabelId_).style.position = PositionMode::Absolute;
    ui.get(placeholderLabelId_).style.inset.left = Length::px(12.f);
    ui.get(placeholderLabelId_).style.inset.top = Length::px(12.f);
    ui.get(placeholderLabelId_).style.width = Length::automatic();
    ui.get(placeholderLabelId_).style.height = Length::px(24.f);

    // Panel shells. Future Hierarchy / Inspector panels will reuse EditorPanel.
    const float winW = std::max(1.f, static_cast<float>(engine_.app.width()));
    const float winH = std::max(1.f, static_cast<float>(engine_.app.height()));
    viewportPanel_.build(
            ui,
            rootId_,
            "Viewport",
            {
                std::floor(winW/4),
                std::floor(winH/3),
                std::floor(2*winW/4),
                std::floor(2*winH/3),
            });
    viewportPanel_.setDockedMode(true);
    if (UIElement& viewportRoot = ui.get(viewportPanel_.rootId()); viewportRoot.widget) {
        auto* bg = static_cast<Rect*>(viewportRoot.widget.get());
        bg->color = {0.f, 0.f, 0.f, 0.f};
        bg->borderWidth = 2.f;
        bg->borderColor = {0.35f, 0.45f, 0.7f, 1.f};
    }

    hierarchyPanel_.build(
            ui,
            rootId_,
            "Hierarchy",
            {
                0.f,
                0.f,
                std::floor(winW/4),
                winH,
            });
    hierarchyPanel_.setDockedMode(true);
    hierarchyView_.bind(ui, hierarchyPanel_);

    samplePanelB_.build(
            ui,
            rootId_,
            "Panel B",
            {
                std::floor(winW/4),
                0.f,
                std::floor(2*winW/4),
                std::floor(winH/3),
            });
    samplePanelB_.setDockedMode(true);

    samplePanelC_.build(
            ui,
            rootId_,
            "Panel C",
            {
                std::floor(3*winW/4),
                0.f,
                std::floor(winW/4),
                winH
            });
    samplePanelC_.setDockedMode(true);

    panels_ = {&viewportPanel_, &hierarchyPanel_, &samplePanelB_, &samplePanelC_};

    dockPreviewIndicatorId_ = ui.createElement();
    ui.reparent(dockPreviewIndicatorId_, rootId_);
    UIElement& indicator = ui.get(dockPreviewIndicatorId_);
    indicator.visible = false;
    indicator.style.position = PositionMode::Absolute;
    indicator.style.display = Display::Block;
    auto& indicatorRect = indicator.addWidget<Rect>();
    indicatorRect.color = {0.3f, 0.55f, 1.f, 0.35f};
    indicatorRect.borderWidth = 2.f;
    indicatorRect.borderColor = {0.55f, 0.75f, 1.f, 0.95f};

    layout_.initialize(
        {viewportPanel_.rootId(), hierarchyPanel_.rootId(),
         samplePanelB_.rootId(), samplePanelC_.rootId()},
        {viewportPanel_.rect(), hierarchyPanel_.rect(),
         samplePanelB_.rect(), samplePanelC_.rect()},
        {0.f, 0.f, winW, winH});
    applyLayoutRects();
    viewportRect_ = viewportPanel_.rect();
    lastWindowSize_ = {winW, winH};
}

void Editor::syncVisibility() {
    if (rootId_ != INVALID_UI_ELEMENT)
        engine_.editorUi.get(rootId_).visible = open_;
    viewportPanel_.setVisible(open_);
    hierarchyPanel_.setVisible(open_);
    samplePanelB_.setVisible(open_);
    samplePanelC_.setVisible(open_);
}

void Editor::resetViewportDefault() {
    const float winW = std::max(1.f, static_cast<float>(engine_.app.width()));
    const float winH = std::max(1.f, static_cast<float>(engine_.app.height()));

    const float vw = std::max(kMinViewportW, std::floor(winW * kDefaultViewportScale));
    const float vh = std::max(kMinViewportH, std::floor(winH * kDefaultViewportScale));
    viewportRect_ = {
        std::floor((winW - vw) * 0.5f),
        std::floor((winH - vh) * 0.5f),
        vw,
        vh,
    };
}

void Editor::clampViewportToWindow() {
    const float winW = std::max(1.f, static_cast<float>(engine_.app.width()));
    const float winH = std::max(1.f, static_cast<float>(engine_.app.height()));

    float x = viewportRect_.x;
    float y = viewportRect_.y;
    float w = viewportRect_.z;
    float h = viewportRect_.w;

    // Clip overflow on each edge instead.
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

    w = std::max(kMinViewportW, std::min(w, winW));
    h = std::max(kMinViewportH, std::min(h, winH));

    if (x + w > winW)
        x = winW - w;
    if (y + h > winH)
        y = winH - h;

    x = clampf(x, 0.f, std::max(0.f, winW - w));
    y = clampf(y, 0.f, std::max(0.f, winH - h));

    viewportRect_ = {x, y, w, h};
}

void Editor::applyLayoutRects() {
    for (EditorPanel* panel : panels_) {
        panel->setRect(layout_.panelRect(panel->rootId()));
    }
}

void Editor::syncResize() {
    Input& input = engine_.input;
    const glm::vec2 mouse = input.mousePosition();
    const float winW = static_cast<float>(engine_.app.width());
    const float winH = static_cast<float>(engine_.app.height());

    EditorPanel* resizingPanel = nullptr;
    for (EditorPanel* panel : panels_) {
        if (panel->isResizing()) {
            resizingPanel = panel;
            break;
        }
    }

    if (!resizingPanel) {
        activeResizePanelId_ = INVALID_UI_ELEMENT;
        activeResizeDrag_ = PanelDrag::None;
        resizeHandleX_ = {};
        resizeHandleY_ = {};
        return;
    }

    const PanelDrag drag = resizingPanel->dragMode();
    if (activeResizePanelId_ != resizingPanel->rootId() || activeResizeDrag_ != drag) {
        activeResizePanelId_ = resizingPanel->rootId();
        activeResizeDrag_ = drag;
        resizeStartMouse_ = mouse;
        resizeHandleX_ = {};
        resizeHandleY_ = {};

        switch (drag) {
            case PanelDrag::Left:
            case PanelDrag::Right:
                resizeHandleX_ = layout_.beginResize(activeResizePanelId_, drag);
                break;
            case PanelDrag::Top:
            case PanelDrag::Bottom:
                resizeHandleY_ = layout_.beginResize(activeResizePanelId_, drag);
                break;
            case PanelDrag::BottomLeft:
                resizeHandleX_ = layout_.beginResize(activeResizePanelId_, PanelDrag::Left);
                resizeHandleY_ = layout_.beginResize(activeResizePanelId_, PanelDrag::Bottom);
                break;
            case PanelDrag::BottomRight:
                resizeHandleX_ = layout_.beginResize(activeResizePanelId_, PanelDrag::Right);
                resizeHandleY_ = layout_.beginResize(activeResizePanelId_, PanelDrag::Bottom);
                break;
            case PanelDrag::TopLeft:
                resizeHandleX_ = layout_.beginResize(activeResizePanelId_, PanelDrag::Left);
                resizeHandleY_ = layout_.beginResize(activeResizePanelId_, PanelDrag::Top);
                break;
            case PanelDrag::TopRight:
                resizeHandleX_ = layout_.beginResize(activeResizePanelId_, PanelDrag::Right);
                resizeHandleY_ = layout_.beginResize(activeResizePanelId_, PanelDrag::Top);
                break;
            default:
                break;
        }
    }

    bool changed = false;
    if (resizeHandleX_.valid) {
        changed |= layout_.updateResize(resizeHandleX_, mouse.x - resizeStartMouse_.x);
    }
    if (resizeHandleY_.valid) {
        changed |= layout_.updateResize(resizeHandleY_, mouse.y - resizeStartMouse_.y);
    }

    if (changed) {
        layout_.relayout({0.f, 0.f, winW, winH});
        applyLayoutRects();
    }
}

void Editor::syncDocking() {
    if (dockPreviewIndicatorId_ == INVALID_UI_ELEMENT)
        return;

    UI& ui = engine_.editorUi;
    UIElement& indicator = ui.get(dockPreviewIndicatorId_);
    Input& input = engine_.input;
    const glm::vec2 mouse = input.mousePosition();

    // Docking/relocation is title-bar move only.
    EditorPanel* draggingPanel = nullptr;
    for (EditorPanel* panel : panels_) {
        if (panel->isMoving()) {
            draggingPanel = panel;
            break;
        }
    }

    if (!draggingPanel) {
        if (input.released(MouseAction::Left) &&
            activeDragPanelId_ != INVALID_UI_ELEMENT &&
            activeDockPreview_.valid) {
            if (layout_.dockPanel(activeDragPanelId_, activeDockPreview_)) {
                const float winW = static_cast<float>(engine_.app.width());
                const float winH = static_cast<float>(engine_.app.height());
                layout_.relayout({0.f, 0.f, winW, winH});
                applyLayoutRects();
            }
        }

        indicator.visible = false;
        activeDragPanelId_ = INVALID_UI_ELEMENT;
        activeDockPreview_ = {};
        return;
    }

    activeDragPanelId_ = draggingPanel->rootId();
    activeDockPreview_ = layout_.findDockPreview(*draggingPanel, mouse);
    if (activeDockPreview_.valid) {
        indicator.visible = true;
        indicator.style.inset.left = Length::px(activeDockPreview_.previewRect.x);
        indicator.style.inset.bottom = Length::px(activeDockPreview_.previewRect.y);
        indicator.style.width = Length::px(activeDockPreview_.previewRect.z);
        indicator.style.height = Length::px(activeDockPreview_.previewRect.w);
    } else {
        indicator.visible = false;
    }

    if (input.released(MouseAction::Left) && activeDragPanelId_ != INVALID_UI_ELEMENT) {
        if (layout_.dockPanel(activeDragPanelId_, activeDockPreview_)) {
            const float winW = static_cast<float>(engine_.app.width());
            const float winH = static_cast<float>(engine_.app.height());
            layout_.relayout({0.f, 0.f, winW, winH});
            applyLayoutRects();
        }
        activeDragPanelId_ = INVALID_UI_ELEMENT;
        activeDockPreview_ = {};
        indicator.visible = false;
    }
}

ViewportDrag Editor::hitTestViewport(glm::vec2 mouse, glm::vec4 r) {
    const float x = r.x;
    const float y = r.y;
    const float w = r.z;
    const float h = r.w;
    const float hs = kResizeHandle;

    if (!pointInRect(
            mouse,
            {x - hs, y - hs},
            {w + hs * 2.f, h + hs * 2.f}))
        return ViewportDrag::None;

    const bool left   = mouse.x <= x + hs;
    const bool right  = mouse.x >= x + w - hs;
    const bool bottom = mouse.y <= y + hs;
    const bool top    = mouse.y >= y + h - hs;

    if (bottom && left)  return ViewportDrag::BottomLeft;
    if (bottom && right) return ViewportDrag::BottomRight;
    if (top && left)     return ViewportDrag::TopLeft;
    if (top && right)    return ViewportDrag::TopRight;
    if (left)            return ViewportDrag::Left;
    if (right)           return ViewportDrag::Right;
    if (bottom)          return ViewportDrag::Bottom;
    if (top)             return ViewportDrag::Top;

    return ViewportDrag::Move;
}

glm::vec4 Editor::applyViewportDrag(
        ViewportDrag drag,
        glm::vec4 start,
        glm::vec2 delta)
{
    float x = start.x;
    float y = start.y;
    float w = start.z;
    float h = start.w;

    switch (drag) {
        case ViewportDrag::Move:
            x += delta.x;
            y += delta.y;
            break;
        case ViewportDrag::Left:
            x += delta.x;
            w -= delta.x;
            break;
        case ViewportDrag::Right:
            w += delta.x;
            break;
        case ViewportDrag::Bottom:
            y += delta.y;
            h -= delta.y;
            break;
        case ViewportDrag::Top:
            h += delta.y;
            break;
        case ViewportDrag::BottomLeft:
            x += delta.x;
            w -= delta.x;
            y += delta.y;
            h -= delta.y;
            break;
        case ViewportDrag::BottomRight:
            w += delta.x;
            y += delta.y;
            h -= delta.y;
            break;
        case ViewportDrag::TopLeft:
            x += delta.x;
            w -= delta.x;
            h += delta.y;
            break;
        case ViewportDrag::TopRight:
            w += delta.x;
            h += delta.y;
            break;
        default:
            break;
    }

    if (w < kMinViewportW) {
        if (drag == ViewportDrag::Left ||
            drag == ViewportDrag::BottomLeft ||
            drag == ViewportDrag::TopLeft) {
            x -= kMinViewportW - w;
        }
        w = kMinViewportW;
    }

    if (h < kMinViewportH) {
        if (drag == ViewportDrag::Bottom ||
            drag == ViewportDrag::BottomLeft ||
            drag == ViewportDrag::BottomRight) {
            y -= kMinViewportH - h;
        }
        h = kMinViewportH;
    }

    return {x, y, w, h};
}

void Editor::updateViewportInteraction() {
    Input& input = engine_.input;
    const glm::vec2 mouse = input.mousePosition();

    if (input.pressed(MouseAction::Left)) {
        viewportDrag_ = hitTestViewport(mouse, viewportRect_);
        if (viewportDrag_ != ViewportDrag::None) {
            dragStartMouse_ = mouse;
            dragStartRect_ = viewportRect_;
        }
    }

    if (input.down(MouseAction::Left) && viewportDrag_ != ViewportDrag::None) {
        const glm::vec2 delta = mouse - dragStartMouse_;
        viewportRect_ = applyViewportDrag(viewportDrag_, dragStartRect_, delta);
        clampViewportToWindow();
    }

    if (input.released(MouseAction::Left))
        viewportDrag_ = ViewportDrag::None;
}

glm::vec4 Editor::gameViewportRect() const {
    const float winW = std::max(1.f, static_cast<float>(engine_.app.width()));
    const float winH = std::max(1.f, static_cast<float>(engine_.app.height()));

    if (!open_)
        return {0.f, 0.f, winW, winH};

    const UIElement& content = engine_.editorUi.get(viewportPanel_.contentId());
    const glm::vec4 contentRect = {
        content.transform.position.x,
        content.transform.position.y,
        content.transform.size.x,
        content.transform.size.y
    };
    if (contentRect.z > 1.f && contentRect.w > 1.f)
        return contentRect;

    // During a dock-tree mutation frame, layout sizes can be transiently zero.
    if (viewportRect_.z > 1.f && viewportRect_.w > 1.f)
        return viewportRect_;

    return {0.f, 0.f, winW, winH};
}
