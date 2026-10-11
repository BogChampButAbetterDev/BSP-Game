#include "renderer.hpp"

void Renderer::loadTextures(const std::vector<std::string>& paths)
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
            continue;
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
        float side = w.line.sideOf(camFloorPos);
        if (side == 0) continue;

        bool frontFacing = side > 0;
        bool twoSided = w.backSect >= 0;

        if (!twoSided && !frontFacing) continue;

        Wall draw = w;
        int nearIdx = frontFacing ? w.frontSect : w.backSect;
        int farIdx = frontFacing ? w.backSect : w.frontSect;

        if (!frontFacing)
        {
            std::swap(draw.UStart, draw.UEnd);
            draw.normal = -draw.normal;
        }

        const Sector& nearSec = map.sectors[nearIdx];
        float s = shade(draw.normal, m_light);

        struct Piece { float bottom = 0.0f; float top = 0.0f; };
        Piece pieces[2];
        int pieceCount = 0;

        if (farIdx < 0)
        {
            pieces[pieceCount++] = { nearSec.floorHt, nearSec.ceilingHt };
        }
        else
        {
            const Sector& farSec = map.sectors[farIdx];
            if (farSec.floorHt > nearSec.floorHt)
                pieces[pieceCount++] = { nearSec.floorHt, farSec.floorHt };
            if (farSec.ceilingHt < nearSec.ceilingHt)
                pieces[pieceCount++] = {farSec.ceilingHt, nearSec.ceilingHt};
        }

        for (int i = 0; i < pieceCount; i++)
        {
            draw.bottom = pieces[i].bottom;
            draw.top = pieces[i].top;
            if (w.texId >= 0 && w.texId < (int)m_textures.size())
            {
                fillWallTextured(&m_buf, draw, m_textures[draw.texId], s, draw.bottom, draw.top);
            }
            else
            {
                m_buf.setDrawColor(applyLighting(baseColor, m_light, draw));
                fillWall(&m_buf, draw);
            }
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
