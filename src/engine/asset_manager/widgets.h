#include "engine/asset_manager/font.h"
#include "engine/asset_manager/ui_element.h"
#include "ui_widget.h"
#include "engine/engine.h"
#include "engine/ui/style.h"
#include "engine/utils/geometry.h"
#include <functional>
#include <string>
#include <vector>

namespace {
glm::vec2 textInnerSize(glm::vec2 rectSize, glm::vec2 padding) {
    return {
        std::max(0.f, rectSize.x - padding.x * 2.f),
        std::max(0.f, rectSize.y - padding.y * 2.f),
    };
}

glm::vec2 textClipOrigin(glm::vec2 rectPos, glm::vec2 padding) {
    return {rectPos.x + padding.x, rectPos.y + padding.y};
}

void emitTextCommands(
    const UIElement& e,
    Font* font,
    const std::string& text,
    glm::vec4 color,
    glm::vec2 padding,
    bool centerHorizontally,
    std::vector<UIRenderCommand>& out)
{
    const glm::vec2 inner = textInnerSize(e.transform.size, padding);
    const glm::vec2 clipPos = textClipOrigin(e.transform.position, padding);
    const bool useClip = e.style.textOverflow == TextOverflow::Clip;
    const bool useWrap = e.style.textOverflow == TextOverflow::Wrap && inner.x > 0.f;

    auto pushLine = [&](const std::string& line, glm::vec2 baseline) {
        glm::vec2 textSize = font->measure(line, e.transform.fontSize);
        if (centerHorizontally) {
            baseline.x = e.transform.position.x
                + (e.transform.size.x - textSize.x) * 0.5f;
        }

        UIRenderCommand cmd{
            .type = UICmdType::Text,
            .position = baseline,
            .size = textSize,
            .color = color,
            .font = font,
            .text = line,
        };
        if (useClip || useWrap) {
            cmd.clip = true;
            cmd.clipPos = clipPos;
            cmd.clipSize = inner;
        }
        out.push_back(cmd);
    };

    if (useWrap) {
        const std::vector<std::string> lines =
            font->wrapLines(text, inner.x, e.transform.fontSize);
        const float lineH = font->lineHeightAt(e.transform.fontSize);
        const float totalH = lineH * static_cast<float>(lines.size());
        const float blockBottom = e.transform.position.y + padding.y
            + std::max(0.f, (inner.y - totalH) * 0.5f);
        const float descender = font->descenderAt(e.transform.fontSize);

        for (size_t i = 0; i < lines.size(); ++i) {
            glm::vec2 baseline{
                e.transform.position.x + padding.x,
                blockBottom + static_cast<float>(i) * lineH + descender,
            };
            pushLine(lines[i], baseline);
        }
        return;
    }

    glm::vec2 baseline = font->baselineInRect(
        e.transform.position, e.transform.size, padding, e.transform.fontSize);
    pushLine(text, baseline);
}
} // namespace

class Label : public UIWidget {
public:
    Font* font = nullptr;
    std::string text;
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};

    void buildCommands(
        const UIElement& e,
        std::vector<UIRenderCommand>& out) const override
    {
        emitTextCommands(
            e, font, text, color, {0.f, 0.f}, false, out);
    }

    glm::vec2 measureContent(
            const UIElement& e,
            glm::vec2 available) const override
    {
        if (e.style.textOverflow == TextOverflow::Wrap && available.x > 0.f)
            return font->measureWrapped(text, available.x, e.transform.fontSize);
        return font->measure(text, e.transform.fontSize);
    }
};

class Rect : public UIWidget {
public:
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    glm::vec4 cornerRadii = {0,0,0,0};
    float borderWidth = 0.f;
    glm::vec4 borderColor = {0,0,0,0};

    void buildCommands(
            const UIElement& e,
            std::vector<UIRenderCommand>& out) const override
    {
        out.push_back({
            .type = UICmdType::Rect,
            .position = e.transform.position,
            .size = e.transform.size,
            .color = color,
            .cornerRadii = cornerRadii,
            .borderWidth = borderWidth,
            .borderColor = borderColor,
        });
    }
};

struct ButtonStyle {
    std::optional<glm::vec4> bgColor;
    std::optional<glm::vec4> textColor;
    std::optional<glm::vec4> borderColor;
    std::optional<float>     borderWidth;
    std::optional<float>     scale;
};

