#pragma once

#include "engine/asset_manager/ui_element.h"
#include "engine/defines.h"
#include "engine/renderer/ui_renderer.h"

// Region of the window this tree occupies.
// `size` is the layout coordinate space. When `presentSize` matches `size`,
// px-based elements keep their pixel dimensions and %-based elements scale
// with the surface. When they differ, the render pass scales uniformly.
struct UISurface {
    glm::vec2 origin = {0.f, 0.f};
    glm::vec2 size = {0.f, 0.f};
    glm::vec2 presentSize = {0.f, 0.f};

    glm::vec2 presentScale() const {
        if (size.x <= 0.f || size.y <= 0.f)
            return {1.f, 1.f};
        const glm::vec2 present = (presentSize.x > 0.f && presentSize.y > 0.f)
            ? presentSize
            : size;
        return {present.x / size.x, present.y / size.y};
    }
};

class UI {
public:
    UI() { rootId = createElementInternal(); }

    void init();
    void update();

    void setSurface(glm::vec2 origin, glm::vec2 logicalSize, glm::vec2 presentSize) {
        surface = {origin, logicalSize, presentSize};
    }
    const UISurface& getSurface() const { return surface; }

    bool hasFocus() const { return focusedId != INVALID_UI_ELEMENT; }

    std::vector<UIRenderCommand> buildRenderList();

    void recurseTick(UIElementID id, float dt);
    void recurseUpdateInput(UIElementID id, const glm::vec2& mouse);

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
            glm::vec4 color,
            glm::vec4 cornerRadii = {0,0,0,0},
            float borderWidth = 0.f,
            glm::vec4 borderColor = {0,0,0,0}
    );

    UIElementID button(
            glm::vec2 pos,
            float fontSize,
            std::string text,
            Font* font
    );

    UIElementID toolbar(
            glm::vec2 pos,
            glm::vec2 size,
            float fontSize,
            Font* font = nullptr
    );

    UIElementID createElement();

    void reparent(UIElementID childId, UIElementID newParentId);
    void requestFocus(UIElementID id);

    void dispatchTextInput(const std::string& text);
    void dispatchKeyInput(Key key);

    UIElement& get(UIElementID id);
    const UIElement& get(UIElementID id) const;

    const Style& resolvedStyle(UIElementID id) const { return get(id).style; }

    UIElement* getFocusedElement();

    UIElementID getRoot() const { return rootId; }
    UIElement& root() { return elements[rootId]; }
    const UIElement& root() const { return elements[rootId]; }

    void onUnpause();
    void recurseResetInteraction(UIElementID id);

private:
    std::vector<UIElement> elements;
    UIElementID rootId = 0;
    UIElementID focusedId = INVALID_UI_ELEMENT;
    UISurface surface;

    UIElementID createElementInternal();
    void recurseBuild(UIElementID id, std::vector<UIRenderCommand>& out, bool clipActive,
                      glm::vec2 clipPos, glm::vec2 clipSize);
    bool recurseScrollInput(UIElementID id, const glm::vec2& mouse, float wheelY);
    void recurseRender(
        const UIElementID elemId,
        const glm::mat4& parentTransfrom);
    void updateLayout(
        const UIElementID elemId,
        const glm::mat4& parentTransform);
};
