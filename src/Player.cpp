#include "Player.hpp"

#include "AnmIdx.hpp"
#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "BombData.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "EffectManager.hpp"
#include "EnemyManager.hpp"
#include "FileSystem.hpp"
#include "GameManager.hpp"
#include "GameWindow.hpp"
#include "Gui.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Touch.hpp"
#include "ZunMath.hpp"
#include "dxutil.hpp"
#include "utils.hpp"

ShtFunc1 g_ShtFireFuncs[6] = {
    NULL,
    ShtData::FireBulletDefault,
    ShtData::FireOrbBulletUnfocused,
    ShtData::FireOrbBulletFocused,
    ShtData::FireHomingBullet,
    ShtData::FireRotatingOrbBullet,
};

ShtFunc2 g_ShtUpdateFuncs[6] = {
    NULL,
    ShtData::UpdateHomingBullet,
    ShtData::UpdateHomingBulletFocused,
    ShtData::UpdateUpwardAcceleratingBullet,
    ShtData::UpdateOrbLaser,
    ShtData::UpdatePlayerLaser,
};

ShtFunc3 g_ShtDrawFuncs[2] = {
    NULL,
    ShtData::DrawBulletWithTrail,
};

ShtFunc4 g_ShtHitFuncs[4] = {
    NULL,
    ShtData::OnMissileHit,
    ShtData::SpawnHitParticles,
    NULL,
};

const char *g_ShooterTable[6] = {
    "data/ply00a.sht", "data/ply00b.sht", "data/ply01a.sht",
    "data/ply01b.sht", "data/ply02a.sht", "data/ply02b.sht",
};

const char *g_ShooterTableFocus[6] = {
    "data/ply00as.sht", "data/ply00bs.sht", "data/ply01as.sht",
    "data/ply01bs.sht", "data/ply02as.sht", "data/ply02bs.sht",
};

Player g_Player;

void DefaultFireBulletCallback(Player *player, PlayerBullet *bullet, ShtEntry *shtEntry)
{
    if (shtEntry->option == 0)
    {
        bullet->pos = player->pos;
    }
    else
    {
        bullet->pos = player->optionsPosition[shtEntry->option - 1];
    }
    *bullet->GetPosX() += shtEntry->offset.x;
    *bullet->GetPosY() += shtEntry->offset.y;
    bullet->pos.z = 0.495f;
    bullet->prevPos = bullet->pos;
    bullet->hitboxSize.x = shtEntry->hitboxSize.x;
    bullet->hitboxSize.y = shtEntry->hitboxSize.y;
    bullet->hitboxSize.z = 1.0f;
    bullet->prevAngle = bullet->angle = shtEntry->angle;
    bullet->speed = shtEntry->speed;
    bullet->velocity.x = cosf(shtEntry->angle) * shtEntry->speed;
    bullet->velocity.y = sinf(shtEntry->angle) * shtEntry->speed;
    bullet->timer = 0;
    bullet->bulletState2 = shtEntry->bulletState2;
    bullet->damage = shtEntry->damage;
    if (shtEntry->soundIdx >= 0)
    {
        g_SoundPlayer.PlaySoundByIdx(shtEntry->soundIdx, 0);
    }
    g_AnmManager->SetAnmIdxAndExecuteScript(&bullet->vm, shtEntry->anmFileIdx);
}

i32 ShtData::FireBulletDefault(Player *player, PlayerBullet *bullet, i32 fireTime,
                               ShtEntry *shtEntry)
{
    if (fireTime % shtEntry->fireInterval == shtEntry->fireOffset)
    {
        DefaultFireBulletCallback(player, bullet, shtEntry);
        return 1;
    }
    return 0;
}

i32 ShtData::FireOrbBulletUnfocused(Player *player, PlayerBullet *bullet, i32 fireTime,
                                    ShtEntry *shtEntry)
{
    (void)fireTime;

    i32 fireOffset = shtEntry->fireOffset;

    if (player->timers[fireOffset].bullet)
    {
        if (player->shtEntries[fireOffset] != shtEntry)
        {
            player->timers[fireOffset].bullet->vm.pendingInterrupt = 1;
            player->timers[fireOffset].bullet = NULL;
        }
        return 0;
    }

    if (player->optionState != OPTION_UNFOCUSED)
    {
        return 0;
    }

    player->timers[fireOffset].timer = shtEntry->fireInterval;
    player->timers[fireOffset].bullet = bullet;
    bullet->timerIdx = fireOffset;
    bullet->optionId = (i16)shtEntry->option;
    bullet->offset.x = shtEntry->offset.x;
    bullet->offset.y = shtEntry->offset.y;
    DefaultFireBulletCallback(player, bullet, shtEntry);
    player->shtEntries[fireOffset] = shtEntry;
    return 1;
}

i32 ShtData::FireOrbBulletFocused(Player *player, PlayerBullet *bullet, i32 fireTime,
                                  ShtEntry *shtEntry)
{
    (void)fireTime;

    i32 fireOffset = shtEntry->fireOffset;

    if (player->timers[fireOffset].bullet)
    {
        if (player->shtEntries[fireOffset] != shtEntry)
        {
            player->timers[fireOffset].bullet->vm.pendingInterrupt = 1;
            player->timers[fireOffset].bullet = NULL;
        }
        return 0;
    }

    if (player->optionState != OPTION_FOCUSED)
    {
        return 0;
    }

    player->timers[fireOffset].timer = 999;
    player->timers[fireOffset].bullet = bullet;
    bullet->timerIdx = fireOffset;
    bullet->optionId = (i16)shtEntry->option;
    bullet->offset.x = shtEntry->offset.x;
    bullet->offset.y = shtEntry->offset.y;
    bullet->trailLength = shtEntry->fireInterval;
    DefaultFireBulletCallback(player, bullet, shtEntry);
    for (i32 i = ARRAY_SIZE_SIGNED(bullet->posHistory) - 1; i >= 0; i--)
    {
        bullet->posHistory[i].x = -999.0f;
    }
    bullet->pos.x = -999.0f;
    player->shtEntries[fireOffset] = shtEntry;
    return 1;
}

i32 ShtData::FireHomingBullet(Player *player, PlayerBullet *bullet, i32 fireTime,
                              ShtEntry *shtEntry)
{
    f32 angle;
    f32 speed;

    if (fireTime % shtEntry->fireInterval == shtEntry->fireOffset)
    {
        DefaultFireBulletCallback(player, bullet, shtEntry);
        if (player->sakuyaTargetPosition.x > -100.0f)
        {
            angle = utils::AddNormalizeAngle(atan2f(player->sakuyaTargetPosition.y - bullet->pos.y,
                                                    player->sakuyaTargetPosition.x - bullet->pos.x),
                                             shtEntry->angle + ZUN_PI / 2.0f);
            speed = shtEntry->speed * 1.5f;
            (*(ZunVec3 *)&bullet->velocity).FromAngleMagnitude(angle, speed);
            bullet->angle = angle;
        }
        return 1;
    }

    return 0;
}

i32 ShtData::FireRotatingOrbBullet(Player *player, PlayerBullet *bullet, i32 fireTime,
                                   ShtEntry *shtEntry)
{
    f32 angle;
    f32 speed;

    if (fireTime % shtEntry->fireInterval == shtEntry->fireOffset)
    {
        DefaultFireBulletCallback(player, bullet, shtEntry);
        angle = utils::AddNormalizeAngle(player->optionAngle, shtEntry->angle + ZUN_PI / 2.0f);
        speed = shtEntry->speed;
        (*(ZunVec3 *)&bullet->velocity).FromAngleMagnitude(angle, speed);
        bullet->angle = angle;

        return 1;
    }

    return 0;
}

i32 ShtData::UpdateHomingBullet(Player *player, PlayerBullet *bullet)
{
    f32 length;
    f32 x;
    f32 y;

    if (bullet->bulletState == 1)
    {
        if (player->positionOfLastEnemyHit.x > -100.0f && bullet->timer.GetCurrent() < 40 &&
            bullet->timer.HasTicked())
        {
            x = player->positionOfLastEnemyHit.x - bullet->pos.x;
            y = player->positionOfLastEnemyHit.y - bullet->pos.y;
            length = sqrtf(x * x + y * y) / (bullet->speed / 4.0f);

            if (length < 1.0f)
            {
                length = 1.0f;
            }

            x = x / length + bullet->velocity.x;
            y = y / length + bullet->velocity.y;
            length = sqrtf(x * x + y * y);

            bullet->speed = length > 10.0f ? 10.0f : length;

            if (bullet->speed < 1.0f)
            {
                bullet->speed = 1.0f;
            }

            bullet->velocity.x = x * bullet->speed / length;
            bullet->velocity.y = y * bullet->speed / length;
        }
        else
        {
            if (bullet->speed < 10.0f)
            {
                bullet->speed += 1.0f / 3.0f;
                x = bullet->velocity.x;
                y = bullet->velocity.y;
                length = sqrtf(x * x + y * y);
                bullet->velocity.x = x * bullet->speed / length;
                bullet->velocity.y = y * bullet->speed / length;
            }
        }
    }
    return 0;
}

i32 ShtData::UpdateHomingBulletFocused(Player *player, PlayerBullet *bullet)
{
    f32 length;
    f32 x;
    f32 y;

    if (bullet->bulletState == 1)
    {
        if (player->positionOfLastEnemyHit.x > -100.0f && bullet->timer.GetCurrent() < 40 &&
            bullet->timer.HasTicked())
        {
            x = player->positionOfLastEnemyHit.x - bullet->pos.x;
            y = player->positionOfLastEnemyHit.y - bullet->pos.y;
            length = sqrtf(x * x + y * y) / (bullet->speed / 4.0f);
            if (length < 1.0f)
            {
                length = 1.0f;
            }
            x = x / length + bullet->velocity.x;
            y = y / length + bullet->velocity.y;
            length = sqrtf(x * x + y * y);
            bullet->speed = length > 18.0f ? 18.0f : length;
            if (bullet->speed < 1.0f)
            {
                bullet->speed = 1.0f;
            }
            bullet->velocity.x = x * bullet->speed / length;
            bullet->velocity.y = y * bullet->speed / length;
        }
        else
        {
            if (bullet->speed < 18.0f)
            {
                bullet->speed = bullet->speed + 0.6f;
                x = bullet->velocity.x;
                y = bullet->velocity.y;
                length = sqrtf(x * x + y * y);
                bullet->velocity.x = x * bullet->speed / length;
                bullet->velocity.y = y * bullet->speed / length;
            }
        }
    }
    return 0;
}

i32 ShtData::UpdateUpwardAcceleratingBullet(Player *player, PlayerBullet *bullet)
{
    (void)player;

    if (bullet->bulletState == 1)
    {
        bullet->velocity.y = bullet->velocity.y - (g_Rng.GetRandomFloatInRange(0.1f) + 0.27f);
    }
    return 0;
}

i32 ShtData::UpdateOrbLaser(Player *player, PlayerBullet *bullet)
{
    if (player->timers[bullet->timerIdx].bullet != bullet && bullet->vm.isStopped)
    {
        bullet->vm.pendingInterrupt = 1;
    }
    if ((g_Gui.HasCurrentMsgIdx() || player->bombInfo.isInUse) &&
        20 < player->timers[bullet->timerIdx].timer.GetCurrent())
    {
        player->timers[bullet->timerIdx].timer = 20;
    }
    if (player->timers[bullet->timerIdx].timer <= 0)
    {
        player->timers[bullet->timerIdx].timer = 0;
        player->timers[bullet->timerIdx].bullet = NULL;
        bullet->bulletState = 0;
        return 1;
    }

    if (player->timers[bullet->timerIdx].timer <= 70 && bullet->vm.isStopped)
    {
        bullet->vm.pendingInterrupt = 1;
    }
    bullet->pos = player->optionsPosition[bullet->optionId - 1];
    bullet->pos.x += bullet->offset.x;
    bullet->pos.z = 0.44f;
    if (player->playerState == PLAYER_STATE_DEAD)
    {
        return 1;
    }
    else
    {
        bullet->vm.scale.y = bullet->pos.y / 14.0f;
        bullet->hitboxSize.y = bullet->pos.y;
        bullet->pos.y = bullet->pos.y / 2.0f;
        return 0;
    }
}

