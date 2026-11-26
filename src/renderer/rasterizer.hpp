#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <span>
#include <algorithm>

#include "framebuffer.hpp"
#include "globals/globals.hpp"
#include "engine/math/vectors.hpp"
#include "engine/math/geometry.hpp"
#include "engine/texture.hpp"

constexpr float CLIP_NEAR = 1e-5f;
constexpr int MAX_CLIP_VERTS = 8;

static inline bool clipLineToScreen(Vector2& a, Vector2& b)
{
    if (!std::isfinite(a.x) || !std::isfinite(a.y) || !std::isfinite(b.x) || !std::isfinite(b.y)) return false;

    const float xmin = 0.0f, ymin = 0.0f;
    const float xmax = (float)(DEF_WIN_WIDTH - 1), ymax = (float)(DEF_WIN_HEIGHT - 1);
    float dx = b.x - a.x, dy = b.y - a.y;
    float p[4] = { -dx, dx, -dy, dy };
    float q[4] = { a.x - xmin, xmax - a.x, a.y - ymin, ymax - a.y };
    float t0 = 0.0f, t1 = 1.0f;

    for (int i = 0; i < 4; i++)
    {
        if (p[i] == 0.0f) { if (q[i] < 0.0f) return false; continue; }
        float r = q[i] / p[i];
        if (p[i] < 0.0f) { if (r > t1) return false; t0 = std::max(t0, r); }
        else             { if (r < t0) return false; t1 = std::min(t1, r); }
    }
    Vector2 na{ a.x + t0 * dx, a.y + t0 * dy };
    Vector2 nb{ a.x + t1 * dx, a.y + t1 * dy };
    a = na;
    b = nb;
    return true;
}

inline void drawLine(Framebuffer* buf, Vector2 a, Vector2 b)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;

    bool steep = std::abs(dy) > std::abs(dx);
    if (steep)
    {
        std::swap(a.x, a.y);
        std::swap(b.x, b.y);
    }

    if (a.x > b.x)
    {
        std::swap(a, b);
    }

    dx = b.x - a.x;
    dy = b.y - a.y;

    float decision = 2 * dy - dx;
    int ystep = (dy >= 0) ? 1 : -1;

    int y = (int)a.y;
    for (int x = (int)a.x; x <= (int)b.x; x++)
    {
        if (steep) { buf->setPixel(y, x); }
        else { buf->setPixel(x, y); }

        if (decision > 0)
        {
            y += ystep;
            decision -= 2 * dx;
        }
        decision += 2 * std::abs(dy);
    }
}

static void drawWorldMarker(Framebuffer* buf, Vector3 p, uint32_t color, int radius)
{
    Camera* cam = buf->activeCamera;
    Vector3 cs = cam->toCamSpace(p);
    if (cs.z < cam->getNear()) return;

    Vector2 s = cam->projectCamSpace(cs);
    for (int dy = -radius; dy <= radius; dy++)
        for (int dx = -radius; dx <= radius; dx++)
            buf->setPixelColor((int)s.x + dx, (int)s.y + dy, color);
}

inline int clipNear(const Vector3* in, size_t n, float nearZ, Vector3* out)
{
    int count = 0;
    for (size_t i = 0; i < n; i++)
    {
        Vector3 a = in[i];
        Vector3 b = in[(i + 1) % n];
        float da = a.z - nearZ;
        float db = b.z - nearZ;

        if (da >= 0) out[count++] = a;
        if ((da >= 0) != (db >= 0))
        {
            out[count++] = a + (b - a) * (da / (da - db));
        }
    }
    return count;
}

inline int clipVertNear(const ClipVertex* in, size_t n, float nearZ, ClipVertex* out)
{
    int count = 0;
    for (size_t i = 0; i < n; i++)
    {
        ClipVertex a = in[i];
        ClipVertex b = in[(i + 1) % n];
        float da = a.pos.z - nearZ;
        float db = b.pos.z - nearZ;

        if (da >= 0) out[count++] = a;
        if ((da >= 0) != (db >= 0))
        {
            out[count++] = a.lerp(b, (da / (da - db)));
        }
    }
    return count;
}

