#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <span>
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

inline void fillWall(Framebuffer* buf, Wall wall)
{
    Camera* cam = buf->activeCamera;
    Vector2 camXZ = { cam->getPos().x, cam->getPos().z };
    if (wall.cullWall(camXZ)) return;

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
