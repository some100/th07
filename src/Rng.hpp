#pragma once

#include "inttypes.hpp"

struct Rng
{
    f32 GetRandomFloat();
    u16 GetRandomU16();
    u32 GetRandomU32();

    u16 seed;
    u16 seedBackup;
    u32 generationCount;
};

extern Rng g_Rng;
