#pragma once

#include <SDL2/SDL_scancode.h>
#include <array>
#include <cstdint>
#include <glm/glm.hpp>

constexpr std::size_t KEY_COUNT = SDL_NUM_SCANCODES;

using Key = SDL_Scancode;
using MouseButton = uint8_t;

constexpr std::size_t MOUSE_BUTTON_COUNT = 8;

enum class Action : uint16_t {
    MoveLeft,
    MoveRight,
    MoveForward,
    MoveBackward,
    ToggleScreen,
    Jump,
    Pause,
    ToggleEditor,
    TogglePlay,
    GizmoMove,
    GizmoRotate,
    GizmoScale,

    Count
};

enum class MouseAction : uint8_t {
    Left,
    Right,
    Middle,
    Count
};

class Input {
public:
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    float wheelY = 0.0f;
    float x = 0.0f;
    float y = 0.0f;

    glm::vec2 mousePosition() const;

    float mouseWheelY() const { return wheelY; }

    void setMousePosition(float _x, float _y) {
        x = _x;
        y = _y;
    }

    void addMouseDelta(float dx, float dy) {
        mouseDeltaX += dx;
        mouseDeltaY += dy;
    }

    void beginFrame() {
        previous = current;
        previousMouse = currentMouse;

        mouseDeltaX = 0.0f;
        mouseDeltaY = 0.0f;
        wheelY = 0.0f;
    }

    void addMouseWheel(float y) { wheelY += y; }

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

    void setMouseButton(MouseButton button, bool down) {
        currentMouse[button] = down;
    }

    bool mouseDown(MouseButton button) const {
        return currentMouse[button];
    }

    bool mousePressed(MouseButton button) const {
        return currentMouse[button] && !previousMouse[button];
    }

    bool mouseReleased(MouseButton button) const {
        return !currentMouse[button] && previousMouse[button];
    }

    void bind(MouseAction action, MouseButton button) {
        mouseBindings[static_cast<size_t>(action)] = button;
    }

    bool down(MouseAction action) const {
        return mouseDown(mouseBindings[static_cast<size_t>(action)]);
    }

    bool pressed(MouseAction action) const {
        return mousePressed(mouseBindings[static_cast<size_t>(action)]);
    }

    bool released(MouseAction action) const {
        return mouseReleased(mouseBindings[static_cast<size_t>(action)]);
    }

    // Live SDL state so modifier release is not delayed by buffered key events.
    bool shiftDown() const;

private:
    std::array<uint8_t, KEY_COUNT> current{};
    std::array<uint8_t, KEY_COUNT> previous{};

    std::array<uint8_t, MOUSE_BUTTON_COUNT> currentMouse{};
    std::array<uint8_t, MOUSE_BUTTON_COUNT> previousMouse{};

    std::array<Key, static_cast<std::size_t>(Action::Count)> bindings{};
    std::array<MouseButton, static_cast<std::size_t>(MouseAction::Count)> mouseBindings{};
};
