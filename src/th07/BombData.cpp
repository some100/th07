#include "BombData.hpp"

#include "AnmManager.hpp"
#include "BulletManager.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "Player.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "ZunMath.hpp"
#include "i18n.hpp"
#include "utils.hpp"

// GLOBAL: TH07 0x0049ec50
BombData g_BombData[6] = {
    {BombData::BombReimuACalc, BombData::BombReimuADraw,
     BombData::BombReimuACalcFocus, BombData::BombReimuADrawFocus},
    {BombData::BombReimuBCalc, BombData::BombReimuBDraw,
     BombData::BombReimuBCalcFocus, BombData::BombReimuBDrawFocus},
    {BombData::BombMarisaACalc, BombData::BombMarisaADraw,
     BombData::BombMarisaACalcFocus, BombData::BombMarisaADrawFocus},
    {BombData::BombMarisaBCalc, BombData::BombMarisaBDraw,
     BombData::BombMarisaBCalcFocus, BombData::BombMarisaBDrawFocus},
    {BombData::BombSakuyaACalc, BombData::BombSakuyaADraw,
     BombData::BombSakuyaACalcFocus, BombData::BombSakuyaADrawFocus},
    {BombData::BombSakuyaBCalc, BombData::BombSakuyaBDraw,
     BombData::BombSakuyaBCalcFocus, BombData::BombSakuyaBDrawFocus}};

// FUNCTION: TH07 0x004083f0
void BombData::DarkenViewport(Player *player)
{
    ZunColor color;

    color.bytes.a = 128;
    if (player->bombInfo.bombTimer < 60)
    {
        color.bytes.b =
            128 - player->bombInfo.bombTimer.GetCurrent() * 80 / 60;
        color.bytes.g = color.bytes.b;
        color.bytes.r = color.bytes.g;
    }
    else if (player->bombInfo.bombTimer >= player->bombInfo.bombDuration - 60)
    {
        color.bytes.b = 128 - (player->bombInfo.bombDuration -
                               player->bombInfo.bombTimer.GetCurrent()) *
                                  80 / 60;
        color.bytes.g = color.bytes.b;
        color.bytes.r = color.bytes.g;
    }
    else
    {
        color.bytes.b = 48;
        color.bytes.g = color.bytes.b;
        color.bytes.r = color.bytes.g;
    }
    g_AnmManager->SetColor(0x80808080);
    g_Stage.SmoothBlendColor(color);
    g_Stage.isDarkening = 1;
}

// FUNCTION: TH07 0x004084f0
void BombData::SpawnBombInvulnEffect(Player *player)
{
    if (player->effect)
    {
        player->effect->inUseFlag = 0;
    }

    Effect *effect = g_EffectManager.SpawnSpecialEffect(25, &player->pos, 0,
                                                        1, 0xffffffff);
    effect->vm.interpStartTimes[4] = 0;
    effect->vm.interpEndTimes[4] = player->invulnerabilityTimer;
    effect->vm.easeModes[4] = 0;
    effect->vm.scaleInterpInitial = effect->vm.scale;
    effect->vm.scaleInterpFinal.x = 1.0f / 16.0f;
    effect->vm.scaleInterpFinal.y = 1.0f / 16.0f;
    effect->vm.intVars1[0] = player->invulnerabilityTimer.GetCurrent();
    effect->vm.angleVel.z *= -1.0f;
    effect->vm.color.bytes.r = 255;
    effect->vm.color.bytes.g = 64;
    effect->vm.color.bytes.b = 64;
    player->effect = effect;
}

// FUNCTION: TH07 0x00408610
void BombData::ComputeBombCherryDrain(Player *player, i32 minCost, f32 scale)
{
    i32 drain = (i32)((g_GameManager.cherry - g_GameManager.globals->cherryStart) * scale);

    switch (g_GameManager.difficulty)
    {
    case DIFF_HARD:
        drain /= 2;
        break;
    case DIFF_LUNATIC:
        drain /= 4;
        break;
    case DIFF_EXTRA:
    case DIFF_PHANTASM:
        drain /= 3;
        break;
    }

    drain /= player->bombInfo.bombDuration;
    drain -= drain % 10;

    minCost /= player->bombInfo.bombDuration;
    minCost -= minCost % 10;

    player->bombInfo.cherryDrain = drain < minCost ? minCost : drain;
}

#pragma var_order(angle, bombInfo, i, subInfo, vm, unused, j)
// FUNCTION: TH07 0x00408710
void BombData::BombReimuACalc(Player *player)
{
    i32 j;
    i32 unused[3];
    AnmVm *vm;
    PlayerBombSubInfo *subInfo;
    i32 i;
    PlayerBombInfo *bombInfo = &player->bombInfo;
    f32 angle;

    if (player->bombInfo.bombTimer >= player->bombInfo.bombDuration)
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        return;
    }

    if (bombInfo->bombTimer.HasTicked() &&
        bombInfo->bombTimer == 0)
    {
        g_Gui.ShowBombNamePortrait(ANM_SPRITE_FACE_PORTRAIT_ARRAY + 1,
                                   TH_BOMB_REIMU_A);
        bombInfo->bombDuration = 140;
        player->invulnerabilityTimer = 200;
        SpawnBombInvulnEffect(player);
        for (i = 0; i < 32; i++)
        {
            bombInfo->subInfo[i].state = 0;
        }
        g_ItemManager.RemoveAllItems();
        g_EffectManager.SpawnEffect(12, &player->pos, 1,
                                    0xff4040ff);
        player->SpawnBombEffect(&player->pos, 32.0f, 8.0f, 16,
                                ITEM_POINT_BULLET);

        bombInfo->startPos = player->pos;
        ComputeBombCherryDrain(player, 4000, 0.2f);
    }
    if (bombInfo->bombTimer.HasTicked() &&
        bombInfo->bombTimer >= 8 &&
        bombInfo->bombTimer < 80 &&
        bombInfo->bombTimer.GetCurrent() % 6 == 0)
    {
        i = (bombInfo->bombTimer.GetCurrent() - 8) / 6;
        subInfo = &bombInfo->subInfo[i];
        subInfo->state = 1;
        subInfo->speed = 15.0f;
        subInfo->pos = player->pos;

        if (bombInfo->startPos.x < 192.0f)
        {
            angle = (f32)i * ZUN_2PI / 8.0f - ZUN_PI / 2.0f;
        }
        else
        {
            angle = (f32)-i * ZUN_2PI / 8.0f - ZUN_PI / 2.0f;
        }
        subInfo->angle = utils::AddNormalizeAngle(angle, 0.0f);
        subInfo->counter = 0;
        player->bombDamageBoxes[i].damage = 0;
        vm = subInfo->vms;
        for (j = 0; j < 4; j++, vm++)
        {
            g_AnmManager->ExecuteAnmIdx(
                vm, j + ANM_SCRIPT_PLAYER_REIMU_A_BOMB_ARRAY);
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_REIMU_A, 0);
    }
    player->playerState = PLAYER_STATE_INVULNERABLE;
    for (i = 0, subInfo = bombInfo->subInfo; i < 8; i++, subInfo++)
    {
        if (subInfo->state == 0)
        {
            continue;
        }

        if (subInfo->state == 1)
        {
            subInfo->speed -=
                0.4f * g_Supervisor.effectiveFramerateMultiplier;
            subInfo->velocity.FromAngleMagnitude(subInfo->angle,
                                                 subInfo->speed);
            if (subInfo->speed < -10.0f)
            {
                g_EffectManager.SpawnEffect(6, &subInfo->pos, 8,
                                            0xffffffff);
                g_EffectManager.SpawnEffect(12, &subInfo->pos,
                                            1, 0xff4040ff);
                subInfo->state = 2;
                subInfo->vms[0].pendingInterrupt = 1;
                subInfo->vms[1].pendingInterrupt = 1;
                subInfo->vms[2].pendingInterrupt = 1;
                subInfo->vms[3].pendingInterrupt = 1;
                player->bombDamageBoxes[i].pos =
                    subInfo->pos;
                player->bombDamageBoxes[i].size.x = 256.0f;
                player->bombDamageBoxes[i].size.y = 256.0f;
                player->bombDamageBoxes[i].lifetime = 400;
                player->SpawnBombEffect(&subInfo->pos, 64.0f,
                                        4.266667f, 30, ITEM_POINT_BULLET);
                subInfo->velocity.x = 0.0f;
                subInfo->velocity.y = 0.0f;
                subInfo->velocity.z = 0.0f;
                g_SoundPlayer.PlaySoundByIdx(SOUND_ENEMY_SPELLCARD_END, 0);
                ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 16, 8, 0, 0);
            }
            if (bombInfo->bombTimer.HasTicked())
            {
                player->bombDamageBoxes[i].size.x = 48.0f;
                player->bombDamageBoxes[i].size.y = 48.0f;
                player->bombDamageBoxes[i].pos =
                    subInfo->pos;
                player->bombDamageBoxes[i].lifetime = 8;
                player->SpawnBombEffect(&subInfo->pos, 128.0f,
                                        0.0f, 0, ITEM_POINT_BULLET);
            }
        }
        else if (subInfo->state != 0 && bombInfo->bombTimer.HasTicked())
        {
            player->bombDamageBoxes[i].pos =
                bombInfo->subInfo[i].pos;
            player->bombDamageBoxes[i].size.x = 256.0f;
            player->bombDamageBoxes[i].size.y = 256.0f;
            player->bombDamageBoxes[i].lifetime = 2;
            subInfo->counter++;
            if (subInfo->counter >= 30)
            {
                subInfo->state = 0;
            }
        }
        subInfo->pos.x +=
            g_Supervisor.effectiveFramerateMultiplier *
            subInfo->velocity.x;
        subInfo->pos.y +=
            g_Supervisor.effectiveFramerateMultiplier *
            subInfo->velocity.y;
        g_AnmManager->ExecuteScript(&subInfo->vms[0]);
        g_AnmManager->ExecuteScript(&subInfo->vms[1]);
        g_AnmManager->ExecuteScript(&subInfo->vms[2]);
        g_AnmManager->ExecuteScript(&subInfo->vms[3]);
    }
    bombInfo->bombTimer++;
}

