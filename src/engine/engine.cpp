#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <glad/glad.h>

#include "engine/engine.h"
#include "engine/profilers/profiler.h"
#include "engine/renderer/ui_renderer.h"
#include "game/game.h"
#include "gj_image/gj_image.h"
#include "glm/ext/matrix_clip_space.hpp"

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
    renderer.init(&meshRegistry);
    debugRenderer.init();
}

void Engine::run() {
    SDL_Event event;
    app.initDeltaTime();

    while (app.running) {
        beginFrame();
        Profiler::instance().beginFrame();

        getInput(event);
        game->update();
        scene.update();
        ui.update();

        renderCommands.clear();
        scene.buildRenderList(renderCommands);

        auto uiCommands = ui.buildRenderList();
        game->render();
        callRenderer(renderCommands, uiCommands);

        Profiler::instance().endFrame();
        endFrame();
    }

    SDL_ShowCursor(1);
    SDL_GL_DeleteContext(app.glContext);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
}

void Engine::callRenderer(
        std::vector<RenderCommand>& que,
        std::vector<UIRenderCommand>& uiQue)
{
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

    glm::mat4 orthoProjection = glm::ortho(
            0.0f, static_cast<float>(app.width()),
            0.0f, static_cast<float>(app.height()));

    renderer.render(que, view, projection);
    uiRenderer.render(uiQue, orthoProjection, assets.getShader("glyph"));
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

    assets.loadShader(
        "glyph",
        "shaders/glyph.v.glsl",
        "shaders/glyph.f.glsl"
    );

    gj_vflip_image(1);
    assets.loadModel("backpack", "assets/backpack/backpack.obj");
    assets.loadFont("InterVariable", "assets/fonts/Inter-4.1/InterVariable.ttf");

    meshRegistry.init();

    Model& backpack = assets.getModel("backpack");
    for (const SubMesh& sub : backpack.getParts()) {
        meshRegistry.addMesh(&sub.mesh);
    }

    meshRegistry.uploadToGPU();
}

void Engine::setupCamera() {
    cameras.emplace_back(Camera());
    activeCamera = 0;
}
