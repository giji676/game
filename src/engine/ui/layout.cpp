#include "engine/ui/layout.h"

#include <algorithm>

#include "engine/asset_manager/ui_element.h"
#include "engine/ui.h"

namespace {

struct Extent {
    float position;
    float size;
};

struct LayoutRect {
    glm::vec2 pos;
    glm::vec2 size;
};

float clampMinMax(
    float size,
    const Length& minLen,
    const Length& maxLen,
    float basis)
{
    if (!minLen.isAuto())
        size = std::max(size, minLen.resolve(basis));
    if (!maxLen.isAuto())
        size = std::min(size, maxLen.resolve(basis));
    return std::max(size, 0.f);
}

LayoutRect contentBox(const LayoutRect& border, const Style& style) {
    const float pl = style.padding.left.resolve(border.size.x);
    const float pr = style.padding.right.resolve(border.size.x);
    const float pb = style.padding.bottom.resolve(border.size.y);
    const float pt = style.padding.top.resolve(border.size.y);

    return {
        {border.pos.x + pl, border.pos.y + pb},
        {std::max(0.f, border.size.x - pl - pr),
         std::max(0.f, border.size.y - pb - pt)},
    };
}

// Resolves one axis in the renderer's coordinate space, so `startInset` is the
// edge the axis grows away from (left for x, bottom for y) and `endInset` is
// the opposite edge (right for x, top for y).
// When a single inset pins the element on this axis, `anchor` (0–1) chooses
// which point on the element aligns to that inset — e.g. 0.5 centers it.
Extent resolveAxis(
    const Length& size,
    const Length& minSize,
    const Length& maxSize,
    const Length& startInset,
    const Length& endInset,
    const Length& marginStart,
    const Length& marginEnd,
    float anchor,
    float containerStart,
    float containerSize,
    float intrinsicSize,
    const Extent& manual)
{
    const bool hasStart = !startInset.isAuto();
    const bool hasEnd = !endInset.isAuto();
    const float marginBefore = marginStart.resolve(containerSize);
    const float marginAfter = marginEnd.resolve(containerSize);

    float resolvedSize;
    if (!size.isAuto())
        resolvedSize = size.resolve(containerSize);
    else if (hasStart && hasEnd)
        resolvedSize = containerSize
            - startInset.resolve(containerSize)
            - endInset.resolve(containerSize)
            - marginBefore
            - marginAfter;
    else if (intrinsicSize > 0.f)
        resolvedSize = intrinsicSize;
    else
        resolvedSize = manual.size;

    resolvedSize = clampMinMax(resolvedSize, minSize, maxSize, containerSize);

    float position = manual.position;
    if (hasStart && !hasEnd) {
        // Start inset (left/bottom): anchor 0 = start edge at target.
        const float target = containerStart + startInset.resolve(containerSize);
        position = target - anchor * resolvedSize + marginBefore;
    } else if (hasEnd && !hasStart) {
        // End inset (right/top): anchor 0 = far edge at target (CSS default).
        const float target = containerStart + containerSize
            - endInset.resolve(containerSize);
        position = target - (1.f - anchor) * resolvedSize - marginAfter;
    } else if (hasStart && hasEnd)
        position = containerStart + startInset.resolve(containerSize) + marginBefore;

    return {position, resolvedSize};
}

void layoutAbsolute(
    UIElement& e,
    const Style& style,
    glm::vec2 containerPosition,
    glm::vec2 containerSize)
{
    glm::vec2 intrinsic = {0.f, 0.f};
    if (e.widget)
        intrinsic = e.widget->measureContent(e, containerSize);

    const Extent x = resolveAxis(
        style.width, style.minWidth, style.maxWidth,
        style.inset.left, style.inset.right,
        style.margin.left, style.margin.right,
        e.transform.anchor.x,
        containerPosition.x, containerSize.x, intrinsic.x,
        {e.transform.position.x, e.transform.size.x});

    const Extent y = resolveAxis(
        style.height, style.minHeight, style.maxHeight,
        style.inset.bottom, style.inset.top,
        style.margin.bottom, style.margin.top,
        e.transform.anchor.y,
        containerPosition.y, containerSize.y, intrinsic.y,
        {e.transform.position.y, e.transform.size.y});

    e.transform.position = {x.position, y.position};
    e.transform.size = {x.size, y.size};
}

float resolveBlockHeight(
    UI& ui,
    UIElementID id,
    const Style& style,
    glm::vec2 contentSize,
    glm::vec2 intrinsic)
{
    float height;
    if (!style.height.isAuto())
        height = style.height.resolve(contentSize.y);
    else if (intrinsic.y > 0.f)
        height = intrinsic.y;
    else
        height = ui.get(id).transform.size.y;

    return clampMinMax(height, style.minHeight, style.maxHeight, contentSize.y);
}

float resolveBlockWidth(
    const Style& style,
    float availableWidth,
    float basisX,
    glm::vec2 intrinsic)
{
    float width;
    if (!style.width.isAuto())
        width = style.width.resolve(basisX);
    else if (intrinsic.x > 0.f)
        width = intrinsic.x;
    else
        width = availableWidth;

    width = clampMinMax(width, style.minWidth, style.maxWidth, basisX);
    return std::min(width, availableWidth);
}

void layoutChildren(
    UI& ui,
    UIElementID parentId,
    LayoutRect content,
    const Style& parentStyle);

void layoutElement(
    UI& ui,
    UIElementID id,
    glm::vec2 containerPosition,
    glm::vec2 containerSize,
    Display parentDisplay)
{
    UIElement& e = ui.get(id);
    const Style& style = ui.resolvedStyle(id);

    const bool inFlow =
        (parentDisplay == Display::Block || parentDisplay == Display::Flex) &&
        style.position == PositionMode::Relative;

    if (!inFlow &&
        style.position == PositionMode::Absolute &&
        style.hasAbsolutePlacement()) {
        layoutAbsolute(e, style, containerPosition, containerSize);
    }

    const LayoutRect border = {e.transform.position, e.transform.size};
    const LayoutRect content = contentBox(border, style);
    layoutChildren(ui, id, content, style);
}

void layoutFlowAbsoluteChildren(
    UI& ui,
    UIElementID parentId,
    LayoutRect content)
{
    for (UIElementID childId : ui.get(parentId).children) {
        const Style& childStyle = ui.resolvedStyle(childId);
        if (childStyle.position != PositionMode::Absolute)
            continue;

        layoutElement(ui, childId, content.pos, content.size, Display::None);
    }
}

void layoutColumnFlow(UI& ui, UIElementID parentId, LayoutRect content) {
    const float gap = ui.resolvedStyle(parentId).gap.resolve(content.size.y);
    float cursorTop = content.pos.y + content.size.y;
    bool placedFlowChild = false;

    layoutFlowAbsoluteChildren(ui, parentId, content);

    for (UIElementID childId : ui.get(parentId).children) {
        UIElement& child = ui.get(childId);
        const Style& childStyle = ui.resolvedStyle(childId);

        if (childStyle.position != PositionMode::Relative)
            continue;

        if (placedFlowChild)
            cursorTop -= gap;

        glm::vec2 intrinsic = {0.f, 0.f};
        if (child.widget)
            intrinsic = child.widget->measureContent(child, content.size);

        const float marginLeft = childStyle.margin.left.resolve(content.size.x);
        const float marginRight = childStyle.margin.right.resolve(content.size.x);
        const float marginTop = childStyle.margin.top.resolve(content.size.y);
        const float marginBottom = childStyle.margin.bottom.resolve(content.size.y);

        const float availableWidth =
            std::max(0.f, content.size.x - marginLeft - marginRight);

        const float childWidth = resolveBlockWidth(
            childStyle, availableWidth, content.size.x, intrinsic);
        const float childHeight = resolveBlockHeight(
            ui, childId, childStyle, content.size, intrinsic);

        cursorTop -= marginTop;
        const float childTop = cursorTop;
        const float childBottom = childTop - childHeight;

        child.transform.position = {content.pos.x + marginLeft, childBottom};
        child.transform.size = {childWidth, childHeight};

        cursorTop = childBottom - marginBottom;
        placedFlowChild = true;

        const LayoutRect childBorder = {child.transform.position, child.transform.size};
        const LayoutRect childContent = contentBox(childBorder, childStyle);
        layoutChildren(ui, childId, childContent, childStyle);
    }
}

void layoutRowFlow(UI& ui, UIElementID parentId, LayoutRect content) {
    const float gap = ui.resolvedStyle(parentId).gap.resolve(content.size.x);
    float cursorLeft = content.pos.x;
    bool placedFlowChild = false;

    layoutFlowAbsoluteChildren(ui, parentId, content);

    for (UIElementID childId : ui.get(parentId).children) {
        UIElement& child = ui.get(childId);
        const Style& childStyle = ui.resolvedStyle(childId);

        if (childStyle.position != PositionMode::Relative)
            continue;

        if (placedFlowChild)
            cursorLeft += gap;

        glm::vec2 intrinsic = {0.f, 0.f};
        if (child.widget)
            intrinsic = child.widget->measureContent(child, content.size);

        const float marginLeft = childStyle.margin.left.resolve(content.size.x);
        const float marginRight = childStyle.margin.right.resolve(content.size.x);
        const float marginTop = childStyle.margin.top.resolve(content.size.y);
        const float marginBottom = childStyle.margin.bottom.resolve(content.size.y);

        const float availableHeight =
            std::max(0.f, content.size.y - marginTop - marginBottom);

        const float childWidth = resolveBlockWidth(
            childStyle, content.size.x, content.size.x, intrinsic);

        float childHeight;
        if (!childStyle.height.isAuto())
            childHeight = childStyle.height.resolve(content.size.y);
        else if (intrinsic.y > 0.f)
            childHeight = intrinsic.y;
        else
            childHeight = availableHeight;
        childHeight = clampMinMax(
            childHeight, childStyle.minHeight, childStyle.maxHeight, content.size.y);
        childHeight = std::min(childHeight, availableHeight);

        cursorLeft += marginLeft;
        const float childLeft = cursorLeft;
        const float childBottom = content.pos.y + marginBottom;

        child.transform.position = {childLeft, childBottom};
        child.transform.size = {childWidth, childHeight};

        cursorLeft = childLeft + childWidth + marginRight;
        placedFlowChild = true;

        const LayoutRect childBorder = {child.transform.position, child.transform.size};
        const LayoutRect childContent = contentBox(childBorder, childStyle);
        layoutChildren(ui, childId, childContent, childStyle);
    }
}

void layoutChildren(
    UI& ui,
    UIElementID parentId,
    LayoutRect content,
    const Style& parentStyle)
{
    switch (parentStyle.display) {
        case Display::Block:
            layoutColumnFlow(ui, parentId, content);
            return;
        case Display::Flex:
            if (parentStyle.flexDirection == FlexDirection::Row)
                layoutRowFlow(ui, parentId, content);
            else
                layoutColumnFlow(ui, parentId, content);
            return;
        default:
            break;
    }

    for (UIElementID childId : ui.get(parentId).children) {
        layoutElement(
            ui, childId, content.pos, content.size,
            Display::None);
    }
}

} // namespace

void layoutTree(UI& ui, UIElementID rootId, glm::vec2 viewportSize) {
    UIElement& root = ui.get(rootId);
    root.transform.position = {0.f, 0.f};
    root.transform.size = viewportSize;

    for (UIElementID child : root.children) {
        layoutElement(
            ui, child, root.transform.position, root.transform.size,
            Display::None);
    }
}
