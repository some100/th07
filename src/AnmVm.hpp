#pragma once

#include <d3d8.h>
#include <d3dx8math.h>

#include "ZunColor.hpp"
#include "ZunTimer.hpp"
#include "inttypes.hpp"
#include "utils.hpp"

typedef enum AnmOpcode
{
    ANM_EXIT_HIDE = -1,
    ANM_EXIT_HIDE2 = 1,
    ANM_EXIT = 2,
    ANM_SET_ACTIVE_SPRITE = 3,
    ANM_JUMP = 4,
    ANM_DEC_JUMP = 5,
    ANM_SET_TRANSLATION = 6,
    ANM_SET_SCALE = 7,
    ANM_SET_ALPHA = 8,
    ANM_SET_COLOR = 9,
    ANM_FLIP_X = 10,
    ANM_FLIP_Y = 11,
    ANM_SET_ROTATION = 12,
    ANM_SET_ANGLE_VEL = 13,
    ANM_SET_SCALE_SPEED = 14,
    ANM_FADE = 15,
    ANM_SET_BLEND = 16,
    ANM_POS_TIME_LINEAR = 17,
    ANM_POS_TIME_DECEL = 18,
    ANM_POS_TIME_ACCEL = 19,
    ANM_STOP = 20,
    ANM_INTERRUPT_LABEL = 21,
    ANM_22 = 22,
    ANM_STOP_HIDE = 23,
    ANM_SET_USE_OFFSET = 24,
    ANM_SET_AUTO_ROTATE = 25,
    ANM_SET_SCROLL_POS_X = 26,
    ANM_SET_SCROLL_POS_Y = 27,
    ANM_SET_VISIBILITY = 28,
    ANM_INTERP_SCALE = 29,
    ANM_SET_ZWRITE_DISABLE = 30,
    ANM_SET_CAMERA_MODE = 31,
    ANM_INTERP_POS = 32,
    ANM_INTERP_COLOR = 33,
    ANM_INTERP_ALPHA = 34,
    ANM_INTERP_ROTATE = 35,
    ANM_INTERP_SCALE_2 = 36,
    ANM_MOV = 37,
    ANM_MOV_FLOAT = 38,
    ANM_ADD = 39,
    ANM_ADD_FLOAT = 40,
    ANM_SUB = 41,
    ANM_SUB_FLOAT = 42,
    ANM_MUL = 43,
    ANM_MUL_FLOAT = 44,
    ANM_DIV = 45,
    ANM_DIV_FLOAT = 46,
    ANM_MOD = 47,
    ANM_MOD_FLOAT = 48,
    ANM_ADD_2 = 49,
    ANM_ADD_FLOAT_2 = 50,
    ANM_SUB_2 = 51,
    ANM_SUB_FLOAT_2 = 52,
    ANM_MUL_2 = 53,
    ANM_MUL_FLOAT_2 = 54,
    ANM_DIV_2 = 55,
    ANM_DIV_FLOAT_2 = 56,
    ANM_MOD_2 = 57,
    ANM_MOD_FLOAT_2 = 58,
    ANM_RAND = 59,
    ANM_RAND_FLOAT = 60,
    ANM_SIN = 61,
    ANM_COS = 62,
    ANM_TAN = 63,
    ANM_ACOS = 64,
    ANM_ATAN = 65,
    ANM_ADD_NORMALIZE_ANGLE = 66,
    ANM_JUMP_IF_EQ = 67,
    ANM_JUMP_IF_EQ_FLOAT = 68,
    ANM_JUMP_IF_NEQ = 69,
    ANM_JUMP_IF_NEQ_FLOAT = 70,
    ANM_JUMP_IF_LT = 71,
    ANM_JUMP_IF_LT_FLOAT = 72,
    ANM_JUMP_IF_LEQ = 73,
    ANM_JUMP_IF_LEQ_FLOAT = 74,
    ANM_JUMP_IF_GT = 75,
    ANM_JUMP_IF_GT_FLOAT = 76,
    ANM_JUMP_IF_GEQ = 77,
    ANM_JUMP_IF_GEQ_FLOAT = 78,
    ANM_WAIT = 79,
    ANM_SET_SCROLLVEL_X = 80,
    ANM_SET_SCROLLVEL_Y = 81
} AnmOpcode;