i32 ShtData::UpdatePlayerLaser(Player *player, PlayerBullet *bullet)
{
    i32 i;

    if (player->timers[bullet->timerIdx].bullet != bullet && bullet->vm.isStopped)
    {
        bullet->vm.pendingInterrupt = 1;
    }
    if ((g_Gui.HasCurrentMsgIdx() || player->bombInfo.isInUse) &&
        player->timers[bullet->timerIdx].timer.GetCurrent() > 20)
    {
        player->timers[bullet->timerIdx].timer = 20;
    }
    if (player->timers[bullet->timerIdx].timer <= 0)
    {
        player->timers[bullet->timerIdx].timer = 0;
        bullet->bulletState = 0;
        player->timers[bullet->timerIdx].bullet = NULL;
        return 1;
    }

    if (player->timers[bullet->timerIdx].timer <= 70 && bullet->vm.isStopped)
    {
        bullet->vm.pendingInterrupt = 1;
    }
    for (i = 0; i < bullet->trailLength; i++)
    {
        if (bullet->posHistory[i].x >= -900.0f)
        {
            player->bombDamageBoxes[i + 96].pos = bullet->posHistory[i];
            player->bombDamageBoxes[i + 96].lifetime = 1;
            player->bombDamageBoxes[i + 96].size = bullet->hitboxSize;
        }
    }
    for (i = ARRAY_SIZE_SIGNED(bullet->posHistory) - 1; i > 0; i--)
    {
        bullet->posHistory[i] = bullet->posHistory[i - 1];
    }
    bullet->posHistory[0] = bullet->pos;
    if (player->playerState == PLAYER_STATE_DEAD)
    {
        return 1;
    }
    else
    {
        bullet->pos = player->pos;
        bullet->pos.x += bullet->offset.x;
        bullet->pos.z = 0.44f;
        bullet->vm.scale.y = (bullet->pos.y + 64.0f) / 14.0f;
        bullet->hitboxSize.y = player->pos.y + 64.0f;
        bullet->pos.y = bullet->pos.y / 2.0f - 32.0f;
        return 0;
    }
}

i32 ShtData::DrawBulletWithTrail(Player *player, PlayerBullet *bullet)
{
    (void)player;

    i32 i;
    i32 origAlpha;
    i32 origPrevAlpha;

    origAlpha = bullet->vm.color.bytes.a;
    origPrevAlpha = bullet->vm.prevColor.bytes.a;
    for (i = 0; i < bullet->trailLength; i++)
    {
        if (bullet->posHistory[i].x == -999.0f)
        {
            break;
        }

        ZunVec3 prevPos = bullet->posHistory[i];
        if (i + 1 < 16 && bullet->posHistory[i + 1].x != -999.0f)
        {
            prevPos = bullet->posHistory[i + 1];
        }

        bullet->vm.pos = prevPos.Lerp(bullet->posHistory[i], g_RenderAlpha);

        bullet->vm.prevColor.bytes.a = bullet->vm.color.bytes.a =
            origAlpha - origAlpha * i / bullet->trailLength;

        *bullet->GetVmPosX() += g_GameManager.arcadeRegionTopLeftPos.x;
        *bullet->GetVmPosY() += g_GameManager.arcadeRegionTopLeftPos.y;

        g_AnmManager->Draw(&bullet->vm);
    }
    bullet->vm.color.bytes.a = origAlpha;
    bullet->vm.prevColor.bytes.a = origPrevAlpha;
    return 0;
}

i32 ShtData::OnMissileHit(Player *player, PlayerBullet *bullet, ZunVec3 *pos)
{
    (void)player;

    f32 angle;

    if (bullet->bulletState == 2)
    {
        if (bullet->timer.GetCurrent() % 2 != 0)
        {
            return 1;
        }
        bullet->damage = bullet->damage / 3;
        if (bullet->damage == 0)
        {
            bullet->damage = 1;
        }
        bullet->velocity.x *= 0.88f;
        bullet->velocity.y *= 0.88f;
    }
    else
    {
        angle = g_Rng.GetRandomFloatInRange(ZUN_PI / 2.0f) - ZUN_3PI / 4.0f;
        switch (bullet->vm.anmFileIdx)
        {
        case 1089:
            bullet->hitboxSize.x = 32.0f;
            bullet->hitboxSize.y = 32.0f;
            (*(ZunVec3 *)&bullet->velocity).FromAngleMagnitude(angle, 4.0f);
            break;
        case 1090:
            bullet->hitboxSize.x = 42.0;
            bullet->hitboxSize.y = 42.0;
            (*(ZunVec3 *)&bullet->velocity).FromAngleMagnitude(angle, 4.0f);
            break;
        case 1091:
            bullet->hitboxSize.x = 48.0f;
            bullet->hitboxSize.y = 48.0f;
            (*(ZunVec3 *)&bullet->velocity).FromAngleMagnitude(angle, 4.0f);
            break;
        case 1092:
            bullet->hitboxSize.x = 56.0f;
            bullet->hitboxSize.y = 56.0f;
            (*(ZunVec3 *)&bullet->velocity).FromAngleMagnitude(angle, 4.0f);
            break;
        case 1093:
            bullet->hitboxSize.x = 48.0f;
            bullet->hitboxSize.y = 48.0f;
            (*(ZunVec3 *)&bullet->velocity).FromAngleMagnitude(angle, 6.0f);
            break;
        case 1094:
            bullet->hitboxSize.x = 64.0f;
            bullet->hitboxSize.y = 64.0f;
            (*(ZunVec3 *)&bullet->velocity).FromAngleMagnitude(angle, 6.0f);
            break;
        case 1095:
            bullet->hitboxSize.x = 80.0f;
            bullet->hitboxSize.y = 80.0f;
            (*(ZunVec3 *)&bullet->velocity).FromAngleMagnitude(angle, 6.0f);
            break;
        case 1096:
            bullet->hitboxSize.x = 96.0f;
            bullet->hitboxSize.y = 96.0f;
            (*(ZunVec3 *)&bullet->velocity).FromAngleMagnitude(angle, 6.0f);
        }
    }
    if (bullet->timer.GetCurrent() % 6 == 0)
    {
        g_EffectManager.SpawnEffect(5, pos, 1, 0xffffffff);
    }
    return 0;
}

i32 ShtData::SpawnHitParticles(Player *player, PlayerBullet *bullet, ZunVec3 *pos)
{
    ZunVec3 particlePos;

    player->bombParticleTime++;
    if (player->bombParticleTime % 8 == 0)
    {
        particlePos = *pos;
        particlePos.x = bullet->pos.x;
        g_EffectManager.SpawnEffect(5, &particlePos, 1, 0xffffffff);
    }
    return 0;
}

void Player::SpawnBullets(Player *player, u32 timer)
{
    ShtEntry *entry;
    i32 ret;
    PlayerBullet *bullet;
    ShtLevel *level;
    i32 i;

    level = !player->isFocus ? player->shooterData->levels : player->shooterDataFocus->levels;

    while ((i32)g_GameManager.globals->currentPower >= level->requiredPower)
    {
        level++;
    }

    entry = level->entry;
    bullet = player->bullets;
    for (i = 0; i < ARRAY_SIZE_SIGNED(player->bullets); i++, bullet++)
    {
        if (bullet->bulletState != 0)
        {
            continue;
        }

    loop_with_goto_for_some_reason:
        if (entry->fireCallback)
        {
            ret = entry->fireCallback(player, bullet, timer, entry);
        }
        else
        {
            ret = ShtData::FireBulletDefault(player, bullet, timer, entry);
        }
        if (ret == 1)
        {
            bullet->vm.zWriteDisable = 1;
            bullet->bulletState = 1;
            bullet->shtEntry = entry;
            bullet->updateCallback = bullet->shtEntry->updateCallback;
            bullet->drawCallback = bullet->shtEntry->drawCallback;
            bullet->hitCallback = bullet->shtEntry->hitCallback;
        }
        entry++;
        if (entry->fireInterval < 0)
        {
            return;
        }

        if (!ret)
        {
            goto loop_with_goto_for_some_reason;
        }
    }
}

void Player::UpdateShots()
{
    PlayerBullet *bullet;
    i32 i;

    if (this->optionState != OPTION_FOCUSED && this->timers[2].bullet)
    {
        this->timers[2].bullet->bulletState = 0;
        this->timers[2].bullet = NULL;
    }
    if (this->optionState != OPTION_UNFOCUSED)
    {
        if (this->timers[0].bullet)
        {
            this->timers[0].bullet->vm.pendingInterrupt = 1;
            this->timers[0].bullet = NULL;
        }
        if (this->timers[1].bullet)
        {
            this->timers[1].bullet->vm.pendingInterrupt = 1;
            this->timers[1].bullet = NULL;
        }
    }
    if (this->playerState == PLAYER_STATE_DEAD)
    {
        for (i = 0; i < ARRAY_SIZE_SIGNED(this->timers); i++)
        {
            if (this->timers[i].bullet)
            {
                this->timers[i].bullet->bulletState = 0;
                this->timers[i].bullet = NULL;
            }
        }
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->timers); i++)
    {
        if (!this->timers[i].bullet)
        {
            continue;
        }
        if (this->timers[i].timer.GetCurrent() > 0 && this->timers[i].timer.GetCurrent() < 999)
        {
            this->timers[i].timer--;
        }
        if (this->fireBulletTimer.GetCurrent() < 0 && this->timers[i].timer.GetCurrent() > 50)
        {
            this->timers[i].timer = 50;
        }
        if (this->timers[i].timer.GetCurrent() == 0)
        {
            this->timers[i].bullet = NULL;
        }
    }
    bullet = this->bullets;
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->bullets); i++, bullet++)
    {
        if (bullet->bulletState == 0)
        {
            continue;
        }

        bullet->prevPos = bullet->pos;
        bullet->prevAngle = bullet->angle;

        if (bullet->updateCallback && bullet->updateCallback(this, bullet))
        {
            bullet->bulletState = 0;
            continue;
        }

        *bullet->GetPosX() += bullet->velocity.x * g_Supervisor.effectiveFramerateMultiplier;
        *bullet->GetPosY() += bullet->velocity.y * g_Supervisor.effectiveFramerateMultiplier;
        if (bullet->bulletState2 != 4 && bullet->bulletState2 != 5 &&
            !g_GameManager.IsInBounds(bullet->pos.x, bullet->pos.y, bullet->vm.sprite->widthPx,
                                      bullet->vm.sprite->heightPx))
        {
            bullet->bulletState = 0;
        }
        if (g_AnmManager->ExecuteScript(&bullet->vm))
        {
            bullet->bulletState = 0;
        }
        bullet->timer++;
    }
}

void Player::DrawBullets()
{
    PlayerBullet *bullet;
    i32 i;

    bullet = this->bullets;
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->bullets); i++, bullet++)
    {
        if (bullet->bulletState != 1)
        {
            continue;
        }

        if (bullet->vm.autoRotate)
        {
            f32 angle = utils::AddNormalizeAngle(
                utils::LerpAngle(bullet->prevAngle, bullet->angle, g_RenderAlpha), ZUN_PI / 2.0f);
            bullet->vm.rotation.z = angle;
            bullet->vm.prevRotation.z = angle;
            bullet->vm.updateRotation = 1;
        }

        ZunVec3 drawPos = bullet->prevPos.Lerp(bullet->pos, g_RenderAlpha);
        bullet->vm.pos.x = g_GameManager.arcadeRegionTopLeftPos.x + drawPos.x;
        bullet->vm.pos.y = g_GameManager.arcadeRegionTopLeftPos.y + drawPos.y;
        bullet->vm.pos.z = 0.4f;
        g_AnmManager->Draw(&bullet->vm);
        if (bullet->drawCallback)
        {
            bullet->drawCallback(this, bullet);
        }
    }
}

