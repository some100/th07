#include "utils.hpp"

#include "ZunMath.hpp"

// FUNCTION: TH07 0x00431930
f32 utils::AddNormalizeAngle(f32 param_1, f32 param_2)
{
    i32 local_8;

    local_8 = 0;
    param_1 += param_2;
    while (param_1 > ZUN_PI)
    {
        param_1 -= ZUN_2PI;
        if (local_8++ > 16)
        {
            break;
        }
    }
    while (param_1 < -ZUN_PI)
    {
        param_1 += ZUN_2PI;
        if (local_8++ > 16)
        {
            break;
        }
    }
    return param_1;
}

// FUNCTION: TH07 0x004319b0
void utils::Rotate(D3DXVECTOR3 *out, D3DXVECTOR3 *point, f32 angle)
{
    f32 sinAngle;
    f32 cosAngle;

    sinAngle = sinf(angle);
    cosAngle = cosf(angle);
    out->x = cosAngle * point->x + sinAngle * point->y;
    out->y = cosAngle * point->y - sinAngle * point->x;
}
