#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "editor/editor_camera.h"
#include "editor/editor_layout.h"
#include "editor/hierarchy_panel.h"
#include "editor/inspector_panel.h"
#include "editor/panel.h"
#include "editor/toolbar_panel.h"
#include "engine/camera.h"
#include "engine/defines.h"
#include "engine/raycasting.h"

class Engine;

enum class EditorPlayState {
    Edit,
    Play,
};

enum class ViewportDrag {
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

enum class GizmoHandle {
    None,
    AxisX,
    AxisY,
    AxisZ,
    PlaneXY,
    PlaneXZ,
    PlaneYZ,
};

// Edit - scene frozen, editor chrome visible (default when editor is open).
// Play - game sim runs while editor shell stays open (will be added later).
class Editor {
public:
    explicit Editor(Engine& engine);

    void init();
    void shutdown();
    void update();
    // After scene world matrices are updated (input + fresh pivots).
    void updateEditorCamera();

    bool isOpen() const { return open_; }
    EditorPlayState playState() const { return playState_; }

    void setOpen(bool open);
    void toggleOpen();
    void setPlayState(EditorPlayState state);
    // Leave editor-tool control without changing engine pause.
    // Used when ESC opens the in-game pause menu over an open editor.
    void exitEditMode();
    bool isEditing() const {
        return open_ && playState_ == EditorPlayState::Edit;
    }

    Camera* editModeCamera() { return &editorCamera_.camera(); }

    // Window-space x, y, w, h the game renders into. While the editor is open
    // this is the floating, resizable sub-window; otherwise it is the whole window.
    glm::vec4 gameViewportRect() const;

private:
    Engine& engine_;

    bool open_ = false;
    bool wasPausedBeforeOpen_ = false;
    EditorPlayState playState_ = EditorPlayState::Edit;

    bool viewportInitialized_ = false;
    glm::vec4 viewportRect_ = {0.f, 0.f, 0.f, 0.f};
    glm::vec2 lastWindowSize_ = {0.f, 0.f};

    ViewportDrag viewportDrag_ = ViewportDrag::None;
    glm::vec2 dragStartMouse_ = {0.f, 0.f};
    glm::vec4 dragStartRect_ = {0.f, 0.f, 0.f, 0.f};

    bool objectDragging_ = false;
    ObjectID dragObjectId_ = INVALID_OBJECT;
    GizmoHandle gizmoActiveHandle_ = GizmoHandle::None;
    GizmoHandle gizmoHoveredHandle_ = GizmoHandle::None;
    glm::vec3 dragPlanePoint_{0.f};
    glm::vec3 dragPlaneNormal_{0.f, 0.f, 1.f};
    glm::vec3 dragAxis_{0.f};
    glm::vec3 dragLastHit_{0.f};

    UIElementID rootId_ = INVALID_UI_ELEMENT;
    UIElementID dockPreviewIndicatorId_ = INVALID_UI_ELEMENT;

    EditorPanel toolbarPanel_;
    EditorPanel viewportPanel_;
    EditorPanel hierarchyPanel_;
    EditorPanel samplePanelB_;
    EditorPanel inspectorPanel_;
    HierarchyPanel hierarchyView_;
    InspectorPanel inspectorView_;
    ToolbarPanel toolbarView_;
    EditorCamera editorCamera_;
    EditorLayout layout_;
    std::vector<EditorPanel*> panels_;

    UIElementID activeDragPanelId_ = INVALID_UI_ELEMENT;
    EditorLayout::DockPreview activeDockPreview_;

    UIElementID activeResizePanelId_ = INVALID_UI_ELEMENT;
    PanelDrag activeResizeDrag_ = PanelDrag::None;
    glm::vec2 resizeStartMouse_ = {0.f, 0.f};
    EditorLayout::ResizeHandle resizeHandleX_;
    EditorLayout::ResizeHandle resizeHandleY_;

    void buildShell();
    void syncVisibility();
    void resetViewportDefault();
    void clampViewportToWindow();
    void syncDocking();
    void syncResize();
    void applyLayoutRects();
    void syncAnchoredPanels(float winW, float winH);
    glm::vec4 dockableBounds(float winW, float winH) const;
    void updateViewportInteraction();
    void updateSceneInteraction();

    bool makeViewportRay(glm::vec2 mouse, Ray& outRay) const;
    float gizmoWorldSize(const glm::vec3& origin) const;
    GizmoHandle pickGizmoHandle(
        const Ray& ray,
        const glm::vec3& origin,
        float size) const;
    bool beginGizmoDrag(
        GizmoHandle handle,
        const Ray& ray,
        ObjectID id,
        const glm::vec3& origin);
    void drawTranslateGizmo(const glm::vec3& origin, float size);
    void clearGizmoDrag();

    static ViewportDrag hitTestViewport(glm::vec2 mouse, glm::vec4 rect);
    static glm::vec4 applyViewportDrag(
        ViewportDrag drag,
        glm::vec4 start,
        glm::vec2 delta);
};