void Player::DrawBulletExplosions()
{
    PlayerBullet *bullet;
    i32 i;

    bullet = this->bullets;
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->bullets); i++, bullet++)
    {
        if (bullet->bulletState != 2)
        {
            continue;
        }

        if (bullet->vm.autoRotate)
        {
            f32 angle = utils::AddNormalizeAngle(bullet->angle, ZUN_PI / 2.0f);
            bullet->vm.rotation.z = angle;
            bullet->vm.updateRotation = 1;
        }

        ZunVec3 drawPos = bullet->prevPos.Lerp(bullet->pos, g_RenderAlpha);
        bullet->vm.pos.x = g_GameManager.arcadeRegionTopLeftPos.x + drawPos.x;
        bullet->vm.pos.y = g_GameManager.arcadeRegionTopLeftPos.y + drawPos.y;
        bullet->vm.pos.z = 0.4f;
        g_AnmManager->Draw(&bullet->vm);
    }
}

i32 Player::UpdateFireBulletTimer()
{
    if (this->fireBulletTimer.GetCurrent() < 0)
    {
        return 0;
    }
    if (this->fireBulletTimer.HasTicked() &&
        (!g_Player.bombInfo.isInUse || g_GameManager.character != CHAR_MARISA ||
         g_GameManager.shotType != 1))
    {
        SpawnBullets(this, this->fireBulletTimer.GetCurrent());
    }
    this->fireBulletTimer++;
    if (this->fireBulletTimer.GetCurrent() >= 30 || this->playerState == PLAYER_STATE_DEAD ||
        this->playerState == PLAYER_STATE_SPAWNING)
    {

        this->fireBulletTimer = -1;
    }
    return 0;
}

void Player::StartFireBulletTimer()
{
    if (this->fireBulletTimer.GetCurrent() < 0)
    {
        this->fireBulletTimer = 0;
    }
}

i32 Player::CalcDamageToEnemy(ZunVec3 *center, ZunVec3 *size, i32 *param_3)
{
    ZunVec3 bulletTopLeft;
    i32 damage;
    ZunVec3 enemyTopLeft;
    ZunVec3 bulletBottomRight;
    ZunVec3 enemyBottomRight;
    i32 i;
    PlayerBullet *bullet;

    damage = 0;
    if (!this->invulnerabilityTimer.HasTicked())
    {
        return 0;
    }

    enemyTopLeft.x = center->x - size->x * 0.5f;
    enemyTopLeft.y = center->y - size->y * 0.5f;
    enemyBottomRight.x = center->x + size->x * 0.5f;
    enemyBottomRight.y = center->y + size->y * 0.5f;

    bullet = this->bullets;
    if (param_3)
    {
        *param_3 = 0;
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->bullets); i++, bullet++)
    {
        if (bullet->bulletState == 0 || (bullet->bulletState != 1 && bullet->bulletState2 != 3))
        {
            continue;
        }

        SetVecCorners(&bulletTopLeft, &bulletBottomRight, &bullet->pos, &bullet->hitboxSize);

        if (bulletTopLeft.y > enemyBottomRight.y || bulletTopLeft.x > enemyBottomRight.x ||
            bulletBottomRight.y < enemyTopLeft.y || bulletBottomRight.x < enemyTopLeft.x)
        {
            continue;
        }

        if (bullet->bulletState2 == 4 || bullet->bulletState2 == 5)
        {
            if (bullet->timer.current % 2 != 0)
            {
                continue;
            }
        }
        if (bullet->hitCallback && bullet->hitCallback(this, bullet, center))
        {
            continue;
        }

        if (!this->bombInfo.isInUse)
        {
            damage += bullet->damage;
        }
        else
        {
            damage += bullet->damage / 3 != 0 ? bullet->damage / 3 : 1;
        }
        if (bullet->bulletState2 != 4 && bullet->bulletState2 != 5)
        {
            if (bullet->bulletState == 1)
            {
                g_AnmManager->SetAnmIdxAndExecuteScript(&bullet->vm, bullet->vm.anmFileIdx + 32);
                g_EffectManager.SpawnEffect(5, &bullet->pos, 1, 0xffffffff);
                bullet->pos.z = 0.1f;
            }
            bullet->bulletState = 2;
            if (bullet->bulletState2 != 3)
            {
                bullet->velocity.x /= 8.0f;
                bullet->velocity.y /= 8.0f;
            }
        }
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->bombDamageBoxes); i++)
    {
        if (this->bombDamageBoxes[i].size.x <= 0.0f)
        {
            continue;
        }

        bulletTopLeft = this->bombDamageBoxes[i].pos - this->bombDamageBoxes[i].size / 2.0f;
        bulletBottomRight = this->bombDamageBoxes[i].pos + this->bombDamageBoxes[i].size / 2.0f;

        if (bulletTopLeft.x > enemyBottomRight.x || bulletBottomRight.x < enemyTopLeft.x ||
            bulletTopLeft.y > enemyBottomRight.y || bulletBottomRight.y < enemyTopLeft.y)
        {
            continue;
        }

        damage += this->bombDamageBoxes[i].lifetime;
        this->bombDamageBoxes[i].damage += this->bombDamageBoxes[i].lifetime;
        this->bombParticleTime++;
        if (this->bombParticleTime % 4 == 0)
        {
            if (i < ARRAY_SIZE_SIGNED(this->bombClearBoxes))
            {
                g_EffectManager.SpawnEffect(3, center, 1, 0xffffffff);
            }
            else
            {
                g_EffectManager.SpawnEffect(5, center, 1, 0xffffffff);
            }
        }
        if (this->bombInfo.isInUse && param_3)
        {
            *param_3 = 1;
        }
    }
    return damage;
}

void Player::RebuildBombBoxCache()
{
    this->numActiveBombClearBoxes = 0;
    this->dirtyBombBoxes = false;

    for (i32 i = 0; i < ARRAY_SIZE_SIGNED(this->bombClearBoxes); i++)
    {
        BombClearBox *bomb = &this->bombClearBoxes[i];
        if (bomb->pos.z != 0.0f)
        {
            CachedBombClearBox *c =
                &this->activeBombClearBoxesCache[this->numActiveBombClearBoxes++];
            c->isBox = true;
            c->minX = bomb->pos.x - bomb->pos.z * 0.5f;
            c->maxX = bomb->pos.x + bomb->pos.z * 0.5f;
            c->minY = bomb->pos.y - bomb->size.x * 0.5f;
            c->maxY = bomb->pos.y + bomb->size.x * 0.5f;
            c->itemType = bomb->itemType;
        }
        else if (bomb->size.y != 0.0f)
        {
            CachedBombClearBox *c =
                &this->activeBombClearBoxesCache[this->numActiveBombClearBoxes++];
            c->isBox = false;
            c->cx = bomb->pos.x;
            c->cy = bomb->pos.y;
            c->radiusSq = bomb->size.y * bomb->size.y;
            c->itemType = bomb->itemType;
        }
    }
}

i32 Player::CheckBombGraze(ZunVec3 *center, ZunVec3 *size)
{
    if (this->dirtyBombBoxes)
    {
        RebuildBombBoxCache();
    }

    if (this->numActiveBombClearBoxes == 0)
    {
        return 0;
    }

    f32 halfW = size->x * 0.5f;
    f32 halfH = size->y * 0.5f;
    f32 bulletMinX = center->x - halfW;
    f32 bulletMinY = center->y - halfH;
    f32 bulletMaxX = center->x + halfW;
    f32 bulletMaxY = center->y + halfH;

    for (i32 i = 0; i < this->numActiveBombClearBoxes; i++)
    {
        const CachedBombClearBox &c = this->activeBombClearBoxesCache[i];
        if (c.isBox)
        {
            if (!(c.minX > bulletMaxX || c.maxX < bulletMinX || c.minY > bulletMaxY ||
                  c.maxY < bulletMinY))
            {
                this->itemType = c.itemType;
                return 2;
            }
        }
        else
        {
            f32 dx = center->x - c.cx;
            f32 dy = center->y - c.cy;
            if (dx * dx + dy * dy < c.radiusSq)
            {
                this->itemType = c.itemType;
                return 2;
            }
        }
    }

    return 0;
}

i32 Player::CalcKillboxCollision(ZunVec3 *center, ZunVec3 *size)
{
    ZunVec3 killboxBottomRight;
    ZunVec3 killboxTopLeft;

    this->itemType = ITEM_POINT_BULLET;
    if (CheckBombGraze(center, size))
    {
        return 2;
    }

    killboxTopLeft.x = center->x - size->x / 2.0f;
    killboxTopLeft.y = center->y - size->y / 2.0f;
    killboxBottomRight.x = center->x + size->x / 2.0f;
    killboxBottomRight.y = center->y + size->y / 2.0f;
    if (this->hitboxTopLeft.x > killboxBottomRight.x ||
        this->hitboxTopLeft.y > killboxBottomRight.y ||
        this->hitboxBottomRight.x < killboxTopLeft.x ||
        this->hitboxBottomRight.y < killboxTopLeft.y)
    {
        return 0;
    }

    g_ReplayManager->replayEventFlags = g_ReplayManager->replayEventFlags | 2;
    if (this->playerState == PLAYER_STATE_BORDER)
    {
        g_Player.BreakBorder();
        return 1;
    }
    if (this->playerState != PLAYER_STATE_ALIVE)
    {
        return 1;
    }

    g_GameManager.RerollRng();
    Die();
    return 1;
}

i32 Player::CheckGraze(ZunVec3 *center, ZunVec3 *size)
{
    ZunVec3 bulletBottomRight;
    ZunVec3 bulletTopLeft;

    this->itemType = ITEM_POINT_BULLET;

    if (CheckBombGraze(center, size))
    {
        return 2;
    }

    bulletTopLeft.x = center->x - size->x / 2.0f - 20.0f;
    bulletTopLeft.y = center->y - size->y / 2.0f - 20.0f;
    bulletBottomRight.x = center->x + size->x / 2.0f + 20.0f;
    bulletBottomRight.y = center->y + size->y / 2.0f + 20.0f;

    if (this->playerState == PLAYER_STATE_DEAD || this->playerState == PLAYER_STATE_SPAWNING)
    {
        return 0;
    }

    if (this->grazeTopLeft.x > bulletBottomRight.x || this->grazeBottomRight.x < bulletTopLeft.x ||
        this->grazeTopLeft.y > bulletBottomRight.y || this->grazeBottomRight.y < bulletTopLeft.y)
    {
        return 0;
    }

    ScoreGraze(center);
    return 1;
}

i32 Player::CalcItemBoxCollision(ZunVec3 *center, ZunVec3 *size)
{
    ZunVec3 itemBottomRight;
    ZunVec3 itemTopLeft;

    if (this->playerState != PLAYER_STATE_ALIVE && this->playerState != PLAYER_STATE_INVULNERABLE &&
        this->playerState != PLAYER_STATE_BORDER)
    {
        return 0;
    }

    itemTopLeft = *center - *size / 2.0f;
    itemBottomRight = *center + *size / 2.0f;

    if (this->grabItemTopLeft.x > itemBottomRight.x ||
        this->grabItemBottomRight.x < itemTopLeft.x ||
        this->grabItemTopLeft.y > itemBottomRight.y || this->grabItemBottomRight.y < itemTopLeft.y)
    {
        return 0;
    }

    return 1;
}

i32 Player::CalcLaserHitbox(ZunVec3 *center, ZunVec3 *size, ZunVec3 *origin, f32 rotation,
                            i32 canGraze)
{
    ZunVec3 playerRelativeTopLeft;
    ZunVec3 playerRelativeBottomRight;
    ZunVec3 laserTopLeft;
    ZunVec3 laserBottomRight;

    laserTopLeft = this->pos - *origin;
    utils::Rotate(&laserBottomRight, &laserTopLeft, rotation);
    laserBottomRight.z = 0;
    laserTopLeft = laserBottomRight + *origin;
    playerRelativeTopLeft = laserTopLeft - this->hitboxSize;
    playerRelativeBottomRight = laserTopLeft + this->hitboxSize;

    laserTopLeft = *center - *size / 2.0f;
    laserBottomRight = *center + *size / 2.0f;
    if (!(playerRelativeTopLeft.x > laserBottomRight.x ||
          playerRelativeBottomRight.x < laserTopLeft.x ||
          playerRelativeTopLeft.y > laserBottomRight.y ||
          playerRelativeBottomRight.y < laserTopLeft.y))
    {
        goto LASER_COLLISION;
    }

    if (!canGraze)
    {
        return 0;
    }

    laserTopLeft.x -= 48.0f;
    laserTopLeft.y -= 48.0f;
    laserBottomRight.x += 48.0f;
    laserBottomRight.y += 48.0f;
    if (playerRelativeTopLeft.x > laserBottomRight.x ||
        playerRelativeBottomRight.x < laserTopLeft.x ||
        playerRelativeTopLeft.y > laserBottomRight.y ||
        playerRelativeBottomRight.y < laserTopLeft.y)
    {
        return 0;
    }

    if (this->playerState == PLAYER_STATE_DEAD || this->playerState == PLAYER_STATE_SPAWNING)
    {
        return 0;
    }

    ScoreGraze(&this->pos);
    return 2;

LASER_COLLISION:
    g_ReplayManager->replayEventFlags = g_ReplayManager->replayEventFlags | 2;
    if (this->playerState == PLAYER_STATE_BORDER)
    {
        // this is already a member function of Player though
        g_Player.BreakBorder();
        return 1;
    }
    if (this->playerState != PLAYER_STATE_ALIVE)
    {
        return 0;
    }

    g_GameManager.RerollRng();
    Die();
    return 1;
}

