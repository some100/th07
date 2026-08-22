#include "BulletManager.hpp"

#include "AnmIdx.hpp"
#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "GameManager.hpp"
#include "GameWindow.hpp"
#include "ItemManager.hpp"
#include "Player.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "Supervisor.hpp"
#include "ZunMath.hpp"
#include "utils.hpp"
#include <algorithm>

const BulletTypeInfo g_BulletTypeInfos[11] = {
    {ANM_SCRIPT_BULLETS_PELLET, ANM_SCRIPT_BULLETS_PELLET_SPAWN_FAST,
     ANM_SCRIPT_BULLETS_PELLET_SPAWN_NORMAL, ANM_SCRIPT_BULLETS_PELLET_SPAWN_SLOW,
     ANM_SCRIPT_BULLETS_SPAWN_DONUT_SMALL},
    {ANM_SCRIPT_BULLETS_RING_BALL, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_FAST,
     ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_NORMAL, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_SLOW,
     ANM_SCRIPT_BULLETS_SPAWN_DONUT_MEDIUM},
    {ANM_SCRIPT_BULLETS_RICE, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_FAST,
     ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_NORMAL, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_SLOW,
     ANM_SCRIPT_BULLETS_SPAWN_DONUT_MEDIUM},
    {ANM_SCRIPT_BULLETS_BALL, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_FAST,
     ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_NORMAL, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_SLOW,
     ANM_SCRIPT_BULLETS_SPAWN_DONUT_MEDIUM},
    {ANM_SCRIPT_BULLETS_KUNAI, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_FAST,
     ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_NORMAL, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_SLOW,
     ANM_SCRIPT_BULLETS_SPAWN_DONUT_MEDIUM},
    {ANM_SCRIPT_BULLETS_SHARD, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_FAST,
     ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_NORMAL, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_SLOW,
     ANM_SCRIPT_BULLETS_SPAWN_DONUT_MEDIUM},
    {ANM_SCRIPT_BULLETS_ARROWHEAD, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_FAST,
     ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_NORMAL, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_SLOW,
     ANM_SCRIPT_BULLETS_SPAWN_DONUT_MEDIUM},
    {ANM_SCRIPT_BULLETS_BIG_BALL, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_HUGE,
     ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_HUGE, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_HUGE,
     ANM_SCRIPT_BULLETS_SPAWN_DONUT_BIG},
    {ANM_SCRIPT_BULLETS_BUTTERFLY, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_HUGE,
     ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_HUGE, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_HUGE,
     ANM_SCRIPT_BULLETS_SPAWN_DONUT_BIG},
    {ANM_SCRIPT_BULLETS_KNIFE, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_HUGE,
     ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_HUGE, ANM_SCRIPT_BULLETS_BIG_BALL_SPAWN_HUGE,
     ANM_SCRIPT_BULLETS_SPAWN_DONUT_BIG},
    {ANM_SCRIPT_BULLETS_BUBBLE, ANM_SCRIPT_BULLETS_BUBBLE_SPAWN_SLOW,
     ANM_SCRIPT_BULLETS_BUBBLE_SPAWN_SLOW, ANM_SCRIPT_BULLETS_BUBBLE_SPAWN_SLOW,
     ANM_SCRIPT_BULLETS_BUBBLE_SPAWN_NORMAL},
};

u32 g_BulletColorsArray[28] = {
    0xFF000000, 0xFF303030, 0xFF606060, 0xFF500000, 0xFF900000, 0xFFFF2020, 0xFF400040,
    0xFF800080, 0xFFFF30FF, 0xFF000050, 0xFF000090, 0xFF2020FF, 0xFF203060, 0xFF304090,
    0xFF3080FF, 0xFF005000, 0xFF009000, 0xFF20FF20, 0xFF206000, 0xFF409010, 0xFF80FF20,
    0xFF505000, 0xFF909000, 0xFFFFFF20, 0xFF603000, 0xFF904010, 0xFFF08020, 0xFFFFFFFF};

u32 g_DefaultBulletColors[28] = {
    0xFFF0F0F0, 0xFFF0F0F0, 0xFFFFFFFF, 0xFFFFE0E0, 0xFFFFE0E0, 0xFFFFE0E0, 0xFFFFE0FF,
    0xFFFFE0FF, 0xFFFFE0FF, 0xFFE0E0FF, 0xFFE0E0FF, 0xFFE0E0FF, 0xFFE0FFFF, 0xFFE0FFFF,
    0xFFE0FFFF, 0xFFE0FFE0, 0xFFE0FFE0, 0xFFE0FFE0, 0xFFE0FFE0, 0xFFE0FFE0, 0xFFE0FFE0,
    0xFFFFFFE0, 0xFFFFFFE0, 0xFFFFFFE0, 0xFFFFE0E0, 0xFFFFE0E0, 0xFFFFE0E0, 0xFFFFFFFF};

u32 *g_BulletColor = g_BulletColorsArray;

i32 g_BulletSpriteOffset16Px[16] = {0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 0};

i32 g_BulletSpriteOffset32Px[8] = {0, 1, 1, 2, 2, 3, 4, 0};

ChainElem g_BulletManagerDrawChain;

BulletManager g_BulletManager;

ChainElem g_BulletManagerCalcChain;

void BulletManager::SetActiveSpriteByResolution(AnmVm *sprite, AnmVm *bulletTypeTemplate,
                                                Bullet *bullet, i32 spriteOffset)
{
    if (sprite->activeSpriteIdx != bulletTypeTemplate->activeSpriteIdx + spriteOffset)
    {
        if (bullet->sprites.spriteBullet.sprite->heightPx <= 16.0f)
        {
            g_AnmManager->SetActiveSprite(sprite, bulletTypeTemplate->activeSpriteIdx +
                                                      g_BulletSpriteOffset16Px[spriteOffset]);
        }
        else
        {
            if (bullet->sprites.spriteBullet.sprite->heightPx <= 32.0f)
            {
                g_AnmManager->SetActiveSprite(sprite, bulletTypeTemplate->activeSpriteIdx +
                                                          g_BulletSpriteOffset32Px[spriteOffset]);
            }
            else
            {
                g_AnmManager->SetActiveSprite(sprite,
                                              bulletTypeTemplate->activeSpriteIdx + spriteOffset);
            }
        }
    }
}

