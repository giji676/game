#pragma once

#include "engine/asset_manager/ui_element.h"
#include "engine/renderer/ui_renderer.h"

class UI {
public:
    UI() { rootId = createElementInternal(); }

    void init();
    void update() {};

    std::vector<UIRenderCommand> buildRenderList() {
        std::vector<UIRenderCommand> out;
        recurseBuild(rootId, out);
        return out;
    }

    void recurseBuild(UIElementID id, std::vector<UIRenderCommand>& out) {
        UIElement& e = get(id);

        if (e.widget)
            e.widget->buildCommands(e, out);

        for (auto child : e.children)
            recurseBuild(child, out);
    }

    UIElementID label(
        glm::vec2 pos,
        glm::vec2 size,
        glm::vec4 color = {1.f, 1.f, 1.f, 1.f},
        std::string text = "",
        Font* font = nullptr
    );

    UIElementID rect(
        glm::vec2 pos,
        glm::vec2 size,
        glm::vec4 color = {1.f, 1.f, 1.f, 1.f}
    );

    UIElementID button(
        glm::vec2 pos,
        glm::vec2 size,
        glm::vec4 bgColor = {1.f, 1.f, 1.f, 1.f},
        glm::vec4 textColor = {0.f, 0.f, 0.f, 1.f},
        std::string text = "",
        Font* font = nullptr
    );

    UIElementID createElement();

    void reparent(UIElementID childId, UIElementID newParentId);

    UIElement& get(UIElementID id) { return elements[id]; }
    const UIElement& get(UIElementID id) const { return elements[id]; }

    UIElementID getRoot() const { return rootId; }
    UIElement& root() { return elements[rootId]; }
    const UIElement& root() const { return elements[rootId]; }

private:
    std::vector<UIElement> elements;
    UIElementID rootId = 0;

    UIElementID createElementInternal();
    void recurseRender(
        const UIElementID elemId,
        const glm::mat4& parentTransfrom);
    void updateLayout(
        const UIElementID elemId,
        const glm::mat4& parentTransform);
};
