#pragma once

#include <string>

#include "engine/defines.h"

class EditorPanel;
class Scene;
class Transform;
class UI;

// Reusable transform display block for inspector-like UIs.
class TransformView {
public:
    void bind(UI& ui, UIElementID parentId);
    void update(const Transform& transform);

private:
    UI* ui_ = nullptr;
    UIElementID rootId_ = INVALID_UI_ELEMENT;
    UIElementID posId_ = INVALID_UI_ELEMENT;
    UIElementID rotId_ = INVALID_UI_ELEMENT;
    UIElementID scaleId_ = INVALID_UI_ELEMENT;
};

class InspectorPanel {
public:
    void bind(UI& ui, EditorPanel& panel);
    void setSelectedId(ObjectID id) { selectedId_ = id; }
    void update(Scene& scene);

private:
    void setLabelText(UIElementID id, const char* value) const;
    void setLabelText(UIElementID id, const std::string& value) const;
    UIElementID addInfoLabel(const char* text);

    UI* ui_ = nullptr;
    UIElementID contentId_ = INVALID_UI_ELEMENT;
    ObjectID selectedId_ = INVALID_OBJECT;
    bool built_ = false;

    UIElementID headerId_ = INVALID_UI_ELEMENT;
    UIElementID nameId_ = INVALID_UI_ELEMENT;
    UIElementID idLabelId_ = INVALID_UI_ELEMENT;
    UIElementID parentId_ = INVALID_UI_ELEMENT;
    UIElementID childrenId_ = INVALID_UI_ELEMENT;
    UIElementID scriptsId_ = INVALID_UI_ELEMENT;
    UIElementID modelId_ = INVALID_UI_ELEMENT;
    UIElementID debugId_ = INVALID_UI_ELEMENT;

    TransformView transformView_;
};