i32 BulletManager::SpawnSingleBullet(EnemyBulletShooter *bulletProps, i32 x, i32 y, f32 angle)
{
    f32 bulletAngle;
    Bullet *bullet;
    i32 i;
    f32 bulletSpeed;

    for (bullet = this->bulletsStart, i = 0; i < MAX_BULLETS; i++)
    {
        if (bullet->state == BULLET_INACTIVE)
        {
            break;
        }
        bullet++;
        if (bullet->state == BULLET_END_ARRAY)
        {
            bullet = this->bullets;
        }
    }
    if (i >= MAX_BULLETS)
    {
        return 1;
    }

    bulletAngle = 0.0f;
    if (bulletProps->count2 > 1)
    {
        bulletSpeed = bulletProps->speed1 - (bulletProps->speed1 - bulletProps->speed2) * (f32)y /
                                                (f32)(i32)bulletProps->count2;
    }
    else
    {
        bulletSpeed = bulletProps->speed1;
    }
    switch (bulletProps->aimMode)
    {
    case BULLET_AIM_SPREAD_AIMED:
    case BULLET_AIM_SPREAD_ABSOLUTE:
        if ((bulletProps->count1 & 1U) != 0)
        {
            bulletAngle += bulletProps->angle2 * (f32)((i32)((x + 1) / 2));
        }
        else
        {
            bulletAngle += (f32)(i32)(x / 2) * bulletProps->angle2 + bulletProps->angle2 * 0.5f;
        }
        if ((x & 1U) != 0)
        {
            bulletAngle *= -1.0f;
        }
        if (bulletProps->aimMode == BULLET_AIM_SPREAD_AIMED)
        {
            bulletAngle += angle;
        }
        bulletAngle += bulletProps->angle1;
        break;
    case BULLET_AIM_RING_AIMED:
        bulletAngle += angle;
    case BULLET_AIM_RING_ABSOLUTE:
        bulletAngle += (f32)x * ZUN_2PI / (f32)(i32)bulletProps->count1;
        bulletAngle += (f32)y * bulletProps->angle2 + bulletProps->angle1;
        break;
    case BULLET_AIM_RING_SHIFTED_AIMED:
        bulletAngle += angle;
    case BULLET_AIM_RING_SHIFTED_ABSOLUTE:
        bulletAngle += ZUN_PI / (f32)(i32)bulletProps->count1;
        bulletAngle += (f32)x * ZUN_2PI / (f32)(i32)bulletProps->count1;
        bulletAngle += bulletProps->angle1;
        break;
    case BULLET_AIM_ANGLE_RANDOM:
        bulletAngle = g_Rng.GetRandomFloatInRange(bulletProps->angle1 - bulletProps->angle2) +
                      bulletProps->angle2;
        break;
    case BULLET_AIM_RING_SPEED_RANDOM:
        bulletSpeed = g_Rng.GetRandomFloatInRange(bulletProps->speed1 - bulletProps->speed2) +
                      bulletProps->speed2;
        bulletAngle += (f32)x * ZUN_2PI / (f32)(i32)bulletProps->count1;
        bulletAngle += (f32)y * bulletProps->angle2 + bulletProps->angle1;
        break;
    case BULLET_AIM_RANDOM:
        bulletAngle = g_Rng.GetRandomFloatInRange(bulletProps->angle1 - bulletProps->angle2) +
                      bulletProps->angle2;
        bulletSpeed = g_Rng.GetRandomFloatInRange(bulletProps->speed1 - bulletProps->speed2) +
                      bulletProps->speed2;
    }
    bullet->state = BULLET_NORMAL;
    bullet->spawned = 1;
    bullet->grazed = 0;
    bullet->timer1 = 0;
    bullet->timer2 = 0;
    bullet->speed = bulletSpeed;
    bullet->prevAngle = bullet->angle = utils::AddNormalizeAngle(bulletAngle, 0.0f);
    bullet->pos = bulletProps->pos;
    bullet->pos.z = 0.1f;
    bullet->velocity.FromAngleMagnitude(bulletAngle,
                                        bulletSpeed * g_Supervisor.effectiveFramerateMultiplier);
    bullet->exFlags = (i16)bulletProps->flags;
    bullet->spriteOffset = bulletProps->spriteOffset;
    bullet->state2 = 0;
    AnmVm::AssignVm(&bullet->sprites.spriteBullet, &bulletProps->sprites->spriteBullet);
    AnmVm::AssignVm(&bullet->sprites.spriteSpawnEffectDonut,
                    &bulletProps->sprites->spriteSpawnEffectDonut);
    bullet->sprites.grazeSize = bulletProps->sprites->grazeSize;
    bullet->sprites.unused_b88 = bulletProps->sprites->unused_b88;
    bullet->sprites.bulletHeight = bulletProps->sprites->bulletHeight;
    bullet->sprites.collisionType = bulletProps->sprites->collisionType;
    bullet->soundIdx = bulletProps->soundOverride;
    bullet->spawnDelay = 0;
    if (bullet->sprites.spriteBullet.activeSpriteIdx !=
        bulletProps->sprites->spriteBullet.activeSpriteIdx + bulletProps->spriteOffset)
    {
        g_AnmManager->SetActiveSprite(&bullet->sprites.spriteBullet,
                                      bulletProps->sprites->spriteBullet.activeSpriteIdx +
                                          bulletProps->spriteOffset);
    }
    if (bullet->sprites.spriteSpawnEffectDonut.activeSpriteIdx !=
        bulletProps->sprites->spriteSpawnEffectDonut.activeSpriteIdx + bulletProps->spriteOffset)
    {
        if (bullet->sprites.spriteBullet.sprite->heightPx <= 16.0f)
        {
            g_AnmManager->SetActiveSprite(
                &bullet->sprites.spriteSpawnEffectDonut,
                bulletProps->sprites->spriteSpawnEffectDonut.activeSpriteIdx +
                    g_BulletSpriteOffset16Px[bulletProps->spriteOffset]);
        }
        else
        {
            if (bullet->sprites.spriteBullet.sprite->heightPx <= 32.0f)
            {
                g_AnmManager->SetActiveSprite(
                    &bullet->sprites.spriteSpawnEffectDonut,
                    bulletProps->sprites->spriteSpawnEffectDonut.activeSpriteIdx +
                        g_BulletSpriteOffset32Px[bulletProps->spriteOffset]);
            }
            else
            {
                g_AnmManager->SetActiveSprite(
                    &bullet->sprites.spriteSpawnEffectDonut,
                    bulletProps->sprites->spriteSpawnEffectDonut.activeSpriteIdx +
                        bulletProps->spriteOffset);
            }
        }
    }

    if (bulletProps->flags & 2)
    {
        AnmVm::AssignVm(&bullet->sprites.spriteSpawnEffectFast,
                        &bulletProps->sprites->spriteSpawnEffectFast);
        SetActiveSpriteByResolution(&bullet->sprites.spriteSpawnEffectFast,
                                    &bulletProps->sprites->spriteSpawnEffectFast, bullet,
                                    bulletProps->spriteOffset);
        bullet->state = BULLET_SPAWNING_FAST;
        bullet->pos -= bullet->velocity * 4.0f;
        bullet->prevPos = bullet->pos;
    }
    else if (bulletProps->flags & 4)
    {
        AnmVm::AssignVm(&bullet->sprites.spriteSpawnEffectNormal,
                        &bulletProps->sprites->spriteSpawnEffectNormal);
        SetActiveSpriteByResolution(&bullet->sprites.spriteSpawnEffectNormal,
                                    &bulletProps->sprites->spriteSpawnEffectNormal, bullet,
                                    (i32)bulletProps->spriteOffset);
        bullet->state = BULLET_SPAWNING_NORMAL;
        bullet->pos -= bullet->velocity * 4.0f;
        bullet->prevPos = bullet->pos;
    }
    else if (bulletProps->flags & 8)
    {
        AnmVm::AssignVm(&bullet->sprites.spriteSpawnEffectSlow,
                        &bulletProps->sprites->spriteSpawnEffectSlow);
        SetActiveSpriteByResolution(&bullet->sprites.spriteSpawnEffectSlow,
                                    &bulletProps->sprites->spriteSpawnEffectSlow, bullet,
                                    (i32)bulletProps->spriteOffset);
        bullet->state = BULLET_SPAWNING_SLOW;
        bullet->pos -= bullet->velocity * 4.0f;
        bullet->prevPos = bullet->pos;
    }
    memcpy(bullet->commands, bulletProps->commands, sizeof(bullet->commands));
    bullet->moreFlags = bulletProps->flags;
    bullet->exFlags = 0;
    bullet->curCmdIdx = 0;
    bullet->RunCommands();
    bullet->sprites.UpdatePrev();

    if (this->screenClearTime != 0 && (bullet->moreFlags & 0x1000) == 0)
    {
        bullet->state = BULLET_DESPAWN;
    }
    bullet++;
    if (bullet->state == BULLET_END_ARRAY)
    {
        this->bulletsStart = this->bullets;
    }
    else
    {
        this->bulletsStart = bullet;
    }
    return 0;
}

