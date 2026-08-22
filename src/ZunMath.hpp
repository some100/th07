#pragma once

#include <cmath>
#include <cstring>

#include "inttypes.hpp"

#define ZUN_PI ((f32)(3.14159265358979323846))
#define ZUN_2PI ((f32)(ZUN_PI * 2.0f))
#define ZUN_3PI ((f32)(ZUN_PI * 3.0f))

inline void sincosf(f32 *outSin, f32 *outCos, f32 angle)
{
    *outCos = cosf(angle);
    *outSin = sinf(angle);
}

struct ZunViewport
{
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    f32 minZ;
    f32 maxZ;
};

struct Float2
{
    Float2 Lerp(const Float2 &to, f32 t) const
    {
        return {this->x + (to.x - this->x) * t, this->y + (to.y - this->y) * t};
    }

    Float2 LerpUv(const Float2 &to, f32 alpha) const
    {
        f32 diffUvX = to.x - this->x;
        if (diffUvX < -0.5f)
        {
            diffUvX += 1.0f;
        }
        else if (diffUvX > 0.5f)
        {
            diffUvX -= 1.0f;
        }
        f32 diffUvY = to.y - this->y;
        if (diffUvY < -0.5f)
        {
            diffUvY += 1.0f;
        }
        else if (diffUvY > 0.5f)
        {
            diffUvY -= 1.0f;
        }

        return {this->x + diffUvX * alpha, this->y + diffUvY * alpha};
    }

    f32 x;
    f32 y;
};

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

    void FromAngleMagnitude(f32 angle, f32 magnitude)
    {
        this->x = cosf(angle) * magnitude;
        this->y = sinf(angle) * magnitude;
    }

    inline void Project(ZunVec3 *pV, ZunViewport *pViewport, struct ZunMatrix *pWVP);

    void Normalize(ZunVec3 *pV)
    {
        f32 len = std::sqrt(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);

        if (len == 0.0f)
        {
            x = y = z = 0.0f;
        }
        else
        {
            f32 inv = 1.0f / len;
            x = pV->x * inv;
            y = pV->y * inv;
            z = pV->z * inv;
        }
    }

    f32 Dot(ZunVec3 *pV) const
    {
        return x * pV->x + y * pV->y + z * pV->z;
    }

    void Cross(ZunVec3 *pV1, ZunVec3 *pV2)
    {
        f32 x = pV1->y * pV2->z - pV1->z * pV2->y;
        f32 y = pV1->z * pV2->x - pV1->x * pV2->z;
        f32 z = pV1->x * pV2->y - pV1->y * pV2->x;

        this->x = x;
        this->y = y;
        this->z = z;
    }

    void TransformCoord(ZunVec3 *pV, struct ZunMatrix *pM);

    f32 Length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    f32 LengthSq() const
    {
        return x * x + y * y + z * z;
    }

    ZunVec3 Lerp(const ZunVec3 &to, f32 t) const
    {
        return *this + (to - *this) * t;
    }

    ZunVec3 operator-() const
    {
        return ZunVec3(-x, -y, -z);
    }

    ZunVec3 operator+(const ZunVec3 &v) const
    {
        return ZunVec3(x + v.x, y + v.y, z + v.z);
    }

    ZunVec3 operator-(const ZunVec3 &v) const
    {
        return ZunVec3(x - v.x, y - v.y, z - v.z);
    }

    ZunVec3 operator*(const ZunVec3 &v) const
    {
        return ZunVec3(x * v.x, y * v.y, z * v.z);
    }

    ZunVec3 operator/(const ZunVec3 &v) const
    {
        return ZunVec3(x / v.x, y / v.y, z / v.z);
    }

    ZunVec3 operator+(f32 f) const
    {
        return ZunVec3(x + f, y + f, z + f);
    }

    ZunVec3 operator-(f32 f) const
    {
        return ZunVec3(x - f, y - f, z - f);
    }

    ZunVec3 operator*(f32 f) const
    {
        return ZunVec3(x * f, y * f, z * f);
    }

    ZunVec3 operator/(f32 f) const
    {
        return ZunVec3(x / f, y / f, z / f);
    }

    void operator+=(const ZunVec3 &v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
    }

    void operator-=(const ZunVec3 &v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
    }

    void operator*=(const ZunVec3 &v)
    {
        x *= v.x;
        y *= v.y;
        z *= v.z;
    }

    void operator=(f32 f)
    {
        x = f;
        y = f;
        z = f;
    }

    void operator*=(f32 f)
    {
        x *= f;
        y *= f;
        z *= f;
    }
};

inline ZunVec3 operator*(f32 f, const ZunVec3 &v)
{
    return ZunVec3(v.x * f, v.y * f, v.z * f);
}

struct ZunQuaternion
{
    f32 x;
    f32 y;
    f32 z;
    f32 w;
};

struct ZunMatrix
{
    f32 m[4][4];

    void Identity()
    {
        memset(m, 0, sizeof(m));
        m[0][0] = 1.0f;
        m[1][1] = 1.0f;
        m[2][2] = 1.0f;
        m[3][3] = 1.0f;
    }

