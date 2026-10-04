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

    bool cullWall(Vector2 p) const { return line.sideOf(p) <= 0; }
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

    Wall a = w;
    Wall b = w;
    a.line.end = p;
    b.line.start = p;
    if (sa > 0) { front = a; back = b; }
    else { front = b; back = a; }
}