void Bullet::RunCommands()
{
    BulletCommand *cmd;

    for (;;)
    {
        if (this->curCmdIdx >= ARRAY_SIZE_SIGNED(this->commands))
        {
            return;
        }

        cmd = &this->commands[this->curCmdIdx];
        if (cmd->type == 0)
        {
            return;
        }
        if (cmd->flag == 0 && this->exFlags != 0)
        {
            return;
        }
        if (((u32)this->moreFlags & cmd->type) == 0)
        {
            this->curCmdIdx++;
            continue;
        }

        switch (cmd->type)
        {
        case 1:
            this->exFlags |= 1;
            this->commandStates[0].timer = 0;
            this->commandStates[0].vec3.z = 0.0f;
            break;
        case 0x10:
            this->exFlags |= 0x10;
            this->commandStates[1].speed = cmd->speed;
            this->commandStates[1].angle = cmd->angle > -990.0f ? cmd->angle : this->angle;
            this->commandStates[1].timer = 0;
            this->commandStates[1].duration = cmd->duration;
            this->commandStates[1].vec3.FromAngleMagnitude(
                this->commandStates[1].angle,
                g_Supervisor.effectiveFramerateMultiplier * this->commandStates[1].speed);
            if (this->curCmdIdx != 0 && this->soundIdx >= 0)
            {
                g_SoundPlayer.PlaySoundByIdx(this->soundIdx, 0);
            }
            break;
        case 0x20:
            this->exFlags |= 0x20;
            this->commandStates[2].speed = cmd->speed;
            this->commandStates[2].angle = cmd->angle;
            this->commandStates[2].timer = 0;
            this->commandStates[2].duration = cmd->duration;
            if (this->curCmdIdx != 0 && this->soundIdx >= 0)
            {
                g_SoundPlayer.PlaySoundByIdx(this->soundIdx, 0);
            }
            break;
        case 0x40:
        case 0x80:
        case 0x100:
            this->exFlags |= cmd->type;
            // ZUN quirk: Using the BulletCommand's speed for BulletCommandState's angle?
            this->commandStates[3].angle = cmd->speed;
            this->commandStates[3].speed = cmd->angle > -999.0f ? cmd->angle : this->speed;
            this->commandStates[3].timer = 0;
            this->commandStates[3].duration = cmd->duration;
            this->commandStates[3].maxTimes = cmd->loopCount;
            this->commandStates[3].minTimes = 0;
            break;
        case 0x400:
        case 0x800:
            this->exFlags |= cmd->type;
            if (cmd->speed >= 0.0f)
            {
                this->commandStates[4].speed = cmd->speed;
            }
            else
            {
                this->commandStates[4].speed = this->speed;
            }
            this->commandStates[4].maxTimes = cmd->duration;
            this->commandStates[4].duration = 0;
            break;
        case 0x2000:
            this->spawnDelay = cmd->duration;
            this->curCmdIdx++;
            continue;
        }
        this->curCmdIdx++;
        return;
    }
}

void BulletManager::RemoveAllBullets(i32 param_1)
{
    f32 local_28;
    f32 local_24;
    Laser *laser;
    Bullet *bullet;
    f32 local_18;
    i32 i;
    ZunVec3 local_10;

    bullet = g_BulletManager.bullets;
    for (i = 0; i < MAX_BULLETS; i++, bullet++)
    {
        if (bullet->state == BULLET_INACTIVE || bullet->state == BULLET_DESPAWN)
        {
            continue;
        }
        if (param_1 != 0 && param_1 < 9)
        {
            if (param_1 < 3)
            {
                g_ItemManager.SpawnItem(&bullet->pos, this->itemType, param_1);
            }
            else
            {
                g_ItemManager.SpawnItem(&bullet->pos, ITEM_CHERRY_SMALL, 1);
            }
            memset(bullet, 0, sizeof(Bullet));
        }
        else
        {
            bullet->state = BULLET_DESPAWN;
        }
    }
    laser = this->lasers;
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->lasers); i++, laser++)
    {
        if (!laser->inUse)
        {
            continue;
        }
        if ((laser->flags & 4) != 0 && param_1 != 10)
        {
            continue;
        }

        if (laser->state < LASER_DESPAWNING)
        {
            laser->state = LASER_DESPAWNING;
            laser->timer = 0;
            laser->width = laser->targetWidth;
            if (param_1 != 0 && param_1 < 9)
            {
                local_28 = laser->startOffset;
                sincosf(&local_18, &local_24, laser->angle);
                while (laser->endOffset > local_28)
                {
                    local_10.x = local_24 * local_28 + laser->pos.x;
                    local_10.y = local_18 * local_28 + laser->pos.y;
                    local_10.z = 0.0f;
                    if (param_1 < 3)
                    {
                        g_ItemManager.SpawnItem(&local_10, this->itemType, param_1);
                    }
                    else
                    {
                        g_ItemManager.SpawnItem(&local_10, ITEM_CHERRY_SMALL, 1);
                    }
                    local_28 += 32.0f;
                }
            }
        }
        laser->hitboxEndTime = 0;
    }
    this->screenClearTime = 10;
}

i32 BulletManager::DespawnBullets(i32 param_1, i32 turnIntoItem)
{
    f32 local_34;
    f32 local_30;
    Laser *laser;
    ZunVec3 local_28;
    Bullet *bullet;
    f32 local_18;
    i32 i;
    i32 local_c;
    i32 local_8;

    local_c = 0;
    local_8 = 2000;
    bullet = g_BulletManager.bullets;
    for (i = 0; i < MAX_BULLETS; i++, bullet++)
    {
        if (bullet->state == BULLET_INACTIVE)
        {
            continue;
        }

        g_ItemManager.SpawnItem(&bullet->pos, this->itemType, 1);
        g_AsciiManager.CreatePopup1(&bullet->pos, local_8,
                                    local_8 >= param_1 ? 0xFFFFFF00 : 0xFFFFFFFF);
        local_c += local_8;
        local_8 += 20;
        if (local_8 > param_1)
        {
            local_8 = param_1;
        }
        bullet->state = BULLET_DESPAWN;
    }
    laser = this->lasers;
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->lasers); i++, laser++)
    {
        if (!laser->inUse)
        {
            continue;
        }
        if (laser->state < LASER_DESPAWNING)
        {
            laser->state = LASER_DESPAWNING;
            laser->timer = 0;
            laser->width = laser->targetWidth;
            if (turnIntoItem)
            {
                g_ItemManager.SpawnItem(&laser->pos, this->itemType, 1);
                local_34 = laser->startOffset;
                sincosf(&local_18, &local_30, laser->angle);
                while (laser->endOffset > local_34)
                {
                    local_28.x = local_30 * local_34 + laser->pos.x;
                    local_28.y = local_18 * local_34 + laser->pos.y;
                    local_28.z = 0.0f;
                    g_ItemManager.SpawnItem(&local_28, this->itemType, 1);
                    local_34 += 32.0f;
                }
            }
        }
        laser->hitboxEndTime = 0;
    }
    this->screenClearTime = 10;
    return local_c;
}

