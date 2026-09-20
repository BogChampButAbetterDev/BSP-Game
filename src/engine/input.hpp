#pragma once
#include <SDL3/SDL.h>

struct Input
{
    bool w = false;
    bool a = false;
    bool s = false;
    bool d = false;
    bool arL = false;
    bool arR = false;
};

inline void pollInput(SDL_Event e, Input* in)
{
    if (e.type == SDL_EVENT_KEY_DOWN)
    {
        if (e.key.scancode == SDL_SCANCODE_W)
        {
            in->w = true;
        }
        if (e.key.scancode == SDL_SCANCODE_A)
        {
            in->a = true;
        }
        if (e.key.scancode == SDL_SCANCODE_S)
        {
            in->s = true;
        }
        if (e.key.scancode == SDL_SCANCODE_D)
        {
            in->d = true;
        }
        if (e.key.scancode == SDL_SCANCODE_LEFT)
        {
            in->arL = true;
        }
        if (e.key.scancode == SDL_SCANCODE_RIGHT)
        {
            in->arR = true;
        }
    }
    if (e.type == SDL_EVENT_KEY_UP)
    {
        if (e.key.scancode == SDL_SCANCODE_W)
        {
            in->w = false;
        }
        if (e.key.scancode == SDL_SCANCODE_A)
        {
            in->a = false;
        }
        if (e.key.scancode == SDL_SCANCODE_S)
        {
            in->s = false;
        }
        if (e.key.scancode == SDL_SCANCODE_D)
        {
            in->d = false;
        }
        if (e.key.scancode == SDL_SCANCODE_LEFT)
        {
            in->arL = false;
        }
        if (e.key.scancode == SDL_SCANCODE_RIGHT)
        {
            in->arR = false;
        }
    }
}
