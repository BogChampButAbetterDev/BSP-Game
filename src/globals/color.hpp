#pragma once
#include <cstdint>

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
    out.r = (uint8_t)(color & 0x000000FF);
    out.g = (uint8_t)((color >> 8) & 0xFF);
    out.b = (uint8_t)((color >> 16) & 0xFF);
    return out;
}

inline uint32_t debugColor(int i)
{
    return rgb(60 + (i * 37) % 195, 60 + (i * 71) % 195, 60 + (i * 113) % 195);
}
