#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <glad/glad.h>

#include "engine/engine.h"
#include "engine/input.h"
#include "engine/profilers/profile_scope.h"
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
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    game = g;
    game->init();
    renderer.init(&meshRegistry);
    uiRenderer.init();
    debugRenderer.init();
}

void Engine::run() {
    SDL_Event event;
    app.initDeltaTime();

    while (app.running) {
        Profiler::instance().beginFrame();
        {
            PROFILE_SCOPE("Engine::run");
            beginFrame();

            getInput(event);
            game->update();
            scene.update();
            ui.update();

            auto uiCommands = ui.buildRenderList();
            game->render();
            callRenderer(uiCommands);

            {
                GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
                PROFILE_SCOPE("CPU idle");
                while (glClientWaitSync(fence, 0, 0) == GL_TIMEOUT_EXPIRED) {
                }
                glDeleteSync(fence);
            }

            endFrame();
        }
        Profiler::instance().endFrame();
    }

    app.cleanup();
}

void Engine::callRenderer(
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

    glm::mat4 vp = projection * view;
    Frustum frustum = Frustum::fromMatrix(vp);

    renderCommands.clear();
    scene.buildRenderList(renderCommands, frustum);

    renderer.render(renderCommands, view, projection);
    uiRenderer.render(
            uiQue,
            orthoProjection,
            assets.getShader("glyph"),
            assets.getShader("rect"));
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

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    char title[128];
    snprintf(title, sizeof(title), "Game | FPS: %.1f", fps);
    SDL_SetWindowTitle(app.window, title);
}

void Engine::endFrame() {
    PROFILE_SCOPE("Engine::endFrame");
    SDL_GL_SwapWindow(app.window);
}

void Engine::getInput(SDL_Event &event) {
    input.beginFrame();

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                app.running = false;
                break;

            case SDL_KEYDOWN:
                input.setKey(event.key.keysym.scancode, true);
                break;

            case SDL_KEYUP:
                input.setKey(event.key.keysym.scancode, false);
                break;

            case SDL_MOUSEMOTION:
                input.setMousePosition(
                    event.motion.x,
                    event.motion.y
                );

                input.addMouseDelta(
                    event.motion.xrel,
                    event.motion.yrel
                );
                break;

            case SDL_MOUSEBUTTONDOWN:
                input.setMouseButton(event.button.button, true);
                break;

            case SDL_MOUSEBUTTONUP:
                input.setMouseButton(event.button.button, false);
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

    input.bind(MouseAction::Left, SDL_BUTTON_LEFT);
    input.bind(MouseAction::Middle, SDL_BUTTON_MIDDLE);
    input.bind(MouseAction::Right, SDL_BUTTON_RIGHT);
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

    assets.loadShader(
        "rect",
        "shaders/rect.v.glsl",
        "shaders/rect.f.glsl"
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
