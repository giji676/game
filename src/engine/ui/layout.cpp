#include "engine/ui/layout.h"

#include <algorithm>
#include <vector>

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

size_t countFlowChildren(UI& ui, UIElementID parentId)
{
    size_t count = 0;
    for (UIElementID childId : ui.get(parentId).children) {
        if (ui.resolvedStyle(childId).position == PositionMode::Relative)
            ++count;
    }
    return count;
}

bool percentFillsContainerOnMainAxis(
    const Style& style,
    bool mainIsWidth,
    size_t flowSiblingCount)
{
    if (flowSiblingCount <= 1 || style.flexGrow > 0.f)
        return false;

    const Length& explicitSize = mainIsWidth ? style.width : style.height;
    if (!style.flexBasis.isAuto() &&
        style.flexBasis.unit == Unit::Percent &&
        style.flexBasis.value >= 100.f)
        return true;
    if (!explicitSize.isAuto() &&
        explicitSize.unit == Unit::Percent &&
        explicitSize.value >= 100.f)
        return true;
    return false;
}

float resolveFlowMainSize(
    UI& ui,
    UIElementID id,
    const Style& style,
    glm::vec2 contentSize,
    glm::vec2 intrinsic,
    bool mainIsWidth,
    size_t flowSiblingCount)
{
    const float containerMain = mainIsWidth ? contentSize.x : contentSize.y;
    const float intrinsicMain = mainIsWidth ? intrinsic.x : intrinsic.y;
    const Length& minLen = mainIsWidth ? style.minWidth : style.minHeight;
    const Length& maxLen = mainIsWidth ? style.maxWidth : style.maxHeight;
    const Length& explicitSize = mainIsWidth ? style.width : style.height;

    const bool useIntrinsicForPercent = percentFillsContainerOnMainAxis(
        style, mainIsWidth, flowSiblingCount);

    float size;
    if (!style.flexBasis.isAuto() && !useIntrinsicForPercent)
        size = style.flexBasis.resolve(containerMain);
    else if (!explicitSize.isAuto() && !useIntrinsicForPercent)
        size = explicitSize.resolve(containerMain);
    else if (intrinsicMain > 0.f)
        size = intrinsicMain;
    else
        size = mainIsWidth ? ui.get(id).transform.size.x : ui.get(id).transform.size.y;

    return clampMinMax(size, minLen, maxLen, containerMain);
}

struct FlowMetrics {
    UIElementID id = INVALID_UI_ELEMENT;
    float mainBefore = 0.f;
    float mainSize = 0.f;
    float mainAfter = 0.f;
};

void applyFlexGrow(
    UI& ui,
    float containerMain,
    float gap,
    bool mainIsWidth,
    glm::vec2 contentSize,
    std::vector<FlowMetrics>& items)
{
    if (items.empty())
        return;

    float total = 0.f;
    float growSum = 0.f;
    for (const FlowMetrics& item : items) {
        total += item.mainBefore + item.mainSize + item.mainAfter;
        growSum += ui.resolvedStyle(item.id).flexGrow;
    }
    if (items.size() > 1)
        total += gap * static_cast<float>(items.size() - 1);

    const float free = containerMain - total;
    if (free <= 0.f || growSum <= 0.f)
        return;

    const float basis = mainIsWidth ? contentSize.x : contentSize.y;
    for (FlowMetrics& item : items) {
        const Style& style = ui.resolvedStyle(item.id);
        if (style.flexGrow <= 0.f)
            continue;

        item.mainSize += free * (style.flexGrow / growSum);
        if (mainIsWidth)
            item.mainSize = clampMinMax(
                item.mainSize, style.minWidth, style.maxWidth, basis);
        else
            item.mainSize = clampMinMax(
                item.mainSize, style.minHeight, style.maxHeight, basis);
    }
}

struct FlowMainAxisLayout {
    float startOffset = 0.f;
    float gap = 0.f;
};

