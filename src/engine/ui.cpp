#include "ui.h"

UIElementID UI::createElement() {
    UIElementID id = createElementInternal();
    elements[id].setID(id);
    elements[id].parent = rootId;
    elements[rootId].children.push_back(id);
    return id;
}

UIElementID UI::createElementInternal() {
    UIElementID id = static_cast<UIElementID>(elements.size());
    elements.emplace_back();
    return id;
}
