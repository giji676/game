#pragma once

#include <vector>

#include "engine/engine.h"
#include "game/player.h"

#include "engine/camera.h"
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
    void initScripts(ObjectID id);
    void updateScripts(ObjectID id);
    void recurseRender(
        const ObjectID objId,
        const glm::mat4& parentMatrix);

private:
    Engine& engine;
    Player player;
    Camera camera;
    World world;
    Light light;

    unsigned int planeVBO, planeVAO, planeEBO;

    void setupTerrain();
};
