#include "editor_layout.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kEdgeBandRatio = 0.25f;
constexpr float kInsertRatio = 0.5f;
constexpr float kMinSplitRatio = 0.15f;
constexpr float kMaxSplitRatio = 0.85f;
}

int EditrLayout::createLeaf(UIElementID panelId) {
    DockNode node;
    node.isLeaf = true;
    node.panelId = panelId;
    nodes_.push_back(node);
    return static_cast<int>(nodes_.size() - 1);
}

int EditrLayout::createSplit(SplitAxis axis, float splitRatio, int firstChild, int secondChild) {
    DockNode node;
    node.isLeaf = false;
    node.axis = axis;
    node.splitRatio = splitRatio;
    node.firstChild = firstChild;
    node.secondChild = secondChild;
    nodes_.push_back(node);
    return static_cast<int>(nodes_.size() - 1);
}

void EditrLayout::initialize(const std::vector<UIElementID>& panels, glm::vec4 bounds) {
    nodes_.clear();
    panelRects_.clear();
    nodeRects_.clear();
    rootNode_ = -1;
    bounds_ = bounds;

    std::vector<UIElementID> validPanels;
    for (UIElementID id : panels) {
        if (id != INVALID_UI_ELEMENT)
            validPanels.push_back(id);
    }

    if (validPanels.empty())
        return;

    rootNode_ = createLeaf(validPanels[0]);
    for (size_t i = 1; i < validPanels.size(); ++i) {
        const int newLeaf = createLeaf(validPanels[i]);
        rootNode_ = createSplit(SplitAxis::Vertical, 0.5f, rootNode_, newLeaf);
    }

    relayout(bounds_);
}

void EditrLayout::assignRectsRecursive(int nodeIndex, const glm::vec4& rect) {
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes_.size()))
        return;

    nodeRects_[nodeIndex] = rect;

    const DockNode& node = nodes_[nodeIndex];
    if (node.isLeaf) {
        if (node.panelId != INVALID_UI_ELEMENT)
            panelRects_[node.panelId] = rect;
        return;
    }

    const float split = std::clamp(node.splitRatio, kMinSplitRatio, kMaxSplitRatio);
    glm::vec4 firstRect = rect;
    glm::vec4 secondRect = rect;

    if (node.axis == SplitAxis::Vertical) {
        const float firstW = std::floor(rect.z * split);
        const float secondW = rect.z - firstW;
        firstRect.z = firstW;
        secondRect.x = rect.x + firstW;
        secondRect.z = secondW;
    } else {
        const float firstH = std::floor(rect.w * split);
        const float secondH = rect.w - firstH;
        firstRect.w = firstH;
        secondRect.y = rect.y + firstH;
        secondRect.w = secondH;
    }

    assignRectsRecursive(node.firstChild, firstRect);
    assignRectsRecursive(node.secondChild, secondRect);
}

void EditrLayout::relayout(glm::vec4 bounds) {
    bounds_ = bounds;
    panelRects_.clear();
    nodeRects_.clear();
    if (rootNode_ == -1)
        return;
    assignRectsRecursive(rootNode_, bounds_);
}

glm::vec4 EditrLayout::panelRect(UIElementID panelId) const {
    auto it = panelRects_.find(panelId);
    if (it == panelRects_.end())
        return {0.f, 0.f, 0.f, 0.f};
    return it->second;
}

void EditrLayout::setPanelRect(UIElementID panelId, const glm::vec4& rect) {
    if (panelId == INVALID_UI_ELEMENT)
        return;
    panelRects_[panelId] = rect;
}

int EditrLayout::findLeafNode(UIElementID panelId) const {
    for (size_t i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i].isLeaf && nodes_[i].panelId == panelId)
            return static_cast<int>(i);
    }
    return -1;
}

