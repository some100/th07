#include "BulletManager.hpp"

#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "GameManager.hpp"
#include "Player.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "Supervisor.hpp"
#include "ZunMath.hpp"
#include "utils.hpp"

// GLOBAL: TH07 0x0062f958
BulletManager g_BulletManager;

// GLOBAL: TH07 0x009a9abc
ChainElem g_BulletManagerCalcChain;

// GLOBAL: TH07 0x0062f934
ChainElem g_BulletManagerDrawChain;

// GLOBAL: TH07 0x0049ef40
i32 g_BulletSpriteOffset16Px[16] = {0, 1, 1, 1, 1, 2, 2, 2,
                                    2, 3, 3, 3, 4, 4, 4, 0};

// GLOBAL: TH07 0x0049ef80
i32 g_BulletSpriteOffset32Px[8] = {0, 1, 1, 2, 3, 4};

// GLOBAL: TH07
D3DCOLOR g_BulletColorsArray[28] = {
    0xFF000000, 0xFF303030, 0xFF606060, 0xFF500000, 0xFF900000, 0xFFFF2020,
    0xFF400040, 0xFF800080, 0xFFFF30FF, 0xFF000050, 0xFF000090, 0xFF2020FF,
    0xFF203060, 0xFF304090, 0xFF3080FF, 0xFF005000, 0xFF009000, 0xFF20FF20,
    0xFF206000, 0xFF409010, 0xFF80FF20, 0xFF505000, 0xFF909000, 0xFFFFFF20,
    0xFF603000, 0xFF904010, 0xFFF08020, 0xFFFFFFFF};

// GLOBAL: TH07 0x0049eec8
D3DCOLOR g_DefaultBulletColors[28] = {
    0xFFF0F0F0, 0xFFF0F0F0, 0xFFFFFFFC, 0xFFFFE0E0, 0xFFFFE0E0, 0xFFFFE0E0,
    0xFFFFE0FF, 0xFFFFE0FF, 0xFFFFE0FF, 0xFFE0E0FF, 0xFFE0E0FF, 0xFFE0E0FF,
    0xFFE0FFFF, 0xFFE0FFFF, 0xFFE0FFFF, 0xFFE0FFE0, 0xFFE0FFE0, 0xFFE0FFE0,
    0xFFE0FFE0, 0xFFE0FFE0, 0xFFE0FFE0, 0xFFFFFFE0, 0xFFFFFFE0, 0xFFFFFFE0,
    0xFFFFE0E0, 0xFFFFE0E0, 0xFFFFE0E0, 0xFFFFFFFF};

// GLOBAL: TH07 0x0049ef38
D3DCOLOR *g_BulletColor = g_BulletColorsArray;

// GLOBAL: TH07 0x00495160
BulletTypeInfo g_BulletTypeInfos[11] = {
    {0x200, 0x212, 0x213, 0x214, 0x20f},
    {0x201, 0x215, 0x216, 0x217, 0x210},
    {0x202, 0x215, 0x216, 0x217, 0x210},
    {0x203, 0x215, 0x216, 0x217, 0x210},
    {0x204, 0x215, 0x216, 0x217, 0x210},
    {0x205, 0x215, 0x216, 0x217, 0x210},
    {0x206, 0x215, 0x216, 0x217, 0x210},
    {0x207, 0x218, 0x218, 0x218, 0x211},
    {0x208, 0x218, 0x218, 0x218, 0x211},
    {0x209, 0x218, 0x218, 0x218, 0x211},
    {0x2a8, 0x2aa, 0x2aa, 0x2aa, 0x2a9},
};

// FUNCTION: TH07 0x004232e0
void BulletManager::Initialize()
{
    memset(this, 0, sizeof(BulletManager));
    this->bulletsStart = this->bullets;
    this->bullets[0x400].state = BULLET_END_ARRAY;
    this->itemType = ITEM_POINT_BULLET;
}

// FUNCTION: TH07 0x00423330
BulletManager::BulletManager()
{
    Initialize();
}

// FUNCTION: TH07 0x00423420
BulletTypeSprites::BulletTypeSprites()
{
}

// FUNCTION: TH07 0x00423510
Bullet::Bullet()
{
}

// FUNCTION: TH07 0x004235d0
Laser::Laser()
{
}

// FUNCTION: TH07 0x00423660
void BulletManager::SetActiveSpriteByResolution(AnmVm *sprite,
                                                AnmVm *bulletTypeTemplate,
                                                Bullet *bullet,
                                                i32 spriteOffset)
{
    f32 fVar1;

    if ((i32)sprite->activeSpriteIdx !=
        bulletTypeTemplate->activeSpriteIdx + spriteOffset)
    {
        fVar1 = (bullet->sprites.spriteBullet.sprite)->heightPx;
        if (fVar1 < 16.0f == (fVar1 == 16.0f))
        {
            fVar1 = (bullet->sprites.spriteBullet.sprite)->heightPx;
            if (fVar1 < 32.0f == (fVar1 == 32.0f))
            {
                g_AnmManager->SetActiveSprite(
                    sprite, bulletTypeTemplate->activeSpriteIdx + spriteOffset);
            }
            else
            {
                g_AnmManager->SetActiveSprite(
                    sprite, (i32)bulletTypeTemplate->activeSpriteIdx +
                                g_BulletSpriteOffset32Px[spriteOffset]);
            }
        }
        else
        {
            g_AnmManager->SetActiveSprite(sprite,
                                          (i32)bulletTypeTemplate->activeSpriteIdx +
                                              g_BulletSpriteOffset16Px[spriteOffset]);
        }
    }
}

