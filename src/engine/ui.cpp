#include "ui.h"
#include "engine/engine.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "asset_manager/widgets.h"
#include "asset_manager/ui_element.h"

void UI::update() {
    const glm::vec2 mouse = Engine::instance().input.mousePosition();
    recurseUpdate(rootId, mouse);
}

void UI::recurseUpdate(UIElementID id, const glm::vec2& mouse) {
    UIElement& e = get(id);
    if (e.widget)
        e.widget->update(e, mouse);

    for (UIElementID child : e.children)
        recurseUpdate(child, mouse);
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
    return elemId;
}

UIElementID UI::rect(
    glm::vec2 pos,
    glm::vec2 size,
    glm::vec4 color)
{
    UIElementID elemId = createElement();
    UIElement& elem = get(elemId);
    elem.transform.position = pos;
    elem.transform.size = size;
    auto& r = elem.addWidget<Rect>();
    r.color = color;
    return elemId;
}

UIElementID UI::button(
    glm::vec2 pos,
    glm::vec2 size,
    float fontSize,
    glm::vec4 bgColor,
    glm::vec4 textColor,
    std::string text,
    Font* font)
{
    UIElementID id = createElement();
    UIElement& elem = get(id);

    auto& btn = elem.addWidget<Button>();

    btn.bgColor = bgColor;
    btn.textColor = textColor;
    btn.text = std::move(text);
    btn.font = font
        ? font
        : &Engine::instance().assets.getFont("InterVariable");

    glm::vec2 textSize = btn.font->measure(btn.text, fontSize);

    elem.transform.position = pos;
    elem.transform.size =
        {
            textSize.x + btn.padding.x * 2.0f,
            textSize.y + btn.padding.y * 2.0f
        };

    return id;
}
