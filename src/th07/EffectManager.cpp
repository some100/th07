#include "EffectManager.hpp"

#include "AnmManager.hpp"
#include "GameManager.hpp"
#include "Player.hpp"
#include "Rng.hpp"
#include "Stage.hpp"
#include "ZunMath.hpp"
#include "ZunResult.hpp"
#include "utils.hpp"

// TODO: a lot of these names suck. find better names and remove script
// comments
// GLOBAL: TH07 0x0049efc0
EffectTypeInfo g_EffectMapping[34] = {
    {ANM_SCRIPT_BULLETS_ENEMY_DEATH_EXPLOSION, NULL, NULL},
    {ANM_SCRIPT_BULLETS_ENEMY_UNK1, NULL, NULL}, // script29
    {ANM_SCRIPT_BULLETS_ENEMY_UNK2, NULL, NULL}, // script30
    {ANM_SCRIPT_BULLETS_ENEMY_UNK3,
     EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurst}, // script31
    {ANM_SCRIPT_BULLETS_ENEMY_UNK8,
     EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast}, // script36
    {ANM_SCRIPT_BULLETS_ENEMY_UNK9,
     EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast}, // script37
    {ANM_SCRIPT_BULLETS_ENEMY_UNK10,
     EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast}, // script38
    {ANM_SCRIPT_BULLETS_ENEMY_UNK11,
     EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast}, // script39
    {ANM_SCRIPT_BULLETS_ENEMY_UNK12,
     EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast}, // script40
    {ANM_SCRIPT_BULLETS_ENEMY_UNK13,
     EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast}, // script41
    {ANM_SCRIPT_BULLETS_ENEMY_UNK14,
     EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast}, // script42
    {ANM_SCRIPT_BULLETS_ENEMY_UNK15,
     EffectManager::UpdatePhysics, EffectManager::InitDeceleratingBurstFast}, // script43
    {ANM_SCRIPT_BULLETS_ENEMY_UNK16, NULL, NULL},                             // script44
    {ANM_SCRIPT_BULLETS_ENEMY_UNK17,
     EffectManager::UpdateOrbitEffect, EffectManager::Init2dEffect}, // script45
    {ANM_SCRIPT_BULLETS_ENEMY_UNK17,
     EffectManager::UpdateOrbitEffect, EffectManager::Init2dEffect},
    {ANM_SCRIPT_BULLETS_ENEMY_UNK17,
     EffectManager::UpdateOrbitEffect, EffectManager::Init2dEffect},
    {ANM_SCRIPT_EFFECTS_SPELLCARD_BG_ARRAY, NULL, NULL},
    {ANM_SCRIPT_BULLETS_ENEMY_UNK4,
     EffectManager::UpdateGather60Frames, EffectManager::InitRandomDir}, // script32
    {ANM_SCRIPT_BULLETS_ENEMY_UNK5,
     EffectManager::UpdateGather240Frames, EffectManager::InitRandomDir}, // script33
    {ANM_SCRIPT_BULLETS_ENEMY_UNK18,
     EffectManager::UpdateNoOp, NULL}, // script46
    {ANM_SCRIPT_BULLETS_FALLING_WEATHER,
     EffectManager::UpdateWeatherPhysics, EffectManager::InitWeatherForward}, // script48
    {ANM_SCRIPT_BULLETS_ENEMY_UNK21, NULL, NULL},                             // script52
    {ANM_SCRIPT_BULLETS_ENEMY_UNK19,
     EffectManager::UpdateBurstEaseOut30Frames, EffectManager::InitRandomDirWithSpeed}, // script49
    {ANM_SCRIPT_STAGE_BG_SHOW_CRESCENT_ST3,
     EffectManager::UpdateAttachToCamera, NULL},
    {ANM_SCRIPT_BULLETS_ENEMY_UNK20,
     EffectManager::UpdateAttachToPlayer, NULL}, // script51
    {ANM_SCRIPT_BULLETS_SHOW_SPELL_RING,
     EffectManager::UpdateNoOp, NULL},
    {ANM_SCRIPT_BULLETS_FALLING_WEATHER,
     EffectManager::UpdateWeatherPhysics, EffectManager::InitWeatherVortex},
    {ANM_SCRIPT_BULLETS_FALLING_WEATHER,
     EffectManager::UpdateWeatherPhysics, EffectManager::InitWeatherBackward},
    {ANM_SCRIPT_BULLETS_SHOW_BORDER,
     EffectManager::UpdateNoOp, NULL},
    {ANM_SCRIPT_BULLETS_ENEMY_UNK7,
     EffectManager::UpdateBurst30Frames, EffectManager::InitRandomDir}, // script35
    {ANM_SCRIPT_BULLETS_FALLING_WEATHER,
     EffectManager::UpdateWeatherPhysics, EffectManager::InitWeatherSlow},
    {ANM_SCRIPT_BULLETS_FALLING_WEATHER,
     EffectManager::UpdateWeatherPhysics, EffectManager::InitWeatherFalling},
    {ANM_SCRIPT_BULLETS_RANDOM_CHERRY_PETAL,
     EffectManager::UpdateBurstEaseOut30Frames, EffectManager::InitRandomDirWithSpeed},
    {ANM_SCRIPT_BULLETS_ENEMY_UNK6,
     EffectManager::UpdateGather60Frames, EffectManager::InitRandomDir}, // script34
};

