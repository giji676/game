#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#include "engine/defines.h"
#include "engine/entity.h"

class EditorPanel;
class UI;
class World;

// Tree view of World entities, parented into an EditorPanel content region.
class HierarchyPanel {
public:
    void bind(UI& ui, EditorPanel& panel);
    void update(World& world, bool interactive = true);

    Entity selectedId() const { return selectedId_; }
    void setSelectedId(Entity id) { selectedId_ = id; }
    bool isDragging() const { return draggingId_ != Entity::invalid(); }
    Entity draggingId() const { return draggingId_; }
    Entity releasedDragId() const { return releasedDragId_; }

private:
    struct Row {
        UIElementID rootId = INVALID_UI_ELEMENT;
        UIElementID toggleId = INVALID_UI_ELEMENT;
        UIElementID labelId = INVALID_UI_ELEMENT;
        Entity objectId = Entity::invalid();
    };

    struct Entry {
        Entity id = Entity::invalid();
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
        Entity targetId = Entity::invalid();
        bool valid = false;
    };

    void rebuildRows(World& world);
    void collectEntries(
        World& world,
        Entity id,
        int depth,
        bool isLastSibling,
        const std::vector<bool>& ancestorOpen,
        std::vector<Entry>& out) const;
    void ensureRowCount(size_t count);
    void applyRowMetrics(Row& row) const;
    void styleToggle(UIElementID id, bool hasChildren, bool expanded) const;
    void styleLabel(UIElementID id, bool selected, bool dropHover = false) const;
    void syncObjectDebug(World& world) const;
    void updateDrag(World& world);
    DropTarget hitTestDrop(World& world, glm::vec2 mouse) const;
    bool canDropOn(World& world, Entity draggedId, const DropTarget& drop) const;
    void applyDrop(World& world, Entity draggedId, const DropTarget& drop);
    void syncDropPreview(World& world);
    int siblingIndex(World& world, Entity id) const;
    std::string treePrefix(const Entry& entry) const;
    uint64_t sceneSignature(World& world) const;

    UI* ui_ = nullptr;
    UIElementID contentId_ = INVALID_UI_ELEMENT;
    UIElementID dropLineId_ = INVALID_UI_ELEMENT;
    std::vector<Row> rows_;
    std::unordered_set<Entity> collapsedIds_;
    Entity selectedId_ = Entity::invalid();
    uint64_t lastSignature_ = 0;
    bool built_ = false;

    Entity dragCandidateId_ = Entity::invalid();
    Entity draggingId_ = Entity::invalid();
    Entity releasedDragId_ = Entity::invalid();
    glm::vec2 dragStartMouse_ = {0.f, 0.f};
    bool dragMoved_ = false;
    DropTarget activeDrop_;
};
