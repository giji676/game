#include "engine/asset_manager/font.h"
#include "ui_widget.h"
#include "engine/engine.h"
#include <functional>
#include <string>

class Label : public UIWidget {
public:
    Font* font = nullptr;
    std::string text;
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};

    void buildCommands(
        const UIElement& e,
        std::vector<UIRenderCommand>& out) const override
    {
        out.push_back({
            .type = UICmdType::Text,
            .position = e.transform.position,
            .size = e.transform.size,
            .color = color,
            .font = font,
            .text = text
        });
    }
};

class Rect : public UIWidget {
public:
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    glm::vec4 cornerRadii = {0,0,0,0};
    float borderWidth = 0.f;
    glm::vec4 borderColor = {0,0,0,0};

    void buildCommands(const UIElement& e, std::vector<UIRenderCommand>& out) const override {
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

    glm::vec4 cornerRadii = {0,0,0,0};
    glm::vec2 padding = {10.f, 10.f};

    bool hovered = false;
    bool pressed = false;
    bool disabled = false;

    std::function<void()> onClick;

    ResolvedButtonStyle current;

    void update(
            const UIElement& e,
            const glm::vec2& pos) override
    {
        if (!disabled) {
            hovered =
                pos.x >= e.transform.position.x &&
                pos.x <= e.transform.position.x + e.transform.size.x &&
                pos.y >= e.transform.position.y &&
                pos.y <= e.transform.position.y + e.transform.size.y;

            if (hovered && Engine::instance().input.pressed(MouseAction::Left))
                pressed = true;
            if (pressed && Engine::instance().input.released(MouseAction::Left)) {
                pressed = false;
                if (hovered && onClick) onClick();
            }
            if (!Engine::instance().input.down(MouseAction::Left))
                pressed = false;
        } else {
            hovered = pressed = false;
        }

        current = resolve(normal, overridesFor(resolveState()));
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

        glm::vec2 textSize = font->measure(text, e.transform.fontSize);

        out.push_back({
            .type = UICmdType::Text,
            .position = e.transform.textPosition,
            .size = textSize,
            .color = current.textColor,
            .font = font,
            .text = text
        });
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
};
