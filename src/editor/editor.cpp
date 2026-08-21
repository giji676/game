#include "editor/editor.h"

#include <algorithm>
#include <cmath>
#include <cfloat>

#include <glm/gtc/matrix_transform.hpp>

#include "engine/asset_manager/object.h"
#include "engine/asset_manager/widgets.h"
#include "engine/camera.h"
#include "engine/engine.h"
#include "engine/input.h"
#include "engine/renderer/debug_renderer.h"
#include "engine/scene.h"
#include "engine/ui.h"
#include "engine/utils/geometry.h"

namespace {

constexpr float kDefaultViewportScale = 0.6f;
constexpr float kMinViewportW = 192.f;
constexpr float kMinViewportH = 108.f;
constexpr float kResizeHandle = 10.f;

constexpr float kGizmoScreenFactor = 0.12f;
constexpr float kGizmoPlanePadStart = 0.2f;
constexpr float kGizmoPlanePadEnd = 0.45f;
constexpr float kGizmoAxisPickRadius = 0.1f;

constexpr glm::vec3 kGizmoColorX{1.f, 0.2f, 0.2f};
constexpr glm::vec3 kGizmoColorY{0.2f, 1.f, 0.2f};
constexpr glm::vec3 kGizmoColorZ{0.25f, 0.45f, 1.f};
constexpr glm::vec3 kGizmoColorHighlight{1.f, 1.f, 0.25f};

glm::vec3 gizmoAxisVector(GizmoHandle handle) {
    switch (handle) {
    case GizmoHandle::AxisX: return {1.f, 0.f, 0.f};
    case GizmoHandle::AxisY: return {0.f, 1.f, 0.f};
    case GizmoHandle::AxisZ: return {0.f, 0.f, 1.f};
    default: return {0.f, 0.f, 0.f};
    }
}

glm::vec3 gizmoPlaneNormal(GizmoHandle handle) {
    switch (handle) {
    case GizmoHandle::PlaneXY: return {0.f, 0.f, 1.f};
    case GizmoHandle::PlaneXZ: return {0.f, 1.f, 0.f};
    case GizmoHandle::PlaneYZ: return {1.f, 0.f, 0.f};
    default: return {0.f, 0.f, 1.f};
    }
}

void gizmoPlaneAxes(GizmoHandle handle, glm::vec3& u, glm::vec3& v) {
    switch (handle) {
    case GizmoHandle::PlaneXY:
        u = {1.f, 0.f, 0.f};
        v = {0.f, 1.f, 0.f};
        break;
    case GizmoHandle::PlaneXZ:
        u = {1.f, 0.f, 0.f};
        v = {0.f, 0.f, 1.f};
        break;
    case GizmoHandle::PlaneYZ:
        u = {0.f, 1.f, 0.f};
        v = {0.f, 0.f, 1.f};
        break;
    default:
        u = {1.f, 0.f, 0.f};
        v = {0.f, 1.f, 0.f};
        break;
    }
}

bool isAxisHandle(GizmoHandle handle) {
    return handle == GizmoHandle::AxisX ||
           handle == GizmoHandle::AxisY ||
           handle == GizmoHandle::AxisZ;
}

bool isPlaneHandle(GizmoHandle handle) {
    return handle == GizmoHandle::PlaneXY ||
           handle == GizmoHandle::PlaneXZ ||
           handle == GizmoHandle::PlaneYZ;
}

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

    if (input.pressed(Action::ToggleEditor)) {
        // When editor is open but gameplay control is active, F3 should return
        // to editor control instead of closing the editor entirely.
        if (open_ && playState_ == EditorPlayState::Play) {
            setPlayState(EditorPlayState::Edit);
        } else {
            toggleOpen();
        }
    }

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

    for (EditorPanel* panel : panels_) {
        if (isEditing())
            panel->update(input, {winW, winH});
        else
            panel->clearDrag();
    }

    if (open_) {
        hierarchyView_.update(engine_.scene, isEditing());
        updateSceneInteraction();
        inspectorView_.setSelectedId(hierarchyView_.selectedId());
        inspectorView_.setEditable(isEditing());
        inspectorView_.setObjectDrag(
            hierarchyView_.draggingId(),
            hierarchyView_.releasedDragId());
        inspectorView_.update(engine_.scene);

        if (isEditing()) {
            const ObjectID selectedId = hierarchyView_.selectedId();
            if (selectedId != INVALID_OBJECT &&
                selectedId != engine_.scene.getRoot()) {
                const Object& obj = engine_.scene.get(selectedId);
                const glm::vec3 origin = glm::vec3(obj.worldMatrix[3]);
                const float size = gizmoWorldSize(origin);
                drawTranslateGizmo(origin, size);
            }
        }
    }

