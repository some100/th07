#pragma once

#include "AnmVm.hpp"
#include "ZunResult.hpp"

typedef i32 (*EffectCallback)(struct Effect *);

struct Effect
{
    Effect();

    AnmVm vm;
    D3DXVECTOR3 pos1;
    D3DXVECTOR3 custom;
    D3DXVECTOR3 velocity;
    D3DXVECTOR3 acceleration;
    D3DXVECTOR3 basePosition;
    D3DXVECTOR3 emitterPosition;
    D3DXVECTOR3 direction;
    D3DXQUATERNION rotationQuat;
    f32 radius;
    f32 angularVelocity;
    ZunTimer timer;
    i32 unused_2c4;
    EffectCallback callback;
    u8 inUseFlag;
    u8 effectId;
    u8 isFadingOut;
    i8 fadeOutTime;
    u8 is2D;
    // pad 3
    Effect *head;
};
C_ASSERT(sizeof(Effect) == 0x2d8);

struct EffectTypeInfo
{
    i32 anmId;
    EffectCallback updateCallback;
    EffectCallback initCallback;
};

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

    static void DoSomethingWithEffects(D3DXVECTOR3 *param_1);
    static void ModifyEffect1eAcceleration();
    static i32 UpdateNoOp(Effect *effect);

    Effect *SpawnParticles(i32 effectId, D3DXVECTOR3 *pos, i32 numParticles,
                           D3DCOLOR color);
    Effect *SpawnEffect(i32 effectId, D3DXVECTOR3 *pos, i32 param_3, i32 param_4,
                        D3DCOLOR color);
    Effect *SpawnMovingParticles(i32 effectId, D3DXVECTOR3 *pos,
                                 D3DXVECTOR3 *velocity, i32 numParticles,
                                 D3DCOLOR color);
    i32 UpdateSpecialEffect();

    i32 nextIndex;
    i32 activeEffects;
    i32 activeEffectsCount;
    f32 globalColorMultiplierR;
    f32 globalColorMultiplierG;
    f32 globalColorMultiplierB;
    f32 globalColorMultiplierA;
    Effect effects[409];
    Effect specialEffects[4];
    Effect *specialEffectsPtrs[4];
    i32 frameCounter;
};
C_ASSERT(sizeof(EffectManager) == 0x496a8);

extern EffectManager g_EffectManager;