// GLOBAL: TH07 0x012fe250
EffectManager g_EffectManager;

// GLOBAL: TH07 0x013478f8
ChainElem g_EffectManagerCalcChain;

// GLOBAL: TH07 0x01347918
ChainElem g_EffectManagerDrawChain;

// FUNCTION: TH07 0x0041a210
EffectManager::EffectManager()
{
    Reset();
    this->globalColorMultiplierR = 1.0f;
    this->globalColorMultiplierG = 1.0f;
    this->globalColorMultiplierB = 1.0f;
    this->globalColorMultiplierA = 1.0f;
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
    effect->accel = -effect->velocity / 19.0f;
    effect->velocity *= g_Supervisor.effectiveFramerateMultiplier;
    effect->accel *= g_Supervisor.effectiveFramerateMultiplier;
    return 0;
}

// FUNCTION: TH07 0x0041a4f0
i32 EffectManager::UpdatePhysics(Effect *effect)
{
    effect->pos += effect->velocity;
    effect->velocity += effect->accel;
    return 1;
}

// FUNCTION: TH07 0x0041a5a0
i32 EffectManager::InitDeceleratingBurst(Effect *effect)
{
    effect->velocity.x =
        (g_Rng.GetRandomFloatInRange(256.0f) - 128.0f) * 4.0f / 33.0f;
    effect->velocity.y =
        (g_Rng.GetRandomFloatInRange(256.0f) - 128.0f) * 4.0f / 33.0f;
    effect->velocity.z = 0.0f;
    effect->accel = -effect->velocity / 20.0f;
    effect->velocity *= g_Supervisor.effectiveFramerateMultiplier;
    effect->accel *= g_Supervisor.effectiveFramerateMultiplier;
    return 0;
}

// FUNCTION: TH07 0x0041a730
i32 EffectManager::Init2dEffect(Effect *effect)
{
    effect->drawType = 2;
    return 0;
}