struct ResolvedButtonStyle {
    glm::vec4 bgColor;
    glm::vec4 textColor;
    glm::vec4 borderColor;
    float     borderWidth;
    float     scale;
};

enum class ButtonState {
    Normal,
    Hovered,
    Pressed,
    Disabled
};

class Button : public UIWidget {
public:
    ButtonStyle normal;
    ButtonStyle hoveredStyle;
    ButtonStyle pressedStyle;
    ButtonStyle disabledStyle;

    Font* font = nullptr;
    std::string text;
    bool centerText = true;

    glm::vec4 cornerRadii = {0,0,0,0};
    glm::vec2 padding = {10.f, 10.f};

    bool hovered = false;
    bool pressed = false;
    bool disabled = false;

    std::function<void()> onClick;

    ResolvedButtonStyle current;

    void resetInteraction() override {
        hovered = false;
        pressed = false;
    }

    void updateInput(
            const UIElement& e,
            const glm::vec2& pos) override
    {
        if (disabled) {
            hovered = pressed = false;
            return;
        }

        hovered = pointInRect(pos, e.transform.position, e.transform.size);

        Input& input = Engine::instance().input;
        if (hovered && input.pressed(MouseAction::Left))
            pressed = true;
        if (pressed && input.released(MouseAction::Left)) {
            pressed = false;
            if (hovered && onClick)
                onClick();
        }
        if (!input.down(MouseAction::Left))
            pressed = false;
    }

    void tick(const UIElement& e, float dt) override {
        if (Engine::instance().app.cursorCaptured)
            resetInteraction();
        current = resolve(normal, overridesFor(resolveState()));
        (void)e;
        (void)dt;
    }

    void buildCommands(
        const UIElement& e,
        std::vector<UIRenderCommand>& out) const override
    {
        out.push_back({
            .type = UICmdType::Rect,
            .position = e.transform.position,
            .size = e.transform.size,
            .color = current.bgColor,
            .cornerRadii = cornerRadii,
            .borderWidth = current.borderWidth,
            .borderColor = current.borderColor
        });

        emitTextCommands(
            e, font, text, current.textColor, padding, centerText, out);
    }

    glm::vec2 measureContent(
            const UIElement& e,
            glm::vec2 available) const override
    {
        const float innerW = std::max(0.f, available.x - padding.x * 2.f);
        glm::vec2 textSize;
        if (e.style.textOverflow == TextOverflow::Wrap && innerW > 0.f)
            textSize = font->measureWrapped(text, innerW, e.transform.fontSize);
        else
            textSize = font->measure(text, e.transform.fontSize);
        return {
            textSize.x + padding.x * 2.f,
            textSize.y + padding.y * 2.f,
        };
    }

    ButtonState resolveState() const {
        if (disabled) return ButtonState::Disabled;
        if (pressed)  return ButtonState::Pressed;
        if (hovered)  return ButtonState::Hovered;
        return ButtonState::Normal;
    }

    ResolvedButtonStyle resolve(const ButtonStyle& base, const ButtonStyle& override) {
        return ResolvedButtonStyle{
            override.bgColor.value_or(base.bgColor.value_or(glm::vec4{1,1,1,1})),
            override.textColor.value_or(base.textColor.value_or(glm::vec4{0,0,0,1})),
            override.borderColor.value_or(base.borderColor.value_or(glm::vec4{0,0,0,0})),
            override.borderWidth.value_or(base.borderWidth.value_or(0.f)),
            override.scale.value_or(base.scale.value_or(1.f)),
        };
    }

    const ButtonStyle& overridesFor(ButtonState s) const {
        switch (s) {
            case ButtonState::Pressed:  return pressedStyle;
            case ButtonState::Hovered:  return hoveredStyle;
            case ButtonState::Disabled: return disabledStyle;
            default: { static ButtonStyle empty; return empty; }
        }
    }

    const ButtonStyle& styleFor(ButtonState s) const {
        switch (s) {
            case ButtonState::Pressed:  return pressedStyle;
            case ButtonState::Hovered:  return hoveredStyle;
            case ButtonState::Disabled: return disabledStyle;
            default:                    return normal;
        }
    }
};

enum class FieldState {
    Normal,
    Hovered,
    Focused,
    Disabled
};