void Player::ScoreGraze(ZunVec3 *param_1)
{
    ZunVec3 grazePos;

    if (!g_Player.bombInfo.isInUse)
    {
        if (g_GameManager.globals->grazeInStage < 9999)
        {
            g_GameManager.globals->grazeInStage++;
        }
        if (g_GameManager.globals->grazeInTotal < 999999)
        {
            g_GameManager.globals->grazeInTotal++;
        }
    }
    grazePos = (this->pos + *param_1) / 2.0f;
    if (this->hasBorder == BORDER_ACTIVE)
    {
        if (this->isFocus)
        {
            g_EffectManager.SpawnEffect(8, &grazePos, 1, 0xffffffff);
        }
        else
        {
            g_EffectManager.SpawnEffect(8, &grazePos, 3, 0xffff8080);
        }
    }
    else
    {
        g_EffectManager.SpawnEffect(8, &grazePos, 1, 0xffffffff);
    }
    g_GameManager.IncreaseSubrank(6);
    g_Gui.grazeDisplayUpdateFrames = 2;
    g_SoundPlayer.PlaySoundByIdx(SOUND_GRAZE, 0);
    g_EnemyManager.spellcardInfo.grazeBonusScore =
        g_EnemyManager.spellcardInfo.grazeBonusScore + 2500 +
        (g_GameManager.cherry - g_GameManager.globals->cherryStart) / 1500 * 20;
    g_GameManager.AddScore(2000);
    if (this->hasBorder == BORDER_ACTIVE)
    {
        if (this->isFocus)
        {
            g_GameManager.IncreaseCherryMax(30);
            g_GameManager.IncreaseCherry(30);
        }
        else
        {
            g_GameManager.IncreaseCherryMax(80);
            g_GameManager.IncreaseCherry(80);
        }
    }
}

void Player::Die()
{
    g_GameManager.RegenerateGameIntegrityCsum();
    g_EffectManager.SpawnSpecialEffect(12, &this->pos, 3, 1, 0xff4040ff);
    g_EffectManager.SpawnEffect(6, &this->pos, 16, 0xffffffff);
    this->playerState = PLAYER_STATE_DEAD;
    this->invulnerabilityTimer = 0;
    g_SoundPlayer.PlaySoundByIdx(SOUND_PICHUN, 0);

    // touch controls are a bit more tricky to deathbomb since your finger doesn't rest on a key
    // that you can physically actuate the moment you need to deathbomb, so because im just such
    // a nice person there's a 5 frame leniency for touch users (ONLY FOR IF YOU BOMBED WITH
    // TOUCH!!!!!)
    this->respawnTimer = g_Player.shooterData->initialRespawnTimer +
                         (Touch::WasUsedThisRun() ? Touch::DEATHBOMB_TOLERANCE : 0);
}

