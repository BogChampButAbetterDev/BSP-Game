#include "renderer.hpp"

void Renderer::loadTextures(const std::vector<std::string> &paths)
{
    for (const std::string& p : paths)
    {
        SDL_Surface* loaded = SDL_LoadPNG(p.c_str());
        if (!loaded) 
        { 
            std::cerr << "Texture loading failure: " << SDL_GetError() << " " << p << "\n"; 
            Texture tex;
            tex.width = 64;
            tex.height = 64;
            tex.pixels.resize(tex.width * tex.height);
            tex.genChecker();
            m_textures.push_back(tex);
        }

        SDL_Surface* converted = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_ABGR8888);
        SDL_DestroySurface(loaded);
        Texture tex;
        tex.width = converted->w;
        tex.height = converted->h;
        tex.pixels.resize(tex.width * tex.height);
        for (int y = 0; y < tex.height; y++)
        {
            const uint8_t* src = (const uint8_t*)converted->pixels + y * converted->pitch;
            std::memcpy(&tex.pixels[y * tex.width], src, tex.width * sizeof(uint32_t));
        }
        m_textures.push_back(tex);
        SDL_DestroySurface(converted);
    }
}

void Renderer::beginRender()
{
    m_buf.clear(rgb(0x00, 0x00, 0x00));
}

void Renderer::renderMap(const Map& map)
{
    Vector2 camFloorPos = {m_buf.activeCamera->getPos().x, m_buf.activeCamera->getPos().z};
    m_renOrder->clear();

    uint32_t baseColor = rgb(255, 255, 255);

    BSPTraverse(map.tree, map.tree.root, camFloorPos, *m_renOrder);
    for (const Wall& w : (*m_renOrder))
    {
        if (w.cullWall(camFloorPos)) continue;
        float s = shade(w.normal, m_light);

        if (w.texId >= 0 && w.texId < (int)m_textures.size())
        {
            fillWallTextured(&m_buf, w, m_textures[w.texId], s);
        }
        else
        {
            m_buf.setDrawColor(applyLighting(baseColor, m_light, w));
            fillWall(&m_buf, w);
        }
    }
}

void Renderer::renderMapDBGCLR(const Map& map)
{
    Vector2 camFloorPos = {m_buf.activeCamera->getPos().x, m_buf.activeCamera->getPos().z};

    m_renOrder->clear();

    BSPTraverse(map.tree, map.tree.root, camFloorPos, *m_renOrder);
    for (size_t i = 0; i < m_renOrder->size(); i++)
    {
        if ((*m_renOrder)[i].cullWall(camFloorPos)) continue;
        m_buf.setDrawColor(applyLighting(debugColor(i), m_light, (*m_renOrder)[i]));
        fillWall(&m_buf, (*m_renOrder)[i]);
    }
}

void Renderer::render2DView(const Map &map)
{
    DebugView::render(&m_buf, map.walls);
}

void Renderer::endRender()
{
    m_buf.present(m_ren);
}
