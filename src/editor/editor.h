#pragma once

#include "engine/defines.h"

class Engine;

// Edit - scene frozen, editor chrome visible (default when editor is open).
// Play - game sim runs while editor shell stays open (will be added later).
enum class EditorPlayState {
    Edit,
    Play,
};

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

private:
    Engine& engine_;

    bool open_ = false;
    bool wasPausedBeforeOpen_ = false;
    EditorPlayState playState_ = EditorPlayState::Edit;

    UIElementID rootId_ = INVALID_UI_ELEMENT;
    UIElementID placeholderLabelId_ = INVALID_UI_ELEMENT;

    // Reserved for later layout split:
    // UIElementID viewportPanelId_;
    // UIElementID chromePanelId_;

    void buildShell();
    void syncVisibility();
};