i32 Player::HandlePlayerInputs()
{
    f32 angleStep;
    f32 targetOffsetX;
    f32 targetOffsetY;
    f32 t;
    f32 optionOffsetX;
    f32 optionOffsetY;
    f32 horizontalSpeed;
    f32 verticalSpeed;

    f32 touchDx;
    f32 touchDy;
    bool touchFocus;

    horizontalSpeed = 0.0f;
    verticalSpeed = 0.0f;
    this->playerDirection = MOVEMENT_NONE;

    if (IS_PRESSED_GAME(TH_BUTTON_UP))
    {
        this->playerDirection = MOVEMENT_UP;
        if (IS_PRESSED_GAME(TH_BUTTON_LEFT))
        {
            this->playerDirection = MOVEMENT_UP_LEFT;
        }
        if (IS_PRESSED_GAME(TH_BUTTON_RIGHT))
        {
            this->playerDirection = MOVEMENT_UP_RIGHT;
        }
    }
    else if (IS_PRESSED_GAME(TH_BUTTON_DOWN))
    {
        this->playerDirection = MOVEMENT_DOWN;
        if (IS_PRESSED_GAME(TH_BUTTON_LEFT))
        {
            this->playerDirection = MOVEMENT_DOWN_LEFT;
        }
        if (IS_PRESSED_GAME(TH_BUTTON_RIGHT))
        {
            this->playerDirection = MOVEMENT_DOWN_RIGHT;
        }
    }
    else
    {
        if (IS_PRESSED_GAME(TH_BUTTON_LEFT))
        {
            this->playerDirection = MOVEMENT_LEFT;
        }
        if (IS_PRESSED_GAME(TH_BUTTON_RIGHT))
        {
            this->playerDirection = MOVEMENT_RIGHT;
        }
    }

    if (IS_PRESSED_GAME(TH_BUTTON_FOCUS))
    {
        this->isFocus = 1;
        switch (this->playerDirection)
        {
        case MOVEMENT_RIGHT:
            horizontalSpeed = this->shooterData->speedFocus;
            break;
        case MOVEMENT_LEFT:
            horizontalSpeed = -this->shooterData->speedFocus;
            break;
        case MOVEMENT_UP:
            verticalSpeed = -this->shooterData->speedFocus;
            break;
        case MOVEMENT_DOWN:
            verticalSpeed = this->shooterData->speedFocus;
            break;
        case MOVEMENT_UP_LEFT:
            horizontalSpeed = -this->shooterData->speedDiagonalFocus;
            verticalSpeed = horizontalSpeed;
            break;
        case MOVEMENT_DOWN_LEFT:
            verticalSpeed = this->shooterData->speedDiagonalFocus;
            horizontalSpeed = -verticalSpeed;
            break;
        case MOVEMENT_UP_RIGHT:
            horizontalSpeed = this->shooterData->speedDiagonalFocus;
            verticalSpeed = -horizontalSpeed;
            break;
        case MOVEMENT_DOWN_RIGHT:
            horizontalSpeed = this->shooterData->speedDiagonalFocus;
            verticalSpeed = horizontalSpeed;
            break;
        case MOVEMENT_NONE:
            break;
        }
    }
    else
    {
        this->isFocus = 0;
        switch (this->playerDirection)
        {
        case MOVEMENT_RIGHT:
            horizontalSpeed = this->shooterData->speed;
            break;
        case MOVEMENT_LEFT:
            horizontalSpeed = -this->shooterData->speed;
            break;
        case MOVEMENT_UP:
            verticalSpeed = -this->shooterData->speed;
            break;
        case MOVEMENT_DOWN:
            verticalSpeed = this->shooterData->speed;
            break;
        case MOVEMENT_UP_LEFT:
            horizontalSpeed = -this->shooterData->speedDiagonal;
            verticalSpeed = horizontalSpeed;
            break;
        case MOVEMENT_DOWN_LEFT:
            verticalSpeed = this->shooterData->speedDiagonal;
            horizontalSpeed = -verticalSpeed;
            break;
        case MOVEMENT_UP_RIGHT:
            horizontalSpeed = this->shooterData->speedDiagonal;
            verticalSpeed = -horizontalSpeed;
            break;
        case MOVEMENT_DOWN_RIGHT:
            horizontalSpeed = this->shooterData->speedDiagonal;
            verticalSpeed = horizontalSpeed;
            break;
        case MOVEMENT_NONE:
            break;
        }
    }

    if (Touch::GetPlayerDelta(&touchDx, &touchDy))
    {
        f32 focusRatio = 1.0f;
        if (this->isFocus && this->shooterData && this->shooterData->speed != 0.0f)
        {
            focusRatio = this->shooterData->speedFocus / this->shooterData->speed;
        }

        f32 reqGameDx = touchDx * focusRatio;
        f32 reqGameDy = touchDy * focusRatio;

        f32 minX = g_GameManager.playerMovementAreaTopLeftPos.x;
        f32 maxX =
            g_GameManager.playerMovementAreaTopLeftPos.x + g_GameManager.playerMovementAreaSize.x;
        f32 minY = g_GameManager.playerMovementAreaTopLeftPos.y;
        f32 maxY =
            g_GameManager.playerMovementAreaTopLeftPos.y + g_GameManager.playerMovementAreaSize.y;

        f32 targetX = this->pos.x + reqGameDx;
        f32 targetY = this->pos.y + reqGameDy;

        if (targetX < minX)
        {
            reqGameDx = minX - this->pos.x;
        }
        else if (targetX > maxX)
        {
            reqGameDx = maxX - this->pos.x;
        }

        if (targetY < minY)
        {
            reqGameDy = minY - this->pos.y;
        }
        else if (targetY > maxY)
        {
            reqGameDy = maxY - this->pos.y;
        }

        if (focusRatio != 0.0f)
        {
            Touch::SetPlayerDelta(reqGameDx / focusRatio, reqGameDy / focusRatio);
        }

        f32 hx = this->horizontalMovementSpeedMultiplierDuringBomb *
                 g_Supervisor.effectiveFramerateMultiplier;
        f32 vy = this->verticalMovementSpeedMultiplierDuringBomb *
                 g_Supervisor.effectiveFramerateMultiplier;

        f32 requestedHorizontalSpeed = hx != 0.0f ? reqGameDx / hx : 0.0f;
        f32 requestedVerticalSpeed = vy != 0.0f ? reqGameDy / vy : 0.0f;

        f32 currentSpeedSq = requestedHorizontalSpeed * requestedHorizontalSpeed +
                             requestedVerticalSpeed * requestedVerticalSpeed;

        f32 maxSpeed = this->isFocus ? this->shooterData->speedFocus : this->shooterData->speed;

        if (currentSpeedSq > maxSpeed * maxSpeed && currentSpeedSq > 0.0f)
        {
            f32 currentSpeed = sqrtf(currentSpeedSq);
            horizontalSpeed = (requestedHorizontalSpeed / currentSpeed) * maxSpeed;
            verticalSpeed = (requestedVerticalSpeed / currentSpeed) * maxSpeed;
        }
        else
        {
            horizontalSpeed = requestedHorizontalSpeed;
            verticalSpeed = requestedVerticalSpeed;
        }

        f32 consumedGameDx = 0.0f;
        f32 consumedGameDy = 0.0f;

        if (hx != 0.0f)
        {
            consumedGameDx = horizontalSpeed * hx;
        }
        if (vy != 0.0f)
        {
            consumedGameDy = verticalSpeed * vy;
        }

        if (focusRatio != 0.0f)
        {
            if (currentSpeedSq > maxSpeed * maxSpeed && currentSpeedSq > 0.0f)
            {
                f32 consumeX = (hx != 0.0f) ? consumedGameDx / focusRatio : touchDx;
                f32 consumeY = (vy != 0.0f) ? consumedGameDy / focusRatio : touchDy;
                Touch::ConsumePlayerDelta(consumeX, consumeY);
            }
            else
            {
                Touch::SetPlayerDelta(0.0f, 0.0f);
            }
        }

        this->playerDirection = MOVEMENT_NONE;

        // this is actually pretty useless since playerdirection handling is above which we
        // completely ignored in the touch handling path. the actual part that handles what
        // direction the playersprite faces is below
        const f32 dirDeadzone = 0.01f;
        bool left = touchDx < -dirDeadzone;
        bool right = touchDx > dirDeadzone;
        bool up = touchDy < -dirDeadzone;
        bool down = touchDy > dirDeadzone;

        if (up)
        {
            this->playerDirection = MOVEMENT_UP;
            if (left)
            {
                this->playerDirection = MOVEMENT_UP_LEFT;
            }
            else if (right)
            {
                this->playerDirection = MOVEMENT_UP_RIGHT;
            }
        }
        else if (down)
        {
            this->playerDirection = MOVEMENT_DOWN;
            if (left)
            {
                this->playerDirection = MOVEMENT_DOWN_LEFT;
            }
            else if (right)
            {
                this->playerDirection = MOVEMENT_DOWN_RIGHT;
            }
        }
        else if (left)
        {
            this->playerDirection = MOVEMENT_LEFT;
        }
        else if (right)
        {
            this->playerDirection = MOVEMENT_RIGHT;
        }
    }

    if (horizontalSpeed < 0.0f && this->previousHorizontalSpeed >= 0.0f)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(&this->playerSprite, ANM_SCRIPT_PLAYER_MOVING_LEFT);
    }
    else if (horizontalSpeed == 0.0f && this->previousHorizontalSpeed < 0.0f)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(&this->playerSprite,
                                                ANM_SCRIPT_PLAYER_STOPPING_LEFT);
    }

    if (horizontalSpeed > 0.0f && this->previousHorizontalSpeed <= 0.0f)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(&this->playerSprite,
                                                ANM_SCRIPT_PLAYER_MOVING_RIGHT);
    }
    else if (horizontalSpeed == 0.0f && this->previousHorizontalSpeed > 0.0f)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(&this->playerSprite,
                                                ANM_SCRIPT_PLAYER_STOPPING_RIGHT);
    }

    this->previousHorizontalSpeed = horizontalSpeed;
    this->previousVerticalSpeed = verticalSpeed;
    this->velocity.x = horizontalSpeed * this->horizontalMovementSpeedMultiplierDuringBomb *
                       g_Supervisor.effectiveFramerateMultiplier;
    this->velocity.y = verticalSpeed * this->verticalMovementSpeedMultiplierDuringBomb *
                       g_Supervisor.effectiveFramerateMultiplier;
    *GetPosX() += this->velocity.x;
    *GetPosY() += this->velocity.y;

    if (this->pos.x < g_GameManager.playerMovementAreaTopLeftPos.x)
    {
        this->pos.x = g_GameManager.playerMovementAreaTopLeftPos.x;
    }
    else if (this->pos.x >
             g_GameManager.playerMovementAreaTopLeftPos.x + g_GameManager.playerMovementAreaSize.x)
    {
        this->pos.x =
            g_GameManager.playerMovementAreaTopLeftPos.x + g_GameManager.playerMovementAreaSize.x;
    }

    if (this->pos.y < g_GameManager.playerMovementAreaTopLeftPos.y)
    {
        this->pos.y = g_GameManager.playerMovementAreaTopLeftPos.y;
    }
    else if (this->pos.y >
             g_GameManager.playerMovementAreaTopLeftPos.y + g_GameManager.playerMovementAreaSize.y)
    {
        this->pos.y =
            g_GameManager.playerMovementAreaTopLeftPos.y + g_GameManager.playerMovementAreaSize.y;
    }

    this->hitboxTopLeft = this->pos - this->hitboxSize;
    this->hitboxBottomRight = this->pos + this->hitboxSize;
    this->grazeTopLeft = this->pos - this->grazeSize;
    this->grazeBottomRight = this->pos + this->grazeSize;
    this->grabItemTopLeft = this->pos - this->grabItemSize;
    this->grabItemBottomRight = this->pos + this->grabItemSize;
    this->optionsPosition[0] = this->pos;
    this->optionsPosition[1] = this->pos;
    optionOffsetX = optionOffsetY = 0.0f;

    if (g_GameManager.character != CHAR_SAKUYA || g_GameManager.shotType != 1)
    {
        switch (this->optionState)
        {
        case OPTION_HIDDEN:
            this->focusMovementTimer = 0;
            break;
        case OPTION_UNFOCUSED:
            optionOffsetX = 24.0f;
            this->focusMovementTimer = 0;
            if (this->isFocus)
            {
                this->optionState = OPTION_FOCUSING;
                this->focusEffect =
                    g_EffectManager.SpawnSpecialEffect(24, &this->pos, 2, 1, 0xffffffff);
            }
            else
            {
                break;
            }
        CASE_OPTION_FOCUSING:
        case OPTION_FOCUSING:
            this->focusMovementTimer++;
            t = this->focusMovementTimer.AsFloat() / 8.0f;
            optionOffsetY = -32.0f + (1.0f - t) * 32.0f;
            t *= t;
            optionOffsetX = -16.0f * t + 24.0f;
            if (this->focusMovementTimer >= 8)
            {
                this->optionState = OPTION_FOCUSED;
            }
            if (!this->isFocus)
            {
                this->optionState = OPTION_UNFOCUSING;
                this->focusMovementTimer = 8 - this->focusMovementTimer.GetCurrent();
                if (this->focusEffect)
                {
                    this->focusEffect->vm.SetInterrupt(1);
                }
                goto CASE_OPTION_UNFOCUSING;
            }
            break;
        case OPTION_FOCUSED:
            optionOffsetX = 8.0f;
            optionOffsetY = -32.0f;
            this->focusMovementTimer = 0;
            if (!this->isFocus)
            {
                this->optionState = OPTION_UNFOCUSING;
                if (this->focusEffect)
                {
                    this->focusEffect->vm.SetInterrupt(1);
                }
                goto CASE_OPTION_UNFOCUSING;
            }
            break;
        CASE_OPTION_UNFOCUSING:
        case OPTION_UNFOCUSING:
            this->focusMovementTimer++;
            t = this->focusMovementTimer.AsFloat() / 8.0f;
            optionOffsetY = -32.0f + 32.0f * t;
            t *= t;
            t = 1.0f - t;
            optionOffsetX = -16.0f * t + 24.0f;
            if (this->focusMovementTimer >= 8)
            {
                this->optionState = OPTION_UNFOCUSED;
            }
            if (this->isFocus)
            {
                this->optionState = OPTION_FOCUSING;
                this->focusMovementTimer = 8 - this->focusMovementTimer.GetCurrent();
                this->focusEffect =
                    g_EffectManager.SpawnSpecialEffect(24, &this->pos, 2, 1, 0xffffffff);
                goto CASE_OPTION_FOCUSING;
            }
        }
        this->optionsPosition[0].x -= optionOffsetX;
        this->optionsPosition[1].x += optionOffsetX;
        this->optionsPosition[0].y += optionOffsetY;
        this->optionsPosition[1].y += optionOffsetY;
    }
    else
    {
        switch (this->optionState)
        {
        case OPTION_HIDDEN:
            this->focusMovementTimer = 0;
            break;
        case OPTION_UNFOCUSED:
            optionOffsetX = cosf(this->optionAngle + ZUN_PI / 2.0f) * 24.0f;
            optionOffsetY = sinf(this->optionAngle + ZUN_PI / 2.0f) * 24.0f;
            this->focusMovementTimer = 0;
            if (this->isFocus)
            {
                this->optionState = OPTION_FOCUSING;
                this->focusEffect =
                    g_EffectManager.SpawnSpecialEffect(24, &this->pos, 2, 1, 0xffffffff);
                goto CASE_OPTION_FOCUSING_2;
            }
            this->optionsPosition[0].x -= optionOffsetX;
            this->optionsPosition[1].x += optionOffsetX;
            this->optionsPosition[0].y -= optionOffsetY;
            this->optionsPosition[1].y += optionOffsetY;
            break;
        CASE_OPTION_FOCUSING_2:
        case OPTION_FOCUSING:
            if (!this->isFocus)
            {
                this->optionState = OPTION_UNFOCUSING;
                this->focusMovementTimer = 8 - this->focusMovementTimer.GetCurrent();
                if (this->focusEffect)
                {
                    this->focusEffect->vm.SetInterrupt(1);
                }
                goto CASE_OPTION_UNFOCUSING_2;
            }
            this->focusMovementTimer++;
            t = this->focusMovementTimer.AsFloat() / 8.0f;
            optionOffsetX = cosf(this->optionAngle + ZUN_PI / 2.0f) * 24.0f;
            optionOffsetY = sinf(this->optionAngle + ZUN_PI / 2.0f) * 24.0f;
            targetOffsetX = cosf(this->optionAngle + ZUN_PI / 14.0f) * 24.0f;
            targetOffsetY = sinf(this->optionAngle + ZUN_PI / 14.0f) * 24.0f;
            targetOffsetX = (targetOffsetX - optionOffsetX) * t + optionOffsetX;
            targetOffsetY = (targetOffsetY - optionOffsetY) * t + optionOffsetY;
            this->optionsPosition[1].x += targetOffsetX;
            this->optionsPosition[1].y += targetOffsetY;
            targetOffsetX = cosf(this->optionAngle - ZUN_PI / 14.0f) * 24.0f;
            targetOffsetY = sinf(this->optionAngle - ZUN_PI / 14.0f) * 24.0f;
            targetOffsetX = (targetOffsetX + optionOffsetX) * t - optionOffsetX;
            targetOffsetY = (targetOffsetY + optionOffsetY) * t - optionOffsetY;
            if (this->focusMovementTimer >= 8)
            {
                this->optionState = OPTION_FOCUSED;
            }
            this->optionsPosition[0].x += targetOffsetX;
            this->optionsPosition[0].y += targetOffsetY;
            break;
        case OPTION_FOCUSED:
            this->focusMovementTimer = 0;
            if (!this->isFocus)
            {
                this->optionState = OPTION_UNFOCUSING;
                if (this->focusEffect)
                {
                    this->focusEffect->vm.SetInterrupt(1);
                }
                goto CASE_OPTION_UNFOCUSING_2;
            }
            targetOffsetX = cosf(this->optionAngle + ZUN_PI / 14.0f) * 24.0f;
            targetOffsetY = sinf(this->optionAngle + ZUN_PI / 14.0f) * 24.0f;
            this->optionsPosition[1].x += targetOffsetX;
            this->optionsPosition[1].y += targetOffsetY;
            targetOffsetX = cosf(this->optionAngle - ZUN_PI / 14.0f) * 24.0f;
            targetOffsetY = sinf(this->optionAngle - ZUN_PI / 14.0f) * 24.0f;
            this->optionsPosition[0].x += targetOffsetX;
            this->optionsPosition[0].y += targetOffsetY;
            break;
        CASE_OPTION_UNFOCUSING_2:
        case OPTION_UNFOCUSING:
            if (this->isFocus)
            {
                this->optionState = OPTION_FOCUSING;
                this->focusMovementTimer = 8 - this->focusMovementTimer.GetCurrent();
                this->focusEffect =
                    g_EffectManager.SpawnSpecialEffect(24, &this->pos, 2, 1, 0xffffffff);
                goto CASE_OPTION_FOCUSING_2;
            }
            this->focusMovementTimer++;
            t = 1.0f - this->focusMovementTimer.AsFloat() / 8.0f;
            optionOffsetX = cosf(this->optionAngle + ZUN_PI / 2.0f) * 24.0f;
            optionOffsetY = sinf(this->optionAngle + ZUN_PI / 2.0f) * 24.0f;
            targetOffsetX = cosf(this->optionAngle + ZUN_PI / 14.0f) * 24.0f;
            targetOffsetY = sinf(this->optionAngle + ZUN_PI / 14.0f) * 24.0f;
            targetOffsetX = (targetOffsetX - optionOffsetX) * t + optionOffsetX;
            targetOffsetY = (targetOffsetY - optionOffsetY) * t + optionOffsetY;
            this->optionsPosition[1].x += targetOffsetX;
            this->optionsPosition[1].y += targetOffsetY;
            targetOffsetX = cosf(this->optionAngle - ZUN_PI / 14.0f) * 24.0f;
            targetOffsetY = sinf(this->optionAngle - ZUN_PI / 14.0f) * 24.0f;
            targetOffsetX = (targetOffsetX + optionOffsetX) * t - optionOffsetX;
            targetOffsetY = (targetOffsetY + optionOffsetY) * t - optionOffsetY;
            if (this->focusMovementTimer >= 8)
            {
                this->optionState = OPTION_UNFOCUSED;
            }
            this->optionsPosition[0].x += targetOffsetX;
            this->optionsPosition[0].y += targetOffsetY;
            break;
        }
    }
    if (IS_PRESSED_GAME(TH_BUTTON_SHOOT) && !g_Gui.HasCurrentMsgIdx())
    {
        if (!g_GameManager.CheckGameIntegrity())
        {
            StartFireBulletTimer();
        }
        if (!IS_PRESSED_GAME(TH_BUTTON_FOCUS))
        {
            if (this->velocity.x != 0.0f)
            {
                angleStep = -(this->velocity.x / 4.0f) * ZUN_PI / 5.0f / 10.0f;
                this->optionAngle -= angleStep;
                if (this->optionAngle < -(7.0f * ZUN_PI / 10.0f))
                {
                    this->optionAngle = -(7.0f * ZUN_PI / 10.0f);
                }
                else if (this->optionAngle > -ZUN_3PI / 10.0f)
                {
                    this->optionAngle = -ZUN_3PI / 10.0f;
                }
            }
            else
            {
                if (fabsf(this->optionAngle - (-ZUN_PI / 2.0f)) > ZUN_PI / 100.0f)
                {
                    angleStep = this->optionAngle < -ZUN_PI / 2.0f
                                    ? ZUN_PI / 50.0f * g_Supervisor.effectiveFramerateMultiplier
                                    : -ZUN_PI / 50.0f * g_Supervisor.effectiveFramerateMultiplier;
                    this->optionAngle += angleStep;
                }
                else
                {
                    this->optionAngle = -ZUN_PI / 2.0f;
                }
            }
        }
    }
    return 0;
}

