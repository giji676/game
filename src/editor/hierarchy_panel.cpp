#include "editor/hierarchy_panel.h"

#include "editor/panel.h"
#include "engine/asset_manager/object.h"
#include "engine/asset_manager/widgets.h"
#include "engine/scene.h"
#include "engine/ui.h"
#include "engine/ui/style.h"

#include <functional>
#include <string>

namespace {

// Font size is the sizing input. Button measureContent derives row height.
constexpr float kRowFontSize = 14.f;
constexpr float kLabelPadX = 4.f;
constexpr float kLabelPadY = 4.f;

std::string objectLabel(const Object& obj, ObjectID rootId) {
    if (obj.getID() == rootId)
        return "Scene";
    return "Object " + std::to_string(obj.getID());
}

} // namespace

void HierarchyPanel::bind(UI& ui, EditorPanel& panel) {
    ui_ = &ui;
    contentId_ = panel.contentId();
    built_ = true;
    lastSignature_ = 0;

    UIElement& content = ui_->get(contentId_);
    content.style.display = Display::Block;
    content.style.gap = Length::px(0.f);
}

void HierarchyPanel::collectEntries(
        Scene& scene,
        ObjectID id,
        int depth,
        bool isLastSibling,
        const std::vector<bool>& ancestorOpen,
        std::vector<Entry>& out) const
{
    const Object& obj = scene.get(id);
    const bool hasChildren = !obj.children.empty();
    const bool isExpanded = hasChildren && collapsedIds_.find(id) == collapsedIds_.end();

    Entry entry;
    entry.id = id;
    entry.depth = depth;
    entry.hasChildren = hasChildren;
    entry.isExpanded = isExpanded;
    entry.isLastSibling = isLastSibling;
    entry.ancestorOpen = ancestorOpen;
    out.push_back(entry);

    if (!isExpanded)
        return;

    std::vector<bool> childAncestors = ancestorOpen;
    if (depth > 0)
        childAncestors.push_back(!isLastSibling);

    for (size_t i = 0; i < obj.children.size(); ++i) {
        const bool childIsLast = (i + 1 == obj.children.size());
        collectEntries(
            scene,
            obj.children[i],
            depth + 1,
            childIsLast,
            childAncestors,
            out);
    }
}

uint64_t HierarchyPanel::sceneSignature(Scene& scene) const {
    uint64_t hash = 1469598103934665603ull;
    auto mix = [&](uint64_t value) {
        hash ^= value + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
    };

    std::function<void(ObjectID)> walk = [&](ObjectID id) {
        const Object& obj = scene.get(id);
        mix(static_cast<uint64_t>(id));
        mix(static_cast<uint64_t>(obj.parent));
        mix(static_cast<uint64_t>(obj.children.size()));
        mix(collapsedIds_.count(id) ? 1ull : 0ull);
        for (ObjectID child : obj.children)
            walk(child);
    };
    walk(scene.getRoot());
    mix(static_cast<uint64_t>(selectedId_));
    return hash;
}

std::string HierarchyPanel::treePrefix(const Entry& entry) const {
    if (entry.depth <= 0)
        return {};

    std::string prefix;
    for (size_t i = 0; i < entry.ancestorOpen.size(); ++i)
        prefix += entry.ancestorOpen[i] ? "|  " : "   ";
    prefix += entry.isLastSibling ? "`- " : "|- ";
    return prefix;
}

void HierarchyPanel::applyRowMetrics(Row& row) const {
    UIElement& labelEl = ui_->get(row.labelId);
    labelEl.transform.fontSize = kRowFontSize;

    auto* labelBtn = dynamic_cast<Button*>(labelEl.widget.get());
    if (!labelBtn)
        return;

    labelBtn->padding = {kLabelPadX, kLabelPadY};

    // Buttons already know how tall they should be for the current fontSize.
    // Flex row parents do not yet infer height from children, so lift that
    // measured height onto the row (and match the toggle to it).
    const glm::vec2 measured = labelBtn->measureContent(labelEl, {0.f, 0.f});
    const float rowH = measured.y;

    UIElement& rootEl = ui_->get(row.rootId);
    rootEl.style.height = Length::px(rowH);
    rootEl.transform.fontSize = kRowFontSize;

    UIElement& toggleEl = ui_->get(row.toggleId);
    toggleEl.transform.fontSize = kRowFontSize;
    toggleEl.style.height = Length::px(rowH);
    toggleEl.style.width = Length::px(rowH);
    toggleEl.transform.size = {rowH, rowH};

    labelEl.style.height = Length::automatic();
}

void HierarchyPanel::ensureRowCount(size_t count) {
    while (rows_.size() < count) {
        UIElementID root = ui_->createElement();
        ui_->reparent(root, contentId_);
        UIElement& rootEl = ui_->get(root);
        rootEl.style.position = PositionMode::Relative;
        rootEl.style.display = Display::Flex;
        rootEl.style.flexDirection = FlexDirection::Row;
        rootEl.style.alignItems = AlignItems::Center;
        rootEl.style.width = Length::percent(100.f);
        rootEl.style.height = Length::automatic();

        UIElementID toggle = ui_->button({0.f, 0.f}, kRowFontSize, " ", nullptr);
        ui_->reparent(toggle, root);
        UIElement& toggleEl = ui_->get(toggle);
        toggleEl.style.position = PositionMode::Relative;
        toggleEl.style.width = Length::automatic();
        toggleEl.style.height = Length::automatic();
        toggleEl.style.flexGrow = 0.f;
        toggleEl.transform.fontSize = kRowFontSize;
        if (auto* btn = dynamic_cast<Button*>(toggleEl.widget.get())) {
            btn->padding = {0.f, 0.f};
            btn->cornerRadii = {0.f, 0.f, 0.f, 0.f};
            btn->centerText = true;
        }

        UIElementID label = ui_->button({0.f, 0.f}, kRowFontSize, "", nullptr);
        ui_->reparent(label, root);
        UIElement& labelEl = ui_->get(label);
        labelEl.style.position = PositionMode::Relative;
        labelEl.style.height = Length::automatic();
        labelEl.style.flexGrow = 1.f;
        labelEl.style.flexBasis = Length::px(0.f);
        labelEl.transform.fontSize = kRowFontSize;
        if (auto* btn = dynamic_cast<Button*>(labelEl.widget.get())) {
            btn->padding = {kLabelPadX, kLabelPadY};
            btn->cornerRadii = {0.f, 0.f, 0.f, 0.f};
            btn->centerText = false;
        }

        rows_.push_back({root, toggle, label, INVALID_OBJECT});
        applyRowMetrics(rows_.back());
    }
}

