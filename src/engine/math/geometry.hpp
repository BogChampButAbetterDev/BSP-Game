#pragma once
#include "vectors.hpp"

const float eps = 0.001;

struct Line
{
    Vector2 start;
    Vector2 end;

    float length() const { return (end - start).length(); }
    float sideOf(Vector2 p) const  { return (end - start).cross(p - start); }

    float signedDistance(Vector2 p) const 
    {
        return sideOf(p) / length();
    }
};

struct Wall
{
    Line line;
    float top = 0.0f;
    float bottom = 0.0f;

    Vector2 normal;

    int texId = -1;

    float UStart = 0.0f;
    float UEnd = 1.0f;

    bool cullWall(Vector2 p) const { return line.sideOf(p) <= 0; }
    void computeNormal() 
    {
        Vector2 d = line.end - line.start;
        Vector2 perp = {-d.y, d.x};
        normal = perp.normalized();
    }
};

struct ClipVertex
{
    Vector3 pos;
    float u = 0.0f;
    float v = 0.0f;

    ClipVertex lerp(const ClipVertex& b, float t)
    {
        return { pos + (b.pos - pos) * t,
                u   + (b.u   - u)   * t,
                v   + (b.v   - v)   * t };
    }
};

struct ScreenVertex
{
    float x = 0.0f;
    float y = 0.0f;
    float invZ = 0.0f;
    float uOverZ = 0.0f;
    float vOverZ = 0.0f;

    ScreenVertex lerp(const ScreenVertex& b, float t) const 
    {
        return { x      + (b.x      - x)      * t,
                 y      + (b.y      - y)      * t,
                 invZ   + (b.invZ   - invZ)   * t,
                 uOverZ + (b.uOverZ - uOverZ) * t,
                 vOverZ + (b.vOverZ - vOverZ) * t };
    }
};

enum class Side {FRONT, BACK, COPLANAR, SPANNING};
inline Side classifyWall(const Line& splitter, const Wall& w)
{
    float dStart = splitter.signedDistance(w.line.start);
    float dEnd = splitter.signedDistance(w.line.end);

    bool aFront = dStart > eps; bool aBack = dStart < -eps;
    bool bFront = dEnd > eps; bool bBack = dEnd < -eps;

    if ((!aFront && !bFront) && (!aBack && !bBack)) return Side::COPLANAR;
    if ((aFront && bBack) || (aBack && bFront)) return Side::SPANNING;
    if (aBack || bBack) return Side::BACK;
    return Side::FRONT;
}

inline void splitWall(const Wall& w, const Line& sp, Wall& front, Wall& back)
{
    float sa = sp.sideOf(w.line.start);
    float sb = sp.sideOf(w.line.end);

    float t = sa / (sa - sb);
    Vector2 p = w.line.start + (w.line.end - w.line.start) * t;
    float USplit = w.UStart + (w.UEnd - w.UStart) * t;

    Wall a = w;
    Wall b = w;

    a.line.end = p;
    a.UEnd = USplit;

    b.line.start = p;
    b.UStart = USplit;

    if (sa > 0) { front = a; back = b; }
    else { front = b; back = a; }
}