void Player::UpdateBombProjectiles()
{
    BombClearBox *bomb;
    i32 i;

    for (i = 0; i < ARRAY_SIZE_SIGNED(this->bombDamageBoxes); i++)
    {
        this->bombDamageBoxes[i].size.x = 0.0f;
    }
    bomb = this->bombClearBoxes;
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->bombClearBoxes); i++, bomb++)
    {
        if (bomb->lifetime <= 0)
        {
            bomb->size.y = 0.0f;
            bomb->pos.z = 0.0f;
        }
        else
        {
            bomb->lifetime--;
            bomb->size.y += bomb->size.z;
        }
    }
    this->dirtyBombBoxes = true;
}

void Player::UpdateBorderAndBombState()
{
    if (this->hasBorder != BORDER_NONE && !this->bombInfo.isInUse &&
        IS_PRESSED_GAME(TH_BUTTON_BOMB))
    {
        BreakBorder();
        this->isBombing = 0;
        g_ItemManager.RemoveAllItems();
    }
    else
    {
        if (this->hasBorder == BORDER_READY)
        {
            ActivateBorder();
        }
        if (this->borderInvulnerabilityTime != 0)
        {
            this->borderInvulnerabilityTime--;
        }
        if (this->bombInfo.isInUse)
        {
            if (this->bombInfo.bombTimer.HasTicked())
            {
                PlayerBombInfo::SubtractCherryDrain(this->bombInfo.cherryDrain);
                g_Gui.pointDisplayUpdateFrames = 2;
            }
            if (!this->bombInfo.isFocus)
            {
                this->bombInfo.bombCalc(this);
            }
            else
            {
                this->bombInfo.bombFocusCalc(this);
            }
        }
        else
        {
            if (!g_GameManager.CheckGameIntegrity() && !g_Gui.HasCurrentMsgIdx() &&
                this->respawnTimer != 0 && (i32)g_GameManager.globals->bombsRemaining > 0 &&
                this->borderInvulnerabilityTime == 0 && IS_PRESSED_GAME(TH_BUTTON_BOMB))
            {
                if (this->playerState == PLAYER_STATE_DEAD)
                {
                    i32 minRequiredTimer = (Touch::WasUsedThisRun() && !Touch::UsedTouchToBomb())
                                               ? Touch::DEATHBOMB_TOLERANCE
                                               : 0;
                    if (this->respawnTimer <= minRequiredTimer)
                    {
                        return;
                    }
                }
                g_ReplayManager->replayEventFlags |= 1;
                g_GameManager.AddBombsUsed(1);
                g_GameManager.AddBombsRemaining(-1);
                g_Gui.bombDisplayUpdateFrames = 2;
                this->bombInfo.isFocus = (i32)this->isFocus;
                this->bombInfo.isInUse = 1;
                this->isBombing = 1;
                this->bombInfo.bombTimer = 0;
                this->bombInfo.bombDuration = 999;
                if (!this->bombInfo.isFocus)
                {
                    this->bombInfo.bombCalc(this);
                }
                else
                {
                    this->bombInfo.bombFocusCalc(this);
                }
                g_EnemyManager.spellcardInfo.captureScore = 0;
                g_EnemyManager.spellcardInfo.isCapturing = 0;
                g_GameManager.DecreaseSubrank(200);
                g_EnemyManager.spellcardInfo.usedBomb = g_EnemyManager.spellcardInfo.isActive;
                this->respawnTimer += 6;
                if (this->respawnTimer > g_Player.shooterData->initialRespawnTimer)
                {
                    this->respawnTimer = g_Player.shooterData->initialRespawnTimer;
                }
            }
            else
            {
                this->isBombing = 0;
            }
        }
    }
}

i32 Player::UpdateDeath()
{
    f32 invulnScale;
    i32 cherryPenalty;

    if (this->respawnTimer != 0)
    {
        if (this->hasBorder == BORDER_ACTIVE)
        {
            BreakBorder();
            return 0;
        }
        this->respawnTimer--;
        if (this->respawnTimer == 0)
        {
            g_ReplayManager->replayEventFlags |= 4;
            g_GameManager.powerItemCountForScore = 0;
            g_EnemyManager.spellcardInfo.captureScore = 0;
            g_EnemyManager.spellcardInfo.isCapturing = 0;
            g_GameManager.CheckGameIntegrityOnDeath(1);
            if ((i32)g_GameManager.globals->livesRemaining > 0)
            {
                if ((i32)g_GameManager.globals->currentPower <= 16)
                {
                    g_GameManager.globals->currentPower = 0.0f;
                    g_GameManager.RegenerateGameIntegrityCsum();
                }
                else
                {
                    g_GameManager.AddCurrentPower(-16);
                }
                g_ItemManager.SpawnItem(&this->pos, ITEM_POWER_BIG, 2);
                g_ItemManager.SpawnItem(&this->pos, ITEM_POWER_SMALL, 2);
                g_ItemManager.SpawnItem(&this->pos, ITEM_POWER_SMALL, 2);
                g_ItemManager.SpawnItem(&this->pos, ITEM_POWER_SMALL, 2);
                g_ItemManager.SpawnItem(&this->pos, ITEM_POWER_SMALL, 2);
                g_ItemManager.SpawnItem(&this->pos, ITEM_POWER_SMALL, 2);
                g_Gui.powerDisplayUpdateFrames = 2;
                cherryPenalty = (f32)(g_GameManager.cherry - g_GameManager.globals->cherryStart) *
                                g_Player.shooterData->cherryPenaltyMultiplier;
                if (g_GameManager.character != CHAR_SAKUYA)
                {
                    if (cherryPenalty > 100000)
                    {
                        cherryPenalty = 100000;
                    }
                }
                else if (cherryPenalty > 60000)
                {
                    cherryPenalty = 60000;
                }
                cherryPenalty -= cherryPenalty % 10;
                g_GameManager.cherry -= cherryPenalty;
                g_Gui.pointDisplayUpdateFrames = 2;
                g_ItemManager.ActivateAllItems();
            }
            else
            {
                g_GameManager.globals->currentPower = 0.0f;
                g_GameManager.RegenerateGameIntegrityCsum();
                g_ItemManager.SpawnItem(&this->pos, ITEM_FULL_POWER, 2);
                g_ItemManager.SpawnItem(&this->pos, ITEM_FULL_POWER, 2);
                g_ItemManager.SpawnItem(&this->pos, ITEM_FULL_POWER, 2);
                g_ItemManager.SpawnItem(&this->pos, ITEM_FULL_POWER, 2);
                g_ItemManager.SpawnItem(&this->pos, ITEM_FULL_POWER, 2);
                g_Gui.powerDisplayUpdateFrames = 2;
            }
            g_GameManager.DecreaseSubrank(1600);
        }
    }
    else
    {
        invulnScale = this->invulnerabilityTimer.AsFloat() / 30.0f;
        this->playerSprite.scale.y = 3.0f * invulnScale + 1.0f;
        this->playerSprite.scale.x = 1.0f - 1.0f * invulnScale;
        this->playerSprite.color.color =
            (u32)(255.0f - this->invulnerabilityTimer.AsFloat() * 255.0f / 30.0f) << 24 | 0xffffff;
        this->playerSprite.blendMode = 1;
        this->previousHorizontalSpeed = 0.0f;
        this->previousVerticalSpeed = 0.0f;
        if (this->invulnerabilityTimer.GetCurrent() >= 30)
        {
            this->playerState = PLAYER_STATE_SPAWNING;
            this->pos.x = g_GameManager.arcadeRegionSize.x / 2.0f;
            this->pos.y = g_GameManager.arcadeRegionSize.y - 64.0f;
            this->pos.z = 0.2f;
            this->prevPos = this->pos;
            this->invulnerabilityTimer = 0;
            this->playerSprite.scale.x = 3.0f;
            this->playerSprite.scale.y = 3.0f;
            g_AnmManager->SetAnmIdxAndExecuteScript(&this->playerSprite, ANM_SCRIPT_PLAYER_IDLE);
            if ((i32)g_GameManager.globals->livesRemaining <= 0)
            {
                g_GameManager.isInRetryMenu = 1;
            }
            else
            {
                g_GameManager.AddLivesRemaining(-1);
                g_Gui.lifeDisplayUpdateFrames = 2;
                g_GameManager.SetBombsRemainingAndComputeCsum(g_Player.shooterData->initialBombs);
                g_Gui.bombDisplayUpdateFrames = 2;
                return 1;
            }
        }
    }
    return 0;
}

void Player::Respawn()
{
    this->bulletGracePeriod = 60;
    f32 invulnScale = 1.0f - this->invulnerabilityTimer.AsFloat() / 30.0f;
    this->playerSprite.scale.y = 2.0f * invulnScale + 1.0f;
    this->playerSprite.scale.x = 1.0f - 1.0f * invulnScale;
    this->playerSprite.blendMode = 1;
    this->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
    this->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
    this->playerSprite.color.color =
        this->invulnerabilityTimer.GetCurrent() * 255 / 30 << 24 | 0xffffff;
    this->respawnTimer = 0;
    if (this->invulnerabilityTimer.GetCurrent() >= 30)
    {
        this->playerState = PLAYER_STATE_INVULNERABLE;
        this->playerSprite.scale.x = 1.0f;
        this->playerSprite.scale.y = 1.0f;
        this->playerSprite.color.color = 0xffffffff;
        this->playerSprite.blendMode = 0;
        this->invulnerabilityTimer = 240;
        this->respawnTimer = g_Player.shooterData->initialRespawnTimer;
    }
}