    void RotateX(f32 angle)
    {
        Identity();

        f32 s = sinf(angle);
        f32 c = cosf(angle);

        m[1][1] = c;
        m[1][2] = s;
        m[2][1] = -s;
        m[2][2] = c;
    }

    void RotateY(f32 angle)
    {
        Identity();

        f32 s = sinf(angle);
        f32 c = cosf(angle);

        m[0][0] = c;
        m[0][2] = -s;
        m[2][0] = s;
        m[2][2] = c;
    }

    void RotateZ(f32 angle)
    {
        Identity();

        f32 s = sinf(angle);
        f32 c = cosf(angle);

        m[0][0] = c;
        m[0][1] = s;
        m[1][0] = -s;
        m[1][1] = c;
    }

    ZunMatrix operator*(const ZunMatrix &m) const
    {
        ZunMatrix tmp;

        for (i32 r = 0; r < 4; r++)
        {
            for (i32 c = 0; c < 4; c++)
            {
                tmp.m[r][c] = this->m[r][0] * m.m[0][c] + this->m[r][1] * m.m[1][c] +
                              this->m[r][2] * m.m[2][c] + this->m[r][3] * m.m[3][c];
            }
        }

        return tmp;
    }

    void operator*=(const ZunMatrix &m)
    {
        *this = *this * m;
    }

    void LookAtLH(ZunVec3 *pEye, ZunVec3 *pAt, ZunVec3 *pUp)
    {
        ZunVec3 zAxis;

        ZunVec3 lookAt = *pAt - *pEye;
        zAxis.Normalize(&lookAt);

        ZunVec3 xAxis;
        xAxis.Cross(pUp, &zAxis);
        xAxis.Normalize(&xAxis);

        ZunVec3 yAxis;
        yAxis.Cross(&zAxis, &xAxis);

        m[0][0] = xAxis.x;
        m[1][0] = xAxis.y;
        m[2][0] = xAxis.z;
        m[0][1] = yAxis.x;
        m[1][1] = yAxis.y;
        m[2][1] = yAxis.z;
        m[3][0] = -xAxis.Dot(pEye);

        m[0][2] = zAxis.x;
        m[1][2] = zAxis.y;
        m[2][2] = zAxis.z;
        m[3][1] = -yAxis.Dot(pEye);
        m[3][2] = -zAxis.Dot(pEye);

        m[0][3] = 0.0f;
        m[1][3] = 0.0f;
        m[2][3] = 0.0f;
        m[3][3] = 1.0f;
    }

    void PerspectiveFovLH(f32 fovy, f32 aspect, f32 zn, f32 zf)
    {
        Identity();

        f32 yScale = 1.0f / tanf(fovy * 0.5f);
        f32 xScale = yScale / aspect;

        m[0][0] = xScale;
        m[1][1] = yScale;

        m[2][2] = zf / (zf - zn);
        m[2][3] = 1.0f;

        m[3][2] = -zn * zf / (zf - zn);
        m[3][3] = 0.0f;
    }

    void RotationQuaternion(ZunQuaternion *pQ)
    {
        f32 xx = pQ->x * pQ->x;
        f32 yy = pQ->y * pQ->y;
        f32 zz = pQ->z * pQ->z;

        f32 xy = pQ->x * pQ->y;
        f32 xz = pQ->x * pQ->z;
        f32 yz = pQ->y * pQ->z;

        f32 wx = pQ->w * pQ->x;
        f32 wy = pQ->w * pQ->y;
        f32 wz = pQ->w * pQ->z;

        Identity();

        m[0][0] = 1.0f - 2.0f * (yy + zz);
        m[0][1] = 2.0f * (xy + wz);
        m[0][2] = 2.0f * (xz - wy);

        m[1][0] = 2.0f * (xy - wz);
        m[1][1] = 1.0f - 2.0f * (xx + zz);
        m[1][2] = 2.0f * (yz + wx);

        m[2][0] = 2.0f * (xz + wy);
        m[2][1] = 2.0f * (yz - wx);
        m[2][2] = 1.0f - 2.0f * (xx + yy);
    }
};

inline void ZunVec3::Project(ZunVec3 *pV, ZunViewport *pViewport, ZunMatrix *pWVP)
{
    TransformCoord(pV, pWVP);

    x = pViewport->x + (1.0f + x) * pViewport->width * 0.5f;
    y = pViewport->y + (1.0f - y) * pViewport->height * 0.5f;
    z = pViewport->minZ + z * (pViewport->maxZ - pViewport->minZ);
}

inline void ZunVec3::TransformCoord(ZunVec3 *pV, ZunMatrix *pM)
{
    f32 x = pV->x * pM->m[0][0] + pV->y * pM->m[1][0] + pV->z * pM->m[2][0] + pM->m[3][0];
    f32 y = pV->x * pM->m[0][1] + pV->y * pM->m[1][1] + pV->z * pM->m[2][1] + pM->m[3][1];
    f32 z = pV->x * pM->m[0][2] + pV->y * pM->m[1][2] + pV->z * pM->m[2][2] + pM->m[3][2];
    f32 w = pV->x * pM->m[0][3] + pV->y * pM->m[1][3] + pV->z * pM->m[2][3] + pM->m[3][3];

    x /= w;
    y /= w;
    z /= w;

    this->x = x;
    this->y = y;
    this->z = z;
}
