#pragma once

#include <SDL2/SDL_scancode.h>
#include <array>
#include <cstdint>

constexpr std::size_t KEY_COUNT = SDL_NUM_SCANCODES;

using Key = SDL_Scancode;

enum class Action : uint16_t {
    MoveLeft,
    MoveRight,
    MoveForward,
    MoveBackward,
    ToggleScreen,
    Jump,
    Quit,

    Count
};

class Input {
public:
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;

    void addMouseDelta(float dx, float dy) {
        mouseDeltaX += dx;
        mouseDeltaY += dy;
    }

    void beginFrame() {
        previous = current;

        mouseDeltaX = 0.0f;
        mouseDeltaY = 0.0f;
    }

    void setKey(Key key, bool down) {
        current[key] = down;
    }

    bool down(Key key) const {
        return current[key];
    }

    bool pressed(Key key) const {
        return current[key] && !previous[key];
    }

    bool released(Key key) const {
        return !current[key] && previous[key];
    }

    void bind(Action action, Key key) {
        bindings[static_cast<std::size_t>(action)] = key;
    }

    bool down(Action action) const {
        return down(bindings[static_cast<std::size_t>(action)]);
    }

    bool pressed(Action action) const {
        return pressed(bindings[static_cast<std::size_t>(action)]);
    }

    bool released(Action action) const {
        return released(bindings[static_cast<std::size_t>(action)]);
    }

private:
    std::array<uint8_t, KEY_COUNT> current{};
    std::array<uint8_t, KEY_COUNT> previous{};

    std::array<Key, static_cast<std::size_t>(Action::Count)> bindings{};
};
