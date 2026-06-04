#include "engine/engine.h"
#include "game/game.h"

int main(int argc, char* argv[]) {
    Game game(Engine::instance());
    Engine::instance().init(&game);
    Engine::instance().run();
}