#pragma var_order(local_10, sinAngle, local_50, cosAngle, local_64, fadeOutRatio)
// FUNCTION: TH07 0x0041a750
i32 EffectManager::UpdateOrbitEffect(Effect *effect)
{
    f32 fadeOutRatio;
    Float3 local_64;
    f32 cosAngle;
    D3DXMATRIX local_50;
    f32 sinAngle;
    Float3 local_10;

    D3DXVec3Normalize(local_64.asD3DX(), effect->direction.asD3DX());
    sinAngle = sinf(effect->angleVel);
    cosAngle = cosf(effect->angleVel);

    effect->rotationQuat.x = local_64.x * sinAngle;
    effect->rotationQuat.y = local_64.y * sinAngle;
    effect->rotationQuat.z = local_64.z * sinAngle;
    effect->rotationQuat.w = cosAngle;

    D3DXMatrixRotationQuaternion(&local_50, &effect->rotationQuat);

    local_10.x = local_64.y * 1.0f - local_64.z * 0.0f;
    local_10.y = local_64.z * 0.0f - local_64.x * 1.0f;
    local_10.z = local_64.x * 0.0f - local_64.y * 0.0f;

    if (D3DXVec3LengthSq(local_10.asD3DX()) < 0.00001f)
    {
        local_64 = Float3(1.0f, 0.0f, 0.0f);
    }
    else
    {
        D3DXVec3Normalize(local_10.asD3DX(), local_10.asD3DX());
    }

    local_10 *= effect->radius;
    D3DXVec3TransformCoord(local_10.asD3DX(), local_10.asD3DX(), &local_50);
    local_10.z *= 6.0f;

    effect->pos = local_10 + effect->emitterPos;

    if ((char)effect->isFadingOut)
    {
        effect->fadeOutTime++;
        if (effect->fadeOutTime >= 16)
        {
            return 0;
        }
        fadeOutRatio = 1.0f - (f32)effect->fadeOutTime / 16.0f;
        effect->vm.color.color =
            (effect->vm.color.color & 0xffffff) | (u32)(fadeOutRatio * 255.0f) << 24;
        effect->vm.scale.y = 2.0f - fadeOutRatio;
        effect->vm.scale.x = effect->vm.scale.y;
    }
    return 1;
}

// FUNCTION: TH07 0x0041aa60
i32 EffectManager::InitRandomDir(Effect *effect)
{
    f32 fVar1;

    effect->emitterPos = effect->pos;
    effect->emitterPos.z = 0.0f;
    fVar1 = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->direction.x = cosf(fVar1);
    effect->direction.y = sinf(fVar1);
    effect->direction.z = 0.0f;
    return 0;
}

// FUNCTION: TH07 0x0041aaf0
i32 EffectManager::UpdateGather60Frames(Effect *effect)
{
    f32 distance = 256.0f - effect->timer.AsFloat() * 256.0f / 60.0f;
    effect->pos = effect->direction * distance + effect->emitterPos;
    effect->pos.z = 0.0f;
    return 1;
}

// FUNCTION: TH07 0x0041abe0
i32 EffectManager::UpdateAttachToPlayer(Effect *effect)
{
    if ((i32)!effect->vm.currentInstruction)
    {
        return false;
    }

    effect->pos = g_Player.pos;
    return true;
}

// FUNCTION: TH07 0x0041ac30
i32 EffectManager::UpdateGather240Frames(Effect *effect)
{
    f32 distance = 256.0f - effect->timer.AsFloat() * 256.0f / 240.0f;
    effect->pos = effect->direction * distance + effect->emitterPos;
    return 1;
}

// FUNCTION: TH07 0x0041ad10
i32 EffectManager::UpdateBurst30Frames(Effect *effect)
{
    f32 distance = effect->timer.AsFloat() * 256.0f / 30.0f;
    effect->pos = effect->direction * distance + effect->emitterPos;
    return 1;
}

