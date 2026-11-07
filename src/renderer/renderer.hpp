#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <cstring>
#include <SDL3/SDL.h>

#include "framebuffer.hpp"
#include "rasterizer.hpp"
#include "debug_view.hpp"

#include "bsp/bsp_traverse.hpp"
#include "engine/mapLoader.hpp"
#include "engine/lighting.hpp"
#include "engine/texture.hpp"

class Renderer
{
public:
    Renderer() {}
    Renderer(SDL_Renderer* ren) : m_ren(ren)
    {
        m_buf.create(m_ren);
    }

    void regLight(const DirectionalLight& light) { m_light = light; m_light.dir = m_light.dir.normalized();}
    void loadTextures(const std::vector<std::string>& paths);

    void beginRender();
    void renderMap(const Map& map);
    void renderMapDBGCLR(const Map& map);
    void render2DView(const Map& map);
    void endRender();

    Framebuffer* getBuf() { return &m_buf; }

private:
    SDL_Renderer* m_ren = nullptr;
    Framebuffer m_buf;

    std::vector<int> m_top, m_bottom; 
    int m_openCols = 0;

    std::unique_ptr<std::vector<Wall>> m_renOrder = std::make_unique<std::vector<Wall>>();
    std::vector<Texture> m_textures;

    Texture m_defaultTex;
    Texture m_defaultFloor;
    Texture m_defaultCeil;
    
    DirectionalLight m_light;

    int updateCameraSector(const Map& map, Vector2 p);

    void drawPlaneClaim(Line line, float height, bool isCeiling, DirectionalLight light, uint32_t color);

    int  findCameraSector(const Map& map, Vector2 p) const;
    void drawFlats(const Map& map, int camSector);
};