void Player::UpdateState()
{
    ZunColor color;

    if (this->bulletGracePeriod != 0)
    {
        this->bulletGracePeriod--;
        g_BulletManager.RemoveAllBullets(0);
    }
    if (this->playerState == PLAYER_STATE_INVULNERABLE)
    {
        if (this->effect)
        {
            this->effect->pos = this->pos;
        }
        this->invulnerabilityTimer--;
        if (this->invulnerabilityTimer.GetCurrent() <= 0)
        {
            if (this->effect)
            {
                this->effect->inUseFlag = 0;
                this->effect = NULL;
            }
            this->playerState = PLAYER_STATE_ALIVE;
            this->invulnerabilityTimer = 0;
            this->playerSprite.color.color = 0xffffffff;
        }
        else
        {
            if (this->invulnerabilityTimer.GetCurrent() % 8 < 2)
            {
                this->playerSprite.color.color = 0xff404040;
            }
            else
            {
                this->playerSprite.color.color = 0xffffffff;
            }
        }
    }
    else if (this->playerState == PLAYER_STATE_BORDER)
    {
        if (this->borderEffect)
        {
            this->borderEffect->pos = this->pos;
        }
        g_GameManager.cherryPlus =
            this->invulnerabilityTimer.GetCurrent() * 50000 / this->borderTimer.GetCurrent();
        if (g_GameManager.cherryPlus < 0)
        {
            g_GameManager.cherryPlus = 0;
        }
        g_GameManager.cherryPlus += g_GameManager.globals->cherryStart;
        this->invulnerabilityTimer--;
        if (this->invulnerabilityTimer.GetCurrent() <= 0)
        {
            this->playerSprite.color.color = 0xffffffff;
            BreakBorderNaturally();
        }
        else
        {
            if (this->invulnerabilityTimer.GetCurrent() % 4 < 2)
            {
                this->playerSprite.color.color = 0xffff0000;
            }
            else
            {
                this->playerSprite.color.color = 0xffffffff;
            }
            color.bytes.a = 128;
            if (g_Player.invulnerabilityTimer >= 510)
            {
                color.bytes.r = color.bytes.g = color.bytes.b =
                    128 - (540 - g_Player.invulnerabilityTimer.GetCurrent()) * 80 / 30;
            }
            else if (g_Player.invulnerabilityTimer < 30)
            {
                color.bytes.r = color.bytes.g = color.bytes.b =
                    128 - g_Player.invulnerabilityTimer.GetCurrent() * 80 / 30;
            }
            else
            {
                color.bytes.r = color.bytes.g = color.bytes.b = 48;
            }
            g_Stage.SmoothBlendColor(color);
        }
    }
    else
    {
        this->invulnerabilityTimer++;
    }
}

void Player::BreakBorderNaturally()
{
    i32 cherryDiff;

    g_GameManager.IncreaseCherryMax(10000);
    g_GameManager.IncreaseCherry(10000);
    cherryDiff = g_GameManager.cherry - g_GameManager.globals->cherryStart;
    cherryDiff *= 10;
    g_GameManager.AddScore(cherryDiff);
    g_Gui.ShowStatusPopup(cherryDiff, 4);
    g_GameManager.cherryPlus = g_GameManager.globals->cherryStart;
    g_SoundPlayer.PlaySoundByIdx(SOUND_BORDER_BREAK, 0);
    if (this->playerState == PLAYER_STATE_SPAWNING)
    {
        this->playerSprite.scale.x = 1.0f;
        this->playerSprite.scale.y = 1.0f;
        this->playerSprite.color.color = 0xffffffff;
        this->playerSprite.blendMode = 0;
        this->invulnerabilityTimer = 240;
        this->respawnTimer = g_Player.shooterData->initialRespawnTimer;
    }
    this->playerState = PLAYER_STATE_INVULNERABLE;
    this->invulnerabilityTimer = 40;
    this->borderInvulnerabilityTime = 40;
    this->hasBorder = BORDER_NONE;
    if (this->borderEffect)
    {
        this->borderEffect->inUseFlag = 0;
        this->borderEffect = NULL;
    }
}

BombClearBox *Player::SpawnBombProjectile(ZunVec3 *centerPosition, f32 posZ, f32 size, i32 itemType)
{
    BombClearBox *bomb;
    i32 i;

    bomb = this->bombClearBoxes;
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->bombClearBoxes) - 1; i++, bomb++)
    {
        if (bomb->pos.z == 0.0f && bomb->size.y == 0.0f)
        {
            break;
        }
    }
    bomb->pos.x = centerPosition->x;
    bomb->pos.y = centerPosition->y;
    bomb->pos.z = posZ;
    bomb->size.x = size;
    bomb->lifetime = 0;
    bomb->itemType = itemType;
    this->dirtyBombBoxes = true;
    return bomb;
}

BombClearBox *Player::SpawnBombEffect(ZunVec3 *pos, f32 sizeY, f32 sizeZ, i32 lifetime,
                                      i32 itemType)
{
    BombClearBox *bomb;
    i32 i;

    bomb = this->bombClearBoxes;
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->bombClearBoxes) - 1; i++, bomb++)
    {
        if (bomb->pos.z == 0.0f && bomb->size.y == 0.0f)
        {
            break;
        }
    }
    bomb->pos.x = pos->x;
    bomb->pos.y = pos->y;
    bomb->size.y = sizeY;
    bomb->size.z = sizeZ;
    bomb->lifetime = lifetime;
    bomb->itemType = itemType;
    this->dirtyBombBoxes = true;
    return bomb;
}

void Player::ActivateBorder()
{
    Effect *spawnedEffect;

    if (this->bombInfo.isInUse || g_Gui.HasCurrentMsgIdx())
    {
        this->hasBorder = BORDER_READY;
        return;
    }

    switch (this->playerState)
    {
    case PLAYER_STATE_SPAWNING:
    case PLAYER_STATE_INVULNERABLE:
        this->hasBorder = BORDER_READY;
        break;
    case PLAYER_STATE_DEAD:
        if (this->respawnTimer != 0)
        {
            BreakBorder();
            return;
        }

        this->hasBorder = BORDER_READY;
        break;
    default:
        this->invulnerabilityTimer = 540;
        this->borderTimer = this->invulnerabilityTimer;
        this->hasBorder = BORDER_ACTIVE;
        this->playerState = PLAYER_STATE_BORDER;
        if (this->borderEffect)
        {
            this->borderEffect->inUseFlag = 0;
        }
        if (this->effect)
        {
            this->effect->inUseFlag = 0;
            this->effect = NULL;
        }
        spawnedEffect = g_EffectManager.SpawnSpecialEffect(28, &this->pos, 4, 1, 0xffffffff);
        spawnedEffect->vm.interpStartTimes[4] = 0;
        spawnedEffect->vm.interpEndTimes[4] = this->invulnerabilityTimer.GetCurrent();
        spawnedEffect->vm.easeModes[4] = 0;
        spawnedEffect->vm.scaleInterpInitial.y = 1.0f;
        spawnedEffect->vm.scaleInterpInitial.x = 1.0f;
        spawnedEffect->vm.scaleInterpFinal.x = 0.25f;
        spawnedEffect->vm.scaleInterpFinal.y = 0.25f;
        spawnedEffect->vm.intVars1[0] = this->invulnerabilityTimer.GetCurrent();
        spawnedEffect->vm.angleVel.z *= -1.0f;
        this->borderEffect = spawnedEffect;
        g_Gui.ShowStatusPopup(0, 2);
        g_SoundPlayer.PlaySoundByIdx(SOUND_BORDER_ACTIVATE, 0);
        g_SoundPlayer.PlaySoundByIdx(SOUND_BORDER_ACTIVATE2, 0);
        g_ReplayManager->replayEventFlags |= 8;
        break;
    }
}

void Player::BreakBorder()
{
    f32 angle;
    i32 i;
    Effect *effect;

    if (this->borderEffect)
    {
        this->borderEffect->inUseFlag = 0;
        this->borderEffect = NULL;
    }
    effect = g_EffectManager.SpawnSpecialEffect(28, &this->pos, 4, 1, 0xffffffff);
    effect->vm.interpStartTimes[4] = 0;
    effect->vm.interpEndTimes[4] = 30;
    effect->vm.easeModes[4] = 0;
    effect->vm.scaleInterpInitial.x = 1.0f / 16.0f;
    effect->vm.scaleInterpInitial.y = 1.0f / 16.0f;
    effect->vm.scaleInterpFinal.x = 1.3f;
    effect->vm.scaleInterpFinal.y = 1.3f;
    effect->vm.interpStartTimes[2] = 0;
    effect->vm.interpEndTimes[2] = 30;
    effect->vm.easeModes[2] = 1;
    effect->vm.colorInterpInitialColor.bytes.a = effect->vm.color.bytes.a;
    effect->vm.colorInterpFinalColor.bytes.a = 0;
    effect->vm.intVars1[0] = 30;
    this->borderEffect = effect;
    g_EnemyManager.spellcardInfo.captureScore = 0;
    g_EnemyManager.spellcardInfo.isCapturing = 0;
    this->hasBorder = BORDER_NONE;
    this->playerState = PLAYER_STATE_INVULNERABLE;
    this->invulnerabilityTimer = 40;
    this->borderInvulnerabilityTime = 40;
    g_GameManager.cherryPlus = g_GameManager.globals->cherryStart;
    SpawnBombEffect(&this->pos, 32.0f, 16.0f, 50, 8);
    angle = -ZUN_PI;
    for (i = 0; i < 32; i++, angle += ZUN_PI / 16.0f)
    {
        effect = g_EffectManager.SpawnEffect(29, &this->pos, 1, 0xffffffff);
        effect->direction.x = cosf(angle);
        effect->direction.y = sinf(angle);
    }
    g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_MARISA_A_FOCUS, 0);
    g_SoundPlayer.PlaySoundByIdx(SOUND_BORDER_BREAK, 0);
    g_ReplayManager->replayEventFlags = g_ReplayManager->replayEventFlags | 0x10;
}

void Player::UpdateUI()
{
    this->positionOfLastEnemyHit = ZunVec3(-999.0f, -999.0f, 0.0f);
    this->sakuyaTargetPosition = ZunVec3(-999.0f, -999.0f, 0.0f);
    this->targetingEnemy = 0;
    if (this->pos.y >= 400.0f)
    {
        if (g_AsciiManager.GetFadeState() != 2 && this->pos.x < 160.0f)
        {
            g_AsciiManager.cherryGauge.pendingInterrupt = 2;
            g_AsciiManager.uiFadeState = 2;
        }
        else if (g_AsciiManager.GetFadeState() == 2 && this->pos.x > 160.0f)
        {
            g_AsciiManager.cherryGauge.pendingInterrupt = 3;
            g_AsciiManager.uiFadeState = 3;
        }
    }
    else if (g_AsciiManager.GetFadeState() == 2)
    {
        g_AsciiManager.cherryGauge.pendingInterrupt = 3;
        g_AsciiManager.uiFadeState = 3;
    }
}

