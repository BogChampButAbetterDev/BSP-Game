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

void Renderer::loadSkyTexture(const std::string &p)
{
    m_defaultSky.width = 64;
    m_defaultSky.height = 64;
    m_defaultSky.pixels.resize(m_defaultSky.width * m_defaultSky.height);
    m_defaultSky.genChecker();

    SDL_Surface *loaded = SDL_LoadPNG(p.c_str());
    if (!loaded)
    {
        std::cerr << "Texture loading failure: " << SDL_GetError() << " " << p << "\n";
        m_textures.push_back(m_defaultTex);
        return;
    }

    SDL_Surface *converted = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(loaded);
    Texture tex;
    tex.width = converted->w;
    tex.height = converted->h;
    tex.pixels.resize(tex.width * tex.height);
    for (int y = 0; y < tex.height; y++)
    {
        const uint8_t *src = (const uint8_t *)converted->pixels + y * converted->pitch;
        std::memcpy(&tex.pixels[y * tex.width], src, tex.width * sizeof(uint32_t));
    }   
    m_skyTex = tex;
    SDL_DestroySurface(converted);
}

void Renderer::loadSkyFaces(const std::vector<std::string> &paths)
{
    if (paths.size() != NUM_SKY_FACES) return;

    m_defaultSky.width = 64;
    m_defaultSky.height = 64;
    m_defaultSky.pixels.resize(m_defaultSky.width * m_defaultSky.height);
    m_defaultSky.genChecker();

    for (const std::string &p : paths)
    {
        for (int i = 0; i < NUM_SKY_FACES; i++)
        {
            SDL_Surface *loaded = SDL_LoadPNG(p.c_str());
            if (!loaded)
            {
                std::cerr << "Texture loading failure: " << SDL_GetError() << " " << p << "\n";
                m_sky.faces[i] = m_defaultSky;
                return;
            }

            SDL_Surface *converted = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_ABGR8888);
            SDL_DestroySurface(loaded);
            Texture tex;
            tex.width = converted->w;
            tex.height = converted->h;
            tex.pixels.resize(tex.width * tex.height);
            for (int y = 0; y < tex.height; y++)
            {
                const uint8_t *src = (const uint8_t *)converted->pixels + y * converted->pitch;
                std::memcpy(&tex.pixels[y * tex.width], src, tex.width * sizeof(uint32_t));
            }

            m_sky.faces[i] = tex;
            SDL_DestroySurface(converted);
        }
    }

    m_sky.valid = true;
}

void Renderer::beginRender()
{
    m_buf.clear(rgb(0x00, 0x00, 0x00));
    m_buf.setDrawColor(rgb(0xFF, 0x00, 0x00));
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
    drawFlats(map, camSector);

    renderSprites(m_sprites);
}

int Renderer::findCameraSector(const Map& map, Vector2 p)
{
    std::vector<uint8_t> parity(map.sectors.size(), 0);

    for (const Wall& w : map.walls)
    {
        if (w.backSect < 0 && w.doubleSided) continue;

        Vector2 a = w.line.start, b = w.line.end;
        if ((a.y > p.y) == (b.y > p.y)) continue;

        float x = a.x + (p.y - a.y) / (b.y - a.y) * (b.x - a.x);
        if (x <= p.x) continue;

        if (w.frontSect >= 0) parity[w.frontSect] ^= 1;
        if (w.backSect  >= 0) parity[w.backSect]  ^= 1;
    }

    if (m_lastCamSector >= 0 && m_lastCamSector < (int)parity.size() && parity[m_lastCamSector])
        return m_lastCamSector;

    for (size_t i = 0; i < parity.size(); i++)
        if (parity[i]) { m_lastCamSector = (int)i; return m_lastCamSector; }

    return m_lastCamSector;
}

