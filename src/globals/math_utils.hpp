#pragma once

const double PI = 3.14159;
inline float deg2rad(float angle)
{
    return angle * (PI / 180);
}

// 0xAABBGGRR
constexpr uint32_t rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return 0xFF000000u | (uint32_t(b) << 16) | (uint32_t(g) << 8) | r;
}
