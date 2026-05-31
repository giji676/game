#include "engine/engine.h"
#include "game/game.h"

Engine engine;
Game game;

int main(int argc, char* argv[]) {
    engine.init(&game);
    engine.run();
}