void BulletManager::RemoveBulletsInRadius(ZunVec3 *centerPos, f32 radius)
{
    ZunVec3 diff;
    Bullet *bullet;
    i32 i;

    bullet = g_BulletManager.bullets;
    radius *= radius;
    for (i = 0; i < MAX_BULLETS; i++, bullet++)
    {
        if (bullet->state == BULLET_INACTIVE || bullet->state == BULLET_DESPAWN)
        {
            continue;
        }

        diff = bullet->pos - *centerPos;

        if (diff.LengthSq() > radius)
        {
            continue;
        }

        g_ItemManager.SpawnItem(&bullet->pos, ITEM_POINT_BULLET, 1);
        memset(bullet, 0, sizeof(Bullet));
    }
}

i32 BulletManager::SpawnBulletPattern(EnemyBulletShooter *bulletProps)
{
    f32 angle;
    i32 x;
    i32 y;

    if (g_BulletManager.bulletCount >= MAX_BULLETS)
    {
        return 0;
    }

    bulletProps->sprites = this->bulletTypeTemplates + bulletProps->sprite;
    angle = g_Player.AngleToPlayer(&bulletProps->pos);
    for (x = 0; x < bulletProps->count2; x++)
    {
        for (y = 0; y < bulletProps->count1; y++)
        {
            if (SpawnSingleBullet(bulletProps, y, x, angle))
            {
                goto stop;
            }
        }
    }
stop:
    if ((bulletProps->flags & 0x200) != 0)
    {
        g_SoundPlayer.PlaySoundByIdx(bulletProps->soundIdx, 0);
    }
    return 0;
}

Laser *BulletManager::SpawnLaserPattern(EnemyLaserShooter *laserShooter)
{
    Laser *laser;
    i32 i;

    laser = this->lasers;
    if (this->screenClearTime != 0 && (laserShooter->flags & 4) == 0)
    {
        return laser;
    }

    for (i = 0; i < ARRAY_SIZE_SIGNED(this->lasers); i++, laser++)
    {
        if (laser->inUse)
        {
            continue;
        }

        g_AnmManager->SetAnmIdxAndExecuteScript(&laser->vm0,
                                                laserShooter->sprite + ANM_SCRIPT_BULLETS_LASER);
        g_AnmManager->SetActiveSprite(&laser->vm0,
                                      laser->vm0.activeSpriteIdx + laserShooter->spriteOffset);
        g_AnmManager->InitializeAndSetActiveSprite(
            &laser->vm1,
            g_BulletSpriteOffset16Px[laserShooter->spriteOffset] + ANM_SPRITE_BULLETS_ORB);
        laser->vm1.blendMode = 1;
        laser->prevPos = laser->pos = laserShooter->pos;
        laser->color = laserShooter->spriteOffset;
        laser->inUse = 1;
        laser->prevAngle = laser->angle = laserShooter->angle1;
        if (laserShooter->type == 0)
        {
            laser->prevAngle = laser->angle =
                g_Player.AngleToPlayer(&laserShooter->pos) + laser->angle;
        }
        laser->flags = laserShooter->flags;
        laser->timer = 0;
        laser->prevStartOffset = laser->startOffset = laserShooter->startOffset;
        laser->prevEndOffset = laser->endOffset = laserShooter->endOffset;
        laser->startLength = laserShooter->startLength;
        laser->width = laserShooter->width;
        laser->speed = laserShooter->speed1;
        laser->startTime = laserShooter->startTime;
        laser->duration = laserShooter->duration;
        laser->endTime = laserShooter->endTime;
        laser->hitboxStartTime = laserShooter->hitboxStartTime;
        laser->hitboxEndTime = laserShooter->hitboxEndTime;
        laser->hideWarning = 0;
        if (laser->startTime == 0)
        {
            laser->state = LASER_ACTIVE;
        }
        else
        {
            laser->state = LASER_SPAWNING;
        }
        laser->vm0.UpdatePrev();
        laser->vm1.UpdatePrev();
        break;
    }
    return laser;
}

void Bullet::UpdateBulletBurstSpeed()
{
    if (this->commandStates[0].timer <= 16)
    {
        f32 local_8 = 5.0f - this->commandStates[0].timer.AsFloat() * 5.0f / 16.0f;
        this->velocity.FromAngleMagnitude(
            this->angle, (local_8 + this->speed) * g_Supervisor.effectiveFramerateMultiplier);
    }
    else
    {
        this->exFlags ^= 1;
    }
    this->commandStates[0].timer++;
}

void Bullet::UpdateBulletTargetVelocity()
{
    if (this->commandStates[1].timer >= this->commandStates[1].duration)
    {
        this->exFlags = this->exFlags & 0xffffffef;
    }
    else
    {
        this->velocity += this->commandStates[1].vec3 * g_Supervisor.effectiveFramerateMultiplier;
        if (fabsf(this->velocity.x) > 0.0001f || fabsf(this->velocity.y) > 0.0001f)
        {
            this->angle = atan2f(this->velocity.y, this->velocity.x);
        }
    }
    this->commandStates[1].timer++;
}

void Bullet::UpdateBulletTargetAngle()
{
    if (this->commandStates[2].timer >= this->commandStates[2].duration)
    {
        this->exFlags = this->exFlags & 0xffffffdf;
    }
    else
    {
        this->angle = utils::AddNormalizeAngle(
            this->angle, this->commandStates[2].angle * g_Supervisor.effectiveFramerateMultiplier);
        this->speed += this->commandStates[2].speed * g_Supervisor.effectiveFramerateMultiplier;
        this->velocity.FromAngleMagnitude(this->angle,
                                          this->speed * g_Supervisor.effectiveFramerateMultiplier);
    }
    this->commandStates[2].timer++;
}

void Bullet::UpdateBulletDirChangeAndResume()
{
    f32 local_8;

    if (this->commandStates[3].timer >= this->commandStates[3].duration)
    {
        if (this->soundIdx >= 0)
        {
            g_SoundPlayer.PlaySoundByIdx(this->soundIdx, 0);
        }
        this->commandStates[3].minTimes++;
        if (this->commandStates[3].minTimes >= this->commandStates[3].maxTimes)
        {
            this->exFlags = this->exFlags & 0xffffffbf;
        }
        this->angle += this->commandStates[3].angle;
        this->speed = this->commandStates[3].speed;
        local_8 = this->speed;
        this->commandStates[3].timer = 0;
    }
    else
    {
        local_8 = this->speed - this->commandStates[3].timer.AsFloat() * this->speed /
                                    (f32)this->commandStates[3].duration;
    }
    this->velocity.FromAngleMagnitude(this->angle,
                                      local_8 * g_Supervisor.effectiveFramerateMultiplier);
    this->commandStates[3].timer++;
}

void Bullet::UpdateBulletDirChangeAbsoluteAndResume()
{
    f32 local_8;

    if (this->commandStates[3].timer >= this->commandStates[3].duration)
    {
        if (this->soundIdx >= 0)
        {
            g_SoundPlayer.PlaySoundByIdx(this->soundIdx, 0);
        }
        this->commandStates[3].minTimes++;
        if (this->commandStates[3].minTimes >= this->commandStates[3].maxTimes)
        {
            this->exFlags = this->exFlags & 0xfffffeff;
        }
        this->angle = this->commandStates[3].angle;
        this->speed = this->commandStates[3].speed;
        local_8 = this->speed;
        this->commandStates[3].timer = 0;
    }
    else
    {
        local_8 = this->speed - this->commandStates[3].timer.AsFloat() * this->speed /
                                    (f32)this->commandStates[3].duration;
    }
    this->velocity.FromAngleMagnitude(this->angle,
                                      local_8 * g_Supervisor.effectiveFramerateMultiplier);
    this->commandStates[3].timer++;
}