struct AnmLoadedSprite
{
    i32 sourceFileIndex;
    D3DXVECTOR2 startPixelInclusive;
    D3DXVECTOR2 endPixelInclusive;
    f32 textureHeight;
    f32 textureWidth;
    D3DXVECTOR2 uvStart;
    D3DXVECTOR2 uvEnd;
    f32 heightPx;
    f32 widthPx;
    f32 cols;
    f32 rows;
    i32 spriteId;
};

struct AnmRawInstr
{
    i16 opcode;
    u16 size;
    i16 time;
    u16 flags;
    AnyArg args[];
};

struct AnmVmBase
{
    // FUNCTION: TH07 0x004011b0
    AnmVmBase()
    {
    }
    D3DXVECTOR3 rotation;
    D3DXVECTOR3 angleVel;
    D3DXVECTOR2 scale;
    D3DXVECTOR2 scaleGrowth;
    D3DXVECTOR2 uvScrollPos;
    ZunTimer currentTimeInScript;
    ZunTimer waitTimer;
    ZunTimer interpStartTimes[5]; /* pos = 0, color, alpha, rotate, scale
                                            in that order */
    ZunTimer interpEndTimes[5];
};

struct AnmVm : AnmVmBase
{
    // FUNCTION: TH07 0x00401170
    AnmVm()
    {
        memset(this, 0, sizeof(AnmVm));
        this->activeSpriteIdx = -1;
    }

    void SetInvisible()
    {
        this->visible = 0;
    }

    void SetPendingInterrupt(i16 interrupt)
    {
        this->pendingInterrupt = interrupt;
    }

    void SetRotationZ(f32 z)
    {
        this->rotation.z = z;
    }

    static void AssignVm(AnmVm *out, AnmVm *vm)
    {
        if (out->anmFileIdx != vm->anmFileIdx || vm->currentInstruction != NULL)
        {
            *out = *vm;
        }
    }

    i32 *GetVar(i32 *paramId, u16 mask, u32 idx);
    f32 *GetFloatVar(f32 *param_1, u16 mask, u32 idx);
    f32 GetFloatVarValue(f32 param_1);
    i32 GetVarValue(i32 arg);
    void Initialize();

    u8 interpModes[5];
    // pad 3
    i32 intVars1[4];
    f32 floatVars[4];
    i32 intVars2[2];
    D3DXVECTOR2 uvScrollVel;
    D3DXMATRIX matrix;
    D3DXMATRIX worldTransformMatrix;
    D3DXMATRIX uvMatrix;
    ZunColor color;
    ZunColor color2;
    union {
        u32 flags;
        struct
        {
            u32 visible : 1;
            u32 active : 1;
            u32 updateRotation : 1;
            u32 updateScale : 1;
            u32 blendMode : 1;
            u32 flag6 : 1;
            u32 flag7 : 1;
            u32 useOffset : 1;
            u32 flip : 2;
            u32 anchor : 2;
            u32 zWriteDisable : 1;
            u32 isStopped : 1;
            u32 cameraMode : 1;
            u32 skipTransform : 1;
            u32 useColor2 : 1;
        };
    };
    i16 autoRotate;
    i16 pendingInterrupt;
    D3DXVECTOR3 pos;
    i16 activeSpriteIdx;
    i16 baseSpriteIdx;
    i16 anmFileIdx;
    // pad 2
    AnmRawInstr *beginningOfScript;
    AnmRawInstr *currentInstruction;
    AnmLoadedSprite *sprite;
    D3DXVECTOR3 posInterpInitial;
    D3DXVECTOR3 posInterpFinal;
    D3DXVECTOR3 rotateInterpInitial;
    D3DXVECTOR3 rotateInterpFinal;
    D3DXVECTOR2 scaleInterpInitial;
    D3DXVECTOR2 scaleInterpFinal;
    ZunColor colorInterpInitialColor;
    ZunColor colorInterpFinalColor;
    D3DXVECTOR3 offset;
    i32 timeOfLastSpriteSet;
    u8 fontWidth;
    u8 fontHeight;
    u8 unused_242[10];
};
C_ASSERT(sizeof(AnmVm) == 0x24c);

extern const D3DFORMAT g_TextureFormatD3D8Mapping[6];
extern const i32 g_TextureBytesPerPixel[7];
