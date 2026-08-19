#include "EffectManager.hpp"

#include "AnmIdx.hpp"
#include "AnmManager.hpp"
#include "GameManager.hpp"
#include "GameWindow.hpp"
#include "Player.hpp"
#include "Rng.hpp"
#include "Stage.hpp"
#include "ZunMath.hpp"
#include "ZunResult.hpp"
#include "utils.hpp"
#include <algorithm>

EffectTypeInfo g_EffectMapping[34] = {
    {0x2ab, NULL, NULL},
    {0x2ac, NULL, NULL},
    {0x2ad, NULL, NULL},
    {0x2ae, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurst},
    {0x2b3, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast},
    {0x2b4, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast},
    {0x2b5, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast},
    {0x2b6, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast},
    {0x2b7, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast},
    {0x2b8, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast},
    {0x2b9, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast},
    {0x2ba, EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast},
    {0x2bb, NULL, NULL},
    {0x2bc, EffectManager::UpdateOrbitEffect, EffectManager::Init2dEffect},
    {0x2bc, EffectManager::UpdateOrbitEffect, EffectManager::Init2dEffect},
    {0x2bc, EffectManager::UpdateOrbitEffect, EffectManager::Init2dEffect},
    {0x2dc, NULL, NULL},
    {0x2af, EffectManager::UpdateGather60Frames, EffectManager::InitRandomDir},
    {0x2b0, EffectManager::UpdateGather240Frames, EffectManager::InitRandomDir},
    {0x2bd, EffectManager::UpdateNoOp, NULL},
    {0x2bf, EffectManager::UpdateWeatherPhysics, EffectManager::InitWeatherForward},
    {0x2c3, NULL, NULL},
    {0x2c0, EffectManager::UpdateBurstEaseOut30Frames, EffectManager::InitRandomDirWithSpeed},
    {0x304, EffectManager::UpdateAttachToCamera, NULL},
    {0x2c2, EffectManager::UpdateAttachToPlayer, NULL},
    {0x2da, EffectManager::UpdateNoOp, NULL},
    {0x2bf, EffectManager::UpdateWeatherPhysics, EffectManager::InitWeatherVortex},
    {0x2bf, EffectManager::UpdateWeatherPhysics, EffectManager::InitWeatherBackward},
    {0x2db, EffectManager::UpdateNoOp, NULL},
    {0x2b2, EffectManager::UpdateBurst30Frames, EffectManager::InitRandomDir},
    {0x2bf, EffectManager::UpdateWeatherPhysics, EffectManager::InitWeatherSlow},
    {0x2bf, EffectManager::UpdateWeatherPhysics, EffectManager::InitWeatherFalling},
    {0x2c1, EffectManager::UpdateBurstEaseOut30Frames, EffectManager::InitRandomDirWithSpeed},
    {0x2b1, EffectManager::UpdateGather60Frames, EffectManager::InitRandomDir},
};

EffectManager g_EffectManager;

ChainElem g_EffectManagerCalcChain;

ChainElem g_EffectManagerDrawChain;

EffectManager::EffectManager()
{
    Reset();
    this->globalColorMultiplierR = 1.0f;
    this->globalColorMultiplierG = 1.0f;
    this->globalColorMultiplierB = 1.0f;
    this->globalColorMultiplierA = 1.0f;
}

void EffectManager::Reset()
{
    memset(this, 0, sizeof(EffectManager));
}

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

i32 EffectManager::UpdatePhysics(Effect *effect)
{
    effect->pos1 += effect->velocity;
    effect->velocity += effect->acceleration;
    return 1;
}

i32 EffectManager::InitDeceleratingBurst(Effect *effect)
{
    effect->velocity.x = (g_Rng.GetRandomFloatInRange(256.0f) - 128.0f) * 4.0f / 33.0f;
    effect->velocity.y = (g_Rng.GetRandomFloatInRange(256.0f) - 128.0f) * 4.0f / 33.0f;
    effect->velocity.z = 0.0f;
    effect->acceleration = -effect->velocity / 20.0f;
    effect->velocity *= g_Supervisor.effectiveFramerateMultiplier;
    effect->acceleration *= g_Supervisor.effectiveFramerateMultiplier;
    return 0;
}

i32 EffectManager::Init2dEffect(Effect *effect)
{
    effect->is2D = 2;
    return 0;
}

