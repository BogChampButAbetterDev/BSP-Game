#pragma once
#include <algorithm>
#include <cmath>

#include "engine/texture.hpp"
#include "engine/math/vectors.hpp"

const int NUM_FACES = 6;

struct SkyBox
{
    Texture faces[NUM_FACES];
    Texture pano;
    bool usePano = false;
    bool valid = false;

    uint32_t sample(Vector3 d) const
    {
        if (!valid)
        {
            float t = std::clamp(d.normalized().y, 0.0f, 1.0f);
            return rgb((uint8_t)(90 + 60 * t), (uint8_t)(140 + 60 * t), 235);
        }
        if (usePano)
        {
            Vector3 n = d.normalized();
            float u = std::atan2(n.x, n.z) / (2.0f * 3.14159265f) + 0.5f;
            float v = 0.5f - std::asin(std::clamp(n.y, -1.0f, 1.0f)) / 3.14159265f;
            return pano.sample(u, v);
        }

        float ax = std::abs(d.x), ay = std::abs(d.y), az = std::abs(d.z);
        int f; float ma, u, v;
        if (ax >= ay && ax >= az) { f = d.x > 0 ? 0 : 1; ma = ax; u = d.x > 0 ? -d.z :  d.z; v = -d.y; }
        else if (ay >= az)        { f = d.y > 0 ? 2 : 3; ma = ay; u = d.x; v = d.y > 0 ? d.z : -d.z; }
        else                      { f = d.z > 0 ? 4 : 5; ma = az; u = d.z > 0 ? d.x : -d.x; v = -d.y; }
        return faces[f].sample((u / ma + 1.0f) * 0.5f, (v / ma + 1.0f) * 0.5f);
    }
};
