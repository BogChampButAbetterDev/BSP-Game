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

struct View
{
    float ox, oy, f;
    float eyeY, camX, camZ;
    float cy, sy;
    float nearZ;
};

inline View makeView(Camera* cam)
{
    Vector3 p = cam->getPos();
    return { (float)HALF_WID, cam->getHorizon(), cam->getFocal(),
             p.y, p.x, p.z, cam->getCosYaw(), cam->getSinYaw(), cam->getNear() };
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

inline bool clipSegmentNear(Vector3& a, Vector3& b, float nearZ)
{
    float da = a.z - nearZ;
    float db = b.z - nearZ;

    if (da < 0 && db < 0) return false;

    if (da < 0) a = a + (b - a) * (da / (da - db)); 
    else if (db < 0) b = a + (b - a) * (da / (da - db));

    return true;
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
                if (!buf->isCovered(x, y)) buf->plotFree(x, y, buf->current_color);
            }
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

inline void fillWallColumn(Framebuffer* buf, const View& V, int x, int y0, int y1, float invZ, float u, const Texture& tex, float shade)
{
    if (y0 > y1) return;
    float hStep = 1.0f / (V.f * invZ);
    float h  = V.eyeY + (V.oy - (y0 + 0.5f)) * hStep; 
    float v  = -h / TEX_WORLD_H;
    float dv = hStep / TEX_WORLD_H;
    for (int y = y0; y <= y1; y++, v += dv)
        buf->setPixelColor(x, y, scaleColor(tex.sample(u, v), shade));
}

inline void fillFlatColumn(Framebuffer* buf, const View& V, int x, int y0, int y1, float planeH, const Texture& tex, float shade)
{
    if (y0 > y1) return;
    float dy = planeH - V.eyeY;
    float xn = (x + 0.5f - V.ox) / V.f;

    for (int y = y0; y <= y1; y++)
    {
        float d = V.oy - (y + 0.5f);
        if (d == 0.0f) continue;
        float z  = V.f * dy / d;
        float xc = xn * z;
        float wx = V.camX + xc * V.cy + z * V.sy;
        float wz = V.camZ - xc * V.sy + z * V.cy;
        buf->setPixelColor(x, y, scaleColor(tex.sample(wx / TEX_WORLD_H, wz / TEX_WORLD_H), shade));
    }
}