i32 EffectManager::UpdateOrbitEffect(Effect *effect)
{
    f32 fadeOutRatio;
    ZunVec3 local_64;
    f32 cosAngle;
    ZunMatrix local_50;
    f32 sinAngle;
    ZunVec3 local_10;

    local_64.Normalize(&effect->direction);
    sinAngle = sinf(effect->angularVelocity);
    cosAngle = cosf(effect->angularVelocity);

    effect->rotationQuat.x = local_64.x * sinAngle;
    effect->rotationQuat.y = local_64.y * sinAngle;
    effect->rotationQuat.z = local_64.z * sinAngle;
    effect->rotationQuat.w = cosAngle;

    local_50.RotationQuaternion(&effect->rotationQuat);

    local_10.x = local_64.y * 1.0f - local_64.z * 0.0f;
    local_10.y = local_64.z * 0.0f - local_64.x * 1.0f;
    local_10.z = local_64.x * 0.0f - local_64.y * 0.0f;

    if (local_10.LengthSq() < 0.00001f)
    {
        local_64 = ZunVec3(1.0f, 0.0f, 0.0f);
    }
    else
    {
        local_10.Normalize(&local_10);
    }

    local_10 *= effect->radius;
    local_10.TransformCoord(&local_10, &local_50);
    local_10.z *= 6.0f;

    effect->pos1 = local_10 + effect->emitterPosition;

    if ((char)effect->isFadingOut)
    {
        effect->fadeOutTime++;
        if (effect->fadeOutTime >= 16)
        {
            return 0;
        }
        fadeOutRatio = 1.0f - (f32)effect->fadeOutTime / 16.0f;
        effect->vm.color.color = (effect->vm.color.color & 0xffffff) | (u32)(fadeOutRatio * 255.0f)
                                                                           << 24;
        effect->vm.scale.y = 2.0f - fadeOutRatio;
        effect->vm.scale.x = effect->vm.scale.y;
    }
    return 1;
}

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

i32 EffectManager::UpdateGather60Frames(Effect *effect)
{
    f32 distance = 256.0f - effect->timer.AsFloat() * 256.0f / 60.0f;
    effect->pos1 = effect->direction * distance + effect->emitterPosition;
    effect->pos1.z = 0.0f;
    return 1;
}

i32 EffectManager::UpdateAttachToPlayer(Effect *effect)
{
    if ((i32)!effect->vm.currentInstruction)
    {
        return false;
    }

    effect->pos1 = g_Player.positionCenter;
    return true;
}

i32 EffectManager::UpdateGather240Frames(Effect *effect)
{
    f32 distance = 256.0f - effect->timer.AsFloat() * 256.0f / 240.0f;
    effect->pos1 = effect->direction * distance + effect->emitterPosition;
    return 1;
}

i32 EffectManager::UpdateBurst30Frames(Effect *effect)
{
    f32 distance = effect->timer.AsFloat() * 256.0f / 30.0f;
    effect->pos1 = effect->direction * distance + effect->emitterPosition;
    return 1;
}

void EffectManager::ShiftEffectsAfterCameraTeleport(ZunVec3 *shift)
{
    i32 i;
    Effect *effect;

    effect = g_EffectManager.effects;
    for (i = 0; i < MAX_NORMAL_EFFECTS; i++, effect++)
    {
        if (effect->effectId == 20 || effect->effectId == 31)
        {
            effect->basePosition += *shift;
        }
    }
}

void EffectManager::ModifyEffect1eAcceleration()
{
    i32 i;
    Effect *effect;

    effect = g_EffectManager.effects;
    for (i = 0; i < MAX_NORMAL_EFFECTS; i++, effect++)
    {
        if (effect->effectId == 30)
        {
            effect->acceleration.z = -0.01f;
        }
    }
}