// FUNCTION: TH07 0x00423730
i32 BulletManager::SpawnSingleBullet(EnemyBulletShooter *bulletProps, f32 x,
                                     f32 y, f32 angle)
{
    f32 bulletAngle;
    Bullet *bullet;
    i32 i;
    f32 bulletSpeed;

    bullet = this->bulletsStart;
    i = 0;
    while ((i < 0x400 && (bullet->state != BULLET_INACTIVE)))
    {
        bullet = bullet + 1;
        if (bullet->state == BULLET_END_ARRAY)
        {
            bullet = this->bullets;
        }
        i++;
    }
    if (i < 0x400)
    {
        bulletAngle = 0.0f;
        if (bulletProps->count2 < 2)
        {
            bulletSpeed = bulletProps->speed1;
        }
        else
        {
            bulletSpeed = bulletProps->speed1 -
                          ((bulletProps->speed1 - bulletProps->speed2) * y) /
                              (f32)(i32)bulletProps->count2;
        }
        switch (bulletProps->aimMode)
        {
        case 0:
        case 1:
            if ((bulletProps->count1 & 1U) == 0)
            {
                bulletAngle = bulletProps->angle2 * 0.5f +
                              (f32)((i32)x / 2) * bulletProps->angle2;
            }
            else
            {
                bulletAngle = (f32)(((i32)x + 1) / 2) * bulletProps->angle2;
            }
            bulletAngle = bulletAngle + 0.0f;
            if (((i32)x & 1U) != 0)
            {
                bulletAngle = bulletAngle * -1.0f;
            }
            if (bulletProps->aimMode == 0)
            {
                bulletAngle = bulletAngle + angle;
            }
            bulletAngle = bulletAngle + bulletProps->angle1;
            break;
        case 2:
            bulletAngle = angle + 0.0f;
        case 3:
            bulletAngle += y * bulletProps->angle2 + bulletProps->angle1 +
                           (x * ZUN_2PI) / (f32)(i32)bulletProps->count1;
            break;
        case 4:
            bulletAngle = angle + 0.0f;
        case 5:
            bulletAngle = (x * ZUN_2PI) / (f32)(i32)bulletProps->count1 +
                          ZUN_PI / (f32)(i32)bulletProps->count1 + bulletAngle +
                          bulletProps->angle1;
            break;
        case 6:
            bulletAngle =
                g_Rng.GetRandomFloatInRange(bulletProps->angle1 - bulletProps->angle2) +
                bulletProps->angle2;
            break;
        case 7:
            bulletSpeed =
                g_Rng.GetRandomFloatInRange(bulletProps->speed1 - bulletProps->speed2) +
                bulletProps->speed2;
            bulletAngle = y * bulletProps->angle2 + bulletProps->angle1 +
                          (x * ZUN_2PI) / (f32)(i32)bulletProps->count1 + 0.0f;
            break;
        case 8:
            bulletAngle =
                g_Rng.GetRandomFloatInRange(bulletProps->angle1 - bulletProps->angle2) +
                bulletProps->angle2;
            bulletSpeed =
                g_Rng.GetRandomFloatInRange(bulletProps->speed1 - bulletProps->speed2) +
                bulletProps->speed2;
        }
        bullet->state = BULLET_NORMAL;
        bullet->spawned = 1;
        bullet->grazed = 0;
        bullet->timer1 = 0;
        bullet->timer2 = 0;
        bullet->speed = bulletSpeed;
        bullet->angle = utils::AddNormalizeAngle(bulletAngle, 0.0f);
        bullet->pos = bulletProps->position;
        bullet->pos.z = 0.1f;
        AngleToVector(&bullet->velocity, bulletAngle,
                      bulletSpeed * g_Supervisor.effectiveFramerateMultiplier);
        bullet->exFlags = (i16)bulletProps->flags;
        bullet->spriteOffset = bulletProps->spriteOffset;
        bullet->state2 = 0;
        if ((bullet->sprites.spriteBullet.anmFileIdx !=
             (bulletProps->sprites->spriteBullet).anmFileIdx) ||
            ((bulletProps->sprites->spriteBullet).currentInstruction != NULL))
        {
            bullet->sprites.spriteBullet = bulletProps->sprites->spriteBullet;
        }
        if ((bullet->sprites.spriteSpawnEffectDonut.anmFileIdx !=
             (bulletProps->sprites->spriteSpawnEffectDonut).anmFileIdx) ||
            ((bulletProps->sprites->spriteSpawnEffectDonut).currentInstruction !=
             NULL))
        {
            bullet->sprites.spriteSpawnEffectDonut =
                bulletProps->sprites->spriteSpawnEffectDonut;
        }
        bullet->sprites.grazeSize = bulletProps->sprites->grazeSize;
        bullet->sprites.unused_b88 = bulletProps->sprites->unused_b88;
        bullet->sprites.bulletHeight = bulletProps->sprites->bulletHeight;
        bullet->sprites.collisionType = bulletProps->sprites->collisionType;
        bullet->soundIdx = bulletProps->soundOverride;
        bullet->spawnDelay = 0;
        if ((i32)bullet->sprites.spriteBullet.activeSpriteIdx !=
            (i32)(bulletProps->sprites->spriteBullet).activeSpriteIdx +
                (i32)bulletProps->spriteOffset)
        {
            g_AnmManager->SetActiveSprite(
                &bullet->sprites.spriteBullet,
                (i32)(bulletProps->sprites->spriteBullet).activeSpriteIdx +
                    (i32)bulletProps->spriteOffset);
        }
        if ((i32)bullet->sprites.spriteSpawnEffectDonut.activeSpriteIdx !=
            (i32)(bulletProps->sprites->spriteSpawnEffectDonut).activeSpriteIdx +
                (i32)bulletProps->spriteOffset)
        {
            if ((bullet->sprites.spriteBullet.sprite)->heightPx > 16.0f)
            {
                if ((bullet->sprites.spriteBullet.sprite)->heightPx > 32.0f)
                {
                    g_AnmManager->SetActiveSprite(
                        &bullet->sprites.spriteSpawnEffectDonut,
                        (i32)(bulletProps->sprites->spriteSpawnEffectDonut)
                                .activeSpriteIdx +
                            (i32)bulletProps->spriteOffset);
                }
                else
                {
                    g_AnmManager->SetActiveSprite(
                        &bullet->sprites.spriteSpawnEffectDonut,
                        (i32)(bulletProps->sprites->spriteSpawnEffectDonut)
                                .activeSpriteIdx +
                            g_BulletSpriteOffset32Px[bulletProps->spriteOffset]);
                }
            }
            else
            {
                g_AnmManager->SetActiveSprite(
                    &bullet->sprites.spriteSpawnEffectDonut,
                    (i32)(bulletProps->sprites->spriteSpawnEffectDonut)
                            .activeSpriteIdx +
                        g_BulletSpriteOffset16Px[bulletProps->spriteOffset]);
            }
        }
        if ((bulletProps->flags & 2) == 0)
        {
            if ((bulletProps->flags & 4) == 0)
            {
                if ((bulletProps->flags & 8) != 0)
                {
                    if ((bullet->sprites.spriteSpawnEffectSlow.anmFileIdx !=
                         (bulletProps->sprites->spriteSpawnEffectSlow).anmFileIdx) ||
                        ((bulletProps->sprites->spriteSpawnEffectSlow)
                             .currentInstruction != NULL))
                    {
                        bullet->sprites.spriteSpawnEffectSlow =
                            bulletProps->sprites->spriteSpawnEffectSlow;
                    }
                    SetActiveSpriteByResolution(
                        &bullet->sprites.spriteSpawnEffectSlow,
                        &bulletProps->sprites->spriteSpawnEffectSlow, bullet,
                        (i32)bulletProps->spriteOffset);
                    bullet->state = BULLET_SPAWNING_SLOW;
                    bullet->pos -= bullet->velocity * 4.0f;
                }
            }
            else
            {
                if ((bullet->sprites.spriteSpawnEffectNormal.anmFileIdx !=
                     (bulletProps->sprites->spriteSpawnEffectNormal).anmFileIdx) ||
                    ((bulletProps->sprites->spriteSpawnEffectNormal)
                         .currentInstruction != NULL))
                {
                    bullet->sprites.spriteSpawnEffectNormal =
                        bulletProps->sprites->spriteSpawnEffectNormal;
                }
                SetActiveSpriteByResolution(
                    &bullet->sprites.spriteSpawnEffectNormal,
                    &bulletProps->sprites->spriteSpawnEffectNormal, bullet,
                    (i32)bulletProps->spriteOffset);
                bullet->state = BULLET_SPAWNING_NORMAL;
                bullet->pos -= bullet->velocity * 4.0f;
            }
        }
        else
        {
            if ((bullet->sprites.spriteSpawnEffectFast.anmFileIdx !=
                 (bulletProps->sprites->spriteSpawnEffectFast).anmFileIdx) ||
                ((bulletProps->sprites->spriteSpawnEffectFast).currentInstruction !=
                 NULL))
            {
                bullet->sprites.spriteSpawnEffectFast =
                    bulletProps->sprites->spriteSpawnEffectFast;
            }
            SetActiveSpriteByResolution(&bullet->sprites.spriteSpawnEffectFast,
                                        &bulletProps->sprites->spriteSpawnEffectFast,
                                        bullet, bulletProps->spriteOffset);
            bullet->state = BULLET_SPAWNING_FAST;
            bullet->pos.x -= bullet->velocity.x * 4.0f;
        }
        memcpy(bullet->commands, bulletProps->commands, sizeof(bullet->commands));
        bullet->moreFlags = bulletProps->flags;
        bullet->exFlags = 0;
        bullet->curCmdIdx = 0;
        bullet->RunCommands();
        if ((this->screenClearTime != 0) && ((bullet->moreFlags & 0x1000) == 0))
        {
            bullet->state = BULLET_DESPAWN;
        }
        if (bullet[1].state == BULLET_END_ARRAY)
        {
            this->bulletsStart = this->bullets;
        }
        else
        {
            this->bulletsStart = bullet + 1;
        }
        return 0;
    }
    else
    {
        return 1;
    }
}

