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
    ZunColor local_8;
    local_8.bytes.a = 0x80;
    if ((i32)(player->bombInfo.bombTimer.current < 60))
    {
        local_8.bytes.b =
            0x80 - (player->bombInfo.bombTimer.GetCurrent() * 80 / 60);
        local_8.bytes.g = local_8.bytes.b;
        local_8.bytes.r = local_8.bytes.g;
    }
    else if ((i32)(player->bombInfo.bombTimer.current >=
                   player->bombInfo.bombDuration - 60))
    {
        local_8.bytes.b = 0x80 - ((player->bombInfo.bombDuration -
                                   player->bombInfo.bombTimer.GetCurrent()) *
                                  80 / 60);
        local_8.bytes.g = local_8.bytes.b;
        local_8.bytes.r = local_8.bytes.g;
    }
    else
    {
        local_8.bytes.b = 0x30;
        local_8.bytes.g = local_8.bytes.b;
        local_8.bytes.r = local_8.bytes.g;
    }
    g_AnmManager->SetColor(0x80808080);
    g_Stage.SmoothBlendColor(local_8);
    g_Stage.isDarkening = 1;
}

// FUNCTION: TH07 0x004084f0
void BombData::SpawnBombInvulnEffect(Player *player)
{
    if (player->effect != NULL)
    {
        player->effect->inUseFlag = 0;
    }
    Effect *effect = g_EffectManager.SpawnEffect(0x19, &player->positionCenter, 0,
                                                 1, 0xffffffff);
    effect->vm.interpStartTimes[4].InitializeForPopup();
    effect->vm.interpEndTimes[4] = player->invulnerabilityTimer;
    effect->vm.interpModes[4] = 0;
    effect->vm.scaleInterpInitial = effect->vm.scale;
    effect->vm.scaleInterpFinal.x = 0.0625;
    effect->vm.scaleInterpFinal.y = 0.0625;
    effect->vm.intVars1[0] = player->invulnerabilityTimer.GetCurrent();
    effect->vm.angleVel.z *= -1.0f;
    effect->vm.color.bytes.r = 0xff;
    effect->vm.color.bytes.g = 0x40;
    effect->vm.color.bytes.b = 0x40;
    player->effect = effect;
}

// FUNCTION: TH07 0x00408610
void BombData::ComputeBombCherryDrain(Player *player, i32 minCost, f32 scale)
{
    i32 iVar1;

    i32 local_8 =
        (f32)(g_GameManager.cherry - g_GameManager.globals->cherryStart) * scale;
    switch (g_GameManager.difficulty)
    {
    case DIFF_HARD:
        local_8 = local_8 / 2;
        break;
    case DIFF_LUNATIC:
        local_8 = local_8 / 4;
        break;
    case DIFF_EXTRA:
    case DIFF_PHANTASM:
        local_8 = local_8 / 3;
    }
    i32 local_1c = local_8 / player->bombInfo.bombDuration;
    local_1c -= local_1c % 10;
    iVar1 = minCost / player->bombInfo.bombDuration;
    iVar1 -= iVar1 % 10;
    if (local_1c < iVar1)
    {
        local_1c = iVar1;
    }
    player->bombInfo.bombCherryDrain = local_1c;
}

// FUNCTION: TH07 0x00408710
void BombData::BombReimuACalc(Player *player)
{
    i32 iVar4;
    i32 iVar5;
    i32 local_28;
    AnmVm *local_18;
    PlayerBombSubInfo *local_14;
    i32 local_10;
    f32 local_8;

    if (player->bombInfo.bombTimer.current < player->bombInfo.bombDuration)
    {
        if ((player->bombInfo.bombTimer.current !=
             player->bombInfo.bombTimer.previous) &&
            (player->bombInfo.bombTimer.current == 0))
        {
            g_Gui.ShowBombNamePortrait(0x4a1, "霊符「夢想封印　集」");
            player->bombInfo.bombDuration = 0x8c;
            player->invulnerabilityTimer = 200;
            SpawnBombInvulnEffect(player);
            for (local_10 = 0; local_10 < 0x20; local_10 += 1)
            {
                player->bombInfo.subInfo[local_10].state = 0;
            }
            g_ItemManager.RemoveAllItems();
            g_EffectManager.SpawnParticles(0xc, &player->positionCenter, 1,
                                           0xff4040ff);
            player->SpawnBombEffect(&player->positionCenter, 32.0f, 8.0f, 0x10,
                                    ITEM_POINT_BULLET);
            player->bombStartPos = player->positionCenter;
            ComputeBombCherryDrain(player, 4000, 0.2f);
        }
        if ((((player->bombInfo.bombTimer.current !=
               player->bombInfo.bombTimer.previous) &&
              (7 < player->bombInfo.bombTimer.current)) &&
             (player->bombInfo.bombTimer.current < 0x50)) &&
            (player->bombInfo.bombTimer.current % 6 == 0))
        {
            iVar4 = (player->bombInfo.bombTimer.current - 8) / 6;
            player->bombInfo.subInfo[iVar4].state = 1;
            player->bombInfo.subInfo[iVar4].speed = 15.0f;
            player->bombInfo.subInfo[iVar4].bombRegionPositions =
                player->positionCenter;
            iVar5 = iVar4;
            if (192.0f <= (player->bombStartPos).x)
            {
                iVar5 = -iVar4;
            }
            local_8 = ((f32)iVar5 * ZUN_2PI) / 8.0f - 1.5707964f;
            player->bombInfo.subInfo[iVar4].angle =
                utils::AddNormalizeAngle(local_8, 0.0f);
            player->bombInfo.subInfo[iVar4].counter = 0;
            player->bombProjectiles[iVar4].payload = 0;
            local_18 = player->bombInfo.subInfo[iVar4].vms;
            for (local_28 = 0; local_28 < 4; local_28 += 1)
            {
                g_AnmManager->ExecuteAnmIdx(local_18, local_28 + 0x485);
                local_18 = local_18 + 1;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_REIMU_A, 0);
        }
        player->playerState = PLAYER_STATE_INVULNERABLE;
        local_14 = player->bombInfo.subInfo;
        for (local_10 = 0; local_10 < 8; local_10 += 1)
        {
            if (local_14->state != 0)
            {
                if (local_14->state == 1)
                {
                    local_14->speed = local_14->speed -
                                      g_Supervisor.effectiveFramerateMultiplier * 0.4f;
                    AngleToVector(&local_14->bombRegionVelocities, local_14->angle,
                                  local_14->speed);
                    if (local_14->speed < -10.0f)
                    {
                        g_EffectManager.SpawnParticles(6, &local_14->bombRegionPositions, 8,
                                                       0xffffffff);
                        g_EffectManager.SpawnParticles(0xc, &local_14->bombRegionPositions,
                                                       1, 0xff4040ff);
                        local_14->state = 2;
                        local_14->vms[0].pendingInterrupt = 1;
                        local_14->vms[1].pendingInterrupt = 1;
                        local_14->vms[2].pendingInterrupt = 1;
                        local_14->vms[3].pendingInterrupt = 1;
                        player->bombProjectiles[local_10].pos =
                            local_14->bombRegionPositions;
                        player->bombProjectiles[local_10].size.x = 256.0f;
                        player->bombProjectiles[local_10].size.y = 256.0f;
                        player->bombProjectiles[local_10].lifetime = 400;
                        player->SpawnBombEffect(&local_14->bombRegionPositions, 64.0f,
                                                4.266667f, 0x1e, ITEM_POINT_BULLET);
                        local_14->bombRegionVelocities.x = 0.0f;
                        local_14->bombRegionVelocities.y = 0.0f;
                        local_14->bombRegionVelocities.z = 0.0f;
                        g_SoundPlayer.PlaySoundByIdx(SOUND_ENEMY_SPELLCARD_END, 0);
                        BombEffects::RegisterChain(1, 0x10, 8, 0, 0);
                    }
                    if (player->bombInfo.bombTimer.current !=
                        player->bombInfo.bombTimer.previous)
                    {
                        player->bombProjectiles[local_10].size.x = 48.0f;
                        player->bombProjectiles[local_10].size.y = 48.0f;
                        player->bombProjectiles[local_10].pos =
                            local_14->bombRegionPositions;
                        player->bombProjectiles[local_10].lifetime = 8;
                        player->SpawnBombEffect(&local_14->bombRegionPositions, 128.0f,
                                                0.0f, 0, ITEM_POINT_BULLET);
                    }
                }
                else if ((local_14->state != 0) &&
                         (player->bombInfo.bombTimer.current !=
                          player->bombInfo.bombTimer.previous))
                {
                    player->bombProjectiles[local_10].pos =
                        player->bombInfo.subInfo[local_10].bombRegionPositions;
                    player->bombProjectiles[local_10].size.x = 256.0f;
                    player->bombProjectiles[local_10].size.y = 256.0f;
                    player->bombProjectiles[local_10].lifetime = 2;
                    local_14->counter = local_14->counter + 1;
                    if (0x1d < local_14->counter)
                    {
                        local_14->state = 0;
                    }
                }
                local_14->bombRegionPositions.x +=
                    g_Supervisor.effectiveFramerateMultiplier *
                    local_14->bombRegionVelocities.x;
                local_14->bombRegionPositions.y +=
                    g_Supervisor.effectiveFramerateMultiplier *
                    local_14->bombRegionVelocities.y;
                g_AnmManager->ExecuteScript(&local_14->vms[0]);
                g_AnmManager->ExecuteScript(&local_14->vms[1]);
                g_AnmManager->ExecuteScript(&local_14->vms[2]);
                g_AnmManager->ExecuteScript(&local_14->vms[3]);
            }
            local_14 = local_14 + 1;
        }
        player->bombInfo.bombTimer.Tick();
    }
    else
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
    }
}