struct InputFieldStyle {
    std::optional<glm::vec4> bgColor;
    std::optional<glm::vec4> textColor;
    std::optional<glm::vec4> borderColor;
    std::optional<float>     borderWidth;
    std::optional<glm::vec4> caretColor;
};

struct ResolvedInputFieldStyle {
    glm::vec4 bgColor;
    glm::vec4 textColor;
    glm::vec4 borderColor;
    float     borderWidth;
    glm::vec4 caretColor;
};

class InputField : public UIWidget {
public:
    InputFieldStyle normal;
    InputFieldStyle hoveredStyle;
    InputFieldStyle focusedStyle;
    InputFieldStyle disabledStyle;
    ResolvedInputFieldStyle current;

    Font* font = nullptr;
    glm::vec4 cornerRadii = {0,0,0,0};
    glm::vec2 padding = {10.f, 10.f};
    std::string placeholder;
    std::string text;
    size_t caretPos = 0;

    bool hovered = false;
    bool focused = false;
    bool disabled = false;

    float caretBlinkTimer = 0.f;
    bool  caretVisible = true;
    float scrollOffset = 0.f;

    std::function<void(const std::string&)> onChange;
    std::function<void(const std::string&)> onSubmit;

    UIElementID selfId = INVALID_UI_ELEMENT;

    void resetInteraction() override {
        hovered = false;
    }

    void updateInput(const UIElement& e, const glm::vec2& pos) override {
        if (disabled) { hovered = false; return; }

        hovered = pointInRect(pos, e.transform.position, e.transform.size);

        if (hovered && Engine::instance().input.pressed(MouseAction::Left)) {
            if (!focused && e.owner)
                e.owner->requestFocus(selfId);
            setCaretFromClick(e, pos);
        }
    }

    void setCaretFromClick(const UIElement& e, glm::vec2 clickPos) {
        glm::vec2 textPos = font->baselineInRect(
            e.transform.position, e.transform.size, padding, e.transform.fontSize);
        textPos.x -= scrollOffset;

        float textX = clickPos.x - textPos.x;
        caretPos = font->caretIndexAt(text, textX, e.transform.fontSize);
        caretVisible = true;
        caretBlinkTimer = 0.f;
    }

    void tick(const UIElement& e, float dt) override {
        current = resolve(normal, overridesFor(resolveState()));

        float innerWidth = e.transform.size.x - padding.x * 2.f;
        if (focused) {
            scrollOffset = font->horizontalScrollOffset(
                text, caretPos, innerWidth, e.transform.fontSize, scrollOffset);
        } else {
            scrollOffset = 0.f;
        }

        if (focused) {
            caretBlinkTimer += dt;
            if (caretBlinkTimer >= 0.5f) {
                caretBlinkTimer = 0.f;
                caretVisible = !caretVisible;
            }
        } else {
            caretVisible = false;
            caretBlinkTimer = 0.f;
        }
    }

    void onFocusGained() override {
        focused = true;
        caretVisible = true;
        caretBlinkTimer = 0.f;
        caretPos = text.size();
    }

    void onFocusLost() override {
        focused = false;
    }

    void onTextInput(const std::string& input) override {
        if (!focused) return;
        text.insert(caretPos, input);
        caretPos += input.size();
        if (onChange) onChange(text);
    }

    void onKeyInput(Key key) override {
        if (!focused) return;
        switch (key) {
            case Key::SDL_SCANCODE_BACKSPACE:
                if (caretPos > 0) {
                    text.erase(caretPos - 1, 1);
                    caretPos--;
                    if (onChange) onChange(text);
                }
                break;
            case Key::SDL_SCANCODE_DELETE:
                if (caretPos < text.size()) {
                    text.erase(caretPos, 1);
                    if (onChange) onChange(text);
                }
                break;
            case Key::SDL_SCANCODE_LEFT:
                if (caretPos > 0) caretPos--;
                break;
            case Key::SDL_SCANCODE_RIGHT:
                if (caretPos < text.size()) caretPos++;
                break;
            case Key::SDL_SCANCODE_RETURN:
                if (onSubmit) onSubmit(text);
                break;
            default: break;
        }
        caretVisible = true;
        caretBlinkTimer = 0.f; // reset blink so caret is visible right after typing
    }