// FUNCTION: TH07 0x00424290
void Bullet::RunCommands()
{
    BulletCommand *pBVar1;
    f32 local_28;
    f32 local_24;

    while (true)
    {
        while (true)
        {
            if (4 < this->curCmdIdx)
            {
                return;
            }
            pBVar1 = &this->commands[this->curCmdIdx];
            if (pBVar1->type == 0)
            {
                return;
            }
            if ((pBVar1->flag == 0) && (this->exFlags != 0))
            {
                return;
            }
            if (((u32)this->moreFlags & pBVar1->type) != 0)
            {
                break;
            }
            this->curCmdIdx = this->curCmdIdx + 1;
        }
        if (pBVar1->type < 0x81)
        {
            break;
        }
        if (pBVar1->type < 0x801)
        {
            if (pBVar1->type != 0x800)
            {
                if (pBVar1->type == 0x100)
                {
                    goto switchD_00424354_caseD_40;
                }
                if (pBVar1->type != 0x400)
                {
                    goto switchD_00424354_caseD_2;
                }
            }
            this->exFlags = this->exFlags | pBVar1->type;
            if (pBVar1->speed < 0.0f)
            {
                this->commandStates[4].speed = this->speed;
            }
            else
            {
                this->commandStates[4].speed = pBVar1->speed;
            }
            this->commandStates[4].maxTimes = pBVar1->duration;
            this->commandStates[4].duration = 0;
            goto switchD_00424354_caseD_2;
        }
        if (pBVar1->type != 0x2000)
        {
            goto switchD_00424354_caseD_2;
        }
        this->spawnDelay = pBVar1->duration;
        this->curCmdIdx = this->curCmdIdx + 1;
    }
    if (pBVar1->type == 0x80)
    {
    switchD_00424354_caseD_40:
        this->exFlags = this->exFlags | (u16)pBVar1->type;
        this->commandStates[3].angle = pBVar1->speed; // Wat
        if (pBVar1->angle <= -999.0f)
        {
            local_28 = this->speed;
        }
        else
        {
            local_28 = pBVar1->angle;
        }
        this->commandStates[3].speed = local_28;
        this->commandStates[3].timer = 0;
        this->commandStates[3].duration = pBVar1->duration;
        this->commandStates[3].maxTimes = pBVar1->loopCount;
        this->commandStates[3].minTimes = 0;
    }
    else
    {
        switch (pBVar1->type)
        {
        case 1:
            this->exFlags = this->exFlags | 1;
            this->commandStates[0].timer = 0;
            this->commandStates[0].vec3.z = 0.0f;
            break;
        case 0x10:
            this->exFlags = this->exFlags | 0x10;
            this->commandStates[1].speed = pBVar1->speed;
            if (pBVar1->angle <= -990.0f)
            {
                local_24 = this->angle;
            }
            else
            {
                local_24 = pBVar1->angle;
            }
            this->commandStates[1].angle = local_24;
            this->commandStates[1].timer = 0;
            this->commandStates[1].duration = pBVar1->duration;
            AngleToVector(&this->commandStates[1].vec3, this->commandStates[1].angle,
                          g_Supervisor.effectiveFramerateMultiplier *
                              this->commandStates[1].speed);
            if ((this->curCmdIdx != 0) && (-1 < this->soundIdx))
            {
                g_SoundPlayer.PlaySoundByIdx(this->soundIdx, 0);
            }
            break;
        case 0x20:
            this->exFlags = this->exFlags | 0x20;
            this->commandStates[2].speed = pBVar1->speed;
            this->commandStates[2].angle = pBVar1->angle;
            this->commandStates[2].timer = 0;
            this->commandStates[2].duration = pBVar1->duration;
            if ((this->curCmdIdx != 0) && (-1 < this->soundIdx))
            {
                g_SoundPlayer.PlaySoundByIdx(this->soundIdx, 0);
            }
            break;
        case 0x40:
            goto switchD_00424354_caseD_40;
        }
    }
switchD_00424354_caseD_2:
    this->curCmdIdx = this->curCmdIdx + 1;
}

// FUNCTION: TH07 0x00424740
void BulletManager::RemoveAllBullets(i32 param_1)
{
    f32 local_28;
    f32 local_24;
    Laser *laser;
    Bullet *bullet;
    f32 local_18;
    i32 i;
    D3DXVECTOR3 local_10;

    bullet = g_BulletManager.bullets;
    for (i = 0; i < 0x400; i++)
    {
        if ((bullet->state != BULLET_INACTIVE) &&
            (bullet->state != BULLET_DESPAWN))
        {
            if ((param_1 == 0) || (8 < param_1))
            {
                bullet->state = BULLET_DESPAWN;
            }
            else
            {
                if (param_1 < 3)
                {
                    g_ItemManager.SpawnItem(&bullet->pos, this->itemType, param_1);
                }
                else
                {
                    g_ItemManager.SpawnItem(&bullet->pos, ITEM_STAR, 1);
                }
                memset(bullet, 0, sizeof(Bullet));
            }
        }
        bullet = bullet + 1;
    }
    laser = this->lasers;
    for (i = 0; i < 0x40; i++)
    {
        if ((laser->inUse != 0) && (((laser->flags & 4) == 0 || (param_1 == 10))))
        {
            if (laser->state < 2)
            {
                laser->state = 2;
                laser->timer = 0;
                laser->width = laser->targetWidth;
                if ((param_1 != 0) && (param_1 < 9))
                {
                    local_28 = laser->startOffset;
                    sincosf(&local_18, &local_24, laser->angle);
                    while (local_28 < laser->endOffset)
                    {
                        local_10.x = local_24 * local_28 + (laser->pos).x;
                        local_10.y = local_18 * local_28 + (laser->pos).y;
                        local_10.z = 0.0f;
                        if (param_1 < 3)
                        {
                            g_ItemManager.SpawnItem(&local_10, this->itemType, param_1);
                        }
                        else
                        {
                            g_ItemManager.SpawnItem(&local_10, ITEM_STAR, 1);
                        }
                        local_28 += 32.0f;
                    }
                }
            }
            laser->grazeInterval = 0;
        }
        laser = laser + 1;
    }
    this->screenClearTime = 10;
}

// FUNCTION: TH07 0x004249a0
i32 BulletManager::DespawnBullets(i32 param_1, i32 turnIntoItem)
{
    f32 local_34;
    f32 local_30;
    Laser *laser;
    D3DXVECTOR3 local_28;
    Bullet *bullet;
    f32 local_18;
    i32 i;
    i32 local_10;
    i32 local_c;
    i32 local_8;

    local_c = 0;
    local_8 = 2000;
    local_10 = 0;
    bullet = g_BulletManager.bullets;
    for (i = 0; i < 0x400; i++)
    {
        if (bullet->state != BULLET_INACTIVE)
        {
            g_ItemManager.SpawnItem(&bullet->pos, this->itemType, 1);
            g_AsciiManager.CreatePopup1(&bullet->pos, local_8,
                                        ((local_8 < param_1) - 1 & 0xffffff01) - 1);
            local_c += local_8;
            local_10 += 1;
            local_8 += 20;
            if (param_1 < local_8)
            {
                local_8 = param_1;
            }
            bullet->state = BULLET_DESPAWN;
        }
        bullet = bullet + 1;
    }
    laser = this->lasers;
    for (i = 0; i < 0x40; i++)
    {
        if (laser->inUse != 0)
        {
            if (laser->state < 2)
            {
                laser->state = 2;
                laser->timer = 0;
                laser->width = laser->targetWidth;
                if (turnIntoItem != 0)
                {
                    g_ItemManager.SpawnItem(&laser->pos, this->itemType, 1);
                    local_34 = laser->startOffset;
                    sincosf(&local_18, &local_30, laser->angle);
                    while (local_34 < laser->endOffset)
                    {
                        local_28.x = local_30 * local_34 + (laser->pos).x;
                        local_28.y = local_18 * local_34 + (laser->pos).y;
                        local_28.z = 0.0f;
                        g_ItemManager.SpawnItem(&local_28, this->itemType, 1);
                        local_34 += 32.0f;
                    }
                }
            }
            laser->grazeInterval = 0;
        }
        laser = laser + 1;
    }
    this->screenClearTime = 10;
    return local_c;
}

