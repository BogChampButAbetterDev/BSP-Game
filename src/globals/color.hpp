#pragma once
#include <cstdint>
#include <algorithm>

struct Color
{
    uint8_t r = 0;  
    uint8_t g = 0; 
    uint8_t b = 0;   
};

// 0xAABBGGRR
constexpr uint32_t rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return 0xFF000000u | (uint32_t(b) << 16) | (uint32_t(g) << 8) | r;
}

constexpr Color unpackRGB(uint32_t color)
{
    Color out;
    out.r = (uint8_t)(color & 0x000000FFu);
    out.g = (uint8_t)((color >> 8) & 0xFFu);
    out.b = (uint8_t)((color >> 16) & 0xFFu);
    return out;
}

inline uint32_t debugColor(int i)
{
    return rgb(60 + (i * 37) % 195, 60 + (i * 71) % 195, 60 + (i * 113) % 195);
}

// slow
inline uint32_t scaleColor(uint32_t color, float s)
{
    Color c = unpackRGB(color);

    uint8_t r = (uint8_t)std::clamp(c.r * s, 0.0f, 255.0f);
    uint8_t g = (uint8_t)std::clamp(c.g * s, 0.0f, 255.0f);
    uint8_t b = (uint8_t)std::clamp(c.b * s, 0.0f, 255.0f);

    return rgb(r, g, b);
}

inline uint32_t shadeToFixed(float s)
{
    return (uint32_t)(std::clamp(s, 0.0f, 1.0f) * 256.0f + 0.5f);
}

inline uint32_t scaleColorFixed(uint32_t c, uint32_t shade256)
{
    uint32_t rb = (((c & 0x00FF00FFu) * shade256) >> 8) & 0x00FF00FFu;
    uint32_t g = (((c & 0x0000FF00u) * shade256) >> 8) & 0x0000FF00u;
    return (c & 0xFF000000) | rb | g;
}
