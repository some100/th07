#pragma once

#include <d3d8.h>
#include <d3dx8math.h>

#include "inttypes.hpp"

#define ZUN_PI ((f32)(3.14159265358979323846))
#define ZUN_2PI ((f32)(ZUN_PI * 2.0f))
#define ZUN_3PI ((f32)(ZUN_PI * 3.0f))

#define sincosf_macro(outSin, outCos, angle) \
    {                                        \
        __asm { \
    __asm fld angle \
    __asm fsincos \
    __asm fstp outCos \
    __asm fstp outSin                            \
        }                                    \
    }

// FUNCTION: TH07 0x00417af0
inline void sincosf(f32 *outSin, f32 *outCos, f32 angle)
{
    __asm {
          fld [angle]
          fsincos
          mov eax, [outCos]
          fstp float ptr [eax]
          mov eax, [outSin]
          fstp float ptr [eax]
    }
}
