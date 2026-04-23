#include "EffectManager.hpp"

#include "AnmManager.hpp"
#include "GameManager.hpp"
#include "Player.hpp"
#include "Rng.hpp"
#include "Stage.hpp"
#include "ZunMath.hpp"
#include "ZunResult.hpp"
#include "utils.hpp"

// GLOBAL: TH07 0x012fe250
EffectManager g_EffectManager;

// GLOBAL: TH07 0x013478f8
ChainElem g_EffectManagerCalcChain;

// GLOBAL: TH07 0x01347918
ChainElem g_EffectManagerDrawChain;

// GLOBAL: TH07 0x0049efc0
EffectTypeInfo g_EffectMapping[34] = {
    {0x2ab, NULL, NULL},
    {0x2ac, NULL, NULL},
    {0x2ad, NULL, NULL},
    {0x2ae, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurst},
    {0x2b3, EffectManager::UpdatePhysics,
     EffectManager::InitDeceleratingBurstFast},
    {0x2b4, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurst},
    {0x2b5, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurst},
    {0x2b6, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurst},
    {0x2b7, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurst},
    {0x2b8, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurst},
    {0x2b9, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurst},
    {0x2ba, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurst},
    {0x2bb, NULL, NULL},
    {0x2bc, EffectManager::UpdateOrbitEffect, EffectManager::Init2dEffect},
    {0x2bc, EffectManager::UpdateOrbitEffect, EffectManager::Init2dEffect},
    {0x2bc, EffectManager::UpdateOrbitEffect, EffectManager::Init2dEffect},
    {0x2dc, NULL, NULL},
    {0x2af, EffectManager::UpdateGather60Frames, EffectManager::InitRandomDir},
    {0x2b0, EffectManager::UpdateGather240Frames, EffectManager::InitRandomDir},
    {0x2bd, EffectManager::UpdateNoOp, NULL},
    {0x2bf, EffectManager::UpdateWeatherPhysics,
     EffectManager::InitWeatherForward},
    {0x2c3, NULL, NULL},
    {0x2c0, EffectManager::UpdateBurstEaseOut30Frames,
     EffectManager::InitRandomDirWithSpeed},
    {0x304, EffectManager::UpdateAttachToCamera, NULL},
    {0x2c2, EffectManager::UpdateAttachToPlayer, NULL},
    {0x2da, EffectManager::UpdateNoOp, NULL},
    {0x2bf, EffectManager::UpdateWeatherPhysics,
     EffectManager::InitWeatherVortex},
    {0x2bf, EffectManager::UpdateWeatherPhysics,
     EffectManager::InitWeatherBackward},
    {0x2db, EffectManager::UpdateNoOp, NULL},
    {0x2b2, EffectManager::UpdateBurst30Frames, EffectManager::InitRandomDir},
    {0x2bf, EffectManager::UpdateWeatherPhysics,
     EffectManager::InitWeatherSlow},
    {0x2bf, EffectManager::UpdateWeatherPhysics,
     EffectManager::InitWeatherFalling},
    {0x2c1, EffectManager::UpdateBurstEaseOut30Frames,
     EffectManager::InitRandomDirWithSpeed},
    {0x2b1, EffectManager::UpdateGather60Frames, EffectManager::InitRandomDir},
};

// FUNCTION: TH07 0x0041a210
EffectManager::EffectManager()
{
    Reset();
    this->globalColorMultiplierR = 1.0;
    this->globalColorMultiplierG = 1.0;
    this->globalColorMultiplierB = 1.0;
    this->globalColorMultiplierA = 1.0;
}

// FUNCTION: TH07 0x0041a2f0
Effect::Effect()
{
}

// FUNCTION: TH07 0x0041a350
void EffectManager::Reset()
{
    memset(this, 0, sizeof(EffectManager));
}

// FUNCTION: TH07 0x0041a370
i32 EffectManager::InitDeceleratingBurstFast(Effect *effect)
{
    effect->velocity.x = (g_Rng.GetRandomFloatInRange(256.0f) - 128.0f) / 12.0f;
    effect->velocity.y = (g_Rng.GetRandomFloatInRange(256.0f) - 128.0f) / 12.0f;
    effect->velocity.z = 0.0f;
    effect->acceleration = -effect->velocity * 0.05263158f;
    effect->velocity *= g_Supervisor.effectiveFramerateMultiplier;
    effect->acceleration *= g_Supervisor.effectiveFramerateMultiplier;
    return 0;
}

