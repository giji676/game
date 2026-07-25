#include "ui.h"
#include "engine/engine.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "asset_manager/widgets.h"
#include "asset_manager/ui_element.h"

void UI::dispatchTextInput(const std::string& text) {
    if (Engine::instance().app.cursorCaptured)
        return;
    if (UIElement* e = getFocusedElement()) {
        if (e->widget) e->widget->onTextInput(text);
    }
}

void UI::dispatchKeyInput(Key key) {
    if (Engine::instance().app.cursorCaptured)
        return;
    if (UIElement* e = getFocusedElement()) {
        if (e->widget) e->widget->onKeyInput(key);
    }
}

void UI::requestFocus(UIElementID id) {
    if (focusedId == id) return;
    if (focusedId != INVALID_UI_ELEMENT) {
        if (UIElement* prev = getFocusedElement())
            if (prev->widget) prev->widget->onFocusLost();
    }
    focusedId = id;
    if (id != INVALID_UI_ELEMENT) {
        get(id).widget->onFocusGained();
        SDL_StartTextInput();
    } else {
        SDL_StopTextInput();
    }
}

UIElement* UI::getFocusedElement() {
    if (focusedId == INVALID_UI_ELEMENT) return nullptr;
    return &get(focusedId);
}

UIElement& UI::get(UIElementID id) {
    assert(id != INVALID_UI_ELEMENT && "UI::get called with INVALID_UI_ELEMENT");
    assert(id < elements.size() && "UI::get called with out-of-range UIElementID");
    return elements[id];
}

const UIElement& UI::get(UIElementID id) const {
    assert(id != INVALID_UI_ELEMENT && "UI::get called with INVALID_UI_ELEMENT");
    assert(id < elements.size() && "UI::get called with out-of-range UIElementID");
    return elements[id];
}

void UI::update() {
    Engine& engine = Engine::instance();
    recurseTick(rootId, engine.app.deltaTime);
    if (!engine.app.cursorCaptured) {
        const glm::vec2 mouse = engine.input.mousePosition();
        recurseUpdateInput(rootId, mouse);
    }
}

void UI::recurseTick(UIElementID id, float dt) {
    UIElement& e = get(id);
    if (!e.visible)
        return;
    if (e.widget) e.widget->tick(e, dt);
    for (UIElementID child : e.children) recurseTick(child, dt);
}

void UI::recurseUpdateInput(UIElementID id, const glm::vec2& mouse) {
    UIElement& e = get(id);
    if (!e.visible)
        return;
    if (e.widget) e.widget->updateInput(e, mouse);
    for (UIElementID child : e.children) recurseUpdateInput(child, mouse);
}

void UI::onUnpause() {
    requestFocus(INVALID_UI_ELEMENT);
    recurseResetInteraction(rootId);
}

void UI::recurseResetInteraction(UIElementID id) {
    UIElement& e = get(id);
    if (e.widget)
        e.widget->resetInteraction();
    for (UIElementID child : e.children)
        recurseResetInteraction(child);
}

UIElementID UI::createElement() {
    UIElementID id = createElementInternal();
    elements[id].setID(id);
    elements[id].parent = rootId;
    elements[rootId].children.push_back(id);
    return id;
}

UIElementID UI::createElementInternal() {
    UIElementID id = static_cast<UIElementID>(elements.size());
    elements.emplace_back();
    return id;
}

UIElementID UI::label(
    glm::vec2 pos,
    glm::vec2 size,
    glm::vec4 color,
    std::string text,
    Font* font)
{
    UIElementID elemId = createElement();
    UIElement& elem = get(elemId);
    elem.transform.position = pos;
    elem.transform.size = size;
    auto& lbl = elem.addWidget<Label>();
    Font* resolvedFont =
        font ? font : &Engine::instance().assets.getFont("InterVariable");
    lbl.font = resolvedFont;
    lbl.color = color;
    lbl.text = std::move(text);
    elem.transform.fontSize = size.y > 0.f ? size.y : resolvedFont->referenceSize;
    return elemId;
}

UIElementID UI::rect(
    glm::vec2 pos,
    glm::vec2 size,
    glm::vec4 color,
    glm::vec4 cornerRadii,
    float borderWidth,
    glm::vec4 borderColor)
{
    UIElementID elemId = createElement();
    UIElement& elem = get(elemId);
    elem.transform.position = pos;
    elem.transform.size = size;
    auto& r = elem.addWidget<Rect>();
    r.color = color;
    r.cornerRadii = cornerRadii;
    r.borderWidth = borderWidth;
    r.borderColor = borderColor;
    return elemId;
}

UIElementID UI::button(
    glm::vec2 pos,
    float fontSize,
    std::string text,
    Font* font)
{
    UIElementID id = createElement();
    UIElement& e = get(id);

    auto& btn = e.addWidget<Button>();

    btn.text = std::move(text);
    btn.font = font
        ? font
        : &Engine::instance().assets.getFont("InterVariable");

    glm::vec2 textSize = btn.font->measure(btn.text, fontSize);

    e.transform.position = pos;
    e.transform.fontSize = fontSize;
    e.transform.size = {
        textSize.x + btn.padding.x * 2.f,
        btn.font->lineHeightAt(fontSize) + btn.padding.y * 2.f
    };
    e.transform.textPosition = btn.font->baselineInRect(
        pos, e.transform.size, btn.padding, fontSize, TextAlignV::Bottom);

    return id;
}
