#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

#include "engine/defines.h"
#include "panel.h"

class EditrLayout {
public:
    enum class SplitAxis {
        Vertical,
        Horizontal,
    };
    enum class DockZone {
        None,
        Left,
        Right,
        Top,
        Bottom,
    };

    struct DockPreview {
        UIElementID targetPanelId = INVALID_UI_ELEMENT;
        DockZone zone = DockZone::None;
        glm::vec4 previewRect = {0.f, 0.f, 0.f, 0.f};
        bool valid = false;
    };

    struct ResizeHandle {
        int splitNode = -1;
        float startRatio = 0.5f;
        float axisSize = 1.f;
        bool valid = false;
    };

    void initialize(const std::vector<UIElementID>& panels, glm::vec4 bounds);
    DockPreview findDockPreview(const EditorPanel& draggingPanel, glm::vec2 mouse) const;
    bool dockPanel(UIElementID draggingPanelId, const DockPreview& preview);
    void relayout(glm::vec4 bounds);
    glm::vec4 panelRect(UIElementID panelId) const;
    void setPanelRect(UIElementID panelId, const glm::vec4& rect);

    // Begin/update a docked edge resize that pushes the adjacent split sibling.
    ResizeHandle beginResize(UIElementID panelId, PanelDrag drag) const;
    bool updateResize(const ResizeHandle& handle, float mouseDeltaAlongAxis);

private:
    struct DockNode {
        bool isLeaf = true;
        UIElementID panelId = INVALID_UI_ELEMENT;
        SplitAxis axis = SplitAxis::Vertical;
        float splitRatio = 0.5f;
        int firstChild = -1;
        int secondChild = -1;
    };

    int createLeaf(UIElementID panelId);
    int createSplit(SplitAxis axis, float splitRatio, int firstChild, int secondChild);
    void assignRectsRecursive(int nodeIndex, const glm::vec4& rect);
    int findLeafNode(UIElementID panelId) const;
    int findParentInTree(int root, int child) const;
    DockZone zoneForMouse(const glm::vec4& rect, glm::vec2 mouse) const;
    ResizeHandle findResizeHandle(UIElementID panelId, SplitAxis axis, bool growFirst) const;

    std::vector<DockNode> nodes_;
    int rootNode_ = -1;
    glm::vec4 bounds_ = {0.f, 0.f, 0.f, 0.f};
    std::unordered_map<UIElementID, glm::vec4> panelRects_;
    std::unordered_map<int, glm::vec4> nodeRects_;
};