// FUNCTION: TH07 0x0041a4f0
i32 EffectManager::UpdatePhysics(Effect *effect)
{
    effect->pos1 += effect->velocity;
    effect->velocity += effect->acceleration;
    return 1;
}

// FUNCTION: TH07 0x0041a5a0
i32 EffectManager::InitDeceleratingBurst(Effect *effect)
{
    effect->velocity.x =
        ((g_Rng.GetRandomFloatInRange(256.0f) - 128.0f) * 4.0f) / 33.0f;
    effect->velocity.y =
        ((g_Rng.GetRandomFloatInRange(256.0f) - 128.0f) * 4.0f) / 33.0f;
    effect->velocity.z = 0.0f;
    effect->acceleration = -effect->velocity * 0.05f;
    effect->velocity.x *= g_Supervisor.effectiveFramerateMultiplier;
    effect->acceleration.x *= g_Supervisor.effectiveFramerateMultiplier;
    return 0;
}

// FUNCTION: TH07 0x0041a730
i32 EffectManager::Init2dEffect(Effect *effect)
{
    effect->is2D = 2;
    return 0;
}

// FUNCTION: TH07 0x0041a750
i32 EffectManager::UpdateOrbitEffect(Effect *effect)
{
    f32 fVar2;
    D3DXVECTOR3 local_64;
    D3DXMATRIX local_54;
    D3DXVECTOR3 local_10;

    D3DXVec3Normalize(&local_64, &effect->direction);
    fVar2 = sinf(effect->angularVelocity);
    effect->rotationQuat.x = local_64.x * fVar2;
    effect->rotationQuat.y = local_64.y * fVar2;
    effect->rotationQuat.z = local_64.z * fVar2;
    effect->rotationQuat.w = cosf(effect->angularVelocity);
    D3DXMatrixRotationQuaternion(&local_54, &effect->rotationQuat);
    local_10.x = local_64.y * 1.0f - local_64.z * 0.0f;
    local_10.y = local_64.z * 0.0f - local_64.x * 1.0f;
    local_10.z = local_64.x * 0.0f - local_64.y * 0.0f;
    if (0.00001f <= local_10.x * local_10.x + local_10.y * local_10.y +
                      local_10.z * local_10.z)
    {
        D3DXVec3Normalize(&local_10, &local_10);
    }
    else
    {
        local_64.x = 1.0f;
        local_64.y = 0.0f;
        local_64.z = 0.0f;
    }
    local_10.x *= effect->radius;
    local_10.y *= effect->radius;
    local_10.z *= effect->radius;
    D3DXVec3TransformCoord(&local_10, &local_10, &local_54);
    local_10.z *= 6.0f;
    effect->pos1 = local_10 + effect->emitterPosition;
    if (effect->isFadingOut != 0)
    {
        effect->fadeOutTime = effect->fadeOutTime + 1;
        if (15 < effect->fadeOutTime)
        {
            return 0;
        }
        fVar2 = 1.0f - (f32)(i32)effect->fadeOutTime / 16.0f;
        effect->vm.color.color =
            (effect->vm.color.color & 0xffffff) | (u32)(fVar2 * 255.0f) << 0x18;
        effect->vm.scale.y = 2.0f - fVar2;
        effect->vm.scale.x = effect->vm.scale.y;
    }
    return 1;
}

// FUNCTION: TH07 0x0041aa60
i32 EffectManager::InitRandomDir(Effect *effect)
{
    f32 fVar1;

    effect->emitterPosition = effect->pos1;
    effect->emitterPosition.z = 0.0f;
    fVar1 = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->direction.x = cosf(fVar1);
    effect->direction.y = sinf(fVar1);
    effect->direction.z = 0.0f;
    return 0;
}

// FUNCTION: TH07 0x0041aaf0
i32 EffectManager::UpdateGather60Frames(Effect *effect)
{
    effect->pos1 =
        (256.0f -
         (((f32)effect->timer.current + effect->timer.subFrame) * 256.0f) /
             60.0f) *
            effect->direction +
        effect->emitterPosition;
    effect->pos1.z = 0.0f;
    return 1;
}

