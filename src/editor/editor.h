#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "editor/editor_layout.h"
#include "editor/hierarchy_panel.h"
#include "editor/inspector_panel.h"
#include "editor/panel.h"
#include "engine/defines.h"

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

// Edit - scene frozen, editor chrome visible (default when editor is open).
// Play - game sim runs while editor shell stays open (will be added later).
class Editor {
public:
    explicit Editor(Engine& engine);

    void init();
    void shutdown();
    void update();

    bool isOpen() const { return open_; }
    EditorPlayState playState() const { return playState_; }

    void setOpen(bool open);
    void toggleOpen();

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

    UIElementID rootId_ = INVALID_UI_ELEMENT;
    UIElementID placeholderLabelId_ = INVALID_UI_ELEMENT;
    UIElementID dockPreviewIndicatorId_ = INVALID_UI_ELEMENT;

    EditorPanel viewportPanel_;
    EditorPanel hierarchyPanel_;
    EditorPanel samplePanelB_;
    EditorPanel inspectorPanel_;
    HierarchyPanel hierarchyView_;
    InspectorPanel inspectorView_;
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
    void updateViewportInteraction();

    static ViewportDrag hitTestViewport(glm::vec2 mouse, glm::vec4 rect);
    static glm::vec4 applyViewportDrag(
        ViewportDrag drag,
        glm::vec4 start,
        glm::vec2 delta);
};
