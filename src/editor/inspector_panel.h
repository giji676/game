#pragma once

#include <glm/glm.hpp>

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
    void update(Transform& transform, bool editable);
    void setEditable(bool editable);

private:
    bool parseFloat(const std::string& text, float& out) const;
    void applyPendingEdits(Transform& transform);

    struct AxisFields {
        UIElementID xId = INVALID_UI_ELEMENT;
        UIElementID yId = INVALID_UI_ELEMENT;
        UIElementID zId = INVALID_UI_ELEMENT;
        std::string pendingX;
        std::string pendingY;
        std::string pendingZ;
        bool dirtyX = false;
        bool dirtyY = false;
        bool dirtyZ = false;
    };

    UI* ui_ = nullptr;
    UIElementID rootId_ = INVALID_UI_ELEMENT;
    AxisFields pos_;
    AxisFields rot_;
    AxisFields scale_;
    bool editable_ = true;
};

class InspectorPanel {
public:
    void bind(UI& ui, EditorPanel& panel);
    void setSelectedId(ObjectID id) { selectedId_ = id; }
    void setEditable(bool editable) { editable_ = editable; }
    void update(Scene& scene);

private:
    void setLabelText(UIElementID id, const char* value) const;
    void setLabelText(UIElementID id, const std::string& value) const;
    UIElementID addInfoLabel(const char* text);

    UI* ui_ = nullptr;
    UIElementID contentId_ = INVALID_UI_ELEMENT;
    ObjectID selectedId_ = INVALID_OBJECT;
    bool editable_ = true;
    bool built_ = false;

    UIElementID headerId_ = INVALID_UI_ELEMENT;
    UIElementID nameId_ = INVALID_UI_ELEMENT;
    UIElementID idLabelId_ = INVALID_UI_ELEMENT;
    UIElementID parentId_ = INVALID_UI_ELEMENT;
    UIElementID childrenId_ = INVALID_UI_ELEMENT;
    UIElementID scriptsId_ = INVALID_UI_ELEMENT;
    UIElementID componentsId_ = INVALID_UI_ELEMENT;
    UIElementID modelId_ = INVALID_UI_ELEMENT;
    UIElementID debugId_ = INVALID_UI_ELEMENT;

    TransformView transformView_;
};