void HierarchyPanel::styleToggle(UIElementID id, bool hasChildren, bool expanded) const {
    auto* btn = dynamic_cast<Button*>(ui_->get(id).widget.get());
    if (!btn)
        return;

    btn->normal.bgColor = {0.f, 0.f, 0.f, 0.f};
    btn->hoveredStyle.bgColor = hasChildren
        ? glm::vec4{0.28f, 0.28f, 0.32f, 1.f}
        : glm::vec4{0.f, 0.f, 0.f, 0.f};
    btn->pressedStyle.bgColor = {0.22f, 0.22f, 0.26f, 1.f};
    btn->normal.textColor = {0.75f, 0.75f, 0.8f, 1.f};
    btn->hoveredStyle.textColor = {0.95f, 0.95f, 0.97f, 1.f};
    btn->text = hasChildren ? (expanded ? "v" : ">") : " ";
    btn->disabled = !hasChildren;
}

void HierarchyPanel::styleLabel(UIElementID id, bool selected) const {
    auto* btn = dynamic_cast<Button*>(ui_->get(id).widget.get());
    if (!btn)
        return;

    btn->centerText = false;
    if (selected) {
        btn->normal.bgColor = {0.22f, 0.38f, 0.62f, 1.f};
        btn->hoveredStyle.bgColor = {0.28f, 0.46f, 0.72f, 1.f};
        btn->pressedStyle.bgColor = {0.18f, 0.32f, 0.55f, 1.f};
    } else {
        btn->normal.bgColor = {0.f, 0.f, 0.f, 0.f};
        btn->hoveredStyle.bgColor = {0.22f, 0.22f, 0.26f, 1.f};
        btn->pressedStyle.bgColor = {0.18f, 0.18f, 0.22f, 1.f};
    }
    btn->normal.textColor = {0.88f, 0.88f, 0.9f, 1.f};
    btn->hoveredStyle.textColor = {0.95f, 0.95f, 0.97f, 1.f};
    btn->pressedStyle.textColor = {0.95f, 0.95f, 0.97f, 1.f};
}

void HierarchyPanel::syncObjectDebug(Scene& scene) const {
    std::function<void(ObjectID)> walk = [&](ObjectID id) {
        Object& obj = scene.get(id);
        obj.debug = (id == selectedId_);
        for (ObjectID child : obj.children)
            walk(child);
    };
    walk(scene.getRoot());
}

void HierarchyPanel::rebuildRows(Scene& scene) {
    std::vector<Entry> entries;
    collectEntries(scene, scene.getRoot(), 0, true, {}, entries);
    ensureRowCount(entries.size());

    for (size_t i = 0; i < rows_.size(); ++i) {
        UIElement& rootEl = ui_->get(rows_[i].rootId);
        if (i >= entries.size()) {
            rootEl.visible = false;
            rows_[i].objectId = INVALID_OBJECT;
            continue;
        }

        const Entry& entry = entries[i];
        rootEl.visible = true;
        rows_[i].objectId = entry.id;

        const Object& obj = scene.get(entry.id);
        const ObjectID capturedId = entry.id;

        styleToggle(rows_[i].toggleId, entry.hasChildren, entry.isExpanded);
        auto* toggle = dynamic_cast<Button*>(ui_->get(rows_[i].toggleId).widget.get());
        if (toggle) {
            if (entry.hasChildren) {
                toggle->onClick = [this, capturedId]() {
                    if (collapsedIds_.count(capturedId))
                        collapsedIds_.erase(capturedId);
                    else
                        collapsedIds_.insert(capturedId);
                    lastSignature_ = 0;
                };
            } else {
                toggle->onClick = nullptr;
            }
        }

        auto* label = dynamic_cast<Button*>(ui_->get(rows_[i].labelId).widget.get());
        if (label) {
            label->text = treePrefix(entry) + objectLabel(obj, scene.getRoot());
            if (entry.hasChildren)
                label->text += " (" + std::to_string(obj.children.size()) + ")";

            label->onClick = [this, capturedId]() {
                selectedId_ = capturedId;
                lastSignature_ = 0;
            };
        }
        styleLabel(rows_[i].labelId, entry.id == selectedId_);
        applyRowMetrics(rows_[i]);
    }
}

void HierarchyPanel::update(Scene& scene) {
    if (!built_ || !ui_ || contentId_ == INVALID_UI_ELEMENT)
        return;

    const uint64_t signature = sceneSignature(scene);
    if (signature == lastSignature_)
        return;

    lastSignature_ = signature;
    rebuildRows(scene);
    syncObjectDebug(scene);
}