// FUNCTION: TH07 0x0041abe0
i32 EffectManager::UpdateAttachToPlayer(Effect *effect)
{
    bool bVar1 = effect->vm.currentInstruction != NULL;
    if (bVar1)
    {
        effect->pos1 = g_Player.positionCenter;
    }
    return bVar1;
}

// FUNCTION: TH07 0x0041ac30
i32 EffectManager::UpdateGather240Frames(Effect *effect)
{
    effect->pos1 =
        (256.0f -
         (((f32)effect->timer.current + effect->timer.subFrame) * 256.0f) /
             240.0f) *
            effect->direction +
        effect->emitterPosition;
    return 1;
}

// FUNCTION: TH07 0x0041ad10
i32 EffectManager::UpdateBurst30Frames(Effect *effect)
{
    effect->pos1 =
        ((((f32)effect->timer.current + effect->timer.subFrame) * 256.0f) /
         30.0f) *
            effect->direction +
        effect->emitterPosition;
    return 1;
}

// FUNCTION: TH07 0x0041adf0
void EffectManager::DoSomethingWithEffects(D3DXVECTOR3 *param_1)
{
    i32 i;
    Effect *effect;

    effect = g_EffectManager.effects;
    for (i = 0; i < 400; i++)
    {
        if ((effect->effectId == 20) || (effect->effectId == 0x1f))
        {
            effect->basePosition += *param_1;
        }
        effect = effect + 1;
    }
}

// FUNCTION: TH07 0x0041ae90
void EffectManager::ModifyEffect1eAcceleration()
{
    i32 i;
    Effect *effect;

    effect = g_EffectManager.effects;
    for (i = 0; i < 400; i++)
    {
        if (effect->effectId == 0x1e)
        {
            (effect->acceleration).z = -0.01f;
        }
        effect = effect + 1;
    }
}

