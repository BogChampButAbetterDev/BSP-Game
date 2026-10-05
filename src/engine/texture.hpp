#pragma once
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdint>
#include <SDL3/SDL.h>

#include "globals/color.hpp"

struct Texture
{
    int width = 0;
    int height = 0;

    std::vector<uint32_t> pixels;

    uint32_t sample(float u, float v) const 
    {
        float fu = u - std::floor(u);
        float fv = v - std::floor(v);

        int x = std::min((int)(fu * width), width - 1);
        int y = std::min((int)(fv * height), height - 1);

        return pixels[y * width + x];
    }

    void genChecker()
    {
        uint32_t colorA = rgb(160, 0, 107);
        uint32_t colorB = rgb(0, 0, 0);

        if (pixels.size() == 0) return;
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                bool even = ((x / 8) + (y / 8)) % 2 == 0;
                pixels[y * width + x] = even ? colorA : colorB;
            }
        }
    }
};
