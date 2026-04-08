#include "utils.hpp"

#include "ZunMath.hpp"

// FUNCTION: TH07 0x00431930
f32 utils::AddNormalizeAngle(f32 param_1, f32 param_2)

{
  bool bVar1;
  i32 iVar2;
  i32 local_8;

  local_8 = 0;
  param_1 = param_1 + param_2;
  do {
    if (param_1 <= ZUN_PI)
      break;
    param_1 = param_1 - ZUN_2PI;
    iVar2 = local_8 + 1;
    bVar1 = local_8 < 0x11;
    local_8 = iVar2;
  } while (bVar1);
  do {
    if (-ZUN_PI <= param_1) {
      return param_1;
    }
    param_1 = param_1 + ZUN_2PI;
    bVar1 = local_8 < 0x11;
    local_8 = local_8 + 1;
  } while (bVar1);
  return param_1;
}

// FUNCTION: TH07 0x004319b0
void utils::Rotate(D3DXVECTOR3 *out, D3DXVECTOR3 *point, f32 angle)

{
  f32 fVar1;
  f32 fVar2;

  fVar1 = sinf(angle);
  fVar2 = cosf(angle);
  out->x = fVar1 * point->y + fVar2 * point->x;
  out->y = fVar2 * point->y - fVar1 * point->x;
}