// FUNCTION: TH07 0x00408e10
void BombData::BombReimuADraw(Player *player)
{
    PlayerBombSubInfo *subInfo;
    i32 i;

    DarkenViewport(player);
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 8; i++)
    {
        if (subInfo->state != 0)
        {
            subInfo->vms[0].pos =
                subInfo->bombRegionPositions + subInfo->vms[0].offset;
            subInfo->vms[0].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            subInfo->vms[0].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            subInfo->vms[0].pos.z = 0.0f;
            g_AnmManager->DrawNoRotation(&subInfo->vms[0]);
            subInfo->vms[1].pos =
                subInfo->bombRegionPositions + subInfo->vms[1].offset;
            subInfo->vms[1].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            subInfo->vms[1].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            subInfo->vms[1].pos.z = 0.0f;
            g_AnmManager->DrawNoRotation(&subInfo->vms[1]);
            subInfo->vms[2].pos =
                subInfo->bombRegionPositions + subInfo->vms[2].offset;
            subInfo->vms[2].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            subInfo->vms[2].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            subInfo->vms[2].pos.z = 0.0f;
            g_AnmManager->DrawNoRotation(&subInfo->vms[2]);
            subInfo->vms[3].pos =
                subInfo->bombRegionPositions + subInfo->vms[3].offset;
            subInfo->vms[3].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            subInfo->vms[3].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            subInfo->vms[3].pos.z = 0.0f;
            g_AnmManager->DrawNoRotation(&subInfo->vms[3]);
        }
        subInfo = subInfo + 1;
    }
}

// FUNCTION: TH07 0x004091b0
void BombData::BombReimuACalcFocus(Player *player)
{
    PlayerBombSubInfo *pPVar1;
    BombProjectile *pBVar2;
    f32 fVar3;
    i32 iVar5;
    f32 fVar6;
    f32 fVar7;
    f32 local_94;
    i32 local_30;
    f32 local_2c;
    f32 local_28;
    f32 local_20;
    AnmVm *local_1c;
    PlayerBombSubInfo *local_18;
    i32 local_14;

    if (player->bombInfo.bombTimer.current < player->bombInfo.bombDuration)
    {
        if ((player->bombInfo.bombTimer.current !=
             player->bombInfo.bombTimer.previous) &&
            (player->bombInfo.bombTimer.current == 0))
        {
            g_Gui.ShowBombNamePortrait(0x4a1, "夢符「封魔陣」");
            player->bombInfo.bombDuration = 300;
            player->invulnerabilityTimer = 360;
            SpawnBombInvulnEffect(player);
            for (local_14 = 0; local_14 < 8; local_14 += 1)
            {
                player->bombInfo.subInfo[local_14].state = 0;
            }
            g_ItemManager.RemoveAllItems();
            g_EffectManager.SpawnParticles(0xc, &player->positionCenter, 1,
                                           0xff4040ff);
            player->SpawnBombEffect(&player->positionCenter, 32.0f, 8.0f, 0x10,
                                    ITEM_POINT_BULLET);
            ComputeBombCherryDrain(player, 5000, 0.22f);
            player->verticalMovementSpeedMultiplierDuringBomb = 0.6f;
            player->horizontalMovementSpeedMultiplierDuringBomb = 0.6f;
        }
        if ((player->bombInfo.bombTimer.current > 60) &&
            (player->bombInfo.bombTimer.current < 180))
        {
            if (player->bombInfo.bombTimer.current % 16 == 0)
            {
                iVar5 = player->bombInfo.bombTimer.current - 60;
                iVar5 = (i32)(iVar5 / 16);
                pPVar1 = player->bombInfo.subInfo + iVar5;
                if (iVar5 != 0)
                {
                    pPVar1->state = 1;
                    pPVar1->counter = 0;
                    pPVar1->accel = 8.0f;
                    pPVar1->bombRegionPositions = player->positionCenter;
                    AngleToVector(&pPVar1->bombRegionVelocities,
                                  g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI,
                                  pPVar1->accel);
                    player->bombProjectiles[iVar5].payload = 0;
                    local_1c = pPVar1->vms;
                    for (local_30 = 0; local_30 < 4; local_30 += 1)
                    {
                        g_AnmManager->ExecuteAnmIdx(local_1c, local_30 + 0x485);
                        local_1c = local_1c + 1;
                    }
                    g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_REIMU_A, 0);
                }
            }
        }
        player->playerState = PLAYER_STATE_INVULNERABLE;
        local_18 = player->bombInfo.subInfo;
        for (local_14 = 0; local_14 < 8; local_14 += 1)
        {
            if (local_18->state != 0)
            {
                if (local_18->state == 1)
                {
                    if (player->bombInfo.bombTimer.current !=
                        player->bombInfo.bombTimer.previous)
                    {
                        if ((player->positionOfLastEnemyHit).x <= -100.0f)
                        {
                            local_2c = player->positionCenter.x;
                            local_28 = player->positionCenter.y;
                        }
                        else
                        {
                            local_2c = (player->positionOfLastEnemyHit).x;
                            local_28 = (player->positionOfLastEnemyHit).y;
                        }
                        local_2c = local_2c - (local_18->bombRegionPositions).x;
                        local_28 = local_28 - (local_18->bombRegionPositions).y;
                        local_20 = sqrtf(local_2c * local_2c + local_28 * local_28);
                        local_20 = local_20 / (local_18->accel / 8.0f);
                        if (local_20 < 1.0f)
                        {
                            local_20 = 1.0f;
                        }
                        fVar6 = local_2c / local_20 + (local_18->bombRegionVelocities).x;
                        fVar3 = local_28 / local_20 + (local_18->bombRegionVelocities).y;
                        fVar7 = sqrtf(fVar6 * fVar6 + fVar3 * fVar3);
                        local_94 = fVar7;
                        if (10.0f < fVar7)
                        {
                            local_94 = 10.0f;
                        }
                        local_18->accel = local_94;
                        if (local_18->accel < 1.0f)
                        {
                            local_18->accel = 1.0f;
                        }
                        (local_18->bombRegionVelocities).x =
                            (fVar6 * local_18->accel) / fVar7;
                        (local_18->bombRegionVelocities).y =
                            (fVar3 * local_18->accel) / fVar7;
                        player->bombProjectiles[local_14].size.x = 48.0f;
                        player->bombProjectiles[local_14].size.y = 48.0f;
                        pBVar2 = player->bombProjectiles + local_14;
                        pBVar2->pos = local_18->bombRegionPositions;
                        player->bombProjectiles[local_14].lifetime = 8;
                        player->SpawnBombEffect(&local_18->bombRegionPositions, 128.0f,
                                                0.0f, 0, ITEM_POINT_BULLET);
                        if ((99 < player->bombProjectiles[local_14].payload) ||
                            (player->bombInfo.bombDuration - 0x1e <=
                             player->bombInfo.bombTimer.current))
                        {
                            g_EffectManager.SpawnParticles(6, &local_18->bombRegionPositions,
                                                           8, 0xffffffff);
                            g_EffectManager.SpawnParticles(
                                0xc, &local_18->bombRegionPositions, 1, 0xff4040ff);
                            local_18->state = 2;
                            local_18->vms[0].pendingInterrupt = 1;
                            local_18->vms[1].pendingInterrupt = 1;
                            local_18->vms[2].pendingInterrupt = 1;
                            local_18->vms[3].pendingInterrupt = 1;
                            player->bombProjectiles[local_14].size.x = 256.0f;
                            player->bombProjectiles[local_14].size.y = 256.0f;
                            player->bombProjectiles[local_14].lifetime = 400;
                            player->SpawnBombEffect(&local_18->bombRegionPositions, 32.0f,
                                                    6.6666665f, 0xf, ITEM_POINT_BULLET);
                            g_SoundPlayer.PlaySoundByIdx(SOUND_ENEMY_SPELLCARD_END, 0);
                            BombEffects::RegisterChain(1, 0x10, 8, 0, 0);
                        }
                    }
                }
                else if (((local_18->state != 0) &&
                          (player->bombInfo.bombTimer.current !=
                           player->bombInfo.bombTimer.previous)) &&
                         (local_18->counter = local_18->counter + 1,
                          0x1d < local_18->counter))
                {
                    local_18->state = 0;
                }
                (local_18->bombRegionPositions).x +=
                    g_Supervisor.effectiveFramerateMultiplier *
                    (local_18->bombRegionVelocities).x;
                (local_18->bombRegionPositions).y +=
                    g_Supervisor.effectiveFramerateMultiplier *
                    (local_18->bombRegionVelocities).y;
                g_AnmManager->ExecuteScript(&local_18->vms[0]);
                g_AnmManager->ExecuteScript(&local_18->vms[1]);
                g_AnmManager->ExecuteScript(&local_18->vms[2]);
                g_AnmManager->ExecuteScript(&local_18->vms[3]);
            }
            local_18 = local_18 + 1;
        }
        player->bombInfo.bombTimer.Tick();
    }
    else
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
    }
}

