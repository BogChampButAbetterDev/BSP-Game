#pragma once
#include <algorithm>
#include <limits>
#include <vector>

#include "engine/collision.hpp"

#include "math/vectors.hpp"
#include "math/geometry.hpp"

#include "globals/build_info.hpp"

inline constexpr float DEBUG_TRACE_LIFETIME = 2.0f;

inline constexpr int MAX_STEPS = 256;

enum class HitType
{
    NONE,
    WALL,
    FLOOR,
    CEILING
};

struct Ray
{
    float maxDist = 0.0f;
    Vector3 dir  = { 0.0f, 0.0f, 0.0f };
    Vector3 origin = { 0.0f, 0.0f, 0.0f };

    bool debugVis = false;
};

struct RayHit
{
    HitType type = HitType::NONE;
    Vector3 point{};
    float distance = 0.0f;
    int sector = -1;
    int wall = -1;
    explicit operator bool() const { return type != HitType::NONE && type != HitType::CEILING; }
};

inline std::vector<std::vector<int>> buildSectorWalls(const std::vector<Wall>& walls, size_t sectorCount)
{
    std::vector<std::vector<int>> out(sectorCount);
    for (int i = 0; i < (int)walls.size(); i++)
    {
        const Wall& w = walls[i];
        if (w.frontSect >= 0) out[w.frontSect].push_back(i);
        if (w.backSect >= 0 && w.backSect != w.frontSect) out[w.backSect].push_back(i);
    }
    return out; 
}

inline RayHit cast(const std::vector<Wall> &walls, const std::vector<Sector> &sectors, const std::vector<std::vector<int>> &sectorWalls, const Ray &ray, int startSector)
{
    if (startSector < 0) return {};

    ray.dir.normalized();

    Vector3 o = ray.origin;
    Vector3 d = ray.dir.normalized();
    float len = ray.maxDist;

    float tCur = 0;
    int skipWall = -1;

    Line flat = Line({o.x, o.z}, {o.x + d.x * len, o.z + d.z * len});

    auto makeHit = [&](HitType hit, float t, int sector, int wall) -> RayHit
    {
        RayHit out;
        out.type = hit;
        out.point = {o.x + d.x * (len * t), o.y + d.y * (len * t), o.z + d.z * (len * t)};
        out.distance = t * len;
        out.sector = sector;
        out.wall = wall;
        return out;
    };

    auto posAt = [&](float t) { return Vector2(o.x + d.x * (len * t), o.z + d.x * (len * t)); };
    auto yAt = [&](float t) -> float { return o.y + d.y * (len * t); };

    int sec = startSector;

    for (int step = 0; step < MAX_STEPS; step++)
    {
        float tWall = 1.0f;
        int hitWall = -1;

        for (int wIdx : sectorWalls[sec])
        {
            if (wIdx == skipWall) continue;
            float t, u;
            if (flat.intersect(walls[wIdx].line, t, u))
            {
                if (t > 0 && t < tWall)
                {
                    tWall = t;
                    hitWall = wIdx;
                }
            }
        }

        auto floorGap = [&](float t) -> float { return yAt(t) - sectors[sec].floorAt(posAt(t)); };
        auto ceilGap = [&](float t) -> float { return sectors[sec].ceilAt(posAt(t)) - yAt(t); }; // (-) up

        float tFloor = std::numeric_limits<float>::infinity();
        float f0 = floorGap(tCur);
        float f1 = floorGap(tWall);
        if (f0 >= 0 && f1 < 0)
            tFloor = tCur + (tWall - tCur) * f0 / (f0 - f1);

        float tCeil = std::numeric_limits<float>::infinity();
        float c0 = ceilGap(tCur);
        float c1 = ceilGap(tWall);
        if (c0 >= 0 && c1 < 0)
            tCeil = tCur + (tWall - tCur) * c0 / (c0 - c1);

        if (tFloor <= tCeil && tFloor <= tWall)
            return makeHit(HitType::FLOOR, tFloor, sec, -1);
        if (tCeil < tFloor && tCeil <= tWall)
        {
            if (sectors[sec].sky)
                return {};
            return makeHit(HitType::CEILING, tCeil, sec, -1);
        }
        
        if (hitWall == 0) return {};
        if (walls[hitWall].backSect < 0) return makeHit(HitType::WALL, tWall, sec, hitWall);

        int other = (walls[hitWall].frontSect == sec) ? walls[hitWall].backSect : walls[hitWall].frontSect;
        Vector2 posXZ = posAt(tWall);
        float y = yAt(tWall);
        
        float lo  = std::max(sectors[sec].floorAt(posXZ), sectors[other].floorAt(posXZ));
        float hi = std::min(sectors[sec].ceilAt(posXZ), sectors[other].ceilAt(posXZ));

        bool skyGap = sectors[sec].sky && sectors[other].sky and y > hi;
        if ((y >= lo && y <= hi) || skyGap)
        {
            sec = other;
            skipWall = hitWall;
            tCur = tWall;
            continue;
        }
        else
        {
            return makeHit(HitType::WALL, tWall, sec, hitWall);
        }
    }

    return {};
}

class RayWorld
{
public:
    RayWorld() {}

    void init(const std::vector<Wall> *walls, const std::vector<Sector> *sectors, const CollisionWorld *col)
    {
        m_walls = walls;
        m_sectors = sectors;
        m_col = col;
        m_sectorWalls = buildSectorWalls(*walls, sectors->size());
    }

    RayHit castRay(const Ray& ray) const 
    {
        if (!m_walls) return {};
        const Sector* s = m_col->sectorAt({ray.origin.x, ray.origin.z});
        int sec = s ? (int)(s - m_sectors->data()) : -1;
        return cast(*m_walls, *m_sectors, m_sectorWalls, ray, sec);
    }

private:
    const std::vector<Wall>* m_walls = nullptr;
    const std::vector<Sector>* m_sectors = nullptr;
    const CollisionWorld* m_col = nullptr;
    std::vector<std::vector<int>> m_sectorWalls;
};

struct DebugTrace
{
    Vector3 start;
    Vector3 end;
    Vector3 hitPoint;
    bool hit = false;
    HitType type = HitType::NONE;
    float life = IS_DEBUG ? DEBUG_TRACE_LIFETIME : 0.0f;
};

inline DebugTrace makeTrace(const Ray& ray, const RayHit& hit)
{
    if constexpr (IS_DEBUG)
    {
        Vector3 d = ray.dir.normalized();
        DebugTrace t;
        t.start = ray.origin;
        t.end = ray.origin + d * ray.maxDist;
        t.hit = (bool)hit;
        t.type = hit.type;
        t.hitPoint = hit.point;
        return t;
    }

    return {};
}
