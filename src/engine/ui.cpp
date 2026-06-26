#include "ui.h"
#include "engine/engine.h"
#include "engine/ui.h"
#include "asset_manager/widgets.h"
#include "asset_manager/ui_element.h"

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

    glm::vec2 textSize = btn.font->measure(btn.text, elem.transform.size.y);
    std::cout << "textSize: {" << textSize.x << ", " << textSize.y << "}\n";

    elem.transform.position = pos;
    elem.transform.size =
        {
            std::max(size.x, textSize.x + btn.padding.x * 2.0f),
            std::max(size.y, textSize.y + btn.padding.y * 2.0f)
        };

    return id;
}