// FUNCTION: TH07 0x00409990
void BombData::BombReimuADrawFocus(Player *player)
{
    i32 i;
    AnmVm *vm;

    DarkenViewport(player);
    for (i = 0; i < 8; i++)
    {
        if (player->bombInfo.subInfo[i].state != 0)
        {
            vm = player->bombInfo.subInfo[i].vms;
            vm->pos = player->bombInfo.subInfo[i].bombRegionPositions + vm->offset;
            vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            vm->pos.z = 0.0f;
            g_AnmManager->DrawNoRotation(&vm[0]);
            vm[1].pos =
                player->bombInfo.subInfo[i].bombRegionPositions + vm[1].offset;
            vm[1].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            vm[1].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            vm[1].pos.z = 0.0f;
            g_AnmManager->DrawNoRotation(&vm[1]);
            vm[2].pos =
                player->bombInfo.subInfo[i].bombRegionPositions + vm[2].offset;
            vm[2].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            vm[2].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            vm[2].pos.z = 0.0f;
            g_AnmManager->DrawNoRotation(&vm[2]);
            vm[3].pos =
                player->bombInfo.subInfo[i].bombRegionPositions + vm[3].offset;
            vm[3].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            vm[3].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            vm[3].pos.z = 0.0f;
            g_AnmManager->DrawNoRotation(&vm[3]);
        }
    }
}

// FUNCTION: TH07 0x00409dd0
void BombData::BombReimuBCalc(Player *player)
{
    BombProjectile *local_28[7];
    AnmVm *local_c;
    i32 local_8;

    if (player->bombInfo.bombTimer.current < player->bombInfo.bombDuration)
    {
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 0))
        {
            if (player->bombInfo.bombTimer.current == 60)
            {
                BombEffects::RegisterChain(1, 0x50, 20, 0, 0);
            }
            local_28[0] = player->SpawnBombProjectile(&player->positionCenter, 62.0f,
                                                      448.0f, 6);
            local_28[1] = player->SpawnBombProjectile(&player->positionCenter, 384.0f,
                                                      62.0f, 6);
            local_28[2] = player->SpawnBombProjectile(&player->positionCenter, 62.0f,
                                                      448.0f, 6);
            local_28[3] = player->SpawnBombProjectile(&player->positionCenter, 384.0f,
                                                      62.0f, 6);
            for (local_8 = 0; local_8 < 4; local_8 += 1)
            {
                g_AnmManager->ExecuteScript(player->bombInfo.subInfo[local_8].vms);
                if (player->bombInfo.bombTimer.current !=
                    player->bombInfo.bombTimer.previous)
                {
                    if (player->bombInfo.bombTimer.current % 2 != 0)
                    {
                        (local_28[local_8]->pos).x =
                            player->bombInfo.subInfo[local_8].bombRegionPositions.x +
                            player->bombInfo.subInfo[local_8].vms[0].offset.x;
                        (local_28[local_8]->pos).y =
                            player->bombInfo.subInfo[local_8].bombRegionPositions.y +
                            player->bombInfo.subInfo[local_8].vms[0].offset.y;
                        player->bombProjectiles[local_8].size.x =
                            (local_28[local_8]->pos).z;
                        player->bombProjectiles[local_8].size.y =
                            (local_28[local_8]->size).x;
                        player->bombProjectiles[local_8].pos =
                            player->bombInfo.subInfo[local_8].bombRegionPositions +
                            player->bombInfo.subInfo[local_8].vms->offset;
                        player->bombProjectiles[local_8].lifetime = 0x10;
                    }
                }
            }
        }
        else
        {
            g_ItemManager.RemoveAllItems();
            g_Gui.ShowBombNamePortrait(0x4a1, "夢符「二重結界」");
            player->bombInfo.bombDuration = 0x8c;
            player->invulnerabilityTimer = 200;
            SpawnBombInvulnEffect(player);
            for (local_8 = 0; local_8 < 4; local_8 += 1)
            {
                local_c = player->bombInfo.subInfo[local_8].vms;
                g_AnmManager->ExecuteAnmIdx(local_c, local_8 + 0x489);
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_REIMARI, 0);
            player->bombInfo.subInfo[0].bombRegionPositions.x =
                player->positionCenter.x;
            player->bombInfo.subInfo[0].bombRegionPositions.y = 224.0f;
            player->bombInfo.subInfo[0].bombRegionPositions.z = 0.42f;
            player->bombInfo.subInfo[1].bombRegionPositions.x = 192.0f;
            player->bombInfo.subInfo[1].bombRegionPositions.y =
                player->positionCenter.y;
            player->bombInfo.subInfo[1].bombRegionPositions.z = 0.415f;
            player->bombInfo.subInfo[2].bombRegionPositions.x =
                player->positionCenter.x;
            player->bombInfo.subInfo[2].bombRegionPositions.y = 224.0f;
            player->bombInfo.subInfo[2].bombRegionPositions.z = 0.41f;
            player->bombInfo.subInfo[3].bombRegionPositions.x = 192.0f;
            player->bombInfo.subInfo[3].bombRegionPositions.y =
                player->positionCenter.y;
            player->bombInfo.subInfo[3].bombRegionPositions.z = 0.405f;
            BombEffects::RegisterChain(1, 60, 2, 6, 0);
            ComputeBombCherryDrain(player, 3000, 0.17f);
        }
        player->playerState = PLAYER_STATE_INVULNERABLE;
        player->bombInfo.bombTimer.Tick();
    }
    else
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
    }
}

#pragma var_order(vm, i)
// FUNCTION: TH07 0x0040a280
void BombData::BombReimuBDraw(Player *player)
{
    AnmVm *vm;
    i32 i;

    DarkenViewport(player);
    for (i = 0; i < 4; i++)
    {
        vm = player->bombInfo.subInfo[i].vms;
        vm->pos = player->bombInfo.subInfo[i].bombRegionPositions + vm->offset;
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.0f;
        g_AnmManager->Draw(vm);
    }
}

// FUNCTION: TH07 0x0040a3a0
void BombData::BombReimuBCalcFocus(Player *player)
{
    AnmVm *local_c;
    i32 local_8;

    if (player->bombInfo.bombTimer.current < player->bombInfo.bombDuration)
    {
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 0))
        {
            if (player->bombInfo.bombTimer.current == 60)
            {
                BombEffects::RegisterChain(1, 0x50, 20, 0, 0);
            }
            g_AnmManager->ExecuteScript(player->bombInfo.subInfo[0].vms);
            g_AnmManager->ExecuteScript(player->bombInfo.subInfo[0].vms + 1);
            g_AnmManager->ExecuteScript(player->bombInfo.subInfo[0].vms + 2);
            player->bombProjectiles[0].size.x = 256.0f;
            player->bombProjectiles[0].size.y = 256.0f;
            player->bombProjectiles[0].pos.x =
                (player->bombStartPos).x +
                player->bombInfo.subInfo[0].vms[0].offset.x;
            player->bombProjectiles[0].pos.y =
                (player->bombStartPos).y +
                player->bombInfo.subInfo[0].vms[0].offset.y;
            player->bombProjectiles[0].pos.z =
                (player->bombStartPos).z +
                player->bombInfo.subInfo[0].vms[0].offset.z;
            player->bombProjectiles[0].lifetime = 0x12;
        }
        else
        {
            g_ItemManager.RemoveAllItems();
            g_Gui.ShowBombNamePortrait(0x4a1, "夢符「二重結界」");
            player->bombInfo.bombDuration = 0xbe;
            player->invulnerabilityTimer = 250;
            SpawnBombInvulnEffect(player);
            local_c = player->bombInfo.subInfo[0].vms;
            for (local_8 = 0; local_8 < 3; local_8 += 1)
            {
                g_AnmManager->ExecuteAnmIdx(local_c, local_8 + 0x48d);
                local_c = local_c + 1;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_REIMARI, 0);
            BombEffects::RegisterChain(1, 60, 2, 6, 0);
            player->bombStartPos = player->positionCenter;
            ComputeBombCherryDrain(player, 3000, 0.17f);
            player->verticalMovementSpeedMultiplierDuringBomb = 0.4f;
            player->horizontalMovementSpeedMultiplierDuringBomb = 0.4f;
            player->SpawnBombEffect(&player->positionCenter, 192.0f, 0.384f, 0xd2,
                                    ITEM_POINT_BULLET);
        }
        player->playerState = PLAYER_STATE_INVULNERABLE;
        player->bombInfo.bombTimer.Tick();
    }
    else
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
    }
}

