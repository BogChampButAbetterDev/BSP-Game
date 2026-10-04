#pragma once
#include <cmath>
#include <vector>

#include "framebuffer.hpp"
#include "rasterizer.hpp"

#include "engine/camera.hpp"
#include "engine/math/geometry.hpp"
#include "engine/math/vectors.hpp"

#include "globals/globals.hpp"
#include "globals/math_utils.hpp"

// Top-down 2D view of a list of walls, centered on the camera.
// World (x, z) -> screen. Wall lines store world z in Vector2::y.
namespace DebugView
{
    constexpr float SCALE       = 12.0f;   // pixels per world unit
    constexpr float TICK_LEN    = 0.75f;   // length of the front-side tick, in world units
    constexpr float HEADING_LEN = 2.0f;    // length of the camera heading line, in world units

    // world.x = world x, world.y = world z
    inline Vector2 worldToScreen(Vector2 world, Vector3 cam)
    {
        return
        {
            HALF_WID + (world.x - cam.x) * SCALE,
            HALF_HT  - (world.y - cam.z) * SCALE   // minus: screen y grows down, world z grows up
        };
    }

    inline void drawWalls(Framebuffer* buf, const std::vector<Wall>& walls)
    {
        Vector3 camPos = buf->activeCamera->getPos();

        for (size_t i = 0; i < walls.size(); i++)
        {
            const Line& l = walls[i].line;

            buf->setDrawColor(debugColor((int)i));
            drawLine(buf, worldToScreen(l.start, camPos), worldToScreen(l.end, camPos));

            // Front-side tick: left-hand perpendicular of the wall direction,
            // the same side sideOf() reports as positive. Built in world space,
            // then converted, so the y flip can't confuse it.
            Vector2 dir  = (l.end - l.start).normalized();
            Vector2 perp = {-dir.y, dir.x};
            Vector2 mid  = (l.start + l.end) * 0.5f;

            drawLine(buf, worldToScreen(mid, camPos),
                          worldToScreen(mid + perp * TICK_LEN, camPos));
        }
    }

    inline void drawCamera(Framebuffer* buf)
    {
        Camera* cam = buf->activeCamera;
        Vector3 camPos = cam->getPos();

        float yaw = deg2rad(cam->getYaw());
        Vector2 camXZ = {camPos.x, camPos.z};
        Vector2 headingEnd = {camPos.x + sinf(yaw) * HEADING_LEN,
                              camPos.z + cosf(yaw) * HEADING_LEN};

        buf->setDrawColor(rgb(255, 60, 60));

        Vector2 c = worldToScreen(camXZ, camPos);
        for (int dy = -2; dy <= 2; dy++)
        {
            for (int dx = -2; dx <= 2; dx++)
            {
                buf->setPixel((int)c.x + dx, (int)c.y + dy);
            }
        }

        drawLine(buf, c, worldToScreen(headingEnd, camPos));
    }

    inline void render(Framebuffer* buf, const std::vector<Wall>& walls)
    {
        drawWalls(buf, walls);
        drawCamera(buf);
    }
}