    void buildCommands(
            const UIElement& e,
            std::vector<UIRenderCommand>& out) const override
    {
        out.push_back({
            .type = UICmdType::Rect,
            .position = e.transform.position,
            .size = e.transform.size,
            .color = current.bgColor,
            .cornerRadii = cornerRadii,
            .borderWidth = current.borderWidth,
            .borderColor = current.borderColor,
        });

        bool showPlaceholder = text.empty() && !focused;
        const std::string& displayText = showPlaceholder ? placeholder : text;
        glm::vec2 clipPos = {
            e.transform.position.x + padding.x,
            e.transform.position.y + padding.y,
        };
        glm::vec2 clipSize = {
            e.transform.size.x - padding.x * 2.f,
            e.transform.size.y - padding.y * 2.f,
        };

        glm::vec2 textPos = font->baselineInRect(
            e.transform.position, e.transform.size, padding, e.transform.fontSize);
        if (!showPlaceholder)
            textPos.x -= scrollOffset;
        glm::vec2 textSize = font->measure(displayText, e.transform.fontSize);

        out.push_back({
            .type = UICmdType::Text,
            .position = textPos,
            .size = textSize,
            .color = current.textColor,
            .font = font,
            .text = displayText,
            .clip = true,
            .clipPos = clipPos,
            .clipSize = clipSize,
        });

        if (focused && caretVisible) {
            float caretX = font->measure(
                text.substr(0, caretPos),
                e.transform.fontSize).x;
            Font::CaretRect caret = font->caretAt(textPos, caretX, e.transform.fontSize);

            out.push_back({
                .type = UICmdType::Rect,
                .position = caret.position,
                .size = caret.size,
                .color = current.caretColor,
                .clip = true,
                .clipPos = clipPos,
                .clipSize = clipSize,
            });
        }
    }

    FieldState resolveState() const {
        if (disabled) return FieldState::Disabled;
        if (focused)  return FieldState::Focused;
        if (hovered)  return FieldState::Hovered;
        return FieldState::Normal;
    }

    const InputFieldStyle& overridesFor(FieldState s) const {
        switch (s) {
            case FieldState::Focused:  return focusedStyle;
            case FieldState::Hovered:  return hoveredStyle;
            case FieldState::Disabled: return disabledStyle;
            default: { static InputFieldStyle empty; return empty; }
        }
    }

    ResolvedInputFieldStyle resolve(
            const InputFieldStyle& base,
            const InputFieldStyle& o) const
    {
        return {
            o.bgColor.value_or(base.bgColor.value_or(glm::vec4{1,1,1,1})),
            o.textColor.value_or(base.textColor.value_or(glm::vec4{0,0,0,1})),
            o.borderColor.value_or(base.borderColor.value_or(glm::vec4{0,0,0,0})),
            o.borderWidth.value_or(base.borderWidth.value_or(0.f)),
            o.caretColor.value_or(base.caretColor.value_or(glm::vec4{0,0,0,1})),
        };
    }
};

struct ToolbarItem {
    std::string label;
    std::function<void()> onClick;
    bool toggleable = false;
    bool toggled = false;
};

class Toolbar : public UIWidget {
public:
    Font* font = nullptr;
    std::vector<ToolbarItem> items;

    glm::vec4 bgColor = {0.18f, 0.18f, 0.18f, 1.f};
    glm::vec4 borderColor = {0.08f, 0.08f, 0.08f, 1.f};
    float borderWidth = 1.f;

    glm::vec4 itemBgColor = {0.28f, 0.28f, 0.28f, 1.f};
    glm::vec4 itemHoverBgColor = {0.36f, 0.36f, 0.36f, 1.f};
    glm::vec4 itemPressedBgColor = {0.22f, 0.22f, 0.22f, 1.f};
    glm::vec4 itemToggledBgColor = {0.2f, 0.45f, 0.82f, 1.f};
    glm::vec4 itemTextColor = {0.95f, 0.95f, 0.95f, 1.f};

    glm::vec2 padding = {8.f, 4.f};
    glm::vec2 itemPadding = {12.f, 4.f};
    float itemSpacing = 4.f;

    void addItem(std::string label, std::function<void()> onClick) {
        items.push_back({
            .label = std::move(label),
            .onClick = std::move(onClick),
            .toggleable = false,
            .toggled = false,
        });
        ensureStateSize();
    }