#pragma var_order(vm, i)
// FUNCTION: TH07 0x0040a6b0
void BombData::BombReimuBDrawFocus(Player *player)
{
    AnmVm *vm;
    i32 i;

    DarkenViewport(player);
    for (i = 0; i < 3; i++)
    {
        vm = &player->bombInfo.subInfo[0].vms[i];
        vm->pos = player->bombStartPos + vm->offset;
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.0f;
        g_AnmManager->Draw(vm);
    }
}

// FUNCTION: TH07 0x0040a7c0
void BombData::BombMarisaACalc(Player *player)
{
    i32 i;

    if (player->bombInfo.bombTimer.current < player->bombInfo.bombDuration)
    {
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 0))
        {
            for (i = 0; i < 8; i++)
            {
                player->bombInfo.subInfo[i].bombRegionPositions +=
                    g_Supervisor.effectiveFramerateMultiplier *
                    player->bombInfo.subInfo[i].bombRegionVelocities;
                if ((player->bombInfo.bombTimer.current !=
                     player->bombInfo.bombTimer.previous) &&
                    (player->bombInfo.bombTimer.current % 3 != 0))
                {
                    player->SpawnBombEffect(
                        &player->bombInfo.subInfo[i].bombRegionPositions, 96.0f, 0.0f, 0,
                        ITEM_POINT_BULLET);
                    player->bombProjectiles[i].size.x = 128.0f;
                    player->bombProjectiles[i].size.y = 128.0f;
                    player->bombProjectiles[i].pos =
                        player->bombInfo.subInfo[i].bombRegionPositions;
                    player->bombProjectiles[i].lifetime = 8;
                }
                g_AnmManager->ExecuteScript(player->bombInfo.subInfo[i].vms);
            }
        }
        else
        {
            g_ItemManager.RemoveAllItems();
            g_Gui.ShowBombNamePortrait(0x4a3, "魔符「スターダストレヴァリエ」");
            player->bombInfo.bombDuration = 200;
            player->invulnerabilityTimer = 250;
            SpawnBombInvulnEffect(player);
            for (i = 0; i < 8; i++)
            {
                g_AnmManager->ExecuteAnmIdx(player->bombInfo.subInfo[i].vms,
                                            i % 3 + 0x405);
                player->bombInfo.subInfo[i].bombRegionPositions =
                    player->positionCenter;
                AngleToVector(&player->bombInfo.subInfo[i].bombRegionVelocities,
                              ((f32)i * ZUN_2PI) / 8.0f, 2.0f);
                player->bombInfo.subInfo[i].bombRegionVelocities.z = 0.0f;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_REIMARI, 0);
            BombEffects::RegisterChain(1, 0x78, 4, 1, 0);
            ComputeBombCherryDrain(player, 8000, 0.3f);
        }
        player->playerState = PLAYER_STATE_INVULNERABLE;
        player->bombInfo.bombTimer.Tick();
    }
    else
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
    }
}

// FUNCTION: TH07 0x0040aba0
void BombData::BombMarisaADraw(Player *player)
{
    AnmVm *vm;
    i32 i;

    DarkenViewport(player);
    for (i = 0; i < 8; i++)
    {
        vm = player->bombInfo.subInfo[i].vms;
        vm->pos = player->bombInfo.subInfo[i].bombRegionPositions;
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.0f;
        vm->scale.x = 3.2f;
        vm->scale.y = 3.2f;
        g_AnmManager->Draw(vm);
        vm->pos -= player->bombInfo.subInfo[i].bombRegionVelocities * 6.0f;
        vm->pos.x -= 32.0f;
        vm->pos.y -= 32.0f;
        vm->pos.z = 0.0f;
        vm->scale.x = 2.2f;
        vm->scale.y = 2.2f;
        g_AnmManager->Draw(vm);
        vm->pos -= (player->bombInfo.subInfo[i].bombRegionVelocities +
                    player->bombInfo.subInfo[i].bombRegionVelocities);
        vm->pos.x += 64.0f;
        vm->pos.y += 64.0f;
        vm->pos.z = 0.0f;
        vm->pos -= (player->bombInfo.subInfo[i].bombRegionVelocities +
                    player->bombInfo.subInfo[i].bombRegionVelocities);
        vm->pos.x -= 32.0f;
        vm->pos.y -= 32.0f;
        vm->pos.z = 0.0f;
        vm->scale.x = 1.0f;
        vm->scale.y = 1.0f;
        g_AnmManager->Draw(vm);
    }
}

// FUNCTION: TH07 0x0040af10
void BombData::BombMarisaACalcFocus(Player *player)
{
    i32 iVar6;
    i32 j;
    PlayerBombSubInfo *subInfo;
    i32 i;

    if (player->bombInfo.bombTimer.current < player->bombInfo.bombDuration)
    {
        if ((player->bombInfo.bombTimer.current !=
             player->bombInfo.bombTimer.previous) &&
            (player->bombInfo.bombTimer.current == 0))
        {
            g_ItemManager.RemoveAllItems();
            g_Gui.ShowBombNamePortrait(0x4a2, "魔符「ミルキーウェイ」");
            player->bombInfo.bombDuration = 260;
            player->invulnerabilityTimer = 310;
            SpawnBombInvulnEffect(player);
            for (i = 0; i < 0x18; i++)
            {
                player->bombInfo.subInfo[i].state = 0;
            }
            ComputeBombCherryDrain(player, 9000, 0.33f);
            player->verticalMovementSpeedMultiplierDuringBomb = 0.4f;
            player->horizontalMovementSpeedMultiplierDuringBomb = 0.4f;
        }
        if (((player->bombInfo.bombTimer.current !=
              player->bombInfo.bombTimer.previous) &&
             (player->bombInfo.bombTimer.current % 6 == 0)) &&
            (iVar6 = player->bombInfo.bombTimer.current / 6, iVar6 < 0x18))
        {
            g_AnmManager->ExecuteAnmIdx(player->bombInfo.subInfo[iVar6].vms,
                                        iVar6 % 3 + 0x405);

            player->bombInfo.subInfo[iVar6].bombRegionPositions =
                player->positionCenter;
            for (j = 0; j < 8; j += 1)
            {
                player->bombInfo.subInfo[iVar6].bombRegionPositionsTrails[j] =
                    player->positionCenter;
            }
            player->bombInfo.subInfo[iVar6].state = 1;
            AngleToVector(&player->bombInfo.subInfo[iVar6].bombRegionVelocities,
                          (g_Rng.GetRandomFloatInRange(0.3926991f) - 0.19634955f) -
                              1.5707964f,
                          -5.0f);
            player->bombInfo.subInfo[iVar6].bombRegionVelocities.z = 0.0f;
            AngleToVector(&player->bombInfo.subInfo[iVar6].bombRegionAcceleration,
                          (g_Rng.GetRandomFloatInRange(0.3926991f) - 0.19634955f) -
                              1.5707964f,
                          0.24f);
            player->bombInfo.subInfo[iVar6].bombRegionAcceleration.z = 0.0f;
            g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_MARISA_A_FOCUS, 0);
            BombEffects::RegisterChain(1, 0x78, 4, 1, 0);
            player->bombProjectiles[iVar6].payload = 0;
        }
        subInfo = player->bombInfo.subInfo;
        for (i = 0; i < 0x18; i++)
        {
            if (subInfo->state != 0)
            {
                for (j = 7; 0 < j; j += -1)
                {
                    subInfo->bombRegionPositionsTrails[j] =
                        subInfo->bombRegionPositionsTrails[j - 1];
                }
                subInfo->bombRegionPositionsTrails[0] = subInfo->bombRegionPositions;
                subInfo->bombRegionPositions +=
                    g_Supervisor.effectiveFramerateMultiplier *
                    subInfo->bombRegionVelocities;
                subInfo->bombRegionVelocities +=
                    g_Supervisor.effectiveFramerateMultiplier *
                    subInfo->bombRegionAcceleration;
                if ((subInfo->bombRegionPositions).y < -256.0f)
                {
                    subInfo->state = 0;
                }
                player->SpawnBombEffect(&subInfo->bombRegionPositions, 96.0f, 0.0f, 0,
                                        ITEM_POINT_BULLET);
                if (player->bombProjectiles[i].payload < 80)
                {
                    player->bombProjectiles[i].size.x = 128.0f;
                    player->bombProjectiles[i].size.y = 128.0f;
                    player->bombProjectiles[i].pos = subInfo->bombRegionPositions;
                    player->bombProjectiles[i].lifetime = 0xc;
                }
                g_AnmManager->ExecuteScript(subInfo->vms);
            }
            subInfo = subInfo + 1;
        }
        player->playerState = PLAYER_STATE_INVULNERABLE;
        player->bombInfo.bombTimer.Tick();
    }
    else
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
    }
}

