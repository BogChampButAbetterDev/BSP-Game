#include "renderer.hpp"

void Renderer::loadTextures(const std::vector<std::string>& paths)
{
    for (Texture *t : {&m_defaultTex, &m_defaultFloor, &m_defaultCeil})
    {
        t->width = 64;
        t->height = 64;
        t->pixels.resize(64 * 64);
        t->genChecker();
    }

    for (const std::string& p : paths)
    {
        SDL_Surface* loaded = SDL_LoadPNG(p.c_str());
        if (!loaded) 
        { 
            std::cerr << "Texture loading failure: " << SDL_GetError() << " " << p << "\n"; 
            m_textures.push_back(m_defaultTex);
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
    Camera* cam = m_buf.activeCamera;
    Vector2 camFloorPos = {cam->getPos().x, cam->getPos().z};
    View V = makeView(cam);

    m_top.assign(DEF_WIN_WIDTH, 0);
    m_bottom.assign(DEF_WIN_WIDTH, DEF_WIN_HEIGHT - 1);
    m_openCols = DEF_WIN_WIDTH;

    m_renOrder->clear();
    BSPTraverse(map.tree, map.tree.root, camFloorPos, *m_renOrder);

    auto pick = [&](int id, const Texture& fallback) -> const Texture&
    {
        return (id >= 0 && id < (int)m_textures.size()) ? m_textures[id] : fallback;
    };

    for (const Wall& w : *m_renOrder)
    {
        if (m_openCols <= 0) break;

        float side = w.line.sideOf(camFloorPos);
        if (side == 0) continue;
        bool frontFacing = side > 0;
        bool twoSided = w.backSect >= 0;
        if (!twoSided && !frontFacing) continue;

        Wall draw = w;
        int nearIdx = frontFacing ? w.frontSect : w.backSect;
        int farIdx  = frontFacing ? w.backSect  : w.frontSect;
        if (!frontFacing) { std::swap(draw.UStart, draw.UEnd); draw.normal = -draw.normal; }

        const Sector& nearSec = map.sectors[nearIdx];
        const Sector* farSec  = (farIdx >= 0) ? &map.sectors[farIdx] : nullptr;
        float s = shade(draw.normal, m_light);

        WallSpan sp;
        if (!projectWallSpan(cam, V, draw, sp)) continue;

        const Texture& wallTex  = pick(w.texId, m_defaultTex);
        const Texture& floorTex = pick(nearSec.floorTexId,   m_defaultFloor);
        const Texture& ceilTex  = pick(nearSec.ceilingTexId, m_defaultCeil);
        float flatShade = nearSec.light;

        int x0 = std::max(0, (int)std::ceil(sp.sx1 - 0.5f));
        int x1 = std::min(DEF_WIN_WIDTH - 1, (int)std::floor(sp.sx2 - 0.5f));
        float invSpan = 1.0f / (sp.sx2 - sp.sx1);

        for (int x = x0; x <= x1; x++)
        {
            int top = m_top[x], bot = m_bottom[x];
            if (top > bot) continue;

            float t    = (x + 0.5f - sp.sx1) * invSpan;
            float invZ = sp.invZ1 + (sp.invZ2 - sp.invZ1) * t;
            float u    = (sp.uZ1 + (sp.uZ2 - sp.uZ1) * t) / invZ;

            auto Y = [&](float h) { return (int)std::ceil(V.oy - V.f * (h - V.eyeY) * invZ - 0.5f); };
            int c = Y(nearSec.ceilingHt);
            int f = Y(nearSec.floorHt);

            fillFlatColumn(&m_buf, V, x, top, std::min(c - 1, bot), nearSec.ceilingHt, ceilTex,  flatShade);
            fillFlatColumn(&m_buf, V, x, std::max(f, top), bot,     nearSec.floorHt,   floorTex, flatShade);

            if (!farSec)
            {
                fillWallColumn(&m_buf, V, x, std::max(c, top), std::min(f - 1, bot), invZ, u, wallTex, s);
                m_top[x] = 1; m_bottom[x] = 0;
                --m_openCols;
                continue;
            }

            int fc = Y(farSec->ceilingHt);
            int ff = Y(farSec->floorHt);

            if (fc > c)
                fillWallColumn(&m_buf, V, x, std::max(c, top), std::min(fc - 1, bot), invZ, u, wallTex, s);
            if (ff < f)
                fillWallColumn(&m_buf, V, x, std::max(ff, top), std::min(f - 1, bot), invZ, u, wallTex, s);

            m_top[x]    = std::max(top, std::max(c, fc));
            m_bottom[x] = std::min(bot, std::min(f, ff) - 1);
            if (m_top[x] > m_bottom[x]) --m_openCols;
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
