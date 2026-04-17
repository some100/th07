#pragma once

#include "inttypes.hpp"

struct Rng
{
    f32 GetRandomFloat();
    u16 GetRandomU16();
    u32 GetRandomU32();

    u16 GetRandomU16InRange(u16 range)
    {
        return range != 0 ? this->GetRandomU16() % range : 0;
    }

    u32 GetRandomU32InRange(u32 range)
    {
        return range != 0 ? this->GetRandomU32() % range : 0;
    }

    f32 GetRandomFloatInRange(f32 range)
    {
        return this->GetRandomFloat() * range;
    }

    u16 seed;
    u16 seedBackup;
    u32 generationCount;
};

extern Rng g_Rng;