void Bullet::UpdateBulletDirChangeAimAtPlayer()
{
    f32 local_8;

    if (this->commandStates[3].timer >= this->commandStates[3].duration)
    {
        if (this->soundIdx >= 0)
        {
            g_SoundPlayer.PlaySoundByIdx(this->soundIdx, 0);
        }
        this->commandStates[3].minTimes++;
        if (this->commandStates[3].minTimes >= this->commandStates[3].maxTimes)
        {
            this->exFlags = this->exFlags & 0xffffff7f;
        }
        this->angle = utils::AddNormalizeAngle(g_Player.AngleToPlayer(&this->pos),
                                               this->commandStates[3].angle);
        this->speed = this->commandStates[3].speed;
        local_8 = this->speed;
        this->commandStates[3].timer = 0;
    }
    else
    {
        local_8 = this->speed - this->commandStates[3].timer.AsFloat() * this->speed /
                                    (f32)this->commandStates[3].duration;
    }
    this->velocity.FromAngleMagnitude(this->angle,
                                      local_8 * g_Supervisor.effectiveFramerateMultiplier);
    this->commandStates[3].timer++;
}

void Bullet::UpdateBulletBounce()
{
    f32 speed;

    if (g_GameManager.IsInBounds(this->pos.x, this->pos.y,
                                 this->sprites.spriteBullet.sprite->widthPx,
                                 this->sprites.spriteBullet.sprite->heightPx) == 0)
    {
        if (this->soundIdx >= 0)
        {
            g_SoundPlayer.PlaySoundByIdx(this->soundIdx, 0);
        }
        if (this->pos.x < 0.0f || this->pos.x >= 384.0f)
        {
            this->angle = -this->angle - ZUN_PI;
            this->angle = utils::AddNormalizeAngle(this->angle, 0.0f);
        }
        if (this->pos.y < 0.0f || (this->pos.y >= 448.0f && (this->exFlags & 0x400U) != 0))
        {
            this->angle = -this->angle;
        }
        this->speed = this->commandStates[4].speed;
        speed = this->speed;
        this->velocity.FromAngleMagnitude(this->angle,
                                          speed * g_Supervisor.effectiveFramerateMultiplier);
        this->commandStates[4].duration++;
        if (this->commandStates[4].duration >= this->commandStates[4].maxTimes)
        {
            this->exFlags = this->exFlags & 0xfffff3ff;
        }
    }
}