void Renderer::drawFlats(const Map& map, int camSector)
{
    Camera* cam = m_buf.activeCamera;
    Vector3 pos = cam->getPos();
    Vector3 fwd   = cam->camToWorldDir({0, 0, 1});
    Vector3 right = cam->camToWorldDir({1, 0, 0});
    Vector3 up    = cam->camToWorldDir({0, 1, 0});
    float f = (float)HALF_WID;

    std::vector<uint32_t> light256(map.sectors.size());
    for (size_t k = 0; k < map.sectors.size(); k++)
        light256[k] = shadeToFixed(map.sectors[k].light);

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

            auto hit = [&](float h0, Vector2 g, Vector2 o, float& denom) -> float
            {
                denom = dir.y - g.x * dir.x - g.y * dir.z;
                if (std::abs(denom) < 1e-6f)
                    return -1.0f;
                float num = h0 + g.x * (pos.x - o.x) + g.y * (pos.z - o.y) - pos.y;
                return num / denom;
            };

            float dF, dC;
            float tf = hit(S.floorHt, S.floorGrad, S.floorOrigin, dF);
            float tc = hit(S.ceilingHt, S.ceilGrad, S.ceilOrigin, dC);

            bool floorOK = tf > 0 && dF < 0;
            bool ceilOK = tc > 0 && dC > 0;

            bool useCeil;
            if (floorOK && (!ceilOK || tf < tc))
                useCeil = false;
            else if (ceilOK)
                useCeil = true;
            else
            {
                if (S.sky)
                    m_buf.pixels[i] = m_sky.sample(dir);
                continue;
            }
            float t = useCeil ? tc : tf;

            if (useCeil && S.sky)
            {
                m_buf.pixels[i] = m_sky.sample(dir);
                continue;
            }

            float wx = pos.x + dir.x * t, wz = pos.z + dir.z * t;
            const Texture &tex = useCeil ? pick(S.ceilingTexId, m_defaultCeil) : pick(S.floorTexId, m_defaultFloor);
            m_buf.pixels[i] = scaleColorFixed(tex.sample(wx / TEX_WORLD_H, wz / TEX_WORLD_H), light256[sec]);
            m_buf.depth[i] = 1.0f / t;
        }
    }
}

void Renderer::drawSprite(const Sprite3D &s, Vector3 c)
{
    Camera* cam = m_buf.activeCamera;
    Vector2 pos = cam->projectCamSpace(c);
    float halfW = 0.5f * s.width  * cam->getFocal() / c.z;
    float h = s.height * cam->getFocal() / c.z;
    fillDepthQuad(&m_buf, (int)(pos.x - halfW), (int)(pos.y - h), (int)(pos.x + halfW), (int)pos.y, 1.0f / c.z, m_buf.current_color);
}

static void drawWorldLine(Framebuffer* buf, Vector3 from, Vector3 to, uint32_t color)
{
    Camera* cam = buf->activeCamera;
    const float nearZ = cam->getNear();

    Vector3 a = cam->toCamSpace(from);
    Vector3 b = cam->toCamSpace(to);
    if (a.z < nearZ && b.z < nearZ) return;
    if (a.z < nearZ) a = a + (b - a) * ((nearZ - a.z) / (b.z - a.z));
    if (b.z < nearZ) b = b + (a - b) * ((nearZ - b.z) / (a.z - b.z));

    Vector2 pa = cam->projectCamSpace(a);
    Vector2 pb = cam->projectCamSpace(b);
    if (!clipLineToScreen(pa, pb)) return;

    buf->setDrawColor(color);
    drawLine(buf, pa, pb);
}

void Renderer::renderSprites(const std::vector<Sprite3D> &sprites)
{
    Camera* cam = m_buf.activeCamera;

    struct Item { const Sprite3D* s; Vector3 camSpacePos; };
    std::vector<Item> items;

    for (const auto& s : sprites)
    {
        Vector3 c = cam->toCamSpace(s.pos);
        if (c.z < CLIP_NEAR) continue;
        items.push_back({&s, c});
    }

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.camSpacePos.z > b.camSpacePos.z; });

    for (const auto& it : items)
    {
        drawSprite(*it.s, it.camSpacePos);
    }
}

void Renderer::renderDebugTraces(const std::vector<DebugTrace> &traces)
{
    if constexpr (!IS_DEBUG) return;

    const uint32_t TRACE_HIT  = rgb(255, 60, 60);
    const uint32_t TRACE_MISS = rgb(60, 255, 60);

    for (const DebugTrace& t : traces)
    {
        if (t.hit)
        {
            drawWorldLine(&m_buf, t.start, t.hitPoint, TRACE_HIT);
            drawWorldLine(&m_buf, t.hitPoint, t.end, TRACE_MISS);
            drawWorldMarker(&m_buf, t.hitPoint, hitColor(t.type), 3);
        }
        else
        {
            drawWorldLine(&m_buf, t.start, t.end, TRACE_MISS);
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