#pragma var_order(vm, i, subInfo)
// FUNCTION: TH07 0x00408e10
void BombData::BombReimuADraw(Player *player)
{
    PlayerBombSubInfo *subInfo;
    i32 i;
    AnmVm *vm;

    DarkenViewport(player);
    for (i = 0, subInfo = player->bombInfo.subInfo; i < 8; i++, subInfo++)
    {
        if (subInfo->state == 0)
        {
            continue;
        }

        vm = subInfo->vms;

        vm->pos =
            subInfo->pos + vm->offset;
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.0f;
        g_AnmManager->DrawNoRotation(vm);
        vm++;
        vm->pos =
            subInfo->pos + vm->offset;
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.0f;
        g_AnmManager->DrawNoRotation(vm);
        vm++;
        vm->pos =
            subInfo->pos + vm->offset;
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.0f;
        g_AnmManager->DrawNoRotation(vm);
        vm++;
        vm->pos =
            subInfo->pos + vm->offset;
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.0f;
        g_AnmManager->DrawNoRotation(vm);
        vm++;
    }
}

#pragma var_order(tmpFloat3, tmpFloat2, bombInfo, i, subInfo, vm, tmpFloat1, \
                  targetPos, j)
// FUNCTION: TH07 0x004091b0
void BombData::BombReimuACalcFocus(Player *player)
{
    i32 j;
    Float3 targetPos;
    f32 tmpFloat1;
    AnmVm *vm;
    PlayerBombSubInfo *subInfo;
    i32 i;
    PlayerBombInfo *bombInfo = &player->bombInfo;
    f32 tmpFloat2;
    f32 tmpFloat3;

    if (bombInfo->bombTimer >= bombInfo->bombDuration)
    {
        g_Gui.EndPlayerSpellcard();
        bombInfo->isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
        return;
    }

    if (bombInfo->bombTimer.HasTicked() &&
        bombInfo->bombTimer == 0)
    {
        g_Gui.ShowBombNamePortrait(ANM_SPRITE_FACE_PORTRAIT_ARRAY + 1,
                                   TH_BOMB_REIMU_A_FOCUS);
        bombInfo->bombDuration = 300;
        player->invulnerabilityTimer = 360;
        SpawnBombInvulnEffect(player);
        for (i = 0; i < 8; i++)
        {
            bombInfo->subInfo[i].state = 0;
        }
        g_ItemManager.RemoveAllItems();
        g_EffectManager.SpawnEffect(12, &player->pos, 1,
                                    0xff4040ff);
        player->SpawnBombEffect(&player->pos, 32.0f, 8.0f, 16,
                                ITEM_POINT_BULLET);
        ComputeBombCherryDrain(player, 5000, 0.22f);
        player->verticalMovementSpeedMultiplierDuringBomb = 0.6f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 0.6f;
    }
    if (bombInfo->bombTimer >= 60 && bombInfo->bombTimer < 180)
    {
        if (bombInfo->bombTimer.GetCurrent() % 16 == 0)
        {
            i = (bombInfo->bombTimer.GetCurrent() - 60) / 16;
            subInfo = bombInfo->subInfo + i;
            if (i != 0)
            {
                subInfo->state = 1;
                subInfo->counter = 0;
                subInfo->custom = 8.0f;
                subInfo->pos = player->pos;

                tmpFloat2 = g_Rng.GetRandomFloat() * ZUN_2PI - ZUN_PI;
                subInfo->velocity.FromAngleMagnitude(tmpFloat2, subInfo->custom);

                player->bombDamageBoxes[i].damage = 0;
                vm = subInfo->vms;
                for (j = 0; j < 4; j++, vm++)
                {
                    g_AnmManager->ExecuteAnmIdx(
                        vm, j + ANM_SCRIPT_PLAYER_REIMU_A_BOMB_ARRAY);
                }
                g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_REIMU_A, 0);
            }
        }
    }
    player->playerState = PLAYER_STATE_INVULNERABLE;
    for (i = 0, subInfo = bombInfo->subInfo; i < 8; i++, subInfo++)
    {
        if (subInfo->state == 0)
        {
            continue;
        }

        if (subInfo->state == 1)
        {
            if (bombInfo->bombTimer.HasTicked())
            {
                if (player->positionOfLastEnemyHit.x > -100.0f)
                {
                    targetPos = player->positionOfLastEnemyHit;
                }
                else
                {
                    targetPos = player->pos;
                }

                tmpFloat2 = targetPos.x - subInfo->pos.x;
                tmpFloat3 = targetPos.y - subInfo->pos.y;
                tmpFloat1 = sqrtf(tmpFloat2 * tmpFloat2 + tmpFloat3 * tmpFloat3) /
                            (subInfo->custom / 8.0f);
                if (tmpFloat1 < 1.0f)
                {
                    tmpFloat1 = 1.0f;
                }

                tmpFloat2 = tmpFloat2 / tmpFloat1 + subInfo->velocity.x;
                tmpFloat3 = tmpFloat3 / tmpFloat1 + subInfo->velocity.y;
                tmpFloat1 = sqrtf(tmpFloat2 * tmpFloat2 + tmpFloat3 * tmpFloat3);

                subInfo->custom = tmpFloat1 > 10.0f ? 10.0f : tmpFloat1;
                if (subInfo->custom < 1.0f)
                {
                    subInfo->custom = 1.0f;
                }
                subInfo->velocity.x = tmpFloat2 * subInfo->custom / tmpFloat1;
                subInfo->velocity.y = tmpFloat3 * subInfo->custom / tmpFloat1;

                player->bombDamageBoxes[i].size.x = 48.0f;
                player->bombDamageBoxes[i].size.y = 48.0f;
                player->bombDamageBoxes[i].pos = subInfo->pos;
                player->bombDamageBoxes[i].lifetime = 8;
                player->SpawnBombEffect(&subInfo->pos, 128.0f,
                                        0.0f, 0, ITEM_POINT_BULLET);
                if (player->bombDamageBoxes[i].damage >= 100 ||
                    bombInfo->bombTimer >= bombInfo->bombDuration - 30)
                {
                    g_EffectManager.SpawnEffect(6, &subInfo->pos,
                                                8, 0xffffffff);
                    g_EffectManager.SpawnEffect(
                        12, &subInfo->pos, 1, 0xff4040ff);
                    subInfo->state = 2;
                    subInfo->vms[0].pendingInterrupt = 1;
                    subInfo->vms[1].pendingInterrupt = 1;
                    subInfo->vms[2].pendingInterrupt = 1;
                    subInfo->vms[3].pendingInterrupt = 1;
                    player->bombDamageBoxes[i].size.x = 256.0f;
                    player->bombDamageBoxes[i].size.y = 256.0f;
                    player->bombDamageBoxes[i].lifetime = 400;
                    player->SpawnBombEffect(&subInfo->pos, 32.0f,
                                            6.6666665f, 15, ITEM_POINT_BULLET);

                    // ZUN bloat: This does absolutely nothing
                    subInfo->velocity / 8.0f;

                    g_SoundPlayer.PlaySoundByIdx(SOUND_ENEMY_SPELLCARD_END, 0);
                    ScreenEffect::RegisterChain(
                        SCREEN_EFFECT_SHAKE, 16, 8, 0, 0);
                }
            }
        }
        else if (subInfo->state != 0 && bombInfo->bombTimer.HasTicked())
        {
            subInfo->counter++;
            if (subInfo->counter >= 30)
            {
                subInfo->state = 0;
            }
        }
        subInfo->pos.x +=
            g_Supervisor.effectiveFramerateMultiplier *
            subInfo->velocity.x;
        subInfo->pos.y +=
            g_Supervisor.effectiveFramerateMultiplier *
            subInfo->velocity.y;
        g_AnmManager->ExecuteScript(&subInfo->vms[0]);
        g_AnmManager->ExecuteScript(&subInfo->vms[1]);
        g_AnmManager->ExecuteScript(&subInfo->vms[2]);
        g_AnmManager->ExecuteScript(&subInfo->vms[3]);
    }
    bombInfo->bombTimer++;
}