u32 Player::OnUpdate(Player *arg)
{
    arg->UpdatePrev();

    arg->prevPos = arg->pos;
    arg->prevOptionsPosition[0] = arg->optionsPosition[0];
    arg->prevOptionsPosition[1] = arg->optionsPosition[1];
    if (g_GameManager.isTimeStopped)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    arg->UpdateBombProjectiles();
    arg->UpdateBorderAndBombState();
    if (arg->playerState == PLAYER_STATE_DEAD)
    {
        if (arg->UpdateDeath())
        {
            goto WHAT;
        }
        else
        {
            goto WHY;
        }
    }
    if (arg->playerState == PLAYER_STATE_SPAWNING)
    {
    WHAT:
        arg->Respawn();
    }
WHY:
    arg->UpdateState();
    if (arg->playerState != PLAYER_STATE_DEAD && arg->playerState != PLAYER_STATE_SPAWNING)
    {
        arg->HandlePlayerInputs();
    }
    g_AnmManager->ExecuteScript(&arg->playerSprite);
    if (arg->optionState != OPTION_HIDDEN)
    {
        g_AnmManager->ExecuteScript(&arg->optionsSprite[0]);
        g_AnmManager->ExecuteScript(&arg->optionsSprite[1]);
    }
    arg->UpdateShots();
    arg->UpdateFireBulletTimer();
    arg->UpdateUI();
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

u32 Player::OnDrawHighPrio(Player *arg)
{
    ZunColor color;

    arg->DrawBullets();
    if (arg->bombInfo.isInUse)
    {
        if (!arg->bombInfo.isFocus)
        {
            arg->bombInfo.draw(arg);
        }
        else
        {
            arg->bombInfo.drawFocus(arg);
        }
    }
    if (!g_GameManager.isInRetryMenu)
    {
        ZunVec3 drawPlayerPos = arg->prevPos.Lerp(arg->pos, g_RenderAlpha);
        ZunVec3 drawOptionsPos[2] = {
            arg->prevOptionsPosition[0].Lerp(arg->optionsPosition[0], g_RenderAlpha),
            arg->prevOptionsPosition[1].Lerp(arg->optionsPosition[1], g_RenderAlpha)};
        arg->playerSprite.pos.x = g_GameManager.arcadeRegionTopLeftPos.x + drawPlayerPos.x;
        arg->playerSprite.pos.y = g_GameManager.arcadeRegionTopLeftPos.y + drawPlayerPos.y;
        arg->playerSprite.pos.z = 0.0f;
        g_AnmManager->DrawNoRotation(&arg->playerSprite);
        if (arg->optionState != OPTION_HIDDEN &&
            (arg->playerState == PLAYER_STATE_ALIVE || arg->playerState == PLAYER_STATE_BORDER ||
             arg->playerState == PLAYER_STATE_INVULNERABLE))
        {
            arg->optionsSprite[0].pos.x =
                g_GameManager.arcadeRegionTopLeftPos.x + drawOptionsPos[0].x;
            arg->optionsSprite[0].pos.y =
                g_GameManager.arcadeRegionTopLeftPos.y + drawOptionsPos[0].y;
            arg->optionsSprite[0].pos.z = 0.0f;
            arg->optionsSprite[1].pos.x =
                g_GameManager.arcadeRegionTopLeftPos.x + drawOptionsPos[1].x;
            arg->optionsSprite[1].pos.y =
                g_GameManager.arcadeRegionTopLeftPos.y + drawOptionsPos[1].y;
            arg->optionsSprite[1].pos.z = 0.0f;
            g_AnmManager->Draw(&arg->optionsSprite[0]);
            g_AnmManager->Draw(&arg->optionsSprite[1]);
        }
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

u32 Player::OnDrawLowPrio(Player *arg)
{
    arg->DrawBulletExplosions();
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

f32 Player::AngleToPlayer(ZunVec3 *pos)
{
    f32 y;
    f32 x;

    x = this->pos.x - pos->x;
    y = this->pos.y - pos->y;
    if (y == 0.0f && x == 0.0f)
    {
        return ZUN_PI / 2.0f;
    }
    else
    {
        return atan2f(y, x);
    }
}

ZunResult Player::AddedCallback(Player *arg)
{
    PlayerBullet *bullet;
    i32 i;

    if (ShtData::LoadShtData(&arg->shooterData,
                             g_ShooterTable[g_GameManager.shotTypeAndCharacter]) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    if (ShtData::LoadShtData(&arg->shooterDataFocus,
                             g_ShooterTableFocus[g_GameManager.shotTypeAndCharacter]) !=
        ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    if ((u32)(g_Supervisor.curState != SUPERVISOR_STATE_NEXT_STAGE &&
              g_Supervisor.curState != SUPERVISOR_STATE_RESTART_STAGE &&
              g_Supervisor.curState != SUPERVISOR_STATE_NEXT_STAGE_USELESS))
    {
        switch (g_GameManager.character)
        {
        case CHAR_REIMU:
            if (g_AnmManager->LoadAnms(ANM_FILE_PLAYER, "data/player00.anm", ANM_OFFSET_PLAYER) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
            break;
        case CHAR_MARISA:
            if (g_AnmManager->LoadAnms(ANM_FILE_PLAYER, "data/player01.anm", ANM_OFFSET_PLAYER) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
            break;
        case CHAR_SAKUYA:
            if (g_AnmManager->LoadAnms(ANM_FILE_PLAYER, "data/player02.anm", ANM_OFFSET_PLAYER) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
        }
    }
    g_AnmManager->SetAnmIdxAndExecuteScript(&arg->playerSprite, ANM_SCRIPT_PLAYER_IDLE);
    arg->pos.x = g_GameManager.arcadeRegionSize.x / 2.0f;
    arg->pos.y = g_GameManager.arcadeRegionSize.y - 64.0f;
    arg->pos.z = 0.49f;
    arg->optionsPosition[0].z = 0.49f;
    arg->optionsPosition[1].z = 0.49f;
    arg->prevPos = arg->pos;
    arg->prevOptionsPosition[0] = arg->optionsPosition[0];
    arg->prevOptionsPosition[1] = arg->optionsPosition[1];

    for (i = 0; i < 112; i++)
    {
        arg->bombDamageBoxes[i].size.x = 0.0f;
    }
    for (i = 0; i < 16; i++) // this looks kinda suspect but thats basically what the old code did
    {
        arg->bombClearBoxes[i].size.x = 0.0f;
    }
    arg->hitboxSize.y = g_Player.shooterData->hitboxRadius / 2.0f;
    arg->hitboxSize.x = arg->hitboxSize.y;
    arg->hitboxSize.z = 5.0f;
    arg->grazeSize.y = g_Player.shooterData->grabItemRadius / 2.0f;
    arg->grazeSize.x = arg->grazeSize.y;
    arg->grazeSize.z = 5.0f;
    arg->grabItemSize.x = 12.0f;
    arg->grabItemSize.y = 12.0f;
    arg->grabItemSize.z = 5.0f;
    arg->playerDirection = MOVEMENT_NONE;
    arg->playerState = PLAYER_STATE_SPAWNING;
    arg->invulnerabilityTimer = 120;
    arg->optionState = OPTION_UNFOCUSED;
    g_AnmManager->SetAnmIdxAndExecuteScript(&arg->optionsSprite[0], ANM_SCRIPT_PLAYER_OPTION_LEFT);
    g_AnmManager->SetAnmIdxAndExecuteScript(&arg->optionsSprite[1], ANM_SCRIPT_PLAYER_OPTION_RIGHT);
    bullet = arg->bullets;
    for (i = 0; i < ARRAY_SIZE_SIGNED(arg->bullets); i++, bullet++)
    {
        bullet->bulletState = 0;
    }
    arg->fireBulletTimer = -1;
    arg->bombInfo.bombCalc = g_BombData[g_GameManager.shotTypeAndCharacter].calc;
    arg->bombInfo.draw = g_BombData[g_GameManager.shotTypeAndCharacter].draw;
    arg->bombInfo.bombFocusCalc = g_BombData[g_GameManager.shotTypeAndCharacter].calcFocus;
    arg->bombInfo.drawFocus = g_BombData[g_GameManager.shotTypeAndCharacter].drawFocus;
    arg->bombInfo.isInUse = 0;
    arg->dirtyBombBoxes = true;
    arg->numActiveBombClearBoxes = 0;
    arg->optionAngle = -ZUN_PI / 2.0f;
    arg->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
    arg->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
    arg->respawnTimer = g_Player.shooterData->initialRespawnTimer;
    if ((u32)(g_Supervisor.curState != SUPERVISOR_STATE_NEXT_STAGE &&
              g_Supervisor.curState != SUPERVISOR_STATE_RESTART_STAGE &&
              g_Supervisor.curState != SUPERVISOR_STATE_NEXT_STAGE_USELESS))
    {
        g_AsciiManager.cherryGauge.pendingInterrupt = 1;
        g_AsciiManager.uiFadeState = 1;
    }
    g_AsciiManager.GetBossMarker(0)->pendingInterrupt = 2;
    g_AsciiManager.GetBossMarker(1)->pendingInterrupt = 2;
    g_AsciiManager.GetBossMarker(2)->pendingInterrupt = 2;
    if (g_GameManager.cherryPlus >= g_GameManager.globals->cherryStart + 50000)
    {
        g_GameManager.cherryPlus = g_GameManager.globals->cherryStart + 50000;
        g_Player.ActivateBorder();
    }
    return ZUN_SUCCESS;
}

ZunResult Player::DeletedCallback(Player *arg)
{
    (void)arg;

    if ((u32)(g_Supervisor.curState != SUPERVISOR_STATE_NEXT_STAGE &&
              g_Supervisor.curState != SUPERVISOR_STATE_RESTART_STAGE &&
              g_Supervisor.curState != SUPERVISOR_STATE_NEXT_STAGE_USELESS))
    {
        g_AnmManager->ReleaseAnm(ANM_FILE_PLAYER);
        g_AsciiManager.cherryGauge.pendingInterrupt = 99;
        g_AsciiManager.uiFadeState = 99;
        g_AsciiManager.GetBossMarker(0)->pendingInterrupt = 99;
        g_AsciiManager.GetBossMarker(1)->pendingInterrupt = 99;
        g_AsciiManager.GetBossMarker(2)->pendingInterrupt = 99;
    }
    SAFE_DELETE_ARRAY(g_Player.shooterData->levels);
    SAFE_DELETE_ARRAY(g_Player.shooterData->entries);
    SAFE_DELETE(g_Player.shooterData);
    SAFE_DELETE_ARRAY(g_Player.shooterDataFocus->levels);
    SAFE_DELETE_ARRAY(g_Player.shooterDataFocus->entries);
    SAFE_DELETE(g_Player.shooterDataFocus);
    return ZUN_SUCCESS;
}

ZunResult Player::RegisterChain(u32 param_1)
{
    Player *mgr = &g_Player;
    memset(mgr, 0, sizeof(Player));
    mgr->invulnerabilityTimer = 0;
    mgr->initParam = param_1;
    mgr->calcChain = g_Chain.CreateElem((ChainCallback)OnUpdate);
    mgr->drawChain1 = g_Chain.CreateElem((ChainCallback)OnDrawHighPrio);
    mgr->drawChain2 = g_Chain.CreateElem((ChainCallback)OnDrawLowPrio);
    mgr->calcChain->arg = mgr;
    mgr->drawChain1->arg = mgr;
    mgr->drawChain2->arg = mgr;
    mgr->calcChain->addedCallback = (ChainLifecycleCallback)AddedCallback;
    mgr->calcChain->deletedCallback = (ChainLifecycleCallback)DeletedCallback;
    if (g_Chain.AddToCalcChain(mgr->calcChain, 8))
    {
        return ZUN_ERROR;
    }

    g_Chain.AddToDrawChain(mgr->drawChain1, 6);
    g_Chain.AddToDrawChain(mgr->drawChain2, 8);
    return ZUN_SUCCESS;
}

void Player::CutChain()
{
    g_Chain.Cut(g_Player.calcChain);
    g_Player.calcChain = NULL;
    g_Chain.Cut(g_Player.drawChain1);
    g_Player.drawChain1 = NULL;
    g_Chain.Cut(g_Player.drawChain2);
    g_Player.drawChain2 = NULL;
}

ZunResult ShtData::LoadShtData(ShtData **data, const char *shtPath)
{
    u8 *rawFile = FileSystem::OpenFile(shtPath, 0);
    if (!rawFile)
    {
        return ZUN_ERROR;
    }

    ShtRawData *rawData = (ShtRawData *)rawFile;

    ShtData *parsed = new ShtData;
    memcpy(parsed, rawData, offsetof(ShtRawData, levels));

    parsed->levels = new ShtLevel[parsed->numLevels];

    i32 totalEntries = 0;
    for (i32 i = 0; i < parsed->numLevels; i++)
    {
        ShtRawEntry *re = (ShtRawEntry *)(rawFile + rawData->levels[i].entryOffset);
        while (re->fireInterval >= 0)
        {
            totalEntries++;
            re++;
        }
        totalEntries++;
    }

    parsed->entries = new ShtEntry[totalEntries];

    i32 entryIdx = 0;
    for (i32 i = 0; i < parsed->numLevels; i++)
    {
        parsed->levels[i].requiredPower = rawData->levels[i].requiredPower;
        parsed->levels[i].entry = &parsed->entries[entryIdx];

        ShtRawEntry *re = (ShtRawEntry *)(rawFile + rawData->levels[i].entryOffset);
        while (re->fireInterval >= 0)
        {
            ShtEntry *e = &parsed->entries[entryIdx++];
            e->fireInterval = re->fireInterval;
            e->fireOffset = re->fireOffset;
            e->offset = re->offset;
            e->hitboxSize = re->hitboxSize;
            e->angle = re->angle;
            e->speed = re->speed;
            e->damage = re->damage;
            e->option = re->option;
            e->bulletState2 = re->bulletState2;
            e->anmFileIdx = re->anmFileIdx;
            e->soundIdx = re->soundIdx;
            e->fireCallback = re->fireCallback < 6 ? g_ShtFireFuncs[re->fireCallback] : NULL;
            e->updateCallback =
                re->updateCallback < 6 ? g_ShtUpdateFuncs[re->updateCallback] : NULL;
            e->drawCallback = re->drawCallback < 2 ? g_ShtDrawFuncs[re->drawCallback] : NULL;
            e->hitCallback = re->hitCallback < 4 ? g_ShtHitFuncs[re->hitCallback] : NULL;
            re++;
        }
        parsed->entries[entryIdx].fireInterval = -1;
        entryIdx++;
    }

    free(rawFile);
    *data = parsed;
    return ZUN_SUCCESS;
}
