#pragma once

#include <SDL2/SDL_events.h>

#include "engine/input.h"
#include "engine/window.h"
#include "engine/asset_manager.h"

class Game;

class Engine {
public:
    App app;
    Input input;
    AssetManager assets;

    float G = 9.81;
    float fps = 0.0f;

    void init(Game *g);
    void run();

private:
    Game *game;

    bool running = true;
    float fpsTimer = 0.0f;
    int fpsFrames = 0;

    void getInput(SDL_Event &event);
    void setupKeyBindings();
    void beginFrame();
    void endFrame();
};
