#pragma once
#include <SDL3/SDL.h>
#include <algorithm>

#include "framebuffer.hpp"
#include "globals/globals.hpp"
#include "engine/math/vectors.hpp"
#include "engine/math/geometry.hpp"

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

inline void fillConvexPolygon(Framebuffer* buf, const std::vector<Vector2>& verts)
{
    if (verts.size() < 3) return;

    float minYf = verts[0].y, maxYf = verts[0].y;
    for (auto& v : verts) { minYf = std::min(minYf, v.y); maxYf = std::max(maxYf, v.y); }

    int yStart = std::max(0, (int)std::floor(minYf));
    int yEnd   = std::min(DEF_WIN_HEIGHT - 1, (int)std::ceil(maxYf));

    int n = (int)verts.size();

    for (int y = yStart; y <= yEnd; y++)
    {
        float yf = y + 0.5f; // sample at pixel center
        std::vector<float> xs;

        for (int i = 0; i < n; i++)
        {
            Vector2 a = verts[i];
            Vector2 b = verts[(i + 1) % n];

            if (a.y == b.y) continue; 

            if ((yf >= a.y && yf < b.y) || (yf >= b.y && yf < a.y))
            {
                float t = (yf - a.y) / (b.y - a.y);
                xs.push_back(a.x + t * (b.x - a.x));
            }
        }

        std::sort(xs.begin(), xs.end());

        for (size_t i = 0; i + 1 < xs.size(); i += 2)
        {
            int xStart = std::max(0, (int)std::ceil(xs[i] - 0.5f));
            int xEnd   = std::min(DEF_WIN_WIDTH - 1, (int)std::floor(xs[i + 1] - 0.5f));

            for (int x = xStart; x <= xEnd; x++)
            {
                buf->setPixel(x, y);
            }
        }
    }
}

inline void fillWall(Framebuffer* buf, Wall wall)
{
    Camera* cam = buf->activeCamera;

    Vector3 topStart = {wall.line.start.x, wall.top, wall.line.start.y};
    Vector3 topEnd = {wall.line.end.x, wall.top, wall.line.end.y};
    Vector3 bottomStart = {wall.line.start.x, wall.bottom, wall.line.start.y};
    Vector3 bottomEnd = {wall.line.end.x, wall.bottom, wall.line.end.y};

    const float nearPlane = 0.01f;

    if (cam->viewDepth(topStart) < nearPlane    || 
        cam->viewDepth(topEnd) < nearPlane      ||
        cam->viewDepth(bottomStart) < nearPlane ||
        cam->viewDepth(bottomEnd) < nearPlane)
    {
        return;
    }

    Vector2 aTop = cam->projectPoint(topStart);
    Vector2 aBottom = cam->projectPoint(bottomStart);
    Vector2 bTop = cam->projectPoint(topEnd);
    Vector2 bBottom = cam->projectPoint(bottomEnd);

    fillConvexPolygon(buf, {aTop, bTop, bBottom, aBottom});
}