#pragma var_order(vm, i)
// FUNCTION: TH07 0x00409990
void BombData::BombReimuADrawFocus(Player *player)
{
    i32 i;
    AnmVm *vm;

    DarkenViewport(player);
    for (i = 0; i < 8; i++)
    {
        if (player->bombInfo.subInfo[i].state == 0)
        {
            continue;
        }

        vm = player->bombInfo.subInfo[i].vms;
        vm->pos = player->bombInfo.subInfo[i].pos + vm->offset;
        player->SetToTopLeftPos(vm);
        g_AnmManager->DrawNoRotation(vm);
        vm++;
        vm->pos =
            player->bombInfo.subInfo[i].pos + vm->offset;
        player->SetToTopLeftPos(vm);
        g_AnmManager->DrawNoRotation(vm);
        vm++;
        vm->pos =
            player->bombInfo.subInfo[i].pos + vm->offset;
        player->SetToTopLeftPos(vm);
        g_AnmManager->DrawNoRotation(vm);
        vm++;
        vm->pos =
            player->bombInfo.subInfo[i].pos + vm->offset;
        player->SetToTopLeftPos(vm);
        g_AnmManager->DrawNoRotation(vm);
        vm++;
    }
}

#pragma var_order(i, vm, projectiles)
// FUNCTION: TH07 0x00409dd0
void BombData::BombReimuBCalc(Player *player)
{
    BombClearBox *projectiles[7];
    AnmVm *vm;
    i32 i;

    if (player->bombInfo.bombTimer >= player->bombInfo.bombDuration)
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        return;
    }

    if (player->bombInfo.bombTimer.HasTicked() &&
        player->bombInfo.bombTimer == 0)
    {
        g_ItemManager.RemoveAllItems();
        g_Gui.ShowBombNamePortrait(ANM_SPRITE_FACE_PORTRAIT_ARRAY + 1,
                                   TH_BOMB_REIMU_B);
        player->bombInfo.bombDuration = 140;
        player->invulnerabilityTimer = 200;
        SpawnBombInvulnEffect(player);
        for (i = 0; i < 4; i++)
        {
            vm = player->bombInfo.subInfo[i].vms;
            g_AnmManager->ExecuteAnmIdx(
                vm, i + ANM_SCRIPT_PLAYER_REIMU_B_BOMB_ARRAY);
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_REIMARI, 0);
        player->bombInfo.subInfo[0].pos.x =
            player->pos.x;
        player->bombInfo.subInfo[0].pos.y = 224.0f;
        player->bombInfo.subInfo[0].pos.z = 0.42f;
        player->bombInfo.subInfo[1].pos.x = 192.0f;
        player->bombInfo.subInfo[1].pos.y =
            player->pos.y;
        player->bombInfo.subInfo[1].pos.z = 0.415f;
        player->bombInfo.subInfo[2].pos.x =
            player->pos.x;
        player->bombInfo.subInfo[2].pos.y = 224.0f;
        player->bombInfo.subInfo[2].pos.z = 0.41f;
        player->bombInfo.subInfo[3].pos.x = 192.0f;
        player->bombInfo.subInfo[3].pos.y =
            player->pos.y;
        player->bombInfo.subInfo[3].pos.z = 0.405f;
        ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 60, 2, 6, 0);
        ComputeBombCherryDrain(player, 3000, 0.17f);
    }
    else
    {
        if (player->bombInfo.bombTimer == 60)
        {
            ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 80, 20, 0, 0);
        }
        projectiles[0] = player->SpawnBombProjectile(&player->pos, 62.0f,
                                                     448.0f, 6);
        projectiles[1] = player->SpawnBombProjectile(&player->pos, 384.0f,
                                                     62.0f, 6);
        projectiles[2] = player->SpawnBombProjectile(&player->pos, 62.0f,
                                                     448.0f, 6);
        projectiles[3] = player->SpawnBombProjectile(&player->pos, 384.0f,
                                                     62.0f, 6);
        for (i = 0; i < 4; i++)
        {
            g_AnmManager->ExecuteScript(player->bombInfo.subInfo[i].vms);
            if (player->bombInfo.bombTimer.HasTicked())
            {
                if (player->bombInfo.bombTimer.GetCurrent() % 2 != 0)
                {
                    projectiles[i]->pos.x =
                        player->bombInfo.subInfo[i].pos.x +
                        player->bombInfo.subInfo[i].vms[0].offset.x;
                    projectiles[i]->pos.y =
                        player->bombInfo.subInfo[i].pos.y +
                        player->bombInfo.subInfo[i].vms[0].offset.y;
                    player->bombDamageBoxes[i].size.x =
                        projectiles[i]->pos.z;
                    player->bombDamageBoxes[i].size.y =
                        projectiles[i]->size.x;
                    player->bombDamageBoxes[i].pos =
                        player->bombInfo.subInfo[i].pos +
                        player->bombInfo.subInfo[i].vms->offset;
                    player->bombDamageBoxes[i].lifetime = 16;
                }
            }
        }
    }
    player->playerState = PLAYER_STATE_INVULNERABLE;
    player->bombInfo.bombTimer++;
}

#pragma var_order(vm, i)
// FUNCTION: TH07 0x0040a280
void BombData::BombReimuBDraw(Player *player)
{
    i32 i;
    AnmVm *vm;

    DarkenViewport(player);
    for (i = 0; i < 4; i++)
    {
        vm = player->bombInfo.subInfo[i].vms;
        vm->pos = player->bombInfo.subInfo[i].pos + vm->offset;
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.0f;
        g_AnmManager->Draw(vm);
    }
}

