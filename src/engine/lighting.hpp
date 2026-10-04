#pragma once
#include <iostream>
#include <algorithm>

#include "math/geometry.hpp"
#include "globals/color.hpp"

struct DirectionalLight
{
    Vector3 dir;
    float ambient = 0.35;
    float maxDist = 30;
};

inline float shade(Vector2 normal, DirectionalLight light)
{
    float dot = (normal.x * light.dir.x) + (normal.y * light.dir.z);
    float wrapped = dot * 0.5f + 0.5f;
    return light.ambient + (1 - light.ambient) * wrapped;
}

inline uint32_t applyLighting(uint32_t baseColor, const DirectionalLight& light, const Wall& wall)
{
    Color base = unpackRGB(baseColor);
    float s = shade(wall.normal, light);
    uint8_t r = (uint8_t)std::min(255.0f, base.r * s);
    uint8_t g = (uint8_t)std::min(255.0f, base.g * s);
    uint8_t b = (uint8_t)std::min(255.0f, base.b * s);
    return rgb(r, g, b);
}
