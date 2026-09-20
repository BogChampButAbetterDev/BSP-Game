#include <iostream>
#include <SDL3/SDL.h>

#include "game/game.hpp"

#undef main

int main()
{
    Game game;
    if (!game.init()) { return EXIT_FAILURE; }
    game.run();
    game.end();
    return EXIT_SUCCESS;
}