// FUNCTION: TH07 0x0040a3a0
void BombData::BombReimuBCalcFocus(Player *player)
{
    i32 unused[3];
    AnmVm *vm;
    i32 i;

    if (player->bombInfo.bombTimer >= player->bombInfo.bombDuration)
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
        return;
    }

    if (player->bombInfo.bombTimer.HasTicked() &&
        player->bombInfo.bombTimer == 0)
    {
        g_ItemManager.RemoveAllItems();
        g_Gui.ShowBombNamePortrait(ANM_SPRITE_FACE_PORTRAIT_ARRAY + 1,
                                   TH_BOMB_REIMU_B_FOCUS);
        player->bombInfo.bombDuration = 190;
        player->invulnerabilityTimer = 250;
        SpawnBombInvulnEffect(player);
        vm = player->bombInfo.subInfo[0].vms;
        for (i = 0; i < 3; i++, vm++)
        {
            g_AnmManager->ExecuteAnmIdx(
                vm, i + ANM_SCRIPT_PLAYER_REIMU_B_FOCUS_BOMB_ARRAY);
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_REIMARI, 0);
        ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 60, 2, 6, 0);
        player->bombInfo.startPos = player->pos;
        ComputeBombCherryDrain(player, 3000, 0.17f);
        player->verticalMovementSpeedMultiplierDuringBomb = 0.4f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 0.4f;
        player->SpawnBombEffect(&player->pos, 192.0f, 0.384f, 210,
                                ITEM_POINT_BULLET);
    }
    else
    {
        if (player->bombInfo.bombTimer == 60)
        {
            ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 80, 20, 0, 0);
        }
        g_AnmManager->ExecuteScript(&player->bombInfo.subInfo[0].vms[0]);
        g_AnmManager->ExecuteScript(&player->bombInfo.subInfo[0].vms[1]);
        g_AnmManager->ExecuteScript(&player->bombInfo.subInfo[0].vms[2]);

        player->bombDamageBoxes[0].size.x = 256.0f;
        player->bombDamageBoxes[0].size.y = 256.0f;
        player->bombDamageBoxes[0].pos =
            player->bombInfo.startPos +
            player->bombInfo.subInfo[0].vms[0].offset;
        player->bombDamageBoxes[0].lifetime = 18;
    }
    player->playerState = PLAYER_STATE_INVULNERABLE;
    player->bombInfo.bombTimer++;
}

#pragma var_order(vm, i)
// FUNCTION: TH07 0x0040a6b0
void BombData::BombReimuBDrawFocus(Player *player)
{
    i32 i;
    AnmVm *vm;

    DarkenViewport(player);
    for (i = 0; i < 3; i++)
    {
        vm = &player->bombInfo.subInfo[0].vms[i];
        vm->pos = player->bombInfo.startPos + vm->offset;
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.0f;
        g_AnmManager->Draw(vm);
    }
}

#pragma var_order(i, vm, unused, angle)
// FUNCTION: TH07 0x0040a7c0
void BombData::BombMarisaACalc(Player *player)
{
    f32 angle;
    i32 unused[3];
    AnmVm *vm;
    i32 i;

    if (player->bombInfo.bombTimer >= player->bombInfo.bombDuration)
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        return;
    }

    if (player->bombInfo.bombTimer.HasTicked() &&
        player->bombInfo.bombTimer == 0)
    {
        g_ItemManager.RemoveAllItems();
        g_Gui.ShowBombNamePortrait(ANM_SPRITE_FACE_PORTRAIT_ARRAY + 3,
                                   TH_BOMB_MARISA_A);
        player->bombInfo.bombDuration = 200;
        player->invulnerabilityTimer = 250;
        SpawnBombInvulnEffect(player);
        for (i = 0; i < 8; i++, vm++)
        {
            vm = player->bombInfo.subInfo[i].vms;
            g_AnmManager->ExecuteAnmIdx(
                vm, i % 3 + ANM_SCRIPT_PLAYER_MARISA_A_BOMB_ARRAY);
            player->bombInfo.subInfo[i].pos =
                player->pos;

            angle = (f32)i * ZUN_2PI / 8.0f;
            player->bombInfo.subInfo[i].velocity.FromAngleMagnitude(angle, 2.0f);
            player->bombInfo.subInfo[i].velocity.z = 0.0f;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_REIMARI, 0);
        ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 120, 4, 1, 0);
        ComputeBombCherryDrain(player, 8000, 0.3f);
    }
    else
    {
        for (i = 0; i < 8; i++)
        {
            player->bombInfo.subInfo[i].pos +=
                player->bombInfo.subInfo[i].velocity *
                g_Supervisor.effectiveFramerateMultiplier;
            if (player->bombInfo.bombTimer.HasTicked() &&
                player->bombInfo.bombTimer.GetCurrent() % 3 != 0)
            {
                player->SpawnBombEffect(
                    &player->bombInfo.subInfo[i].pos, 96.0f, 0.0f, 0,
                    ITEM_POINT_BULLET);
                player->bombDamageBoxes[i].size.x = 128.0f;
                player->bombDamageBoxes[i].size.y = 128.0f;
                player->bombDamageBoxes[i].pos =
                    player->bombInfo.subInfo[i].pos;
                player->bombDamageBoxes[i].lifetime = 8;
            }
            g_AnmManager->ExecuteScript(&player->bombInfo.subInfo[i].vms[0]);
        }
    }
    player->playerState = PLAYER_STATE_INVULNERABLE;
    player->bombInfo.bombTimer++;
}

#pragma var_order(vm, i)
// FUNCTION: TH07 0x0040aba0
void BombData::BombMarisaADraw(Player *player)
{
    i32 i;
    AnmVm *vm;

    DarkenViewport(player);
    for (i = 0; i < 8; i++)
    {
        vm = player->bombInfo.subInfo[i].vms;
        vm->pos = player->bombInfo.subInfo[i].pos;
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.0f;
        vm->scale.x = 3.2f;
        vm->scale.y = 3.2f;
        g_AnmManager->Draw(vm);
        vm->pos -= player->bombInfo.subInfo[i].velocity * 6;
        vm->pos.x += -32.0f;
        vm->pos.y += -32.0f;
        vm->pos.z = 0.0f;
        vm->scale.x = 2.2f;
        vm->scale.y = 2.2f;
        g_AnmManager->Draw(vm);
        vm->pos -= player->bombInfo.subInfo[i].velocity * 2;
        vm->pos.x += 64.0f;
        vm->pos.y += 64.0f;
        vm->pos.z = 0.0f;
        vm->pos -= player->bombInfo.subInfo[i].velocity * 2;
        vm->pos.x += -32.0f;
        vm->pos.y += -32.0f;
        vm->pos.z = 0.0f;
        vm->scale.x = 1.0f;
        vm->scale.y = 1.0f;
        g_AnmManager->Draw(vm);
        vm++;
    }
}