// FUNCTION: TH07 0x0040b5d0
void BombData::BombMarisaADrawFocus(Player *player)
{
    AnmVm *vm;
    PlayerBombSubInfo *subInfo;
    i32 i;

    DarkenViewport(player);
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 0x18; i++)
    {
        if (player->bombInfo.subInfo[i].state != 0)
        {
            vm = subInfo->vms;
            subInfo->vms[0].pos = subInfo->bombRegionPositions;
            subInfo->vms[0].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            subInfo->vms[0].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            subInfo->vms[0].pos.z = 0.3f;
            subInfo->vms[0].scale.x = 3.2f;
            subInfo->vms[0].scale.y = 3.2f;
            g_AnmManager->Draw(vm);
            subInfo->vms[0].pos = subInfo->bombRegionPositionsTrails[3];
            subInfo->vms[0].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            subInfo->vms[0].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            subInfo->vms[0].pos.z = 0.32f;
            subInfo->vms[0].scale.x = 2.2f;
            subInfo->vms[0].scale.y = 2.2f;
            g_AnmManager->Draw(vm);
            subInfo->vms[0].pos = subInfo->bombRegionPositionsTrails[7];
            subInfo->vms[0].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            subInfo->vms[0].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            subInfo->vms[0].pos.z = 0.34f;
            subInfo->vms[0].scale.x = 1.3f;
            subInfo->vms[0].scale.y = 1.3f;
            g_AnmManager->Draw(vm);
        }
        subInfo = subInfo + 1;
    }
}

// FUNCTION: TH07 0x0040b7d0
void BombData::BombMarisaBCalc(Player *player)
{
    i32 local_24;
    BombProjectile *local_14;
    f32 local_10;
    PlayerBombSubInfo *local_c;
    i32 local_8;

    if (player->bombInfo.bombTimer.current < player->bombInfo.bombDuration)
    {
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 0))
        {
            local_c = player->bombInfo.subInfo;
            local_14 = player->bombProjectiles;
            for (local_8 = 0; local_8 < 3; local_8 += 1)
            {
                if (192.0f <= (player->bombStartPos).x)
                {
                    local_c->accel = utils::AddNormalizeAngle(
                        local_c->accel, ((((f32)player->bombInfo.bombTimer.current +
                                           player->bombInfo.bombTimer.subFrame) *
                                          -ZUN_PI) /
                                         30.0f) /
                                            (f32)player->bombInfo.bombDuration);
                }
                else
                {
                    local_c->accel = utils::AddNormalizeAngle(
                        local_c->accel, ((((f32)player->bombInfo.bombTimer.current +
                                           player->bombInfo.bombTimer.subFrame) *
                                          ZUN_PI) /
                                         30.0f) /
                                            (f32)player->bombInfo.bombDuration);
                }
                local_c->vms[0].pos = player->positionCenter;
                local_c->vms[0].pos.x +=
                    (cosf(local_c->accel) * (local_c->vms[0].sprite)->heightPx *
                     local_c->vms[0].scale.y) /
                    2.0f;
                local_c->vms[0].pos.y +=
                    (sinf(local_c->accel) * (local_c->vms[0].sprite)->heightPx *
                     local_c->vms[0].scale.y) /
                    2.0f;
                local_10 = 32.0f;
                for (local_24 = 0; local_24 < 6; local_24 += 1)
                {
                    local_14->pos = player->positionCenter;
                    local_14->pos.x += cosf(local_c->accel) * local_10;
                    local_14->pos.y += sinf(local_c->accel) * local_10;
                    local_14->size.x = 128.0f;
                    local_14->size.y = 128.0f;
                    local_14->lifetime = 10;
                    player->SpawnBombEffect(&local_14->pos, 64.0f, 0.0f, 0,
                                            ITEM_POINT_BULLET);
                    local_10 =
                        ((local_c->vms[0].sprite)->heightPx * local_c->vms[0].scale.y) /
                            5.0f +
                        local_10;
                    local_14 = local_14 + 1;
                }
                g_AnmManager->ExecuteScript(local_c->vms);
                local_c = local_c + 1;
            }
            if (player->bombInfo.bombTimer.current == 20)
            {
                BombEffects::RegisterChain(1, 60, 1, 7, 0);
            }
            else if (player->bombInfo.bombTimer.current == 0x50)
            {
                BombEffects::RegisterChain(1, 100, 0x18, 0, 0);
            }
        }
        else
        {
            g_ItemManager.RemoveAllItems();
            player->bombStartPos = player->positionCenter;
            g_Gui.ShowBombNamePortrait(0x4a1, "恋符「ノンディレクショナルレーザー」");
            player->bombInfo.bombDuration = 300;
            player->invulnerabilityTimer = 300;
            SpawnBombInvulnEffect(player);
            local_c = player->bombInfo.subInfo;
            for (local_8 = 0; local_8 < 3; local_8 += 1)
            {
                g_AnmManager->ExecuteAnmIdx(local_c->vms, local_8 + 0x40c);
                local_c->bombRegionPositions = player->positionCenter;
                local_c->accel = ((f32)local_8 * ZUN_2PI) / 3.0f - 1.5707964f;
                local_c = local_c + 1;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_SAKUMARI, 0);
            player->verticalMovementSpeedMultiplierDuringBomb = 0.4f;
            player->horizontalMovementSpeedMultiplierDuringBomb = 0.4f;
            ComputeBombCherryDrain(player, 8000, 0.35f);
        }
        player->playerState = PLAYER_STATE_INVULNERABLE;
        player->bombInfo.bombTimer.Tick();
    }
    else
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
    }
}

// FUNCTION: TH07 0x0040bca0
void BombData::BombMarisaBDraw(Player *player)
{
    AnmVm *vm;
    i32 i;

    DarkenViewport(player);
    for (i = 0; i < 3; i++)
    {
        vm = player->bombInfo.subInfo[i].vms;
        vm->pos = player->positionCenter;
        vm->pos.x += (cosf(player->bombInfo.subInfo[i].accel) *
                      vm->sprite->heightPx * vm->scale.y) /
                     2.0f;
        vm->pos.y += (sinf(player->bombInfo.subInfo[i].accel) *
                      vm->sprite->heightPx * vm->scale.y) /
                     2.0f;
        vm->rotation.z =
            utils::AddNormalizeAngle(player->bombInfo.subInfo[i].accel, 1.5707964f);
        vm->updateRotation = 1;
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.0f;
        g_AnmManager->Draw(vm);
    }
}

