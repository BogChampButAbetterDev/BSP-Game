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

constexpr int MAX_CLIP_VERTS = 8;

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
            if ((yf >= a.y && yf < b.y) || (yf >= b.y && yf < a.y))
                if (cnt < 2) xs[cnt++] = a.x + (yf - a.y) / (b.y - a.y) * (b.x - a.x);
        }
        if (cnt != 2) continue;
        if (xs[0] > xs[1]) std::swap(xs[0], xs[1]);
        int xs0 = std::max(0, (int)std::ceil(xs[0] - 0.5f));
        int xe  = std::min(DEF_WIN_WIDTH - 1, (int)std::floor(xs[1] - 0.5f));
        for (int x = xs0; x <= xe; x++) visit(x, y);
    }
}

inline void markOpening(Framebuffer* buf, const Wall& w, float bottom, float top, int nearSector, int farSector, int camSector)
{
    Camera* cam = buf->activeCamera;
    Vector3 c[4] =
    {
        cam->toCamSpace({w.line.start.x, top,    w.line.start.y}),
        cam->toCamSpace({w.line.end.x,   top,    w.line.end.y}),
        cam->toCamSpace({w.line.end.x,   bottom, w.line.end.y}),
        cam->toCamSpace({w.line.start.x, bottom, w.line.start.y}),
    };
    Vector3 clipped[MAX_CLIP_VERTS];
    int n = clipNear(c, 4, 1e-3f, clipped);
    if (n < 3) return;
    Vector2 p[MAX_CLIP_VERTS];
    for (int i = 0; i < n; i++) p[i] = cam->projectCamSpace(clipped[i]);

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
                float t = (yf - a.y) / (b.y - a.y);
                if (xsCount < 2) xs[xsCount++] = a.x + t * (b.x - a.x);
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

inline void fillTexturedPolygon(Framebuffer* buf, std::span<const ScreenVertex> verts, const Texture& tex, float shade)
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
        ScreenVertex cross[2];
        int crossCount = 0;

        for (int i = 0; i < n; i++)
        {
            ScreenVertex a = verts[i];
            ScreenVertex b = verts[(i + 1) % n];
            if (a.y == b.y) continue;

            if ((yf >= a.y && yf < b.y) || (yf >= b.y && yf < a.y))
            {
                float t = (yf - a.y) / (b.y - a.y);
                if (crossCount < 2) cross[crossCount++] = a.lerp(b, t);
            }
        }

        if (crossCount != 2) continue;
        if (cross[0].x > cross[1].x) std::swap(cross[0], cross[1]);

        const ScreenVertex& L = cross[0];
        const ScreenVertex& R = cross[1];
        float spanW = R.x - L.x;
        if (spanW <= 0.0f) continue;

        int xStart = std::max(0, (int)std::ceil(L.x - 0.5f));
        int xEnd   = std::min(DEF_WIN_WIDTH - 1, (int)std::floor(R.x - 0.5f));

        for (int x = xStart; x <= xEnd; x++)
        {
            if (buf->isCovered(x, y)) continue;
            if (!buf->sectorMatches(y * DEF_WIN_WIDTH + x)) continue;

            float s = (x + 0.5f - L.x) / spanW;
            ScreenVertex p = L.lerp(R, s);
            float u = p.uOverZ / p.invZ;
            float v = p.vOverZ / p.invZ;

            buf->plotFree(x, y, scaleColor(tex.sample(u, v), shade));
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
        { cam->toCamSpace({wall.line.start.x, wall.bottom, wall.line.start.y}) },
        { cam->toCamSpace({wall.line.end.x, wall.bottom, wall.line.end.y}) }
    };

    const float nearPlane = cam->getNear();

    Vector3 clipped[MAX_CLIP_VERTS];
    int count = clipNear(corners, 4, nearPlane, clipped);
    if (count < 3) return;

    std::array<Vector2, 8> vertsProj;
    for (int i = 0; i < count; i++)
    {  
        vertsProj[i] = cam->projectCamSpace(clipped[i]);
    }

    fillConvexPolygon(buf, std::span<const Vector2>(vertsProj.data(), count));
}

inline void fillWallTextured(Framebuffer* buf, Wall wall, const Texture& tex, float shade, float yBottom, float yTop)
{
    Camera* cam = buf->activeCamera;

    float vTop = -yTop / TEX_WORLD_H;
    float vBottom = -yBottom / TEX_WORLD_H;

    ClipVertex corners[4] =
    {
        { cam->toCamSpace({wall.line.start.x, wall.top,    wall.line.start.y}), wall.UStart, vTop    },
        { cam->toCamSpace({wall.line.end.x,   wall.top,    wall.line.end.y}),   wall.UEnd,   vTop    },
        { cam->toCamSpace({wall.line.end.x,   wall.bottom, wall.line.end.y}),   wall.UEnd,   vBottom },
        { cam->toCamSpace({wall.line.start.x, wall.bottom, wall.line.start.y}), wall.UStart, vBottom },
    };

    ClipVertex clipped[MAX_CLIP_VERTS];
    int count = clipVertNear(corners, 4, cam->getNear(), clipped);
    if (count < 3) return;

    ScreenVertex screen[MAX_CLIP_VERTS];
    for (int i = 0; i < count; i++)
    {
        Vector2 p = cam->projectCamSpace(clipped[i].pos);
        float invZ = 1.0f / clipped[i].pos.z;
        screen[i] = {p.x, p.y, invZ, clipped[i].u * invZ, clipped[i].v * invZ};
    }

    fillTexturedPolygon(buf, std::span<const ScreenVertex>(screen, count), tex, shade);
}
