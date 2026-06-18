#include "engine/asset_manager/font.h"
#include "ui_widget.h"
#include <string>

class Label : public UIWidget {
public:
    Font* font;
    std::string text;
    glm::vec4 color;

    void buildCommands(
        const UIElement& e,
        std::vector<UIRenderCommand>& out) const override
    {
        out.push_back({
            .type = UICmdType::Text,
            .position = e.transform.position,
            .size = {0.f, 48.f},
            .color = color,
            .font = font,
            .text = text
        });
    }
};