int EditrLayout::findParentInTree(int root, int child) const {
    if (root < 0 || root >= static_cast<int>(nodes_.size()))
        return -1;

    std::vector<int> stack = {root};
    while (!stack.empty()) {
        const int nodeIndex = stack.back();
        stack.pop_back();
        const DockNode& node = nodes_[nodeIndex];
        if (node.isLeaf)
            continue;
        if (node.firstChild == child || node.secondChild == child)
            return nodeIndex;
        if (node.firstChild >= 0)
            stack.push_back(node.firstChild);
        if (node.secondChild >= 0)
            stack.push_back(node.secondChild);
    }
    return -1;
}

EditrLayout::DockZone EditrLayout::zoneForMouse(const glm::vec4& rect, glm::vec2 mouse) const {
    const float leftBand = rect.x + rect.z * kEdgeBandRatio;
    const float rightBand = rect.x + rect.z * (1.f - kEdgeBandRatio);
    const float bottomBand = rect.y + rect.w * kEdgeBandRatio;
    const float topBand = rect.y + rect.w * (1.f - kEdgeBandRatio);

    if (mouse.x <= leftBand)
        return DockZone::Left;
    if (mouse.x >= rightBand)
        return DockZone::Right;
    if (mouse.y <= bottomBand)
        return DockZone::Bottom;
    if (mouse.y >= topBand)
        return DockZone::Top;
    return DockZone::None;
}

EditrLayout::DockPreview EditrLayout::findDockPreview(const EditorPanel& draggingPanel, glm::vec2 mouse) const {
    DockPreview preview;
    const UIElementID draggingId = draggingPanel.rootId();

    for (const auto& pair : panelRects_) {
        const UIElementID panelId = pair.first;
        const glm::vec4 rect = pair.second;
        if (panelId == draggingId)
            continue;
        if (mouse.x < rect.x || mouse.x > rect.x + rect.z || mouse.y < rect.y || mouse.y > rect.y + rect.w)
            continue;

        const DockZone zone = zoneForMouse(rect, mouse);
        if (zone == DockZone::None)
            return preview;

        preview.targetPanelId = panelId;
        preview.zone = zone;
        preview.valid = true;

        if (zone == DockZone::Left) {
            preview.previewRect = {rect.x, rect.y, rect.z * kInsertRatio, rect.w};
        } else if (zone == DockZone::Right) {
            const float w = rect.z * kInsertRatio;
            preview.previewRect = {rect.x + rect.z - w, rect.y, w, rect.w};
        } else if (zone == DockZone::Bottom) {
            preview.previewRect = {rect.x, rect.y, rect.z, rect.w * kInsertRatio};
        } else if (zone == DockZone::Top) {
            const float h = rect.w * kInsertRatio;
            preview.previewRect = {rect.x, rect.y + rect.w - h, rect.z, h};
        }
        return preview;
    }

    return preview;
}

