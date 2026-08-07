#pragma once

#include <d3d8.h>
#include <d3dx8math.h>

#include "inttypes.hpp"

#define ZUN_PI ((f32)(3.14159265358979323846))
#define ZUN_2PI ((f32)(ZUN_PI * 2.0f))

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

struct PodFloat3
{
    f32 x;
    f32 y;
    f32 z;
};

struct Float3
{
    Float3()
    {
    }

    Float3(f32 x, f32 y, f32 z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    void FromAngleMagnitude(f32 angle, f32 magnitude);

    operator f32 *()
    {
        return &x;
    }

    Float3 *operator+=(const Float3 &other)
    {
        this->x += other.x;
        this->y += other.y;
        this->z += other.z;
        return this;
    }

    Float3 *operator-=(const Float3 &other)
    {
        this->x -= other.x;
        this->y -= other.y;
        this->z -= other.z;
        return this;
    }

    Float3 *operator*=(f32 s)
    {
        this->x *= s;
        this->y *= s;
        this->z *= s;
        return this;
    }

    Float3 operator-() const
    {
        return Float3(-x, -y, -z);
    }

    Float3 operator+(const Float3 &other) const
    {
        return Float3(
            x + other.x,
            y + other.y,
            z + other.z);
    }

    Float3 operator-(const Float3 &other) const
    {
        return Float3(
            x - other.x,
            y - other.y,
            z - other.z);
    }

    Float3 operator*(f32 s) const
    {
        return Float3(
            x * s,
            y * s,
            z * s);
    }

    Float3 operator/(f32 s) const
    {
        f32 inv = 1.0f / s;
        return Float3(
            x * inv,
            y * inv,
            z * inv);
    }

    friend Float3 operator*(f32 s, const Float3 &v)
    {
        return Float3(
            s * v.x,
            s * v.y,
            s * v.z);
    }

    D3DXVECTOR3 *asD3DX()
    {
        return (D3DXVECTOR3 *)this;
    }

    f32 x;
    f32 y;
    f32 z;
};

struct Float2
{
    f32 x;
    f32 y;
};
