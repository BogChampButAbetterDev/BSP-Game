#pragma once
#include "vectors.hpp"

struct Line
{
    Vector2 start;
    Vector2 end;
};

struct Wall
{
    Line line;
    float top = 0.0f;
    float bottom = 0.0f;
};
