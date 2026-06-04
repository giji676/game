#pragma once

#include <SDL2/SDL_events.h>

#include "camera.h"
#include "scene.h"
#include "input.h"
#include "window.h"
#include "asset_manager/asset_manager.h"
#include "renderer/renderer.h"

class Game;

class Engine {
public:
    static Engine& instance();

    App app;
    Input input;
    AssetManager assets;
    Renderer renderer;
    Scene scene;

    unsigned int activeCamera = 0;
    std::vector<Camera> cameras;

    float G = 9.81;
    float fps = 0.0f;

    void init(Game *g);
    void run();

    Camera* getActiveCamera() { return &cameras[activeCamera]; }

    Camera* getCamera(unsigned int camera) { return &cameras[camera]; }

private:
    Game *game;

    bool running = true;
    float fpsTimer = 0.0f;
    int fpsFrames = 0;

    void getInput(SDL_Event &event);
    void setupKeyBindings();
    void loadAssets();
    void setupCamera();

    void beginFrame();
    void endFrame();

    Engine() = default;
    ~Engine() = default;

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
};