#pragma var_order(i, subInfo, vm, unused, j, angle)
// FUNCTION: TH07 0x0040af10
void BombData::BombMarisaACalcFocus(Player *player)
{
    f32 angle;
    i32 j;
    i32 unused[3];
    AnmVm *vm;
    PlayerBombSubInfo *subInfo;
    i32 i;

    if (player->bombInfo.bombTimer >= player->bombInfo.bombDuration)
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
        return;
    }

    if (player->bombInfo.bombTimer.HasTicked() &&
        player->bombInfo.bombTimer == 0)
    {
        g_ItemManager.RemoveAllItems();
        g_Gui.ShowBombNamePortrait(ANM_SPRITE_FACE_PORTRAIT_ARRAY + 2,
                                   TH_BOMB_MARISA_A_FOCUS);
        player->bombInfo.bombDuration = 260;
        player->invulnerabilityTimer = 310;
        SpawnBombInvulnEffect(player);
        for (i = 0; i < 24; i++)
        {
            player->bombInfo.subInfo[i].state = 0;
        }
        ComputeBombCherryDrain(player, 9000, 0.33f);
        player->verticalMovementSpeedMultiplierDuringBomb = 0.4f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 0.4f;
    }
    if (player->bombInfo.bombTimer.HasTicked() &&
        player->bombInfo.bombTimer.current % 6 == 0)
    {
        i = player->bombInfo.bombTimer.GetCurrent() / 6;
        if (i < 24)
        {
            vm = player->bombInfo.subInfo[i].vms;
            g_AnmManager->ExecuteAnmIdx(
                vm, i % 3 + ANM_SCRIPT_PLAYER_MARISA_A_BOMB_ARRAY);

            player->bombInfo.subInfo[i].pos =
                player->pos;
            for (j = 0; j < 8; j++)
            {
                player->bombInfo.subInfo[i].posHistory[j] =
                    player->pos;
            }
            player->bombInfo.subInfo[i].state = 1;
            angle = g_Rng.GetRandomFloatInRange(ZUN_PI / 8.0f) -
                    (ZUN_PI / 16.0f) - (ZUN_PI / 2.0f);
            player->bombInfo.subInfo[i].velocity.FromAngleMagnitude(angle, -5.0f);
            player->bombInfo.subInfo[i].velocity.z = 0.0f;
            angle = g_Rng.GetRandomFloatInRange(ZUN_PI / 8.0f) -
                    (ZUN_PI / 16.0f) - (ZUN_PI / 2.0f);
            player->bombInfo.subInfo[i].accel.FromAngleMagnitude(angle, 0.24f);
            player->bombInfo.subInfo[i].accel.z = 0.0f;
            g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_MARISA_A_FOCUS, 0);
            ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 120, 4, 1, 0);
            player->bombDamageBoxes[i].damage = 0;
        }
    }
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 24; i++, subInfo++)
    {
        if (subInfo->state == 0)
        {
            continue;
        }

        for (j = 7; j > 0; j--)
        {
            subInfo->posHistory[j] =
                subInfo->posHistory[j - 1];
        }
        subInfo->posHistory[0] = subInfo->pos;
        subInfo->pos +=
            subInfo->velocity *
            g_Supervisor.effectiveFramerateMultiplier;
        subInfo->velocity +=
            subInfo->accel *
            g_Supervisor.effectiveFramerateMultiplier;
        if (subInfo->pos.y < -256.0f)
        {
            subInfo->state = 0;
        }
        player->SpawnBombEffect(&subInfo->pos, 96.0f, 0.0f, 0,
                                ITEM_POINT_BULLET);
        if (player->bombDamageBoxes[i].damage < 80)
        {
            player->bombDamageBoxes[i].size.x = 128.0f;
            player->bombDamageBoxes[i].size.y = 128.0f;
            player->bombDamageBoxes[i].pos = subInfo->pos;
            player->bombDamageBoxes[i].lifetime = 12;
        }
        g_AnmManager->ExecuteScript(subInfo->vms);
    }
    player->playerState = PLAYER_STATE_INVULNERABLE;
    player->bombInfo.bombTimer++;
}

#pragma var_order(vm, i, subInfo)
// FUNCTION: TH07 0x0040b5d0
void BombData::BombMarisaADrawFocus(Player *player)
{
    PlayerBombSubInfo *subInfo;
    i32 i;
    AnmVm *vm;

    DarkenViewport(player);
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 24; i++, subInfo++)
    {
        if (player->bombInfo.subInfo[i].state == 0)
        {
            continue;
        }

        vm = subInfo->vms;
        vm->pos = subInfo->pos;
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.3f;
        vm->scale.x = 3.2f;
        vm->scale.y = 3.2f;
        g_AnmManager->Draw(vm);
        vm->pos = subInfo->posHistory[3];
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.32f;
        vm->scale.x = 2.2f;
        vm->scale.y = 2.2f;
        g_AnmManager->Draw(vm);
        vm->pos = subInfo->posHistory[7];
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.34f;
        vm->scale.x = 1.3f;
        vm->scale.y = 1.3f;
        g_AnmManager->Draw(vm);
        vm++;
    }
}

#pragma var_order(i, subInfo, offset, projectile, unused, j, angle)
// FUNCTION: TH07 0x0040b7d0
void BombData::BombMarisaBCalc(Player *player)
{
    f32 angle;
    i32 j;
    i32 unused[3];
    BombProjectile *projectile;
    f32 offset;
    PlayerBombSubInfo *subInfo;
    i32 i;

    if (player->bombInfo.bombTimer >= player->bombInfo.bombDuration)
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
        return;
    }

    if (player->bombInfo.bombTimer.HasTicked() &&
        player->bombInfo.bombTimer == 0)
    {
        g_ItemManager.RemoveAllItems();
        player->bombInfo.startPos = player->pos;
        g_Gui.ShowBombNamePortrait(ANM_SPRITE_FACE_PORTRAIT_ARRAY + 1,
                                   TH_BOMB_MARISA_B);
        player->bombInfo.bombDuration = 300;
        player->invulnerabilityTimer = 300;
        SpawnBombInvulnEffect(player);
        subInfo = player->bombInfo.subInfo;
        for (i = 0; i < 3; i++, subInfo++)
        {
            g_AnmManager->ExecuteAnmIdx(
                subInfo->vms, i + ANM_SCRIPT_PLAYER_MARISA_B_BOMB_ARRAY);
            subInfo->pos = player->pos;
            subInfo->custom = (f32)i * ZUN_2PI / 3.0f + (-ZUN_PI / 2.0f);
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_SAKUMARI, 0);
        player->verticalMovementSpeedMultiplierDuringBomb = 0.4f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 0.4f;
        ComputeBombCherryDrain(player, 8000, 0.35f);
    }
    else
    {
        subInfo = player->bombInfo.subInfo;
        projectile = player->bombDamageBoxes;
        for (i = 0; i < 3; i++, subInfo++)
        {
            if (player->bombInfo.startPos.x < 192.0f)
            {
                subInfo->custom = utils::AddNormalizeAngle(
                    subInfo->custom, player->bombInfo.bombTimer.AsFloat() *
                                         ZUN_PI /
                                         30.0f /
                                         (f32)player->bombInfo.bombDuration);
            }
            else
            {
                subInfo->custom = utils::AddNormalizeAngle(
                    subInfo->custom, player->bombInfo.bombTimer.AsFloat() *
                                         -ZUN_PI /
                                         30.0f /
                                         (f32)player->bombInfo.bombDuration);
            }
            angle = subInfo->custom;
            subInfo->vms[0].pos = player->pos;
            subInfo->vms[0].pos.x +=
                cosf(angle) * subInfo->vms[0].sprite->heightPx *
                subInfo->vms[0].scale.y /
                2.0f;
            subInfo->vms[0].pos.y +=
                sinf(angle) * subInfo->vms[0].sprite->heightPx *
                subInfo->vms[0].scale.y /
                2.0f;
            offset = 32.0f;
            for (j = 0; j < 6; j++, projectile++)
            {
                projectile->pos = player->pos;
                projectile->pos.x += cosf(angle) * offset;
                projectile->pos.y += sinf(angle) * offset;
                projectile->size.x = 128.0f;
                projectile->size.y = 128.0f;
                projectile->lifetime = 10;
                player->SpawnBombEffect(&projectile->pos, 64.0f, 0.0f, 0,
                                        ITEM_POINT_BULLET);
                offset =
                    subInfo->vms[0].sprite->heightPx * subInfo->vms[0].scale.y /
                        5.0f +
                    offset;
            }
            g_AnmManager->ExecuteScript(subInfo->vms);
        }
        if (player->bombInfo.bombTimer == 20)
        {
            ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 60, 1, 7, 0);
        }
        else if (player->bombInfo.bombTimer == 80)
        {
            ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 100, 24, 0, 0);
        }
    }
    player->playerState = PLAYER_STATE_INVULNERABLE;
    player->bombInfo.bombTimer++;
}

#pragma var_order(vm, i, angle)
// FUNCTION: TH07 0x0040bca0
void BombData::BombMarisaBDraw(Player *player)
{
    f32 angle;
    i32 i;
    AnmVm *vm;

    DarkenViewport(player);
    for (i = 0; i < 3; i++)
    {
        vm = player->bombInfo.subInfo[i].vms;
        angle = player->bombInfo.subInfo[i].custom;
        vm->pos = player->pos;
        vm->pos.x += cosf(angle) * vm->sprite->heightPx * vm->scale.y / 2.0f;
        vm->pos.y += sinf(angle) * vm->sprite->heightPx * vm->scale.y / 2.0f;
        vm->SetRotationZ(utils::AddNormalizeAngle(angle, ZUN_PI / 2.0f));
        vm->updateRotation = 1;
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.0f;
        g_AnmManager->Draw(vm);
        vm++;
    }
}

