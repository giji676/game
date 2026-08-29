#include "editor/toolbar_panel.h"

#include "editor/editor.h"
#include "engine/asset_manager/font.h"
#include "engine/asset_manager/widgets.h"
#include "engine/engine.h"
#include "engine/ui.h"
#include "engine/ui/style.h"

namespace {

constexpr float kToolbarFontSize = 16.f;
constexpr float kButtonGap = 8.f;
constexpr float kGroupGap = 16.f;

void styleToolbarButton(UIElement& el, bool active = false) {
    auto* btn = dynamic_cast<Button*>(el.widget.get());
    if (!btn)
        return;
    btn->centerText = true;
    btn->normal.bgColor = active
        ? glm::vec4{0.16f, 0.38f, 0.72f, 1.f}
        : glm::vec4{0.2f, 0.22f, 0.28f, 1.f};
    btn->normal.textColor = {0.92f, 0.92f, 0.96f, 1.f};
    btn->normal.borderColor = active
        ? glm::vec4{0.35f, 0.55f, 0.9f, 1.f}
        : glm::vec4{0.34f, 0.36f, 0.42f, 1.f};
    btn->normal.borderWidth = 1.f;
    btn->hoveredStyle.bgColor = {0.28f, 0.3f, 0.36f, 1.f};
    btn->hoveredStyle.borderColor = {0.45f, 0.48f, 0.55f, 1.f};
    btn->pressedStyle.bgColor = {0.16f, 0.38f, 0.72f, 1.f};
    btn->pressedStyle.borderColor = {0.35f, 0.55f, 0.9f, 1.f};
    btn->cornerRadii = {4.f, 4.f, 4.f, 4.f};
    btn->padding = {8.f, 4.f};
}

void layoutToolbarButton(UI& ui, UIElementID id, float marginRight = kButtonGap) {
    UIElement& el = ui.get(id);
    el.style.position = PositionMode::Relative;
    el.style.height = Length::px(28.f);
    el.style.width = Length::automatic();
    el.style.margin.right = Length::px(marginRight);
    el.style.textOverflow = TextOverflow::Visible;
}

} // namespace

