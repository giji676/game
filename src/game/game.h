#pragma once

#include <vector>

#include "game/player.h"

#include "engine/camera.h"
#include "engine/asset_manager/object.h"

class Engine;

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
    glm::vec3 color;
} Light;

class Game {
public:
    void init(Engine *engine);
    void update();
    void render();
    void recurseRender(
        const Object& obj,
        const glm::mat4& parentMatrix);

private:
    Engine *engine;
    Player player;
    Camera camera;
    World world;
    Light light;

    std::vector<Object> scene;

    unsigned int planeVBO, planeVAO, planeEBO;

    void setupTerrain();
};