inline float edgeX(float ax, float ay, float bx, float by, float yf)
{
    if (ay > by) { std::swap(ax, bx); std::swap(ay, by); }
    return ax + (yf - ay) / (by - ay) * (bx - ax);
}

inline void drawWall(Framebuffer* buf, Wall wall)
{

    Vector2 aTop = buf->activeCamera->projectPoint({wall.line.start.x, wall.top, wall.line.start.y});
    Vector2 aBottom = buf->activeCamera->projectPoint({wall.line.start.x, wall.bottom, wall.line.start.y});
    Vector2 bTop = buf->activeCamera->projectPoint({wall.line.end.x, wall.top, wall.line.end.y});
    Vector2 bBottom = buf->activeCamera->projectPoint({wall.line.end.x, wall.bottom, wall.line.end.y});

    drawLine(buf, aTop, bTop);
    drawLine(buf, aBottom, bBottom);
    drawLine(buf, aTop, aBottom);
    drawLine(buf, bTop, bBottom);
}

template <class F>
inline void scanConvex(std::span<const Vector2> verts, F&& visit)
{
    if (verts.size() < 3) return;
    float minYf = verts[0].y, maxYf = verts[0].y;
    for (auto& v : verts) { minYf = std::min(minYf, v.y); maxYf = std::max(maxYf, v.y); }
    int yStart = std::max(0, (int)std::floor(minYf));
    int yEnd   = std::min(DEF_WIN_HEIGHT - 1, (int)std::ceil(maxYf));
    int n = (int)verts.size();

    for (int y = yStart; y <= yEnd; y++)
    {
        float yf = y + 0.5f, xs[2]; int cnt = 0;
        for (int i = 0; i < n; i++)
        {
            Vector2 a = verts[i], b = verts[(i + 1) % n];
            if (a.y == b.y) continue;
            if (a.y > b.y) std::swap(a, b);
            if (yf >= a.y && yf < b.y)
                xs[cnt++] = edgeX(a.x, a.y, b.x, b.y, yf);
        }
        if (cnt != 2) continue;
        if (xs[0] > xs[1]) std::swap(xs[0], xs[1]);
        int xs0 = std::max(0, (int)std::ceil(xs[0] - 0.5f));
        int xe  = std::min(DEF_WIN_WIDTH - 1, (int)std::floor(xs[1] - 0.5f));
        for (int x = xs0; x <= xe; x++) visit(x, y);
    }
}

template <class D>
inline int clipPlane(Vector3 *in, int n, D dist, Vector3 *out)
{
    int count = 0;
    for (int i = 0; i < n; i++)
    {
        Vector3 a = in[i], b = in[(i + 1) % n];
        float da = dist(a), db = dist(b);
        if (da >= 0)
            out[count++] = a;
        if ((da >= 0) != (db >= 0))
        {
            out[count++] = (da > db) ? a + (b - a) * (da / (da - db)) : b + (a - b) * (db / (db - da));
        }
    }

    return count;
}

template <class D>
inline int clipVertPlane(const ClipVertex *in, int n, D dist, ClipVertex *out)
{
    int count = 0;
    for (int i = 0; i < n; i++)
    {
        ClipVertex a = in[i], b = in[(i + 1) % n];
        float da = dist(a.pos), db = dist(b.pos);
        if (da >= 0)
            out[count++] = a;
        if ((da >= 0) != (db >= 0))
        {
            out[count++] = (da > db) ? a.lerp(b, da / (da - db)) : b.lerp(a, db / (db - da));
        }
    }
    return count;
}

