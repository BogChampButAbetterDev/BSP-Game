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

inline std::vector<Vector3> clipNear(const std::vector<Vector3>& in, float nearZ)
{
    std::vector<Vector3> out;
    for (size_t i = 0; i < in.size(); i++)
    {
        Vector3 a = in[i];
        Vector3 b = in[(i + 1) % in.size()];
        float da = a.z - nearZ;
        float db = b.z - nearZ;
        if (da >= 0) out.push_back(a);
        if ((da >= 0) != (db >= 0))
        {
            out.push_back(a + (b - a) * (da / (da - db)));
        }
    }
    return out;
}

inline std::vector<ClipVertex> clipVertNear(const std::vector<ClipVertex>& in, float nearZ)
{
    std::vector<ClipVertex> out;
    for (size_t i = 0; i < in.size(); i++)
    {
        ClipVertex a = in[i];
        ClipVertex b = in[(i + 1) % in.size()];
        float da = a.pos.z - nearZ;
        float db = b.pos.z - nearZ;
        if (da >= 0) out.push_back(a);
        if ((da >= 0) != (db >= 0))
        {
            out.push_back(a.lerp(b, (da / (da - db))));
        }
    }
    return out;
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
                buf->setPixel(x, y);
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
            float s = (x + 0.5f - L.x) / spanW;
            ScreenVertex p = L.lerp(R, s);

            float u = p.uOverZ / p.invZ;
            float v = p.vOverZ / p.invZ;

            buf->setPixelColor(x, y, scaleColor(tex.sample(u, v), shade));
        }
    }
}

inline void fillWall(Framebuffer* buf, Wall wall)
{
    Camera* cam = buf->activeCamera;

    Vector3 c_topStart = cam->toCamSpace({wall.line.start.x, wall.top, wall.line.start.y});
    Vector3 c_topEnd = cam->toCamSpace({wall.line.end.x, wall.top, wall.line.end.y});
    Vector3 c_bottomStart = cam->toCamSpace({wall.line.start.x, wall.bottom, wall.line.start.y});
    Vector3 c_bottomEnd = cam->toCamSpace({wall.line.end.x, wall.bottom, wall.line.end.y});

    const float nearPlane = cam->getNear();

    std::vector<Vector3> verts = clipNear({c_topStart, c_topEnd, c_bottomEnd, c_bottomStart}, nearPlane);
    if (verts.size() < 3) return;
    std::array<Vector2, 8> vertsProj;
    for (size_t i = 0; i < verts.size(); i++)
    {  
        vertsProj[i] = cam->projectCamSpace(verts[i]);
    }

    fillConvexPolygon(buf, std::span<const Vector2>(vertsProj.data(), verts.size()));
}

inline void fillWallTextured(Framebuffer* buf, Wall wall, const Texture& tex, float shade)
{
    Camera* cam = buf->activeCamera;
    float vBottom = (wall.top - wall.bottom) / TEX_WORLD_H;

    std::vector<ClipVertex> corners =
    {
        { cam->toCamSpace({wall.line.start.x, wall.top,    wall.line.start.y}), wall.UStart, 0.0f    },
        { cam->toCamSpace({wall.line.end.x,   wall.top,    wall.line.end.y}),   wall.UEnd,   0.0f    },
        { cam->toCamSpace({wall.line.end.x,   wall.bottom, wall.line.end.y}),   wall.UEnd,   vBottom },
        { cam->toCamSpace({wall.line.start.x, wall.bottom, wall.line.start.y}), wall.UStart, vBottom },
    };

    std::vector<ClipVertex> clipped = clipVertNear(corners, cam->getNear());

    if (clipped.size() < 3) return;
    std::vector<ScreenVertex> screen;
    screen.reserve(clipped.size());

    for (const ClipVertex& c : clipped)
    {
        Vector2 p = cam->projectCamSpace(c.pos);
        float invZ = 1.0f / c.pos.z;
        screen.push_back({p.x, p.y, invZ, c.u * invZ, c.v * invZ});
    }

    fillTexturedPolygon(buf, screen, tex, shade);
}
