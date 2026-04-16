#pragma once

#include <d3d8.h>

#include "Chain.hpp"
#include "ZunTimer.hpp"
#include "inttypes.hpp"

struct ZunRect
{
    f32 left;
    f32 top;
    f32 right;
    f32 bottom;
};

namespace ScreenEffect
{
void Clear(D3DCOLOR color);
void DrawSquare(ZunRect *rect, D3DCOLOR color);
void DrawColoredQuad(ZunRect *rect, D3DCOLOR param_2, D3DCOLOR param_3,
                     D3DCOLOR param_4, D3DCOLOR param_5);
void SetViewport(D3DCOLOR color);
} // namespace ScreenEffect

struct BombEffects
{
    static BombEffects *RegisterChain(i32 type, i32 duration, u32 arg1, u32 arg2,
                                      u32 arg3);

    static ZunResult AddedCallback(BombEffects *arg);
    static ZunResult DeletedCallback(BombEffects *arg);
    static u32 OnUpdateFadeIn(BombEffects *arg);
    static u32 OnUpdateFadeOut(BombEffects *arg);
    static u32 OnUpdatePulse(BombEffects *arg);
    static u32 OnUpdateScreenShake(BombEffects *arg);
    static u32 OnDrawFullScreenColor(BombEffects *arg);
    static u32 OnDrawPlayAreaColor(BombEffects *arg);
    static u32 OnDrawPlayAreaPulseColor(BombEffects *arg);

    i32 type;
    ChainElem *calcChain;
    ChainElem *drawChain;
    i32 field3_0xc;
    f32 alpha;
    i32 duration;
    u32 args[3];
    ZunTimer timer;
};
C_ASSERT(sizeof(BombEffects) == 0x30);