// FUNCTION: TH07 0x0040be20
void BombData::BombMarisaBCalcFocus(Player *player)
{
    AnmVm *local_c;
    i32 local_8;

    if (player->bombInfo.bombTimer.current < player->bombInfo.bombDuration)
    {
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 0))
        {
            if (player->bombInfo.bombTimer.current == 60)
            {
                BombEffects::RegisterChain(1, 60, 1, 7, 0);
            }
            else if (player->bombInfo.bombTimer.current == 0x78)
            {
                BombEffects::RegisterChain(1, 200, 0x18, 0, 0);
            }
            if (player->bombInfo.bombTimer.current !=
                player->bombInfo.bombTimer.previous)
            {
                if (player->bombInfo.bombTimer.current % 4 != 0)
                {
                    player->bombProjectiles[0].size.x = 384.0f;
                    player->bombProjectiles[0].size.y = player->positionCenter.y;
                    player->bombProjectiles[0].pos.x = 192.0f;
                    player->bombProjectiles[0].pos.y = player->positionCenter.y / 2.0f;
                    player->bombProjectiles[0].lifetime = 0x17;
                    player->SpawnBombProjectile(&player->bombProjectiles[0].pos, 384.0f,
                                                player->positionCenter.y, 6);
                }
            }
            g_AnmManager->ExecuteScript(player->bombInfo.subInfo[0].vms);
            g_AnmManager->ExecuteScript(player->bombInfo.subInfo[0].vms + 1);
            g_AnmManager->ExecuteScript(player->bombInfo.subInfo[0].vms + 2);
            g_AnmManager->ExecuteScript(player->bombInfo.subInfo[0].vms + 3);
        }
        else
        {
            g_ItemManager.RemoveAllItems();
            g_Gui.ShowBombNamePortrait(0x4a2, "恋符「マスタースパーク」");
            player->bombInfo.bombDuration = 0x154;
            player->invulnerabilityTimer = 390;
            SpawnBombInvulnEffect(player);
            local_c = player->bombInfo.subInfo[0].vms;
            for (local_8 = 0; local_8 < 4; local_8 += 1)
            {
                g_AnmManager->ExecuteAnmIdx(local_c, local_8 + 0x408);
                player->bombInfo.subInfo[local_8].bombRegionPositions =
                    player->positionCenter;
                local_c = local_c + 1;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_SAKUMARI, 0);
            player->verticalMovementSpeedMultiplierDuringBomb = 0.2f;
            player->horizontalMovementSpeedMultiplierDuringBomb = 0.2f;
            ComputeBombCherryDrain(player, 10000, 0.41f);
        }
        player->playerState = PLAYER_STATE_INVULNERABLE;
        player->bombInfo.bombTimer.Tick();
    }
    else
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
    }
}

// FUNCTION: TH07 0x0040c160
void BombData::BombMarisaBDrawFocus(Player *player)
{
    f32 fVar1;
    i32 i;
    AnmVm *local_8;

    DarkenViewport(player);
    local_8 = player->bombInfo.subInfo[0].vms;
    for (i = 0; i < 4; i++)
    {
        fVar1 = (((f32)i * 0.62831855f) / 3.0f - ZUN_PI) + 1.2566371f;
        local_8->pos = player->positionCenter;
        local_8->pos.x +=
            (cosf(fVar1) * local_8->sprite->heightPx * local_8->scale.y) / 2.0f;
        local_8->pos.y +=
            (sinf(fVar1) * local_8->sprite->heightPx * local_8->scale.y) / 2.0f;
        local_8->rotation.z = utils::AddNormalizeAngle(fVar1, 1.5707964f);
        local_8->updateRotation = 1;
        local_8->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        local_8->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        local_8->pos.z = 0.0f;
        g_AnmManager->Draw(local_8);
        local_8 = local_8 + 1;
    }
}

// FUNCTION: TH07 0x0040c2e0
void BombData::BombSakuyaACalc(Player *player)
{
    bool bVar5;
    i32 local_24;
    PlayerBombSubInfo *local_c;
    u32 local_8;

    if (player->bombInfo.bombTimer.current < player->bombInfo.bombDuration)
    {
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 0))
        {
            bVar5 = false;
        }
        else
        {
            bVar5 = true;
        }
        if (bVar5)
        {
            g_ItemManager.RemoveAllItems();
            g_Gui.ShowBombNamePortrait(0x4a1, "幻符「インディスクリミネイト」");
            player->bombInfo.bombDuration = 0xa0;
            player->invulnerabilityTimer = 210;
            SpawnBombInvulnEffect(player);
            player->bombStartPos = player->positionCenter;
            local_c = player->bombInfo.subInfo;
            for (local_8 = 0; local_8 < 0x60; local_8 += 1)
            {
                local_c->state = 0;
                local_c = local_c + 1;
            }
            ComputeBombCherryDrain(player, 6000, 0.28f);
            player->bombInfo.subInfo[0].effect = g_EffectManager.SpawnParticles(
                21, &player->positionCenter, 1, 0xffffffff);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_SAKUYA_A, 0);
        }
        if (player->bombInfo.bombTimer.current > 60)
        {
            local_24 = 5;
            local_c = player->bombInfo.subInfo;
            for (local_8 = 0; local_8 < 0x60; local_8 += 1)
            {
                if (local_c->state == 0)
                {
                    if ((player->bombInfo.bombTimer.current < 0x79) && (local_24 != 0))
                    {
                        local_c->state = 1;
                        g_AnmManager->ExecuteAnmIdx(local_c->vms, (local_8 & 1) + 0x405);
                        local_c->angle = g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
                        local_c->speed = g_Rng.GetRandomFloatInRange(6.0f) + 5.5f;
                        local_c->accel = g_Rng.GetRandomFloatInRange(0.1f) + 0.1f;
                        (local_c->bombRegionAcceleration).x =
                            g_Rng.GetRandomFloatInRange(0.06283186f) - 0.03141593f;
                        (local_c->bombRegionVelocities).x = cosf(local_c->angle) * 24.0f;
                        (local_c->bombRegionVelocities).y = sinf(local_c->angle) * 24.0f;
                        local_c->bombRegionPositions =
                            player->bombStartPos + local_c->bombRegionVelocities;
                        (local_c->bombRegionVelocities).z = 0.0f;
                        player->bombProjectiles[local_8].payload = 0;
                        local_24 += -1;
                    }
                }
                else
                {
                    local_c->angle = utils::AddNormalizeAngle(
                        local_c->angle, (local_c->bombRegionAcceleration).x);
                    local_c->speed = local_c->speed + local_c->accel;
                    (local_c->bombRegionVelocities).x =
                        cosf(local_c->angle) * local_c->speed;
                    (local_c->bombRegionVelocities).y =
                        sinf(local_c->angle) * local_c->speed;
                    if (player->bombProjectiles[local_8].payload < 0x1e)
                    {
                        local_c->bombRegionPositions +=
                            g_Supervisor.effectiveFramerateMultiplier *
                            local_c->bombRegionVelocities;
                        player->SpawnBombEffect(&local_c->bombRegionPositions, 32.0f, 0.0f,
                                                0, ITEM_POINT_BULLET);
                        player->bombProjectiles[local_8].size.x = 24.0f;
                        player->bombProjectiles[local_8].size.y = 24.0f;
                        player->bombProjectiles[local_8].pos = local_c->bombRegionPositions;
                        player->bombProjectiles[local_8].lifetime = 10;
                    }
                    else if (player->bombProjectiles[local_8].payload < 999)
                    {
                        g_AnmManager->ExecuteAnmIdx(local_c->vms, 0x460);
                        player->bombProjectiles[local_8].payload = 999;
                    }
                    if (g_GameManager.IsInBounds((local_c->bombRegionPositions).x,
                                                 (local_c->bombRegionPositions).y, 64.0f,
                                                 64.0f) == 0)
                    {
                        local_c->state = 0;
                    }
                    g_AnmManager->ExecuteScript(local_c->vms);
                }
                local_c = local_c + 1;
            }
        }
        player->playerState = PLAYER_STATE_INVULNERABLE;
        player->bombInfo.bombTimer.Tick();
    }
    else
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
    }
}

// FUNCTION: TH07 0x0040c970
void BombData::BombSakuyaADraw(Player *player)
{
    PlayerBombSubInfo *subInfo;
    i32 i;

    DarkenViewport(player);
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 0x60; i++)
    {
        if (subInfo->state != 0)
        {
            subInfo->vms[0].rotation.z =
                utils::AddNormalizeAngle(subInfo->angle, 1.5707964f);
            subInfo->vms[0].updateRotation = 1;
            subInfo->vms[0].pos = subInfo->bombRegionPositions;
            subInfo->vms[0].pos.z = 0.0f;
            g_AnmManager->Draw(subInfo->vms);
        }
        subInfo = subInfo + 1;
    }
}