// FUNCTION: TH07 0x0040be20
void BombData::BombMarisaBCalcFocus(Player *player)
{
    i32 unused[3];
    AnmVm *vm;
    i32 i;

    if (player->bombInfo.bombTimer >= player->bombInfo.bombDuration)
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
        return;
    }

    if (player->bombInfo.bombTimer.HasTicked() &&
        player->bombInfo.bombTimer == 0)
    {
        g_ItemManager.RemoveAllItems();
        g_Gui.ShowBombNamePortrait(ANM_SPRITE_FACE_PORTRAIT_ARRAY + 2,
                                   TH_BOMB_MARISA_B_FOCUS);
        player->bombInfo.bombDuration = 340;
        player->invulnerabilityTimer = 390;
        SpawnBombInvulnEffect(player);
        vm = player->bombInfo.subInfo[0].vms;
        for (i = 0; i < 4; i++, vm++)
        {
            g_AnmManager->ExecuteAnmIdx(
                vm, i + ANM_SCRIPT_PLAYER_MARISA_B_FOCUS_BOMB_ARRAY);
            player->bombInfo.subInfo[i].pos =
                player->pos;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_SAKUMARI, 0);
        player->verticalMovementSpeedMultiplierDuringBomb = 0.2f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 0.2f;
        ComputeBombCherryDrain(player, 10000, 0.41f);
    }
    else
    {
        if (player->bombInfo.bombTimer == 60)
        {
            ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 60, 1, 7, 0);
        }
        else if (player->bombInfo.bombTimer == 120)
        {
            ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 200, 24, 0, 0);
        }
        if (player->bombInfo.bombTimer.HasTicked())
        {
            if (player->bombInfo.bombTimer.GetCurrent() % 4 != 0)
            {
                player->bombDamageBoxes[0].size.x = 384.0f;
                player->bombDamageBoxes[0].size.y = player->pos.y;
                player->bombDamageBoxes[0].pos.x = 192.0f;
                player->bombDamageBoxes[0].pos.y = player->pos.y / 2.0f;
                player->bombDamageBoxes[0].lifetime = 23;
                player->SpawnBombProjectile(&player->bombDamageBoxes[0].pos, 384.0f,
                                            player->pos.y, 6);
            }
        }
        g_AnmManager->ExecuteScript(&player->bombInfo.subInfo[0].vms[0]);
        g_AnmManager->ExecuteScript(&player->bombInfo.subInfo[0].vms[1]);
        g_AnmManager->ExecuteScript(&player->bombInfo.subInfo[0].vms[2]);
        g_AnmManager->ExecuteScript(&player->bombInfo.subInfo[0].vms[3]);
    }

    player->playerState = PLAYER_STATE_INVULNERABLE;
    player->bombInfo.bombTimer++;
}

#pragma var_order(vm, i, angle)
// FUNCTION: TH07 0x0040c160
void BombData::BombMarisaBDrawFocus(Player *player)
{
    f32 angle;
    i32 i;
    AnmVm *vm;

    DarkenViewport(player);
    vm = player->bombInfo.subInfo[0].vms;
    for (i = 0; i < 4; i++)
    {
        angle = (f32)i * (ZUN_PI / 5.0f) / 3.0f - ZUN_PI + (ZUN_2PI / 5.0f);
        vm->pos = player->pos;
        vm->pos.x +=
            cosf(angle) * vm->sprite->heightPx * vm->scale.y / 2.0f;
        vm->pos.y +=
            sinf(angle) * vm->sprite->heightPx * vm->scale.y / 2.0f;
        vm->SetRotationZ(utils::AddNormalizeAngle(angle, ZUN_PI / 2.0f));
        vm->updateRotation = 1;
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.0f;
        g_AnmManager->Draw(vm);
        vm++;
    }
}

#pragma var_order(i, subInfo, vm, unused, angle, spawnsRemaining)
// FUNCTION: TH07 0x0040c2e0
void BombData::BombSakuyaACalc(Player *player)
{
    i32 spawnsRemaining;
    f32 angle;
    i32 unused[3];
    AnmVm *vm;
    PlayerBombSubInfo *subInfo;
    i32 i;

    if (player->bombInfo.bombTimer >= player->bombInfo.bombDuration)
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        return;
    }

    if (player->GetBombTimer()->HasTickedAndIsEq(0))
    {
        g_ItemManager.RemoveAllItems();
        g_Gui.ShowBombNamePortrait(ANM_SPRITE_FACE_PORTRAIT_ARRAY + 1,
                                   TH_BOMB_SAKUYA_A);
        player->bombInfo.bombDuration = 160;
        player->invulnerabilityTimer = 210;
        SpawnBombInvulnEffect(player);
        player->bombInfo.startPos = player->pos;
        subInfo = player->bombInfo.subInfo;
        for (i = 0; i < 96; i++, subInfo++)
        {
            subInfo->state = 0;
        }
        ComputeBombCherryDrain(player, 6000, 0.28f);
        player->bombInfo.subInfo[0].effect = g_EffectManager.SpawnEffect(
            21, &player->pos, 1, 0xffffffff);
        g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_SAKUYA_A, 0);
    }
    if (player->bombInfo.bombTimer >= 60)
    {
        spawnsRemaining = 5;
        subInfo = player->bombInfo.subInfo;
        for (i = 0; i < 96; i++, subInfo++)
        {
            if (subInfo->state == 0)
            {
                if (player->bombInfo.bombTimer <= 120 && spawnsRemaining != 0)
                {
                    subInfo->state = 1;
                    vm = subInfo->vms;
                    g_AnmManager->ExecuteAnmIdx(
                        vm, ANM_SCRIPT_PLAYER_SAKUYA_A_BOMB_ARRAY + (i & 1));
                    angle = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
                    subInfo->angle = angle;
                    subInfo->speed = g_Rng.GetRandomFloatInRange(6.0f) + 5.5f;
                    subInfo->custom = g_Rng.GetRandomFloatInRange(0.1f) + 0.1f;
                    subInfo->accel.x =
                        g_Rng.GetRandomFloatInRange(ZUN_PI / 50.0f) - ZUN_PI / 100.0f;
                    subInfo->velocity.x = cosf(subInfo->angle) * 24.0f;
                    subInfo->velocity.y = sinf(subInfo->angle) * 24.0f;
                    subInfo->pos =
                        player->bombInfo.startPos + subInfo->velocity;
                    subInfo->velocity.z = 0.0f;
                    player->bombDamageBoxes[i].damage = 0;
                    spawnsRemaining--;
                }
                continue;
            }

            subInfo->angle = utils::AddNormalizeAngle(
                subInfo->angle, subInfo->accel.x);
            subInfo->speed = subInfo->speed + subInfo->custom;
            subInfo->velocity.x =
                cosf(subInfo->angle) * subInfo->speed;
            subInfo->velocity.y =
                sinf(subInfo->angle) * subInfo->speed;
            if (player->bombDamageBoxes[i].damage < 30)
            {
                subInfo->pos +=
                    subInfo->velocity *
                    g_Supervisor.effectiveFramerateMultiplier;
                player->SpawnBombEffect(&subInfo->pos, 32.0f, 0.0f,
                                        0, ITEM_POINT_BULLET);
                player->bombDamageBoxes[i].size.x = 24.0f;
                player->bombDamageBoxes[i].size.y = 24.0f;
                player->bombDamageBoxes[i].pos = subInfo->pos;
                player->bombDamageBoxes[i].lifetime = 10;
            }
            else if (player->bombDamageBoxes[i].damage < 999)
            {
                g_AnmManager->ExecuteAnmIdx(subInfo->vms, 1120);
                player->bombDamageBoxes[i].damage = 999;
            }
            if (g_GameManager.IsInBounds(subInfo->pos.x,
                                         subInfo->pos.y, 64.0f,
                                         64.0f) == 0)
            {
                subInfo->state = 0;
            }
            g_AnmManager->ExecuteScript(subInfo->vms);
        }
    }
    player->playerState = PLAYER_STATE_INVULNERABLE;
    player->bombInfo.bombTimer++;
}

