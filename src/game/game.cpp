#include <glad/glad.h>
#include <string>

#include "game/game.h"
#include "engine/defines.h"
#include "engine/profilers/profile_scope.h"
#include "engine/raycasting.h"
#include "game/perlin.h"
#include "game/scripts/test.h"

#include "engine/engine.h"
#include "engine/asset_manager/widgets.h"

Terrain generateTerrain(int width, int height, float scale, float heightScale);

void clearDebug(ObjectID id) {
    Object& obj = Engine::instance().scene.get(id);

    obj.debug = false;

    for (ObjectID child : obj.children)
        clearDebug(child);
}

Game::Game(Engine& engine)
    : engine(engine)
{}

void Game::init() {
    Scene& scene = engine.scene;
    UI& ui = engine.ui;
    setupTerrain();

    player.pos = glm::vec3(0.f);
    player.velocity = glm::vec3(0.f);

    light.pos = glm::vec3(0.f, 3.f, 0.f);
    light.color = glm::vec3(1.f);

    Shader& sceneShader = engine.assets.getShader("scene");

    sceneShader.use();
    sceneShader.setVec3("lightPos", light.pos);
    sceneShader.setVec3("lightColor", light.color);

    int width = 10;

    for (int i = 0; i < 1000; i++) {
        ObjectID objId = scene.createObject();
        Object& obj = scene.get(objId);
        obj.model = &engine.assets.getModel("backpack");
        obj.transform.setPosition({i % width, 0.5f, -i/10.f});
        obj.transform.setScale({0.1f, 0.1f, 0.1f});
        obj.addScript<Test>();
    }

    ObjectID objId = scene.createObject();
    Object& obj = scene.get(objId);
    obj.model = &engine.assets.getModel("backpack");
    obj.transform.setPosition({0.0f, 0.5f, -1.5f});
    obj.transform.setScale({0.1f, 0.1f, 0.1f});
    obj.addScript<Test>();

    ObjectID objId2 = scene.createObject();
    Object& obj2 = scene.get(objId2);
    obj2.model = &engine.assets.getModel("backpack");
    obj2.transform.setPosition({2.0f, 0.0f, 0.0f});
    obj2.transform.setScale({0.2f, 0.2f, 0.2f});
    scene.reparent(objId2, objId);

    Shader& texturedMatShader = engine.assets.getShader("textured_mat");
    texturedMatShader.use();
    texturedMatShader.setVec3("lightPos", light.pos);
    texturedMatShader.setVec3("lightColor", light.color);

    ui.label(
            {50.f, engine.app.height() - 60.f},
            {0.f, 48.f},
            {1.f, 0.f, 0.f, 1.f},
            "hello");

    ui.rect(
            {engine.app.width()/2-5, engine.app.height()/2-5},
            {10.f, 10.f},
            {1.f, 1.f, 1.f, 1.f},
            {5.f, 5.f, 5.f, 5.f});

    auto btnId = ui.button(
            {10.f, 100.f},
            64.f,
            "abcdefghijklmnopqABCDEFGHIJKLMNOPQ",
            nullptr);

    auto& elem = ui.get(btnId);
    auto* btn = dynamic_cast<Button*>(elem.widget.get());
    btn->normal.bgColor = {1, 1, 1, 1};
    btn->hoveredStyle.bgColor = {0, 1, 0, 0.2};
    btn->normal.borderColor = {1, 0, 0, 1};
    btn->normal.borderWidth = 10.f;
    btn->cornerRadii = {10.f, 10.f, 10.f, 10.f};

    btn->onClick = []() {
        std::cout << "Button clicked\n";
    };
}

void Game::setupTerrain() {
    world.width = 500;
    world.height = 500;
    world.scale = 0.05f;
    world.terrain = generateTerrain(world.width, world.height, world.scale, 0.4f);

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
}

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

void Game::update() {
    PROFILE_SCOPE("Game::update");

    Input& input = engine.input;

    // -------------------------
    // 1. ALWAYS RUN (control input)
    // -------------------------
    if (input.pressed(Action::Quit)) {
        is_paused = !is_paused;
        if (is_paused)
            Engine::instance().app.getCursor();
        else
            Engine::instance().app.releaseCursor();
    }

    if (input.pressed(Action::ToggleScreen))
        engine.app.toggleWindow();

    // -------------------------
    // 2. SIMULATION (paused gate)
    // -------------------------
    if (!is_paused)
    {
        Camera& camera = *engine.getActiveCamera();
        float dt = engine.app.deltaTime;
        float cameraSpeed = player.speed * dt;

        if (input.down(Action::MoveForward))
            player.pos += cameraSpeed * camera.front;

        if (input.down(Action::MoveBackward))
            player.pos -= cameraSpeed * camera.front;

        if (input.down(Action::MoveLeft))
            player.pos -= glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;

        if (input.down(Action::MoveRight))
            player.pos += glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;

        if (input.pressed(Action::Jump) && player.grounded)
            player.velocity.y = player.jumpForce;

        player.velocity.y += -engine.G * dt;
        player.pos.y += player.velocity.y * dt;

        // collision
        float halfW = (world.width - 1) * world.scale * 0.5f;
        float halfH = (world.height - 1) * world.scale * 0.5f;

        int x = floor((camera.pos.x + halfW) / world.scale);
        int z = floor((camera.pos.z + halfH) / world.scale);

        int idx = z * world.width + x;
        float groundY = world.terrain.vertices[idx * 6 + 1];

        if (player.pos.y <= groundY) {
            player.velocity.y = 0.f;
            player.pos.y = groundY;
            player.grounded = true;
        } else {
            player.grounded = false;
        }

        // camera
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
        camera.pos = player.pos;

        clearDebug(engine.scene.getRoot());

        Ray ray{
            .origin = camera.pos,
            .direction = glm::normalize(camera.front),
        };

        RaycastHit hit = engine.raycasting.castRay(ray);
        if (hit.object != INVALID_OBJECT_ID) {
            engine.scene.get(hit.object).debug = true;
        }
    }

    // -------------------------
    // 3. UI (always runs)
    // -------------------------
    UIElement& e = engine.ui.get(1);
    if (auto* lbl = dynamic_cast<Label*>(e.widget.get())) {
        lbl->text = "FPS: " + std::to_string(engine.fps);
    }
}

void Game::render() {
    PROFILE_SCOPE("Game::render");
    Camera& camera = *engine.getActiveCamera();

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(camera.pos, camera.pos + camera.front, camera.up);
    glm::mat4 projection = glm::perspective(
        glm::radians(45.f),
        (float)engine.app.width() / (float)engine.app.height(),
        0.1f, 100.0f);

    Shader& sceneShader = engine.assets.getShader("scene");
    sceneShader.use();
    sceneShader.setMat4("model", model);
    sceneShader.setMat4("view", view);
    sceneShader.setMat4("projection", projection);

    glBindVertexArray(planeVAO);
    glDrawElements(GL_TRIANGLES,
                   world.terrain.indices.size(),
                   GL_UNSIGNED_INT,
                   0);
    glBindVertexArray(0);
}
