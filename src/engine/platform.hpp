#pragma once

#include <SDL3/SDL.h>

namespace Platform
{
    bool init();

    SDL_Window* create_window(int width, int height);
    SDL_Renderer* create_renderer(SDL_Window* window);

    void destroy_renderer(SDL_Renderer* renderer);
    void destroy_window(SDL_Window* window);

    void shutdown();
}
