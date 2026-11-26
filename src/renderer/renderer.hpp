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
#include "engine/sky.hpp"
#include "engine/raycast.hpp"

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
    void loadSkyTexture(const std::string& path);
    void loadSkyFaces(const std::vector<std::string>& paths);

    void beginRender();
    void renderMap(const Map& map);
    void renderMapDBGCLR(const Map& map);
    void render2DView(const Map& map);
    void endRender();

    void renderDebugTraces(const std::vector<DebugTrace>& traces);

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
    Texture m_defaultSky;

    Texture m_skyTex;
    SkyBox m_sky;
    
    DirectionalLight m_light;

    int m_lastCamSector = -1;

    bool m_sectorDBG = true;

    bool loadPNG(const std::string& path, Texture& out);

    int updateCameraSector(const Map& map, Vector2 p);

    void drawPlaneClaim(Line line, float height, bool isCeiling, DirectionalLight light, uint32_t color);

    int  findCameraSector(const Map& map, Vector2 p);
    void drawFlats(const Map& map, int camSector);

    static uint32_t hitColor(HitType t)
    {
        switch (t)
        {
            case HitType::WALL:    return rgb(255, 80, 80);
            case HitType::FLOOR:   return rgb(80, 255, 80);
            case HitType::CEILING: return rgb(80, 160, 255);
            case HitType::NONE:     return rgb(255, 255, 255);
            default:               return rgb(160, 160, 160);
        }
    }
};