// FUNCTION: TH07 0x0041aef0
i32 EffectManager::UpdateWeatherPhysics(Effect *effect)
{
    D3DXVECTOR3 local_10;

    effect->velocity += effect->acceleration;
    effect->basePosition += effect->velocity;
    effect->pos1 = effect->basePosition;
    local_10.z = effect->pos1.z - g_Stage.camPos.z;
    local_10.y = effect->pos1.y - g_Stage.camPos.y;
    local_10.x = effect->pos1.x - g_Stage.camPos.x;
    D3DXVec3Normalize(&local_10, &local_10);
    if (0.94f <= g_Stage.camLookAtDir.x * local_10.x +
                     g_Stage.camLookAtDir.y * local_10.y +
                     g_Stage.camLookAtDir.z * local_10.z)
    {
        effect->vm.rotation.z =
            utils::AddNormalizeAngle(effect->vm.rotation.z, effect->vm.rotation.x);
        effect->vm.updateRotation = 1;
        if (effect->pos1.z < 0.0f)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

// FUNCTION: TH07 0x0041b0b0
i32 EffectManager::InitWeatherForward(Effect *effect)
{
    u32 uVar2;

    effect->basePosition = g_Stage.camLookAt + g_Stage.camPos;
    effect->basePosition.x = -g_Stage.camLookAt.x / 2.0f +
                             (g_Rng.GetRandomFloatInRange(120.0f) - 60.0f) +
                             effect->basePosition.x;
    effect->basePosition.y = -g_Stage.camLookAt.y / 2.0f +
                             (g_Rng.GetRandomFloatInRange(200.0f) - 100.0f) +
                             effect->basePosition.y;
    effect->basePosition.z = -g_Stage.camLookAt.z / 2.0f +
                             (g_Rng.GetRandomFloatInRange(100.0f) - 100.0f) +
                             effect->basePosition.z;
    effect->velocity.x =
        (g_Rng.GetRandomFloatInRange(0.06f) - 0.03f) + effect->custom.x;
    effect->velocity.y =
        (g_Rng.GetRandomFloatInRange(0.06f) - 0.03f) + effect->custom.y;
    effect->velocity.z = g_Rng.GetRandomFloatInRange(0.1f) + 0.03f + effect->custom.z;
    effect->acceleration.x = g_Rng.GetRandomFloatInRange(0.0002f) - 0.0001f;
    effect->acceleration.y = g_Rng.GetRandomFloatInRange(0.0002f) - 0.0001f;
    effect->velocity *= g_Supervisor.effectiveFramerateMultiplier;
    effect->acceleration *= g_Supervisor.effectiveFramerateMultiplier;
    effect->is2D = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(0.03141593f) - 0.015707964f;
    uVar2 = ((g_GameManager.cherry - g_GameManager.globals->cherryStart) * 100) /
            g_GameManager.cherryMax;

    if (g_Rng.GetRandomU32InRange(100) <= uVar2)
    {
        g_AnmManager->SetActiveSprite(&effect->vm, 0x2d8);
        effect->vm.color.bytes.r = 0xff;
        effect->vm.color.bytes.g = 0xff;
        effect->vm.color.bytes.b = 0xff;
    }
    return 0;
}

// FUNCTION: TH07 0x0041b4a0
i32 EffectManager::InitWeatherVortex(Effect *effect)
{
    u32 uVar2;

    effect->basePosition.x = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.y = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.z = g_Rng.GetRandomFloatInRange(100.0f) - 50.0f;
    effect->velocity.x = -effect->basePosition.y / effect->custom.x;
    effect->velocity.y = effect->basePosition.x / effect->custom.x;
    effect->velocity.z = g_Rng.GetRandomFloatInRange(0.1f) + 0.09f;
    effect->basePosition +=
        g_Stage.camLookAt * 0.5f + g_Stage.camPos + effect->basePosition;
    effect->velocity.x *= g_Supervisor.effectiveFramerateMultiplier;
    effect->is2D = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(0.06283186f) - 0.03141593f;
    uVar2 = ((g_GameManager.cherry - g_GameManager.globals->cherryStart) * 100) /
            g_GameManager.cherryMax;

    if (g_Rng.GetRandomU32InRange(100) <= uVar2)
    {
        g_AnmManager->SetActiveSprite(&effect->vm, 0x2d8);
        effect->vm.color.bytes.r = 0xff;
        effect->vm.color.bytes.g = 0xff;
        effect->vm.color.bytes.b = 0xff;
    }
    effect->acceleration.x = 0.0f;
    effect->acceleration.y = 0.0f;
    effect->acceleration.z = 0.0f;
    return 0;
}

// FUNCTION: TH07 0x0041b770
i32 EffectManager::InitWeatherBackward(Effect *effect)
{
    effect->basePosition.x = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.y = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.z = g_Rng.GetRandomFloatInRange(100.0f) - 50.0f;
    effect->velocity.x = -effect->basePosition.y / effect->custom.x;
    effect->velocity.y = effect->basePosition.x / effect->custom.x;
    effect->velocity.z = -(g_Rng.GetRandomFloatInRange(0.2f)) - 0.06f;
    effect->basePosition += g_Stage.camLookAt * 0.5f + g_Stage.camPos;
    effect->velocity *= g_Supervisor.effectiveFramerateMultiplier;
    effect->is2D = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(0.06283186f) - 0.03141593f;
    g_AnmManager->SetActiveSprite(&effect->vm, 0x2d8);
    effect->vm.color.bytes.r = 0xff;
    effect->vm.color.bytes.g = 0xff;
    effect->vm.color.bytes.b = 0xff;
    effect->acceleration.x = 0.0f;
    effect->acceleration.y = 0.0f;
    effect->acceleration.z = 0.0f;
    return 0;
}

// FUNCTION: TH07 0x0041b9f0
i32 EffectManager::InitWeatherSlow(Effect *effect)
{
    effect->basePosition.x = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.y = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.z = g_Rng.GetRandomFloatInRange(100.0f) - 100.0f;
    effect->velocity.x =
        (g_Rng.GetRandomFloatInRange(0.06f) - 0.03f) + effect->custom.x;
    effect->velocity.y =
        (g_Rng.GetRandomFloatInRange(0.06f) - 0.03f) + effect->custom.y;
    effect->velocity.z =
        g_Rng.GetRandomFloatInRange(0.02f) + 0.01f + effect->custom.z;
    effect->basePosition.x += g_Stage.camLookAt.x * 0.5f + g_Stage.camPos.x;
    effect->is2D = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(0.06283186f) - 0.03141593f;
    g_AnmManager->SetActiveSprite(&effect->vm, 0x2d8);
    effect->vm.color.bytes.r = 0xff;
    effect->vm.color.bytes.g = 0xff;
    effect->vm.color.bytes.b = 0xff;
    effect->acceleration.x = 0.0f;
    effect->acceleration.y = 0.0f;
    effect->acceleration.z = 0.0f;
    return 0;
}

// FUNCTION: TH07 0x0041bc20
i32 EffectManager::InitWeatherFalling(Effect *effect)
{
    effect->basePosition.x = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.y = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.z = g_Rng.GetRandomFloatInRange(200.0f) - 0.0f;
    effect->velocity.x =
        (g_Rng.GetRandomFloatInRange(0.06f) - 0.03f) + effect->custom.x;
    effect->velocity.y =
        (g_Rng.GetRandomFloatInRange(0.06f) - 0.03f) + effect->custom.y;
    effect->velocity.z = -(g_Rng.GetRandomFloatInRange(0.1f)) + effect->custom.z;
    effect->basePosition.x += g_Stage.camLookAt.x * 0.5f + g_Stage.camPos.x;
    effect->velocity.x *= g_Supervisor.effectiveFramerateMultiplier;
    effect->is2D = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(0.06283186f) - 0.03141593f;
    g_AnmManager->SetActiveSprite(&effect->vm, 0x2d8);
    effect->vm.angleVel.z += effect->vm.angleVel.z;
    effect->vm.color.bytes.r = 0xff;
    effect->vm.color.bytes.g = 0xff;
    effect->vm.color.bytes.b = 0xff;
    effect->acceleration.x = 0.0f;
    effect->acceleration.y = 0.0f;
    effect->acceleration.z = -0.015f;
    return 0;
}

// FUNCTION: TH07 0x0041bec0
i32 EffectManager::InitRandomDirWithSpeed(Effect *effect)
{
    f32 local_8;

    // double intentionally used here, strangely
    if (effect->custom.x <= -990.0)
    {
        local_8 = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    }
    else
    {
        local_8 = utils::AddNormalizeAngle(effect->custom.x, 0.0f);
    }
    effect->emitterPosition = effect->pos1;
    effect->emitterPosition.z = 0.0f;
    effect->direction.x = cosf(local_8);
    effect->direction.y = sinf(local_8);
    effect->direction.z = 0.0f;
    effect->direction *= g_Rng.GetRandomFloatInRange(1.5f) + 1.0f;
    return 0;
}

// FUNCTION: TH07 0x0041bfd0
i32 EffectManager::UpdateBurstEaseOut30Frames(Effect *effect)
{
    f32 fVar1;

    fVar1 = ((f32)effect->timer.current + effect->timer.subFrame) / 90.0f;
    effect->pos1 =
        (1.0f - (1.0f - fVar1) * (1.0f - fVar1)) * effect->direction * 128.0f +
        effect->emitterPosition;
    effect->pos1.z = 0.0f;
    return 1;
}

// FUNCTION: TH07 0x0041c100
i32 EffectManager::UpdateAttachToCamera(Effect *effect)
{
    effect->is2D = 1;
    effect->basePosition = g_Stage.camLookAt + g_Stage.camPos;
    effect->pos1 = effect->basePosition;
    effect->pos1.z = 0.0f;
    effect->is2D = 3;
    return 1;
}

// FUNCTION: TH07 0x0041c1b0
i32 EffectManager::UpdateNoOp(Effect *effect)
{
    return 1;
}

// FUNCTION: TH07 0x0041c1c0
Effect *EffectManager::SpawnParticles(i32 effectId, D3DXVECTOR3 *pos,
                                      i32 numParticles, D3DCOLOR color)
{
    i32 iVar1;
    i16 local_1c;
    Effect *effect;

    i32 i = 0;
    effect = this->effects + this->nextIndex;
    while (i < 400)
    {
        this->nextIndex = this->nextIndex + 1;
        if (399 < this->nextIndex)
        {
            this->nextIndex = 0;
        }
        if (effect->inUseFlag == 0)
        {
            effect->is2D = 0;
            effect->inUseFlag = 1;
            effect->effectId = (u8)effectId;
            effect->pos1 = *pos;
            iVar1 = g_EffectMapping[effectId].anmId;
            local_1c = (i16)iVar1;
            effect->vm.anmFileIdx = local_1c;
            g_AnmManager->SetAndExecuteScript(&effect->vm,
                                              g_AnmManager->scripts[iVar1]);
            effect->vm.zWriteDisable = 1;
            effect->vm.color.color = color;
            effect->callback = g_EffectMapping[effectId].updateCallback;
            effect->timer = 0;
            effect->isFadingOut = 0;
            effect->fadeOutTime = 0;
            effect->custom.x = 0.0f;
            effect->custom.y = 0.0f;
            effect->custom.z = 0.0f;
            if ((g_EffectMapping[effectId].initCallback != NULL) &&
                (g_EffectMapping[effectId].initCallback(effect) != 0))
            {
                effect->inUseFlag = 0;
            }
            numParticles += -1;
            if (numParticles == 0)
                break;
            if (this->nextIndex == 0)
            {
                effect = this->effects;
            }
            else
            {
                effect = effect + 1;
            }
        }
        else if (this->nextIndex == 0)
        {
            effect = this->effects;
        }
        else
        {
            effect = effect + 1;
        }
        ++i;
    }
    if (i < 400)
    {
        return effect;
    }
    else
    {
        return this->effects + 0x198;
    }
}

// FUNCTION: TH07 0x0041c400
Effect *EffectManager::SpawnMovingParticles(i32 effectId, D3DXVECTOR3 *pos,
                                            D3DXVECTOR3 *velocity,
                                            i32 numParticles, D3DCOLOR color)
{
    Effect *effect = &this->effects[this->nextIndex];

    i32 i = 0;
    while (i < 400)
    {
        this->nextIndex = this->nextIndex + 1;
        if (399 < this->nextIndex)
        {
            this->nextIndex = 0;
        }
        if (effect->inUseFlag == 0)
        {
            effect->is2D = 0;
            effect->inUseFlag = 1;
            effect->effectId = effectId;
            effect->pos1 = *pos;
            i16 local_10 = g_EffectMapping[effectId].anmId;
            effect->vm.anmFileIdx = local_10;
            g_AnmManager->SetAndExecuteScript(
                &effect->vm, g_AnmManager->scripts[g_EffectMapping[effectId].anmId]);
            effect->vm.color.color = color;
            effect->callback = g_EffectMapping[effectId].updateCallback;
            effect->timer = 0;
            effect->isFadingOut = 0;
            effect->fadeOutTime = 0;
            effect->custom = *velocity;
            if ((g_EffectMapping[effectId].initCallback != NULL) &&
                (g_EffectMapping[effectId].initCallback(effect) != 0))
            {
                effect->inUseFlag = 0;
            }
            numParticles += -1;
            if (numParticles == 0)
                break;
            if (this->nextIndex == 0)
            {
                effect = this->effects;
            }
            else
            {
                effect = effect + 1;
            }
        }
        else if (this->nextIndex == 0)
        {
            effect = this->effects;
        }
        else
        {
            effect = effect + 1;
        }
        ++i;
    }

    Effect *local_20;
    if (i < 400)
    {
        local_20 = effect;
    }
    else
    {
        local_20 = this->effects + 0x198;
    }
    return local_20;
}

// FUNCTION: TH07 0x0041c610
Effect *EffectManager::SpawnEffect(i32 effectId, D3DXVECTOR3 *pos, i32 param_3,
                                   i32 param_4, D3DCOLOR color)
{
    Effect *effect;
    i16 local_18;
    i32 iVar1;

    effect = this->effects + param_3 + 400;
    effect->is2D = 0;
    effect->inUseFlag = 1;
    effect->effectId = effectId;
    effect->pos1 = *pos;
    iVar1 = g_EffectMapping[effectId].anmId;
    local_18 = (i16)iVar1;
    effect->vm.anmFileIdx = local_18;
    g_AnmManager->SetAndExecuteScript(&effect->vm, g_AnmManager->scripts[iVar1]);
    effect->vm.zWriteDisable = 1;
    effect->vm.color.color = color;
    effect->callback = g_EffectMapping[effectId].updateCallback;
    effect->timer = 0;
    effect->isFadingOut = 0;
    effect->fadeOutTime = 0;
    effect->custom.x = 0.0f;
    effect->custom.y = 0.0f;
    effect->custom.z = 0.0f;
    if (g_EffectMapping[effectId].initCallback != NULL)
    {
        if (g_EffectMapping[effectId].initCallback(effect) != 0)
        {
            effect->inUseFlag = 0;
        }
    }
    return effect;
}

// FUNCTION: TH07 0x0041c790
u32 EffectManager::OnUpdate(EffectManager *arg)
{
    i32 local_c;
    Effect *effect;

    effect = arg->effects;
    arg->activeEffectsCount = 0;
    arg->specialEffectsPtrs[0] = &arg->specialEffects[0];
    arg->specialEffectsPtrs[1] = &arg->specialEffects[1];
    arg->specialEffectsPtrs[2] = &arg->specialEffects[2];
    arg->specialEffectsPtrs[3] = &arg->specialEffects[3];
    arg->specialEffects[0].head = NULL;
    arg->specialEffects[1].head = NULL;
    arg->specialEffects[2].head = NULL;
    arg->specialEffects[3].head = NULL;
    for (local_c = 0; local_c < 0x198; local_c += 1)
    {
        if (effect->inUseFlag != 0)
        {
            arg->activeEffectsCount = arg->activeEffectsCount + 1;
            if ((effect->callback == NULL) || (effect->callback(effect) == 1))
            {
                if (g_AnmManager->ExecuteScript(&effect->vm) == 0)
                {
                    effect->timer.Tick();
                    effect->head = NULL;
                    if ((effect->is2D == 1) || (effect->is2D == 3))
                    {
                        arg->specialEffectsPtrs[1]->head = effect;
                        arg->specialEffectsPtrs[1] = effect;
                    }
                    else if (effect->is2D == 0)
                    {
                        if (effect->vm.blendMode == 0)
                        {
                            arg->specialEffectsPtrs[0]->head = effect;
                            arg->specialEffectsPtrs[0] = effect;
                        }
                        else
                        {
                            arg->specialEffectsPtrs[3]->head = effect;
                            arg->specialEffectsPtrs[3] = effect;
                        }
                    }
                    else
                    {
                        arg->specialEffectsPtrs[2]->head = effect;
                        arg->specialEffectsPtrs[2] = effect;
                    }
                }
                else
                {
                    effect->inUseFlag = 0;
                }
            }
            else
            {
                effect->inUseFlag = 0;
            }
        }
        effect = effect + 1;
    }
    arg->frameCounter = arg->frameCounter + 1;
    if ((arg->frameCounter % 300 == 100) &&
        (g_GameManager.CheckGameIntegrity() != 0))
    {
        return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
    }
    else
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
}

// FUNCTION: TH07 0x0041ca10
u32 EffectManager::OnDraw(EffectManager *arg)
{
    Effect *effect;

    for (effect = arg->specialEffects[0].head; effect != NULL;
         effect = effect->head)
    {
        effect->vm.pos = effect->pos1;
        effect->vm.pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        effect->vm.pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        g_AnmManager->Draw(&effect->vm);
    }
    for (effect = arg->specialEffects[2].head; effect != NULL;
         effect = effect->head)
    {
        effect->vm.pos = effect->pos1;
        g_AnmManager->DrawBillboard(&effect->vm);
    }
    for (effect = arg->specialEffects[3].head; effect != NULL;
         effect = effect->head)
    {
        effect->vm.pos = effect->pos1;
        effect->vm.pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        effect->vm.pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        g_AnmManager->Draw(&effect->vm);
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0041cb80
i32 EffectManager::UpdateSpecialEffect()
{
    bool bVar1;
    i32 blue;
    i32 green;
    i32 red;
    i32 alpha;
    f32 local_1c;
    f32 local_18;
    f32 local_14;
    f32 local_c;
    Effect *effect;

    effect = this->specialEffects[1].head;
    bVar1 = false;
    if (g_Supervisor.cfg.effectQuality != QUALITY_WORST)
    {
        while ((effect != NULL &&
                ((bVar1 = (bool)(bVar1 ^ 1),
                  g_Supervisor.cfg.effectQuality != QUALITY_MEDIUM || (!bVar1)))))
        {
            if (effect->effectId == 20)
            {
                local_1c = (f32)effect->vm.color.bytes.r;
                local_18 = (f32)effect->vm.color.bytes.g;
                local_14 = (f32)effect->vm.color.bytes.b;
                local_c = (f32)effect->vm.color.bytes.a;
                alpha = local_1c * this->globalColorMultiplierR;
                if (0xff < alpha)
                {
                    alpha = 0xff;
                }
                effect->vm.color.bytes.r = (u8)alpha;
                red = local_18 * this->globalColorMultiplierG;
                if (0xff < red)
                {
                    red = 0xff;
                }
                effect->vm.color.bytes.g = (u8)red;
                green = local_14 * this->globalColorMultiplierB;
                if (0xff < green)
                {
                    green = 0xff;
                }
                effect->vm.color.bytes.b = (u8)green;
                blue = local_c * this->globalColorMultiplierA;
                if (0xff < blue)
                {
                    blue = 0xff;
                }
                effect->vm.color.bytes.a = (u8)blue;
            }
            effect->vm.pos = effect->pos1;
            if (effect->is2D == 1)
            {
                g_AnmManager->DrawBillboard(&effect->vm);
            }
            else
            {
                g_AnmManager->DrawProjected(&effect->vm);
            }
            if (effect->effectId == 20)
            {
                effect->vm.color.bytes.r = (u8)local_1c;
                effect->vm.color.bytes.g = (u8)local_18;
                effect->vm.color.bytes.b = (u8)local_14;
                effect->vm.color.bytes.a = (u8)local_c;
            }
            effect = effect->head;
        }
    }
    return 1;
}

// FUNCTION: TH07 0x0041cde0
ZunResult EffectManager::AddedCallback(EffectManager *arg)
{
    arg->Reset();
    g_Stage.spellcardVmsIdx = 0;
    switch (g_GameManager.currentStage)
    {
    case 0:
    case 1:
        g_Stage.numSpellcardVms = 1;
        if (g_AnmManager->LoadAnms(0x11, "data/eff01.anm", 0x2dc) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 2:
        g_Stage.numSpellcardVms = 1;
        if (g_AnmManager->LoadAnms(0x11, "data/eff02.anm", 0x2dc) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 3:
        g_Stage.numSpellcardVms = 1;
        if (g_AnmManager->LoadAnms(0x11, "data/eff03.anm", 0x2dc) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 4:
        g_Stage.numSpellcardVms = 2;
        if (g_AnmManager->LoadAnms(0x11, "data/eff04.anm", 0x2dc) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(0x12, "data/eff04b.anm", 0x2dd) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 5:
        g_Stage.numSpellcardVms = 2;
        if (g_AnmManager->LoadAnms(0x11, "data/eff05.anm", 0x2dc) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 6:
        g_Stage.numSpellcardVms = 2;
        if (g_AnmManager->LoadAnms(0x11, "data/eff05.anm", 0x2dc) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(0x13, "data/eff06.anm", 0x2de) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 7:
        g_Stage.numSpellcardVms = 1;
        if (g_AnmManager->LoadAnms(0x11, "data/eff02.anm", 0x2dc) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(0x12, "data/eff07.anm", 0x2dd) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 8:
        g_Stage.numSpellcardVms = 2;
        if (g_AnmManager->LoadAnms(0x11, "data/eff07.anm", 0x2dc) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(0x13, "data/eff08.anm", 0x2de) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0041d050
ZunResult EffectManager::DeletedCallback(EffectManager *arg)
{
    g_AnmManager->ReleaseAnm(0x11);
    g_AnmManager->ReleaseAnm(0x12);
    g_AnmManager->ReleaseAnm(0x13);
    g_AnmManager->ReleaseAnm(0x14);
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0041d0a0
ZunResult EffectManager::RegisterChain()
{
    EffectManager *mgr = &g_EffectManager;
    mgr->Reset();
    g_EffectManagerCalcChain.callback = (ChainCallback)OnUpdate;
    g_EffectManagerCalcChain.addedCallback = NULL;
    g_EffectManagerCalcChain.deletedCallback = NULL;
    g_EffectManagerCalcChain.addedCallback =
        (ChainLifecycleCallback)AddedCallback;
    g_EffectManagerCalcChain.deletedCallback =
        (ChainLifecycleCallback)DeletedCallback;
    g_EffectManagerCalcChain.arg = mgr;
    if (g_Chain.AddToCalcChain(&g_EffectManagerCalcChain, 0xb) != 0)
        return ZUN_ERROR;

    g_EffectManagerDrawChain.callback = (ChainCallback)OnDraw;
    g_EffectManagerDrawChain.addedCallback = NULL;
    g_EffectManagerDrawChain.deletedCallback = NULL;
    g_EffectManagerDrawChain.arg = mgr;
    g_Chain.AddToDrawChain(&g_EffectManagerDrawChain, 9);
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0041d150
void EffectManager::CutChain()
{
    g_Chain.Cut(&g_EffectManagerCalcChain);
    g_Chain.Cut(&g_EffectManagerDrawChain);
}
