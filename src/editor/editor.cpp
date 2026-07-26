#include "editor/editor.h"

#include "engine/asset_manager/widgets.h"
#include "engine/engine.h"
#include "engine/input.h"
#include "engine/ui.h"

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

    // Play/Stop and panel updates land here in later stages.
}

void Editor::setOpen(bool open) {
    if (open_ == open)
        return;

    open_ = open;

    if (open_) {
        wasPausedBeforeOpen_ = engine_.isPaused();
        playState_ = EditorPlayState::Edit;
        engine_.setPaused(true);
    } else {
        engine_.setPaused(wasPausedBeforeOpen_);
        if (!wasPausedBeforeOpen_)
            engine_.ui.onUnpause();
    }

    syncVisibility();
}

void Editor::toggleOpen() {
    setOpen(!open_);
}

void Editor::buildShell() {
    UI& ui = engine_.ui;

    rootId_ = ui.createElement();
    UIElement& root = ui.get(rootId_);
    root.visible = false;
    root.style.inset.left = Length::px(0.f);
    root.style.inset.right = Length::px(0.f);
    root.style.inset.top = Length::px(0.f);
    root.style.inset.bottom = Length::px(0.f);
    root.style.display = Display::Block;
    root.style.padding.left = Length::px(24.f);
    root.style.padding.right = Length::px(24.f);
    root.style.padding.top = Length::px(24.f);
    root.style.padding.bottom = Length::px(24.f);

    auto& bg = root.addWidget<Rect>();
    bg.color = {0.08f, 0.08f, 0.1f, 0.97f};

    placeholderLabelId_ = ui.label(
            {0.f, 0.f},
            {0.f, 24.f},
            {0.85f, 0.85f, 0.9f, 1.f},
            "Editor (stage 1) - F3 to close");
    ui.reparent(placeholderLabelId_, rootId_);
    ui.get(placeholderLabelId_).style.position = PositionMode::Relative;
    ui.get(placeholderLabelId_).style.height = Length::px(32.f);
}

void Editor::syncVisibility() {
    if (rootId_ != INVALID_UI_ELEMENT)
        engine_.ui.get(rootId_).visible = open_;
}
