#pragma once

#include "editor/panel.h"

class Editor;
class UI;

// Play / Stop controls in the anchored top toolbar panel.
class ToolbarPanel {
public:
    void bind(UI& ui, EditorPanel& panel, Editor& editor);
    void update(Editor& editor);

private:
    UI* ui_ = nullptr;
    Editor* editor_ = nullptr;
    UIElementID playButtonId_ = INVALID_UI_ELEMENT;
    UIElementID stopButtonId_ = INVALID_UI_ELEMENT;
    UIElementID moveButtonId_ = INVALID_UI_ELEMENT;
    UIElementID rotateButtonId_ = INVALID_UI_ELEMENT;
    UIElementID scaleButtonId_ = INVALID_UI_ELEMENT;
    UIElementID statusLabelId_ = INVALID_UI_ELEMENT;
};
