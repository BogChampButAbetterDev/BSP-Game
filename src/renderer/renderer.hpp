#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <SDL3/SDL.h>

#include "framebuffer.hpp"
#include "rasterizer.hpp"

#include "bsp/bsp_traverse.hpp"
#include "engine/mapLoader.hpp"
#include "debug_view.hpp"
#include "globals/color.hpp"

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
    void renderMapDBGCLR(const Map& map);
    void render2DView(const Map& map);
    void endRender();

    Framebuffer* getBuf() { return &m_buf; }

private:
    SDL_Renderer* m_ren = nullptr;
    Framebuffer m_buf;

    std::unique_ptr<std::vector<Wall>> m_renOrder = std::make_unique<std::vector<Wall>>();
};
