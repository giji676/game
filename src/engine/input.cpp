#include "input.h"
#include "engine.h"


glm::vec2 Input::mousePosition() const {
    return {
        x,
        Engine::instance().app.height() - y
    };
}

