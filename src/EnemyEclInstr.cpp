#include "EnemyEclInstr.hpp"

#include "BulletManager.hpp"
#include "EnemyManager.hpp"
#include "GameManager.hpp"
#include "Player.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "ZunMath.hpp"
#include "utils.hpp"

// GLOBAL: TH07 0x0049f158
EclExInstr g_EclExInstr[24] = {
    EnemyEclInstr::ExInsSetPosToBoss,
    EnemyEclInstr::ExInsAliceCurveBullets,
    EnemyEclInstr::ExInsTurnBulletsIntoOtherBullets,
    EnemyEclInstr::ExInsNoOp,
    EnemyEclInstr::ExInsDespawnLargeBulletAndSavePos,
    EnemyEclInstr::ExInsCopyMainBossMovement,
    EnemyEclInstr::ExInsSplitBulletsOrShootBackwards,
    EnemyEclInstr::ExInsReflectBulletsFromLasers,
    EnemyEclInstr::ExInsShootBulletsAlongLaser,
    EnemyEclInstr::ExInsEffect1eAccel,
    EnemyEclInstr::ExInsYoumuSetGameSpeed,
    EnemyEclInstr::ExInsYoumuRestoreGameSpeed,
    EnemyEclInstr::ExInsBurstLargeBullets,
    EnemyEclInstr::ExInsYoumuCurveBulletsBelow,
    EnemyEclInstr::ExInsYoumuRedirectBulletsToPlayer,
    EnemyEclInstr::ExInsFlashScreen,
    EnemyEclInstr::ExInsYuyukoTransformButterflyBullets,
    EnemyEclInstr::ExInsYuyukoButterflySpawnEnemy,
    EnemyEclInstr::ExInsYuyukoCountButterflyBullets,
    EnemyEclInstr::ExInsYuyukoFadeOutMusic,
    EnemyEclInstr::ExInsYuyukoPlayResurrectionButterflyBgm,
    EnemyEclInstr::ExInsBurstLargeBullets2,
    EnemyEclInstr::ExInsSpawnBulletsWithDirChange,
    EnemyEclInstr::ExInsSpawnBulletsWithDirChange2,
};

// GLOBAL: TH07 0x0049f3ec
EclInterpFn g_EclInterpFuncs[8] = {
    EclManager::MathLerp,
    EclManager::MathLerp,
    EclManager::MathLerp,
    EclManager::MathLerp,
    EclManager::MathLerp,
    EclManager::MathLerp,
    EclManager::MathLerp,
    EclManager::MathCubicInterp,
};

// FUNCTION: TH07 0x00417b90
void EnemyEclInstr::ExInsSetPosToBoss(Enemy *enemy, EclRawInstr *instr)
{
    enemy->position = g_EnemyManager.bosses[instr->args[1].i]->position;
    enemy->axisSpeed = g_EnemyManager.bosses[instr->args[1].i]->axisSpeed;
    enemy->angle = g_EnemyManager.bosses[instr->args[1].i]->angle;
    enemy->flags4 = enemy->flags4 | 1;
}