    void addToggle(
        std::string label,
        bool initialToggled,
        std::function<void(bool toggled)> onToggle = nullptr)
    {
        size_t index = items.size();
        items.push_back({
            .label = std::move(label),
            .onClick = nullptr,
            .toggleable = true,
            .toggled = initialToggled,
        });

        items[index].onClick = [this, index, onToggle = std::move(onToggle)]() {
            items[index].toggled = !items[index].toggled;
            if (onToggle)
                onToggle(items[index].toggled);
        };
        ensureStateSize();
    }

    void resetInteraction() override {
        hovered.assign(items.size(), false);
        pressed.assign(items.size(), false);
    }

    void updateInput(const UIElement& e, const glm::vec2& pos) override {
        Input& input = Engine::instance().input;
        ensureStateSize();

        for (size_t i = 0; i < items.size(); ++i) {
            glm::vec2 itemPos = itemPosition(e, static_cast<int>(i));
            glm::vec2 itemSize = measureItem(static_cast<int>(i), e.transform.fontSize);

            hovered[i] = pointInRect(pos, itemPos, itemSize);
            if (!hovered[i]) {
                if (!input.down(MouseAction::Left))
                    pressed[i] = false;
                continue;
            }

            if (input.pressed(MouseAction::Left))
                pressed[i] = true;

            if (pressed[i] && input.released(MouseAction::Left)) {
                pressed[i] = false;
                if (items[i].onClick)
                    items[i].onClick();
            }

            if (!input.down(MouseAction::Left))
                pressed[i] = false;
        }
    }

    void buildCommands(
        const UIElement& e,
        std::vector<UIRenderCommand>& out) const override
    {
        out.push_back({
            .type = UICmdType::Rect,
            .position = e.transform.position,
            .size = e.transform.size,
            .color = bgColor,
            .borderWidth = borderWidth,
            .borderColor = borderColor,
        });

        float fontSize = e.transform.fontSize;
        for (size_t i = 0; i < items.size(); ++i) {
            const ToolbarItem& item = items[i];
            glm::vec2 itemPos = itemPosition(e, static_cast<int>(i));
            glm::vec2 itemSize = measureItem(static_cast<int>(i), fontSize);

            glm::vec4 bg = itemBgColor;
            if (item.toggleable && item.toggled)
                bg = itemToggledBgColor;
            else if (pressed.size() > i && pressed[i])
                bg = itemPressedBgColor;
            else if (hovered.size() > i && hovered[i])
                bg = itemHoverBgColor;

            out.push_back({
                .type = UICmdType::Rect,
                .position = itemPos,
                .size = itemSize,
                .color = bg,
            });

            UIElement itemElem;
            itemElem.transform.position = itemPos;
            itemElem.transform.size = itemSize;
            itemElem.transform.fontSize = fontSize;
            itemElem.style.textOverflow = e.style.textOverflow;
            emitTextCommands(
                itemElem, font, item.label, itemTextColor, itemPadding, false, out);
        }
    }

    glm::vec2 measureContent(
            const UIElement& e,
            glm::vec2 available) const override
    {
        float fontSize = e.transform.fontSize;
        float width = padding.x * 2.f;
        float itemHeight = font->lineHeightAt(fontSize) + itemPadding.y * 2.f;

        for (size_t i = 0; i < items.size(); ++i) {
            if (i > 0)
                width += itemSpacing;
            width += measureItem(static_cast<int>(i), fontSize).x;
        }

        return {width, itemHeight + padding.y * 2.f};
    }

private:
    mutable std::vector<bool> hovered;
    mutable std::vector<bool> pressed;

    void ensureStateSize() {
        hovered.resize(items.size(), false);
        pressed.resize(items.size(), false);
    }

    glm::vec2 measureItem(int index, float fontSize) const {
        const ToolbarItem& item = items[index];
        glm::vec2 textSize = font->measure(item.label, fontSize);
        return {
            textSize.x + itemPadding.x * 2.f,
            font->lineHeightAt(fontSize) + itemPadding.y * 2.f,
        };
    }

    glm::vec2 itemPosition(const UIElement& e, int index) const {
        float x = e.transform.position.x + padding.x;
        float y = e.transform.position.y + padding.y;
        float fontSize = e.transform.fontSize;

        for (int i = 0; i < index; ++i) {
            x += measureItem(i, fontSize).x + itemSpacing;
        }

        float itemH = measureItem(index, fontSize).y;
        float innerH = e.transform.size.y - padding.y * 2.f;
        y += (innerH - itemH) * 0.5f;

        return {x, y};
    }
};