#pragma var_order(diff, i, bullet)
// FUNCTION: TH07 0x00424c00
void BulletManager::RemoveBulletsInRadius(D3DXVECTOR3 *centerPos, f32 radius)
{
    D3DXVECTOR3 diff;
    Bullet *bullet;
    i32 i;

    bullet = g_BulletManager.bullets;
    radius *= radius;
    for (i = 0; i < 0x400; i++, bullet++)
    {
        if (bullet->state == BULLET_INACTIVE || bullet->state == BULLET_DESPAWN)
        {
            continue;
        }

        diff = bullet->pos - *centerPos;

        if (D3DXVec3LengthSq(&diff) > radius)
        {
            continue;
        }

        g_ItemManager.SpawnItem(&bullet->pos, ITEM_POINT_BULLET, 1);
        memset(bullet, 0, sizeof(Bullet));
    }
}

// FUNCTION: TH07 0x00424d20
i32 BulletManager::SpawnBulletPattern(EnemyBulletShooter *bulletProps)
{
    f32 angle;
    i32 local_10;
    i32 local_8;

    if (g_BulletManager.bulletCount < 0x400)
    {
        bulletProps->sprites = this->bulletTypeTemplates + (i16)bulletProps->sprite;
        angle = g_Player.AngleToPlayer(&bulletProps->position);
        for (local_10 = 0; local_10 < bulletProps->count2; local_10 += 1)
        {
            for (local_8 = 0; local_8 < bulletProps->count1; local_8 += 1)
            {
                if (SpawnSingleBullet(bulletProps, local_8, local_10, angle) != 0)
                {
                    goto LAB_00424dce;
                }
            }
        }
    LAB_00424dce:
        if ((bulletProps->flags & 0x200) != 0)
        {
            g_SoundPlayer.PlaySoundByIdx(bulletProps->soundIdx, 0);
        }
    }
    return 0;
}

#pragma var_order(i, laser)
// FUNCTION: TH07 0x00424e00
Laser *BulletManager::SpawnLaserPattern(EnemyLaserShooter *laserShooter)
{
    Laser *laser;
    i32 i;

    laser = this->lasers;
    if (this->screenClearTime != 0 && (laserShooter->flags & 4) == 0)
    {
        return laser;
    }

    for (i = 0; i < 0x40; i++, laser++)
    {
        if (laser->inUse != 0)
        {
            continue;
        }

        g_AnmManager->SetAnmIdxAndExecuteScript(&laser->vm0, laserShooter->sprite + 0x20a);
        g_AnmManager->SetActiveSprite(&laser->vm0,
                                      (i32)laser->vm0.activeSpriteIdx +
                                          (i32)laserShooter->spriteOffset);
        g_AnmManager->InitializeAndSetActiveSprite(&laser->vm1, g_BulletSpriteOffset16Px[laserShooter->spriteOffset] + 0x292);
        laser->vm1.blendMode = 1;
        laser->pos = laserShooter->position;
        laser->color = laserShooter->spriteOffset;
        laser->inUse = 1;
        laser->angle = laserShooter->angle1;
        if (laserShooter->type == 0)
        {
            laser->angle =
                g_Player.AngleToPlayer(&laserShooter->position) + laser->angle;
        }
        laser->flags = laserShooter->flags;
        laser->timer.InitializeForPopup();
        laser->startOffset = laserShooter->startOffset;
        laser->endOffset = laserShooter->endOffset;
        laser->startLength = laserShooter->startLength;
        laser->width = laserShooter->width;
        laser->speed = laserShooter->speed1;
        laser->startTime = laserShooter->startTime;
        laser->duration = laserShooter->duration;
        laser->endTime = laserShooter->endTime;
        laser->grazeDelay = laserShooter->grazeDelay;
        laser->grazeInterval = laserShooter->aimMode;
        laser->hideWarning = 0;
        if (laser->startTime == 0)
        {
            laser->state = 1;
            break;
        }
        laser->state = 0;
        break;
    }
    return laser;
}

// FUNCTION: TH07 0x004250d0
void Bullet::UpdateBulletBurstSpeed()
{
    if (this->commandStates[0].timer < 0x11)
    {
        AngleToVector(&this->velocity, this->angle,
                      ((5.0f - (((f32)this->commandStates[0].timer.current +
                                 this->commandStates[0].timer.subFrame) *
                                5.0f) /
                                   16.0f) +
                       this->speed) *
                          g_Supervisor.effectiveFramerateMultiplier);
    }
    else
    {
        this->exFlags = this->exFlags ^ 1;
    }
    this->commandStates[0].timer++;
}

// FUNCTION: TH07 0x004251a0
void Bullet::UpdateBulletTargetVelocity()
{
    if (this->commandStates[1].timer < this->commandStates[1].duration)
    {
        (this->velocity).x = g_Supervisor.effectiveFramerateMultiplier *
                                 this->commandStates[1].vec3.x +
                             (this->velocity).x;
        (this->velocity).y = g_Supervisor.effectiveFramerateMultiplier *
                                 this->commandStates[1].vec3.y +
                             (this->velocity).y;
        (this->velocity).z = g_Supervisor.effectiveFramerateMultiplier *
                                 this->commandStates[1].vec3.z +
                             (this->velocity).z;
        if (fabsf((this->velocity).x) <= 0.0001f)
        {
            if (fabsf((this->velocity).y) <= 0.0001f)
            {
                goto LAB_004252d1;
            }
        }
        this->angle = atan2f((this->velocity).y, (this->velocity).x);
    }
    else
    {
        this->exFlags = this->exFlags & 0xffef;
    }
LAB_004252d1:
    this->commandStates[1].timer++;
}

// FUNCTION: TH07 0x00425310
void Bullet::UpdateBulletTargetAngle()
{
    if (this->commandStates[2].timer < this->commandStates[2].duration)
    {
        this->angle = utils::AddNormalizeAngle(
            this->angle, g_Supervisor.effectiveFramerateMultiplier *
                             this->commandStates[2].angle);
        this->speed = g_Supervisor.effectiveFramerateMultiplier *
                          this->commandStates[2].speed +
                      this->speed;
        AngleToVector(&this->velocity, this->angle,
                      g_Supervisor.effectiveFramerateMultiplier * this->speed);
    }
    else
    {
        this->exFlags = this->exFlags & 0xffdf;
    }
    this->commandStates[2].timer++;
}

