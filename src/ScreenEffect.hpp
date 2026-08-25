#pragma once

#include "Chain.hpp"
#include "ZunTimer.hpp"
#include "inttypes.hpp"

enum ScreenEffectsType
{
    SCREEN_EFFECT_FADE_OUT,
    SCREEN_EFFECT_SHAKE,
    SCREEN_EFFECT_FADE_IN_PLAY_AREA,
    SCREEN_EFFECT_PULSE,
    SCREEN_EFFECT_FADE_IN_FULLSCREEN,
};

struct ZunRect
{
    f32 left;
    f32 top;
    f32 right;
    f32 bottom;
};

struct ScreenEffect
{
    static ScreenEffect *RegisterChain(i32 type, i32 duration, u32 arg1, u32 arg2, u32 arg3);

    static ZunResult AddedCallback(ScreenEffect *arg);
    static ZunResult DeletedCallback(ScreenEffect *arg);
    static u32 OnUpdateFadeIn(ScreenEffect *arg);
    static u32 OnUpdateFadeOut(ScreenEffect *arg);
    static u32 OnUpdatePulse(ScreenEffect *arg);
    static u32 OnUpdateScreenShake(ScreenEffect *arg);
    static u32 OnDrawFullScreenColor(ScreenEffect *arg);
    static u32 OnDrawPlayAreaColor(ScreenEffect *arg);
    static u32 OnDrawPlayAreaPulseColor(ScreenEffect *arg);

    static void Clear(ZunColor color);
    static void DrawSquare(ZunRect *rect, u32 color);
    static void DrawColoredQuad(ZunRect *rect, u32 param_2, u32 param_3, u32 param_4, u32 param_5);
    static void SetViewport(u32 color);

    i32 type;
    ChainElem *calcChain;
    ChainElem *drawChain;
    i32 field3_0xc;
    i32 alpha;
    i32 prevAlpha;
    i32 duration;
    u32 args[3];
    ZunTimer timer;
};
