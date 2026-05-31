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
#include <vector>
#include "model.h"
#include "window.h"
#include "camera.h"
#include "perlin.h"
#include "input.h"

#define CAMERA_SPEED 1.0

void setupKeyBindings(GJ_Input &input);
void getInput(SDL_Event &event, bool &running, GJ_Input &input);
void update(GJ_Input &input, GJ_App &app);

GJ_App app;
GJ_Camera camera;
GJ_Input input;

typedef struct {
    std::vector<float> vertices;   // x y z nx ny nz
    std::vector<unsigned int> indices;
} Terrain;

typedef struct {
    Terrain terrain;
    int width;
    int height;
    float scale;
} World;

typedef struct {
    glm::vec3 pos;
} Light;

World world;
Light light;

const float G = -9.81;

Terrain generateTerrain(int width, int height, float scale, float heightScale) {
    Terrain t;

    float halfW = (width - 1) * scale * 0.5f;
    float halfH = (height - 1) * scale * 0.5f;

    // store heights temporarily for normal calculation
    std::vector<float> heights(width * height);

    auto idx = [&](int x, int z) {
        return z * width + x;
    };

    // 1) generate heightmap first
    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {

            float wx = x * scale - halfW;
            float wz = z * scale - halfH;

            heights[idx(x, z)] = noise2D(wx, wz) * heightScale;
        }
    }

    // clamp helper
    auto h = [&](int x, int z) {
        x = std::max(0, std::min(x, width - 1));
        z = std::max(0, std::min(z, height - 1));
        return heights[idx(x, z)];
    };

    // 2) build vertices + normals
    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {

            float wx = x * scale - halfW;
            float wz = z * scale - halfH;
            float y  = heights[idx(x, z)];

            // central difference normal
            float hl = h(x - 1, z);
            float hr = h(x + 1, z);
            float hd = h(x, z - 1);
            float hu = h(x, z + 1);

            glm::vec3 n;
            n.x = hl - hr;
            n.y = 2.0f;
            n.z = hd - hu;
            n = glm::normalize(n);

            t.vertices.push_back(wx);
            t.vertices.push_back(y);
            t.vertices.push_back(wz);

            t.vertices.push_back(n.x);
            t.vertices.push_back(n.y);
            t.vertices.push_back(n.z);
        }
    }

    // 3) indices unchanged
    for (int z = 0; z < height - 1; z++) {
        for (int x = 0; x < width - 1; x++) {

            int i0 = z * width + x;
            int i1 = i0 + 1;
            int i2 = (z + 1) * width + x;
            int i3 = i2 + 1;

            t.indices.push_back(i0);
            t.indices.push_back(i2);
            t.indices.push_back(i1);

            t.indices.push_back(i1);
            t.indices.push_back(i2);
            t.indices.push_back(i3);
        }
    }

    return t;
}

int main(int argc, char* argv[]) {
    app = GJ_App();
    app.initialize();

    setupKeyBindings(input);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    Shader shader = Shader("shaders/scene.v.glsl", "shaders/scene.f.glsl");

    world.width = 500;
    world.height = 500;
    world.scale = 0.05f;
    world.terrain = generateTerrain(world.width, world.height, world.scale, 0.4f);

    unsigned int planeVBO, planeVAO, planeEBO;

    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glGenBuffers(1, &planeEBO);

    glBindVertexArray(planeVAO);

    // vertices
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 world.terrain.vertices.size() * sizeof(float),
                 world.terrain.vertices.data(),
                 GL_DYNAMIC_DRAW);

    // indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 world.terrain.indices.size() * sizeof(unsigned int),
                 world.terrain.indices.data(),
                 GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    light.pos = glm::vec3(0.f, 3.f, 0.f);

    shader.use();
    shader.setVec3("lightPos", light.pos);
    shader.setVec3("lightColor", glm::vec3(1.0f));

    SDL_Event event;
    float fpsTimer = 0.0f;
    int fpsFrames = 0;
    float fps = 0.0f;

    app.initDeltaTime();
    while (app.running) {
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

        getInput(event, app.running, input);
        update(input, app);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(camera.pos, camera.pos + camera.front, camera.up);
        glm::mat4 projection = glm::perspective(
            glm::radians(45.f),
            (float)app.width() / (float)app.height(), 0.1f, 100.0f);

        shader.use();
        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        glBindVertexArray(planeVAO);
        glDrawElements(GL_TRIANGLES,
                       world.terrain.indices.size(),
                       GL_UNSIGNED_INT,
                       0);
        glBindVertexArray(0);

        SDL_GL_SwapWindow(app.window);
    }

    SDL_ShowCursor(1);
    SDL_GL_DeleteContext(app.glContext);
    SDL_DestroyWindow(app.window);
    SDL_Quit();

    return 0;
}

void getInput(SDL_Event &event, bool &running, GJ_Input &input) {
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

void update(GJ_Input &input, GJ_App &app) {
    float cameraSpeed = CAMERA_SPEED * app.deltaTime;

    // Input processing
    if (input.down(Action::MoveForward))
        camera.pos += cameraSpeed * camera.front;

    if (input.down(Action::MoveBackward))
        camera.pos -= cameraSpeed * camera.front;

    if (input.down(Action::MoveLeft))
        camera.pos -=
            glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;

    if (input.down(Action::MoveRight))
        camera.pos +=
            glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;

    if (input.pressed(Action::ToggleScreen))
        app.toggleWindow();

    if (input.pressed(Action::Quit))
        app.running = false;

    // Physics and collision
    camera.velocity.y += G * app.deltaTime;
    camera.pos.y += camera.velocity.y * app.deltaTime;

    float halfW = (world.width - 1) * world.scale * 0.5f;
    float halfH = (world.height - 1) * world.scale * 0.5f;

    int x = floor((camera.pos.x + halfW) / world.scale);
    int z = floor((camera.pos.z + halfH) / world.scale);

    int idx = z * world.width + x;
    float groundY = world.terrain.vertices[idx * 6 + 1];
    if (camera.pos.y <= groundY) {
        camera.velocity.y = 0.f;
        camera.pos.y = groundY;
    }

    float xoffset = input.mouseDeltaX;
    float yoffset = input.mouseDeltaY;

    const float sensitivity = 0.1f;

    camera.yaw += xoffset * sensitivity;
    camera.pitch -= yoffset * sensitivity;

    camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);

    glm::vec3 direction;
    direction.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    direction.y = sin(glm::radians(camera.pitch));
    direction.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));

    camera.front = glm::normalize(direction);
}

void setupKeyBindings(GJ_Input &input) {
    input.bind(Action::MoveForward, SDL_SCANCODE_W);
    input.bind(Action::MoveBackward, SDL_SCANCODE_S);
    input.bind(Action::MoveLeft, SDL_SCANCODE_A);
    input.bind(Action::MoveRight, SDL_SCANCODE_D);
    input.bind(Action::ToggleScreen, SDL_SCANCODE_F11);
    input.bind(Action::Quit, SDL_SCANCODE_ESCAPE);
}
