#pragma once
#include <cmath>
#include <algorithm>
#include "engine/math/geometry.hpp"
#include "framebuffer.hpp"
#include "engine/texture.hpp"
#include "globals/color.hpp"

struct WallSpan { float sx1, sx2, invZ1, invZ2, uZ1, uZ2; };

inline bool projectWallSpan(Camera* cam, const View& V, const Wall& w, WallSpan& out)
{
    Vector3 a = cam->toCamSpace({w.line.start.x, V.eyeY, w.line.start.y});
    Vector3 b = cam->toCamSpace({w.line.end.x,   V.eyeY, w.line.end.y});
    float au = w.UStart, bu = w.UEnd;
    const float n = V.nearZ;

    if (a.z < n && b.z < n) return false;
    if (a.z < n) { float t = (n - a.z) / (b.z - a.z); a = a + (b - a) * t; au += (bu - au) * t; }
    else if (b.z < n) { float t = (n - a.z) / (b.z - a.z); b = a + (b - a) * t; bu = au + (bu - au) * t; }

    float ia = 1.0f / a.z, ib = 1.0f / b.z;
    float sa = V.ox + V.f * a.x * ia;
    float sb = V.ox + V.f * b.x * ib;

    if (sa > sb) { std::swap(sa, sb); std::swap(ia, ib); std::swap(au, bu); }
    if (sb - sa < 1e-4f) return false;

    out = { sa, sb, ia, ib, au * ia, bu * ib };
    return true;
}
