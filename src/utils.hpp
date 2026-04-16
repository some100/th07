#pragma once

#include <d3dx8math.h>

#include "inttypes.hpp"

union AnyArg {
    i32 i;
    u32 u;
    f32 f;
    i16 s[2];
    u16 us[2];
    i8 c[4];
    u8 b[4];
};

namespace utils
{
f32 AddNormalizeAngle(f32 a, f32 b);
void Rotate(D3DXVECTOR3 *out, D3DXVECTOR3 *point, f32 angle);
} // namespace utils
