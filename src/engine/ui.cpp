#include "ui.h"
#include "engine/engine.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "asset_manager/widgets.h"
#include "asset_manager/ui_element.h"
#include "engine/ui/layout.h"

#include <algorithm>

namespace {

struct UIClipRect {
    bool active = false;
    glm::vec2 pos = {0.f, 0.f};
    glm::vec2 size = {0.f, 0.f};
};

UIClipRect elementContentClip(const UIElement& e) {
    const Style& s = e.style;
    const float pl = s.padding.left.resolve(e.transform.size.x);
    const float pr = s.padding.right.resolve(e.transform.size.x);
    const float pb = s.padding.bottom.resolve(e.transform.size.y);
    const float pt = s.padding.top.resolve(e.transform.size.y);
    return {
        true,
        {e.transform.position.x + pl, e.transform.position.y + pb},
        {std::max(0.f, e.transform.size.x - pl - pr),
         std::max(0.f, e.transform.size.y - pb - pt)},
    };
}

UIClipRect intersectClip(const UIClipRect& a, const UIClipRect& b) {
    if (!a.active)
        return b;
    if (!b.active)
        return a;

    const float x0 = std::max(a.pos.x, b.pos.x);
    const float y0 = std::max(a.pos.y, b.pos.y);
    const float x1 = std::min(a.pos.x + a.size.x, b.pos.x + b.size.x);
    const float y1 = std::min(a.pos.y + a.size.y, b.pos.y + b.size.y);
    return {true, {x0, y0}, {std::max(0.f, x1 - x0), std::max(0.f, y1 - y0)}};
}

void applyClip(UIRenderCommand& cmd, const UIClipRect& clip) {
    if (!clip.active)
        return;

    if (!cmd.clip) {
        cmd.clip = true;
        cmd.clipPos = clip.pos;
        cmd.clipSize = clip.size;
        return;
    }

    const UIClipRect merged = intersectClip(
        {true, cmd.clipPos, cmd.clipSize}, clip);
    cmd.clipPos = merged.pos;
    cmd.clipSize = merged.size;
}

} // namespace

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

    // Window space -> logical space. When the surface is presented smaller than
    // its layout size, divide out the scale so hit tests land on the right
    // elements.
    const glm::vec2 scale = surface.presentScale();
    const glm::vec2 mouse =
        (engine.input.mousePosition() - surface.origin) / scale;

    if (!engine.app.cursorCaptured) {
        const float wheelY = engine.input.mouseWheelY();
        if (wheelY != 0.f)
            recurseScrollInput(rootId, mouse, wheelY);
    }

    layoutTree(*this, rootId, surface.size);

    recurseTick(rootId, engine.app.deltaTime);
    if (!engine.app.cursorCaptured) {
        recurseUpdateInput(rootId, mouse);

        if (engine.input.pressed(MouseAction::Left) && focusedId != INVALID_UI_ELEMENT) {
            const UIElement& focused = get(focusedId);
            const glm::vec2& pos = focused.transform.position;
            const glm::vec2& size = focused.transform.size;
            bool overFocused =
                mouse.x >= pos.x && mouse.x <= pos.x + size.x &&
                mouse.y >= pos.y && mouse.y <= pos.y + size.y;
            if (!overFocused)
                requestFocus(INVALID_UI_ELEMENT);
        }
    }
}

std::vector<UIRenderCommand> UI::buildRenderList() {
    std::vector<UIRenderCommand> out;
    recurseBuild(rootId, out, false, {0.f, 0.f}, {0.f, 0.f});
    return out;
}

void UI::recurseBuild(
    UIElementID id,
    std::vector<UIRenderCommand>& out,
    bool clipActive,
    glm::vec2 clipPos,
    glm::vec2 clipSize)
{
    UIElement& e = get(id);
    if (!e.visible)
        return;

    UIClipRect clip{clipActive, clipPos, clipSize};
    if (e.style.overflow == Overflow::Hidden ||
        e.style.overflow == Overflow::Scroll) {
        clip = intersectClip(clip, elementContentClip(e));
    }

    if (e.widget) {
        std::vector<UIRenderCommand> local;
        e.widget->buildCommands(e, local);
        for (UIRenderCommand& cmd : local)
            applyClip(cmd, clip);
        out.insert(out.end(), local.begin(), local.end());
    }

    for (UIElementID child : e.children) {
        recurseBuild(child, out, clip.active, clip.pos, clip.size);
    }
}

bool UI::recurseScrollInput(UIElementID id, const glm::vec2& mouse, float wheelY) {
    UIElement& e = get(id);
    if (!e.visible)
        return false;

    for (auto it = e.children.rbegin(); it != e.children.rend(); ++it) {
        if (recurseScrollInput(*it, mouse, wheelY))
            return true;
    }

    if (e.style.overflow != Overflow::Scroll)
        return false;

    const glm::vec2& pos = e.transform.position;
    const glm::vec2& size = e.transform.size;
    if (mouse.x < pos.x || mouse.x > pos.x + size.x ||
        mouse.y < pos.y || mouse.y > pos.y + size.y)
        return false;

    constexpr float scrollStep = 24.f;
    e.scrollOffset.y -= wheelY * scrollStep;
    return true;
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

void UI::reparent(UIElementID childId, UIElementID newParentId) {
    UIElement& child = get(childId);
    UIElement& newParent = get(newParentId);

    if (child.parent != INVALID_UI_ELEMENT && child.parent != newParentId) {
        UIElement& oldParent = get(child.parent);
        auto& siblings = oldParent.children;
        siblings.erase(
            std::remove(siblings.begin(), siblings.end(), childId),
            siblings.end());
    }

    child.parent = newParentId;
    if (std::find(newParent.children.begin(), newParent.children.end(), childId)
            == newParent.children.end()) {
        newParent.children.push_back(childId);
    }
}

UIElementID UI::createElementInternal() {
    UIElementID id = static_cast<UIElementID>(elements.size());
    elements.emplace_back();
    elements[id].owner = this;
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

    return id;
}

UIElementID UI::inputField(
    glm::vec2 pos,
    glm::vec2 size,
    float fontSize,
    std::string placeholder,
    Font* font)
{
    UIElementID id = createElement();
    UIElement& e = get(id);
    auto& field = e.addWidget<InputField>();

    field.selfId = id;
    field.font = font
        ? font
        : &Engine::instance().assets.getFont("InterVariable");
    field.placeholder = std::move(placeholder);

    e.transform.position = pos;
    e.transform.size = size;
    e.transform.fontSize = fontSize;
    return id;
}

UIElementID UI::toolbar(
    glm::vec2 pos,
    glm::vec2 size,
    float fontSize,
    Font* font)
{
    UIElementID id = createElement();
    UIElement& e = get(id);

    auto& tb = e.addWidget<Toolbar>();
    tb.font = font
        ? font
        : &Engine::instance().assets.getFont("InterVariable");

    e.transform.position = pos;
    e.transform.size = size;
    e.transform.fontSize = fontSize;

    return id;
}