u32 BulletManager::OnUpdate(BulletManager *arg)
{
    ZunVec3 laserCenter;
    Laser *laser;
    i32 alpha;
    Bullet *bullet;
    ZunVec3 laserHitbox;
    i32 blockIdx;
    f32 width;
    i32 i;
    i32 collisionRes;

    for (i = 0; i < 1024; i++)
    {
        if (arg->bullets[i].state != BULLET_INACTIVE)
        {
            arg->bullets[i].sprites.UpdatePrev();
            arg->bullets[i].prevPos = arg->bullets[i].pos;
            arg->bullets[i].prevAngle = arg->bullets[i].angle;
        }
    }

    blockIdx = 0;
    bullet = arg->bullets;
    if (g_GameManager.isTimeStopped)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    g_ItemManager.OnUpdate();
    arg->bulletCount = 0;
    arg->bulletsPtrs[5] = NULL;
    arg->bulletsPtrs[4] = NULL;
    arg->bulletsPtrs[3] = NULL;
    arg->bulletsPtrs[2] = NULL;
    arg->bulletsPtrs[1] = NULL;
    arg->bulletsPtrs[0] = NULL;

    for (i = 0; i < MAX_BULLETS; i++)
    {
        if (bullet->state == BULLET_INACTIVE)
        {
            goto bullet_loop_continue;
        }
        arg->bulletCount++;

        switch (bullet->state)
        {
        switch_break:
            bullet->state = BULLET_NORMAL;
            bullet->timer1 = 0;
        case BULLET_NORMAL:
            bullet->RunCommands();
            if (bullet->exFlags != 0)
            {
                if ((bullet->exFlags & 1) != 0)
                {
                    bullet->UpdateBulletBurstSpeed();
                }
                if ((bullet->exFlags & 0x10) != 0)
                {
                    bullet->UpdateBulletTargetVelocity();
                }
                if ((bullet->exFlags & 0x20) != 0)
                {
                    bullet->UpdateBulletTargetAngle();
                }
                if ((bullet->exFlags & 0x40) != 0)
                {
                    bullet->UpdateBulletDirChangeAndResume();
                }
                if ((bullet->exFlags & 0x100) != 0)
                {
                    bullet->UpdateBulletDirChangeAbsoluteAndResume();
                }
                if ((bullet->exFlags & 0x80) != 0)
                {
                    bullet->UpdateBulletDirChangeAimAtPlayer();
                }
                if ((bullet->exFlags & 0xc00) != 0)
                {
                    bullet->UpdateBulletBounce();
                }
            }

            if (bullet->spawnDelay != 0)
            {
                bullet->spawnDelay--;
            }
            bullet->pos += bullet->velocity;

            if (bullet->spawnDelay == 0)
            {
                if (!g_GameManager.IsInBounds(bullet->pos.x, bullet->pos.y,
                                              bullet->sprites.spriteBullet.sprite->widthPx,
                                              bullet->sprites.spriteBullet.sprite->heightPx))
                {
                    if ((bullet->exFlags & 0xdc0) != 0)
                    {
                        bullet->outOfBoundsTime++;
                        if (bullet->outOfBoundsTime >= 128)
                        {
                            bullet->Initialize();
                            goto bullet_loop_continue;
                        }
                    }
                    else
                    {
                        if (bullet->outOfBoundsTime == 0)
                        {
                            bullet->Initialize();
                            goto bullet_loop_continue;
                        }
                        bullet->outOfBoundsTime--;
                    }
                    goto do_collision;
                }
                bullet->outOfBoundsTime = 0;
                goto do_collision;
            }

        do_collision:
            if (!bullet->grazed && bullet->timer2.GetCurrent() >= 16)
            {
                collisionRes = g_Player.CheckGraze(&bullet->pos, &bullet->sprites.grazeSize);
                if (collisionRes == 1)
                {
                    bullet->grazed = 1;
                    goto do_player_collision;
                }
                else if (collisionRes == 2)
                {
                    if ((bullet->moreFlags & 0x1000) == 0)
                    {
                        bullet->state = BULLET_DESPAWN;
                        g_ItemManager.SpawnItem(&bullet->pos, g_Player.itemType, 1);
                    }
                }
                goto do_sprite_anim;
            }

        do_player_collision:
            collisionRes = g_Player.CalcKillboxCollision(&bullet->pos, &bullet->sprites.grazeSize);
            if (collisionRes != 0)
            {
                if (collisionRes != 2 || (bullet->moreFlags & 0x1000) == 0)
                {
                    bullet->state = BULLET_DESPAWN;
                    if (collisionRes == 2)
                    {
                        g_ItemManager.SpawnItem(&bullet->pos, g_Player.itemType, 1);
                    }
                }
            }

        do_sprite_anim:
            if (bullet->sprites.spriteBullet.currentInstruction)
            {
                g_AnmManager->ExecuteScript(&bullet->sprites.spriteBullet);
            }
            goto update_timers;

        case BULLET_SPAWNING_FAST:
            bullet->timer2--;
            bullet->pos += bullet->velocity / 2.0f;
            if (!g_AnmManager->ExecuteScript(&bullet->sprites.spriteSpawnEffectFast))
            {
                goto update_timers;
            }
            goto switch_break;

        case BULLET_SPAWNING_NORMAL:
            bullet->timer2--;
            bullet->pos += bullet->velocity / 2.5f;
            if (!g_AnmManager->ExecuteScript(&bullet->sprites.spriteSpawnEffectNormal))
            {
                goto update_timers;
            }
            goto switch_break;

        case BULLET_SPAWNING_SLOW:
            bullet->timer2--;
            bullet->pos += bullet->velocity / 3.0f;
            if (!g_AnmManager->ExecuteScript(&bullet->sprites.spriteSpawnEffectSlow))
            {
                goto update_timers;
            }
            goto switch_break;

        case BULLET_DESPAWN:
            bullet->pos += bullet->velocity / 2.0f;
            if (g_AnmManager->ExecuteScript(&bullet->sprites.spriteSpawnEffectDonut))
            {
                bullet->Initialize();
                goto bullet_loop_continue;
            }
            goto update_timers;

        default:
            goto update_timers;
        }

    update_timers:
        bullet->timer1++;
        bullet->timer2++;
        bullet->next = arg->bulletsPtrs[bullet->sprites.collisionType];
        arg->bulletsPtrs[bullet->sprites.collisionType] = bullet;

    bullet_loop_continue:
        blockIdx--;
        if (blockIdx < 0)
        {
            blockIdx = MAX_BULLETS - 1;
            bullet += MAX_BULLETS;
        }
        bullet--;
    }

    laser = arg->lasers;
    for (i = 0; i < ARRAY_SIZE_SIGNED(arg->lasers); i++, laser++)
    {
        if (!laser->inUse)
        {
            continue;
        }

        laser->vm0.UpdatePrev();
        laser->vm1.UpdatePrev();

        laser->prevPos = laser->pos;
        laser->prevAngle = laser->angle;
        laser->prevStartOffset = laser->startOffset;
        laser->prevEndOffset = laser->endOffset;

        laser->endOffset =
            g_Supervisor.effectiveFramerateMultiplier * laser->speed + laser->endOffset;
        if (laser->startLength < laser->endOffset - laser->startOffset)
        {
            laser->startOffset = laser->endOffset - laser->startLength;
        }
        if (laser->startOffset < 0.0f)
        {
            laser->startOffset = 0.0f;
        }
        laserHitbox.y = laser->width / 2.0f;
        laserHitbox.x = laser->endOffset - laser->startOffset;
        laserCenter.x =
            (laser->endOffset - laser->startOffset) / 2.0f + laser->startOffset + laser->pos.x;
        laserCenter.y = laser->pos.y;
        laser->vm0.scale.x = laser->width / laser->vm0.sprite->widthPx;
        width = laser->endOffset - laser->startOffset; // width is used as length here
        laser->vm0.scale.y = width / laser->vm0.sprite->heightPx;
        laser->UpdateRotationZFromAngle();
        laser->vm0.flags |= 4;

        switch (laser->state)
        {
        case LASER_SPAWNING:
            if ((laser->flags & 1) != 0)
            {
                alpha = laser->timer.AsFloat() * 255.0f / (f32)laser->startTime;
                if (alpha > 255)
                {
                    alpha = 255;
                }
                laser->vm0.color.color = alpha << 24;
            }
            else
            {
                i32 waitTime = laser->startTime > 30 ? 30 : laser->startTime;
                if (laser->startTime - waitTime < laser->timer.GetCurrent())
                {
                    width = laser->timer.AsFloat() * laser->width / (f32)laser->startTime;
                }
                else
                {
                    width = 1.2f;
                }
                laser->targetWidth = width;
                laser->vm0.scale.x = width / 16.0f;

                // ZUN bug: ZUN stores width / 2.0f in laserHitbox.x as though
                // it controlled the width of the hitbox, even though it's
                // actually the length of the laser as in
                // laser->endOffset - laser->startOffset. As a result, when a
                // laser is in its spawning state and its hitbox is set to
                // start, it'll only have a small hitbox on its midpoint
                // equal to the halfwidth.
                laserHitbox.x = width / 2.0f;
            }
            if (laser->timer >= laser->hitboxStartTime)
            {
                g_Player.CalcLaserHitbox(&laserCenter, &laserHitbox, &laser->pos, laser->angle,
                                         laser->timer.GetCurrent() % 12 == 0);
            }
            if (laser->timer < laser->startTime)
            {
                break;
            }
            laser->timer = 0;
            laser->state++;
            laser->targetWidth = laser->width;
        case LASER_ACTIVE:
            g_Player.CalcLaserHitbox(&laserCenter, &laserHitbox, &laser->pos, laser->angle,
                                     laser->timer.GetCurrent() % 12 == 0);
            if (laser->timer < laser->duration)
            {
                break;
            }
            laser->timer = 0;
            laser->state++;
            if (laser->endTime == 0)
            {
                laser->inUse = 0;
                continue;
            }
        case LASER_DESPAWNING:
            if ((laser->flags & 1) != 0)
            {
                alpha = laser->timer.AsFloat() * 255.0f / (f32)laser->startTime;
                if (alpha > 255)
                {
                    alpha = 255;
                }
                laser->vm0.color.color = alpha << 24;
            }
            else
            {
                if (laser->endTime > 0)
                {
                    width =
                        laser->width - laser->timer.AsFloat() * laser->width / (f32)laser->endTime;
                    laser->vm0.scale.x = width / 16.0f;

                    // ZUN bug: Same bug as in the laser spawning. The laser
                    // will only have a hitbox on its midpoint.
                    laserHitbox.x = width / 2.0f;
                }
            }
            if (laser->timer < laser->hitboxEndTime)
            {
                g_Player.CalcLaserHitbox(&laserCenter, &laserHitbox, &laser->pos, laser->angle,
                                         laser->timer.GetCurrent() % 12 == 0);
            }
            if (laser->timer < laser->endTime)
            {
                break;
            }
            laser->inUse = 0;
            continue;
        }
        if (laser->startOffset >= (f32)GAME_WINDOW_WIDTH)
        {
            laser->inUse = 0;
        }
        laser->timer++;
        g_AnmManager->ExecuteScript(&laser->vm0);
    }

    if (arg->screenClearTime != 0)
    {
        arg->screenClearTime--;
    }

    arg->time++;
    arg->updateCount++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

void Bullet::Draw()
{
    AnmVm *vm;

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
        break;
    default:
        vm = &this->sprites.spriteBullet;
        break;
    }

    ZunVec3 drawPos = this->prevPos.Lerp(this->pos, g_RenderAlpha);
    vm->pos.x = g_GameManager.arcadeRegionTopLeftPos.x + drawPos.x;
    vm->pos.y = g_GameManager.arcadeRegionTopLeftPos.y + drawPos.y;
    vm->pos.z = 0.05f;
    vm->color.color = (vm->color.color & 0xff000000) | 0xffffff;
    vm->prevColor = vm->color;
    if (vm->autoRotate)
    {
        vm->SetRotationZ(utils::AddNormalizeAngle(
            ZUN_PI / 2.0f + utils::LerpAngle(this->prevAngle, this->angle, g_RenderAlpha), 0.0f));
        vm->prevRotation.z = vm->rotation.z;
        vm->updateRotation = 1;
    }
    g_AnmManager->Draw(vm);
}

