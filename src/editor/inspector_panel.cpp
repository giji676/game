#include "editor/inspector_panel.h"

#include <algorithm>
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
constexpr float kAxisFontSize = 18.f;

std::string formatFloat(float v) {
    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "%.3f", v);
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

    auto createRow = [&](const char* title, AxisFields& fields) {
        UIElementID titleId = ui.label({0.f, 0.f}, {0.f, 0.f}, {0.85f, 0.85f, 0.9f, 1.f}, title);
        ui.reparent(titleId, rootId_);
        styleInfoLabel(ui.get(titleId), kRowFontSize);

        UIElementID rowId = ui.createElement();
        ui.reparent(rowId, rootId_);
        UIElement& rowEl = ui.get(rowId);
        rowEl.style.position = PositionMode::Relative;
        rowEl.style.display = Display::Flex;
        rowEl.style.flexDirection = FlexDirection::Row;
        rowEl.style.gap = Length::px(6.f);
        rowEl.style.width = Length::percent(100.f);
        rowEl.style.height = Length::px(kAxisFontSize + 12.f);

        fields.xId = ui.inputField({0.f, 0.f}, {0.f, 0.f}, kAxisFontSize, "X");
        fields.yId = ui.inputField({0.f, 0.f}, {0.f, 0.f}, kAxisFontSize, "Y");
        fields.zId = ui.inputField({0.f, 0.f}, {0.f, 0.f}, kAxisFontSize, "Z");
        ui.reparent(fields.xId, rowId);
        ui.reparent(fields.yId, rowId);
        ui.reparent(fields.zId, rowId);
        for (UIElementID id : {fields.xId, fields.yId, fields.zId}) {
            UIElement& el = ui.get(id);
            el.style.position = PositionMode::Relative;
            el.style.flexGrow = 1.f;
            el.style.flexBasis = Length::px(0.f);
            el.style.height = Length::percent(100.f);
        }
    };
    createRow("Position", pos_);
    createRow("Rotation", rot_);
    createRow("Scale", scale_);

    auto setupField = [&](UIElementID id, const char* placeholder, std::string* pendingText, bool* dirty) {
        UIElement& el = ui.get(id);
        auto* field = dynamic_cast<InputField*>(el.widget.get());
        if (!field)
            return;
        field->placeholder = placeholder;
        field->normal.bgColor = {0.13f, 0.13f, 0.15f, 1.f};
        field->normal.textColor = {0.9f, 0.9f, 0.95f, 1.f};
        field->normal.borderColor = {0.28f, 0.28f, 0.34f, 1.f};
        field->normal.borderWidth = 1.f;
        field->hoveredStyle.borderColor = {0.42f, 0.42f, 0.52f, 1.f};
        field->focusedStyle.borderColor = {0.35f, 0.55f, 0.9f, 1.f};
        field->focusedStyle.borderWidth = 2.f;
        field->disabledStyle.bgColor = {0.09f, 0.09f, 0.11f, 1.f};
        field->disabledStyle.textColor = {0.62f, 0.62f, 0.68f, 1.f};
        field->disabledStyle.borderColor = {0.2f, 0.2f, 0.24f, 1.f};
        field->disabledStyle.borderWidth = 1.f;
        field->cornerRadii = {4.f, 4.f, 4.f, 4.f};
        field->padding = {8.f, 6.f};
        field->onSubmit = [pendingText, dirty](const std::string& value) {
            *pendingText = value;
            *dirty = true;
        };
    };
    auto setupAxis = [&](AxisFields& f) {
        setupField(f.xId, "X", &f.pendingX, &f.dirtyX);
        setupField(f.yId, "Y", &f.pendingY, &f.dirtyY);
        setupField(f.zId, "Z", &f.pendingZ, &f.dirtyZ);
    };
    setupAxis(pos_);
    setupAxis(rot_);
    setupAxis(scale_);
}

bool TransformView::parseFloat(const std::string& text, float& out) const {
    char tail = '\0';
    if (std::sscanf(text.c_str(), " %f %c", &out, &tail) == 1)
        return true;
    return false;
}

void TransformView::applyPendingEdits(Transform& transform) {
    auto applyAxis = [&](AxisFields& f, glm::vec3 current, auto setFn) {
        float parsed = 0.f;
        bool changed = false;
        if (f.dirtyX) {
            if (parseFloat(f.pendingX, parsed)) { current.x = parsed; changed = true; }
            f.dirtyX = false;
        }
        if (f.dirtyY) {
            if (parseFloat(f.pendingY, parsed)) { current.y = parsed; changed = true; }
            f.dirtyY = false;
        }
        if (f.dirtyZ) {
            if (parseFloat(f.pendingZ, parsed)) { current.z = parsed; changed = true; }
            f.dirtyZ = false;
        }
        if (changed)
            setFn(current);
    };
    applyAxis(pos_, transform.position(), [&](const glm::vec3& v) { transform.setPosition(v); });
    applyAxis(rot_, transform.rotation(), [&](const glm::vec3& v) { transform.setRotation(v); });
    applyAxis(scale_, transform.scale(), [&](const glm::vec3& v) { transform.setScale(v); });
}

void TransformView::setEditable(bool editable) {
    editable_ = editable;
    if (!ui_)
        return;
    auto setFieldDisabled = [&](UIElementID id) {
        if (id == INVALID_UI_ELEMENT)
            return;
        auto* field = dynamic_cast<InputField*>(ui_->get(id).widget.get());
        if (!field)
            return;
        field->disabled = !editable_;
    };
    auto setTriplet = [&](const AxisFields& f) {
        setFieldDisabled(f.xId);
        setFieldDisabled(f.yId);
        setFieldDisabled(f.zId);
    };
    setTriplet(pos_);
    setTriplet(rot_);
    setTriplet(scale_);
}

void TransformView::update(Transform& transform, bool editable) {
    if (!ui_ || rootId_ == INVALID_UI_ELEMENT)
        return;

    setEditable(editable);

    if (editable_)
        applyPendingEdits(transform);

    auto syncField = [&](UIElementID id, const std::string& value) {
        auto* field = dynamic_cast<InputField*>(ui_->get(id).widget.get());
        if (!field)
            return;
        // Do not clobber active typing while editor control is active.
        if (editable_ && field->focused)
            return;
        field->text = value;
        field->caretPos = std::min(field->caretPos, field->text.size());
    };
    const glm::vec3 pos = transform.position();
    const glm::vec3 rot = transform.rotation();
    const glm::vec3 scale = transform.scale();
    syncField(pos_.xId, formatFloat(pos.x));
    syncField(pos_.yId, formatFloat(pos.y));
    syncField(pos_.zId, formatFloat(pos.z));
    syncField(rot_.xId, formatFloat(rot.x));
    syncField(rot_.yId, formatFloat(rot.y));
    syncField(rot_.zId, formatFloat(rot.z));
    syncField(scale_.xId, formatFloat(scale.x));
    syncField(scale_.yId, formatFloat(scale.y));
    syncField(scale_.zId, formatFloat(scale.z));
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
        transformView_.setEditable(false);
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

    Object& editableObj = scene.get(selectedId_);
    transformView_.update(editableObj.transform, editable_);
}