#pragma var_order(effect, i)
// FUNCTION: TH07 0x0041adf0
void EffectManager::ShiftEffectsAfterCameraTeleport(Float3 *shift)
{
    i32 i;
    Effect *effect;

    effect = g_EffectManager.effects;
    for (i = 0; i < MAX_NORMAL_EFFECTS; i++, effect++)
    {
        if (effect->effectId == 20 || effect->effectId == 31)
        {
            effect->basePos += *shift;
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
    for (i = 0; i < MAX_NORMAL_EFFECTS; i++, effect++)
    {
        if (effect->effectId == 30)
        {
            effect->accel.z = -0.01f;
        }
    }
}

#pragma var_order(local_10, dot)
// FUNCTION: TH07 0x0041aef0
i32 EffectManager::UpdateWeatherPhysics(Effect *effect)
{
    Float3 local_10;

    effect->velocity += effect->accel;
    effect->basePos += effect->velocity;
    effect->pos = effect->basePos;

    local_10 = effect->pos - g_Stage.cam.pos;
    D3DXVec3Normalize(local_10.asD3DX(), local_10.asD3DX());
    f32 dot = D3DXVec3Dot(g_Stage.cam.lookAtDir.asD3DX(), local_10.asD3DX());
    if (dot < 0.94f)
    {
        return 0;
    }

    effect->vm.SetRotationZ(utils::AddNormalizeAngle(effect->vm.rotation.z, effect->vm.rotation.x));
    effect->vm.updateRotation = 1;
    if (effect->pos.z >= 0.0f)
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
    Float3 camLookAtInv;

    camLookAtInv = -g_Stage.cam.lookAt;

    effect->basePos = g_Stage.cam.lookAt + g_Stage.cam.pos;
    effect->basePos.x += g_Rng.GetRandomFloatInRange(120.0f) - 60.0f + camLookAtInv.x / 2.0f;
    effect->basePos.y += g_Rng.GetRandomFloatInRange(200.0f) - 100.0f + camLookAtInv.y / 2.0f;
    effect->basePos.z += g_Rng.GetRandomFloatInRange(100.0f) - 100.0f + camLookAtInv.z / 2.0f;
    effect->velocity.x =
        g_Rng.GetRandomFloatInRange(0.06f) - 0.03f + effect->custom.x;
    effect->velocity.y =
        g_Rng.GetRandomFloatInRange(0.06f) - 0.03f + effect->custom.y;
    effect->velocity.z = g_Rng.GetRandomFloatInRange(0.1f) + 0.03f + effect->custom.z;
    effect->accel.x = g_Rng.GetRandomFloatInRange(0.0002f) - 0.0001f;
    effect->accel.y = g_Rng.GetRandomFloatInRange(0.0002f) - 0.0001f;
    effect->velocity = effect->velocity * g_Supervisor.effectiveFramerateMultiplier;
    effect->accel = effect->accel * g_Supervisor.effectiveFramerateMultiplier;
    effect->drawType = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(ZUN_PI / 100.0f) - ZUN_PI / 200.0f;

    chance = g_GameManager.cherry - g_GameManager.globals->cherryStart;
    chance = chance * 100 / g_GameManager.cherryMax;

    if ((u32)chance >= g_Rng.GetRandomU32InRange(100))
    {
        g_AnmManager->SetActiveSprite(&effect->vm, ANM_SPRITE_BULLETS_CHERRY_PETAL);
        effect->vm.color.bytes.r = 255;
        effect->vm.color.bytes.g = 255;
        effect->vm.color.bytes.b = 255;
    }
    return 0;
}

// FUNCTION: TH07 0x0041b4a0
i32 EffectManager::InitWeatherVortex(Effect *effect)
{
    i32 chance;

    effect->basePos.x = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePos.y = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePos.z = g_Rng.GetRandomFloatInRange(100.0f) - 50.0f;
    effect->velocity.x = -effect->basePos.y / effect->custom.x;
    effect->velocity.y = effect->basePos.x / effect->custom.x;
    effect->velocity.z = g_Rng.GetRandomFloatInRange(0.1f) + 0.09f;
    effect->basePos += g_Stage.cam.lookAt / 2.0f + g_Stage.cam.pos;
    effect->velocity = effect->velocity * g_Supervisor.effectiveFramerateMultiplier;
    effect->drawType = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(ZUN_PI / 50.0f) - ZUN_PI / 100.0f;

    chance = g_GameManager.cherry - g_GameManager.globals->cherryStart;
    chance = chance * 100 / g_GameManager.cherryMax;

    if ((u32)chance >= g_Rng.GetRandomU32InRange(100))
    {
        g_AnmManager->SetActiveSprite(&effect->vm, ANM_SPRITE_BULLETS_CHERRY_PETAL);
        effect->vm.color.bytes.r = 255;
        effect->vm.color.bytes.g = 255;
        effect->vm.color.bytes.b = 255;
    }
    effect->accel.x = 0.0f;
    effect->accel.y = 0.0f;
    effect->accel.z = 0.0f;
    return 0;
}

// FUNCTION: TH07 0x0041b770
i32 EffectManager::InitWeatherBackward(Effect *effect)
{
    effect->basePos.x = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePos.y = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePos.z = g_Rng.GetRandomFloatInRange(100.0f) - 50.0f;
    effect->velocity.x = -effect->basePos.y / effect->custom.x;
    effect->velocity.y = effect->basePos.x / effect->custom.x;
    effect->velocity.z = -g_Rng.GetRandomFloatInRange(0.2f) - 0.06f;
    effect->basePos += g_Stage.cam.lookAt / 2.0f + g_Stage.cam.pos;
    effect->velocity = effect->velocity * g_Supervisor.effectiveFramerateMultiplier;
    effect->drawType = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(ZUN_PI / 50.0f) - ZUN_PI / 100.0f;
    g_AnmManager->SetActiveSprite(&effect->vm, ANM_SPRITE_BULLETS_CHERRY_PETAL);
    effect->vm.color.bytes.r = 255;
    effect->vm.color.bytes.g = 255;
    effect->vm.color.bytes.b = 255;
    effect->accel.x = 0.0f;
    effect->accel.y = 0.0f;
    effect->accel.z = 0.0f;
    return 0;
}

// FUNCTION: TH07 0x0041b9f0
i32 EffectManager::InitWeatherSlow(Effect *effect)
{
    effect->basePos.x = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePos.y = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePos.z = g_Rng.GetRandomFloatInRange(100.0f) - 100.0f;
    effect->velocity.x =
        g_Rng.GetRandomFloatInRange(0.06f) - 0.03f + effect->custom.x;
    effect->velocity.y =
        g_Rng.GetRandomFloatInRange(0.06f) - 0.03f + effect->custom.y;
    effect->velocity.z =
        g_Rng.GetRandomFloatInRange(0.02f) + 0.01f + effect->custom.z;
    effect->basePos += g_Stage.cam.lookAt / 2.0f + g_Stage.cam.pos;
    effect->drawType = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(ZUN_PI / 50.0f) - ZUN_PI / 100.0f;
    g_AnmManager->SetActiveSprite(&effect->vm, ANM_SPRITE_BULLETS_CHERRY_PETAL);
    effect->vm.color.bytes.r = 255;
    effect->vm.color.bytes.g = 255;
    effect->vm.color.bytes.b = 255;
    effect->accel.x = 0.0f;
    effect->accel.y = 0.0f;
    effect->accel.z = 0.0f;
    return 0;
}

// FUNCTION: TH07 0x0041bc20
i32 EffectManager::InitWeatherFalling(Effect *effect)
{
    effect->basePos.x = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePos.y = g_Rng.GetRandomFloatInRange(160.0f) - 80.0f;
    effect->basePos.z = g_Rng.GetRandomFloatInRange(200.0f) - 0.0f;
    effect->velocity.x =
        g_Rng.GetRandomFloatInRange(0.06f) - 0.03f + effect->custom.x;
    effect->velocity.y =
        g_Rng.GetRandomFloatInRange(0.06f) - 0.03f + effect->custom.y;
    effect->velocity.z = -g_Rng.GetRandomFloatInRange(0.1f) + effect->custom.z;
    effect->basePos += g_Stage.cam.lookAt / 2.0f + g_Stage.cam.pos;
    effect->velocity = effect->velocity * g_Supervisor.effectiveFramerateMultiplier;
    effect->drawType = 1;
    effect->vm.rotation.z = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    effect->vm.rotation.x = g_Rng.GetRandomFloatInRange(ZUN_PI / 50.0f) - ZUN_PI / 100.0f;
    g_AnmManager->SetActiveSprite(&effect->vm, ANM_SPRITE_BULLETS_CHERRY_PETAL);
    effect->vm.angleVel.z *= 2;
    effect->vm.color.bytes.r = 255;
    effect->vm.color.bytes.g = 255;
    effect->vm.color.bytes.b = 255;
    effect->accel.x = 0.0f;
    effect->accel.y = 0.0f;
    effect->accel.z = -0.015f;
    return 0;
}

// FUNCTION: TH07 0x0041bec0
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
    effect->emitterPos = effect->pos;
    effect->emitterPos.z = 0.0f;
    effect->direction.x = cosf(angle);
    effect->direction.y = sinf(angle);
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
    effect->pos = fVar1 * effect->direction * 128.0f + effect->emitterPos;
    effect->pos.z = 0.0f;
    return 1;
}

// FUNCTION: TH07 0x0041c100
i32 EffectManager::UpdateAttachToCamera(Effect *effect)
{
    effect->drawType = 1;
    effect->basePos = g_Stage.cam.lookAt + g_Stage.cam.pos;
    effect->pos = effect->basePos;
    effect->pos.z = 0.0f;
    effect->drawType = 3;
    return 1;
}

// FUNCTION: TH07 0x0041c1b0
i32 EffectManager::UpdateNoOp(Effect *effect)
{
    return 1;
}

#pragma var_order(effect, i)
// FUNCTION: TH07 0x0041c1c0
Effect *EffectManager::SpawnEffect(i32 effectId, Float3 *pos,
                                   i32 numParticles, D3DCOLOR color)
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

        effect->drawType = 0;
        effect->inUseFlag = 1;
        effect->effectId = (u8)effectId;
        effect->pos = *pos;
        g_AnmManager->SetAnmIdxAndExecuteScript(&effect->vm, g_EffectMapping[effectId].anmId);
        effect->vm.zWriteDisable = 1;
        effect->vm.color.color = color;
        effect->callback = g_EffectMapping[effectId].updateCallback;
        effect->timer = 0;
        effect->isFadingOut = 0;
        effect->fadeOutTime = 0;
        effect->custom = Float3(0.0f, 0.0f, 0.0f);
        if (g_EffectMapping[effectId].initCallback &&
            g_EffectMapping[effectId].initCallback(effect))
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

    return i >= MAX_NORMAL_EFFECTS ? &this->effects[MAX_EFFECTS] : effect;
}

