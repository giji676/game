#include "input.h"

#include <SDL2/SDL.h>

#include "engine.h"


glm::vec2 Input::mousePosition() const {
    return {
        x,
        ENGINE().app.height() - y
    };
}

bool Input::shiftDown() const {
    SDL_PumpEvents();
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    if (!keys)
        return down(SDL_SCANCODE_LSHIFT) || down(SDL_SCANCODE_RSHIFT);
    return keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
}