#pragma var_order(i, subInfo, angle, vm)
// FUNCTION: TH07 0x0040c970
void BombData::BombSakuyaADraw(Player *player)
{
    AnmVm *vm;
    f32 angle;
    PlayerBombSubInfo *subInfo;
    i32 i;

    DarkenViewport(player);
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 96; i++, subInfo++)
    {
        if (subInfo->state == 0)
        {
            continue;
        }

        angle = utils::AddNormalizeAngle(subInfo->angle, ZUN_PI / 2.0f);
        vm = subInfo->vms;
        vm->rotation.z = angle;
        vm->updateRotation = 1;
        subInfo->vms[0].pos = subInfo->pos;
        subInfo->vms[0].pos.z = 0.0f;
        g_AnmManager->Draw(subInfo->vms);
    }
}

#pragma var_order(i, subInfo, vm, unused, angle)
// FUNCTION: TH07 0x0040ca50
void BombData::BombSakuyaACalcFocus(Player *player)
{
    f32 angle;
    i32 unused[3];
    AnmVm *vm;
    PlayerBombSubInfo *subInfo;
    i32 i;

    if (player->bombInfo.bombTimer >= player->bombInfo.bombDuration)
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
        return;
    }

    if (player->GetBombTimer()->HasTickedAndIsEq(0))
    {
        g_ItemManager.RemoveAllItems();
        g_Gui.ShowBombNamePortrait(ANM_SPRITE_FACE_PORTRAIT_ARRAY + 1,
                                   TH_BOMB_SAKUYA_A_FOCUS);
        player->bombInfo.bombDuration = 250;
        player->invulnerabilityTimer = 290;
        SpawnBombInvulnEffect(player);
        subInfo = player->bombInfo.subInfo;
        for (i = 0; i < 96; i++, subInfo++)
        {
            subInfo->state = 0;
        }
        ComputeBombCherryDrain(player, 6500, 0.29f);
        player->verticalMovementSpeedMultiplierDuringBomb = 0.3f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 0.3f;
        player->bombInfo.subInfo[0].effect = g_EffectManager.SpawnEffect(
            21, &player->pos, 1, 0xffffffff);
        g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_SAKUYA_A, 0);
    }
    if (player->bombInfo.bombTimer >= 0 &&
        player->bombInfo.bombTimer <= 60)
    {
        player->bombInfo.subInfo[0].effect->pos = player->pos;
    }
    if (player->bombInfo.bombTimer >= 20 &&
        player->bombInfo.bombTimer < 116)
    {
        subInfo = player->bombInfo.subInfo;
        for (i = 0; i < 96; i++, subInfo++)
        {
            if (!player->GetBombTimer()->HasTickedAndIsEq(i % 48 * 2 + 20))
            {
                continue;
            }

            subInfo->state = 1;
            vm = subInfo->vms;
            g_AnmManager->ExecuteAnmIdx(
                vm, (i & 1) + ANM_SCRIPT_PLAYER_SAKUYA_A_FOCUS_BOMB_ARRAY);
            angle = (f32)i * ZUN_2PI / 96.0f - ZUN_PI;
            subInfo->angle = angle;
            subInfo->speed = g_Rng.GetRandomFloatInRange(1.0f) + 0.5f;
            subInfo->custom = g_Rng.GetRandomFloatInRange(0.1f) + 0.03f;
            subInfo->accel.x =
                g_Rng.GetRandomU16InRange(1) ? ZUN_PI / 20.0f
                                             : -ZUN_PI / 20.0f;
            subInfo->velocity.x = cosf(subInfo->angle) * 24.0f;
            subInfo->velocity.y = sinf(subInfo->angle) * 24.0f;
            subInfo->pos =
                player->pos + subInfo->velocity;
            subInfo->timer = 0;
            subInfo->velocity.z = 0.0f;
            player->bombDamageBoxes[i].damage = 0;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_REIMARI, 0);
        ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 120, 4, 1, 0);
    }
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 96; i++, subInfo++)
    {
        if (subInfo->state == 0)
        {
            continue;
        }

        if (subInfo->timer.GetCurrent() < 30 ||
            subInfo->timer.GetCurrent() >= 70)
        {
            if (subInfo->timer.HasTickedAndIsEq(70))
            {
                if (player->positionOfLastEnemyHit.x > -100.0f)
                {
                    subInfo->angle = utils::AddNormalizeAngle(
                        atan2f(player->positionOfLastEnemyHit.y -
                                   subInfo->pos.y,
                               player->positionOfLastEnemyHit.x -
                                   subInfo->pos.x),
                        0.0f);
                }
                subInfo->speed = 14.0f;
            }
            subInfo->speed = subInfo->speed + subInfo->custom;
            subInfo->velocity.x =
                cosf(subInfo->angle) * subInfo->speed;
            subInfo->velocity.y =
                sinf(subInfo->angle) * subInfo->speed;
        }
        else
        {
            subInfo->angle = utils::AddNormalizeAngle(
                subInfo->angle, subInfo->accel.x);
            subInfo->velocity.x = 0.0f;
            subInfo->velocity.y = 0.0f;
        }
        if (player->bombDamageBoxes[i].damage == 0)
        {
            subInfo->pos +=
                subInfo->velocity *
                g_Supervisor.effectiveFramerateMultiplier;
            player->SpawnBombEffect(
                &player->bombInfo.subInfo[i].pos, 32.0f,
                0.0f, 0, ITEM_POINT_BULLET);
            player->bombDamageBoxes[i].size.x = 24.0f;
            player->bombDamageBoxes[i].size.y = 24.0f;
            player->bombDamageBoxes[i].pos = subInfo->pos;
            player->bombDamageBoxes[i].lifetime = 22;
        }
        else if (player->bombDamageBoxes[i].damage < 999)
        {
            g_AnmManager->ExecuteAnmIdx(subInfo->vms, 1120);
            player->bombDamageBoxes[i].damage = 999;
            g_EffectManager.SpawnEffect(
                0, &player->bombInfo.subInfo[i].pos, 1,
                0xffff80ff);
        }
        g_AnmManager->ExecuteScript(subInfo->vms);
        subInfo->timer++;
    }
    player->playerState = PLAYER_STATE_INVULNERABLE;
    player->bombInfo.bombTimer++;
}

#pragma var_order(i, subInfo, angle, vm)
// FUNCTION: TH07 0x0040d3b0
void BombData::BombSakuyaADrawFocus(Player *player)
{
    AnmVm *vm;
    f32 angle;
    PlayerBombSubInfo *subInfo;
    i32 i;

    DarkenViewport(player);
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 96; i++, subInfo++)
    {
        if (subInfo->state == 0)
        {
            continue;
        }
        angle = utils::AddNormalizeAngle(subInfo->angle, ZUN_PI / 2.0f);
        vm = subInfo->vms;
        vm->rotation.z = angle;
        vm->updateRotation = 1;
        subInfo->vms[0].pos = subInfo->pos;
        subInfo->vms[0].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        subInfo->vms[0].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        subInfo->vms[0].pos.z = 0.0f;
        g_AnmManager->Draw(subInfo->vms);
    }
}

