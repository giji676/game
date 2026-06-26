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
    bool rounded = false;
    float radius = 0.f;

    void buildCommands(
        const UIElement& e,
        std::vector<UIRenderCommand>& out) const override
    {
        out.push_back({
            .type = UICmdType::Rect,
            .position = e.transform.position,
            .size = e.transform.size,
            .color = color,
            // .radius = radius
        });
    }
};

class Button : public UIWidget {
public:
    Font* font = nullptr;
    glm::vec2 padding = {0.f, 0.f};

    std::string text;

    glm::vec4 bgColor = {1,1,1,1};
    glm::vec4 textColor = {0,0,0,1};

    bool hovered = false;
    bool pressed = false;

    std::function<void()> onClick;

    void update(
            const UIElement& e,
            const glm::vec2& pos) override
    {
        if (auto* btn = dynamic_cast<Button*>(e.widget.get())) {
            btn->hovered =
                pos.x >= e.transform.position.x &&
                pos.x <= e.transform.position.x + e.transform.size.x &&
                pos.y >= e.transform.position.y &&
                pos.y <= e.transform.position.y + e.transform.size.y;

            if (btn->hovered &&
                    Engine::instance().input.pressed(MouseAction::Left))
            {
                btn->pressed = true;
            }

            if (btn->pressed &&
                    Engine::instance().input.released(MouseAction::Left))
            {
                btn->pressed = false;

                if (btn->hovered && btn->onClick)
                    btn->onClick();
            }

            if (!Engine::instance().input.down(MouseAction::Left))
                btn->pressed = false;
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
            .color = bgColor
        });

        glm::vec2 textSize = font->measure(text, e.transform.size.y);

        glm::vec2 textPos =
            e.transform.position +
            glm::vec2(
                    (e.transform.size.x - textSize.x) * 0.5f,
                    (e.transform.size.y - textSize.y) * 0.5f
                    );

        out.push_back({
                .type = UICmdType::Text,
                .position = textPos,
                .size = e.transform.size,
                .color = textColor,
                .font = font,
                .text = text
                });
    }
};
