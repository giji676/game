#include "engine/engine.h"
#include "game/game.h"

int main(int argc, char* argv[]) {
    Game game(ENGINE());
    ENGINE().init(&game);
    ENGINE().run();
}


