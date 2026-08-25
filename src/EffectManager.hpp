#pragma once

#include "AnmVm.hpp"
#include "ZunMath.hpp"
#include "ZunResult.hpp"

typedef i32 (*EffectCallback)(struct Effect *);

struct Effect
{
    AnmVm vm;
    ZunVec3 pos;
    ZunVec3 prevPos;
    ZunVec3 custom;
    ZunVec3 velocity;
    ZunVec3 accel;
    ZunVec3 basePos;
    ZunVec3 emitterPos;
    ZunVec3 direction;
    ZunQuaternion rotationQuat;
    f32 radius;
    f32 angleVel;
    ZunTimer timer;
    i32 unused_2c4;
    EffectCallback callback;
    i8 inUseFlag;
    i8 effectId;
    u8 isFadingOut;
    i8 fadeOutTime;
    i8 drawType;
    // pad 3
    Effect *next;
};

struct EffectTypeInfo
{
    i32 anmId;
    EffectCallback updateCallback;
    EffectCallback initCallback;
};

#define MAX_NORMAL_EFFECTS 400
#define MAX_SPECIAL_EFFECTS 8
#define MAX_EFFECTS (MAX_NORMAL_EFFECTS + MAX_SPECIAL_EFFECTS)

struct EffectManager
{
    EffectManager();
    void Reset();

    static ZunResult RegisterChain();
    static void CutChain();

    static ZunResult AddedCallback(EffectManager *arg);
    static ZunResult DeletedCallback(EffectManager *arg);
    static u32 OnUpdate(EffectManager *arg);
    static u32 OnDraw(EffectManager *arg);

    static i32 UpdatePhysics(Effect *effect);
    static i32 UpdateOrbitEffect(Effect *effect);
    static i32 UpdateGather60Frames(Effect *effect);
    static i32 UpdateGather240Frames(Effect *effect);
    static i32 UpdateBurstEaseOut30Frames(Effect *effect);
    static i32 UpdateAttachToCamera(Effect *effect);
    static i32 UpdateAttachToPlayer(Effect *effect);
    static i32 UpdateWeatherPhysics(Effect *effect);
    static i32 UpdateBurst30Frames(Effect *effect);

    static i32 InitDeceleratingBurst(Effect *effect);
    static i32 InitDeceleratingBurstFast(Effect *effect);
    static i32 Init2dEffect(Effect *effect);
    static i32 InitRandomDir(Effect *effect);
    static i32 InitRandomDirWithSpeed(Effect *effect);
    static i32 InitWeatherForward(Effect *effect);
    static i32 InitWeatherVortex(Effect *effect);
    static i32 InitWeatherBackward(Effect *effect);
    static i32 InitWeatherSlow(Effect *effect);
    static i32 InitWeatherFalling(Effect *effect);

    static void ShiftEffectsAfterCameraTeleport(ZunVec3 *shift);
    static void ModifyEffect1eAcceleration();
    static i32 UpdateNoOp(Effect *effect);

    Effect *SpawnEffect(i32 effectId, ZunVec3 *pos, i32 numParticles, u32 color);
    Effect *SpawnSpecialEffect(i32 effectId, ZunVec3 *pos, i32 effectIdx, i32 param_4, u32 color);
    Effect *SpawnMovingParticles(i32 effectId, ZunVec3 *pos, ZunVec3 *velocity, i32 numParticles,
                                 u32 color);
    i32 DrawLayer1Effects();

    i32 nextIndex;
    i32 unused;
    i32 activeEffects;
    f32 globalColorMultiplierR;
    f32 globalColorMultiplierG;
    f32 globalColorMultiplierB;
    f32 globalColorMultiplierA;
    Effect effects[MAX_EFFECTS + 1];
    Effect layer0;
    Effect layer1;
    Effect layer2;
    Effect layer3;
    Effect *layerPtrs[4];
    i32 frameCounter;
};

extern EffectManager g_EffectManager;
