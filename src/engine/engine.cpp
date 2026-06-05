#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <glad/glad.h>

#include "engine/engine.h"
#include "engine/renderer/renderer.h"
#include "game/game.h"
#include "gj_image/gj_image.h"

Engine& Engine::instance() {
    static Engine instance;
    return instance;
}

void Engine::init(Game *g) {
    app = App();
    app.initialize();

    setupKeyBindings();
    loadAssets();
    setupCamera();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    game = g;
    game->init();
    debugRenderer.init();
}

void Engine::run() {
    SDL_Event event;
    app.initDeltaTime();

    while (app.running) {
        beginFrame();

        getInput(event);
        game->update();
        scene.update();
        auto commands = scene.buildRenderList();
        game->render();
        callRenderer(commands);

        endFrame();
    }

    SDL_ShowCursor(1);
    SDL_GL_DeleteContext(app.glContext);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
}

void Engine::callRenderer(std::vector<RenderCommand>& que) {
    Camera& camera = *getActiveCamera();

    glm::mat4 view = glm::lookAt(
        camera.pos,
        camera.pos + camera.front,
        camera.up
    );

    glm::mat4 projection = glm::perspective(
        glm::radians(45.f),
        app.width() / (float)app.height(),
        0.1f,
        100.0f
    );

    renderer.render(que, view, projection);
    debugRenderer.render(view, projection);
}

void Engine::beginFrame() {
    app.getDeltaTime();
    fpsTimer += app.deltaTime;
    fpsFrames++;
    if (fpsTimer >= 0.5f) {
        fps = fpsFrames / fpsTimer;
        fpsFrames = 0;
        fpsTimer = 0.0f;
    }

    char title[128];
    snprintf(title, sizeof(title), "Game | FPS: %.1f", fps);
    SDL_SetWindowTitle(app.window, title);
}

void Engine::endFrame() {
    SDL_GL_SwapWindow(app.window);
}

void Engine::getInput(SDL_Event &event) {
    input.beginFrame();

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
                input.setKey(event.key.keysym.scancode, true);
                break;

            case SDL_KEYUP:
                input.setKey(event.key.keysym.scancode, false);
                break;

            case SDL_MOUSEMOTION:
                input.addMouseDelta(
                    event.motion.xrel,
                    event.motion.yrel
                );
                break;
        }
    }
}

void Engine::setupKeyBindings() {
    input.bind(Action::MoveForward, SDL_SCANCODE_W);
    input.bind(Action::MoveBackward, SDL_SCANCODE_S);
    input.bind(Action::MoveLeft, SDL_SCANCODE_A);
    input.bind(Action::MoveRight, SDL_SCANCODE_D);
    input.bind(Action::Jump, SDL_SCANCODE_SPACE);
    input.bind(Action::ToggleScreen, SDL_SCANCODE_F11);
    input.bind(Action::Quit, SDL_SCANCODE_ESCAPE);
}

void Engine::loadAssets() {
    assets.loadShader(
        "scene",
        "shaders/scene.v.glsl",
        "shaders/scene.f.glsl"
    );

    assets.loadShader(
        "textured_mat",
        "shaders/textured_mat.v.glsl",
        "shaders/textured_mat.f.glsl"
    );

    assets.loadShader(
        "debug",
        "shaders/debug_shader.v.glsl",
        "shaders/debug_shader.f.glsl"
    );


    gj_vflip_image(1);
    assets.loadModel("backpack", "assets/backpack/backpack.obj");
}

void Engine::setupCamera() {
    cameras.emplace_back(Camera());
    activeCamera = 0;
}