FlowMainAxisLayout computeMainAxisLayout(
    float containerSize,
    const std::vector<FlowMetrics>& items,
    float gap,
    JustifyContent justify)
{
    FlowMainAxisLayout result{.startOffset = 0.f, .gap = gap};
    if (items.empty())
        return result;

    float total = 0.f;
    for (const FlowMetrics& item : items)
        total += item.mainBefore + item.mainSize + item.mainAfter;
    if (items.size() > 1)
        total += gap * static_cast<float>(items.size() - 1);

    const float free = containerSize - total;
    if (items.size() == 1) {
        switch (justify) {
            case JustifyContent::Center:
                result.startOffset = free * 0.5f;
                break;
            case JustifyContent::End:
                result.startOffset = free;
                break;
            default:
                break;
        }
        result.gap = 0.f;
        return result;
    }

    switch (justify) {
        case JustifyContent::Center:
            result.startOffset = free * 0.5f;
            break;
        case JustifyContent::End:
            result.startOffset = free;
            break;
        case JustifyContent::SpaceBetween:
            result.gap = gap + free / static_cast<float>(items.size() - 1);
            break;
        default:
            break;
    }

    return result;
}

float flowMainExtent(
    const std::vector<FlowMetrics>& items,
    float gap)
{
    if (items.empty())
        return 0.f;

    float total = 0.f;
    for (const FlowMetrics& item : items)
        total += item.mainBefore + item.mainSize + item.mainAfter;
    if (items.size() > 1)
        total += gap * static_cast<float>(items.size() - 1);
    return total;
}

void offsetElementTree(UI& ui, UIElementID id, glm::vec2 scrollOffset)
{
    UIElement& e = ui.get(id);
    // Y-up: scroll down (offset.y increases) moves content up to reveal lower items.
    e.transform.position.x -= scrollOffset.x;
    e.transform.position.y += scrollOffset.y;
    for (UIElementID child : e.children)
        offsetElementTree(ui, child, scrollOffset);
}

void applyOverflowScroll(
    UI& ui,
    UIElementID parentId,
    LayoutRect content,
    const Style& parentStyle,
    const std::vector<FlowMetrics>& items,
    float gap,
    bool verticalFlow)
{
    if (parentStyle.overflow == Overflow::Visible)
        return;

    UIElement& parent = ui.get(parentId);
    parent.scrollViewportSize = content.size;

    const float mainExtent = flowMainExtent(items, gap);
    if (verticalFlow)
        parent.scrollContentSize = {content.size.x, mainExtent};
    else
        parent.scrollContentSize = {mainExtent, content.size.y};

    if (parentStyle.overflow == Overflow::Scroll) {
        const float maxScrollX = std::max(
            0.f, parent.scrollContentSize.x - parent.scrollViewportSize.x);
        const float maxScrollY = std::max(
            0.f, parent.scrollContentSize.y - parent.scrollViewportSize.y);
        parent.scrollOffset.x = std::clamp(parent.scrollOffset.x, 0.f, maxScrollX);
        parent.scrollOffset.y = std::clamp(parent.scrollOffset.y, 0.f, maxScrollY);

        for (const FlowMetrics& item : items)
            offsetElementTree(ui, item.id, parent.scrollOffset);
    } else {
        parent.scrollOffset = {0.f, 0.f};
    }
}

float crossAxisPosition(
    AlignItems align,
    float contentOrigin,
    float contentSize,
    float marginStart,
    float marginEnd,
    float itemSize,
    bool axisIsVertical)
{
    const float inner = contentSize - marginStart - marginEnd;
    switch (align) {
        case AlignItems::End:
            if (axisIsVertical)
                return contentOrigin + marginEnd;
            return contentOrigin + contentSize - marginEnd - itemSize;
        case AlignItems::Center: {
            const float free = std::max(0.f, inner - itemSize) * 0.5f;
            if (axisIsVertical)
                return contentOrigin + marginEnd + free;
            return contentOrigin + marginStart + free;
        }
        case AlignItems::Stretch:
        case AlignItems::Start:
        default:
            if (axisIsVertical)
                return contentOrigin + contentSize - marginStart - itemSize;
            return contentOrigin + marginStart;
    }
}