inline int clipProjectQuad(Camera* cam, const Wall& w, float bS, float bE, float tS, float tE, ScreenVertex* out)
{
    ClipVertex corners[4] =
    {
        { cam->toCamSpace({w.line.start.x, tS, w.line.start.y}), w.UStart, -tS / TEX_WORLD_H },
        { cam->toCamSpace({w.line.end.x,   tE, w.line.end.y}),   w.UEnd,   -tE / TEX_WORLD_H },
        { cam->toCamSpace({w.line.end.x,   bE, w.line.end.y}),   w.UEnd,   -bE / TEX_WORLD_H },
        { cam->toCamSpace({w.line.start.x, bS, w.line.start.y}), w.UStart, -bS / TEX_WORLD_H },
    };

    const float hw = 1.0f;
    const float hh = (float)HALF_HT / HALF_WID;
    ClipVertex b1[12], b2[12];

    int n = clipVertPlane(corners, 4, [](Vector3 v){ return v.z - CLIP_NEAR; }, b1);
    if (n >= 3) n = clipVertPlane(b1, n, [&](Vector3 v){ return  v.x + v.z * hw; }, b2);
    if (n >= 3) n = clipVertPlane(b2, n, [&](Vector3 v){ return -v.x + v.z * hw; }, b1);
    if (n >= 3) n = clipVertPlane(b1, n, [&](Vector3 v){ return  v.y + v.z * hh; }, b2);
    if (n >= 3) n = clipVertPlane(b2, n, [&](Vector3 v){ return -v.y + v.z * hh; }, b1);
    if (n < 3) return 0;

    for (int i = 0; i < n; i++)
    {
        Vector2 p = cam->projectCamSpace(b1[i].pos);
        float invZ = 1.0f / b1[i].pos.z;
        out[i] = { p.x, p.y, invZ, b1[i].u * invZ, b1[i].v * invZ };
    }
    return n;
}

inline void markOpening(Framebuffer* buf, const Wall& w, float bS, float bE, float tS, float tE, int nearSector, int farSector, int camSector)
{
    ScreenVertex s[12];
    int n = clipProjectQuad(buf->activeCamera, w, bS, bE, tS, tE, s);
    if (n < 3) return;

    Vector2 p[12];
    for (int i = 0; i < n; i++) p[i] = { s[i].x, s[i].y };

    scanConvex(std::span<const Vector2>(p, n), [&](int x, int y)
    {
        int i = y * DEF_WIN_WIDTH + x;
        if (buf->covered[i]) return;
        int cur = buf->sectorAt[i];
        if (cur < 0) cur = camSector;
        if (cur != nearSector) return;
        buf->sectorAt[i] = (int16_t)farSector;
    });
}

inline void fillConvexPolygon(Framebuffer* buf, std::span<const Vector2> verts)
{
    if (verts.size() < 3) return;

    float minYf = verts[0].y, maxYf = verts[0].y;
    for (auto& v : verts) { minYf = std::min(minYf, v.y); maxYf = std::max(maxYf, v.y); }

    int yStart = std::max(0, (int)std::floor(minYf));
    int yEnd   = std::min(DEF_WIN_HEIGHT - 1, (int)std::ceil(maxYf));

    int n = (int)verts.size();

    for (int y = yStart; y <= yEnd; y++)
    {
        float yf = y + 0.5f;
        float xs[2];
        int xsCount = 0;

        for (int i = 0; i < n; i++)
        {
            Vector2 a = verts[i];
            Vector2 b = verts[(i + 1) % n];

            if (a.y == b.y) continue;

            if ((yf >= a.y && yf < b.y) || (yf >= b.y && yf < a.y))
            {
                if (xsCount < 2) xs[xsCount++] = edgeX(a.x, a.y, b.x, b.y, yf);
            }
        }

        if (xsCount == 2)
        {
            if (xs[0] > xs[1]) std::swap(xs[0], xs[1]);

            int xStart = std::max(0, (int)std::ceil(xs[0] - 0.5f));
            int xEnd   = std::min(DEF_WIN_WIDTH - 1, (int)std::floor(xs[1] - 0.5f));

            for (int x = xStart; x <= xEnd; x++)
            {
                if (!buf->isCovered(x, y) && buf->sectorMatches(y * DEF_WIN_WIDTH + x)) buf->plotFree(x, y, buf->current_color);
            }
        }
    }
}

