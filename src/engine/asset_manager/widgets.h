#include "engine/asset_manager/font.h"
#include "ui_widget.h"
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
    glm::vec2 padding = {12.f, 6.f};

    std::string text;

    glm::vec4 bgColor = {1,1,1,1};
    glm::vec4 textColor = {0,0,0,1};

    bool hovered = false;
    bool pressed = false;

    std::function<void()> onClick;

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

        out.push_back({
            .type = UICmdType::Text,
            .position = e.transform.position + padding,
            .size = e.transform.size,
            .color = textColor,
            .font = font,
            .text = text
        });
    }
};