float resolveFlowCrossHeight(
    UI& ui,
    UIElementID id,
    const Style& style,
    glm::vec2 contentSize,
    glm::vec2 intrinsic,
    float availableHeight,
    AlignItems align)
{
    float height;
    if (!style.height.isAuto())
        height = style.height.resolve(contentSize.y);
    else if (align == AlignItems::Stretch)
        height = availableHeight;
    else if (intrinsic.y > 0.f)
        height = intrinsic.y;
    else
        height = ui.get(id).transform.size.y;

    height = clampMinMax(height, style.minHeight, style.maxHeight, contentSize.y);
    return std::min(height, availableHeight);
}

float resolveFlexCrossWidth(
    const Style& style,
    float availableWidth,
    float basisX,
    glm::vec2 intrinsic,
    AlignItems align)
{
    float width;
    if (!style.width.isAuto())
        width = style.width.resolve(basisX);
    else if (align == AlignItems::Stretch)
        width = availableWidth;
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

void layoutColumnFlow(
    UI& ui,
    UIElementID parentId,
    LayoutRect content,
    const Style& parentStyle)
{
    layoutFlowAbsoluteChildren(ui, parentId, content);

    const float gap = parentStyle.gap.resolve(content.size.y);
    const size_t flowChildCount = countFlowChildren(ui, parentId);
    std::vector<FlowMetrics> items;
    items.reserve(ui.get(parentId).children.size());

    for (UIElementID childId : ui.get(parentId).children) {
        const Style& childStyle = ui.resolvedStyle(childId);
        if (childStyle.position != PositionMode::Relative)
            continue;

        UIElement& child = ui.get(childId);
        glm::vec2 intrinsic = {0.f, 0.f};
        if (child.widget)
            intrinsic = child.widget->measureContent(child, content.size);

        const float marginTop = childStyle.margin.top.resolve(content.size.y);
        const float marginBottom = childStyle.margin.bottom.resolve(content.size.y);
        const float childHeight = resolveFlowMainSize(
            ui, childId, childStyle, content.size, intrinsic, false, flowChildCount);

        items.push_back({
            childId,
            marginTop,
            childHeight,
            marginBottom,
        });
    }

    if (parentStyle.display == Display::Flex)
        applyFlexGrow(ui, content.size.y, gap, false, content.size, items);

    const FlowMainAxisLayout mainAxis = computeMainAxisLayout(
        content.size.y, items, gap, parentStyle.justifyContent);

    const AlignItems crossAlign = parentStyle.display == Display::Flex
        ? parentStyle.alignItems
        : AlignItems::Stretch;

    float cursorTop = content.pos.y + content.size.y - mainAxis.startOffset;
    bool placedFlowChild = false;

    for (const FlowMetrics& item : items) {
        UIElement& child = ui.get(item.id);
        const Style& childStyle = ui.resolvedStyle(item.id);

        if (placedFlowChild)
            cursorTop -= mainAxis.gap;

        glm::vec2 intrinsic = {0.f, 0.f};
        if (child.widget)
            intrinsic = child.widget->measureContent(child, content.size);

        const float marginLeft = childStyle.margin.left.resolve(content.size.x);
        const float marginRight = childStyle.margin.right.resolve(content.size.x);
        const float availableWidth =
            std::max(0.f, content.size.x - marginLeft - marginRight);
        const float childWidth = parentStyle.display == Display::Flex
            ? resolveFlexCrossWidth(
                childStyle, availableWidth, content.size.x, intrinsic, crossAlign)
            : resolveBlockWidth(
                childStyle, availableWidth, content.size.x, intrinsic);

        cursorTop -= item.mainBefore;
        const float childBottom = cursorTop - item.mainSize;
        const float childLeft = crossAxisPosition(
            crossAlign,
            content.pos.x,
            content.size.x,
            marginLeft,
            marginRight,
            childWidth,
            false);

        child.transform.position = {childLeft, childBottom};
        child.transform.size = {childWidth, item.mainSize};

        cursorTop = childBottom - item.mainAfter;
        placedFlowChild = true;

        const LayoutRect childBorder = {child.transform.position, child.transform.size};
        layoutChildren(ui, item.id, contentBox(childBorder, childStyle), childStyle);
    }

    applyOverflowScroll(
        ui, parentId, content, parentStyle, items, gap, true);
}

void layoutRowFlow(
    UI& ui,
    UIElementID parentId,
    LayoutRect content,
    const Style& parentStyle)
{
    layoutFlowAbsoluteChildren(ui, parentId, content);

    const float gap = parentStyle.gap.resolve(content.size.x);
    const size_t flowChildCount = countFlowChildren(ui, parentId);
    std::vector<FlowMetrics> items;
    items.reserve(ui.get(parentId).children.size());

    for (UIElementID childId : ui.get(parentId).children) {
        const Style& childStyle = ui.resolvedStyle(childId);
        if (childStyle.position != PositionMode::Relative)
            continue;

        UIElement& child = ui.get(childId);
        glm::vec2 intrinsic = {0.f, 0.f};
        if (child.widget)
            intrinsic = child.widget->measureContent(child, content.size);

        const float marginLeft = childStyle.margin.left.resolve(content.size.x);
        const float marginRight = childStyle.margin.right.resolve(content.size.x);
        const float childWidth = resolveFlowMainSize(
            ui, childId, childStyle, content.size, intrinsic, true, flowChildCount);

        items.push_back({
            childId,
            marginLeft,
            childWidth,
            marginRight,
        });
    }

    applyFlexGrow(ui, content.size.x, gap, true, content.size, items);

    const FlowMainAxisLayout mainAxis = computeMainAxisLayout(
        content.size.x, items, gap, parentStyle.justifyContent);

    const AlignItems crossAlign = parentStyle.alignItems;

    float cursorLeft = content.pos.x + mainAxis.startOffset;
    bool placedFlowChild = false;

    for (const FlowMetrics& item : items) {
        UIElement& child = ui.get(item.id);
        const Style& childStyle = ui.resolvedStyle(item.id);

        if (placedFlowChild)
            cursorLeft += mainAxis.gap;

        glm::vec2 intrinsic = {0.f, 0.f};
        if (child.widget)
            intrinsic = child.widget->measureContent(child, content.size);

        const float marginTop = childStyle.margin.top.resolve(content.size.y);
        const float marginBottom = childStyle.margin.bottom.resolve(content.size.y);
        const float availableHeight =
            std::max(0.f, content.size.y - marginTop - marginBottom);
        const float childHeight = resolveFlowCrossHeight(
            ui, item.id, childStyle, content.size, intrinsic, availableHeight,
            crossAlign);

        cursorLeft += item.mainBefore;
        const float childBottom = crossAxisPosition(
            crossAlign,
            content.pos.y,
            content.size.y,
            marginTop,
            marginBottom,
            childHeight,
            true);

        child.transform.position = {cursorLeft, childBottom};
        child.transform.size = {item.mainSize, childHeight};

        cursorLeft += item.mainSize + item.mainAfter;
        placedFlowChild = true;

        const LayoutRect childBorder = {child.transform.position, child.transform.size};
        layoutChildren(ui, item.id, contentBox(childBorder, childStyle), childStyle);
    }

    applyOverflowScroll(
        ui, parentId, content, parentStyle, items, gap, false);
}

void layoutChildren(
    UI& ui,
    UIElementID parentId,
    LayoutRect content,
    const Style& parentStyle)
{
    switch (parentStyle.display) {
        case Display::Block:
            layoutColumnFlow(ui, parentId, content, parentStyle);
            return;
        case Display::Flex:
            if (parentStyle.flexDirection == FlexDirection::Row)
                layoutRowFlow(ui, parentId, content, parentStyle);
            else
                layoutColumnFlow(ui, parentId, content, parentStyle);
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
