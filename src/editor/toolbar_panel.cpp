#include "editor/toolbar_panel.h"

#include "editor/editor.h"
#include "engine/asset_manager/font.h"
#include "engine/asset_manager/widgets.h"
#include "engine/engine.h"
#include "engine/ui.h"
#include "engine/ui/style.h"

namespace {

constexpr float kToolbarFontSize = 18.f;
constexpr float kButtonGap = 8.f;

void styleToolbarButton(UIElement& el) {
    auto* btn = dynamic_cast<Button*>(el.widget.get());
    if (!btn)
        return;
    btn->centerText = true;
    btn->normal.bgColor = {0.2f, 0.22f, 0.28f, 1.f};
    btn->normal.textColor = {0.92f, 0.92f, 0.96f, 1.f};
    btn->normal.borderColor = {0.34f, 0.36f, 0.42f, 1.f};
    btn->normal.borderWidth = 1.f;
    btn->hoveredStyle.bgColor = {0.28f, 0.3f, 0.36f, 1.f};
    btn->hoveredStyle.borderColor = {0.45f, 0.48f, 0.55f, 1.f};
    btn->pressedStyle.bgColor = {0.16f, 0.38f, 0.72f, 1.f};
    btn->pressedStyle.borderColor = {0.35f, 0.55f, 0.9f, 1.f};
    btn->cornerRadii = {4.f, 4.f, 4.f, 4.f};
    btn->padding = {10.f, 4.f};
}

} // namespace

void ToolbarPanel::bind(UI& ui, EditorPanel& panel, Editor& editor) {
    ui_ = &ui;
    editor_ = &editor;
    UIElementID contentId = panel.contentId();
    Font &font = ENGINE().assets.getFont("InterVariable");

    UIElement& content = ui.get(contentId);
    content.style.display = Display::Flex;
    content.style.flexDirection = FlexDirection::Row;
    content.style.alignItems = AlignItems::Center;
    content.style.padding.left = Length::px(10.f);
    content.style.padding.right = Length::px(10.f);
    content.style.padding.top = Length::px(0.f);
    content.style.padding.bottom = Length::px(0.f);
    content.style.overflow = Overflow::Hidden;

    playButtonId_ = ui.button({0.f, 0.f}, kToolbarFontSize, "Play", &font);
    stopButtonId_ = ui.button({0.f, 0.f}, kToolbarFontSize, "Edit", &font);
    statusLabelId_ = ui.label(
        {0.f, 0.f},
        {0.f, kToolbarFontSize},
        {0.72f, 0.76f, 0.84f, 1.f},
        "Edit");

    ui.reparent(playButtonId_, contentId);
    ui.reparent(stopButtonId_, contentId);
    ui.reparent(statusLabelId_, contentId);

    UIElement& playEl = ui.get(playButtonId_);
    playEl.style.position = PositionMode::Relative;
    playEl.style.height = Length::px(28.f);
    playEl.style.width = Length::px(72.f);
    playEl.style.margin.right = Length::px(kButtonGap);
    styleToolbarButton(playEl);
    if (auto* playBtn = dynamic_cast<Button*>(playEl.widget.get()))
        playBtn->onClick = [this]() {
            if (editor_ && editor_->isEditing())
                editor_->setPlayState(EditorPlayState::Play);
        };

    UIElement& stopEl = ui.get(stopButtonId_);
    stopEl.style.position = PositionMode::Relative;
    stopEl.style.height = Length::px(28.f);
    stopEl.style.width = Length::px(72.f);
    stopEl.style.margin.right = Length::px(16.f);
    styleToolbarButton(stopEl);
    if (auto* stopBtn = dynamic_cast<Button*>(stopEl.widget.get()))
        stopBtn->onClick = [this]() {
            if (editor_ && !editor_->isEditing())
                editor_->setPlayState(EditorPlayState::Edit);
        };

    UIElement& statusEl = ui.get(statusLabelId_);
    statusEl.style.position = PositionMode::Relative;
    statusEl.style.height = Length::px(24.f);
    statusEl.style.flexGrow = 1.f;
}

void ToolbarPanel::update(Editor& editor) {
    if (!ui_ || playButtonId_ == INVALID_UI_ELEMENT)
        return;

    const bool editing = editor.isEditing();

    UIElement& playEl = ui_->get(playButtonId_);
    UIElement& stopEl = ui_->get(stopButtonId_);
    UIElement& statusEl = ui_->get(statusLabelId_);

    playEl.visible = editing;
    stopEl.visible = !editing;

    auto* statusLbl = dynamic_cast<Label*>(statusEl.widget.get());
    if (statusLbl)
        statusLbl->text = editing ? "Edit" : "Play";
}