inline void fillTexturedPolygon(Framebuffer *buf, std::span<const ScreenVertex> verts, const Texture &tex, float shade)
{
    if (verts.size() < 3)
        return;

    float minYf = verts[0].y, maxYf = verts[0].y;
    for (auto &v : verts)
    {
        minYf = std::min(minYf, v.y);
        maxYf = std::max(maxYf, v.y);
    }

    int yStart = std::max(0, (int)std::floor(minYf));
    int yEnd = std::min(DEF_WIN_HEIGHT - 1, (int)std::ceil(maxYf));
    int n = (int)verts.size();

    uint32_t shade256 = shadeToFixed(shade);

    for (int y = yStart; y <= yEnd; y++)
    {
        float yf = y + 0.5f;
        ScreenVertex cross[2];
        int crossCount = 0;

        for (int i = 0; i < n; i++)
        {
            ScreenVertex a = verts[i];
            ScreenVertex b = verts[(i + 1) % n];
            if (a.y == b.y)
                continue;

            if ((yf >= a.y && yf < b.y) || (yf >= b.y && yf < a.y))
            {
                if (crossCount < 2)
                {
                    float t = (yf - a.y) / (b.y - a.y);
                    ScreenVertex c = a.lerp(b, t);
                    c.x = edgeX(a.x, a.y, b.x, b.y, yf);
                    c.y = yf;
                    cross[crossCount++] = c;
                }
            }
        }

        if (crossCount != 2)
            continue;
        if (cross[0].x > cross[1].x)
            std::swap(cross[0], cross[1]);

        const ScreenVertex &L = cross[0];
        const ScreenVertex &R = cross[1];
        float spanW = R.x - L.x;
        if (spanW <= 0.0f)
            continue;

        int xStart = std::max(0, (int)std::ceil(L.x - 0.5f));
        int xEnd = std::min(DEF_WIN_WIDTH - 1, (int)std::ceil(R.x - 0.5f) - 1);

        for (int x = xStart; x <= xEnd; x++)
        {
            if (buf->isCovered(x, y))
                continue;
            if (!buf->sectorMatches(y * DEF_WIN_WIDTH + x))
                continue;

            float s = (x + 0.5f - L.x) / spanW;
            ScreenVertex p = L.lerp(R, s);
            float u = p.uOverZ / p.invZ;
            float v = p.vOverZ / p.invZ;

            buf->plotFree(x, y, scaleColorFixed(tex.sample(u, v), shade256));
        }
    }
}

inline void fillWall(Framebuffer* buf, Wall wall)
{
    Camera* cam = buf->activeCamera;

    Vector3 corners[4] =
    {
        { cam->toCamSpace({wall.line.start.x, wall.top, wall.line.start.y}) },
        { cam->toCamSpace({wall.line.end.x, wall.top, wall.line.end.y}) },
        { cam->toCamSpace({wall.line.end.x, wall.bottom, wall.line.end.y}) },
        { cam->toCamSpace({wall.line.start.x, wall.bottom, wall.line.start.y}) }
    };

    Vector3 clipped[MAX_CLIP_VERTS];
    int count = clipNear(corners, 4, CLIP_NEAR, clipped);
    if (count < 3) return;

    std::array<Vector2, 8> vertsProj;
    for (int i = 0; i < count; i++)
    {  
        vertsProj[i] = cam->projectCamSpace(clipped[i]);
    }

    fillConvexPolygon(buf, std::span<const Vector2>(vertsProj.data(), count));
}

inline void fillWallTextured(Framebuffer* buf, Wall wall, const Texture& tex, float shade, float bS, float bE, float tS, float tE)
{
    ScreenVertex s[12];
    int n = clipProjectQuad(buf->activeCamera, wall, bS, bE, tS, tE, s);
    if (n >= 3)
        fillTexturedPolygon(buf, std::span<const ScreenVertex>(s, n), tex, shade);
}
