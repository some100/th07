#pragma once

#include <d3d8.h>

#include "inttypes.hpp"

union ZunColor {
    D3DCOLOR color;
    struct ColorBytes
    {
        u8 b;
        u8 g;
        u8 r;
        u8 a;
    } bytes;
    u8 raw[4];
};
