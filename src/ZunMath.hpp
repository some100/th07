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

struct ZunVec2
{
    f32 x;
    f32 y;

    __forceinline ZunVec2()
    {
        x = 0.0f;
        y = 0.0f;
    }

    __forceinline ZunVec2(f32 x, f32 y)
    {
        this->x = x;
        this->y = y;
    }

    __forceinline ZunVec2 operator+(const ZunVec2 &v)
    {
        return ZunVec2(x + v.x, y + v.y);
    }

    __forceinline ZunVec2 operator-(const ZunVec2 &v)
    {
        return ZunVec2(x - v.x, y - v.y);
    }

    __forceinline ZunVec2 operator*(const ZunVec2 &v)
    {
        return ZunVec2(v.x * x, v.y * y);
    }

    __forceinline ZunVec2 operator*(f32 f)
    {
        return ZunVec2(x * f, y * f);
    }
};
C_ASSERT(sizeof(ZunVec2) == 0x8);

__forceinline ZunVec2 operator*(f32 f, const ZunVec2 &v)
{
    return ZunVec2(v.x * f, v.y * f);
}

struct ZunVec3
{
    f32 x;
    f32 y;
    f32 z;

    ZunVec3()
    {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }

    ZunVec3(f32 x, f32 y, f32 z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    __forceinline D3DXVECTOR3 *asD3DX()
    {
        return (D3DXVECTOR3 *)this;
    }

    __forceinline ZunVec3 *Project(ZunVec3 *pV, D3DVIEWPORT8 *pViewport,
                                   struct ZunMatrix *pProjection,
                                   struct ZunMatrix *pView,
                                   struct ZunMatrix *pWorld);

    __forceinline ZunVec3 *Normalize(ZunVec3 *pV)
    {
        return (ZunVec3 *)D3DXVec3Normalize(this->asD3DX(), pV->asD3DX());
    }

    __forceinline ZunVec3 *Cross(ZunVec3 *pV1, ZunVec3 *pV2)
    {
        return (ZunVec3 *)D3DXVec3Cross(this->asD3DX(), pV1->asD3DX(),
                                        pV2->asD3DX());
    }

    __forceinline ZunVec3 *TransformCoord(ZunVec3 *pV, struct ZunMatrix *pM);

    __forceinline f32 Length()
    {
        return D3DXVec3Length(this->asD3DX());
    }

    __forceinline ZunVec3 operator-()
    {
        return ZunVec3(-x, -y, -z);
    }

    __forceinline ZunVec3 operator+(const ZunVec3 &v)
    {
        return ZunVec3(x + v.x, y + v.y, z + v.z);
    }

    __forceinline ZunVec3 operator-(const ZunVec3 &v)
    {
        return ZunVec3(x - v.x, y - v.y, z - v.z);
    }

    __forceinline ZunVec3 operator*(const ZunVec3 &v)
    {
        return ZunVec3(x * v.x, y * v.y, z * v.z);
    }

    __forceinline ZunVec3 operator*(f32 f)
    {
        return ZunVec3(x * f, y * f, z * f);
    }

    __forceinline void operator+=(const ZunVec3 &v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
    }

    __forceinline void operator-=(const ZunVec3 &v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
    }

    __forceinline void operator*=(const ZunVec3 &v)
    {
        x *= v.x;
        y *= v.y;
        z *= v.z;
    }

    __forceinline void operator=(f32 f)
    {
        x = f;
        y = f;
        z = f;
    }

    __forceinline void operator*=(f32 f)
    {
        x *= f;
        y *= f;
        z *= f;
    }
};
C_ASSERT(sizeof(ZunVec3) == 0xc);

__forceinline ZunVec3 operator*(f32 f, const ZunVec3 &v)
{
    return ZunVec3(v.x * f, v.y * f, v.z * f);
}

struct ZunQuaternion
{
    f32 x;
    f32 y;
    f32 z;
    f32 w;

    D3DXQUATERNION *asD3DX()
    {
        return (D3DXQUATERNION *)this;
    }
};
C_ASSERT(sizeof(ZunQuaternion) == 0x10);

struct ZunMatrix
{
    f32 m[4][4];

    D3DXMATRIX *asD3DX()
    {
        return (D3DXMATRIX *)this;
    }

    __forceinline void Identity()
    {
        D3DXMatrixIdentity((D3DXMATRIX *)this);
    }

    __forceinline void RotateX(f32 angle)
    {
        D3DXMatrixRotationX((D3DXMATRIX *)this, angle);
    }

    __forceinline void RotateY(f32 angle)
    {
        D3DXMatrixRotationY((D3DXMATRIX *)this, angle);
    }

    __forceinline void RotateZ(f32 angle)
    {
        D3DXMatrixRotationZ((D3DXMATRIX *)this, angle);
    }

    __forceinline ZunMatrix *Multiply(ZunMatrix *pM1, ZunMatrix *pM2)
    {
        return (ZunMatrix *)D3DXMatrixMultiply(this->asD3DX(), pM1->asD3DX(),
                                               pM2->asD3DX());
    }

    __forceinline ZunMatrix *LookAtLH(ZunVec3 *pEye, ZunVec3 *pAt, ZunVec3 *pUp)
    {
        return (ZunMatrix *)D3DXMatrixLookAtLH(this->asD3DX(), pEye->asD3DX(),
                                               pAt->asD3DX(), pUp->asD3DX());
    }

    __forceinline ZunMatrix *PerspectiveFovLH(f32 fovy, f32 Aspect, f32 zn,
                                              f32 zf)
    {
        return (ZunMatrix *)D3DXMatrixPerspectiveFovLH(this->asD3DX(), fovy, Aspect,
                                                       zn, zf);
    }

    __forceinline ZunMatrix *RotationQuaternion(ZunQuaternion *pQ)
    {
        return (ZunMatrix *)D3DXMatrixRotationQuaternion(this->asD3DX(),
                                                         pQ->asD3DX());
    }
};
C_ASSERT(sizeof(ZunMatrix) == 0x40);

__forceinline ZunVec3 *ZunVec3::Project(ZunVec3 *pV, D3DVIEWPORT8 *pViewport,
                                        ZunMatrix *pProjection,
                                        ZunMatrix *pView, ZunMatrix *pWorld)
{
    return (ZunVec3 *)D3DXVec3Project(this->asD3DX(), pV->asD3DX(), pViewport,
                                      pProjection->asD3DX(), pView->asD3DX(),
                                      pWorld->asD3DX());
}

__forceinline ZunVec3 *ZunVec3::TransformCoord(ZunVec3 *pV, ZunMatrix *pM)
{
    return (ZunVec3 *)D3DXVec3TransformCoord(this->asD3DX(), pV->asD3DX(),
                                             pM->asD3DX());
}