bool EditrLayout::dockPanel(UIElementID draggingPanelId, const DockPreview& preview) {
    if (!preview.valid || preview.zone == DockZone::None)
        return false;

    const int draggingLeaf = findLeafNode(draggingPanelId);
    const int targetLeaf = findLeafNode(preview.targetPanelId);
    if (draggingLeaf == -1 || targetLeaf == -1 || draggingLeaf == targetLeaf)
        return false;

    const int draggingParent = findParentInTree(rootNode_, draggingLeaf);
    if (draggingParent == -1)
        return false;

    const int draggingGrandParent = findParentInTree(rootNode_, draggingParent);
    const int draggingSibling =
        (nodes_[draggingParent].firstChild == draggingLeaf)
            ? nodes_[draggingParent].secondChild
            : nodes_[draggingParent].firstChild;

    if (draggingGrandParent == -1) {
        rootNode_ = draggingSibling;
    } else {
        DockNode& grand = nodes_[draggingGrandParent];
        if (grand.firstChild == draggingParent) {
            grand.firstChild = draggingSibling;
        } else if (grand.secondChild == draggingParent) {
            grand.secondChild = draggingSibling;
        }
    }

    int firstChild = targetLeaf;
    int secondChild = draggingLeaf;
    SplitAxis axis = SplitAxis::Vertical;

    if (preview.zone == DockZone::Left || preview.zone == DockZone::Right) {
        axis = SplitAxis::Vertical;
        if (preview.zone == DockZone::Left) {
            firstChild = draggingLeaf;
            secondChild = targetLeaf;
        }
    } else {
        axis = SplitAxis::Horizontal;
        if (preview.zone == DockZone::Bottom) {
            firstChild = draggingLeaf;
            secondChild = targetLeaf;
        }
    }

    const int splitNode = createSplit(axis, 0.5f, firstChild, secondChild);

    const int targetParent = findParentInTree(rootNode_, targetLeaf);
    if (rootNode_ == targetLeaf) {
        rootNode_ = splitNode;
    } else if (targetParent != -1) {
        DockNode& parent = nodes_[targetParent];
        if (parent.firstChild == targetLeaf) {
            parent.firstChild = splitNode;
        } else if (parent.secondChild == targetLeaf) {
            parent.secondChild = splitNode;
        } else {
            return false;
        }
    } else {
        return false;
    }

    return true;
}

EditrLayout::ResizeHandle EditrLayout::findResizeHandle(
        UIElementID panelId,
        SplitAxis axis,
        bool growFirst) const
{
    ResizeHandle handle;
    int node = findLeafNode(panelId);
    if (node == -1)
        return handle;

    while (true) {
        const int parent = findParentInTree(rootNode_, node);
        if (parent == -1)
            break;

        const DockNode& parentNode = nodes_[parent];
        const bool isFirst = (parentNode.firstChild == node);
        if (parentNode.axis == axis && isFirst == growFirst) {
            handle.splitNode = parent;
            handle.startRatio = parentNode.splitRatio;
            const auto it = nodeRects_.find(parent);
            if (it != nodeRects_.end()) {
                handle.axisSize = (axis == SplitAxis::Vertical) ? it->second.z : it->second.w;
            } else {
                handle.axisSize = (axis == SplitAxis::Vertical) ? bounds_.z : bounds_.w;
            }
            handle.axisSize = std::max(1.f, handle.axisSize);
            handle.valid = true;
            return handle;
        }

        node = parent;
    }

    return handle;
}

EditrLayout::ResizeHandle EditrLayout::beginResize(UIElementID panelId, PanelDrag drag) const {
    // Vertical split: first = left, second = right.
    // Horizontal split: first = bottom, second = top.
    switch (drag) {
        case PanelDrag::Left:
            return findResizeHandle(panelId, SplitAxis::Vertical, false);
        case PanelDrag::Right:
            return findResizeHandle(panelId, SplitAxis::Vertical, true);
        case PanelDrag::Bottom:
            return findResizeHandle(panelId, SplitAxis::Horizontal, false);
        case PanelDrag::Top:
            return findResizeHandle(panelId, SplitAxis::Horizontal, true);
        case PanelDrag::BottomLeft:
        case PanelDrag::BottomRight:
        case PanelDrag::TopLeft:
        case PanelDrag::TopRight:
            // Corners are handled by calling beginResize twice from Editor.
            return {};
        default:
            return {};
    }
}

bool EditrLayout::updateResize(const ResizeHandle& handle, float mouseDeltaAlongAxis) {
    if (!handle.valid || handle.splitNode < 0 || handle.splitNode >= static_cast<int>(nodes_.size()))
        return false;

    DockNode& node = nodes_[handle.splitNode];
    if (node.isLeaf)
        return false;

    const float ratio = handle.startRatio + (mouseDeltaAlongAxis / handle.axisSize);
    node.splitRatio = std::clamp(ratio, kMinSplitRatio, kMaxSplitRatio);
    return true;
}