u32 BulletManager::OnDraw(BulletManager *arg)
{
    Bullet *bullet;
    f32 local_18;
    f32 local_14;
    Laser *laser;
    f32 local_c;
    i32 i;

    laser = arg->lasers;
    for (i = 0; i < ARRAY_SIZE_SIGNED(arg->lasers); i++, laser++)
    {
        if (!laser->inUse)
        {
            continue;
        }
        ZunVec3 drawPos = laser->prevPos.Lerp(laser->pos, g_RenderAlpha);
        f32 drawAngle = utils::LerpAngle(laser->prevAngle, laser->angle, g_RenderAlpha);
        f32 drawStart = utils::Lerp(laser->prevStartOffset, laser->startOffset, g_RenderAlpha);
        f32 drawEnd = utils::Lerp(laser->prevEndOffset, laser->endOffset, g_RenderAlpha);

        sincosf(&local_c, &local_18, drawAngle);
        ZunVec3 origVm0Pos = laser->vm0.pos;
        f32 origVm0RotZ = laser->vm0.rotation.z;
        f32 origVm0PrevRotZ = laser->vm0.prevRotation.z;
        Float2 origVm0Scale = laser->vm0.scale;
        Float2 origVm0PrevScale = laser->vm0.prevScale;

        local_14 = (drawEnd - drawStart) / 2.0f + drawStart;
        laser->vm0.pos.x = local_18 * local_14 + drawPos.x + g_GameManager.arcadeRegionTopLeftPos.x;
        laser->vm0.pos.y = local_c * local_14 + drawPos.y + g_GameManager.arcadeRegionTopLeftPos.y;
        laser->vm0.pos.z = 0.05f;
        laser->vm0.rotation.z = utils::AddNormalizeAngle(ZUN_PI / 2.0f + drawAngle, 0.0f);
        laser->vm0.prevRotation.z = laser->vm0.rotation.z;
        laser->vm0.updateRotation = 1;

        if (laser->vm0.sprite && laser->vm0.sprite->heightPx > 0.0f)
        {
            laser->vm0.scale.y = (drawEnd - drawStart) / laser->vm0.sprite->heightPx;
            laser->vm0.prevScale.y = laser->vm0.scale.y;
        }

        laser->color = (laser->color & 0xff000000) | 0xffffff;
        g_AnmManager->Draw(&laser->vm0);

        laser->vm0.pos = origVm0Pos;
        laser->vm0.rotation.z = origVm0RotZ;
        laser->vm0.prevRotation.z = origVm0PrevRotZ;
        laser->vm0.scale = origVm0Scale;
        laser->vm0.prevScale = origVm0PrevScale;

        if ((drawStart < 16.0f || laser->speed == 0.0f) &&
            (laser->hideWarning == 0 || laser->state != LASER_SPAWNING))
        {
            ZunVec3 origVm1Pos = laser->vm1.pos;
            Float2 origVm1Scale = laser->vm1.scale;
            Float2 origVm1PrevScale = laser->vm1.prevScale;

            laser->vm1.pos.x =
                local_18 * drawStart + drawPos.x + g_GameManager.arcadeRegionTopLeftPos.x;
            laser->vm1.pos.y =
                local_c * drawStart + drawPos.y + g_GameManager.arcadeRegionTopLeftPos.y;
            laser->vm1.pos.z = 0.05f;
            laser->vm1.color.color = laser->vm0.color.color;
            laser->vm1.flag6 = 1;
            laser->vm1.color.color = (laser->vm1.color.color & 0xffffff) | 0xff000000;
            laser->vm1.prevColor.color = laser->vm1.color.color;
            laser->vm1.scale.x = laser->width / 10.0f * ((16.0f - drawStart) / 16.0f);
            laser->vm1.scale.y = laser->vm1.scale.x;
            if (laser->vm1.scale.y <= 0.0f)
            {
                laser->vm1.scale.x = laser->width / 10.0f;
                laser->vm1.scale.y = laser->vm1.scale.x;
            }
            laser->vm1.prevScale = laser->vm1.scale;
            g_AnmManager->Draw(&laser->vm1);

            laser->vm1.pos = origVm1Pos;
            laser->vm1.scale = origVm1Scale;
            laser->vm1.prevScale = origVm1PrevScale;
        }
    }
    g_ItemManager.OnDraw();

    Bullet *activeBullets[1024];
    i32 activeCount = 0;
    for (i = 0; i < ARRAY_SIZE_SIGNED(activeBullets); i++)
    {
        if (arg->bullets[i].state != BULLET_INACTIVE && arg->bullets[i].state != BULLET_END_ARRAY)
        {
            activeBullets[activeCount++] = &arg->bullets[i];
        }
    }
    std::stable_sort(activeBullets, activeBullets + activeCount, [](Bullet *a, Bullet *b) {
        AnmVm *vmA = (a->state == BULLET_DESPAWN) ? &a->sprites.spriteSpawnEffectDonut
                                                  : &a->sprites.spriteBullet;
        AnmVm *vmB = (b->state == BULLET_DESPAWN) ? &b->sprites.spriteSpawnEffectDonut
                                                  : &b->sprites.spriteBullet;

        if (vmA->blendMode != vmB->blendMode)
        {
            return vmA->blendMode < vmB->blendMode;
        }

        if (a->sprites.collisionType != b->sprites.collisionType)
        {
            return a->sprites.collisionType < b->sprites.collisionType;
        }

        i32 texA = vmA->sprite ? vmA->sprite->sourceFileIndex : -1;
        i32 texB = vmB->sprite ? vmB->sprite->sourceFileIndex : -1;
        return texA < texB;
    });

    for (i = 0; i < activeCount; i++)
    {
        activeBullets[i]->Draw();
    }

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult BulletManager::AddedCallback(BulletManager *arg)
{
    u32 i;

    if ((u32)(g_Supervisor.curState != SUPERVISOR_STATE_NEXT_STAGE &&
              g_Supervisor.curState != SUPERVISOR_STATE_RESTART_STAGE &&
              g_Supervisor.curState != SUPERVISOR_STATE_NEXT_STAGE_USELESS))
    {
        if (g_AnmManager->LoadAnms(ANM_FILE_BULLETS, "data/etama.anm", ANM_OFFSET_BULLETS) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
    }

    for (i = 0; i < ARRAY_SIZE_SIGNED(g_BulletTypeInfos); i++)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(&arg->bulletTypeTemplates[i].spriteBullet,
                                                g_BulletTypeInfos[i].anmFileIdx);
        g_AnmManager->SetAnmIdxAndExecuteScript(&arg->bulletTypeTemplates[i].spriteSpawnEffectFast,
                                                g_BulletTypeInfos[i].spawnFastIdx);
        g_AnmManager->SetAnmIdxAndExecuteScript(
            &arg->bulletTypeTemplates[i].spriteSpawnEffectNormal,
            g_BulletTypeInfos[i].spawnNormalIdx);
        g_AnmManager->SetAnmIdxAndExecuteScript(&arg->bulletTypeTemplates[i].spriteSpawnEffectSlow,
                                                g_BulletTypeInfos[i].spawnSlowIdx);
        g_AnmManager->SetAnmIdxAndExecuteScript(&arg->bulletTypeTemplates[i].spriteSpawnEffectDonut,
                                                g_BulletTypeInfos[i].spawnDonutIdx);
        arg->bulletTypeTemplates[i].spriteBullet.zWriteDisable = 1;
        arg->bulletTypeTemplates[i].spriteSpawnEffectFast.zWriteDisable = 1;
        arg->bulletTypeTemplates[i].spriteSpawnEffectNormal.zWriteDisable = 1;
        arg->bulletTypeTemplates[i].spriteSpawnEffectSlow.zWriteDisable = 1;
        arg->bulletTypeTemplates[i].spriteSpawnEffectDonut.zWriteDisable = 1;
        arg->bulletTypeTemplates[i].spriteBullet.baseSpriteIdx =
            arg->bulletTypeTemplates[i].spriteBullet.activeSpriteIdx;
        arg->bulletTypeTemplates[i].bulletHeight =
            (u8)arg->bulletTypeTemplates[i].spriteBullet.sprite->heightPx;
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
                case ANM_SCRIPT_BULLETS_RICE:
                    arg->bulletTypeTemplates[i].grazeSize.x = 4.0f;
                    arg->bulletTypeTemplates[i].grazeSize.y = 4.0f;
                    arg->bulletTypeTemplates[i].collisionType = 4;
                    break;
                case ANM_SCRIPT_BULLETS_KUNAI:
                case ANM_SCRIPT_BULLETS_ARROWHEAD:
                    arg->bulletTypeTemplates[i].grazeSize.x = 4.0f;
                    arg->bulletTypeTemplates[i].grazeSize.y = 4.0f;
                    arg->bulletTypeTemplates[i].collisionType = 4;
                    break;
                case ANM_SCRIPT_BULLETS_SHARD:
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
                if (arg->bulletTypeTemplates[i].spriteBullet.sprite->heightPx <= 32.0f)
                {
                    switch (g_BulletTypeInfos[i].anmFileIdx)
                    {
                    case ANM_SCRIPT_BULLETS_BUTTERFLY:
                        arg->bulletTypeTemplates[i].grazeSize.x = 5.0f;
                        arg->bulletTypeTemplates[i].grazeSize.y = 5.0f;
                        arg->bulletTypeTemplates[i].collisionType = 1;
                        break;
                    case ANM_SCRIPT_BULLETS_KNIFE:
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

ZunResult BulletManager::DeletedCallback(BulletManager *arg)
{
    if ((u32)(g_Supervisor.curState != SUPERVISOR_STATE_NEXT_STAGE &&
              g_Supervisor.curState != SUPERVISOR_STATE_RESTART_STAGE &&
              g_Supervisor.curState != SUPERVISOR_STATE_NEXT_STAGE_USELESS))
    {
        g_AnmManager->ReleaseAnm(ANM_FILE_BULLETS_0);
        g_AnmManager->ReleaseAnm(ANM_FILE_BULLETS_1);
        g_AnmManager->ReleaseAnm(ANM_FILE_BULLETS_2);
        g_AnmManager->ReleaseAnm(ANM_FILE_BULLETS_3);
    }
    return ZUN_SUCCESS;
}

ZunResult BulletManager::RegisterChain(const char *etamaAnmPath)
{
    BulletManager *mgr = &g_BulletManager;
    g_BulletColor = g_DefaultBulletColors;
    mgr->Initialize();
    mgr->etamaAnmPath = etamaAnmPath;
    g_BulletManagerCalcChain.callback = (ChainCallback)OnUpdate;
    g_BulletManagerCalcChain.addedCallback = NULL;
    g_BulletManagerCalcChain.deletedCallback = NULL;
    g_BulletManagerCalcChain.addedCallback = (ChainLifecycleCallback)AddedCallback;
    g_BulletManagerCalcChain.deletedCallback = (ChainLifecycleCallback)DeletedCallback;
    g_BulletManagerCalcChain.arg = mgr;
    if (g_Chain.AddToCalcChain(&g_BulletManagerCalcChain, 12))
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

void BulletManager::CutChain()
{
    g_Chain.Cut(&g_BulletManagerCalcChain);
    g_Chain.Cut(&g_BulletManagerDrawChain);
    memset(&g_BulletManager, 0, sizeof(BulletManager));
}

void BulletManager::StopBulletMovement()
{
    Bullet *bullet;
    i32 i;

    bullet = g_BulletManager.bullets;
    for (i = 0; i < MAX_BULLETS; i++, bullet++)
    {
        if (bullet->state == BULLET_INACTIVE)
        {
            continue;
        }

        bullet->velocity = ZunVec3(0.0f, 0.0f, 0.0f);
        bullet->unused_ba4 = ZunVec3(0.0f, 0.0f, 0.0f);
        bullet->angularVelocity = 0.0f;
        bullet->acceleration = 0.0f;
        bullet->speed = 0.0f;
        bullet->spriteOffset = 0;
        g_AnmManager->SetActiveSprite(&bullet->sprites.spriteBullet,
                                      (i32)bullet->sprites.spriteBullet.baseSpriteIdx +
                                          (i32)bullet->spriteOffset);
    }
}

BulletCommand *Bullet::AddCommand(i32 command, i32 flag, u32 type)
{
    BulletCommand *bulletCommand = &this->commands[command];
    bulletCommand->type = type;
    bulletCommand->flag = flag;
    this->moreFlags |= type;
    this->curCmdIdx = 0;
    return bulletCommand;
}

BulletCommand *EnemyBulletShooter::AddCommand(i32 command, i32 flag, u32 type)
{
    BulletCommand *bulletCommand = &this->commands[command];
    bulletCommand->type = type;
    bulletCommand->flag = flag;
    this->flags |= type;
    return bulletCommand;
}

void Bullet::AddAngleAccelCommand(i32 command, i32 flag, i32 duration, f32 angle, f32 speed)
{
    BulletCommand *bulletCommand;

    bulletCommand = AddCommand(command, flag, 0x20);
    bulletCommand->duration = duration;
    bulletCommand->speed = speed;
    bulletCommand->angle = angle;
}

void Bullet::AddTargetVelocityCommand(i32 command, i32 flag, i32 duration, f32 speed, f32 angle)
{
    BulletCommand *bulletCommand;

    bulletCommand = AddCommand(command, flag, 0x10);
    bulletCommand->duration = duration;
    bulletCommand->speed = speed;
    bulletCommand->angle = angle;
}

void EnemyBulletShooter::AddAngleAccelCommand(i32 command, i32 flag, i32 duration, f32 angle,
                                              f32 speed)
{
    BulletCommand *bulletCommand = AddCommand(command, flag, 0x20);
    bulletCommand->duration = duration;
    bulletCommand->speed = speed;
    bulletCommand->angle = angle;
}

void EnemyBulletShooter::AddDirChangeCommand(i32 command, i32 flag, i32 duration, i32 loopCount,
                                             f32 speed, f32 angle)
{
    BulletCommand *bulletCommand = AddCommand(command, flag, 0x80);
    bulletCommand->duration = duration;
    bulletCommand->loopCount = loopCount;
    bulletCommand->speed = speed;
    bulletCommand->angle = angle;
}

void EnemyBulletShooter::AddTargetVelocityCommand(i32 command, i32 flag, i32 duration, f32 speed,
                                                  f32 angle)
{
    BulletCommand *bulletCommand = AddCommand(command, flag, 0x10);
    bulletCommand->duration = duration;
    bulletCommand->speed = speed;
    bulletCommand->angle = angle;
}

void EnemyBulletShooter::AddSpawnDelayCommand(i32 command, i32 flag, i32 duration)
{
    BulletCommand *bulletCommand = AddCommand(command, flag, 0x2000);
    bulletCommand->duration = duration;
}