void ToolbarPanel::bind(UI& ui, EditorPanel& panel, Editor& editor) {
    ui_ = &ui;
    editor_ = &editor;
    UIElementID contentId = panel.contentId();
    Font& font = ENGINE().assets.getFont("InterVariable");

    UIElement& content = ui.get(contentId);
    content.style.display = Display::Flex;
    content.style.flexDirection = FlexDirection::Row;
    content.style.alignItems = AlignItems::Center;
    content.style.padding.left = Length::px(10.f);
    content.style.padding.right = Length::px(56.f);
    content.style.padding.top = Length::px(0.f);
    content.style.padding.bottom = Length::px(0.f);
    content.style.overflow = Overflow::Hidden;

    playButtonId_ = ui.button({0.f, 0.f}, kToolbarFontSize, "Play", &font);
    stopButtonId_ = ui.button({0.f, 0.f}, kToolbarFontSize, "Edit", &font);
    moveButtonId_ = ui.button({0.f, 0.f}, kToolbarFontSize, "Move (W)", &font);
    rotateButtonId_ = ui.button({0.f, 0.f}, kToolbarFontSize, "Rotate (E)", &font);
    scaleButtonId_ = ui.button({0.f, 0.f}, kToolbarFontSize, "Scale (R)", &font);
    localSpaceButtonId_ = ui.button({0.f, 0.f}, kToolbarFontSize, "Local (L)", &font);
    worldSpaceButtonId_ = ui.button({0.f, 0.f}, kToolbarFontSize, "World (Shift+W)", &font);
    statusLabelId_ = ui.label(
        {0.f, 0.f},
        {0.f, kToolbarFontSize},
        {0.72f, 0.76f, 0.84f, 1.f},
        "Edit");

    ui.reparent(playButtonId_, contentId);
    ui.reparent(stopButtonId_, contentId);
    ui.reparent(moveButtonId_, contentId);
    ui.reparent(rotateButtonId_, contentId);
    ui.reparent(scaleButtonId_, contentId);
    ui.reparent(localSpaceButtonId_, contentId);
    ui.reparent(worldSpaceButtonId_, contentId);
    ui.reparent(statusLabelId_, contentId);

    layoutToolbarButton(ui, playButtonId_);
    styleToolbarButton(ui.get(playButtonId_));
    if (auto* playBtn = dynamic_cast<Button*>(ui.get(playButtonId_).widget.get()))
        playBtn->onClick = [this]() {
            if (editor_ && editor_->isEditing())
                editor_->setPlayState(EditorPlayState::Play);
        };

    layoutToolbarButton(ui, stopButtonId_, kGroupGap);
    styleToolbarButton(ui.get(stopButtonId_));
    if (auto* stopBtn = dynamic_cast<Button*>(ui.get(stopButtonId_).widget.get()))
        stopBtn->onClick = [this]() {
            if (editor_ && !editor_->isEditing())
                editor_->setPlayState(EditorPlayState::Edit);
        };

    layoutToolbarButton(ui, moveButtonId_);
    styleToolbarButton(ui.get(moveButtonId_));
    if (auto* moveBtn = dynamic_cast<Button*>(ui.get(moveButtonId_).widget.get()))
        moveBtn->onClick = [this]() {
            if (editor_)
                editor_->setGizmoMode(GizmoMode::Move);
        };

    layoutToolbarButton(ui, rotateButtonId_);
    styleToolbarButton(ui.get(rotateButtonId_));
    if (auto* rotateBtn = dynamic_cast<Button*>(ui.get(rotateButtonId_).widget.get()))
        rotateBtn->onClick = [this]() {
            if (editor_)
                editor_->setGizmoMode(GizmoMode::Rotate);
        };

    layoutToolbarButton(ui, scaleButtonId_, kGroupGap);
    styleToolbarButton(ui.get(scaleButtonId_));
    if (auto* scaleBtn = dynamic_cast<Button*>(ui.get(scaleButtonId_).widget.get()))
        scaleBtn->onClick = [this]() {
            if (editor_)
                editor_->setGizmoMode(GizmoMode::Scale);
        };

    layoutToolbarButton(ui, localSpaceButtonId_);
    styleToolbarButton(ui.get(localSpaceButtonId_));
    if (auto* localBtn = dynamic_cast<Button*>(ui.get(localSpaceButtonId_).widget.get()))
        localBtn->onClick = [this]() {
            if (editor_)
                editor_->setGizmoSpace(GizmoSpace::Local);
        };

    layoutToolbarButton(ui, worldSpaceButtonId_, kGroupGap);
    styleToolbarButton(ui.get(worldSpaceButtonId_));
    if (auto* worldBtn = dynamic_cast<Button*>(ui.get(worldSpaceButtonId_).widget.get()))
        worldBtn->onClick = [this]() {
            if (editor_)
                editor_->setGizmoSpace(GizmoSpace::World);
        };

    UIElement& statusEl = ui.get(statusLabelId_);
    statusEl.style.position = PositionMode::Absolute;
    statusEl.style.inset.right = Length::px(10.f);
    statusEl.style.inset.bottom = Length::px(6.f);
    statusEl.style.height = Length::px(24.f);
}

void ToolbarPanel::update(Editor& editor) {
    if (!ui_ || playButtonId_ == INVALID_UI_ELEMENT)
        return;

    const bool editing = editor.isEditing();
    const GizmoMode mode = editor.gizmoMode();
    const GizmoSpace space = editor.gizmoSpace();

    UIElement& playEl = ui_->get(playButtonId_);
    UIElement& stopEl = ui_->get(stopButtonId_);
    UIElement& moveEl = ui_->get(moveButtonId_);
    UIElement& rotateEl = ui_->get(rotateButtonId_);
    UIElement& scaleEl = ui_->get(scaleButtonId_);
    UIElement& localEl = ui_->get(localSpaceButtonId_);
    UIElement& worldEl = ui_->get(worldSpaceButtonId_);
    UIElement& statusEl = ui_->get(statusLabelId_);

    playEl.visible = editing;
    stopEl.visible = !editing;
    moveEl.visible = editing;
    rotateEl.visible = editing;
    scaleEl.visible = editing;
    localEl.visible = editing;
    worldEl.visible = editing;

    styleToolbarButton(moveEl, mode == GizmoMode::Move);
    styleToolbarButton(rotateEl, mode == GizmoMode::Rotate);
    styleToolbarButton(scaleEl, mode == GizmoMode::Scale);
    styleToolbarButton(localEl, space == GizmoSpace::Local);
    styleToolbarButton(worldEl, space == GizmoSpace::World);

    auto* statusLbl = dynamic_cast<Label*>(statusEl.widget.get());
    if (statusLbl)
        statusLbl->text = editing ? "Edit" : "Play";
}
