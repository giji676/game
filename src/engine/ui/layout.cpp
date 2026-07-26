#include "engine/ui/layout.h"

#include <algorithm>

#include "engine/asset_manager/ui_element.h"
#include "engine/ui.h"

namespace {

struct Extent {
    float position;
    float size;
};

// Resolves one axis in the renderer's coordinate space, so `startInset` is the
// edge the axis grows away from (left for x, bottom for y) and `endInset` is
// the opposite edge (right for x, top for y).
Extent resolveAxis(
    const Length& size,
    const Length& startInset,
    const Length& endInset,
    float containerStart,
    float containerSize,
    float intrinsicSize,
    const Extent& manual)
{
    const bool hasStart = !startInset.isAuto();
    const bool hasEnd = !endInset.isAuto();

    float resolvedSize;
    if (!size.isAuto())
        resolvedSize = size.resolve(containerSize);
    else if (hasStart && hasEnd)
        resolvedSize = containerSize
            - startInset.resolve(containerSize)
            - endInset.resolve(containerSize);
    else if (intrinsicSize > 0.f)
        resolvedSize = intrinsicSize;
    else
        resolvedSize = manual.size;

    resolvedSize = std::max(resolvedSize, 0.f);

    float position = manual.position;
    if (hasStart)
        position = containerStart + startInset.resolve(containerSize);
    else if (hasEnd)
        position = containerStart + containerSize
            - endInset.resolve(containerSize)
            - resolvedSize;

    return {position, resolvedSize};
}

void layoutElement(
    UI& ui,
    UIElementID id,
    glm::vec2 containerPosition,
    glm::vec2 containerSize)
{
    UIElement& e = ui.get(id);
    const Style& style = ui.resolvedStyle(id);

    if (style.specifiesLayout()) {
        glm::vec2 intrinsic = {0.f, 0.f};
        if (e.widget)
            intrinsic = e.widget->measureContent(e, containerSize);

        const Extent x = resolveAxis(
            style.width, style.inset.left, style.inset.right,
            containerPosition.x, containerSize.x, intrinsic.x,
            {e.transform.position.x, e.transform.size.x});

        const Extent y = resolveAxis(
            style.height, style.inset.bottom, style.inset.top,
            containerPosition.y, containerSize.y, intrinsic.y,
            {e.transform.position.y, e.transform.size.y});

        e.transform.position = {x.position, y.position};
        e.transform.size = {x.size, y.size};
    }

    for (UIElementID child : e.children)
        layoutElement(ui, child, e.transform.position, e.transform.size);
}

} // namespace

void layoutTree(UI& ui, UIElementID rootId, glm::vec2 viewportSize) {
    UIElement& root = ui.get(rootId);
    root.transform.position = {0.f, 0.f};
    root.transform.size = viewportSize;

    for (UIElementID child : root.children)
        layoutElement(ui, child, root.transform.position, root.transform.size);
}
