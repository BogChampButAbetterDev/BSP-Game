#pragma once
#include <iostream>
#include "engine/platform.hpp"
#include "renderer/renderer.hpp"
#include "engine/camera.hpp"
#include "engine/input.hpp"

#include "globals/globals.hpp"

class Game
{
public:
    Game() {}

    bool init();
    void run();
    void end();

private:
    SDL_Window* m_win = nullptr;
    SDL_Renderer* m_sdlren = nullptr;

    Renderer m_ren;

    SDL_Event event;
    bool m_running = true;

    Uint64 lastTime = 0;
    float delta = 0.0f; 

    Camera m_cam;

    void pollEvents();
};
