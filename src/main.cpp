#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_mouse.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "model.h"
#include "window.h"
#include "camera.h"

void processInput(SDL_Window *window, SDL_Event *event, bool *running);
void getDeltaTime(float *deltaTime, float *lastFrame);

GJ_App app;
GJ_Camera camera;

int main(int argc, char* argv[]) {
    app = GJ_App();
    app.initialize();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    SDL_Event event;
    bool running = true;
    float fpsTimer = 0.0f;
    int fpsFrames = 0;
    float fps = 0.0f;

    while (running) {
        app.getDeltaTime();
        fpsTimer += app.deltaTime;
        fpsFrames++;
        if (fpsTimer >= 0.5f) {
            fps = fpsFrames / fpsTimer;
            fpsFrames = 0;
            fpsTimer = 0.0f;
        }

        char title[128];
        snprintf(title, sizeof(title), "SDL2 + OpenGL - FPS: %.1f", fps);
        SDL_SetWindowTitle(app.window, title);

        processInput(app.window, &event, &running);
        glViewport(0, 0, app.width(), app.height());

        SDL_GL_SwapWindow(app.window);
    }

    SDL_ShowCursor(1);
    SDL_GL_DeleteContext(app.glContext);
    SDL_DestroyWindow(app.window);
    SDL_Quit();

    return 0;
}

void processInput(SDL_Window *window, SDL_Event *event, bool *running) {
    float cameraSpeed = 10.0f * app.deltaTime;
    while (SDL_PollEvent(event)) {
        if (event->type == SDL_QUIT) {
            *running = false;
        } else if (event->type == SDL_KEYDOWN) {
            switch (event->key.keysym.sym) {
                case SDLK_ESCAPE:
                    *running = false;
                    break;
                case SDLK_F11:
                    app.toggleWindow();
                    break;
                case SDLK_w:
                case SDLK_UP:
                    camera.pos += cameraSpeed * camera.front;
                    break;
                case SDLK_s:
                case SDLK_DOWN:
                    camera.pos -= cameraSpeed * camera.front;
                    break;
                case SDLK_d:
                case SDLK_RIGHT:
                    camera.pos +=
                        glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;
                    break;
                case SDLK_a:
                case SDLK_LEFT:
                    camera.pos -=
                        glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;
                    break;
            }
        } else if (event->type == SDL_MOUSEMOTION) {
            float xoffset = event->motion.xrel;
            float yoffset = event->motion.yrel;

            const float sensitivity = 0.1f;
            xoffset *= sensitivity;
            yoffset *= sensitivity;

            camera.yaw += xoffset;
            camera.pitch -= yoffset; // note: invert Y if needed

            // clamp pitch to avoid flipping
            if (camera.pitch > 89.0f)
                camera.pitch = 89.0f;
            if (camera.pitch < -89.0f)
                camera.pitch = -89.0f;

            glm::vec3 direction;
            direction.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            direction.y = sin(glm::radians(camera.pitch));
            direction.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));

            camera.front = glm::normalize(direction);
        }
    }
}