i32 EffectManager::UpdateWeatherPhysics(Effect *effect)
{
    ZunVec3 local_10;

    effect->velocity += effect->acceleration;
    effect->basePosition += effect->velocity;
    effect->pos1 = effect->basePosition;

    local_10 = effect->pos1 - g_Stage.cam.pos;
    local_10.Normalize(&local_10);
    f32 dot = g_Stage.cam.lookAtDir.Dot(&local_10);
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

i32 EffectManager::InitWeatherForward(Effect *effect)
{
    i32 chance;
    ZunVec3 camLookAtInv;

    camLookAtInv = -g_Stage.cam.lookAt;

    effect->basePosition = g_Stage.cam.lookAt + g_Stage.cam.pos;
    effect->basePosition.x += g_Rng.GetRandomFloatInRange(120.0f) - 60.0f + camLookAtInv.x / 2.0f;
    effect->basePosition.y += g_Rng.GetRandomFloatInRange(200.0f) - 100.0f + camLookAtInv.y / 2.0f;
    effect->basePosition.z += g_Rng.GetRandomFloatInRange(100.0f) - 100.0f + camLookAtInv.z / 2.0f;
    effect->velocity.x = g_Rng.GetRandomFloatInRange(0.06f) - 0.03f + effect->custom.x;
    effect->velocity.y = g_Rng.GetRandomFloatInRange(0.06f) - 0.03f + effect->custom.y;
    effect->velocity.z = g_Rng.GetRandomFloatInRange(0.1f) + 0.03f + effect->custom.z;
    effect->acceleration.x = g_Rng.GetRandomFloatInRange(0.0002f) - 0.0001f;
    effect->acceleration.y = g_Rng.GetRandomFloatInRange(0.0002f) - 0.0001f;
    effect->velocity = effect->velocity * g_Supervisor.effectiveFramerateMultiplier;
    effect->acceleration = effect->acceleration * g_Supervisor.effectiveFramerateMultiplier;
    effect->is2D = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(0.03141593f) - 0.015707964f;

    chance = g_GameManager.cherry - g_GameManager.globals->cherryStart;
    chance = chance * 100 / g_GameManager.cherryMax;

    if ((u32)chance >= g_Rng.GetRandomU32InRange(100))
    {
        g_AnmManager->SetActiveSprite(&effect->vm, 728);
        effect->vm.color.bytes.r = 255;
        effect->vm.color.bytes.g = 255;
        effect->vm.color.bytes.b = 255;
    }
    return 0;
}

i32 EffectManager::InitWeatherVortex(Effect *effect)
{
    i32 chance;

    effect->basePosition.x = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.y = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.z = g_Rng.GetRandomFloatInRange(100.0f) - 50.0f;
    effect->velocity.x = -effect->basePosition.y / effect->custom.x;
    effect->velocity.y = effect->basePosition.x / effect->custom.x;
    effect->velocity.z = g_Rng.GetRandomFloatInRange(0.1f) + 0.09f;
    effect->basePosition += g_Stage.cam.lookAt / 2.0f + g_Stage.cam.pos;
    effect->velocity = effect->velocity * g_Supervisor.effectiveFramerateMultiplier;
    effect->is2D = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(0.06283186f) - 0.03141593f;

    chance = g_GameManager.cherry - g_GameManager.globals->cherryStart;
    chance = chance * 100 / g_GameManager.cherryMax;

    if ((u32)chance >= g_Rng.GetRandomU32InRange(100))
    {
        g_AnmManager->SetActiveSprite(&effect->vm, 728);
        effect->vm.color.bytes.r = 255;
        effect->vm.color.bytes.g = 255;
        effect->vm.color.bytes.b = 255;
    }
    effect->acceleration.x = 0.0f;
    effect->acceleration.y = 0.0f;
    effect->acceleration.z = 0.0f;
    return 0;
}

i32 EffectManager::InitWeatherBackward(Effect *effect)
{
    effect->basePosition.x = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.y = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.z = g_Rng.GetRandomFloatInRange(100.0f) - 50.0f;
    effect->velocity.x = -effect->basePosition.y / effect->custom.x;
    effect->velocity.y = effect->basePosition.x / effect->custom.x;
    effect->velocity.z = -g_Rng.GetRandomFloatInRange(0.2f) - 0.06f;
    effect->basePosition += g_Stage.cam.lookAt / 2.0f + g_Stage.cam.pos;
    effect->velocity = effect->velocity * g_Supervisor.effectiveFramerateMultiplier;
    effect->is2D = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(0.06283186f) - 0.03141593f;
    g_AnmManager->SetActiveSprite(&effect->vm, 728);
    effect->vm.color.bytes.r = 255;
    effect->vm.color.bytes.g = 255;
    effect->vm.color.bytes.b = 255;
    effect->acceleration.x = 0.0f;
    effect->acceleration.y = 0.0f;
    effect->acceleration.z = 0.0f;
    return 0;
}

i32 EffectManager::InitWeatherSlow(Effect *effect)
{
    effect->basePosition.x = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.y = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.z = g_Rng.GetRandomFloatInRange(100.0f) - 100.0f;
    effect->velocity.x = g_Rng.GetRandomFloatInRange(0.06f) - 0.03f + effect->custom.x;
    effect->velocity.y = g_Rng.GetRandomFloatInRange(0.06f) - 0.03f + effect->custom.y;
    effect->velocity.z = g_Rng.GetRandomFloatInRange(0.02f) + 0.01f + effect->custom.z;
    effect->basePosition += g_Stage.cam.lookAt / 2.0f + g_Stage.cam.pos;
    effect->is2D = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(0.06283186f) - 0.03141593f;
    g_AnmManager->SetActiveSprite(&effect->vm, 728);
    effect->vm.color.bytes.r = 255;
    effect->vm.color.bytes.g = 255;
    effect->vm.color.bytes.b = 255;
    effect->acceleration.x = 0.0f;
    effect->acceleration.y = 0.0f;
    effect->acceleration.z = 0.0f;
    return 0;
}

i32 EffectManager::InitWeatherFalling(Effect *effect)
{
    effect->basePosition.x = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.y = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePosition.z = g_Rng.GetRandomFloatInRange(200.0f) - 0.0f;
    effect->velocity.x = g_Rng.GetRandomFloatInRange(0.06f) - 0.03f + effect->custom.x;
    effect->velocity.y = g_Rng.GetRandomFloatInRange(0.06f) - 0.03f + effect->custom.y;
    effect->velocity.z = -g_Rng.GetRandomFloatInRange(0.1f) + effect->custom.z;
    effect->basePosition += g_Stage.cam.lookAt / 2.0f + g_Stage.cam.pos;
    effect->velocity = effect->velocity * g_Supervisor.effectiveFramerateMultiplier;
    effect->is2D = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(0.06283186f) - 0.03141593f;
    g_AnmManager->SetActiveSprite(&effect->vm, 728);
    effect->vm.angleVel.z *= 2;
    effect->vm.color.bytes.r = 255;
    effect->vm.color.bytes.g = 255;
    effect->vm.color.bytes.b = 255;
    effect->acceleration.x = 0.0f;
    effect->acceleration.y = 0.0f;
    effect->acceleration.z = -0.015f;
    return 0;
}

i32 EffectManager::InitRandomDirWithSpeed(Effect *effect)
{
    f32 angle;

    // double intentionally used here, strangely
    if (effect->custom.x > -990.0)
    {
        angle = utils::AddNormalizeAngle(effect->custom.x, 0.0f);
    }
    else
    {
        angle = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    }
    effect->emitterPosition = effect->pos1;
    effect->emitterPosition.z = 0.0f;
    effect->direction.x = cosf(angle);
    effect->direction.y = sinf(angle);
    effect->direction.z = 0.0f;
    effect->direction *= g_Rng.GetRandomFloatInRange(1.5f) + 1.0f;
    return 0;
}

i32 EffectManager::UpdateBurstEaseOut30Frames(Effect *effect)
{
    f32 fVar1;

    fVar1 = effect->timer.AsFloat() / 90.0f;
    fVar1 = 1.0f - (1.0f - fVar1) * (1.0f - fVar1);
    effect->pos1 = fVar1 * effect->direction * 128.0f + effect->emitterPosition;
    effect->pos1.z = 0.0f;
    return 1;
}

i32 EffectManager::UpdateAttachToCamera(Effect *effect)
{
    effect->is2D = 1;
    effect->basePosition = g_Stage.cam.lookAt + g_Stage.cam.pos;
    effect->pos1 = effect->basePosition;
    effect->pos1.z = 0.0f;
    effect->is2D = 3;
    return 1;
}

i32 EffectManager::UpdateNoOp(Effect *effect)
{
    (void)effect;
    return 1;
}

Effect *EffectManager::SpawnEffect(i32 effectId, ZunVec3 *pos, i32 numParticles, u32 color)
{
    i32 i;
    Effect *effect;

    effect = &this->effects[this->nextIndex];
    for (i = 0; i < MAX_NORMAL_EFFECTS; i++)
    {
        this->nextIndex++;
        if (this->nextIndex >= MAX_NORMAL_EFFECTS)
        {
            this->nextIndex = 0;
        }
        if (effect->inUseFlag)
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
        effect->custom = ZunVec3(0.0f, 0.0f, 0.0f);
        if (g_EffectMapping[effectId].initCallback &&
            g_EffectMapping[effectId].initCallback(effect))
        {
            effect->inUseFlag = 0;
        }
        effect->prevPos = effect->pos1;
        effect->vm.UpdatePrev();
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

    return i >= MAX_NORMAL_EFFECTS ? &this->effects[MAX_EFFECTS] : effect;
}

Effect *EffectManager::SpawnMovingParticles(i32 effectId, ZunVec3 *pos, ZunVec3 *velocity,
                                            i32 numParticles, u32 color)
{
    i32 i;
    Effect *effect;

    effect = &this->effects[this->nextIndex];

    for (i = 0; i < MAX_NORMAL_EFFECTS; i++)
    {
        this->nextIndex++;
        if (this->nextIndex >= MAX_NORMAL_EFFECTS)
        {
            this->nextIndex = 0;
        }
        if (effect->inUseFlag)
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
        if (g_EffectMapping[effectId].initCallback &&
            g_EffectMapping[effectId].initCallback(effect))
        {
            effect->inUseFlag = 0;
        }
        effect->prevPos = effect->pos1;
        effect->vm.UpdatePrev();
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

    return i >= MAX_NORMAL_EFFECTS ? &this->effects[MAX_EFFECTS] : effect;
}

Effect *EffectManager::SpawnSpecialEffect(i32 effectId, ZunVec3 *pos, i32 effectIdx, i32 param_4,
                                          u32 color)
{
    (void)param_4;

    Effect *effect;

    effect = &this->effects[effectIdx + MAX_NORMAL_EFFECTS];
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
    effect->custom = ZunVec3(0.0f, 0.0f, 0.0f);
    if (g_EffectMapping[effectId].initCallback)
    {
        if (g_EffectMapping[effectId].initCallback(effect))
        {
            effect->inUseFlag = 0;
        }
    }
    effect->prevPos = effect->pos1;
    effect->vm.UpdatePrev();
    return effect;
}

u32 EffectManager::OnUpdate(EffectManager *arg)
{
    i32 i;
    Effect *effect;

    effect = arg->effects;
    arg->activeEffects = 0;
    arg->layerPtrs[0] = &arg->layer0;
    arg->layerPtrs[1] = &arg->layer1;
    arg->layerPtrs[2] = &arg->layer2;
    arg->layerPtrs[3] = &arg->layer3;
    arg->layer0.next = NULL;
    arg->layer1.next = NULL;
    arg->layer2.next = NULL;
    arg->layer3.next = NULL;
    for (i = 0; i < MAX_EFFECTS; i++, effect++)
    {
        if (!effect->inUseFlag)
        {
            continue;
        }

        bool firstUpdate = effect->timer.GetCurrent() == 0;

        effect->vm.UpdatePrev();
        effect->prevPos = effect->pos1;

        arg->activeEffects++;
        if (effect->callback && effect->callback(effect) != 1)
        {
            effect->inUseFlag = 0;
            continue;
        }

        if (g_AnmManager->ExecuteScript(&effect->vm))
        {
            effect->inUseFlag = 0;
            continue;
        }

        if (firstUpdate)
        {
            effect->prevPos = effect->pos1;
            effect->vm.UpdatePrev();
        }

        effect->timer++;
        effect->next = NULL;
        if (effect->is2D == 1 || effect->is2D == 3)
        {
            arg->layerPtrs[1]->next = effect;
            arg->layerPtrs[1] = effect;
        }
        else if (!effect->is2D)
        {
            if (effect->vm.blendMode != 0)
            {
                arg->layerPtrs[3]->next = effect;
                arg->layerPtrs[3] = effect;
            }
            else
            {
                arg->layerPtrs[0]->next = effect;
                arg->layerPtrs[0] = effect;
            }
        }
        else
        {
            arg->layerPtrs[2]->next = effect;
            arg->layerPtrs[2] = effect;
        }
    }
    arg->frameCounter++;
    if (arg->frameCounter % 300 == 100 && g_GameManager.CheckGameIntegrity())
    {
        return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
    }
    else
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
}

u32 EffectManager::OnDraw(EffectManager *arg)
{
    auto sortAndDraw = [](Effect *layerHead, bool isBillboard) {
        Effect *active[409];
        i32 count = 0;
        Effect *effect = layerHead->next;
        while (effect)
        {
            active[count++] = effect;
            effect = effect->next;
        }

        std::sort(active, active + count, [](Effect *a, Effect *b) {
            i32 texA = a->vm.sprite ? a->vm.sprite->sourceFileIndex : -1;
            i32 texB = b->vm.sprite ? b->vm.sprite->sourceFileIndex : -1;
            return texA < texB;
        });

        for (i32 i = 0; i < count; i++)
        {
            active[i]->vm.pos = active[i]->prevPos.Lerp(active[i]->pos1, g_RenderAlpha);
            if (isBillboard)
            {
                g_AnmManager->DrawBillboard(&active[i]->vm);
            }
            else
            {
                active[i]->vm.pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
                active[i]->vm.pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
                g_AnmManager->Draw(&active[i]->vm);
            }
        }
    };

    sortAndDraw(&arg->layer0, false);
    sortAndDraw(&arg->layer2, true);
    sortAndDraw(&arg->layer3, false);
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

i32 EffectManager::DrawLayer1Effects()
{
    i32 temp;
    f32 r;
    f32 g;
    f32 b;
    i32 counter;
    f32 a;
    Effect *effect;

    effect = this->layer1.next;
    counter = 0;

    if (g_Supervisor.cfg.effectQuality == QUALITY_WORST)
    {
        return 1;
    }

    while (effect)
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

            temp = (i32)(r * this->globalColorMultiplierR);
            effect->vm.color.bytes.r = temp > 255 ? 255 : temp;

            temp = (i32)(g * this->globalColorMultiplierG);
            effect->vm.color.bytes.g = temp > 255 ? 255 : temp;

            temp = (i32)(b * this->globalColorMultiplierB);
            effect->vm.color.bytes.b = temp > 255 ? 255 : temp;

            temp = (i32)(a * this->globalColorMultiplierA);
            effect->vm.color.bytes.a = temp > 255 ? 255 : temp;
            effect->vm.prevColor = effect->vm.color;
        }

        effect->vm.pos = effect->prevPos.Lerp(effect->pos1, g_RenderAlpha);
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

        effect = effect->next;
    }

    return 1;
}

ZunResult EffectManager::AddedCallback(EffectManager *arg)
{
    arg->Reset();
    g_Stage.spellcardVmsIdx = 0;
    switch (g_GameManager.currentStage)
    {
    case 0:
    case 1:
        g_Stage.numSpellcardVms = 1;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff01.anm", ANM_OFFSET_EFFECTS) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 2:
        g_Stage.numSpellcardVms = 1;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff02.anm", ANM_OFFSET_EFFECTS) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 3:
        g_Stage.numSpellcardVms = 1;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff03.anm", ANM_OFFSET_EFFECTS) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 4:
        g_Stage.numSpellcardVms = 2;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff04.anm", ANM_OFFSET_EFFECTS) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS2, "data/eff04b.anm", ANM_OFFSET_EFFECTS2) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 5:
        g_Stage.numSpellcardVms = 2;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff05.anm", ANM_OFFSET_EFFECTS) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 6:
        g_Stage.numSpellcardVms = 2;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff05.anm", ANM_OFFSET_EFFECTS) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS3, "data/eff06.anm", ANM_OFFSET_EFFECTS3) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 7:
        g_Stage.numSpellcardVms = 1;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff02.anm", ANM_OFFSET_EFFECTS) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS2, "data/eff07.anm", ANM_OFFSET_EFFECTS2) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 8:
        g_Stage.numSpellcardVms = 2;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff07.anm", ANM_OFFSET_EFFECTS) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS3, "data/eff08.anm", ANM_OFFSET_EFFECTS3) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
    }
    return ZUN_SUCCESS;
}

ZunResult EffectManager::DeletedCallback(EffectManager *arg)
{
    (void)arg;

    g_AnmManager->ReleaseAnm(17);
    g_AnmManager->ReleaseAnm(18);
    g_AnmManager->ReleaseAnm(19);
    g_AnmManager->ReleaseAnm(20);
    return ZUN_SUCCESS;
}

ZunResult EffectManager::RegisterChain()
{
    EffectManager *mgr = &g_EffectManager;
    mgr->Reset();
    g_EffectManagerCalcChain.callback = (ChainCallback)OnUpdate;
    g_EffectManagerCalcChain.addedCallback = NULL;
    g_EffectManagerCalcChain.deletedCallback = NULL;
    g_EffectManagerCalcChain.addedCallback = (ChainLifecycleCallback)AddedCallback;
    g_EffectManagerCalcChain.deletedCallback = (ChainLifecycleCallback)DeletedCallback;
    g_EffectManagerCalcChain.arg = mgr;
    if (g_Chain.AddToCalcChain(&g_EffectManagerCalcChain, 11))
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

void EffectManager::CutChain()
{
    g_Chain.Cut(&g_EffectManagerCalcChain);
    g_Chain.Cut(&g_EffectManagerDrawChain);
}
