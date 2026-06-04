#include <glad/glad.h>

#include "game/game.h"
#include "game/perlin.h"
#include "game/scripts/test.h"

#include "engine/engine.h"

Terrain generateTerrain(int width, int height, float scale, float heightScale);

glm::mat4 transformToMatrix(const Transform& t);

Game::Game(Engine& engine)
    : engine(engine)
{}

void Game::init() {
    Scene& scene = engine.scene;
    setupTerrain();

    player.pos = glm::vec3(0.f);
    player.velocity = glm::vec3(0.f);

    camera.pos = glm::vec3(0.0f, 0.0f, 3.0f);
    camera.front = glm::vec3(0.0f, 0.0f, -1.0f);
    camera.up = glm::vec3(0.0f, 1.0f, 0.0f);

    light.pos = glm::vec3(0.f, 3.f, 0.f);
    light.color = glm::vec3(1.f);

    Shader& sceneShader = engine.assets.getShader("scene");

    sceneShader.use();
    sceneShader.setVec3("lightPos", light.pos);
    sceneShader.setVec3("lightColor", light.color);

    ObjectID objId = scene.createObject();
    Object& obj = scene.get(objId);
    obj.model = &engine.assets.getModel("backpack");
    obj.transform.position = glm::vec3(0.0f, 0.5f, -1.5f);
    obj.transform.scale = glm::vec3(0.1f);
    obj.addScript<Test>();

    ObjectID objId2 = scene.createObject();
    Object& obj2 = scene.get(objId2);
    obj2.model = &engine.assets.getModel("backpack");
    obj2.transform.position = glm::vec3(2.0f, 0.0f, 0.0f);
    obj2.transform.scale = glm::vec3(0.2f);
    scene.reparent(objId2, objId);

    Shader& texturedMatShader = engine.assets.getShader("textured_mat");
    texturedMatShader.use();
    texturedMatShader.setVec3("lightPos", light.pos);
    texturedMatShader.setVec3("lightColor", light.color);

    initScripts(scene.getRoot());
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
    float cameraSpeed = player.speed * engine.app.deltaTime;
    Input &input = engine.input;

    // Input processing
    if (input.down(Action::MoveForward))
        player.pos += cameraSpeed * camera.front;

    if (input.down(Action::MoveBackward))
        player.pos -= cameraSpeed * camera.front;

    if (input.down(Action::MoveLeft))
        player.pos -=
            glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;

    if (input.down(Action::MoveRight))
        player.pos +=
            glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;

    if (input.pressed(Action::Jump) && player.grounded)
        player.velocity.y = player.jumpForce;

    if (input.pressed(Action::ToggleScreen))
        engine.app.toggleWindow();

    if (input.pressed(Action::Quit))
        engine.app.running = false;

    // Physics and collision
    player.velocity.y += -engine.G * engine.app.deltaTime;
    player.pos.y += player.velocity.y * engine.app.deltaTime;

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
}

void Game::render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    glm::mat4 identity(1.0f);

    updateScripts(engine.scene.getRoot());
    recurseRender(engine.scene.getRoot(), identity);
    engine.renderer.render(view, projection);
}

void Game::recurseRender(
    const ObjectID objId,
    const glm::mat4& parentMatrix)
{
    Object& obj = engine.scene.get(objId);

    glm::mat4 localMatrix =
        transformToMatrix(obj.transform);

    glm::mat4 worldMatrix =
        parentMatrix * localMatrix;

    engine.renderer.submit(obj, worldMatrix);

    for (const auto& child : obj.children) {
        recurseRender(child, worldMatrix);
    }
}

void Game::initScripts(ObjectID id) {
    Object& obj = engine.scene.get(id);

    for (auto& script : obj.scripts) {
        script->init();
    }

    for (ObjectID child : obj.children) {
        initScripts(child);
    }
}

void Game::updateScripts(ObjectID id) {
    Object& obj = engine.scene.get(id);

    for (auto& script : obj.scripts) {
        script->update();
    }

    for (ObjectID child : obj.children) {
        updateScripts(child);
    }
}

glm::mat4 transformToMatrix(const Transform& t) {
    glm::mat4 m(1.0f);

    m = glm::translate(m, t.position);

    m = glm::rotate(m, glm::radians(t.rotation.x),
                    glm::vec3(1,0,0));
    m = glm::rotate(m, glm::radians(t.rotation.y),
                    glm::vec3(0,1,0));
    m = glm::rotate(m, glm::radians(t.rotation.z),
                    glm::vec3(0,0,1));

    m = glm::scale(m, t.scale);

    return m;
}