    if (isEditing()) {
        syncResize();
        syncDocking();
    }

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
        clearGizmoDrag();
        engine_.setPaused(wasPausedBeforeOpen_);
        if (!wasPausedBeforeOpen_)
            engine_.gameUi.onUnpause();
        samplePanelB_.setVisible(false);
        inspectorPanel_.setVisible(false);
        hierarchyPanel_.setVisible(false);
    }

    syncVisibility();
}

void Editor::toggleOpen() {
    setOpen(!open_);
}

void Editor::setPlayState(EditorPlayState state) {
    playState_ = state;
    if (!open_)
        return;

    if (state == EditorPlayState::Edit) {
        engine_.setPaused(true);
    } else {
        clearGizmoDrag();
        engine_.editorUi.onUnpause();
    }
}

void Editor::exitEditMode() {
    if (!open_ || playState_ != EditorPlayState::Edit)
        return;

    playState_ = EditorPlayState::Play;
    clearGizmoDrag();
    engine_.editorUi.onUnpause();
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
            "Editor - F3 to close  |  drag translate gizmo to move");
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

    inspectorPanel_.build(
            ui,
            rootId_,
            "Inspector",
            {
                std::floor(3*winW/4),
                0.f,
                std::floor(winW/4),
                winH
            });
    inspectorPanel_.setDockedMode(true);
    inspectorView_.bind(ui, inspectorPanel_);

    panels_ = {&viewportPanel_, &hierarchyPanel_, &samplePanelB_, &inspectorPanel_};

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
         samplePanelB_.rootId(), inspectorPanel_.rootId()},
        {viewportPanel_.rect(), hierarchyPanel_.rect(),
         samplePanelB_.rect(), inspectorPanel_.rect()},
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
    inspectorPanel_.setVisible(open_);
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

bool Editor::makeViewportRay(glm::vec2 mouse, Ray& outRay) const {
    const glm::vec4 vp = gameViewportRect();
    if (vp.z <= 1.f || vp.w <= 1.f)
        return false;
    if (!pointInRect(mouse, {vp.x, vp.y}, {vp.z, vp.w}))
        return false;

    const float ndcX = ((mouse.x - vp.x) / vp.z) * 2.f - 1.f;
    const float ndcY = ((mouse.y - vp.y) / vp.w) * 2.f - 1.f;

    const Camera& camera = *engine_.getActiveCamera();
    const glm::mat4 view = camera.view();
    const glm::mat4 projection = glm::perspective(
        glm::radians(45.f),
        vp.z / vp.w,
        0.1f,
        100.0f);
    const glm::mat4 invVP = glm::inverse(projection * view);

    glm::vec4 nearH = invVP * glm::vec4(ndcX, ndcY, -1.f, 1.f);
    if (std::abs(nearH.w) < 1e-8f)
        return false;
    nearH /= nearH.w;

    const glm::vec3 target(nearH);
    const glm::vec3 dir = target - camera.pos;
    if (glm::dot(dir, dir) < 1e-12f)
        return false;

    outRay.origin = camera.pos;
    outRay.direction = glm::normalize(dir);
    outRay.t = FLT_MAX;
    return true;
}

void Editor::clearGizmoDrag() {
    objectDragging_ = false;
    dragObjectId_ = INVALID_OBJECT;
    gizmoActiveHandle_ = GizmoHandle::None;
    dragAxis_ = {0.f, 0.f, 0.f};
}

float Editor::gizmoWorldSize(const glm::vec3& origin) const {
    const Camera* camera = engine_.getActiveCamera();
    if (!camera)
        return 1.f;
    const float dist = glm::length(camera->pos - origin);
    return std::max(0.05f, dist * kGizmoScreenFactor);
}

