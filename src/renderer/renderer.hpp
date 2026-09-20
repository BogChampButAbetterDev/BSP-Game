#pragma once
#include <SDL3/SDL.h>

#include "framebuffer.hpp"
#include "rasterizer.hpp"

#include "engine/mapLoader.hpp"

class Renderer
{
public:
    Renderer() {}
    Renderer(SDL_Renderer* ren) : m_ren(ren)
    {
        m_buf.create(m_ren);
    }

    void beginRender();
    void renderMap(const Map& map);
    void endRender();

    Framebuffer* getBuf() { return &m_buf; }

private:
    SDL_Renderer* m_ren = nullptr;
    Framebuffer m_buf;
};
