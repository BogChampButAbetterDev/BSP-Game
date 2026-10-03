#pragma once
#include "vectors.hpp"

struct Line
{
    Vector2 start;
    Vector2 end;

    float sideOf(Vector2 p) { return (end - start).cross(p - start); }
};

struct Wall
{
    Line line;
    float top = 0.0f;
    float bottom = 0.0f;

    bool cullWall(Vector2 p)
    {
        return line.sideOf(p) <= 0;
    }
};