#pragma var_order(i, subInfo, vm, unused)
// FUNCTION: TH07 0x0040d4c0
void BombData::BombSakuyaBCalc(Player *player)
{
    i32 unused[3];
    AnmVm *vm;
    PlayerBombSubInfo *subInfo;
    i32 i;

    if (player->bombInfo.bombTimer >= player->bombInfo.bombDuration)
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->SpawnBombEffect(&player->pos, 800.0f, 0.0f, 0,
                                ITEM_POINT_BULLET);
        return;
    }

    if (player->GetBombTimer()->HasTickedAndIsEq(0))
    {
        g_ItemManager.RemoveAllItems();
        g_Gui.ShowBombNamePortrait(ANM_SPRITE_FACE_PORTRAIT_ARRAY + 3,
                                   TH_BOMB_SAKUYA_B);
        player->bombInfo.bombDuration = 160;
        player->invulnerabilityTimer = 260;
        SpawnBombInvulnEffect(player);
        subInfo = player->bombInfo.subInfo;
        for (i = 0; i < 4; i++, subInfo++)
        {
            subInfo->state = 0;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_SAKUMARI, 0);
        ComputeBombCherryDrain(player, 5500, 0.26f);
        player->verticalMovementSpeedMultiplierDuringBomb = 2.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 2.0f;
        g_BulletManager.StopBulletMovement();
    }
    if (player->GetBombTimer()->HasTickedAndIsEq(60))
    {
        g_BulletManager.StopBulletMovement();
    }
    if (player->GetBombTimer()->HasTickedAndIsEq(120))
    {
        g_BulletManager.StopBulletMovement();
    }
    if (player->GetBombTimer()->HasTickedAndIsEq(30))
    {
        subInfo = player->bombInfo.subInfo;
        for (i = 0; i < 4; i++, subInfo++)
        {
            subInfo->state = 1;
            vm = subInfo->vms;
            g_AnmManager->ExecuteAnmIdx(
                vm, i + ANM_SCRIPT_PLAYER_SAKUYA_B_BOMB_ARRAY);
            vm->pos.x = 192.0f + ((i & 1) != 0 ? 128.0f : -128.0f);
            vm->pos.y = 224.0f + (i / 2 != 0 ? 128.0f : -128.0f);
            vm->pos.z = 0.49f;
            subInfo->velocity.z = 0.0f;
        }
    }
    if (player->bombInfo.bombTimer >= 30 &&
        player->bombInfo.bombTimer.HasTicked())
    {
        if (player->bombInfo.bombTimer.current % 4 == 0)
        {
            player->bombDamageBoxes[0].pos.x = 192.0f;
            player->bombDamageBoxes[0].pos.y = 224.0f;
            player->bombDamageBoxes[0].size.x = 352.0f;
            player->bombDamageBoxes[0].size.y = 416.0f;
            player->bombDamageBoxes[0].lifetime = 3;
        }
    }

    if (player->GetBombTimer()->HasTickedAndIsEq(40))
    {
        ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 60, 1, 7, 0);
    }
    if (player->GetBombTimer()->HasTickedAndIsEq(100))
    {
        ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 70, 24, 0, 0);
    }
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 4; i++, subInfo++)
    {
        if (subInfo->state == 0)
        {
            continue;
        }

        g_AnmManager->ExecuteScript(subInfo->vms);
    }
    player->playerState = PLAYER_STATE_INVULNERABLE;
    player->bombInfo.bombTimer++;
}

#pragma var_order(i, subInfo)
// FUNCTION: TH07 0x0040d9a0
void BombData::BombSakuyaBDraw(Player *player)
{
    PlayerBombSubInfo *subInfo;
    i32 i;

    DarkenViewport(player);
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 4; i++, subInfo++)
    {
        if (subInfo->state == 0)
        {
            continue;
        }

        subInfo->vms[0].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        subInfo->vms[0].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        subInfo->vms[0].pos.z = 0.0f;
        g_AnmManager->Draw(subInfo->vms);
        subInfo->vms[0].pos.x -= g_GameManager.arcadeRegionTopLeftPos.x;
        subInfo->vms[0].pos.y -= g_GameManager.arcadeRegionTopLeftPos.y;
    }
}

#pragma var_order(i, subInfo, vm, unused, j)
// FUNCTION: TH07 0x0040da80
void BombData::BombSakuyaBCalcFocus(Player *player)
{
    i32 j;
    i32 unused[3];
    AnmVm *vm;
    PlayerBombSubInfo *subInfo;
    i32 i;

    if (player->bombInfo.bombTimer >= player->bombInfo.bombDuration)
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->SpawnBombEffect(&player->pos, 800.0f, 0.0f, 0,
                                ITEM_POINT_BULLET);
        player->bombClearBoxes[0].pos.x = 192.0f;
        player->bombClearBoxes[0].pos.y = 224.0f;
        player->bombClearBoxes[0].pos.z = 448.0f;
        player->bombClearBoxes[0].size.x = 512.0f;
        return;
    }

    if (player->GetBombTimer()->HasTickedAndIsEq(0))
    {
        g_ItemManager.RemoveAllItems();
        g_Gui.ShowBombNamePortrait(ANM_SPRITE_FACE_PORTRAIT_ARRAY + 3,
                                   TH_BOMB_SAKUYA_B_FOCUS);
        player->bombInfo.bombDuration = 300;
        player->invulnerabilityTimer = 420;
        SpawnBombInvulnEffect(player);
        player->isBombing = 0;
        subInfo = player->bombInfo.subInfo;
        for (i = 0; i < 2; i++, subInfo++)
        {
            subInfo->state = 1;
            vm = subInfo->vms;
            g_AnmManager->ExecuteAnmIdx(
                vm, i + ANM_SCRIPT_PLAYER_SAKUYA_B_FOCUS_BOMB_ARRAY);
            subInfo->pos = player->pos;
            for (j = 31; j >= 0; j--)
            {
                subInfo->posHistory[j] =
                    subInfo->pos;
            }
            subInfo->velocity.x = 0.0f;
            subInfo->velocity.y = 0.0f;
            subInfo->velocity.z = 0.0f;
            subInfo->accel.x = 0.0f;
            subInfo->accel.y = -0.008f;
            subInfo->accel.z = 0.0f;
        }
        player->verticalMovementSpeedMultiplierDuringBomb = 1.5f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.5f;
        g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_SAKUMARI, 0);
        ComputeBombCherryDrain(player, 6000, 0.29f);
    }
    player->SpawnBombEffect(&player->bombInfo.subInfo[0].pos,
                            96.0f, 0.0f, 0, ITEM_POINT_BULLET);
    player->bombDamageBoxes[0].pos.x =
        player->bombInfo.subInfo[0].pos.x;
    player->bombDamageBoxes[0].pos.y =
        player->bombInfo.subInfo[0].pos.y;
    player->bombDamageBoxes[0].size.x = 160.0f;
    player->bombDamageBoxes[0].size.y = 160.0f;
    player->bombDamageBoxes[0].lifetime = 1;
    if (player->GetBombTimer()->HasTickedAndIsEq(40))
    {
        ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 60, 1, 7, 0);
        g_BulletManager.StopBulletMovement();
    }
    if (player->GetBombTimer()->HasTickedAndIsEq(100))
    {
        g_BulletManager.StopBulletMovement();
        ScreenEffect::RegisterChain(SCREEN_EFFECT_SHAKE, 70, 24, 0, 0);
    }
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 2; i++, subInfo++)
    {
        if (subInfo->state == 0)
        {
            continue;
        }

        subInfo->accel =
            (player->pos - subInfo->pos) / 1700.0f *
            g_Supervisor.effectiveFramerateMultiplier;
        subInfo->velocity +=
            subInfo->accel *
            g_Supervisor.effectiveFramerateMultiplier;
        for (j = 31; j > 0; j--)
        {
            subInfo->posHistory[j] =
                subInfo->posHistory[j - 1];
        }
        subInfo->posHistory[0] = subInfo->pos;
        subInfo->pos +=
            subInfo->velocity *
            g_Supervisor.effectiveFramerateMultiplier;
        g_AnmManager->ExecuteScript(subInfo->vms);
    }
    player->playerState = PLAYER_STATE_INVULNERABLE;
    player->bombInfo.bombTimer++;
}

#pragma var_order(oldAlpha, i, subInfo, j)
// FUNCTION: TH07 0x0040e280
void BombData::BombSakuyaBDrawFocus(Player *player)
{
    i32 j;
    PlayerBombSubInfo *subInfo;
    i32 i;
    i32 oldAlpha;

    DarkenViewport(player);
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 2; i++, subInfo++)
    {
        if (subInfo->state == 0)
        {
            continue;
        }

        oldAlpha = subInfo->vms[0].color.bytes.a;
        subInfo->vms[0].pos = subInfo->pos;
        subInfo->vms[0].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        subInfo->vms[0].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        subInfo->vms[0].pos.z = 0.0f;
        g_AnmManager->Draw(subInfo->vms);
        for (j = 3; j < 32; j += 4)
        {
            subInfo->vms[0].pos = subInfo->posHistory[j];
            subInfo->vms[0].color.bytes.a =
                oldAlpha - oldAlpha * j / 32;
            subInfo->vms[0].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            subInfo->vms[0].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            subInfo->vms[0].pos.z = 0.0f;
            g_AnmManager->Draw(subInfo->vms);
        }
        subInfo->vms[0].color.bytes.a = oldAlpha;
    }
}
