#pragma once

#include <vector>

#include "engine/engine.h"
#include "game/player.h"

#include "engine/defines.h"

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
    Game(Engine& engine);

    void init();
    void update();
    void render();
    void recurseRender(
        const ObjectID objId,
        const glm::mat4& parentMatrix);

private:
    Engine& engine;
    Player player;
    World world;
    Light light;

    unsigned int planeVBO, planeVAO, planeEBO;

    void setupTerrain();
};