// FUNCTION: TH07 0x00425400
void Bullet::UpdateBulletDirChangeAndResume()
{
    f32 local_8;

    if (this->commandStates[3].timer < this->commandStates[3].duration)
    {
        local_8 = this->speed - (((f32)this->commandStates[3].timer.current +
                                  this->commandStates[3].timer.subFrame) *
                                 this->speed) /
                                    (f32)this->commandStates[3].duration;
    }
    else
    {
        if (-1 < this->soundIdx)
        {
            g_SoundPlayer.PlaySoundByIdx(this->soundIdx, 0);
        }
        this->commandStates[3].minTimes += 1;
        if (this->commandStates[3].maxTimes <= this->commandStates[3].minTimes)
        {
            this->exFlags = this->exFlags & 0xffbf;
        }
        this->angle = this->angle + this->commandStates[3].angle;
        this->speed = this->commandStates[3].speed;
        local_8 = this->speed;
        this->commandStates[3].timer = 0;
    }
    AngleToVector(&this->velocity, this->angle,
                  local_8 * g_Supervisor.effectiveFramerateMultiplier);
    this->commandStates[3].timer++;
}

// FUNCTION: TH07 0x00425580
void Bullet::UpdateBulletDirChangeAbsoluteAndResume()
{
    f32 local_8;

    if (this->commandStates[3].timer < this->commandStates[3].duration)
    {
        local_8 = this->speed - (((f32)this->commandStates[3].timer.current +
                                  this->commandStates[3].timer.subFrame) *
                                 this->speed) /
                                    (f32)this->commandStates[3].duration;
    }
    else
    {
        if (-1 < this->soundIdx)
        {
            g_SoundPlayer.PlaySoundByIdx(this->soundIdx, 0);
        }
        this->commandStates[3].minTimes += 1;
        if (this->commandStates[3].maxTimes <= this->commandStates[3].minTimes)
        {
            this->exFlags = this->exFlags & 0xfeff;
        }
        this->angle = this->commandStates[3].angle;
        this->speed = this->commandStates[3].speed;
        local_8 = this->speed;
        this->commandStates[3].timer = 0;
    }
    AngleToVector(&this->velocity, this->angle,
                  local_8 * g_Supervisor.effectiveFramerateMultiplier);
    this->commandStates[3].timer++;
}

// FUNCTION: TH07 0x00425700
void Bullet::UpdateBulletDirChangeAimAtPlayer()
{
    f32 local_8;

    if (this->commandStates[3].timer < this->commandStates[3].duration)
    {
        local_8 = this->speed - (((f32)this->commandStates[3].timer.current +
                                  this->commandStates[3].timer.subFrame) *
                                 this->speed) /
                                    (f32)this->commandStates[3].duration;
    }
    else
    {
        if (-1 < this->soundIdx)
        {
            g_SoundPlayer.PlaySoundByIdx(this->soundIdx, 0);
        }
        this->commandStates[3].minTimes = this->commandStates[3].minTimes + 1;
        if (this->commandStates[3].maxTimes <= this->commandStates[3].minTimes)
        {
            this->exFlags = this->exFlags & 0xff7f;
        }
        this->angle = utils::AddNormalizeAngle(g_Player.AngleToPlayer(&this->pos),
                                               this->commandStates[3].angle);
        this->speed = this->commandStates[3].speed;
        local_8 = this->speed;
        this->commandStates[3].timer = 0;
    }
    AngleToVector(&this->velocity, this->angle,
                  local_8 * g_Supervisor.effectiveFramerateMultiplier);
    this->commandStates[3].timer++;
}

// FUNCTION: TH07 0x004258a0
void Bullet::UpdateBulletBounce()
{
    if (g_GameManager.IsInBounds((this->pos).x, (this->pos).y,
                                 (this->sprites.spriteBullet.sprite)->widthPx,
                                 (this->sprites.spriteBullet.sprite)->heightPx) ==
        0)
    {
        if (-1 < this->soundIdx)
        {
            g_SoundPlayer.PlaySoundByIdx(this->soundIdx, 0);
        }
        if (((this->pos).x < 0.0f) || (384.0f <= (this->pos).x))
        {
            this->angle = -this->angle - ZUN_PI;
            this->angle = utils::AddNormalizeAngle(this->angle, 0.0f);
        }
        if (((this->pos).y < 0.0f) ||
            ((448.0f <= (this->pos).y && ((this->exFlags & 0x400U) != 0))))
        {
            this->angle = -this->angle;
        }
        this->speed = this->commandStates[4].speed;
        AngleToVector(&this->velocity, this->angle,
                      this->speed * g_Supervisor.effectiveFramerateMultiplier);
        this->commandStates[4].duration = this->commandStates[4].duration + 1;
        if (this->commandStates[4].maxTimes <= this->commandStates[4].duration)
        {
            this->exFlags = this->exFlags & 0xf3ff;
        }
    }
}

