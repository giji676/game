#include "editor/inspector_panel.h"

#include <cstdio>
#include <string>

#include "editor/panel.h"
#include "engine/asset_manager/object.h"
#include "engine/asset_manager/transform.h"
#include "engine/asset_manager/widgets.h"
#include "engine/scene.h"
#include "engine/ui.h"
#include "engine/ui/style.h"

namespace {

constexpr float kHeaderFontSize = 24.f;
constexpr float kRowFontSize = 20.f;

std::string formatVec3(const glm::vec3& v) {
    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f)", v.x, v.y, v.z);
    return std::string(buffer);
}

void styleInfoLabel(UIElement& element, float fontSize) {
    element.style.position = PositionMode::Relative;
    element.style.height = Length::px(fontSize + 10.f);
    element.style.width = Length::percent(100.f);
    element.transform.fontSize = fontSize;
}

} // namespace

void TransformView::bind(UI& ui, UIElementID parentId) {
    ui_ = &ui;
    rootId_ = ui.createElement();
    ui.reparent(rootId_, parentId);

    UIElement& root = ui.get(rootId_);
    root.style.position = PositionMode::Relative;
    root.style.display = Display::Block;
    root.style.gap = Length::px(4.f);
    root.style.width = Length::percent(100.f);
    root.style.padding.left = Length::px(2.f);

    posId_ = ui.label({0.f, 0.f}, {0.f, 0.f}, {0.88f, 0.88f, 0.9f, 1.f}, "Position: (0, 0, 0)");
    rotId_ = ui.label({0.f, 0.f}, {0.f, 0.f}, {0.88f, 0.88f, 0.9f, 1.f}, "Rotation: (0, 0, 0)");
    scaleId_ = ui.label({0.f, 0.f}, {0.f, 0.f}, {0.88f, 0.88f, 0.9f, 1.f}, "Scale: (1, 1, 1)");

    ui.reparent(posId_, rootId_);
    ui.reparent(rotId_, rootId_);
    ui.reparent(scaleId_, rootId_);

    styleInfoLabel(ui.get(posId_), kRowFontSize);
    styleInfoLabel(ui.get(rotId_), kRowFontSize);
    styleInfoLabel(ui.get(scaleId_), kRowFontSize);
}

void TransformView::update(const Transform& transform) {
    if (!ui_ || rootId_ == INVALID_UI_ELEMENT)
        return;

    if (auto* label = dynamic_cast<Label*>(ui_->get(posId_).widget.get()))
        label->text = "Position: " + formatVec3(transform.position());
    if (auto* label = dynamic_cast<Label*>(ui_->get(rotId_).widget.get()))
        label->text = "Rotation: " + formatVec3(transform.rotation());
    if (auto* label = dynamic_cast<Label*>(ui_->get(scaleId_).widget.get()))
        label->text = "Scale: " + formatVec3(transform.scale());
}

void InspectorPanel::bind(UI& ui, EditorPanel& panel) {
    ui_ = &ui;
    contentId_ = panel.contentId();
    built_ = true;

    UIElement& content = ui.get(contentId_);
    content.style.display = Display::Block;
    content.style.gap = Length::px(6.f);
    content.style.padding.left = Length::px(8.f);
    content.style.padding.right = Length::px(8.f);
    content.style.padding.top = Length::px(8.f);
    content.style.padding.bottom = Length::px(8.f);
    content.style.overflow = Overflow::Scroll;

    headerId_ = ui.label({0.f, 0.f}, {0.f, 0.f}, {0.95f, 0.95f, 0.98f, 1.f}, "Inspector");
    ui.reparent(headerId_, contentId_);
    styleInfoLabel(ui.get(headerId_), kHeaderFontSize);

    nameId_ = addInfoLabel("Name: (none)");
    idLabelId_ = addInfoLabel("ID: -");
    parentId_ = addInfoLabel("Parent: -");
    childrenId_ = addInfoLabel("Children: 0");
    scriptsId_ = addInfoLabel("Scripts: 0");
    modelId_ = addInfoLabel("Has Model: no");
    debugId_ = addInfoLabel("Debug: off");

    transformView_.bind(ui, contentId_);
}

UIElementID InspectorPanel::addInfoLabel(const char* text) {
    UIElementID id = ui_->label({0.f, 0.f}, {0.f, 0.f}, {0.82f, 0.82f, 0.86f, 1.f}, text);
    ui_->reparent(id, contentId_);
    styleInfoLabel(ui_->get(id), kRowFontSize);
    return id;
}

void InspectorPanel::setLabelText(UIElementID id, const char* value) const {
    if (id == INVALID_UI_ELEMENT || !ui_)
        return;
    if (auto* label = dynamic_cast<Label*>(ui_->get(id).widget.get()))
        label->text = value;
}

void InspectorPanel::setLabelText(UIElementID id, const std::string& value) const {
    if (id == INVALID_UI_ELEMENT || !ui_)
        return;
    if (auto* label = dynamic_cast<Label*>(ui_->get(id).widget.get()))
        label->text = value;
}

void InspectorPanel::update(Scene& scene) {
    if (!built_ || !ui_ || contentId_ == INVALID_UI_ELEMENT)
        return;

    if (selectedId_ == INVALID_OBJECT) {
        setLabelText(nameId_, "Name: (none selected)");
        setLabelText(idLabelId_, "ID: -");
        setLabelText(parentId_, "Parent: -");
        setLabelText(childrenId_, "Children: 0");
        setLabelText(scriptsId_, "Scripts: 0");
        setLabelText(modelId_, "Has Model: no");
        setLabelText(debugId_, "Debug: off");
        return;
    }

    const Object& obj = scene.get(selectedId_);
    const std::string displayName = obj.name.empty() ? ("Object " + std::to_string(selectedId_)) : obj.name;

    setLabelText(nameId_, "Name: " + displayName);
    setLabelText(idLabelId_, "ID: " + std::to_string(selectedId_));
    setLabelText(parentId_, "Parent: " + std::to_string(obj.parent));
    setLabelText(childrenId_, "Children: " + std::to_string(obj.children.size()));
    setLabelText(scriptsId_, "Scripts: " + std::to_string(obj.scripts.size()));
    setLabelText(modelId_, std::string("Has Model: ") + (obj.model ? "yes" : "no"));
    setLabelText(debugId_, std::string("Debug: ") + (obj.debug ? "on" : "off"));

    transformView_.update(obj.transform);
}