// FUNCTION: TH07 0x00417c30
void EnemyEclInstr::ExInsAliceCurveBullets(Enemy *enemy, EclRawInstr *instr)
{
    f32 local_10;
    Bullet *bullet;
    i32 i;

    bullet = g_BulletManager.bullets;
    BombEffects::RegisterChain(1, 0x1e, 0xc, 0, 0);
    BombEffects::RegisterChain(3, 4, 3, 0x80ffcfcf, 0);
    for (i = 0; i < 0x400; i += 1)
    {
        if (((((bullet->state != BULLET_INACTIVE) &&
               (bullet->state != BULLET_DESPAWN)) &&
              (bullet->sprites.spriteBullet.sprite != NULL)) &&
             ((bullet->state2 == 0 &&
               ((instr->args[1].i != 1 || (bullet->spriteOffset == 8)))))) &&
            ((instr->args[1].i != 2 || (bullet->spriteOffset == 4))))
        {
            if (bullet->spriteOffset == 2)
            {
                local_10 = -ZUN_PI / (g_Rng.GetRandomFloat() * 60.0f + 180.0f);
            }
            else if (bullet->spriteOffset == 6)
            {
                local_10 = ZUN_PI / (g_Rng.GetRandomFloat() * 60.0f + 180.0f);
            }
            else if (bullet->spriteOffset == 8)
            {
                local_10 = ZUN_PI / (g_Rng.GetRandomFloat() * 60.0f + 180.0f);
            }
            else if (bullet->spriteOffset == 4)
            {
                local_10 = -ZUN_PI / (g_Rng.GetRandomFloat() * 60.0f + 180.0f);
            }
            bullet->speed = 0.3f;
            memset(bullet->commands, 0, sizeof(bullet->commands));
            if (g_GameManager.difficulty < 3)
            {
                bullet->AddAngleAccelCommand(0, 0, 0x3c, local_10, 0.016666668f);
            }
            else
            {
                bullet->AddAngleAccelCommand(0, 0, 0xf0, local_10, 0.005263158f);
            }
            bullet->state2 = 1;
        }
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x00417e50
void EnemyEclInstr::ExInsTurnBulletsIntoOtherBullets(Enemy *enemy,
                                                     EclRawInstr *instr)
{
    f32 angle;
    Bullet *bullet;
    f32 local_e4;
    i32 i;
    EnemyBulletShooter local_dc;

    bullet = g_BulletManager.bullets;
    memset(&local_dc, 0, sizeof(EnemyBulletShooter));
    local_dc.soundOverride = -1;
    switch (instr->args[1].i)
    {
    case 0:
        BombEffects::RegisterChain(1, 0x20, 0xc, 0, 0);
        BombEffects::RegisterChain(3, 4, 1, 0x80cfcfff, 0);
        local_e4 = 128.0f;
        break;
    case 1:
        local_e4 = 192.0f;
        break;
    case 2:
        local_e4 = 256.0f;
        break;
    case 3:
        local_e4 = 999.0;
    }
    for (i = 0; i < 0x400; i += 1)
    {
        if ((((bullet->state != BULLET_INACTIVE) &&
              (bullet->state != BULLET_DESPAWN)) &&
             (bullet->sprites.spriteBullet.sprite != NULL)) &&
            ((bullet->spriteOffset == 2 &&
              (sqrtf((enemy->position.x - bullet->pos.x) *
                         (enemy->position.x - bullet->pos.x) +
                     (enemy->position.y - bullet->pos.y) *
                         (enemy->position.y - bullet->pos.y)) < local_e4))))
        {
            local_dc.position = bullet->pos;
            local_dc.sprite = 0;
            local_dc.spriteOffset = 6;
            local_dc.angle1 = 0.0f;
            local_dc.angle2 = -ZUN_PI;
            local_dc.speed1 = 0.7f;
            local_dc.count1 = 2;
            local_dc.count2 = 1;
            local_dc.flags = 2;
            local_dc.aimMode = 6;
            angle = 1.5707964f;
            local_dc.AddTargetVelocityCommand(
                0, 0, 0xb4, g_Rng.GetRandomFloat() * 0.005f + 0.013f, angle);
            g_BulletManager.SpawnBulletPattern(&local_dc);
            bullet->Initialize();
        }
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x00418110
void EnemyEclInstr::ExInsNoOp(Enemy *enemy, EclRawInstr *instr)
{
    return;
}

// FUNCTION: TH07 0x00418120
void EnemyEclInstr::ExInsDespawnLargeBulletAndSavePos(Enemy *enemy,
                                                      EclRawInstr *instr)
{
    Bullet *bullet;
    i32 local_e0;
    EnemyBulletShooter local_dc;

    bullet = g_BulletManager.bullets;
    memset(&local_dc, 0, sizeof(EnemyBulletShooter));
    local_dc.soundOverride = -1;
    enemy->currentContext.eclContextArgs.floatVars1[0] = -999.0f;
    local_e0 = 0;
    while (true)
    {
        if (0x3ff < local_e0)
        {
            return;
        }
        if (((bullet->state != BULLET_INACTIVE) &&
             (bullet->state != BULLET_DESPAWN)) &&
            (60.0f <= (bullet->sprites.spriteBullet.sprite)->heightPx))
            break;
        local_e0 += 1;
        bullet = bullet + 1;
    }
    enemy->currentContext.eclContextArgs.floatVars1[0] = bullet->pos.x;
    enemy->currentContext.eclContextArgs.floatVars1[1] = bullet->pos.y;
    g_EffectManager.SpawnParticles(2, &bullet->pos, 1, 0xffffffff);
    bullet->Initialize();
}

// FUNCTION: TH07 0x00418260
void EnemyEclInstr::ExInsCopyMainBossMovement(Enemy *enemy, EclRawInstr *instr)
{
    enemy->moveInterpStartPos = g_EnemyManager.bosses[0]->position;
    enemy->moveRadius = g_EnemyManager.bosses[0]->moveRadius;
    enemy->moveAngularVelocity = g_EnemyManager.bosses[0]->moveAngularVelocity;
}

// FUNCTION: TH07 0x004182d0
void EnemyEclInstr::ExInsSplitBulletsOrShootBackwards(Enemy *enemy,
                                                      EclRawInstr *instr)
{
    Bullet *bullet;
    i32 i;
    EnemyBulletShooter local_dc;

    bullet = g_BulletManager.bullets;
    memset(&local_dc, 0, sizeof(EnemyBulletShooter));
    local_dc.soundOverride = -1;
    for (i = 0; i < 0x400; i += 1)
    {
        if ((((bullet->state != BULLET_INACTIVE) &&
              (bullet->state != BULLET_DESPAWN)) &&
             (bullet->sprites.spriteBullet.sprite != NULL)) &&
            ((((instr->args[1].i == 0 && (bullet->spriteOffset == 6)) ||
               ((instr->args[1].i == 1 && (bullet->spriteOffset == 0xf)))) ||
              ((instr->args[1].i == 2 && (bullet->spriteOffset == 2))))))
        {
            local_dc.position = bullet->pos;
            local_dc.sprite = 6;
            local_dc.spriteOffset = 0xf;
            local_dc.angle1 = utils::AddNormalizeAngle(bullet->angle, ZUN_PI);
            local_dc.angle2 = 0.5235988f;
            local_dc.speed1 = bullet->speed * 1.1f;
            if (g_GameManager.difficulty < 3)
            {
                local_dc.count1 = 4;
            }
            else
            {
                local_dc.count1 = 2;
                local_dc.angle2 = 1.5707964f;
            }
            local_dc.count2 = 1;
            local_dc.flags = 2;
            local_dc.AddSpawnDelayCommand(0, 0, 0x82);
            if (instr->args[1].i == 0)
            {
                local_dc.flags = 0x2002;
            }
            else if (instr->args[1].i == 1)
            {
                if (g_GameManager.difficulty == DIFF_LUNATIC)
                {
                    local_dc.flags = 0x2002;
                }
                else
                {
                    local_dc.flags = 2;
                }
                local_dc.spriteOffset = 2;
            }
            else if (instr->args[1].i == 2)
            {
                local_dc.flags = 2;
                local_dc.spriteOffset = 10;
            }
            local_dc.aimMode = 1;
            g_BulletManager.SpawnBulletPattern(&local_dc);
            local_dc.angle2 = 1.0471976f;
            if (instr->args[1].i == 0)
            {
                local_dc.flags = 0x2000;
            }
            else if (instr->args[1].i == 1)
            {
                if (g_GameManager.difficulty == DIFF_LUNATIC)
                {
                    local_dc.flags = 0x2000;
                }
                else
                {
                    local_dc.flags = 0;
                }
            }
            else
            {
                local_dc.flags = 0;
            }
            local_dc.speed1 = bullet->speed * 0.7f;
            local_dc.count1 = 2;
            g_BulletManager.SpawnBulletPattern(&local_dc);
            local_dc.speed1 = bullet->speed * 0.85f;
            local_dc.count1 = 1;
            g_BulletManager.SpawnBulletPattern(&local_dc);
            bullet->Initialize();
        }
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x004185d0
i32 IsPointInRotatedRect(D3DXVECTOR3 *param_1, D3DXVECTOR3 *param_2,
                         D3DXVECTOR3 *param_3, D3DXVECTOR3 *param_4,
                         f32 param_5, f32 param_6)
{
    f32 fVar1;
    f32 fVar2;
    f32 fVar3;

    fVar1 = param_1->y - param_4->y;
    fVar2 = param_1->x - param_4->x;
    fVar3 = (fVar1 * param_6 - fVar2 * param_5) + param_4->y;
    fVar1 = fVar1 * param_5 + fVar2 * param_6 + param_4->x;
    if ((((param_3->x * 0.5f + param_2->x < fVar1) ||
          (fVar1 < param_2->x - param_3->x * 0.5f)) ||
         (param_3->y * 0.5f + param_2->y < fVar3)) ||
        (fVar3 < param_2->y - param_3->y * 0.5f))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

// FUNCTION: TH07 0x00418880
void EnemyEclInstr::ExInsReflectBulletsFromLasers(Enemy *enemy,
                                                  EclRawInstr *instr)
{
    D3DXVECTOR3 local_38;
    f32 local_2c;
    u32 local_28;
    Laser *laser;
    Bullet *bullet;
    D3DXVECTOR3 local_1c;
    f32 local_10;
    f32 local_c;
    i32 local_8;

    laser = g_BulletManager.lasers;
    for (local_28 = 0; (i32)local_28 < 0x40; local_28 += 1)
    {
        if (laser->inUse != 0)
        {
            if ((enemy->timer.current % 2 == local_28) && (laser->state < 2))
            {
                local_1c.y = laser->width;
                local_1c.x = laser->endOffset - laser->startOffset;
                local_38.x = (laser->endOffset - laser->startOffset) / 2.0f +
                             laser->startOffset + (laser->pos).x;
                local_38.y = (laser->pos).y;
                sincosf(&local_c, &local_2c, laser->angle);
                bullet = g_BulletManager.bullets;
                for (local_8 = 0; local_8 < 0x400; local_8 += 1)
                {
                    if ((((bullet->state != BULLET_INACTIVE) &&
                          (bullet->state != BULLET_DESPAWN)) &&
                         (bullet->sprites.spriteBullet.sprite != NULL)) &&
                        (IsPointInRotatedRect(&bullet->pos, &local_38, &local_1c,
                                              &laser->pos, local_c, local_2c) != 0))
                    {
                        if (0 < bullet->state2)
                        {
                            bullet->state2 = bullet->state2 - 1;
                        }
                        if (bullet->state2 == 0)
                        {
                            if (0.5f < bullet->speed)
                            {
                                bullet->speed = bullet->speed - 0.1f;
                            }
                            local_10 =
                                local_c * bullet->velocity.x + local_2c * bullet->velocity.y;
                            if (local_10 < 0.0f)
                            {
                                bullet->angle =
                                    utils::AddNormalizeAngle(laser->angle, -1.5707964f);
                            }
                            else
                            {
                                bullet->angle =
                                    utils::AddNormalizeAngle(laser->angle, 1.5707964f);
                            }
                            AngleToVector(&bullet->velocity, bullet->angle,
                                          g_Supervisor.effectiveFramerateMultiplier *
                                              bullet->speed);
                            bullet->state2 = 10;
                            bullet->sprites = g_BulletManager.bulletTypeTemplates[5];
                            g_AnmManager->SetActiveSprite(
                                &bullet->sprites.spriteBullet,
                                (i32)bullet->sprites.spriteBullet.activeSpriteIdx +
                                    (i32)bullet->spriteOffset);
                        }
                    }
                    bullet = bullet + 1;
                }
            }
        }
        laser = laser + 1;
    }
}

// FUNCTION: TH07 0x00418b40
void EnemyEclInstr::ExInsShootBulletsAlongLaser(Enemy *enemy,
                                                EclRawInstr *instr)
{
    i32 iVar1;
    D3DXVECTOR3 local_40;
    f32 local_34;
    i32 local_30;
    Laser *laser;
    f32 local_28;
    Bullet *bullet;
    D3DXVECTOR3 local_20;
    f32 local_14;
    f32 local_10;
    i32 local_c;
    f32 local_8;

    laser = g_BulletManager.lasers;
    for (local_30 = 0; local_30 < 0x40; local_30 += 1)
    {
        if (((laser->inUse != 0) && (enemy->timer.current % 3 == local_30 % 3)) &&
            (laser->state < 2))
        {
            local_20.y = laser->width * 1.5f;
            local_20.x = laser->endOffset - laser->startOffset;
            local_40.x = (laser->endOffset - laser->startOffset) / 2.0f +
                         laser->startOffset + (laser->pos).x;
            local_40.y = (laser->pos).y;
            sincosf(&local_10, &local_34, laser->angle);
            local_28 = -local_10;
            local_8 = local_34;
            bullet = g_BulletManager.bullets;
            for (local_c = 0; local_c < 0x400; local_c += 1)
            {
                if ((((bullet->state != BULLET_INACTIVE) &&
                      (bullet->state != BULLET_DESPAWN)) &&
                     ((bullet->sprites.spriteBullet.sprite != NULL &&
                       ((bullet->state2 != local_30 + 1 && (-1 < bullet->state2)))))) &&
                    (iVar1 = IsPointInRotatedRect(&bullet->pos, &local_40, &local_20,
                                                  &laser->pos, local_10, local_34),
                     iVar1 != 0))
                {
                    if (g_GameManager.difficulty < 2)
                    {
                        bullet->speed =
                            (g_Rng.GetRandomFloat() * 0.3f + 0.7f) * bullet->speed;
                    }
                    else
                    {
                        bullet->speed =
                            (g_Rng.GetRandomFloat() * 0.4f + 0.8f) * bullet->speed;
                    }
                    local_14 =
                        local_8 * bullet->velocity.y + local_28 * bullet->velocity.x;
                    if (local_14 < 0.0f)
                    {
                        bullet->velocity.x = -local_28;
                        bullet->velocity.y = -local_8;
                    }
                    else
                    {
                        bullet->velocity.x = local_28;
                        bullet->velocity.y = local_8;
                    }
                    bullet->sprites = g_BulletManager.bulletTypeTemplates[5];
                    g_AnmManager->SetActiveSprite(
                        &bullet->sprites.spriteBullet,
                        (i32)bullet->sprites.spriteBullet.activeSpriteIdx +
                            (i32)bullet->spriteOffset);
                    bullet->angle = atan2f(bullet->velocity.y, bullet->velocity.x);
                    AngleToVector(&bullet->velocity, bullet->angle, bullet->speed);
                    if (g_GameManager.difficulty < 2)
                    {
                        bullet->state2 = -1;
                    }
                    else
                    {
                        bullet->state2 = local_30 + 1;
                    }
                }
                bullet = bullet + 1;
            }
        }
        laser = laser + 1;
    }
}

// FUNCTION: TH07 0x00418e80
void EnemyEclInstr::ExInsEffect1eAccel(Enemy *enemy, EclRawInstr *instr)
{
    BombEffects::RegisterChain(1, 0x50, 8, 0, 0);
    EffectManager::ModifyEffect1eAcceleration();
}

// FUNCTION: TH07 0x00418eb0
void EnemyEclInstr::ExInsYoumuSetGameSpeed(Enemy *enemy, EclRawInstr *instr)
{
    Bullet *bullet;
    i32 i;

    g_Supervisor.effectiveFramerateMultiplier = 1.0f / (f32)instr->args[1].i;
    g_Stage.spellcardVms[0].pendingInterrupt = 2;
    g_Stage.spellcardVms[1].pendingInterrupt = 2;
    bullet = g_BulletManager.bullets;
    for (i = 0; i < 0x400; i += 1)
    {
        if (bullet->state != BULLET_INACTIVE)
        {
            bullet->velocity *= g_Supervisor.effectiveFramerateMultiplier;
            bullet->sprites.spriteBullet.baseSpriteIdx =
                bullet->sprites.spriteBullet.activeSpriteIdx;
            if ((0x25f < bullet->sprites.spriteBullet.activeSpriteIdx) &&
                (bullet->sprites.spriteBullet.activeSpriteIdx < 0x270))
            {
                g_AnmManager->SetActiveSprite(&bullet->sprites.spriteBullet, 0x26f);
            }
        }
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x00418fc0
void EnemyEclInstr::ExInsYoumuRestoreGameSpeed(Enemy *enemy, EclRawInstr *instr)
{
    Bullet *bullet;
    i32 i;

    bullet = g_BulletManager.bullets;
    for (i = 0; i < 0x400; i += 1)
    {
        if (bullet->state != BULLET_INACTIVE)
        {
            bullet->velocity *= 1.0f / g_Supervisor.effectiveFramerateMultiplier;
            if ((0x25f < bullet->sprites.spriteBullet.activeSpriteIdx) &&
                (bullet->sprites.spriteBullet.activeSpriteIdx < 0x270))
            {
                g_AnmManager->SetActiveSprite(
                    &bullet->sprites.spriteBullet,
                    (i32)bullet->sprites.spriteBullet.baseSpriteIdx);
            }
        }
        bullet = bullet + 1;
    }
    if (1.0f / (f32)instr->args[1].i < 1.0f)
    {
        g_Supervisor.flags |= 0x20;
    }
    g_Supervisor.effectiveFramerateMultiplier = 1.0f;
    g_Stage.spellcardVms[0].pendingInterrupt = 1;
    g_Stage.spellcardVms[1].pendingInterrupt = 1;
}

// FUNCTION: TH07 0x004190f0
void EnemyEclInstr::ExInsBurstLargeBullets(Enemy *enemy, EclRawInstr *instr)
{
    f32 fVar5;
    u32 local_108;
    i32 local_104;
    i32 local_100;
    u32 j;
    Bullet *bullet;
    i32 i;
    EnemyBulletShooter local_dc;

    bullet = g_BulletManager.bullets;
    memset(&local_dc, 0, sizeof(EnemyBulletShooter));
    local_dc.soundOverride = -1;
    BombEffects::RegisterChain(3, 8, 1, 0x50cfcfff, 0);
    if (g_GameManager.difficulty == DIFF_EASY)
    {
        local_100 = 10;
    }
    else
    {
        if (g_GameManager.difficulty == DIFF_NORMAL)
        {
            local_104 = 0x12;
        }
        else
        {
            local_104 =
                ((g_GameManager.difficulty != DIFF_HARD) - 1 & 0xfffffffd) + 0x19;
        }
        local_100 = local_104;
    }
    for (i = 0; i < 0x400; i += 1)
    {
        if ((bullet->state != BULLET_INACTIVE) &&
            (((((g_GameManager.difficulty < 2 &&
                 (48.0f < (bullet->sprites.spriteBullet.sprite)->heightPx)) &&
                (enemy->position.y - 64.0f < bullet->pos.y)) &&
               (bullet->pos.y < enemy->position.y + 64.0f)) ||
              (((1 < g_GameManager.difficulty &&
                 (48.0f < (bullet->sprites.spriteBullet.sprite)->heightPx)) &&
                ((enemy->position.y - 48.0f < bullet->pos.y &&
                  (bullet->pos.y < enemy->position.y + 48.0f))))))))
        {
            for (j = 0; (i32)j < local_100; j += 1)
            {
                local_dc.position = bullet->pos;
                local_dc.position.x += (g_Rng.GetRandomFloat() * 32.0f - 16.0f);
                local_dc.position.y += (g_Rng.GetRandomFloat() * 32.0f - 16.0f);

                local_108 = (u32)g_Rng.GetRandomU16() % 3;
                if (local_108 == 0)
                {
                    local_dc.sprite = 0;
                    local_dc.spriteOffset = 2;
                }
                else if (local_108 == 1)
                {
                    local_dc.sprite = 3;
                    local_dc.spriteOffset = 2;
                }
                else if (local_108 == 2)
                {
                    local_dc.sprite = 7;
                    local_dc.spriteOffset = 1;
                }
                if (instr->args[1].i == 0)
                {
                    local_dc.angle1 = g_Rng.GetRandomFloat() * 4.712389f - 1.5707964f;
                }
                else
                {
                    fVar5 = 0.7853982f;
                    local_dc.angle1 = utils::AddNormalizeAngle(
                        g_Rng.GetRandomFloat() * 4.712389f, fVar5);
                }
                local_dc.speed1 = 0.1f;
                local_dc.count1 = 1;
                local_dc.count2 = 1;
                local_dc.flags = (j & 1) ? 2 : 0;
                local_dc.aimMode = 1;
                local_dc.AddAngleAccelCommand(0, 0, 100, 0.0f,
                                              g_Rng.GetRandomFloat() * 0.008f + 0.01f);
                g_BulletManager.SpawnBulletPattern(&local_dc);
            }
            bullet->Initialize();
        }
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x004194e0
void EnemyEclInstr::ExInsYoumuCurveBulletsBelow(Enemy *enemy,
                                                EclRawInstr *instr)
{
    f32 local_18;
    Bullet *bullet;
    i32 i;

    bullet = g_BulletManager.bullets;
    for (i = 0; i < 0x400; i += 1)
    {
        if ((((bullet->state != BULLET_INACTIVE) && (bullet->state2 == 0)) &&
             (enemy->position.y < bullet->pos.y)) &&
            (((bullet->pos.y < 352.0f &&
               (enemy->position.x - 16.0f < bullet->pos.x)) &&
              (bullet->pos.x < enemy->position.x + 16.0f))))
        {
            if ((i & 1) == 0)
            {
                local_18 = -0.05235988f;
            }
            else
            {
                local_18 = 0.05235988f;
            }
            bullet->AddAngleAccelCommand(0, 0, 0xa0, local_18,
                                         -bullet->speed / 180.0f);
            bullet->state2 = 1;
        }
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x00419610
void EnemyEclInstr::ExInsYoumuRedirectBulletsToPlayer(Enemy *enemy,
                                                      EclRawInstr *instr)
{
    Bullet *bullet;
    i32 i;

    bullet = g_BulletManager.bullets;
    BombEffects::RegisterChain(3, 0x10, 1, 0x50cfcfff, 0);
    for (i = 0; i < 0x400; i += 1)
    {
        if ((bullet->state != BULLET_INACTIVE) && (bullet->state2 == 1))
        {
            bullet->AddTargetVelocityCommand(0, 0, 0x5a, 0.026666667f,
                                             g_Player.AngleToPlayer(&bullet->pos));
            bullet->commands[1].type = 0;
            bullet->state2 = 2;
        }
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x004196e0
void EnemyEclInstr::ExInsFlashScreen(Enemy *enemy, EclRawInstr *instr)
{
    BombEffects::RegisterChain(3, instr->args[1].i, 1, 0xd0cfcfff, 0);
}

// FUNCTION: TH07 0x00419710
void EnemyEclInstr::ExInsYuyukoTransformButterflyBullets(Enemy *enemy,
                                                         EclRawInstr *instr)
{
    Bullet *bullet;
    EnemyBulletShooter local_e4;
    i32 i;

    bullet = g_BulletManager.bullets;
    memset(&local_e4, 0, sizeof(EnemyBulletShooter));
    local_e4.soundOverride = -1;
    for (i = 0; i < 0x400; i += 1)
    {
        if ((((bullet->state != BULLET_INACTIVE) && (bullet->state2 == 0)) &&
             (0x277 < bullet->sprites.spriteBullet.activeSpriteIdx)) &&
            (bullet->sprites.spriteBullet.activeSpriteIdx < 0x280))
        {
            local_e4.position = bullet->pos;
            local_e4.sprite = 0;
            local_e4.spriteOffset = 6;
            local_e4.angle1 = utils::AddNormalizeAngle(bullet->angle, ZUN_PI);
            local_e4.angle2 = 0.3926991f;
            local_e4.speed1 = enemy->currentContext.eclContextArgs.floatVars1[1];
            local_e4.count1 = 5;
            local_e4.count2 = 1;
            local_e4.flags = 2;
            local_e4.aimMode = 1;
            g_BulletManager.SpawnBulletPattern(&local_e4);
        }
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x00419880
void EnemyEclInstr::ExInsYuyukoButterflySpawnEnemy(Enemy *enemy,
                                                   EclRawInstr *instr)
{
    f32 local_78;
    EclContextArgs local_74;
    Bullet *bullet;
    i32 local_8;

    bullet = g_BulletManager.bullets;
    local_74 = enemy->currentContext.eclContextArgs;
    local_78 = -ZUN_PI;
    BombEffects::RegisterChain(3, 0xc, 1, 0x80cfcfff, 0);
    for (local_8 = 0; local_8 < 0x400; local_8 += 1)
    {
        if (bullet->state != BULLET_INACTIVE)
        {
            if ((bullet->state2 == 0) &&
                (bullet->sprites.spriteBullet.activeSpriteIdx == 0x27c))
            {
                local_74.floatVars1[0] = bullet->angle;
                local_74.floatVars1[7] = local_78;
                local_78 = local_78 + 0.7853982f;
                g_EnemyManager.SpawnEnemyEx(enemy->currentContext.subId + 1,
                                            &bullet->pos, 1, -2, 10, &local_74);
                bullet->Initialize();
            }
            else if ((0x277 < bullet->sprites.spriteBullet.activeSpriteIdx) &&
                     (bullet->sprites.spriteBullet.activeSpriteIdx < 0x280))
            {
                bullet->Initialize();
            }
        }
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x004199c0
void EnemyEclInstr::ExInsYuyukoCountButterflyBullets(Enemy *enemy,
                                                     EclRawInstr *instr)
{
    Bullet *bullet;
    i32 i;

    bullet = g_BulletManager.bullets;
    enemy->currentContext.eclContextArgs.intVars1[0] = 0;
    for (i = 0; i < 0x400; i += 1)
    {
        if (((bullet->state != BULLET_INACTIVE) && (bullet->state2 == 0)) &&
            (bullet->sprites.spriteBullet.activeSpriteIdx == 0x27c))
        {
            enemy->currentContext.eclContextArgs.intVars1[0] =
                enemy->currentContext.eclContextArgs.intVars1[0] + 1;
        }
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x00419a50
void EnemyEclInstr::ExInsBurstLargeBullets2(Enemy *enemy, EclRawInstr *instr)
{
    f32 fVar5;
    u32 local_108;
    f32 local_104;
    i32 j;
    Bullet *bullet;
    i32 i;
    EnemyBulletShooter local_e4;

    bullet = g_BulletManager.bullets;
    memset(&local_e4, 0, sizeof(EnemyBulletShooter));
    local_e4.soundOverride = -1;
    if (g_GameManager.difficulty == DIFF_HARD)
    {
        local_104 = 128.0f;
    }
    else
    {
        local_104 = 180.0f;
    }
    BombEffects::RegisterChain(3, 8, 1, 0x50cfcfff, 0);
    for (i = 0; i < 0x400; i += 1)
    {
        if ((((bullet->state != BULLET_INACTIVE) &&
              (48.0f < (bullet->sprites.spriteBullet.sprite)->heightPx)) &&
             (enemy->position.y - local_104 < bullet->pos.y)) &&
            (bullet->pos.y < local_104 + enemy->position.y))
        {
            for (j = 0; j < 0xf; j += 1)
            {
                local_e4.position = bullet->pos;
                local_e4.position.x += (g_Rng.GetRandomFloat() * 32.0f - 16.0f);
                local_e4.position.y += (g_Rng.GetRandomFloat() * 32.0f - 16.0f);

                local_108 = (u32)g_Rng.GetRandomU16() % 3;
                if (local_108 == 0)
                {
                    local_e4.sprite = 0;
                    local_e4.spriteOffset = 4;
                }
                else if (local_108 == 1)
                {
                    local_e4.sprite = 3;
                    local_e4.spriteOffset = 4;
                }
                else if (local_108 == 2)
                {
                    local_e4.sprite = 7;
                    local_e4.spriteOffset = 2;
                }
                if (instr->args[1].i == 0)
                {
                    local_e4.angle1 = g_Rng.GetRandomFloat() * 4.712389f - 1.5707964f;
                }
                else
                {
                    fVar5 = 0.7853982f;
                    local_e4.angle1 = utils::AddNormalizeAngle(
                        g_Rng.GetRandomFloat() * 4.712389f, fVar5);
                }
                local_e4.speed1 = 0.1f;
                local_e4.count1 = 1;
                local_e4.count2 = 1;
                local_e4.flags = 0;
                local_e4.aimMode = 1;
                local_e4.AddAngleAccelCommand(0, 0, 100, 0.0f,
                                              g_Rng.GetRandomFloat() * 0.008f + 0.01f);
                g_BulletManager.SpawnBulletPattern(&local_e4);
            }
            bullet->Initialize();
        }
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x00419d70
void EnemyEclInstr::ExInsYuyukoFadeOutMusic(Enemy *enemy, EclRawInstr *instr)
{
    g_Supervisor.FadeOutMusic(3.0f);
}

// FUNCTION: TH07 0x00419d90
void EnemyEclInstr::ExInsYuyukoPlayResurrectionButterflyBgm(Enemy *enemy,
                                                            EclRawInstr *instr)
{
    if (Supervisor::PlayLoadedAudio(2) != ZUN_SUCCESS)
    {
        Supervisor::PlayAudio("bgm/th07_13b.mid");
    }
}

// FUNCTION: TH07 0x00419dc0
void EnemyEclInstr::ExInsSpawnBulletsWithDirChange(Enemy *enemy,
                                                   EclRawInstr *instr)
{
    Bullet *bullet;
    EnemyBulletShooter local_e4;
    i32 i;

    bullet = g_BulletManager.bullets;
    memset(&local_e4, 0, sizeof(EnemyBulletShooter));
    local_e4.soundOverride = -1;
    if (enemy->timer.current % 3 != 0)
    {
        for (i = 0; i < 0x400; i += 1)
        {
            if ((((bullet->state != BULLET_INACTIVE) &&
                  ((bullet->exFlags & 0x40U) == 0)) &&
                 (bullet->pos.y < 320.0f)) &&
                (60.0f < (bullet->sprites.spriteBullet.sprite)->heightPx))
            {
                local_e4.position = bullet->pos;
                if (enemy->timer.current % 2 == 0)
                {
                    local_e4.sprite = 3;
                }
                else
                {
                    local_e4.sprite = 1;
                }
                local_e4.spriteOffset =
                    (-(u16)(bullet->spriteOffset != 1) & 0xfffc) + 6;
                local_e4.angle1 = g_Rng.GetRandomFloat() * ZUN_2PI - ZUN_PI;
                local_e4.angle2 = -ZUN_PI;
                if (enemy->timer.current % 2 == 0)
                {
                    local_e4.speed1 = 0.8f;
                    local_e4.count1 = 2;
                }
                else
                {
                    local_e4.speed1 = 1.2f;
                    local_e4.count1 = 1;
                }
                local_e4.count2 = 1;
                local_e4.flags = 0x208;
                local_e4.aimMode = 3;
                local_e4.soundOverride = SOUND_25;
                if (enemy->timer.current % 2 != 0)
                {
                    local_e4.AddDirChangeCommand(0, 0, 0x3c, 1, 0.0f, 3.1f);
                }
                g_BulletManager.SpawnBulletPattern(&local_e4);
            }
            bullet = bullet + 1;
        }
    }
}

// FUNCTION: TH07 0x00419ff0
void EnemyEclInstr::ExInsSpawnBulletsWithDirChange2(Enemy *enemy,
                                                    EclRawInstr *instr)
{
    i32 iVar1;
    Bullet *bullet;
    EnemyBulletShooter local_e4;
    i32 i;

    bullet = g_BulletManager.bullets;
    memset(&local_e4, 0, sizeof(EnemyBulletShooter));
    local_e4.soundOverride = -1;
    if (enemy->timer.current % 3 != 2)
    {
        iVar1 = enemy->timer.current % 3;
        for (i = 0; i < 0x400; i += 1)
        {
            if ((((bullet->state != BULLET_INACTIVE) &&
                  ((bullet->exFlags & 0x40U) == 0)) &&
                 (bullet->pos.y < 320.0f)) &&
                (60.0f < (bullet->sprites.spriteBullet.sprite)->heightPx))
            {
                local_e4.position = bullet->pos;
                if (iVar1 == 0)
                {
                    local_e4.sprite = 3;
                }
                else
                {
                    local_e4.sprite = 1;
                }
                local_e4.spriteOffset = (-(u16)(bullet->spriteOffset != 2) & 3) + 10;
                local_e4.angle1 = g_Rng.GetRandomFloat() * ZUN_2PI - ZUN_PI;
                local_e4.angle2 = -ZUN_PI;
                if (iVar1 == 0)
                {
                    local_e4.speed1 = 0.8f;
                }
                else
                {
                    local_e4.speed1 = 1.2f;
                }
                local_e4.count1 = 1;
                local_e4.count2 = 1;
                local_e4.flags = 0x208;
                local_e4.aimMode = 3;
                local_e4.soundOverride = SOUND_25;
                if (iVar1 != 0)
                {
                    local_e4.AddDirChangeCommand(0, 0, 0x28, 1, 0.0f, 2.9f);
                }
                g_BulletManager.SpawnBulletPattern(&local_e4);
            }
            bullet = bullet + 1;
        }
    }
}
