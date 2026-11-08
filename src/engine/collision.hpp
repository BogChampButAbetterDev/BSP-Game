#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

#include "bsp/bsp.hpp"
#include "math/geometry.hpp"

struct CollisionParams
{
    float radius     = 0.4f;
    float height     = 1.9f; 
    float stepHeight = 0.5f;  
};

inline Vector2 closestPointOnSegment(const Line& s, Vector2 p)
{
    Vector2 ab = s.end - s.start;
    float len2 = ab.len_squared();
    float t = len2 > 0.0f ? std::clamp((p - s.start).dot(ab) / len2, 0.0f, 1.0f) : 0.0f;
    return s.start + ab * t;
}

struct IWorldQuery
{
    virtual ~IWorldQuery() = default;
    virtual void queryWalls(Vector2 center, float radius, std::vector<const Wall*>& out) const = 0;
};

class BSPWorld : public IWorldQuery
{
public:
    BSPWorld() {}
    explicit BSPWorld(const BSPTree* tree) : m_tree(tree) {}

    void queryWalls(Vector2 c, float r, std::vector<const Wall*>& out) const override
    {
        if (m_tree) visit(m_tree->root, c, r, out);
    }

private:
    void visit(int idx, Vector2 c, float r, std::vector<const Wall*>& out) const
    {
        if (idx < 0) return;
        const BSPNode& n = m_tree->nodes[idx];

        float d = n.splitter.signedDistance(c);
        if (std::abs(d) <= r)
            for (const Wall& w : n.coplanarWalls) out.push_back(&w);

        if (d > -r) visit(n.front, c, r, out);
        if (d <  r) visit(n.back,  c, r, out);
    }

    const BSPTree* m_tree = nullptr;
};

using BlockFn = std::function<bool(const Wall&, Vector2 pos)>;

inline Vector2 moveAndSlide(const IWorldQuery& world, const BlockFn& blocks, Vector2 pos, Vector2 vel, float radius)
{
    // Substep so a lag spike can't tunnel through a wall
    int steps = std::max(1, (int)std::ceil(vel.length() / (radius * 0.5f)));
    Vector2 stepVec = vel / (float)steps;

    std::vector<const Wall*> walls;
    for (int s = 0; s < steps; ++s)
    {
        pos += stepVec;

        for (int iter = 0; iter < 4; ++iter)
        {
            walls.clear();
            world.queryWalls(pos, radius, walls);

            bool hit = false;
            for (const Wall* w : walls)
            {
                if (!blocks(*w, pos)) continue;

                Vector2 cp = closestPointOnSegment(w->line, pos);
                Vector2 d = pos - cp;
                float dist2 = d.len_squared();
                if (dist2 >= radius * radius) continue;

                float dist = std::sqrt(dist2);
                Vector2 n = dist > 1e-6f ? d / dist : w->normal;
                pos += n * (radius - dist);
                hit = true;
            }
            if (!hit) break;
        }
    }
    return pos;
}

class CollisionWorld
{
public:
    CollisionWorld() {}
    CollisionWorld(const BSPTree* tree, const std::vector<Sector>* sectors)
        : m_bsp(tree), m_sectors(sectors) {}

    Vector2 move(Vector2 pos, Vector2 vel, float feetY, const CollisionParams& cp) const
    {
        BlockFn blocks = [&](const Wall& w, Vector2 p) { return wallBlocks(w, p, feetY, cp); };
        return moveAndSlide(m_bsp, blocks, pos, vel, cp.radius);
    }

    const Sector* sectorAt(Vector2 p) const
    {
        std::vector<const Wall*> walls;
        for (float r = 2.0f; r <= 256.0f; r *= 2.0f)
        {
            walls.clear();
            m_bsp.queryWalls(p, r, walls);

            const Wall* best = nullptr;
            float bestD2 = r * r;
            for (const Wall* w : walls)
            {
                float d2 = (p - closestPointOnSegment(w->line, p)).len_squared();
                if (d2 < bestD2) { bestD2 = d2; best = w; }
            }

            if (best)
            {
                int id = best->line.sideOf(p) > 0 ? best->frontSect : best->backSect;
                return id >= 0 ? &(*m_sectors)[id] : nullptr;
            }
        }
        return nullptr;
    }

private:
    bool wallBlocks(const Wall& w, Vector2 p, float feetY, const CollisionParams& cp) const
    {
        if (w.backSect < 0 || w.frontSect < 0) return true;      // solid wall

        bool onFront = w.line.sideOf(p) > 0;
        const Sector& dest = (*m_sectors)[onFront ? w.backSect : w.frontSect];
        const Sector& src  = (*m_sectors)[onFront ? w.frontSect : w.backSect];

        float gap = dest.ceilingHt - std::max(dest.floorHt, src.floorHt);
        if (gap < cp.height) return true;                        // too low to fit
        if (dest.floorHt - feetY > cp.stepHeight) return true;   // ledge too high
        return false;                                            // open portal
    }

    BSPWorld m_bsp;
    const std::vector<Sector>* m_sectors = nullptr;
};
