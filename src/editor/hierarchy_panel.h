#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "engine/defines.h"

class EditorPanel;
class Scene;
class UI;

// Tree view of Scene objects, parented into an EditorPanel content region.
class HierarchyPanel {
public:
    void bind(UI& ui, EditorPanel& panel);
    void update(Scene& scene);

    ObjectID selectedId() const { return selectedId_; }
    void setSelectedId(ObjectID id) { selectedId_ = id; }

private:
    struct Row {
        UIElementID rootId = INVALID_UI_ELEMENT;
        UIElementID toggleId = INVALID_UI_ELEMENT;
        UIElementID labelId = INVALID_UI_ELEMENT;
        ObjectID objectId = INVALID_OBJECT;
    };

    struct Entry {
        ObjectID id = INVALID_OBJECT;
        int depth = 0;
        bool hasChildren = false;
        bool isExpanded = false;
        bool isLastSibling = false;
        // For each ancestor depth above this node: true if a vertical guide continues.
        std::vector<bool> ancestorOpen;
    };

    void rebuildRows(Scene& scene);
    void collectEntries(
        Scene& scene,
        ObjectID id,
        int depth,
        bool isLastSibling,
        const std::vector<bool>& ancestorOpen,
        std::vector<Entry>& out) const;
    void ensureRowCount(size_t count);
    void styleToggle(UIElementID id, bool hasChildren, bool expanded) const;
    void styleLabel(UIElementID id, bool selected) const;
    std::string treePrefix(const Entry& entry) const;
    uint64_t sceneSignature(Scene& scene) const;

    UI* ui_ = nullptr;
    UIElementID contentId_ = INVALID_UI_ELEMENT;
    std::vector<Row> rows_;
    std::unordered_set<ObjectID> collapsedIds_;
    ObjectID selectedId_ = INVALID_OBJECT;
    uint64_t lastSignature_ = 0;
    bool built_ = false;
};