#pragma var_order(effect, i)
// FUNCTION: TH07 0x0041c400
Effect *EffectManager::SpawnMovingParticles(i32 effectId, Float3 *pos,
                                            Float3 *velocity,
                                            i32 numParticles, D3DCOLOR color)
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

        effect->drawType = 0;
        effect->inUseFlag = 1;
        effect->effectId = effectId;
        effect->pos = *pos;
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

// FUNCTION: TH07 0x0041c610
Effect *EffectManager::SpawnSpecialEffect(i32 effectId, Float3 *pos, i32 effectIdx,
                                          i32 param_4, D3DCOLOR color)
{
    Effect *effect;

    effect = &this->effects[effectIdx + MAX_NORMAL_EFFECTS];
    effect->drawType = 0;
    effect->inUseFlag = 1;
    effect->effectId = effectId;
    effect->pos = *pos;
    g_AnmManager->SetAnmIdxAndExecuteScript(&effect->vm, g_EffectMapping[effectId].anmId);
    effect->vm.zWriteDisable = 1;
    effect->vm.color.color = color;
    effect->callback = g_EffectMapping[effectId].updateCallback;
    effect->timer = 0;
    effect->isFadingOut = 0;
    effect->fadeOutTime = 0;
    effect->custom = Float3(0.0f, 0.0f, 0.0f);
    if (g_EffectMapping[effectId].initCallback)
    {
        if (g_EffectMapping[effectId].initCallback(effect))
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

        effect->timer++;
        effect->next = NULL;
        if (effect->drawType == 1 || effect->drawType == 3)
        {
            arg->layerPtrs[1]->next = effect;
            arg->layerPtrs[1] = effect;
        }
        else if (!effect->drawType)
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
    if (arg->frameCounter % 300 == 100 &&
        g_GameManager.CheckGameIntegrity())
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

    effect = arg->layer0.next;
    while (effect)
    {
        effect->vm.pos = effect->pos;
        effect->vm.pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        effect->vm.pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        g_AnmManager->Draw(&effect->vm);
        effect = effect->next;
    }
    effect = arg->layer2.next;
    while (effect)
    {
        effect->vm.pos = effect->pos;
        g_AnmManager->DrawBillboard(&effect->vm);
        effect = effect->next;
    }
    effect = arg->layer3.next;
    while (effect)
    {
        effect->vm.pos = effect->pos;
        effect->vm.pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        effect->vm.pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        g_AnmManager->Draw(&effect->vm);
        effect = effect->next;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(effect, a, counter, b, g, r, temp)
// FUNCTION: TH07 0x0041cb80
i32 EffectManager::DrawLayer1Effects()
{
    int temp;
    f32 r;
    f32 g;
    f32 b;
    int counter;
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
        }

        effect->vm.pos = effect->pos;
        if (effect->drawType == 1)
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

// FUNCTION: TH07 0x0041cde0
ZunResult EffectManager::AddedCallback(EffectManager *arg)
{
    arg->Reset();
    g_Stage.spellcardVmsIdx = 0;
    switch (g_GameManager.currentStage)
    {
    case DUMMYSTAGE:
    case STAGE1:
        g_Stage.numSpellcardVms = 1;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff01.anm", ANM_OFFSET_EFFECTS) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case STAGE2:
        g_Stage.numSpellcardVms = 1;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff02.anm", ANM_OFFSET_EFFECTS) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case STAGE3:
        g_Stage.numSpellcardVms = 1;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff03.anm", ANM_OFFSET_EFFECTS) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case STAGE4:
        g_Stage.numSpellcardVms = 2;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff04.anm", ANM_OFFSET_EFFECTS) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS2, "data/eff04b.anm", ANM_OFFSET_EFFECTS2) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case STAGE5:
        g_Stage.numSpellcardVms = 2;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff05.anm", ANM_OFFSET_EFFECTS) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case STAGE6:
        g_Stage.numSpellcardVms = 2;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff05.anm", ANM_OFFSET_EFFECTS) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS3, "data/eff06.anm", ANM_OFFSET_EFFECTS3) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case EXTRASTAGE:
        g_Stage.numSpellcardVms = 1;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff02.anm", ANM_OFFSET_EFFECTS) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS2, "data/eff07.anm", ANM_OFFSET_EFFECTS2) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case PHANTASMSTAGE:
        g_Stage.numSpellcardVms = 2;
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS, "data/eff07.anm", ANM_OFFSET_EFFECTS) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_EFFECTS3, "data/eff08.anm", ANM_OFFSET_EFFECTS3) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0041d050
ZunResult EffectManager::DeletedCallback(EffectManager *arg)
{
    g_AnmManager->ReleaseAnm(ANM_FILE_EFFECTS);
    g_AnmManager->ReleaseAnm(ANM_FILE_EFFECTS2);
    g_AnmManager->ReleaseAnm(ANM_FILE_EFFECTS3_0);
    g_AnmManager->ReleaseAnm(ANM_FILE_EFFECTS3_1);
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

// FUNCTION: TH07 0x0041d150
void EffectManager::CutChain()
{
    g_Chain.Cut(&g_EffectManagerCalcChain);
    g_Chain.Cut(&g_EffectManagerDrawChain);
}
