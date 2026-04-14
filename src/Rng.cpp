#include "Rng.hpp"

// GLOBAL: TH07 0x0049fe20
Rng g_Rng;

// FUNCTION: TH07 0x00431870
u16 Rng::GetRandomU16()

{
  u16 uVar1;

  uVar1 = (this->seed ^ 0x9630) - 0x6553;
  this->seed = (((uVar1 & 0xc000) >> 14) + uVar1 * 4) & 0xFFFF;
  this->generationCount += 1;
  return this->seed;
}

// FUNCTION: TH07 0x004318d0
u32 Rng::GetRandomU32()

{
  return GetRandomU16() << 16 | GetRandomU16();
}

// FUNCTION: TH07 0x00431900
f32 Rng::GetRandomFloat()

{
  return (f32)GetRandomU32() / 4294967296.0f;
}