// FUNCTION: TH07 0x0040ca50
void BombData::BombSakuyaACalcFocus(Player *player)
{
    bool bVar4;
    f32 fVar8;
    PlayerBombSubInfo *local_c;
    i32 local_8;

    if (player->bombInfo.bombTimer.current < player->bombInfo.bombDuration)
    {
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 0))
        {
            bVar4 = false;
        }
        else
        {
            bVar4 = true;
        }
        if (bVar4)
        {
            g_ItemManager.RemoveAllItems();
            g_Gui.ShowBombNamePortrait(0x4a1, "幻符「殺人ドール」");
            player->bombInfo.bombDuration = 0xfa;
            player->invulnerabilityTimer = 290;
            SpawnBombInvulnEffect(player);
            local_c = player->bombInfo.subInfo;
            for (local_8 = 0; local_8 < 0x60; local_8 += 1)
            {
                local_c->state = 0;
                local_c = local_c + 1;
            }
            ComputeBombCherryDrain(player, 0x1964, 0.29f);
            player->verticalMovementSpeedMultiplierDuringBomb = 0.3f;
            player->horizontalMovementSpeedMultiplierDuringBomb = 0.3f;
            player->bombInfo.subInfo[0].effect = g_EffectManager.SpawnParticles(
                21, &player->positionCenter, 1, 0xffffffff);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_SAKUYA_A, 0);
        }
        if ((-1 < player->bombInfo.bombTimer.current) &&
            (player->bombInfo.bombTimer.current < 0x3d))
        {
            player->bombInfo.subInfo[0].effect->pos1 = player->positionCenter;
        }
        if ((0x13 < player->bombInfo.bombTimer.current) &&
            (player->bombInfo.bombTimer.current < 0x74))
        {
            local_c = player->bombInfo.subInfo;
            for (local_8 = 0; local_8 < 0x60; local_8 += 1)
            {
                if ((player->bombInfo.bombTimer.current ==
                     player->bombInfo.bombTimer.previous) ||
                    (player->bombInfo.bombTimer.current !=
                     (local_8 % 0x30) * 2 + 20))
                {
                    bVar4 = false;
                }
                else
                {
                    bVar4 = true;
                }
                if (bVar4)
                {
                    local_c->state = 1;
                    g_AnmManager->ExecuteAnmIdx(local_c->vms, (local_8 & 1) + 0x407);
                    local_c->angle = ((f32)local_8 * ZUN_2PI) / 96.0f - ZUN_PI;
                    local_c->speed = g_Rng.GetRandomFloatInRange(1.0f) + 0.5f;
                    local_c->accel = g_Rng.GetRandomFloatInRange(0.1f) + 0.03f;
                    g_Rng.GetRandomU16(); // What
                    (local_c->bombRegionAcceleration).x = -0.15707964f;
                    (local_c->bombRegionVelocities).x = cosf(local_c->angle) * 24.0f;
                    (local_c->bombRegionVelocities).y = sinf(local_c->angle) * 24.0f;
                    local_c->bombRegionPositions =
                        player->positionCenter + local_c->bombRegionVelocities;
                    (local_c->timer) = 0;
                    (local_c->bombRegionVelocities).z = 0.0f;
                    player->bombProjectiles[local_8].payload = 0;
                }
                local_c = local_c + 1;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_REIMARI, 0);
            BombEffects::RegisterChain(1, 0x78, 4, 1, 0);
        }
        local_c = player->bombInfo.subInfo;
        for (local_8 = 0; local_8 < 0x60; local_8 += 1)
        {
            if (local_c->state != 0)
            {
                if (((local_c->timer).current < 0x1e) ||
                    (0x45 < (local_c->timer).current))
                {
                    if (((local_c->timer).current == (local_c->timer).previous) ||
                        ((local_c->timer).current != 0x46))
                    {
                        bVar4 = false;
                    }
                    else
                    {
                        bVar4 = true;
                    }
                    if (bVar4)
                    {
                        if (-100.0f < (player->positionOfLastEnemyHit).x)
                        {
                            fVar8 = 0.0f;
                            local_c->angle = utils::AddNormalizeAngle(
                                atan2f(((player->positionOfLastEnemyHit).y -
                                        (local_c->bombRegionPositions).y),
                                       ((player->positionOfLastEnemyHit).x -
                                        (local_c->bombRegionPositions).x)),
                                fVar8);
                        }
                        local_c->speed = 14.0f;
                    }
                    local_c->speed = local_c->speed + local_c->accel;
                    (local_c->bombRegionVelocities).x =
                        cosf(local_c->angle) * local_c->speed;
                    (local_c->bombRegionVelocities).y =
                        sinf(local_c->angle) * local_c->speed;
                }
                else
                {
                    local_c->angle = utils::AddNormalizeAngle(
                        local_c->angle, (local_c->bombRegionAcceleration).x);
                    (local_c->bombRegionVelocities).x = 0.0f;
                    (local_c->bombRegionVelocities).y = 0.0f;
                }
                if (player->bombProjectiles[local_8].payload == 0)
                {
                    local_c->bombRegionPositions +=
                        g_Supervisor.effectiveFramerateMultiplier *
                        local_c->bombRegionVelocities;
                    player->SpawnBombEffect(
                        &player->bombInfo.subInfo[local_8].bombRegionPositions, 32.0f,
                        0.0f, 0, ITEM_POINT_BULLET);
                    player->bombProjectiles[local_8].size.x = 24.0f;
                    player->bombProjectiles[local_8].size.y = 24.0f;
                    player->bombProjectiles[local_8].pos = local_c->bombRegionPositions;
                    player->bombProjectiles[local_8].lifetime = 0x16;
                }
                else if (player->bombProjectiles[local_8].payload < 999)
                {
                    g_AnmManager->ExecuteAnmIdx(local_c->vms, 0x460);
                    player->bombProjectiles[local_8].payload = 999;
                    g_EffectManager.SpawnParticles(
                        0, &player->bombInfo.subInfo[local_8].bombRegionPositions, 1,
                        0xffff80ff);
                }
                g_AnmManager->ExecuteScript(local_c->vms);
                local_c->timer.Tick();
            }
            local_c = local_c + 1;
        }
        player->playerState = PLAYER_STATE_INVULNERABLE;
        player->bombInfo.bombTimer.Tick();
    }
    else
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
    }
}

// FUNCTION: TH07 0x0040d3b0
void BombData::BombSakuyaADrawFocus(Player *player)
{
    PlayerBombSubInfo *subInfo;
    i32 i;

    DarkenViewport(player);
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 0x60; i++)
    {
        if (subInfo->state != 0)
        {
            subInfo->vms[0].rotation.z =
                utils::AddNormalizeAngle(subInfo->angle, 1.5707964f);
            subInfo->vms[0].updateRotation = 1;
            subInfo->vms[0].pos = subInfo->bombRegionPositions;
            subInfo->vms[0].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            subInfo->vms[0].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            subInfo->vms[0].pos.z = 0.0f;
            g_AnmManager->Draw(subInfo->vms);
        }
        subInfo = subInfo + 1;
    }
}

// FUNCTION: TH07 0x0040d4c0
void BombData::BombSakuyaBCalc(Player *player)
{
    bool bVar1;
    f32 local_74;
    f32 local_70;
    PlayerBombSubInfo *local_c;
    u32 local_8;

    if (player->bombInfo.bombTimer.current < player->bombInfo.bombDuration)
    {
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 0))
        {
            bVar1 = false;
        }
        else
        {
            bVar1 = true;
        }
        if (bVar1)
        {
            g_ItemManager.RemoveAllItems();
            g_Gui.ShowBombNamePortrait(0x4a3, "時符「パーフェクトスクウェア」");
            player->bombInfo.bombDuration = 0xa0;
            player->invulnerabilityTimer = 260;
            SpawnBombInvulnEffect(player);
            local_c = player->bombInfo.subInfo;
            for (local_8 = 0; local_8 < 4; local_8 += 1)
            {
                local_c->state = 0;
                local_c = local_c + 1;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_SAKUMARI, 0);
            ComputeBombCherryDrain(player, 0x157c, 0.26f);
            player->verticalMovementSpeedMultiplierDuringBomb = 2.0f;
            player->horizontalMovementSpeedMultiplierDuringBomb = 2.0f;
            g_BulletManager.StopBulletMovement();
        }
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 60))
        {
            bVar1 = false;
        }
        else
        {
            bVar1 = true;
        }
        if (bVar1)
        {
            g_BulletManager.StopBulletMovement();
        }
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 0x78))
        {
            bVar1 = false;
        }
        else
        {
            bVar1 = true;
        }
        if (bVar1)
        {
            g_BulletManager.StopBulletMovement();
        }
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 0x1e))
        {
            bVar1 = false;
        }
        else
        {
            bVar1 = true;
        }
        if (bVar1)
        {
            local_c = player->bombInfo.subInfo;
            for (local_8 = 0; local_8 < 4; local_8 += 1)
            {
                local_c->state = 1;
                g_AnmManager->ExecuteAnmIdx(local_c->vms, local_8 + 0x409);
                if ((local_8 & 1) == 0)
                {
                    local_70 = -128.0f;
                }
                else
                {
                    local_70 = 128.0f;
                }
                local_c->vms[0].pos.x = local_70 + 192.0f;
                if (local_8 / 2 == 0)
                {
                    local_74 = -128.0f;
                }
                else
                {
                    local_74 = 128.0f;
                }
                local_c->vms[0].pos.y = local_74 + 224.0f;
                local_c->vms[0].pos.z = 0.49f;
                (local_c->bombRegionVelocities).z = 0.0f;
                local_c = local_c + 1;
            }
        }
        if ((0x1d < player->bombInfo.bombTimer.current) &&
            (player->bombInfo.bombTimer.current !=
             player->bombInfo.bombTimer.previous))
        {
            if (player->bombInfo.bombTimer.current % 4 == 0)
            {
                player->bombProjectiles[0].pos.x = 192.0f;
                player->bombProjectiles[0].pos.y = 224.0f;
                player->bombProjectiles[0].size.x = 352.0f;
                player->bombProjectiles[0].size.y = 416.0f;
                player->bombProjectiles[0].lifetime = 3;
            }
        }
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 0x28))
        {
            bVar1 = false;
        }
        else
        {
            bVar1 = true;
        }
        if (bVar1)
        {
            BombEffects::RegisterChain(1, 60, 1, 7, 0);
        }
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 100))
        {
            bVar1 = false;
        }
        else
        {
            bVar1 = true;
        }
        if (bVar1)
        {
            BombEffects::RegisterChain(1, 0x46, 0x18, 0, 0);
        }
        local_c = player->bombInfo.subInfo;
        for (local_8 = 0; local_8 < 4; local_8 += 1)
        {
            if (local_c->state != 0)
            {
                g_AnmManager->ExecuteScript(local_c->vms);
            }
            local_c = local_c + 1;
        }
        player->playerState = PLAYER_STATE_INVULNERABLE;
        player->bombInfo.bombTimer.Tick();
    }
    else
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->SpawnBombEffect(&player->positionCenter, 800.0f, 0.0f, 0,
                                ITEM_POINT_BULLET);
    }
}

