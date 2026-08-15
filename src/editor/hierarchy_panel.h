#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#include "engine/defines.h"

class EditorPanel;
class Scene;
class UI;

// Tree view of Scene objects, parented into an EditorPanel content region.
class HierarchyPanel {
public:
    void bind(UI& ui, EditorPanel& panel);
    void update(Scene& scene, bool interactive = true);

    ObjectID selectedId() const { return selectedId_; }
    void setSelectedId(ObjectID id) { selectedId_ = id; }
    bool isDragging() const { return draggingId_ != INVALID_OBJECT; }
    ObjectID draggingId() const { return draggingId_; }
    ObjectID releasedDragId() const { return releasedDragId_; }

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

    enum class DropKind {
        None,
        Reparent,
        InsertBefore,
        InsertAfter,
    };

    struct DropTarget {
        DropKind kind = DropKind::None;
        ObjectID targetId = INVALID_OBJECT;
        bool valid = false;
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
    void applyRowMetrics(Row& row) const;
    void styleToggle(UIElementID id, bool hasChildren, bool expanded) const;
    void styleLabel(UIElementID id, bool selected, bool dropHover = false) const;
    void syncObjectDebug(Scene& scene) const;
    void updateDrag(Scene& scene);
    DropTarget hitTestDrop(Scene& scene, glm::vec2 mouse) const;
    bool canDropOn(Scene& scene, ObjectID draggedId, const DropTarget& drop) const;
    void applyDrop(Scene& scene, ObjectID draggedId, const DropTarget& drop);
    void syncDropPreview(Scene& scene);
    int siblingIndex(Scene& scene, ObjectID id) const;
    std::string treePrefix(const Entry& entry) const;
    uint64_t sceneSignature(Scene& scene) const;

    UI* ui_ = nullptr;
    UIElementID contentId_ = INVALID_UI_ELEMENT;
    UIElementID dropLineId_ = INVALID_UI_ELEMENT;
    std::vector<Row> rows_;
    std::unordered_set<ObjectID> collapsedIds_;
    ObjectID selectedId_ = INVALID_OBJECT;
    uint64_t lastSignature_ = 0;
    bool built_ = false;

    ObjectID dragCandidateId_ = INVALID_OBJECT;
    ObjectID draggingId_ = INVALID_OBJECT;
    ObjectID releasedDragId_ = INVALID_OBJECT;
    glm::vec2 dragStartMouse_ = {0.f, 0.f};
    bool dragMoved_ = false;
    DropTarget activeDrop_;
};
