#include "Rng.hpp"

Rng g_Rng;

u16 Rng::GetRandomU16()

{
  u16 uVar1;

  uVar1 = (this->seed ^ 0x9630) + 0x9aad;
  this->seed = ((i32)(uVar1 & 0xc000) >> 14) + uVar1 * 4;
  this->generationCount += 1;
  return this->seed;
}

u32 Rng::GetRandomU32()

{
  return GetRandomU16() << 16 | GetRandomU16();
}

f32 Rng::GetRandomFloat()

{
  return (f32)GetRandomU32() / 4294967296.0f;
}
