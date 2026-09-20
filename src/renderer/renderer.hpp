#pragma once
#include <SDL3/SDL.h>

#include "framebuffer.hpp"
#include "rasterizer.hpp"

class Renderer
{
public:
    Renderer() {}
    Renderer(SDL_Renderer* ren) : m_ren(ren)
    {
        m_buf.create(m_ren);
    }

    void beginRender();
    void renderLoop();
    void endRender();

    Framebuffer* getBuf() { return &m_buf; }

private:
    SDL_Renderer* m_ren = nullptr;
    Framebuffer m_buf;
};
