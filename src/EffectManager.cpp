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
    effect->acceleration = -effect->velocity / 19.0f;
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
    effect->acceleration = -effect->velocity / 20.0f;
    effect->velocity *= g_Supervisor.effectiveFramerateMultiplier;
    effect->acceleration *= g_Supervisor.effectiveFramerateMultiplier;
    return 0;
}

// FUNCTION: TH07 0x0041a730
i32 EffectManager::Init2dEffect(Effect *effect)
{
    effect->is2D = 2;
    return 0;
}

#pragma var_order(local_10, sinAngle, local_50, cosAngle, local_64, fadeOutRatio)
// FUNCTION: TH07 0x0041a750
i32 EffectManager::UpdateOrbitEffect(Effect *effect)
{
    f32 fadeOutRatio;
    D3DXVECTOR3 local_64;
    f32 cosAngle;
    D3DXMATRIX local_50;
    f32 sinAngle;
    D3DXVECTOR3 local_10;

    D3DXVec3Normalize(&local_64, &effect->direction);
    sinAngle = sinf(effect->angularVelocity);
    cosAngle = cosf(effect->angularVelocity);

    effect->rotationQuat.x = local_64.x * sinAngle;
    effect->rotationQuat.y = local_64.y * sinAngle;
    effect->rotationQuat.z = local_64.z * sinAngle;
    effect->rotationQuat.w = cosAngle;

    D3DXMatrixRotationQuaternion(&local_50, &effect->rotationQuat);

    local_10.x = local_64.y * 1.0f - local_64.z * 0.0f;
    local_10.y = local_64.z * 0.0f - local_64.x * 1.0f;
    local_10.z = local_64.x * 0.0f - local_64.y * 0.0f;

    if (D3DXVec3LengthSq(&local_10) < 0.00001f)
    {
        local_64 = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
    }
    else
    {
        D3DXVec3Normalize(&local_10, &local_10);
    }

    local_10 *= effect->radius;
    D3DXVec3TransformCoord(&local_10, &local_10, &local_50);
    local_10.z *= 6.0f;

    effect->pos1 = local_10 + effect->emitterPosition;

    if ((char)effect->isFadingOut != 0)
    {
        effect->fadeOutTime++;
        if (effect->fadeOutTime >= 16)
        {
            return 0;
        }
        fadeOutRatio = 1.0f - (f32)effect->fadeOutTime / 16.0f;
        effect->vm.color.color =
            (effect->vm.color.color & 0xffffff) | (u32)(fadeOutRatio * 255.0f) << 0x18;
        effect->vm.scale.y = 2.0f - fadeOutRatio;
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
    f32 distance = 256.0f - (effect->timer.AsFloat() * 256.0f) / 60.0f;
    effect->pos1 = effect->direction * distance + effect->emitterPosition;
    effect->pos1.z = 0.0f;
    return 1;
}

// FUNCTION: TH07 0x0041abe0
i32 EffectManager::UpdateAttachToPlayer(Effect *effect)
{
    if ((i32)(effect->vm.currentInstruction == NULL))
    {
        return false;
    }

    effect->pos1 = g_Player.positionCenter;
    return true;
}

// FUNCTION: TH07 0x0041ac30
i32 EffectManager::UpdateGather240Frames(Effect *effect)
{
    f32 distance = 256.0f - (effect->timer.AsFloat() * 256.0f) / 240.0f;
    effect->pos1 = effect->direction * distance + effect->emitterPosition;
    return 1;
}

// FUNCTION: TH07 0x0041ad10
i32 EffectManager::UpdateBurst30Frames(Effect *effect)
{
    f32 distance = (effect->timer.AsFloat() * 256.0f) / 30.0f;
    effect->pos1 = effect->direction * distance + effect->emitterPosition;
    return 1;
}

#pragma var_order(effect, i)
// FUNCTION: TH07 0x0041adf0
void EffectManager::DoSomethingWithEffects(D3DXVECTOR3 *param_1)
{
    i32 i;
    Effect *effect;

    effect = g_EffectManager.effects;
    for (i = 0; i < 400; i++, effect++)
    {
        if ((effect->effectId == 20) || (effect->effectId == 0x1f))
        {
            effect->basePosition += *param_1;
        }
    }
}

#pragma var_order(effect, i)
// FUNCTION: TH07 0x0041ae90
void EffectManager::ModifyEffect1eAcceleration()
{
    i32 i;
    Effect *effect;

    effect = g_EffectManager.effects;
    for (i = 0; i < 400; i++, effect++)
    {
        if (effect->effectId == 0x1e)
        {
            effect->acceleration.z = -0.01f;
        }
    }
}

#pragma var_order(local_10, dot)
// FUNCTION: TH07 0x0041aef0
i32 EffectManager::UpdateWeatherPhysics(Effect *effect)
{
    D3DXVECTOR3 local_10;

    effect->velocity += effect->acceleration;
    effect->basePosition += effect->velocity;
    effect->pos1 = effect->basePosition;

    local_10 = effect->pos1 - g_Stage.camPos;
    D3DXVec3Normalize(&local_10, &local_10);
    f32 dot = D3DXVec3Dot(&g_Stage.camLookAtDir, &local_10);
    if (dot < 0.94f)
    {
        return 0;
    }

    effect->vm.SetRotationZ(utils::AddNormalizeAngle(effect->vm.rotation.z, effect->vm.rotation.x));
    effect->vm.updateRotation = 1;
    if (effect->pos1.z >= 0.0f)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

#pragma var_order(camLookAtInv, chance)
// FUNCTION: TH07 0x0041b0b0
i32 EffectManager::InitWeatherForward(Effect *effect)
{
    i32 chance;
    D3DXVECTOR3 camLookAtInv;

    camLookAtInv = -g_Stage.camLookAt;

    effect->basePosition = g_Stage.camLookAt + g_Stage.camPos;
    effect->basePosition.x += (g_Rng.GetRandomFloatInRange(120.0f) - 60.0f) + camLookAtInv.x / 2.0f;
    effect->basePosition.y += (g_Rng.GetRandomFloatInRange(200.0f) - 100.0f) + camLookAtInv.y / 2.0f;
    effect->basePosition.z += (g_Rng.GetRandomFloatInRange(100.0f) - 100.0f) + camLookAtInv.z / 2.0f;
    effect->velocity.x =
        (g_Rng.GetRandomFloatInRange(0.06f) - 0.03f) + effect->custom.x;
    effect->velocity.y =
        (g_Rng.GetRandomFloatInRange(0.06f) - 0.03f) + effect->custom.y;
    effect->velocity.z = g_Rng.GetRandomFloatInRange(0.1f) + 0.03f + effect->custom.z;
    effect->acceleration.x = g_Rng.GetRandomFloatInRange(0.0002f) - 0.0001f;
    effect->acceleration.y = g_Rng.GetRandomFloatInRange(0.0002f) - 0.0001f;
    effect->velocity = effect->velocity * g_Supervisor.effectiveFramerateMultiplier;
    effect->acceleration = effect->acceleration * g_Supervisor.effectiveFramerateMultiplier;
    effect->is2D = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(0.03141593f) - 0.015707964f;

    chance = g_GameManager.cherry - g_GameManager.globals->cherryStart;
    chance = (chance * 100) / g_GameManager.cherryMax;

    if (chance >= g_Rng.GetRandomU32InRange(100))
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
    i32 chance;

    effect->basePosition.x = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.y = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.z = g_Rng.GetRandomFloatInRange(100.0f) - 50.0f;
    effect->velocity.x = -effect->basePosition.y / effect->custom.x;
    effect->velocity.y = effect->basePosition.x / effect->custom.x;
    effect->velocity.z = g_Rng.GetRandomFloatInRange(0.1f) + 0.09f;
    effect->basePosition += g_Stage.camLookAt / 2.0f + g_Stage.camPos;
    effect->velocity = effect->velocity * g_Supervisor.effectiveFramerateMultiplier;
    effect->is2D = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(0.06283186f) - 0.03141593f;

    chance = g_GameManager.cherry - g_GameManager.globals->cherryStart;
    chance = (chance * 100) / g_GameManager.cherryMax;

    if (chance >= g_Rng.GetRandomU32InRange(100))
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
    effect->basePosition += g_Stage.camLookAt / 2.0f + g_Stage.camPos;
    effect->velocity = effect->velocity * g_Supervisor.effectiveFramerateMultiplier;
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
    effect->basePosition += g_Stage.camLookAt / 2.0f + g_Stage.camPos;
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
    effect->basePosition += g_Stage.camLookAt / 2.0f + g_Stage.camPos;
    effect->velocity = effect->velocity * g_Supervisor.effectiveFramerateMultiplier;
    effect->is2D = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(0.06283186f) - 0.03141593f;
    g_AnmManager->SetActiveSprite(&effect->vm, 0x2d8);
    effect->vm.angleVel.z *= 2;
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
    if (effect->custom.x > -990.0)
    {
        local_8 = utils::AddNormalizeAngle(effect->custom.x, 0.0f);
    }
    else
    {
        local_8 = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
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

    fVar1 = effect->timer.AsFloat() / 90.0f;
    fVar1 = 1.0f - (1.0f - fVar1) * (1.0f - fVar1);
    effect->pos1 = fVar1 * effect->direction * 128.0f + effect->emitterPosition;
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

#pragma var_order(effect, i)
// FUNCTION: TH07 0x0041c1c0
Effect *EffectManager::SpawnParticles(i32 effectId, D3DXVECTOR3 *pos,
                                      i32 numParticles, D3DCOLOR color)
{
    i32 i;
    Effect *effect;

    effect = &this->effects[this->nextIndex];
    for (i = 0; i < 400; i++)
    {
        this->nextIndex++;
        if (this->nextIndex >= 400)
        {
            this->nextIndex = 0;
        }
        if (effect->inUseFlag != 0)
        {
            if (this->nextIndex == 0)
            {
                effect = this->effects;
            }
            else
            {
                effect++;
            }
            continue;
        }

        effect->is2D = 0;
        effect->inUseFlag = 1;
        effect->effectId = (u8)effectId;
        effect->pos1 = *pos;
        g_AnmManager->SetAnmIdxAndExecuteScript(&effect->vm, g_EffectMapping[effectId].anmId);
        effect->vm.zWriteDisable = 1;
        effect->vm.color.color = color;
        effect->callback = g_EffectMapping[effectId].updateCallback;
        effect->timer = 0;
        effect->isFadingOut = 0;
        effect->fadeOutTime = 0;
        effect->custom = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        if ((g_EffectMapping[effectId].initCallback != NULL) &&
            (g_EffectMapping[effectId].initCallback(effect) != 0))
        {
            effect->inUseFlag = 0;
        }
        numParticles--;
        if (numParticles == 0)
        {
            break;
        }
        if (this->nextIndex == 0)
        {
            effect = this->effects;
        }
        else
        {
            effect++;
        }
    }

    return i >= 400 ? &this->effects[0x198] : effect;
}

#pragma var_order(effect, i)
// FUNCTION: TH07 0x0041c400
Effect *EffectManager::SpawnMovingParticles(i32 effectId, D3DXVECTOR3 *pos,
                                            D3DXVECTOR3 *velocity,
                                            i32 numParticles, D3DCOLOR color)
{
    i32 i;
    Effect *effect;

    effect = &this->effects[this->nextIndex];

    for (i = 0; i < 400; i++)
    {
        this->nextIndex++;
        if (this->nextIndex >= 400)
        {
            this->nextIndex = 0;
        }
        if (effect->inUseFlag != 0)
        {
            if (this->nextIndex == 0)
            {
                effect = this->effects;
            }
            else
            {
                effect++;
            }
            continue;
        }

        effect->is2D = 0;
        effect->inUseFlag = 1;
        effect->effectId = effectId;
        effect->pos1 = *pos;
        g_AnmManager->SetAnmIdxAndExecuteScript(&effect->vm, g_EffectMapping[effectId].anmId);
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
        numParticles--;
        if (numParticles == 0)
        {
            break;
        }
        if (this->nextIndex == 0)
        {
            effect = this->effects;
        }
        else
        {
            effect++;
        }
    }

    return i >= 400 ? &this->effects[0x198] : effect;
}

// FUNCTION: TH07 0x0041c610
Effect *EffectManager::SpawnEffect(i32 effectId, D3DXVECTOR3 *pos, i32 param_3,
                                   i32 param_4, D3DCOLOR color)
{
    Effect *effect;

    effect = &this->effects[param_3 + 400];
    effect->is2D = 0;
    effect->inUseFlag = 1;
    effect->effectId = effectId;
    effect->pos1 = *pos;
    g_AnmManager->SetAnmIdxAndExecuteScript(&effect->vm, g_EffectMapping[effectId].anmId);
    effect->vm.zWriteDisable = 1;
    effect->vm.color.color = color;
    effect->callback = g_EffectMapping[effectId].updateCallback;
    effect->timer = 0;
    effect->isFadingOut = 0;
    effect->fadeOutTime = 0;
    effect->custom = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    if (g_EffectMapping[effectId].initCallback != NULL)
    {
        if (g_EffectMapping[effectId].initCallback(effect) != 0)
        {
            effect->inUseFlag = 0;
        }
    }
    return effect;
}

#pragma var_order(effect, i)
// FUNCTION: TH07 0x0041c790
u32 EffectManager::OnUpdate(EffectManager *arg)
{
    i32 i;
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
    for (i = 0; i < 0x198; i++, effect++)
    {
        if (effect->inUseFlag == 0)
        {
            continue;
        }

        arg->activeEffectsCount++;
        if (effect->callback != NULL && effect->callback(effect) != 1)
        {
            effect->inUseFlag = 0;
            continue;
        }

        if (g_AnmManager->ExecuteScript(&effect->vm) != 0)
        {
            effect->inUseFlag = 0;
            continue;
        }

        effect->timer++;
        effect->head = NULL;
        if ((effect->is2D == 1) || (effect->is2D == 3))
        {
            arg->specialEffectsPtrs[1]->head = effect;
            arg->specialEffectsPtrs[1] = effect;
        }
        else if (effect->is2D == 0)
        {
            if (effect->vm.blendMode != 0)
            {
                arg->specialEffectsPtrs[3]->head = effect;
                arg->specialEffectsPtrs[3] = effect;
            }
            else
            {
                arg->specialEffectsPtrs[0]->head = effect;
                arg->specialEffectsPtrs[0] = effect;
            }
        }
        else
        {
            arg->specialEffectsPtrs[2]->head = effect;
            arg->specialEffectsPtrs[2] = effect;
        }
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

    effect = arg->specialEffects[0].head;
    while (effect != NULL)
    {
        effect->vm.pos = effect->pos1;
        effect->vm.pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        effect->vm.pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        g_AnmManager->Draw(&effect->vm);
        effect = effect->head;
    }
    effect = arg->specialEffects[2].head;
    while (effect != NULL)
    {
        effect->vm.pos = effect->pos1;
        g_AnmManager->DrawBillboard(&effect->vm);
        effect = effect->head;
    }
    effect = arg->specialEffects[3].head;
    while (effect != NULL)
    {
        effect->vm.pos = effect->pos1;
        effect->vm.pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        effect->vm.pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        g_AnmManager->Draw(&effect->vm);
        effect = effect->head;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(effect, a, counter, b, g, r, temp)
// FUNCTION: TH07 0x0041cb80
i32 EffectManager::UpdateSpecialEffect()
{
    int temp;
    f32 r;
    f32 g;
    f32 b;
    int counter;
    f32 a;
    Effect *effect;

    effect = this->specialEffects[1].head;
    counter = 0;

    if (g_Supervisor.cfg.effectQuality == QUALITY_WORST)
    {
        return 1;
    }

    while (effect != NULL)
    {
        counter++;
        if (g_Supervisor.cfg.effectQuality == QUALITY_MEDIUM)
        {
            if (counter & 1)
            {
                return 1;
            }
        }

        if (effect->effectId == 20)
        {
            r = (f32)effect->vm.color.bytes.r;
            g = (f32)effect->vm.color.bytes.g;
            b = (f32)effect->vm.color.bytes.b;
            a = (f32)effect->vm.color.bytes.a;

            temp = r * this->globalColorMultiplierR;
            effect->vm.color.bytes.r = (temp > 255) ? 255 : temp;

            temp = g * this->globalColorMultiplierG;
            effect->vm.color.bytes.g = (temp > 255) ? 255 : temp;

            temp = b * this->globalColorMultiplierB;
            effect->vm.color.bytes.b = (temp > 255) ? 255 : temp;

            temp = a * this->globalColorMultiplierA;
            effect->vm.color.bytes.a = (temp > 255) ? 255 : temp;
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
            effect->vm.color.bytes.r = (u8)r;
            effect->vm.color.bytes.g = (u8)g;
            effect->vm.color.bytes.b = (u8)b;
            effect->vm.color.bytes.a = (u8)a;
        }

        effect = effect->head;
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
    {
        return ZUN_ERROR;
    }

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
