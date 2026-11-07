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
    Vector2 camFloorPos = {m_buf.activeCamera->getPos().x, m_buf.activeCamera->getPos().z};
    int camSector = findCameraSector(map, camFloorPos);

    m_renOrder->clear();
    uint32_t baseColor = rgb(255, 255, 255);
    BSPTraverse(map.tree, map.tree.root, camFloorPos, *m_renOrder);

    m_buf.camSector = camSector;

    for (const Wall& w : *m_renOrder)
    {
        float side = w.line.sideOf(camFloorPos);
        if (side == 0) continue;
        bool frontFacing = side > 0;
        bool twoSided = w.backSect >= 0;
        if (!twoSided && !frontFacing) continue;

        Wall draw = w;
        int nearIdx = frontFacing ? w.frontSect : w.backSect;
        m_buf.drawSector = nearIdx;

        int farIdx  = frontFacing ? w.backSect  : w.frontSect;
        if (!frontFacing) { std::swap(draw.UStart, draw.UEnd); draw.normal = -draw.normal; }

        const Sector& nearSec = map.sectors[nearIdx];
        float s = shade(draw.normal, m_light) * nearSec.light;

        struct Piece { float bottom, top; };
        Piece pieces[2]; int pieceCount = 0;

        if (farIdx < 0)
            pieces[pieceCount++] = { nearSec.floorHt, nearSec.ceilingHt };
        else
        {
            const Sector& farSec = map.sectors[farIdx];
            if (farSec.floorHt > nearSec.floorHt)
                pieces[pieceCount++] = { nearSec.floorHt, farSec.floorHt };
            if (farSec.ceilingHt < nearSec.ceilingHt)
                pieces[pieceCount++] = { farSec.ceilingHt, nearSec.ceilingHt };

            float oBottom = std::max(nearSec.floorHt, farSec.floorHt);
            float oTop    = std::min(nearSec.ceilingHt, farSec.ceilingHt);
            if (oTop > oBottom) markOpening(&m_buf, draw, oBottom, oTop, nearIdx, farIdx, camSector);
        }

        for (int i = 0; i < pieceCount; i++)
        {
            draw.bottom = pieces[i].bottom;
            draw.top = pieces[i].top;
            if (w.texId >= 0 && w.texId < (int)m_textures.size())
                fillWallTextured(&m_buf, draw, m_textures[w.texId], s, draw.bottom, draw.top);
            else
            {
                m_buf.setDrawColor(applyLighting(baseColor, m_light, draw));
                fillWall(&m_buf, draw);
            }
        }
    }

    m_buf.drawSector = -1;
    drawFlats(map, camSector);
}

static int castSector(const Map& map, Vector2 p, Vector2 d)
{
    float best = 1e30f; int sec = -1;
    auto cross = [](float ax, float ay, float bx, float by) { return ax * by - ay * bx; };

    for (const Wall& w : map.walls)
    {
        float rx = w.line.end.x - w.line.start.x, ry = w.line.end.y - w.line.start.y;
        float qx = w.line.start.x - p.x,          qy = w.line.start.y - p.y;
        float denom = cross(d.x, d.y, rx, ry);
        if (std::abs(denom) < 1e-8f) continue;
        float t = cross(qx, qy, rx, ry) / denom;
        float u = cross(qx, qy, d.x, d.y) / denom;
        if (t > 0 && u >= 0 && u <= 1 && t < best)
        {
            best = t;
            sec = (w.line.sideOf(p) > 0) ? w.frontSect : w.backSect;
        }
    }
    return sec;
}

int Renderer::findCameraSector(const Map& map, Vector2 p) const
{
    constexpr int RAYS = 8;
    std::vector<int> votes(map.sectors.size(), 0);

    for (int k = 0; k < RAYS; k++)
    {
        float a = (k + 0.37f) * (6.2831853f / RAYS);
        int s = castSector(map, p, { std::cos(a), std::sin(a) });
        if (s >= 0) votes[s]++;
    }

    int best = -1, bestVotes = 0;
    for (size_t i = 0; i < votes.size(); i++)
        if (votes[i] > bestVotes) { bestVotes = votes[i]; best = (int)i; }
    return best;
}

void Renderer::drawFlats(const Map& map, int camSector)
{
    Camera* cam = m_buf.activeCamera;
    Vector3 pos = cam->getPos();
    Vector3 fwd   = cam->camToWorldDir({0, 0, 1});
    Vector3 right = cam->camToWorldDir({1, 0, 0});
    Vector3 up    = cam->camToWorldDir({0, 1, 0});
    float f = (float)HALF_WID;

    auto pick = [&](int id, const Texture& fb) -> const Texture&
    { return (id >= 0 && id < (int)m_textures.size()) ? m_textures[id] : fb; };

    for (int y = 0; y < DEF_WIN_HEIGHT; y++)
    {
        float yn = (HALF_HT - (y + 0.5f)) / f;
        for (int x = 0; x < DEF_WIN_WIDTH; x++)
        {
            int i = y * DEF_WIN_WIDTH + x;
            if (m_buf.covered[i]) continue;

            int sec = m_buf.sectorAt[i];
            //bool tagged = sec >= 0;
            //if (!tagged) sec = camSector;
            if (sec < 0) sec = camSector;
            if (sec < 0) continue;
            const Sector& S = map.sectors[sec];

            float xn = (x + 0.5f - HALF_WID) / f;
            Vector3 dir = fwd + right * xn + up * yn;

            bool ceil = dir.y > 0;
            float h = ceil ? S.ceilingHt : S.floorHt;
            float dy = h - pos.y;
            if (std::abs(dir.y) < 1e-6f || (dy > 0) != ceil) continue;

            float t = dy / dir.y;
            float wx = pos.x + dir.x * t, wz = pos.z + dir.z * t;
            const Texture& tex = ceil ? pick(S.ceilingTexId, m_defaultCeil)
                                      : pick(S.floorTexId,   m_defaultFloor);
            m_buf.pixels[i] = scaleColor(tex.sample(wx / TEX_WORLD_H, wz / TEX_WORLD_H), S.light);
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