GizmoHandle Editor::pickGizmoHandle(
    const Ray& ray,
    const glm::vec3& origin,
    float size) const
{
    const float pad0 = size * kGizmoPlanePadStart;
    const float pad1 = size * kGizmoPlanePadEnd;

    GizmoHandle bestPlane = GizmoHandle::None;
    float bestPlaneT = FLT_MAX;

    const GizmoHandle planes[] = {
        GizmoHandle::PlaneXY,
        GizmoHandle::PlaneXZ,
        GizmoHandle::PlaneYZ,
    };
    for (GizmoHandle handle : planes) {
        glm::vec3 u, v;
        gizmoPlaneAxes(handle, u, v);
        const glm::vec3 normal = gizmoPlaneNormal(handle);

        glm::vec3 hit;
        if (!Raycasting::testPlaneIntersection(ray, origin, normal, hit))
            continue;

        const glm::vec3 rel = hit - origin;
        const float du = glm::dot(rel, u);
        const float dv = glm::dot(rel, v);
        if (du < pad0 || du > pad1 || dv < pad0 || dv > pad1)
            continue;

        const float t = glm::length(hit - ray.origin);
        if (t < bestPlaneT) {
            bestPlaneT = t;
            bestPlane = handle;
        }
    }
    if (bestPlane != GizmoHandle::None)
        return bestPlane;

    GizmoHandle bestAxis = GizmoHandle::None;
    float bestAxisDist = size * kGizmoAxisPickRadius;
    float bestAxisT = FLT_MAX;
    const float pickRadius = size * kGizmoAxisPickRadius;

    const GizmoHandle axes[] = {
        GizmoHandle::AxisX,
        GizmoHandle::AxisY,
        GizmoHandle::AxisZ,
    };
    for (GizmoHandle handle : axes) {
        const glm::vec3 axis = gizmoAxisVector(handle);
        const glm::vec3 tip = origin + axis * size;
        float dist = 0.f;
        float rayT = 0.f;
        if (!Raycasting::testRaySegmentDistance(ray, origin, tip, dist, rayT))
            continue;
        if (dist > pickRadius)
            continue;
        if (dist < bestAxisDist - 1e-6f ||
            (std::abs(dist - bestAxisDist) < 1e-6f && rayT < bestAxisT)) {
            bestAxisDist = dist;
            bestAxisT = rayT;
            bestAxis = handle;
        }
    }
    return bestAxis;
}

bool Editor::beginGizmoDrag(
    GizmoHandle handle,
    const Ray& ray,
    ObjectID id,
    const glm::vec3& origin)
{
    const Camera* camera = engine_.getActiveCamera();
    if (!camera)
        return false;

    dragObjectId_ = id;
    gizmoActiveHandle_ = handle;
    dragPlanePoint_ = origin;
    dragAxis_ = {0.f, 0.f, 0.f};

    if (isAxisHandle(handle)) {
        const glm::vec3 axis = gizmoAxisVector(handle);
        dragAxis_ = axis;
        const glm::vec3 camDir = glm::normalize(camera->front);
        glm::vec3 planeNormal = glm::cross(axis, glm::cross(axis, camDir));
        if (glm::dot(planeNormal, planeNormal) < 1e-8f) {
            const glm::vec3 fallback =
                (std::abs(axis.y) < 0.9f)
                    ? glm::vec3(0.f, 1.f, 0.f)
                    : glm::vec3(1.f, 0.f, 0.f);
            planeNormal = glm::cross(axis, fallback);
        }
        dragPlaneNormal_ = glm::normalize(planeNormal);
    } else if (isPlaneHandle(handle)) {
        dragPlaneNormal_ = gizmoPlaneNormal(handle);
    } else {
        return false;
    }

    objectDragging_ = Raycasting::testPlaneIntersection(
        ray, dragPlanePoint_, dragPlaneNormal_, dragLastHit_);
    if (!objectDragging_) {
        clearGizmoDrag();
        return false;
    }
    return true;
}

void Editor::drawTranslateGizmo(const glm::vec3& origin, float size) {
    DebugRenderer& debug = engine_.debugRenderer;

    const auto colorFor = [this](GizmoHandle handle, const glm::vec3& base) {
        if (handle == gizmoActiveHandle_ || handle == gizmoHoveredHandle_)
            return kGizmoColorHighlight;
        return base;
    };

    // depthTest=false: gizmo always draws in front of scene geometry
    debug.arrow(origin, {1.f, 0.f, 0.f}, size, colorFor(GizmoHandle::AxisX, kGizmoColorX), false);
    debug.arrow(origin, {0.f, 1.f, 0.f}, size, colorFor(GizmoHandle::AxisY, kGizmoColorY), false);
    debug.arrow(origin, {0.f, 0.f, 1.f}, size, colorFor(GizmoHandle::AxisZ, kGizmoColorZ), false);

    const float pad0 = size * kGizmoPlanePadStart;
    const float pad1 = size * kGizmoPlanePadEnd;

    const auto drawPad = [&](GizmoHandle handle, const glm::vec3& baseColor) {
        glm::vec3 u, v;
        gizmoPlaneAxes(handle, u, v);
        const glm::vec3 a = origin + u * pad0 + v * pad0;
        const glm::vec3 b = origin + u * pad1 + v * pad0;
        const glm::vec3 c = origin + u * pad1 + v * pad1;
        const glm::vec3 d = origin + u * pad0 + v * pad1;
        debug.quadOutline(a, b, c, d, colorFor(handle, baseColor), false);
    };

    drawPad(GizmoHandle::PlaneXY, glm::mix(kGizmoColorX, kGizmoColorY, 0.5f));
    drawPad(GizmoHandle::PlaneXZ, glm::mix(kGizmoColorX, kGizmoColorZ, 0.5f));
    drawPad(GizmoHandle::PlaneYZ, glm::mix(kGizmoColorY, kGizmoColorZ, 0.5f));
}

