#include "platform.hpp"
#include <iostream>

namespace Platform
{
    bool init()
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            std::cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
            return false;
        }
        return true;
    }

    SDL_Window *create_window(int width, int height)
    {
        SDL_Window *window = SDL_CreateWindow("BSP Renderer", width, height, 0);
        if (!window)
        {
            std::cerr << "Window creation failed: " << SDL_GetError() << '\n';
        }

        SDL_SetWindowRelativeMouseMode(window, true);
        return window;
    }

    SDL_Renderer *create_renderer(SDL_Window *window)
    {
        SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer)
        {
            std::cerr << "Renderer creation failed: " << SDL_GetError() << '\n';
        }
        return renderer;
    }

    void destroy_renderer(SDL_Renderer *renderer)
    {
        if (renderer)
        {
            SDL_DestroyRenderer(renderer);
        }
    }

    void destroy_window(SDL_Window *window)
    {
        if (window)
        {
            SDL_DestroyWindow(window);
        }
    }
    
    void shutdown() { SDL_Quit(); }
}
