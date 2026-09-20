#pragma once
#include <iostream>
#include <SDL3/SDL.h>
#include <vector>

#include "engine/camera.hpp"

#include "globals/globals.hpp"

struct Framebuffer
{   
    SDL_Texture* tex = nullptr;
    std::vector<uint32_t> pixels;

    Camera* activeCamera = nullptr;

    uint32_t current_color = 0x000000FF;

    Framebuffer() {}

    void create(SDL_Renderer* ren)
    {
        tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, DEF_WIN_WIDTH, DEF_WIN_HEIGHT);
        pixels.resize(DEF_WIN_WIDTH * DEF_WIN_HEIGHT);
    }

    void setActiveCamera(Camera* cam)
    {
        activeCamera = cam;
    }

    void clear(uint32_t color)
    {
        std::fill(pixels.begin(), pixels.end(), color);
    }

    void setDrawColor(uint32_t color)
    {
        current_color = color;
    }

    void setPixel(int x, int y)
    {
        if (x < 0 || x >= DEF_WIN_WIDTH || y < 0 || y >= DEF_WIN_HEIGHT)
        {
            return;
        }

        pixels[y * DEF_WIN_WIDTH + x] = current_color;
    }

    void present(SDL_Renderer* ren)
    {
        SDL_UpdateTexture(tex, nullptr, pixels.data(), DEF_WIN_WIDTH * sizeof(uint32_t));
        SDL_RenderTexture(ren, tex, nullptr, nullptr);
    }
};