// FUNCTION: TH07 0x0040d9a0
void BombData::BombSakuyaBDraw(Player *player)
{
    PlayerBombSubInfo *subInfo;
    i32 i;

    DarkenViewport(player);
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 4; i++)
    {
        if (subInfo->state != 0)
        {
            subInfo->vms[0].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            subInfo->vms[0].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            subInfo->vms[0].pos.z = 0.0f;
            g_AnmManager->Draw(subInfo->vms);
            subInfo->vms[0].pos.x -= g_GameManager.arcadeRegionTopLeftPos.x;
            subInfo->vms[0].pos.y -= g_GameManager.arcadeRegionTopLeftPos.y;
        }
        subInfo = subInfo + 1;
    }
}

// FUNCTION: TH07 0x0040da80
void BombData::BombSakuyaBCalcFocus(Player *player)
{
    bool bVar5;
    i32 local_20;
    PlayerBombSubInfo *local_c;
    i32 i;

    if (player->bombInfo.bombTimer.current < player->bombInfo.bombDuration)
    {
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 0))
        {
            bVar5 = false;
        }
        else
        {
            bVar5 = true;
        }
        if (bVar5)
        {
            g_ItemManager.RemoveAllItems();
            g_Gui.ShowBombNamePortrait(0x4a3, "時符「プライベートスクウェア」");
            player->bombInfo.bombDuration = 300;
            player->invulnerabilityTimer = 420;
            SpawnBombInvulnEffect(player);
            player->isBombing = 0;
            local_c = player->bombInfo.subInfo;
            for (i = 0; i < 2; i++)
            {
                local_c->state = 1;
                g_AnmManager->ExecuteAnmIdx(local_c->vms, i + 0x40d);
                local_c->bombRegionPositions = player->positionCenter;
                for (local_20 = 0x1f; -1 < local_20; local_20 += -1)
                {
                    local_c->bombRegionPositionsTrails[local_20] =
                        local_c->bombRegionPositions;
                }
                (local_c->bombRegionVelocities).x = 0.0f;
                (local_c->bombRegionVelocities).y = 0.0f;
                (local_c->bombRegionVelocities).z = 0.0f;
                (local_c->bombRegionAcceleration).x = 0.0f;
                (local_c->bombRegionAcceleration).y = -0.008f;
                (local_c->bombRegionAcceleration).z = 0.0f;
                local_c = local_c + 1;
            }
            player->verticalMovementSpeedMultiplierDuringBomb = 1.5f;
            player->horizontalMovementSpeedMultiplierDuringBomb = 1.5f;
            g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_SAKUMARI, 0);
            ComputeBombCherryDrain(player, 6000, 0.29f);
        }
        player->SpawnBombEffect(&player->bombInfo.subInfo[0].bombRegionPositions,
                                96.0f, 0.0f, 0, ITEM_POINT_BULLET);
        player->bombProjectiles[0].pos.x =
            player->bombInfo.subInfo[0].bombRegionPositions.x;
        player->bombProjectiles[0].pos.y =
            player->bombInfo.subInfo[0].bombRegionPositions.y;
        player->bombProjectiles[0].size.x = 160.0f;
        player->bombProjectiles[0].size.y = 160.0f;
        player->bombProjectiles[0].lifetime = 1;
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 0x28))
        {
            bVar5 = false;
        }
        else
        {
            bVar5 = true;
        }
        if (bVar5)
        {
            BombEffects::RegisterChain(1, 60, 1, 7, 0);
            g_BulletManager.StopBulletMovement();
        }
        if ((player->bombInfo.bombTimer.current ==
             player->bombInfo.bombTimer.previous) ||
            (player->bombInfo.bombTimer.current != 100))
        {
            bVar5 = false;
        }
        else
        {
            bVar5 = true;
        }
        if (bVar5)
        {
            g_BulletManager.StopBulletMovement();
            BombEffects::RegisterChain(1, 0x46, 0x18, 0, 0);
        }
        local_c = player->bombInfo.subInfo;
        for (i = 0; i < 2; i++)
        {
            if (local_c->state != 0)
            {
                local_c->bombRegionAcceleration =
                    (player->positionCenter - local_c->bombRegionPositions) *
                    0.00058823527f * g_Supervisor.effectiveFramerateMultiplier;
                local_c->bombRegionVelocities +=
                    g_Supervisor.effectiveFramerateMultiplier *
                    local_c->bombRegionAcceleration;
                for (local_20 = 0x1f; 0 < local_20; local_20 += -1)
                {
                    local_c->bombRegionPositionsTrails[local_20] =
                        local_c->bombRegionPositionsTrails[local_20 - 1];
                }
                local_c->bombRegionPositionsTrails[0] = local_c->bombRegionPositions;
                local_c->bombRegionPositions +=
                    g_Supervisor.effectiveFramerateMultiplier *
                    local_c->bombRegionVelocities;
                g_AnmManager->ExecuteScript(local_c->vms);
            }
            local_c = local_c + 1;
        }
        player->playerState = PLAYER_STATE_INVULNERABLE;
        player->bombInfo.bombTimer.Tick();
    }
    else
    {
        g_Gui.EndPlayerSpellcard();
        player->bombInfo.isInUse = 0;
        player->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
        player->SpawnBombEffect(&player->positionCenter, 800.0f, 0.0f, 0,
                                ITEM_POINT_BULLET);
        player->bombHitboxes[0].pos.x = 192.0f;
        player->bombHitboxes[0].pos.y = 224.0f;
        player->bombHitboxes[0].pos.z = 448.0f;
        player->bombHitboxes[0].size.x = 512.0f;
    }
}

// FUNCTION: TH07 0x0040e280
void BombData::BombSakuyaBDrawFocus(Player *player)
{
    u8 bVar2;
    i32 j;
    PlayerBombSubInfo *subInfo;
    i32 i;

    DarkenViewport(player);
    subInfo = player->bombInfo.subInfo;
    for (i = 0; i < 2; i++)
    {
        if (subInfo->state != 0)
        {
            bVar2 = subInfo->vms[0].color.bytes.a;
            subInfo->vms[0].pos = subInfo->bombRegionPositions;
            subInfo->vms[0].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            subInfo->vms[0].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            subInfo->vms[0].pos.z = 0.0f;
            g_AnmManager->Draw(subInfo->vms);
            for (j = 3; j < 0x20; j += 4)
            {
                subInfo->vms[0].pos = subInfo->bombRegionPositionsTrails[j];
                subInfo->vms[0].color.bytes.a =
                    bVar2 - (char)((i32)((u32)bVar2 * j) / 32);
                subInfo->vms[0].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
                subInfo->vms[0].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
                subInfo->vms[0].pos.z = 0.0f;
                g_AnmManager->Draw(subInfo->vms);
            }
            subInfo->vms[0].color.bytes.a = bVar2;
        }
        subInfo = subInfo + 1;
    }
}
