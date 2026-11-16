#include "renderer.hpp"

bool Renderer::loadPNG(const std::string& path, Texture& out)
{
    SDL_Surface* loaded = SDL_LoadPNG(path.c_str());
    if (!loaded)
    {
        std::cerr << "Texture loading failure: " << SDL_GetError() << " " << path << "\n";
        return false;
    }
    SDL_Surface* conv = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(loaded);
    if (!conv) return false;

    out.width = conv->w;
    out.height = conv->h;
    out.pixels.resize(out.width * out.height);
    for (int y = 0; y < out.height; y++)
    {
        const uint8_t* src = (const uint8_t*)conv->pixels + y * conv->pitch;
        std::memcpy(&out.pixels[y * out.width], src, out.width * sizeof(uint32_t));
    }
    SDL_DestroySurface(conv);
    return true;
}

void Renderer::loadTextures(const std::vector<std::string>& paths)
{
    for (Texture* t : {&m_defaultTex, &m_defaultFloor, &m_defaultCeil})
    {
        t->width = 64; t->height = 64;
        t->pixels.resize(64 * 64);
        t->genChecker();
    }
    for (const std::string& p : paths)
    {
        Texture tex;
        m_textures.push_back(loadPNG(p, tex) ? tex : m_defaultTex);
    }
}

void Renderer::loadSkyFaces(const std::vector<std::string>& paths)
{
    m_sky.valid = false;
    if (paths.size() != NUM_FACES)
    {
        std::cerr << "Sky needs " << NUM_FACES << " faces, got " << paths.size() << "\n";
        return;
    }
    for (int i = 0; i < NUM_FACES; i++)
        if (!loadPNG(paths[i], m_sky.faces[i])) return;   // stay on the gradient fallback
    m_sky.valid = true;
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
        bool portal = w.backSect >= 0;
        if (!portal && !w.doubleSided && !frontFacing) continue;

        Wall draw = w;
        int nearIdx, farIdx;
        if (w.doubleSided && !portal)
        {
            nearIdx = w.frontSect;
            farIdx  = -1;
        }
        else
        {
            nearIdx = frontFacing ? w.frontSect : w.backSect;
            farIdx  = frontFacing ? w.backSect  : w.frontSect;
        }
        m_buf.drawSector = nearIdx;

        if (!frontFacing) { std::swap(draw.UStart, draw.UEnd); draw.normal = -draw.normal; }

        const Sector& nearSec = map.sectors[nearIdx];
        float s = shade(draw.normal, m_light) * nearSec.light;

        struct Piece
        {
            float bS, bE, tS, tE;
        };
        Piece pieces[2];
        int pieceCount = 0;

        Vector2 a = w.line.start, b = w.line.end;
        float nfA = nearSec.floorAt(a), nfB = nearSec.floorAt(b);
        float ncA = nearSec.ceilAt(a), ncB = nearSec.ceilAt(b);

        if (farIdx < 0)
            pieces[pieceCount++] = {nfA, nfB, ncA, ncB};
        else
        {
            const Sector &farSec = map.sectors[farIdx];
            float ffA = farSec.floorAt(a), ffB = farSec.floorAt(b);
            float fcA = farSec.ceilAt(a), fcB = farSec.ceilAt(b);

            if (ffA > nfA || ffB > nfB)
                pieces[pieceCount++] = {nfA, nfB, std::max(ffA, nfA), std::max(ffB, nfB)};

            bool bothSky = nearSec.sky && farSec.sky;
            if (!bothSky && (fcA < ncA || fcB < ncB))
                pieces[pieceCount++] = {std::min(fcA, ncA), std::min(fcB, ncB), ncA, ncB};

            float obA = std::max(nfA, ffA), obB = std::max(nfB, ffB);
            float otA = std::min(ncA, fcA), otB = std::min(ncB, fcB);
            if (otA > obA || otB > obB)
                markOpening(&m_buf, draw, obA, obB, otA, otB, nearIdx, farIdx, camSector);
        }

        for (int i = 0; i < pieceCount; i++)
        {
            const Piece &p = pieces[i];
            if (w.texId >= 0 && w.texId < (int)m_textures.size())
                fillWallTextured(&m_buf, draw, m_textures[w.texId], s, p.bS, p.bE, p.tS, p.tE);
            else
            {
                draw.bottom = p.bS;
                draw.top = p.tS;
                m_buf.setDrawColor(applyLighting(baseColor, m_light, draw));
                fillWall(&m_buf, draw);
            }
        }
    }

    m_buf.drawSector = -1;

    // if (m_sectorDBG)
    // {
    //     for (int i = 0; i < DEF_WIN_WIDTH * DEF_WIN_HEIGHT; i++)
    //     {
    //         if (m_buf.covered[i]) continue;
    //         int sec = m_buf.sectorAt[i];
    //         if (sec < 0) { m_buf.pixels[i] = rgb(40, 40, 40); continue; }
    //         m_buf.pixels[i] = rgb((uint8_t)(60 + (sec * 50) % 195),
    //                             100,
    //                             (uint8_t)(200 - (sec * 40) % 150));
    //     }
    //     return;
    // }

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
            if (sec < 0 && w.doubleSided) sec = w.frontSect;
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
            if (m_buf.covered[i])
                continue;

            int sec = m_buf.sectorAt[i];

            if (sec < 0)
                sec = camSector;
            if (sec < 0)
                continue;
            const Sector &S = map.sectors[sec];
            float xn = (x + 0.5f - HALF_WID) / f;
            Vector3 dir = fwd + right * xn + up * yn;

            auto hit = [&](float h0, Vector2 g, Vector2 o) -> float
            {
                float denom = dir.y - g.x * dir.x - g.y * dir.z;
                if (std::abs(denom) < 1e-6f)
                    return -1.0f;
                float num = h0 + g.x * (pos.x - o.x) + g.y * (pos.z - o.y) - pos.y;
                return num / denom;
            };

            float tf = hit(S.floorHt, S.floorGrad, S.floorOrigin);
            float tc = hit(S.ceilingHt, S.ceilGrad, S.ceilOrigin);

            bool useCeil;
            if (tf > 0 && (tc <= 0 || tf < tc))
                useCeil = false;
            else if (tc > 0)
                useCeil = true;
            else
                continue;
            float t = useCeil ? tc : tf;

            if (useCeil && S.sky)
            {
                m_buf.pixels[i] = m_sky.sample(dir);
                continue;
            }

            float wx = pos.x + dir.x * t, wz = pos.z + dir.z * t;
            const Texture &tex = useCeil ? pick(S.ceilingTexId, m_defaultCeil)
                                         : pick(S.floorTexId, m_defaultFloor);
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
        if (!(*m_renOrder)[i].doubleSided && (*m_renOrder)[i].cullWall(camFloorPos)) continue;
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
