#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_mouse.h>

#include "window.h"

int GJ_App::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,
                        videoSettings.antiAliasing == AntiAliasing::None ? 0 : 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, antiAliasing());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    window = SDL_CreateWindow(
        "Game",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        videoSettings.resolution.width,
        videoSettings.resolution.height,
        getWindowFlags()
    );

    if (!window) {
        return -1;
    }

    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        return -1;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        return -1;
    }

    return 0;
}

int GJ_App::width() const {
    if (!window) {
        return videoSettings.resolution.width;
    }
    int w, h;
    SDL_GL_GetDrawableSize(window, &w, &h);
    return w;
}

int GJ_App::height() const {
    if (!window) {
        return videoSettings.resolution.height;
    }

    int w, h;
    SDL_GL_GetDrawableSize(window, &w, &h);
    return h;
}

void GJ_App::toggleWindow() {
    switch (videoSettings.mode) {

        case WindowMode::Windowed:
            videoSettings.mode = WindowMode::BorderlessFullscreen;
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            break;

        case WindowMode::BorderlessFullscreen:
            videoSettings.mode = WindowMode::Windowed;
            SDL_SetWindowFullscreen(window, 0);
            SDL_SetWindowBordered(window, SDL_TRUE);
            SDL_SetWindowSize(window,
                              videoSettings.resolution.width,
                              videoSettings.resolution.height);
            break;

        case WindowMode::ExclusiveFullscreen:
            videoSettings.mode = WindowMode::Windowed;
            SDL_SetWindowFullscreen(window, 0);
            SDL_SetWindowBordered(window, SDL_TRUE);
            SDL_SetWindowSize(window,
                              videoSettings.resolution.width,
                              videoSettings.resolution.height);
            break;
    }

    updateViewport();

    SDL_PumpEvents();
    updateViewport();
}

void GJ_App::updateViewport() {
    int w = 0, h = 0;
    SDL_GL_GetDrawableSize(window, &w, &h);

    glViewport(0, 0, w, h);
}

Uint32 GJ_App::getWindowFlags() const {
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

    if (videoSettings.mode == WindowMode::BorderlessFullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    else if (videoSettings.mode == WindowMode::ExclusiveFullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    return flags;
}

float GJ_App::getDeltaTime() {
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();
    float currentTime = (float)now / (float)freq;
    deltaTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;
    return deltaTime;
}

void GJ_App::initDeltaTime() {
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();
    lastFrameTime = (float)now / (float)freq;
}