void Editor::updateSceneInteraction() {
    Input& input = engine_.input;

    if (!isEditing() ||
        hierarchyView_.isDragging() ||
        activeDragPanelId_ != INVALID_UI_ELEMENT ||
        activeResizePanelId_ != INVALID_UI_ELEMENT) {
        clearGizmoDrag();
        gizmoHoveredHandle_ = GizmoHandle::None;
        return;
    }

    const glm::vec2 mouse = input.mousePosition();
    const glm::vec4 vp = gameViewportRect();
    const bool mouseInViewport = pointInRect(mouse, {vp.x, vp.y}, {vp.z, vp.w});

    const ObjectID selectedId = hierarchyView_.selectedId();
    const bool hasSelection =
        selectedId != INVALID_OBJECT &&
        selectedId != engine_.scene.getRoot();

    glm::vec3 gizmoOrigin{0.f};
    float gizmoSize = 1.f;
    if (hasSelection) {
        gizmoOrigin = glm::vec3(engine_.scene.get(selectedId).worldMatrix[3]);
        gizmoSize = gizmoWorldSize(gizmoOrigin);
    }

    if (!objectDragging_) {
        gizmoHoveredHandle_ = GizmoHandle::None;
        if (hasSelection && mouseInViewport) {
            Ray hoverRay;
            if (makeViewportRay(mouse, hoverRay))
                gizmoHoveredHandle_ = pickGizmoHandle(hoverRay, gizmoOrigin, gizmoSize);
        }
    } else {
        gizmoHoveredHandle_ = gizmoActiveHandle_;
    }

    if (input.pressed(MouseAction::Left) && mouseInViewport) {
        Ray ray;
        if (!makeViewportRay(mouse, ray)) {
            clearGizmoDrag();
            return;
        }

        if (hasSelection) {
            const GizmoHandle handle = pickGizmoHandle(ray, gizmoOrigin, gizmoSize);
            if (handle != GizmoHandle::None) {
                beginGizmoDrag(handle, ray, selectedId, gizmoOrigin);
                return;
            }
        }

        const RaycastHit hit = engine_.raycasting.castRay(ray);
        if (hit.distance < FLT_MAX &&
            hit.object != INVALID_OBJECT &&
            hit.object != engine_.scene.getRoot()) {
            hierarchyView_.setSelectedId(hit.object);
            inspectorView_.setSelectedId(hit.object);
            clearGizmoDrag();
        } else {
            hierarchyView_.setSelectedId(INVALID_OBJECT);
            inspectorView_.setSelectedId(INVALID_OBJECT);
            clearGizmoDrag();
        }
    }

    if (objectDragging_ &&
        dragObjectId_ != INVALID_OBJECT &&
        gizmoActiveHandle_ != GizmoHandle::None &&
        input.down(MouseAction::Left)) {
        Ray ray;
        const glm::vec2 clampedMouse = {
            clampf(mouse.x, vp.x, vp.x + vp.z),
            clampf(mouse.y, vp.y, vp.y + vp.w),
        };
        if (!makeViewportRay(clampedMouse, ray))
            return;

        glm::vec3 hitPoint;
        if (!Raycasting::testPlaneIntersection(
                ray, dragPlanePoint_, dragPlaneNormal_, hitPoint))
            return;

        glm::vec3 worldDelta = hitPoint - dragLastHit_;
        dragLastHit_ = hitPoint;

        if (isAxisHandle(gizmoActiveHandle_)) {
            const float along = glm::dot(worldDelta, dragAxis_);
            worldDelta = dragAxis_ * along;
        }

        if (glm::dot(worldDelta, worldDelta) < 1e-12f)
            return;

        Object& obj = engine_.scene.get(dragObjectId_);
        const Object& parent = engine_.scene.get(obj.parent);
        const glm::vec3 localDelta = glm::vec3(
            parent.worldInvMatrix * glm::vec4(worldDelta, 0.f));
        obj.transform.setPosition(obj.transform.position() + localDelta);
    }

    if (input.released(MouseAction::Left))
        clearGizmoDrag();
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
