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
    void render(const glm::mat4& view, const glm::mat4& projection);
    void recurseRender(
        const ObjectID objId,
        const glm::mat4& parentMatrix);

private:
    Engine& engine;
    Player player;
    World world;
    Light light;

    UIElementID fpsLabelId = INVALID_UI_ELEMENT;
    UIElementID pausePanelId = INVALID_UI_ELEMENT;
    UIElementID pauseButtonId = INVALID_UI_ELEMENT;
    UIElementID pauseInputId = INVALID_UI_ELEMENT;
    UIElementID blockLabelId = INVALID_UI_ELEMENT;
    UIElementID toolbarId = INVALID_UI_ELEMENT;
    UIElementID flexRowDemoId = INVALID_UI_ELEMENT;
    UIElementID flexColDemoId = INVALID_UI_ELEMENT;
    UIElementID absoluteDemoId = INVALID_UI_ELEMENT;
    UIElementID flexGrowDemoId = INVALID_UI_ELEMENT;
    UIElementID flexGrowRatioDemoId = INVALID_UI_ELEMENT;

    bool playToggled = false;

    unsigned int planeVBO, planeVAO, planeEBO;

    void setupTerrain();
};