// FUNCTION: TH07 0x00425a50
u32 BulletManager::OnUpdate(BulletManager *arg)
{
    i32 iVar3;
    f32 fVar4;
    i32 local_140;
    D3DXVECTOR3 local_38;
    Laser *laser;
    u32 local_28;
    Bullet *bullets;
    D3DXVECTOR3 local_20;
    i32 local_14;
    f32 local_10;
    i32 i;

    local_14 = 0;
    bullets = arg->bullets;
    if (g_GameManager.isTimeStopped == 0)
    {
        g_ItemManager.OnUpdate();
        arg->bulletCount = 0;
        arg->bulletsHeadPtrs[5] = NULL;
        arg->bulletsHeadPtrs[4] = NULL;
        arg->bulletsHeadPtrs[3] = NULL;
        arg->bulletsHeadPtrs[2] = NULL;
        arg->bulletsHeadPtrs[1] = NULL;
        arg->bulletsHeadPtrs[0] = NULL;
        for (i = 0; i < 0x400; i++)
        {
            if (bullets->state == BULLET_INACTIVE)
            {
                goto bulletLoopContinue;
            }
            arg->bulletCount = arg->bulletCount + 1;
            switch (bullets->state)
            {
            case BULLET_NORMAL:
                goto switchD_00425b82_caseD_1;
            case BULLET_SPAWNING_FAST:
                bullets->timer2--;
                bullets->pos += bullets->velocity * 0.5f;
                iVar3 = g_AnmManager->ExecuteScript(
                    &(bullets->sprites).spriteSpawnEffectFast);
                break;
            case BULLET_SPAWNING_NORMAL:
                bullets->timer2--;
                bullets->pos += bullets->velocity * 0.4f;
                iVar3 = g_AnmManager->ExecuteScript(
                    &(bullets->sprites).spriteSpawnEffectNormal);
                break;
            case BULLET_SPAWNING_SLOW:
                bullets->timer2--;
                bullets->pos = bullets->velocity * 0.33333334f + bullets->pos;
                iVar3 = g_AnmManager->ExecuteScript(
                    &(bullets->sprites).spriteSpawnEffectSlow);
                break;
            case BULLET_DESPAWN:
                bullets->pos += bullets->velocity * 0.5f;
                iVar3 = g_AnmManager->ExecuteScript(
                    &(bullets->sprites).spriteSpawnEffectDonut);
                if (iVar3 != 0)
                {
                    bullets->Initialize();
                    goto bulletLoopContinue;
                }
            default:
                goto switchD_00425b82_default;
            }
            if (iVar3 == 0)
            {
            switchD_00425b82_default:
                bullets->timer1++;
                bullets->timer2++;
                bullets->next = arg->bulletsHeadPtrs[(bullets->sprites).collisionType];
                arg->bulletsHeadPtrs[(bullets->sprites).collisionType] = bullets;
            }
            else
            {
                bullets->state = BULLET_NORMAL;
                bullets->timer1.Initialize(0);
            switchD_00425b82_caseD_1:
                bullets->RunCommands();
                if (bullets->exFlags != 0)
                {
                    if ((bullets->exFlags & 1U) != 0)
                    {
                        bullets->UpdateBulletBurstSpeed();
                    }
                    if ((bullets->exFlags & 0x10U) != 0)
                    {
                        bullets->UpdateBulletTargetVelocity();
                    }
                    if ((bullets->exFlags & 0x20U) != 0)
                    {
                        bullets->UpdateBulletTargetAngle();
                    }
                    if ((bullets->exFlags & 0x40U) != 0)
                    {
                        bullets->UpdateBulletDirChangeAndResume();
                    }
                    if ((bullets->exFlags & 0x100U) != 0)
                    {
                        bullets->UpdateBulletDirChangeAbsoluteAndResume();
                    }
                    if ((bullets->exFlags & 0x80U) != 0)
                    {
                        bullets->UpdateBulletDirChangeAimAtPlayer();
                    }
                    if ((bullets->exFlags & 0xc00U) != 0)
                    {
                        bullets->UpdateBulletBounce();
                    }
                }
                if (bullets->spawnDelay != 0)
                {
                    bullets->spawnDelay = bullets->spawnDelay - 1;
                }
                bullets->pos += bullets->velocity;
                if (bullets->spawnDelay != 0)
                {
                LAB_00425dd7:
                    if ((bullets->grazed == 0) && (0xf < (bullets->timer2).current))
                    {
                        iVar3 = g_Player.CheckGraze(&bullets->pos,
                                                    &(bullets->sprites).grazeSize);
                        if (iVar3 == 1)
                        {
                            bullets->grazed = 1;
                            goto LAB_00425e76;
                        }
                        if ((iVar3 == 2) && ((bullets->moreFlags & 0x1000) == 0))
                        {
                            bullets->state = BULLET_DESPAWN;
                            g_ItemManager.SpawnItem(&bullets->pos, g_Player.itemType, 1);
                        }
                    }
                    else
                    {
                    LAB_00425e76:
                        iVar3 = g_Player.CalcKillboxCollision(
                            &bullets->pos, &(bullets->sprites).grazeSize);
                        if ((iVar3 != 0) &&
                            (((iVar3 != 2 || ((bullets->moreFlags & 0x1000) == 0)) &&
                              (bullets->state = BULLET_DESPAWN, iVar3 == 2))))
                        {
                            g_ItemManager.SpawnItem(&bullets->pos, g_Player.itemType, 1);
                        }
                    }
                    if ((bullets->sprites).spriteBullet.currentInstruction != NULL)
                    {
                        g_AnmManager->ExecuteScript(&bullets->sprites.spriteBullet);
                    }
                    goto switchD_00425b82_default;
                }
                if (g_GameManager.IsInBounds(
                        bullets->pos.x, bullets->pos.y,
                        ((bullets->sprites).spriteBullet.sprite)->widthPx,
                        ((bullets->sprites).spriteBullet.sprite)->heightPx) != 0)
                {
                    bullets->outOfBoundsTime = 0;
                    goto LAB_00425dd7;
                }
                if ((bullets->exFlags & 0xdc0U) == 0)
                {
                    if (bullets->outOfBoundsTime != 0)
                    {
                        bullets->outOfBoundsTime = bullets->outOfBoundsTime - 1;
                        goto LAB_00425dd7;
                    }
                    bullets->Initialize();
                }
                else
                {
                    bullets->outOfBoundsTime = bullets->outOfBoundsTime + 1;
                    if (bullets->outOfBoundsTime < 0x80)
                    {
                        goto LAB_00425dd7;
                    }
                    bullets->Initialize();
                }
            }
        bulletLoopContinue:
            local_14 += -1;
            if (local_14 < 0)
            {
                local_14 = 0x3ff;
                bullets = bullets + 0x400;
            }
            bullets = bullets - 1;
        }
        laser = arg->lasers;
        for (i = 0; i < 0x40; i++)
        {
            if (laser->inUse != 0)
            {
                laser->endOffset =
                    g_Supervisor.effectiveFramerateMultiplier * laser->speed +
                    laser->endOffset;
                if (laser->startLength < laser->endOffset - laser->startOffset)
                {
                    laser->startOffset = laser->endOffset - laser->startLength;
                }
                if (laser->startOffset < 0.0f)
                {
                    laser->startOffset = 0.0f;
                }
                local_20.y = laser->width / 2.0f;
                local_20.x = laser->endOffset - laser->startOffset;
                local_38.x = (laser->endOffset - laser->startOffset) / 2.0f +
                             laser->startOffset + (laser->pos).x;
                local_38.y = (laser->pos).y;
                laser->vm0.scale.x = laser->width / (laser->vm0.sprite)->widthPx;
                laser->vm0.scale.y = (laser->endOffset - laser->startOffset) /
                                     (laser->vm0.sprite)->heightPx;
                laser->vm0.rotation.z =
                    utils::AddNormalizeAngle(laser->angle + 1.5707964f, 0.0f);
                laser->vm0.updateRotation = 1;
                if (laser->state == 0)
                {
                    if ((laser->flags & 1) == 0)
                    {
                        if (laser->startTime < 0x1f)
                        {
                            local_140 = laser->startTime;
                        }
                        else
                        {
                            local_140 = 0x1e;
                        }
                        if (laser->startTime - local_140 < laser->timer.current)
                        {
                            local_10 = (((f32)laser->timer.current + laser->timer.subFrame) *
                                        laser->width) /
                                       (f32)laser->startTime;
                        }
                        else
                        {
                            local_10 = 1.2f;
                        }
                        laser->targetWidth = local_10;
                        laser->vm0.scale.x = local_10 / 16.0f;
                        local_20.x = local_10 / 2.0f;
                    }
                    else
                    {
                        local_28 =
                            (((f32)laser->timer.current + laser->timer.subFrame) * 255.0f) /
                            (f32)laser->startTime;
                        if (0xff < (i32)local_28)
                        {
                            local_28 = 0xff;
                        }
                        laser->vm0.color.color = local_28 << 0x18;
                    }
                    if (laser->grazeDelay <= laser->timer.current)
                    {
                        g_Player.CalcLaserHitbox(&local_38, &local_20, &laser->pos,
                                                 laser->angle,
                                                 (laser->timer.current % 0xc == 0));
                    }
                    if (laser->startTime <= laser->timer.current)
                    {
                        laser->timer = 0;
                        laser->state = laser->state + 1;
                        laser->targetWidth = laser->width;
                        goto LAB_004267a8;
                    }
                }
                else
                {
                    if (laser->state == 1)
                    {
                    LAB_004267a8:
                        g_Player.CalcLaserHitbox(&local_38, &local_20, &laser->pos,
                                                 laser->angle,
                                                 (laser->timer.current % 0xc == 0));
                        if (laser->timer < laser->duration)
                        {
                            goto LAB_004269d9;
                        }
                        laser->timer = 0;
                        laser->state = laser->state + 1;
                        if (laser->endTime == 0)
                        {
                            laser->inUse = 0;
                            goto LAB_004263d8;
                        }
                    }
                    else if (laser->state != 2)
                    {
                        goto LAB_004269d9;
                    }
                    if ((laser->flags & 1) == 0)
                    {
                        if (0 < laser->endTime)
                        {
                            fVar4 = laser->width -
                                    (((f32)laser->timer.current + laser->timer.subFrame) *
                                     laser->width) /
                                        (f32)laser->endTime;
                            laser->vm0.scale.x = fVar4 / 16.0f;
                            local_20.x = fVar4 / 2.0f;
                        }
                    }
                    else
                    {
                        local_28 =
                            (((f32)laser->timer.current + laser->timer.subFrame) * 255.0f) /
                            (f32)laser->startTime;
                        if (0xff < (i32)local_28)
                        {
                            local_28 = 0xff;
                        }
                        laser->vm0.color.color = local_28 << 0x18;
                    }
                    if (laser->timer < laser->grazeInterval)
                    {
                        g_Player.CalcLaserHitbox(&local_38, &local_20, &laser->pos,
                                                 laser->angle,
                                                 (laser->timer.current % 0xc == 0));
                    }
                    if (laser->endTime <= laser->timer.current)
                    {
                        laser->inUse = 0;
                        goto LAB_004263d8;
                    }
                }
            LAB_004269d9:
                if (640.0f <= laser->startOffset)
                {
                    laser->inUse = 0;
                }
                laser->timer++;
                g_AnmManager->ExecuteScript(&laser->vm0);
            }
        LAB_004263d8:
            laser = laser + 1;
        }
        if (arg->screenClearTime != 0)
        {
            arg->screenClearTime = arg->screenClearTime - 1;
        }
        arg->time++;
        arg->updateCount = arg->updateCount + 1;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00426b00
void Bullet::Draw()
{
    AnmVm *vm = &this->sprites.spriteBullet;
    switch (this->state)
    {
    case BULLET_SPAWNING_FAST:
        vm = &this->sprites.spriteSpawnEffectFast;
        break;
    case BULLET_SPAWNING_NORMAL:
        vm = &this->sprites.spriteSpawnEffectNormal;
        break;
    case BULLET_SPAWNING_SLOW:
        vm = &this->sprites.spriteSpawnEffectSlow;
        break;
    case BULLET_DESPAWN:
        vm = &this->sprites.spriteSpawnEffectDonut;
    }
    vm->pos.x = g_GameManager.arcadeRegionTopLeftPos.x + this->pos.x;
    vm->pos.y = g_GameManager.arcadeRegionTopLeftPos.y + this->pos.y;
    vm->pos.z = 0.05f;
    vm->color.color = (vm->color.color & 0xff000000) | 0xffffff;
    if (vm->autoRotate != 0)
    {
        vm->rotation.z = utils::AddNormalizeAngle(this->angle + 1.5707964f, 0.0f);
        vm->updateRotation = 1;
    }
    g_AnmManager->Draw(vm);
}

// FUNCTION: TH07 0x00426c40
u32 BulletManager::OnDraw(BulletManager *arg)
{
    i32 i;
    f32 local_18;
    f32 local_14;
    f32 local_c;

    Laser *laser = arg->lasers;
    for (i = 0; i < 0x40; i++)
    {
        if (laser->inUse != 0)
        {
            sincosf(&local_c, &local_18, laser->angle);
            local_14 =
                (laser->endOffset - laser->startOffset) / 2.0f + laser->startOffset;
            laser->vm0.pos.x = local_18 * local_14 + (laser->pos).x;
            laser->vm0.pos.y = local_c * local_14 + (laser->pos).y;
            laser->vm0.pos.z = 0.05f;
            laser->color = -1;
            laser->vm0.pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
            laser->vm0.pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
            g_AnmManager->Draw(&laser->vm0);
            if (((laser->startOffset < 16.0f) || (laser->speed == 0.0f)) &&
                ((laser->hideWarning == 0 || (laser->state != 0))))
            {
                laser->vm1.pos.x = local_18 * laser->startOffset + (laser->pos).x;
                laser->vm1.pos.y = local_c * laser->startOffset + (laser->pos).y;
                laser->vm1.pos.z = 0.05f;
                laser->vm1.color = laser->vm0.color;
                laser->vm1.flag6 = 1;
                laser->vm1.color.color =
                    (laser->vm1.color.color & 0xffffff) | 0xff000000;
                laser->vm1.scale.x =
                    ((16.0f - laser->startOffset) / 16.0f) * (laser->width / 10.0f);
                laser->vm1.scale.y = laser->vm1.scale.x;
                if (laser->vm1.scale.y > 0.0f)
                {
                    laser->vm1.scale.x = laser->width / 10.0f;
                    laser->vm1.scale.y = laser->vm1.scale.x;
                }
                laser->vm1.pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
                laser->vm1.pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
                g_AnmManager->Draw(&laser->vm1);
            }
        }
        laser = laser + 1;
    }
    g_ItemManager.OnDraw();
    for (i = 0; i < 6; i++)
    {
        for (Bullet *bullet = arg->bulletsHeadPtrs[i]; bullet != NULL;
             bullet = bullet->next)
        {
            bullet->Draw();
        }
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00426f60
ZunResult BulletManager::AddedCallback(BulletManager *arg)
{
    u32 i;

    if ((u32)(g_Supervisor.curState != 3 && g_Supervisor.curState != 11 &&
              g_Supervisor.curState != 12))
    {
        if (g_AnmManager->LoadAnms(0xb, "data/etama.anm", 0x200) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
    }

    for (i = 0; i < 0xb; i++)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(
            &arg->bulletTypeTemplates[i].spriteBullet,
            g_BulletTypeInfos[i].anmFileIdx);
        g_AnmManager->SetAnmIdxAndExecuteScript(
            &arg->bulletTypeTemplates[i].spriteSpawnEffectFast,
            g_BulletTypeInfos[i].spawnFastIdx);
        g_AnmManager->SetAnmIdxAndExecuteScript(
            &arg->bulletTypeTemplates[i].spriteSpawnEffectNormal,
            g_BulletTypeInfos[i].spawnNormalIdx);
        g_AnmManager->SetAnmIdxAndExecuteScript(
            &arg->bulletTypeTemplates[i].spriteSpawnEffectSlow,
            g_BulletTypeInfos[i].spawnSlowIdx);
        g_AnmManager->SetAnmIdxAndExecuteScript(
            &arg->bulletTypeTemplates[i].spriteSpawnEffectDonut,
            g_BulletTypeInfos[i].spawnDonutIdx);
        arg->bulletTypeTemplates[i].spriteBullet.zWriteDisable = 1;
        arg->bulletTypeTemplates[i].spriteSpawnEffectFast.zWriteDisable = 1;
        arg->bulletTypeTemplates[i].spriteSpawnEffectNormal.zWriteDisable = 1;
        arg->bulletTypeTemplates[i].spriteSpawnEffectSlow.zWriteDisable = 1;
        arg->bulletTypeTemplates[i].spriteSpawnEffectDonut.zWriteDisable = 1;
        arg->bulletTypeTemplates[i].spriteBullet.baseSpriteIdx =
            arg->bulletTypeTemplates[i].spriteBullet.activeSpriteIdx;
        arg->bulletTypeTemplates[i].bulletHeight =
            (u8)(arg->bulletTypeTemplates[i].spriteBullet.sprite)->heightPx;
        if (arg->bulletTypeTemplates[i].spriteBullet.sprite->heightPx <= 8.0f)
        {
            arg->bulletTypeTemplates[i].grazeSize.x = 4.0f;
            arg->bulletTypeTemplates[i].grazeSize.y = 4.0f;
            arg->bulletTypeTemplates[i].collisionType = 5;
        }
        else
        {
            if (arg->bulletTypeTemplates[i].spriteBullet.sprite->heightPx <= 16.0f)
            {
                switch (g_BulletTypeInfos[i].anmFileIdx)
                {
                case 0x202:
                    arg->bulletTypeTemplates[i].grazeSize.x = 4.0f;
                    arg->bulletTypeTemplates[i].grazeSize.y = 4.0f;
                    arg->bulletTypeTemplates[i].collisionType = 4;
                    break;
                case 0x204:
                case 0x206:
                    arg->bulletTypeTemplates[i].grazeSize.x = 4.0f;
                    arg->bulletTypeTemplates[i].grazeSize.y = 4.0f;
                    arg->bulletTypeTemplates[i].collisionType = 4;
                    break;
                case 0x205:
                    arg->bulletTypeTemplates[i].grazeSize.x = 4.0f;
                    arg->bulletTypeTemplates[i].grazeSize.y = 4.0f;
                    arg->bulletTypeTemplates[i].collisionType = 4;
                    break;
                default:
                    arg->bulletTypeTemplates[i].grazeSize.x = 6.0f;
                    arg->bulletTypeTemplates[i].grazeSize.y = 6.0f;
                    arg->bulletTypeTemplates[i].collisionType = 3;
                }
            }
            else
            {
                if (arg->bulletTypeTemplates[i].spriteBullet.sprite->heightPx <=
                    32.0f)
                {
                    switch (g_BulletTypeInfos[i].anmFileIdx)
                    {
                    case 0x208:
                        arg->bulletTypeTemplates[i].grazeSize.x = 5.0f;
                        arg->bulletTypeTemplates[i].grazeSize.y = 5.0f;
                        arg->bulletTypeTemplates[i].collisionType = 1;
                        break;
                    case 0x209:
                        arg->bulletTypeTemplates[i].grazeSize.x = 8.0f;
                        arg->bulletTypeTemplates[i].grazeSize.y = 8.0f;
                        arg->bulletTypeTemplates[i].collisionType = 2;
                        break;
                    default:
                        arg->bulletTypeTemplates[i].grazeSize.x = 10.0f;
                        arg->bulletTypeTemplates[i].grazeSize.y = 10.0f;
                        arg->bulletTypeTemplates[i].collisionType = 2;
                    }
                }
                else
                {
                    arg->bulletTypeTemplates[i].collisionType = 0;
                    arg->bulletTypeTemplates[i].grazeSize.x = 24.0f;
                    arg->bulletTypeTemplates[i].grazeSize.y = 24.0f;
                }
            }
        }
    }
    memset(&g_ItemManager, 0, sizeof(ItemManager));
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00427620
ZunResult BulletManager::DeletedCallback(BulletManager *arg)
{
    bool bVar1;

    if (((g_Supervisor.curState == 3) || (g_Supervisor.curState == 0xb)) ||
        (g_Supervisor.curState == 0xc))
    {
        bVar1 = false;
    }
    else
    {
        bVar1 = true;
    }
    if (bVar1)
    {
        g_AnmManager->ReleaseAnm(0xb);
        g_AnmManager->ReleaseAnm(0xc);
        g_AnmManager->ReleaseAnm(0xd);
        g_AnmManager->ReleaseAnm(0xe);
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x004276a0
ZunResult BulletManager::RegisterChain(const char *etamaAnmPath)
{
    BulletManager *mgr = &g_BulletManager;
    g_BulletColor = g_DefaultBulletColors;
    mgr->Initialize();
    mgr->etamaAnmPath = etamaAnmPath;
    g_BulletManagerCalcChain.callback = (ChainCallback)OnUpdate;
    g_BulletManagerCalcChain.addedCallback = NULL;
    g_BulletManagerCalcChain.deletedCallback = NULL;
    g_BulletManagerCalcChain.addedCallback =
        (ChainLifecycleCallback)AddedCallback;
    g_BulletManagerCalcChain.deletedCallback =
        (ChainLifecycleCallback)DeletedCallback;
    g_BulletManagerCalcChain.arg = mgr;
    if (g_Chain.AddToCalcChain(&g_BulletManagerCalcChain, 0xc) != 0)
    {
        return ZUN_ERROR;
    }

    g_BulletManagerDrawChain.callback = (ChainCallback)OnDraw;
    g_BulletManagerDrawChain.addedCallback = NULL;
    g_BulletManagerDrawChain.deletedCallback = NULL;
    g_BulletManagerDrawChain.arg = mgr;
    g_Chain.AddToDrawChain(&g_BulletManagerDrawChain, 10);
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00427760
void BulletManager::CutChain()
{
    g_Chain.Cut(&g_BulletManagerCalcChain);
    g_Chain.Cut(&g_BulletManagerDrawChain);
    memset(&g_BulletManager, 0, sizeof(BulletManager));
}

#pragma var_order(i, bullet)
// FUNCTION: TH07 0x004277a0
void BulletManager::StopBulletMovement()
{
    Bullet *bullet;
    i32 i;

    bullet = g_BulletManager.bullets;
    for (i = 0; i < 0x400; i++, bullet++)
    {
        if (bullet->state == BULLET_INACTIVE)
        {
            continue;
        }

        bullet->velocity = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        bullet->unused_ba4 = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        bullet->angularVelocity = 0.0f;
        bullet->acceleration = 0.0f;
        bullet->speed = 0.0f;
        bullet->spriteOffset = 0;
        g_AnmManager->SetActiveSprite(
            &bullet->sprites.spriteBullet,
            (i32)bullet->sprites.spriteBullet.baseSpriteIdx +
                (i32)bullet->spriteOffset);
    }
}

// FUNCTION: TH07 0x004278b0
BulletCommand *Bullet::AddCommand(i32 command, i32 flag, u32 type)
{
    BulletCommand *bulletCommand = this->commands + command;
    bulletCommand->type = type;
    bulletCommand->flag = flag;
    this->moreFlags = this->moreFlags | (u16)type;
    this->curCmdIdx = 0;
    return bulletCommand;
}

// FUNCTION: TH07 0x00427910
BulletCommand *EnemyBulletShooter::AddCommand(i32 command, i32 flag, u32 type)
{
    BulletCommand *bulletCommand = &this->commands[command];
    bulletCommand->type = type;
    bulletCommand->flag = flag;
    this->flags |= type;
    return bulletCommand;
}

// FUNCTION: TH07 0x00427960
void Bullet::AddAngleAccelCommand(i32 command, i32 flag, i32 duration,
                                  f32 angle, f32 speed)
{
    BulletCommand *bulletCommand;

    bulletCommand = AddCommand(command, flag, 0x20);
    bulletCommand->duration = duration;
    bulletCommand->speed = speed;
    bulletCommand->angle = angle;
}

// FUNCTION: TH07 0x004279a0
void Bullet::AddTargetVelocityCommand(i32 command, i32 flag, i32 duration,
                                      f32 speed, f32 angle)
{
    BulletCommand *bulletCommand;

    bulletCommand = AddCommand(command, flag, 0x10);
    bulletCommand->duration = duration;
    bulletCommand->speed = speed;
    bulletCommand->angle = angle;
}

// FUNCTION: TH07 0x004279e0
void EnemyBulletShooter::AddAngleAccelCommand(i32 command, i32 flag,
                                              i32 duration, f32 angle,
                                              f32 speed)
{
    BulletCommand *bulletCommand = AddCommand(command, flag, 0x20);
    bulletCommand->duration = duration;
    bulletCommand->speed = speed;
    bulletCommand->angle = angle;
}

// FUNCTION: TH07 0x00427a20
void EnemyBulletShooter::AddDirChangeCommand(i32 command, i32 flag,
                                             i32 duration, i32 loopCount,
                                             f32 angle, f32 speed)
{
    BulletCommand *bulletCommand = AddCommand(command, flag, 0x80);
    bulletCommand->duration = duration;
    bulletCommand->loopCount = loopCount;
    bulletCommand->speed = speed;
    bulletCommand->angle = angle;
}

// FUNCTION: TH07 0x00427a70
void EnemyBulletShooter::AddTargetVelocityCommand(i32 command, i32 flag,
                                                  i32 duration, f32 speed,
                                                  f32 angle)
{
    BulletCommand *bulletCommand = AddCommand(command, flag, 0x10);
    bulletCommand->duration = duration;
    bulletCommand->speed = speed;
    bulletCommand->angle = angle;
}

// FUNCTION: TH07 0x00427ab0
void EnemyBulletShooter::AddSpawnDelayCommand(i32 command, i32 flag,
                                              i32 duration)
{
    BulletCommand *bulletCommand = AddCommand(command, flag, 0x2000);
    bulletCommand->duration = duration;
}
