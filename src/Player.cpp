#include "Player.hpp"

#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "BombData.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "EffectManager.hpp"
#include "EnemyManager.hpp"
#include "FileSystem.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "ZunMath.hpp"
#include "d3dx8.h"
#include "dxutil.hpp"
#include "utils.hpp"

// GLOBAL: TH07 0x0049ecb0
ShtFunc1 g_ShtFireFuncs[6] = {
    NULL,
    ShtData::FireBulletDefault,
    ShtData::FireOrbBulletUnfocused,
    ShtData::FireOrbBulletFocused,
    ShtData::FireHomingBullet,
    ShtData::FireRotatingOrbBullet,
};

// GLOBAL: TH07 0x0049ecc8
ShtFunc2 g_ShtUpdateFuncs[6] = {
    NULL,
    ShtData::UpdateHomingBullet,
    ShtData::UpdateHomingBulletFocused,
    ShtData::UpdateUpwardAcceleratingBullet,
    ShtData::UpdateOrbLaser,
    ShtData::UpdatePlayerLaser,
};

// GLOBAL: TH07 0x0049ece0
ShtFunc3 g_ShtDrawFuncs[2] = {
    NULL,
    ShtData::DrawBulletWithTrail,
};

// GLOBAL: TH07 0x0049ece8
ShtFunc4 g_ShtHitFuncs[4] = {
    NULL,
    ShtData::OnMissileHit,
    ShtData::SpawnHitParticles,
    (ShtFunc4)0x00000001, // i guess bro
};

// GLOBAL: TH07 0x0049f530
const char *g_ShooterTable[6] = {
    // STRING: TH07 0x00496bb4
    "data/ply00a.sht",
    // STRING: TH07 0x00496ba4
    "data/ply00b.sht",
    // STRING: TH07 0x00496b94
    "data/ply01a.sht",
    // STRING: TH07 0x00496b84
    "data/ply01b.sht",
    // STRING: TH07 0x00496b74
    "data/ply02a.sht",
    // STRING: TH07 0x00496b64
    "data/ply02b.sht",
};

// GLOBAL: TH07 0x0049f548
const char *g_ShooterTable2[6] = {
    // STRING: TH07 0x00496b50
    "data/ply00as.sht",
    // STRING: TH07 0x00496b3c
    "data/ply00bs.sht",
    // STRING: TH07 0x00496b28
    "data/ply01as.sht",
    // STRING: TH07 0x00496b14
    "data/ply01bs.sht",
    // STRING: TH07 0x00496b00
    "data/ply02as.sht",
    // STRING: TH07 0x00496aec
    "data/ply02bs.sht",
};

// GLOBAL: TH07 0x004bdad8
Player g_Player;

// FUNCTION: TH07 0x0043b7f0
Player::Player()
{
}

// FUNCTION: TH07 0x0043ba10
PlayerBullet::PlayerBullet()
{
}

// FUNCTION: TH07 0x0043baa0
PlayerBombInfo::PlayerBombInfo()
{
}

// FUNCTION: TH07 0x0043bb10
PlayerBombSubInfo::PlayerBombSubInfo()
{
}

// FUNCTION: TH07 0x0043bbd0
void DefaultFireBulletCallback(Player *player, PlayerBullet *bullet,
                               ShtEntry *shtEntry)
{
    if (shtEntry->option == 0)
    {
        bullet->pos = player->positionCenter;
    }
    else
    {
        bullet->pos = player->orbsPosition[shtEntry->option - 1];
    }
    bullet->pos.x += shtEntry->offset.x;
    bullet->pos.y += shtEntry->offset.y;
    bullet->pos.z = 0.495f;
    bullet->hitboxSize.x = shtEntry->hitboxSize.x;
    bullet->hitboxSize.y = shtEntry->hitboxSize.y;
    bullet->hitboxSize.z = 1.0f;
    bullet->angle = shtEntry->angle;
    bullet->speed = shtEntry->speed;
    bullet->velocity.x = cosf(shtEntry->angle) * shtEntry->speed;
    bullet->velocity.y = sinf(shtEntry->angle) * shtEntry->speed;
    bullet->timer.Initialize(0);
    bullet->bulletState2 = shtEntry->bulletState2;
    bullet->damage = shtEntry->damage;
    if (-1 < shtEntry->soundIdx)
    {
        g_SoundPlayer.PlaySoundByIdx((SoundIdx)shtEntry->soundIdx, 0);
    }
    bullet->vm.anmFileIdx = shtEntry->anmFileIdx;
    g_AnmManager->SetAndExecuteScript(
        &bullet->vm, g_AnmManager->scripts[shtEntry->anmFileIdx]);
}

// FUNCTION: TH07 0x0043bdc0
i32 ShtData::FireBulletDefault(Player *player, PlayerBullet *bullet,
                               i32 fireTime, ShtEntry *shtEntry)
{
    bool bVar1;

    bVar1 = fireTime % (i32)shtEntry->fireInterval == (i32)shtEntry->fireOffset;
    if (bVar1)
    {
        DefaultFireBulletCallback(player, bullet, shtEntry);
    }
    return bVar1;
}

// FUNCTION: TH07 0x0043be10
i32 ShtData::FireOrbBulletUnfocused(Player *player, PlayerBullet *bullet,
                                    i32 fireTime, ShtEntry *shtEntry)
{
    i16 sVar2 = shtEntry->fireOffset;
    if (player->timers[sVar2].bullet == NULL)
    {
        if (player->orbState == ORB_UNFOCUSED)
        {
            player->timers[sVar2].Initialize(shtEntry->fireInterval);
            player->timers[sVar2].bullet = bullet;
            bullet->timerIdx = sVar2;
            bullet->optionId = (i16)shtEntry->option;
            bullet->offset = shtEntry->offset;
            DefaultFireBulletCallback(player, bullet, shtEntry);
            player->shtEntries[sVar2] = shtEntry;
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        if (player->shtEntries[sVar2] != shtEntry)
        {
            ((player->timers[sVar2].bullet)->vm).pendingInterrupt = 1;
            player->timers[sVar2].bullet = NULL;
        }
        return 0;
    }
}

// FUNCTION: TH07 0x0043bf50
i32 ShtData::FireOrbBulletFocused(Player *player, PlayerBullet *bullet,
                                  i32 fireTime, ShtEntry *shtEntry)
{
    i16 sVar2 = shtEntry->fireOffset;
    if (player->timers[sVar2].bullet == NULL)
    {
        if (player->orbState == ORB_FOCUSED)
        {
            player->timers[sVar2].Initialize(999);
            player->timers[sVar2].bullet = bullet;
            bullet->timerIdx = sVar2;
            bullet->optionId = (i16)shtEntry->option;
            bullet->offset = shtEntry->offset;
            bullet->trailLength = shtEntry->fireInterval;
            DefaultFireBulletCallback(player, bullet, shtEntry);
            for (i32 i = 0xf; -1 < i; i += -1)
            {
                bullet->posHistory[i].x = -999.0f;
            }
            bullet->pos.x = -999.0f;
            player->shtEntries[sVar2] = shtEntry;
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        if (player->shtEntries[sVar2] != shtEntry)
        {
            ((player->timers[sVar2].bullet)->vm).pendingInterrupt = 1;
            player->timers[sVar2].bullet = NULL;
        }
        return 0;
    }
}

// FUNCTION: TH07 0x0043c0d0
i32 ShtData::FireHomingBullet(Player *player, PlayerBullet *bullet,
                              i32 fireTime, ShtEntry *shtEntry)
{
    f32 fVar2;

    if (fireTime % (i32)shtEntry->fireInterval == (i32)shtEntry->fireOffset)
    {
        DefaultFireBulletCallback(player, bullet, shtEntry);
        if (-100.0f < (player->sakuyaTargetPosition).x)
        {
            fVar2 = utils::AddNormalizeAngle(
                atan2f(((player->sakuyaTargetPosition).y - bullet->pos.y),
                       ((player->sakuyaTargetPosition).x - bullet->pos.x)),
                shtEntry->angle + 1.5707964f);
            AngleToVector((D3DXVECTOR3 *)&bullet->velocity, fVar2,
                          shtEntry->speed * 1.5f);
            bullet->angle = fVar2;
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

// FUNCTION: TH07 0x0043c1c0
i32 ShtData::FireRotatingOrbBullet(Player *player, PlayerBullet *bullet,
                                   i32 fireTime, ShtEntry *shtEntry)
{
    bool bVar1;
    f32 angle;

    bVar1 = fireTime % (i32)shtEntry->fireInterval == (i32)shtEntry->fireOffset;
    if (bVar1)
    {
        DefaultFireBulletCallback(player, bullet, shtEntry);
        angle = utils::AddNormalizeAngle(player->orbAngle,
                                         shtEntry->angle + 1.5707964f);
        AngleToVector((D3DXVECTOR3 *)&bullet->velocity, angle, shtEntry->speed);
        bullet->angle = angle;
    }
    return bVar1;
}

// FUNCTION: TH07 0x0043c250
i32 ShtData::UpdateHomingBullet(Player *player, PlayerBullet *bullet)
{
    f32 fVar1;
    f32 fVar2;
    f32 fVar3;
    f32 local_2c;
    f32 local_10;

    if (bullet->bulletState == 1)
    {
        if ((((player->positionOfLastEnemyHit).x <= -100.0f) ||
             (0x27 < bullet->timer.current)) ||
            (bullet->timer.current == bullet->timer.previous))
        {
            if (bullet->speed < 10.0f)
            {
                bullet->speed = bullet->speed + 0.33333334f;
                fVar1 = bullet->velocity.x;
                fVar2 = bullet->velocity.y;
                fVar3 = sqrtf(fVar1 * fVar1 + fVar2 * fVar2);
                bullet->velocity.x = (fVar1 * bullet->speed) / fVar3;
                bullet->velocity.y = (fVar2 * bullet->speed) / fVar3;
            }
        }
        else
        {
            fVar1 = (player->positionOfLastEnemyHit).x - bullet->pos.x;
            fVar2 = (player->positionOfLastEnemyHit).y - bullet->pos.y;
            local_10 = sqrtf(fVar1 * fVar1 + fVar2 * fVar2);
            local_10 = local_10 / (bullet->speed / 4.0f);
            if (local_10 < 1.0f)
            {
                local_10 = 1.0f;
            }
            fVar1 = fVar1 / local_10 + bullet->velocity.x;
            fVar2 = fVar2 / local_10 + bullet->velocity.y;
            fVar3 = sqrtf(fVar1 * fVar1 + fVar2 * fVar2);
            local_2c = fVar3;
            if (10.0f < fVar3)
            {
                local_2c = 10.0f;
            }
            bullet->speed = local_2c;
            if (bullet->speed < 1.0f)
            {
                bullet->speed = 1.0f;
            }
            bullet->velocity.x = (fVar1 * bullet->speed) / fVar3;
            bullet->velocity.y = (fVar2 * bullet->speed) / fVar3;
        }
    }
    return 0;
}

// FUNCTION: TH07 0x0043c480
i32 ShtData::UpdateHomingBulletFocused(Player *player, PlayerBullet *bullet)
{
    f32 fVar1;
    f32 fVar2;
    f32 fVar3;
    f32 local_2c;
    f32 local_10;

    if (bullet->bulletState == 1)
    {
        if ((((player->positionOfLastEnemyHit).x <= -100.0f) ||
             (0x27 < bullet->timer.current)) ||
            (bullet->timer.current == bullet->timer.previous))
        {
            if (bullet->speed < 18.0f)
            {
                bullet->speed = bullet->speed + 0.6f;
                fVar1 = bullet->velocity.x;
                fVar2 = bullet->velocity.y;
                fVar3 = sqrtf(fVar1 * fVar1 + fVar2 * fVar2);
                bullet->velocity.x = (fVar1 * bullet->speed) / fVar3;
                bullet->velocity.y = (fVar2 * bullet->speed) / fVar3;
            }
        }
        else
        {
            fVar1 = (player->positionOfLastEnemyHit).x - bullet->pos.x;
            fVar2 = (player->positionOfLastEnemyHit).y - bullet->pos.y;
            local_10 = sqrtf(fVar1 * fVar1 + fVar2 * fVar2);
            local_10 = local_10 / (bullet->speed / 4.0f);
            if (local_10 < 1.0f)
            {
                local_10 = 1.0f;
            }
            fVar1 = fVar1 / local_10 + bullet->velocity.x;
            fVar2 = fVar2 / local_10 + bullet->velocity.y;
            fVar3 = sqrtf(fVar1 * fVar1 + fVar2 * fVar2);
            local_2c = fVar3;
            if (18.0f < fVar3)
            {
                local_2c = 18.0f;
            }
            bullet->speed = local_2c;
            if (bullet->speed < 1.0f)
            {
                bullet->speed = 1.0f;
            }
            bullet->velocity.x = (fVar1 * bullet->speed) / fVar3;
            bullet->velocity.y = (fVar2 * bullet->speed) / fVar3;
        }
    }
    return 0;
}

// FUNCTION: TH07 0x0043c6b0
i32 ShtData::UpdateUpwardAcceleratingBullet(Player *player,
                                            PlayerBullet *bullet)
{
    if (bullet->bulletState == 1)
    {
        bullet->velocity.y =
            bullet->velocity.y - (g_Rng.GetRandomFloatInRange(0.1f) + 0.27f);
    }
    return 0;
}

// FUNCTION: TH07 0x0043c700
i32 ShtData::UpdateOrbLaser(Player *player, PlayerBullet *bullet)
{
    if ((player->timers[bullet->timerIdx].bullet != bullet) &&
        (bullet->vm.isStopped != 0))
    {
        bullet->vm.pendingInterrupt = 1;
    }
    if (((g_Gui.HasCurrentMsgIdx() != 0) || ((player->bombInfo).isInUse != 0)) &&
        (0x14 < player->timers[bullet->timerIdx].current))
    {
        player->timers[bullet->timerIdx].Initialize(0x14);
    }
    if (player->timers[bullet->timerIdx].current < 1)
    {
        player->timers[bullet->timerIdx].Initialize(0);
        player->timers[bullet->timerIdx].bullet = NULL;
        bullet->bulletState = 0;
        return 1;
    }
    else
    {
        if ((player->timers[bullet->timerIdx].current < 0x47) &&
            (bullet->vm.isStopped != 0))
        {
            bullet->vm.pendingInterrupt = 1;
        }
        bullet->pos = player->orbsPosition[bullet->optionId - 1];
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
}

// FUNCTION: TH07 0x0043c940
i32 ShtData::UpdatePlayerLaser(Player *player, PlayerBullet *bullet)
{
    if ((player->timers[bullet->timerIdx].bullet != bullet) &&
        (bullet->vm.isStopped != 0))
    {
        bullet->vm.pendingInterrupt = 1;
    }
    if (((g_Gui.HasCurrentMsgIdx() != 0) || ((player->bombInfo).isInUse != 0)) &&
        (0x14 < player->timers[bullet->timerIdx].current))
    {
        player->timers[bullet->timerIdx].Initialize(0x14);
    }
    if (player->timers[bullet->timerIdx].current < 1)
    {
        player->timers[bullet->timerIdx].Initialize(0);
        bullet->bulletState = 0;
        player->timers[bullet->timerIdx].bullet = NULL;
        return 1;
    }
    else
    {
        if ((player->timers[bullet->timerIdx].current < 0x47) &&
            (bullet->vm.isStopped != 0))
        {
            bullet->vm.pendingInterrupt = 1;
        }
        for (i32 i = 0; i < bullet->trailLength; i++)
        {
            if (-900.0f <= bullet->posHistory[i].x)
            {
                player->playerLasers[i + 0x58].pos = bullet->posHistory[i];
                player->playerLasers[i + 0x58].active = 1;
                player->playerLasers[i + 0x58].size = bullet->hitboxSize;
            }
        }
        for (i32 i = 0xf; 0 < i; i += -1)
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
            bullet->pos = player->positionCenter;
            bullet->pos.x += bullet->offset.x;
            bullet->pos.z = 0.44f;
            bullet->vm.scale.y = (bullet->pos.y + 64.0f) / 14.0f;
            bullet->hitboxSize.y = player->positionCenter.y + 64.0f;
            bullet->pos.y = bullet->pos.y / 2.0f - 32.0f;
            return 0;
        }
    }
}

// FUNCTION: TH07 0x0043ccb0
i32 ShtData::DrawBulletWithTrail(Player *player, PlayerBullet *bullet)
{
    u8 origAlpha;
    i32 local_c;

    origAlpha = bullet->vm.color.bytes.a;
    for (local_c = 0; (local_c < bullet->trailLength &&
                       (bullet->posHistory[local_c].x != -999.0f));
         local_c += 1)
    {
        bullet->vm.pos = bullet->posHistory[local_c];
        bullet->vm.color.bytes.a =
            origAlpha -
            (char)((i32)((u32)origAlpha * local_c) / (i32)bullet->trailLength);
        bullet->vm.pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        bullet->vm.pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        g_AnmManager->Draw(&bullet->vm);
    }
    bullet->vm.color.bytes.a = origAlpha;
    return 0;
}

// FUNCTION: TH07 0x0043cde0
i32 ShtData::OnMissileHit(Player *player, PlayerBullet *bullet,
                          D3DXVECTOR3 *pos)
{
    f32 fVar2;

    if (bullet->bulletState == 2)
    {
        if (bullet->timer.current % 2 != 0)
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
        fVar2 = g_Rng.GetRandomFloatInRange(1.5707964f) - 2.3561945f;
        switch (bullet->vm.anmFileIdx)
        {
        case 0x441:
            bullet->hitboxSize.x = 32.0f;
            bullet->hitboxSize.y = 32.0f;
            AngleToVector((D3DXVECTOR3 *)&bullet->velocity, fVar2, 4.0f);
            break;
        case 0x442:
            bullet->hitboxSize.x = 42.0;
            bullet->hitboxSize.y = 42.0;
            AngleToVector((D3DXVECTOR3 *)&bullet->velocity, fVar2, 4.0f);
            break;
        case 0x443:
            bullet->hitboxSize.x = 48.0f;
            bullet->hitboxSize.y = 48.0f;
            AngleToVector((D3DXVECTOR3 *)&bullet->velocity, fVar2, 4.0f);
            break;
        case 0x444:
            bullet->hitboxSize.x = 56.0f;
            bullet->hitboxSize.y = 56.0f;
            AngleToVector((D3DXVECTOR3 *)&bullet->velocity, fVar2, 4.0f);
            break;
        case 0x445:
            bullet->hitboxSize.x = 48.0f;
            bullet->hitboxSize.y = 48.0f;
            AngleToVector((D3DXVECTOR3 *)&bullet->velocity, fVar2, 6.0f);
            break;
        case 0x446:
            bullet->hitboxSize.x = 64.0f;
            bullet->hitboxSize.y = 64.0f;
            AngleToVector((D3DXVECTOR3 *)&bullet->velocity, fVar2, 6.0f);
            break;
        case 0x447:
            bullet->hitboxSize.x = 80.0f;
            bullet->hitboxSize.y = 80.0f;
            AngleToVector((D3DXVECTOR3 *)&bullet->velocity, fVar2, 6.0f);
            break;
        case 0x448:
            bullet->hitboxSize.x = 96.0f;
            bullet->hitboxSize.y = 96.0f;
            AngleToVector((D3DXVECTOR3 *)&bullet->velocity, fVar2, 6.0f);
        }
    }
    if (bullet->timer.current % 6 == 0)
    {
        g_EffectManager.SpawnParticles(5, pos, 1, 0xffffffff);
    }
    return 0;
}

// FUNCTION: TH07 0x0043d0e0
i32 ShtData::SpawnHitParticles(Player *player, PlayerBullet *bullet,
                               D3DXVECTOR3 *pos)
{
    D3DXVECTOR3 local_10;

    player->bombParticleTime = player->bombParticleTime + 1;
    if ((player->bombParticleTime & 0x80000007) == 0)
    {
        local_10.y = pos->y;
        local_10.z = pos->z;
        local_10.x = bullet->pos.x;
        g_EffectManager.SpawnParticles(5, &local_10, 1, 0xffffffff);
    }
    return 0;
}

// FUNCTION: TH07 0x0043d160
void Player::SpawnBullets(Player *player, u32 timer)
{
    ShtData *pSVar1;
    ShtLevel *local_24;
    ShtEntry *entry;
    i32 local_14;
    PlayerBullet *bullet;
    ShtLevel *local_c;
    i32 local_8;

    if (player->isFocus == 0)
    {
        pSVar1 = player->shooterData;
    }
    else
    {
        pSVar1 = player->shooterData2;
    }
    local_24 = &pSVar1->levels;
    for (local_c = local_24;
         local_c->requiredPower <= (i32)g_GameManager.globals->currentPower;
         local_c = local_c + 1)
    {
    }
    entry = local_c->entry;
    bullet = player->bullets;
    local_8 = 0;
    while (local_8 <= 0x5f)
    {
        if (bullet->bulletState == 0)
        {
            do
            {
                if (entry->fireCallback == NULL)
                {
                    local_14 = ShtData::FireBulletDefault(player, bullet, timer, entry);
                }
                else
                {
                    local_14 = (*entry->fireCallback)(player, bullet, timer, entry);
                }
                if (local_14 == 1)
                {
                    bullet->vm.zWriteDisable = 1;
                    bullet->bulletState = 1;
                    bullet->shtEntry = entry;
                    bullet->updateCallback = bullet->shtEntry->updateCallback;
                    bullet->drawCallback = bullet->shtEntry->drawCallback;
                    bullet->hitCallback = bullet->shtEntry->hitCallback;
                }
                entry = entry + 1;
                if (entry->fireInterval < 0)
                {
                    return;
                }
            } while (local_14 == 0);
        }
        local_8 += 1;
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x0043d2f0
void Player::UpdateShots()
{
    PlayerBullet *bullet;

    if ((this->orbState != ORB_FOCUSED) && (this->timers[2].bullet != NULL))
    {
        (this->timers[2].bullet)->bulletState = 0;
        this->timers[2].bullet = NULL;
    }
    if (this->orbState != ORB_UNFOCUSED)
    {
        if (this->timers[0].bullet != NULL)
        {
            ((this->timers[0].bullet)->vm).pendingInterrupt = 1;
            this->timers[0].bullet = NULL;
        }
        if (this->timers[1].bullet != NULL)
        {
            ((this->timers[1].bullet)->vm).pendingInterrupt = 1;
            this->timers[1].bullet = NULL;
        }
    }
    if (this->playerState == PLAYER_STATE_DEAD)
    {
        for (i32 i = 0; i < 3; i++)
        {
            if (this->timers[i].bullet != NULL)
            {
                (this->timers[i].bullet)->bulletState = 0;
                this->timers[i].bullet = NULL;
            }
        }
    }
    for (i32 i = 0; i < 3; i++)
    {
        if (this->timers[i].bullet != NULL)
        {
            if ((0 < this->timers[i].current) && (this->timers[i].current < 999))
            {
                this->timers[i].Decrement(1);
            }
            if ((this->fireBulletTimer.current < 0) &&
                (0x32 < this->timers[i].current))
            {
                this->timers[i].Initialize(0x32);
            }
            if (this->timers[i].current == 0)
            {
                this->timers[i].bullet = NULL;
            }
        }
    }
    bullet = this->bullets;
    for (i32 i = 0; i < 0x60; i++)
    {
        if (bullet->bulletState != 0)
        {
            if ((bullet->updateCallback == NULL) ||
                (bullet->updateCallback(this, bullet) == 0))
            {
                bullet->pos.x +=
                    g_Supervisor.effectiveFramerateMultiplier * bullet->velocity.x;
                bullet->pos.y +=
                    g_Supervisor.effectiveFramerateMultiplier * bullet->velocity.y;
                if (((bullet->bulletState2 != 4) && (bullet->bulletState2 != 5)) &&
                    (g_GameManager.IsInBounds(bullet->pos.x, bullet->pos.y,
                                              bullet->vm.sprite->widthPx,
                                              bullet->vm.sprite->heightPx) == 0))
                {
                    bullet->bulletState = 0;
                }
                if (g_AnmManager->ExecuteScript(&bullet->vm) != 0)
                {
                    bullet->bulletState = 0;
                }
                bullet->timer.Tick();
            }
            else
            {
                bullet->bulletState = 0;
            }
        }
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x0043d690
void Player::DrawBullets()
{
    PlayerBullet *bullet = this->bullets;
    for (i32 i = 0; i < 0x60; i++)
    {
        if (bullet->bulletState == 1)
        {
            if (bullet->vm.autoRotate != 0)
            {
                bullet->vm.rotation.z =
                    utils::AddNormalizeAngle(bullet->angle, 1.5707964f);
                bullet->vm.updateRotation = 1;
            }
            bullet->vm.pos.x = g_GameManager.arcadeRegionTopLeftPos.x + bullet->pos.x;
            bullet->vm.pos.y = g_GameManager.arcadeRegionTopLeftPos.y + bullet->pos.y;
            bullet->vm.pos.z = 0.4f;
            g_AnmManager->Draw(&bullet->vm);
            if (bullet->drawCallback != NULL)
            {
                bullet->drawCallback(this, bullet);
            }
        }
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x0043d790
void Player::DrawBulletExplosions()
{
    PlayerBullet *bullet = this->bullets;
    for (i32 i = 0; i < 0x60; i++)
    {
        if (bullet->bulletState == 2)
        {
            if (bullet->vm.autoRotate != 0)
            {
                bullet->vm.rotation.z =
                    utils::AddNormalizeAngle(bullet->angle, 1.5707964f);
                bullet->vm.updateRotation = 1;
            }
            bullet->vm.pos.x = g_GameManager.arcadeRegionTopLeftPos.x + bullet->pos.x;
            bullet->vm.pos.y = g_GameManager.arcadeRegionTopLeftPos.y + bullet->pos.y;
            bullet->vm.pos.z = 0.4f;
            g_AnmManager->Draw(&bullet->vm);
        }
        bullet = bullet + 1;
    }
}

// FUNCTION: TH07 0x0043d880
void Player::UpdateFireBulletTimer()
{
    if (-1 < this->fireBulletTimer.current)
    {
        if ((this->fireBulletTimer.current != this->fireBulletTimer.previous) &&
            (((g_Player.bombInfo.isInUse == 0 ||
               (g_GameManager.character != CHAR_MARISA)) ||
              (g_GameManager.shotType != 1))))
        {
            SpawnBullets(this, this->fireBulletTimer.current);
        }
        this->fireBulletTimer.Tick();
        if (((0x1d < this->fireBulletTimer.current) ||
             (this->playerState == PLAYER_STATE_DEAD)) ||
            (this->playerState == PLAYER_STATE_SPAWNING))
        {

            this->fireBulletTimer.Initialize(-1);
        }
    }
}

// FUNCTION: TH07 0x0043d990
void Player::StartFireBulletTimer()
{
    if (this->fireBulletTimer.current < 0)
    {
        this->fireBulletTimer.Initialize(0);
    }
}

// FUNCTION: TH07 0x0043d9e0
i32 Player::CheckCollisionWithEnemy(D3DXVECTOR3 *param_1, D3DXVECTOR3 *param_2,
                                    i32 *param_3)
{
    D3DXVECTOR3 *pDVar1;
    D3DXVECTOR3 *pDVar2;
    f32 fVar3;
    i16 sVar4;
    f32 fVar5;
    f32 fVar6;
    f32 fVar7;
    f32 fVar8;
    i32 local_d4;
    i16 local_80;
    i32 local_34;
    i32 i;
    PlayerBullet *bullet;

    local_34 = 0;
    if (this->invulnerabilityTimer.current ==
        this->invulnerabilityTimer.previous)
    {
        local_34 = 0;
    }
    else
    {
        fVar5 = param_1->x - param_2->x * 0.5f;
        fVar6 = param_1->y - param_2->y * 0.5f;
        fVar7 = param_2->x * 0.5f + param_1->x;
        fVar8 = param_2->y * 0.5f + param_1->y;
        bullet = this->bullets;
        if (param_3 != NULL)
        {
            *param_3 = 0;
        }
        for (i = 0; i < 0x60; i++)
        {
            if ((bullet->bulletState != 0) &&
                ((bullet->bulletState == 1 || (bullet->bulletState2 == 3))))
            {
                if ((bullet->pos.y - bullet->hitboxSize.y * 0.5f <= fVar8) &&
                    (((bullet->pos.x - bullet->hitboxSize.x * 0.5f <= fVar7 &&
                       (fVar6 <= bullet->hitboxSize.y * 0.5f + bullet->pos.y)) &&
                      (fVar5 <= bullet->hitboxSize.x * 0.5f + bullet->pos.x))))
                {
                    if ((bullet->bulletState2 == 4) || (bullet->bulletState2 == 5))
                    {
                        if (bullet->timer.current % 2 != 0)
                            goto LAB_0043da96;
                    }
                    if ((bullet->hitCallback == NULL) ||
                        (bullet->hitCallback(this, bullet, param_1) == 0))
                    {
                        if (this->bombInfo.isInUse == 0)
                        {
                            local_d4 = bullet->damage;
                        }
                        else if (bullet->damage / 3 == 0)
                        {
                            local_d4 = 1;
                        }
                        else
                        {
                            local_d4 = (i32)bullet->damage / 3;
                        }
                        local_34 += local_d4;
                        if ((bullet->bulletState2 != 4) && (bullet->bulletState2 != 5))
                        {
                            if (bullet->bulletState == 1)
                            {
                                sVar4 = bullet->vm.anmFileIdx;
                                local_80 = sVar4 + 0x20;
                                bullet->vm.anmFileIdx = local_80;
                                g_AnmManager->SetAndExecuteScript(
                                    &bullet->vm, g_AnmManager->scripts[sVar4 + 0x20]);
                                g_EffectManager.SpawnParticles(5, &bullet->pos, 1, 0xffffffff);
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
                }
            }
        LAB_0043da96:
            bullet = bullet + 1;
        }
        for (i = 0; i < 0x70; i++)
        {
            fVar3 = this->bombProjectiles[i].size.x;
            if (fVar3 < 0.0f == (fVar3 == 0.0f))
            {
                pDVar1 = &this->bombProjectiles[i].size;
                pDVar2 = &this->bombProjectiles[i].size;
                if ((((this->bombProjectiles[i].pos.x - pDVar1->x * 0.5f <= fVar7) &&
                      (fVar5 <= pDVar2->x * 0.5f + this->bombProjectiles[i].pos.x)) &&
                     (this->bombProjectiles[i].pos.y - pDVar1->y * 0.5f <= fVar8)) &&
                    (fVar6 <= pDVar2->y * 0.5f + this->bombProjectiles[i].pos.y))
                {
                    local_34 += this->bombProjectiles[i].lifetime;
                    this->bombProjectiles[i].payload =
                        (this->bombProjectiles[i].payload +
                         this->bombProjectiles[i].lifetime);
                    this->bombParticleTime = this->bombParticleTime + 1;
                    if ((this->bombParticleTime & 0x80000003) == 0)
                    {
                        if (i < 0x60)
                        {
                            g_EffectManager.SpawnParticles(3, param_1, 1, 0xffffffff);
                        }
                        else
                        {
                            g_EffectManager.SpawnParticles(5, param_1, 1, 0xffffffff);
                        }
                    }
                    if ((this->bombInfo.isInUse != 0) && (param_3 != NULL))
                    {
                        *param_3 = 1;
                    }
                }
            }
        }
    }
    return local_34;
}

#pragma var_order(bombTopLeft, bombY, bombX, i, bulletBottomRight, bulletTopLeft, bombProjectile, bombBottomRight)
// FUNCTION: TH07 0x0043e0a0
i32 Player::CheckBombGraze(D3DXVECTOR3 *center, D3DXVECTOR3 *size)
{
    BombProjectile *bombProjectile;
    i32 i;
    D3DXVECTOR3 bulletBottomRight;
    D3DXVECTOR3 bulletTopLeft;
    D3DXVECTOR3 bombBottomRight;
    D3DXVECTOR3 bombTopLeft;
    f32 bombY;
    f32 bombX;

    bombProjectile = this->bombHitboxes;
    bulletTopLeft.x = center->x - size->x / 2.0f;
    bulletTopLeft.y = center->y - size->y / 2.0f;
    bulletBottomRight.x = center->x + size->x / 2.0f;
    bulletBottomRight.y = center->y + size->y / 2.0f;
    for (i = 0; i < 0x60; i++, bombProjectile++)
    {
        if (bombProjectile->pos.z != 0.0f)
        {
            bombTopLeft.x = bombProjectile->pos.x - bombProjectile->pos.z / 2.0f;
            bombTopLeft.y = bombProjectile->pos.y - bombProjectile->size.x / 2.0f;
            bombBottomRight.x = bombProjectile->pos.z / 2.0f + bombProjectile->pos.x;
            bombBottomRight.y = bombProjectile->size.x / 2.0f + bombProjectile->pos.y;
            if (!(bombTopLeft.x > bulletBottomRight.x ||
                  bombBottomRight.x < bulletTopLeft.x ||
                  bombTopLeft.y > bulletBottomRight.y ||
                  bombBottomRight.y < bulletTopLeft.y))
            {
                this->itemType = bombProjectile->payload;
                return 2;
            }
        }
        else if (bombProjectile->size.y != 0.0) // double used here for some reason
        {
            bombX = center->x - bombProjectile->pos.x;
            bombY = center->y - bombProjectile->pos.y;
            if (bombX * bombX + bombY * bombY <
                bombProjectile->size.y * bombProjectile->size.y)
            {
                this->itemType = bombProjectile->payload;
                return 2;
            }
        }
    }
    return 0;
}

#pragma var_order(killboxBottomRight, killboxTopLeft)
// FUNCTION: TH07 0x0043e260
i32 Player::CalcKillboxCollision(D3DXVECTOR3 *center, D3DXVECTOR3 *size)
{
    D3DXVECTOR3 killboxBottomRight;
    D3DXVECTOR3 killboxTopLeft;

    this->itemType = ITEM_POINT_BULLET;
    if (CheckBombGraze(center, size) != 0)
        return 2;

    killboxTopLeft.x = center->x - size->x / 2.0f;
    killboxTopLeft.y = center->y - size->y / 2.0f;
    killboxBottomRight.x = center->x + size->x / 2.0f;
    killboxBottomRight.y = center->y + size->y / 2.0f;
    if (this->hitboxTopLeft.x > killboxBottomRight.x ||
        this->hitboxTopLeft.y > killboxBottomRight.y ||
        this->hitboxBottomRight.x < killboxTopLeft.x ||
        this->hitboxBottomRight.y < killboxTopLeft.y)
        return 0;

    g_ReplayManager->replayEventFlags = g_ReplayManager->replayEventFlags | 2;
    if (this->playerState == PLAYER_STATE_BORDER)
    {
        g_Player.BreakBorder(0);
        return 1;
    }
    if (this->playerState != PLAYER_STATE_ALIVE)
        return 1;

    g_GameManager.RerollRng();
    Die();
    return 1;
}

#pragma var_order(bulletBottomRight, bulletTopLeft)
// FUNCTION: TH07 0x0043e3b0
i32 Player::CheckGraze(D3DXVECTOR3 *center, D3DXVECTOR3 *size)
{
    D3DXVECTOR3 bulletBottomRight;
    D3DXVECTOR3 bulletTopLeft;

    this->itemType = ITEM_POINT_BULLET;

    if (CheckBombGraze(center, size) != 0)
        return 2;

    bulletTopLeft.x = center->x - size->x / 2.0f - 20.0f;
    bulletTopLeft.y = center->y - size->y / 2.0f - 20.0f;
    bulletBottomRight.x = center->x + size->x / 2.0f + 20.0f;
    bulletBottomRight.y = center->y + size->y / 2.0f + 20.0f;

    if (this->playerState == PLAYER_STATE_DEAD ||
        this->playerState == PLAYER_STATE_SPAWNING)
        return 0;

    if (this->grazeTopLeft.x > bulletBottomRight.x || this->grazeBottomRight.x < bulletTopLeft.x ||
        this->grazeTopLeft.y > bulletBottomRight.y || this->grazeBottomRight.y < bulletTopLeft.y)
        return 0;

    ScoreGraze(center);
    return 1;
}

#pragma var_order(itemBottomRight, itemTopLeft)
// FUNCTION: TH07 0x0043e4e0
i32 Player::CalcItemBoxCollision(D3DXVECTOR3 *center, D3DXVECTOR3 *size)
{
    D3DXVECTOR3 itemBottomRight;
    D3DXVECTOR3 itemTopLeft;

    if (this->playerState != PLAYER_STATE_ALIVE &&
        this->playerState != PLAYER_STATE_INVULNERABLE &&
        this->playerState != PLAYER_STATE_BORDER)
        return 0;

    memcpy(&itemTopLeft, &(*center - *size / 2.0f), sizeof(D3DXVECTOR3));
    memcpy(&itemBottomRight, &(*center + *size / 2.0f), sizeof(D3DXVECTOR3));

    if (this->grabItemTopLeft.x > itemBottomRight.x ||
        this->grabItemBottomRight.x < itemTopLeft.x ||
        this->grabItemTopLeft.y > itemBottomRight.y ||
        this->grabItemBottomRight.y < itemTopLeft.y)
        return 0;

    return 1;
}

#pragma var_order(playerRelativeTopLeft, laserBottomRight, laserTopLeft, playerRelativeBottomRight)
// FUNCTION: TH07 0x0043e6b0
i32 Player::CalcLaserHitbox(D3DXVECTOR3 *param_1, D3DXVECTOR3 *param_2,
                            D3DXVECTOR3 *param_3, f32 param_4, i32 canGraze)
{
    D3DXVECTOR3 playerRelativeTopLeft;
    D3DXVECTOR3 playerRelativeBottomRight;
    D3DXVECTOR3 laserTopLeft;
    D3DXVECTOR3 laserBottomRight;

    laserTopLeft = this->positionCenter - *param_3;
    utils::Rotate(&laserBottomRight, &laserTopLeft, param_4);
    laserBottomRight.z = 0;
    laserTopLeft = laserBottomRight + *param_3;
    playerRelativeTopLeft = laserTopLeft - this->hitboxSize;
    playerRelativeBottomRight = laserTopLeft + this->hitboxSize;

    laserTopLeft = *param_1 - *param_2 / 2.0f;
    laserBottomRight = *param_1 + *param_2 / 2.0f;
    if (!(playerRelativeTopLeft.x > laserBottomRight.x ||
          playerRelativeBottomRight.x < laserTopLeft.x ||
          playerRelativeTopLeft.y > laserBottomRight.y ||
          playerRelativeBottomRight.y < laserTopLeft.y))
        goto LASER_COLLISION;

    if (canGraze == 0)
        return 0;

    laserTopLeft.x -= 48.0f;
    laserTopLeft.y -= 48.0f;
    laserBottomRight.x += 48.0f;
    laserBottomRight.y += 48.0f;
    if (playerRelativeTopLeft.x > laserBottomRight.x ||
        playerRelativeBottomRight.x < laserTopLeft.x ||
        playerRelativeTopLeft.y > laserBottomRight.y ||
        playerRelativeBottomRight.y < laserTopLeft.y)
        return 0;

    if ((this->playerState == PLAYER_STATE_DEAD) ||
        (this->playerState == PLAYER_STATE_SPAWNING))
        return 0;

    ScoreGraze(&this->positionCenter);
    return 2;

LASER_COLLISION:
    g_ReplayManager->replayEventFlags = g_ReplayManager->replayEventFlags | 2;
    if (this->playerState == PLAYER_STATE_BORDER)
    {
        // this is already a member function of Player though
        g_Player.BreakBorder(0);
        return 1;
    }
    if (this->playerState != PLAYER_STATE_ALIVE)
        return 0;

    g_GameManager.RerollRng();
    Die();
    return 1;
}

// FUNCTION: TH07 0x0043eb90
void Player::ScoreGraze(D3DXVECTOR3 *param_1)
{
    D3DXVECTOR3 local_10;

    if (g_Player.bombInfo.isInUse == 0)
    {
        if (g_GameManager.globals->grazeInStage < 9999)
        {
            g_GameManager.globals->grazeInStage =
                g_GameManager.globals->grazeInStage + 1;
        }
        if (g_GameManager.globals->grazeInTotal < 999999)
        {
            g_GameManager.globals->grazeInTotal =
                g_GameManager.globals->grazeInTotal + 1;
        }
    }
    local_10.z = (this->positionCenter.z + param_1->z) * 0.5f;
    local_10.y = (this->positionCenter.y + param_1->y) * 0.5f;
    local_10.x = (this->positionCenter.x + param_1->x) * 0.5f;
    if (this->hasBorder == BORDER_ACTIVE)
    {
        if (this->isFocus == 0)
        {
            g_EffectManager.SpawnParticles(8, &local_10, 3, 0xffff8080);
        }
        else
        {
            g_EffectManager.SpawnParticles(8, &local_10, 1, 0xffffffff);
        }
    }
    else
    {
        g_EffectManager.SpawnParticles(8, &local_10, 1, 0xffffffff);
    }
    g_GameManager.IncreaseSubrank(6);
    g_Gui.flags = (g_Gui.flags & 0xffffff3f) | 0x80;
    g_SoundPlayer.PlaySoundByIdx(SOUND_GRAZE, 0);
    g_EnemyManager.spellcardInfo.grazeBonusScore =
        g_EnemyManager.spellcardInfo.grazeBonusScore + 2500 +
        ((g_GameManager.cherry - g_GameManager.globals->cherryStart) / 1500) * 20;
    g_GameManager.globals->score = g_GameManager.globals->score + 200;
    if (this->hasBorder == BORDER_ACTIVE)
    {
        if (this->isFocus == 0)
        {
            g_GameManager.IncreaseCherryMax(0x50);
            g_GameManager.IncreaseCherry(0x50);
        }
        else
        {
            g_GameManager.IncreaseCherryMax(0x1e);
            g_GameManager.IncreaseCherry(0x1e);
        }
    }
}

// FUNCTION: TH07 0x0043edc0
void Player::Die()
{
    g_GameManager.RegenerateGameIntegrityCsum();
    g_EffectManager.SpawnEffect(0xc, &this->positionCenter, 3, 1, 0xff4040ff);
    g_EffectManager.SpawnParticles(6, &this->positionCenter, 0x10, 0xffffffff);
    this->playerState = PLAYER_STATE_DEAD;
    this->invulnerabilityTimer.Initialize2(0);
    g_SoundPlayer.PlaySoundByIdx(SOUND_PICHUN, 0);
}

// FUNCTION: TH07 0x0043ee50
void Player::HandlePlayerInputs()
{
    f32 fVar1;
    f32 fVar2;
    f32 fVar3;
    f32 local_218;
    f32 local_18;
    f32 local_14;
    f32 horizontalSpeed;
    f32 verticalSpeed;

    horizontalSpeed = 0.0f;
    verticalSpeed = 0.0f;
    PlayerDirection direction = this->playerDirection;
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
    else
    {
        if (IS_PRESSED_GAME(TH_BUTTON_DOWN))
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
    }
    if (IS_PRESSED_GAME(TH_BUTTON_FOCUS))
    {
        this->isFocus = 1;
        switch (this->playerDirection)
        {
        case MOVEMENT_UP:
            verticalSpeed = -this->shooterData->speedFocus;
            break;
        case MOVEMENT_DOWN:
            verticalSpeed = this->shooterData->speedFocus;
            break;
        case MOVEMENT_LEFT:
            horizontalSpeed = -this->shooterData->speedFocus;
            break;
        case MOVEMENT_RIGHT:
            horizontalSpeed = this->shooterData->speedFocus;
            break;
        case MOVEMENT_UP_LEFT:
            horizontalSpeed = -this->shooterData->speedDiagonalFocus;
            verticalSpeed = horizontalSpeed;
            break;
        case MOVEMENT_UP_RIGHT:
            horizontalSpeed = this->shooterData->speedDiagonalFocus;
            verticalSpeed = -horizontalSpeed;
            break;
        case MOVEMENT_DOWN_LEFT:
            verticalSpeed = this->shooterData->speedDiagonalFocus;
            horizontalSpeed = -verticalSpeed;
            break;
        case MOVEMENT_DOWN_RIGHT:
            horizontalSpeed = this->shooterData->speedDiagonalFocus;
            verticalSpeed = horizontalSpeed;
        }
    }
    else
    {
        this->isFocus = 0;
        switch (this->playerDirection)
        {
        case MOVEMENT_UP:
            verticalSpeed = -this->shooterData->speed;
            break;
        case MOVEMENT_DOWN:
            verticalSpeed = this->shooterData->speed;
            break;
        case MOVEMENT_LEFT:
            horizontalSpeed = -this->shooterData->speed;
            break;
        case MOVEMENT_RIGHT:
            horizontalSpeed = this->shooterData->speed;
            break;
        case MOVEMENT_UP_LEFT:
            horizontalSpeed = -this->shooterData->speedDiagonal;
            verticalSpeed = horizontalSpeed;
            break;
        case MOVEMENT_UP_RIGHT:
            horizontalSpeed = this->shooterData->speedDiagonal;
            verticalSpeed = -horizontalSpeed;
            break;
        case MOVEMENT_DOWN_LEFT:
            verticalSpeed = this->shooterData->speedDiagonal;
            horizontalSpeed = -verticalSpeed;
            break;
        case MOVEMENT_DOWN_RIGHT:
            horizontalSpeed = this->shooterData->speedDiagonal;
            verticalSpeed = horizontalSpeed;
        }
    }
    if (horizontalSpeed < 0.0f && this->previousHorizontalSpeed >= 0.0f)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(&this->playerSprite, 0x401);
    }
    else if (horizontalSpeed == 0.0f && this->previousHorizontalSpeed < 0.0f)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(&this->playerSprite, 0x402);
    }

    if (horizontalSpeed > 0.0f && this->previousHorizontalSpeed <= 0.0f)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(&this->playerSprite, 0x403);
    }
    else if (horizontalSpeed == 0.0f && this->previousHorizontalSpeed > 0.0f)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(&this->playerSprite, 0x404);
    }
    this->previousHorizontalSpeed = horizontalSpeed;
    this->previousVerticalSpeed = verticalSpeed;
    this->velocity.x = horizontalSpeed *
                       this->horizontalMovementSpeedMultiplierDuringBomb *
                       g_Supervisor.effectiveFramerateMultiplier;
    this->velocity.y = verticalSpeed *
                       this->verticalMovementSpeedMultiplierDuringBomb *
                       g_Supervisor.effectiveFramerateMultiplier;
    this->positionCenter.x += this->velocity.x;
    this->positionCenter.y += this->velocity.y;
    if (this->positionCenter.x < g_GameManager.playerMovementAreaTopLeftPos.x)
    {
        this->positionCenter.x = g_GameManager.playerMovementAreaTopLeftPos.x;
    }
    else if (this->positionCenter.x > g_GameManager.playerMovementAreaTopLeftPos.x + g_GameManager.playerMovementAreaSize.x)
    {
        this->positionCenter.x = g_GameManager.playerMovementAreaTopLeftPos.x + g_GameManager.playerMovementAreaSize.x;
    }

    if (this->positionCenter.y < g_GameManager.playerMovementAreaTopLeftPos.y)
    {
        this->positionCenter.y = g_GameManager.playerMovementAreaTopLeftPos.y;
    }
    else if (this->positionCenter.y > g_GameManager.playerMovementAreaTopLeftPos.y + g_GameManager.playerMovementAreaSize.y)
    {
        this->positionCenter.y = g_GameManager.playerMovementAreaTopLeftPos.y + g_GameManager.playerMovementAreaSize.y;
    }
    this->hitboxTopLeft = this->positionCenter - this->hitboxSize;
    this->hitboxBottomRight = this->positionCenter + this->hitboxSize;
    this->grazeTopLeft = this->positionCenter - this->grazeSize;
    this->grazeBottomRight = this->positionCenter + this->grazeSize;
    this->grabItemTopLeft = this->positionCenter - this->grabItemSize;
    this->grabItemBottomRight = this->positionCenter + this->grabItemSize;
    this->orbsPosition[0] = this->positionCenter;
    this->orbsPosition[1] = this->positionCenter;
    local_14 = 0.0f;
    local_18 = 0.0f;
    if (g_GameManager.character != CHAR_SAKUYA || g_GameManager.shotType != 1)
    {
        switch (this->orbState)
        {
        case ORB_HIDDEN:
            this->focusMovementTimer.InitializeForPopup();
            break;
        case ORB_UNFOCUSED:
            local_18 = 24.0f;
            this->focusMovementTimer.InitializeForPopup();
            if (this->isFocus != 0)
            {
                this->orbState = ORB_FOCUSING;
                this->focusEffect = g_EffectManager.SpawnEffect(
                    0x18, &this->positionCenter, 2, 1, 0xffffffff);
                break;
            }
            break;
        case ORB_FOCUSING:
            this->focusMovementTimer.Tick();
            fVar1 = ((f32)this->focusMovementTimer.current +
                     this->focusMovementTimer.subFrame) /
                    8.0f;
            local_14 = (1.0f - fVar1) * 32.0f - 32.0f;
            local_18 = fVar1 * fVar1 * -16.0f + 24.0f;
            if (this->focusMovementTimer.current >= 8)
            {
                this->orbState = ORB_FOCUSED;
            }
            if (this->isFocus != 0)
                break;
            this->orbState = ORB_UNFOCUSING;
            this->focusMovementTimer.Initialize2(8 -
                                                 this->focusMovementTimer.current);
            if (this->focusEffect != NULL)
            {
                this->focusEffect->vm.pendingInterrupt = 1;
            }
        switchD_0043f936_caseD_4:
            this->focusMovementTimer.Tick();
            fVar1 = ((f32)this->focusMovementTimer.current +
                     this->focusMovementTimer.subFrame) /
                    8.0f;
            local_14 = fVar1 * 32.0f - 32.0f;
            local_18 = (1.0f - fVar1 * fVar1) * -16.0f + 24.0f;
            if (this->focusMovementTimer.current >= 8)
            {
                this->orbState = ORB_UNFOCUSED;
            }
            if (this->isFocus == 0)
                break;
            this->orbState = ORB_FOCUSING;
            this->focusMovementTimer.Initialize2(8 -
                                                 this->focusMovementTimer.current);
            this->focusEffect = g_EffectManager.SpawnEffect(
                0x18, &this->positionCenter, 2, 1, 0xffffffff);
            break;
        case ORB_FOCUSED:
            local_18 = 8.0f;
            local_14 = -32.0f;
            this->focusMovementTimer.InitializeForPopup();
            if (this->isFocus == 0)
            {
                this->orbState = ORB_UNFOCUSING;
                if (this->focusEffect == NULL)
                    goto switchD_0043f936_caseD_4;
                this->focusEffect->vm.pendingInterrupt = 1;
                goto switchD_0043f936_caseD_4;
            }
            break;
        case ORB_UNFOCUSING:
            goto switchD_0043f936_caseD_4;
        }
        this->orbsPosition[0].x -= local_18;
        this->orbsPosition[1].x += local_18;
        this->orbsPosition[0].y += local_14;
        this->orbsPosition[1].y += local_14;
    }
    else
    {
        switch (this->orbState)
        {
        case ORB_HIDDEN:
            this->focusMovementTimer.InitializeForPopup();
            break;
        case ORB_UNFOCUSED:
            local_18 = cosf(this->orbAngle + 1.5707964f) * 24.0f;
            local_14 = sinf(this->orbAngle + 1.5707964f) * 24.0f;
            this->focusMovementTimer.InitializeForPopup();
            if (this->isFocus != 0)
            {
                this->orbState = ORB_FOCUSING;
                this->focusEffect = g_EffectManager.SpawnEffect(
                    0x18, &this->positionCenter, 2, 1, 0xffffffff);
                goto CASE_ORB_FOCUSING;
            }
            this->orbsPosition[0].x -= local_18;
            this->orbsPosition[1].x += local_18;
            this->orbsPosition[0].y -= local_14;
            this->orbsPosition[1].y += local_14;
            break;
        CASE_ORB_FOCUSING:
        case ORB_FOCUSING:
            if (this->isFocus == 0)
            {
                this->orbState = ORB_UNFOCUSING;
                this->focusMovementTimer.Initialize2(8 -
                                                     this->focusMovementTimer.current);
                if (this->focusEffect != NULL)
                {
                    this->focusEffect->vm.pendingInterrupt = 1;
                }
                goto CASE_ORB_UNFOCUSING;
            }
            this->focusMovementTimer.Tick();
            fVar1 = ((f32)this->focusMovementTimer.current +
                     this->focusMovementTimer.subFrame) /
                    8.0f;
            fVar2 = cosf(this->orbAngle + 1.5707964f) * 24.0f;
            fVar3 = sinf(this->orbAngle + 1.5707964f) * 24.0f;
            this->orbsPosition[1].x +=
                (cosf(this->orbAngle + 0.22439948f) * 24.0f - fVar2) * fVar1 + fVar2;
            this->orbsPosition[1].y +=
                (sinf(this->orbAngle + 0.22439948f) * 24.0f - fVar3) * fVar1 + fVar3;
            if (this->focusMovementTimer.current >= 8)
            {
                this->orbState = ORB_FOCUSED;
            }
            this->orbsPosition[0].x +=
                (cosf(this->orbAngle - 0.22439948f) * 24.0f + fVar2) * fVar1 - fVar2;
            this->orbsPosition[0].y +=
                (sinf(this->orbAngle - 0.22439948f) * 24.0f + fVar3) * fVar1 - fVar3;
            break;
        case ORB_FOCUSED:
            this->focusMovementTimer.InitializeForPopup();
            if (this->isFocus == 0)
            {
                this->orbState = ORB_UNFOCUSING;
                if (this->focusEffect == NULL)
                    goto CASE_ORB_UNFOCUSING;
                this->focusEffect->vm.pendingInterrupt = 1;
                goto CASE_ORB_UNFOCUSING;
            }
            this->orbsPosition[1].x += cosf(this->orbAngle + 0.22439948f) * 24.0f;
            this->orbsPosition[1].y += sinf(this->orbAngle + 0.22439948f) * 24.0f;
            this->orbsPosition[0].x += cosf(this->orbAngle - 0.22439948f) * 24.0f;
            this->orbsPosition[0].y += sinf(this->orbAngle - 0.22439948f) * 24.0f;
            break;
        CASE_ORB_UNFOCUSING:
        case ORB_UNFOCUSING:
            if (this->isFocus == 0)
            {
                this->focusMovementTimer.Tick();
                fVar1 = 1.0f - ((f32)this->focusMovementTimer.current +
                                this->focusMovementTimer.subFrame) /
                                   8.0f;
                fVar2 = cosf(this->orbAngle + 1.5707964f) * 24.0f;
                fVar3 = sinf(this->orbAngle + 1.5707964f) * 24.0f;
                this->orbsPosition[1].x +=
                    (cosf(this->orbAngle + 0.22439948f) * 24.0f - fVar2) * fVar1 +
                    fVar2;
                this->orbsPosition[1].y +=
                    (sinf(this->orbAngle + 0.22439948f) * 24.0f - fVar3) * fVar1 +
                    fVar3;
                if (this->focusMovementTimer.current >= 8)
                {
                    this->orbState = ORB_UNFOCUSED;
                }
                this->orbsPosition[0].x +=
                    ((cosf(this->orbAngle - 0.22439948f) * 24.0f + fVar2) * fVar1 -
                     fVar2);
                this->orbsPosition[0].y +=
                    ((sinf(this->orbAngle - 0.22439948f) * 24.0f + fVar3) * fVar1 -
                     fVar3);
                goto switchD_0043fe16_default;
            }
            this->orbState = ORB_FOCUSING;
            this->focusMovementTimer.Initialize(8 -
                                                this->focusMovementTimer.current);
            this->focusEffect = g_EffectManager.SpawnEffect(
                0x18, &this->positionCenter, 2, 1, 0xffffffff);
            break;
        }
    }
switchD_0043fe16_default:
    if (IS_PRESSED_GAME(TH_BUTTON_SHOOT) && g_Gui.HasCurrentMsgIdx() == 0)
    {
        if (g_GameManager.CheckGameIntegrity() == 0)
        {
            StartFireBulletTimer();
        }
        if (!IS_PRESSED_GAME(TH_BUTTON_FOCUS))
        {
            if (this->velocity.x != 0.0f)
            {
                f32 angleDelta = ((-(this->velocity.x / 4.0f) * ZUN_PI) / 5.0f) / 10.0f;
                this->orbAngle -= angleDelta;
                if (this->orbAngle < -2.1991148f)
                {
                    this->orbAngle = -2.1991148f;
                }
                else if (this->orbAngle > -0.9424778f)
                {
                    this->orbAngle = -0.9424778f;
                }
            }
            else
            {
                if (fabsf(this->orbAngle - -1.5707964f) <= 0.03141593f)
                {
                    this->orbAngle = -1.5707964f;
                }
                else
                {
                    if (-1.5707964f <= this->orbAngle)
                    {
                        local_218 =
                            g_Supervisor.effectiveFramerateMultiplier * -0.06283186f;
                    }
                    else
                    {
                        local_218 = g_Supervisor.effectiveFramerateMultiplier * 0.06283186f;
                    }
                    this->orbAngle = local_218 + this->orbAngle;
                }
            }
        }
    }
}

// FUNCTION: TH07 0x00440940
void Player::UpdateBombProjectiles()
{
    for (i32 i = 0; i < 0x70; i++)
    {
        this->bombProjectiles[i].size.x = 0.0f;
    }
    BombProjectile *local_c = this->bombHitboxes;
    for (i32 i = 0; i < 0x60; i++)
    {
        if (local_c->lifetime < 1)
        {
            local_c->size.y = 0.0f;
            local_c->pos.z = 0.0f;
        }
        else
        {
            local_c->lifetime = local_c->lifetime - 1;
            local_c->size.y = local_c->size.y + local_c->size.z;
        }
        local_c = local_c + 1;
    }
}

// FUNCTION: TH07 0x004409f0
void Player::UpdateBorderAndBombState()
{
    if (((this->hasBorder == BORDER_NONE) || (this->bombInfo.isInUse != 0)) ||
        !IS_PRESSED_GAME(TH_BUTTON_BOMB))
    {
        if (this->hasBorder == BORDER_READY)
        {
            ActivateBorder();
        }
        if (this->borderInvulnerabilityTime != 0)
        {
            this->borderInvulnerabilityTime = this->borderInvulnerabilityTime - 1;
        }
        if (this->bombInfo.isInUse == 0)
        {
            if ((((g_GameManager.CheckGameIntegrity() == 0) &&
                  (g_Gui.HasCurrentMsgIdx() == 0)) &&
                 ((this->respawnTimer != 0 &&
                   ((0 < (i32)g_GameManager.globals->bombsRemaining &&
                     (this->borderInvulnerabilityTime == 0)))))) &&
                IS_PRESSED_GAME(TH_BUTTON_BOMB))
            {
                g_ReplayManager->replayEventFlags |= 1;
                g_GameManager.AddBombsUsed(1);
                g_GameManager.AddBombsRemaining(-1);
                g_Gui.flags = (g_Gui.flags & 0xfffffff3) | 8;
                this->bombInfo.isFocus = (i32)this->isFocus;
                this->bombInfo.isInUse = 1;
                this->isBombing = 1;
                this->bombInfo.bombTimer.Initialize(0);
                this->bombInfo.bombDuration = 999;
                if (this->bombInfo.isFocus == 0)
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
                g_EnemyManager.spellcardInfo.usedBomb =
                    g_EnemyManager.spellcardInfo.isActive;
                this->respawnTimer = this->respawnTimer + 6;
                if (g_Player.shooterData->initialRespawnTimer < this->respawnTimer)
                {
                    this->respawnTimer = g_Player.shooterData->initialRespawnTimer;
                }
            }
            else
            {
                this->isBombing = 0;
            }
        }
        else
        {
            if (this->bombInfo.bombTimer.current !=
                this->bombInfo.bombTimer.previous)
            {
                if (g_GameManager.cherry - g_GameManager.globals->cherryStart <
                    this->bombInfo.bombCherryDrain)
                {
                    g_GameManager.cherry = g_GameManager.globals->cherryStart;
                }
                else
                {
                    g_GameManager.cherry -= this->bombInfo.bombCherryDrain;
                }
                g_Gui.flags = (g_Gui.flags & 0xfffffcff) | 0x200;
            }
            if (this->bombInfo.isFocus == 0)
            {
                this->bombInfo.bombCalc(this);
            }
            else
            {
                this->bombInfo.bombFocusCalc(this);
            }
        }
    }
    else
    {
        BreakBorder(1);
        this->isBombing = 0;
        g_ItemManager.RemoveAllItems();
    }
}

// FUNCTION: TH07 0x00440cf0
i32 Player::UpdateDeath()
{
    f32 fVar1;
    u32 local_8;

    if (this->respawnTimer == 0)
    {
        fVar1 = ((f32)this->invulnerabilityTimer.current +
                 this->invulnerabilityTimer.subFrame) /
                30.0f;
        this->playerSprite.scale.y = fVar1 * 3.0f + 1.0f;
        this->playerSprite.scale.x = 1.0f - fVar1 * 1.0f;
        this->playerSprite.color.color =
            (u32)(255.0f - (((f32)this->invulnerabilityTimer.current +
                             this->invulnerabilityTimer.subFrame) *
                            255.0f) /
                               30.0f)
                << 0x18 |
            0xffffff;
        this->playerSprite.blendMode = 1;
        this->previousHorizontalSpeed = 0.0f;
        this->previousVerticalSpeed = 0.0f;
        if (0x1d < this->invulnerabilityTimer.current)
        {
            this->playerState = PLAYER_STATE_SPAWNING;
            this->positionCenter.x = g_GameManager.arcadeRegionSize.x / 2.0f;
            this->positionCenter.y = g_GameManager.arcadeRegionSize.y - 64.0f;
            this->positionCenter.z = 0.2f;
            this->invulnerabilityTimer.Initialize(0);
            this->playerSprite.scale.x = 3.0f;
            this->playerSprite.scale.y = 3.0f;
            this->playerSprite.anmFileIdx = 0x400;
            g_AnmManager->SetAndExecuteScript(&this->playerSprite,
                                              g_AnmManager->scripts[0x400]);
            if (0 < (i32)g_GameManager.globals->livesRemaining)
            {
                g_GameManager.AddLivesRemaining(-1);
                g_Gui.flags = (g_Gui.flags & 0xfffffffc) | 2;
                g_GameManager.SetBombsRemainingAndComputeCsum(
                    g_Player.shooterData->initialBombs);
                g_Gui.flags = (g_Gui.flags & 0xfffffff3) | 8;
                return 1;
            }
            g_GameManager.isInPauseMenu = 1;
        }
    }
    else
    {
        if (this->hasBorder == BORDER_ACTIVE)
        {
            BreakBorder(0);
            return 0;
        }
        this->respawnTimer = this->respawnTimer - 1;
        if (this->respawnTimer == 0)
        {
            g_ReplayManager->replayEventFlags = g_ReplayManager->replayEventFlags | 4;
            g_GameManager.powerItemCountForScore = 0;
            g_EnemyManager.spellcardInfo.captureScore = 0;
            g_EnemyManager.spellcardInfo.isCapturing = 0;
            g_GameManager.CheckGameIntegrityOnDeath(1);
            if ((i32)g_GameManager.globals->livesRemaining < 1)
            {
                g_GameManager.globals->currentPower = 0.0f;
                g_GameManager.RegenerateGameIntegrityCsum();
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_FULL_POWER, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_FULL_POWER, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_FULL_POWER, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_FULL_POWER, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_FULL_POWER, 2);
                g_Gui.flags = (g_Gui.flags & 0xffffffcf) | 0x20;
            }
            else
            {
                if ((i32)g_GameManager.globals->currentPower < 17)
                {
                    g_GameManager.globals->currentPower = 0.0f;
                    g_GameManager.RegenerateGameIntegrityCsum();
                }
                else
                {
                    g_GameManager.AddCurrentPower(-0x10);
                }
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_BIG, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_SMALL, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_SMALL, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_SMALL, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_SMALL, 2);
                g_ItemManager.SpawnItem(&this->positionCenter, ITEM_POWER_SMALL, 2);
                g_Gui.flags = (g_Gui.flags & 0xffffffcf) | 0x20;
                local_8 =
                    (f32)(g_GameManager.cherry - g_GameManager.globals->cherryStart) *
                    g_Player.shooterData->cherryPenaltyMultiplier;
                if (g_GameManager.character == CHAR_SAKUYA)
                {
                    if (60000 < (i32)local_8)
                    {
                        local_8 = 60000;
                    }
                }
                else if (100000 < (i32)local_8)
                {
                    local_8 = 100000;
                }
                g_GameManager.cherry -= local_8 - (i32)local_8 % 10;
                g_Gui.flags = (g_Gui.flags & 0xfffffcff) | 0x200;
                g_ItemManager.ActivateAllItems();
            }
            g_GameManager.DecreaseSubrank(0x640);
        }
    }
    return 0;
}

// FUNCTION: TH07 0x004411c0
void Player::Respawn()
{
    this->bulletGracePeriod = 60;
    f32 fVar1 = 1.0f - ((f32)this->invulnerabilityTimer.current +
                        this->invulnerabilityTimer.subFrame) /
                           30.0f;
    this->playerSprite.scale.y = fVar1 * 2.0f + 1.0f;
    this->playerSprite.scale.x = 1.0f - fVar1 * 1.0f;
    this->playerSprite.blendMode = 1;
    this->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
    this->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
    this->playerSprite.color.color =
        (this->invulnerabilityTimer.current * 0xff) / 0x1e << 0x18 | 0xffffff;
    this->respawnTimer = 0;
    if (0x1d < this->invulnerabilityTimer.current)
    {
        this->playerState = PLAYER_STATE_INVULNERABLE;
        this->playerSprite.scale.x = 1.0f;
        this->playerSprite.scale.y = 1.0f;
        this->playerSprite.color.color = 0xffffffff;
        this->playerSprite.blendMode = 0;
        this->invulnerabilityTimer.Initialize(0xf0);
        this->respawnTimer = g_Player.shooterData->initialRespawnTimer;
    }
}

// FUNCTION: TH07 0x00441330
void Player::UpdateState()
{
    ZunColor local_8;

    if (this->bulletGracePeriod != 0)
    {
        this->bulletGracePeriod = this->bulletGracePeriod - 1;
        g_BulletManager.RemoveAllBullets(0);
    }
    if (this->playerState == PLAYER_STATE_INVULNERABLE)
    {
        if (this->effect != NULL)
        {
            this->effect->pos1 = this->positionCenter;
        }
        this->invulnerabilityTimer.Decrement(1);
        if (this->invulnerabilityTimer.current < 1)
        {
            if (this->effect != NULL)
            {
                this->effect->inUseFlag = 0;
                this->effect = NULL;
            }
            this->playerState = PLAYER_STATE_ALIVE;
            this->invulnerabilityTimer.Initialize(0);
            this->playerSprite.color.color = 0xffffffff;
        }
        else
        {
            if ((i32)this->invulnerabilityTimer.current % 8 < 2)
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
        if (this->borderEffect != NULL)
        {
            this->borderEffect->pos1 = this->positionCenter;
        }
        g_GameManager.cherryPlus = (this->invulnerabilityTimer.current * 50000) /
                                   this->borderTimer.current;
        if (g_GameManager.cherryPlus < 0)
        {
            g_GameManager.cherryPlus = 0;
        }
        g_GameManager.cherryPlus += g_GameManager.globals->cherryStart;
        this->invulnerabilityTimer.Decrement(1);
        if (this->invulnerabilityTimer.current < 1)
        {
            this->playerSprite.color.color = 0xffffffff;
            BreakBorderNaturally();
        }
        else
        {
            if (this->invulnerabilityTimer.current % 4 < 2)
            {
                this->playerSprite.color.color = 0xffff0000;
            }
            else
            {
                this->playerSprite.color.color = 0xffffffff;
            }
            if (g_Player.invulnerabilityTimer.current < 0x1fe)
            {
                if (g_Player.invulnerabilityTimer.current < 0x1e)
                {
                    local_8.bytes.g =
                        -0x80 -
                        (char)((g_Player.invulnerabilityTimer.current * 0x50) / 0x1e);
                    local_8.bytes.b = local_8.bytes.g;
                    local_8.bytes.r = local_8.bytes.g;
                    local_8.bytes.a = 0x80;
                }
                else
                {
                    local_8.color = 0x80303030;
                }
            }
            else
            {
                local_8.bytes.g =
                    -0x80 -
                    (char)(((0x21c - g_Player.invulnerabilityTimer.current) * 0x50) /
                           0x1e);
                local_8.bytes.b = local_8.bytes.g;
                local_8.bytes.r = local_8.bytes.g;
                local_8.bytes.a = 0x80;
            }
            g_Stage.SmoothBlendColor(local_8);
        }
    }
    else
    {
        this->invulnerabilityTimer.Tick();
    }
}

// FUNCTION: TH07 0x00441670
void Player::BreakBorderNaturally()
{
    i32 fmtArg;

    g_GameManager.IncreaseCherryMax(10000);
    g_GameManager.IncreaseCherry(10000);
    fmtArg = (g_GameManager.cherry - g_GameManager.globals->cherryStart) * 10;
    g_GameManager.globals->score = fmtArg / 10 + g_GameManager.globals->score;
    g_Gui.ShowFullPowerMode(fmtArg, 4);
    g_GameManager.cherryPlus = g_GameManager.globals->cherryStart;
    g_SoundPlayer.PlaySoundByIdx(SOUND_BORDER_BREAK, 0);
    if (this->playerState == PLAYER_STATE_SPAWNING)
    {
        this->playerSprite.scale.x = 1.0f;
        this->playerSprite.scale.y = 1.0f;
        this->playerSprite.color.color = 0xffffffff;
        this->playerSprite.blendMode = 0;
        this->invulnerabilityTimer.Initialize(0xf0);
        this->respawnTimer = g_Player.shooterData->initialRespawnTimer;
    }
    this->playerState = PLAYER_STATE_INVULNERABLE;
    this->invulnerabilityTimer.Initialize(0x28);
    this->borderInvulnerabilityTime = 0x28;
    this->hasBorder = BORDER_NONE;
    if (this->borderEffect != NULL)
    {
        this->borderEffect->inUseFlag = 0;
        this->borderEffect = NULL;
    }
}

// FUNCTION: TH07 0x00441800
BombProjectile *Player::SpawnBombProjectile(D3DXVECTOR3 *centerPosition,
                                            f32 posZ, f32 size, i32 payload)
{
    BombProjectile *local_c;
    i32 local_8;

    local_8 = 0;
    for (local_c = this->bombHitboxes;
         (local_8 < 0x5f &&
          ((local_c->pos.z != 0.0f || (local_c->size.y != 0.0f))));
         local_c = local_c + 1)
    {
        local_8 += 1;
    }
    local_c->pos.x = centerPosition->x;
    local_c->pos.y = centerPosition->y;
    local_c->pos.z = posZ;
    local_c->size.x = size;
    local_c->lifetime = 0;
    local_c->payload = payload;
    return local_c;
}

#pragma var_order(local_8, bomb)
// FUNCTION: TH07 0x004418b0
BombProjectile *Player::SpawnBombEffect(D3DXVECTOR3 *pos, f32 sizeY, f32 sizeZ,
                                        i32 lifetime, i32 payload)
{
    BombProjectile *bomb;
    i32 local_8;

    for (bomb = this->bombHitboxes, local_8 = 0;
         local_8 < 0x5f;
         local_8++, bomb++)
    {
        if (bomb->pos.z == 0.0f && bomb->size.y == 0.0f)
            break;
    }
    bomb->pos.x = pos->x;
    bomb->pos.y = pos->y;
    bomb->size.y = sizeY;
    bomb->size.z = sizeZ;
    bomb->lifetime = lifetime;
    bomb->payload = payload;
    return bomb;
}

// FUNCTION: TH07 0x00441960
void Player::ActivateBorder()
{
    Effect *pEVar3;

    if ((this->bombInfo.isInUse != 0) || (g_Gui.HasCurrentMsgIdx() != 0))
    {
        this->hasBorder = BORDER_READY;
        return;
    }
    if (this->playerState != PLAYER_STATE_SPAWNING)
    {
        if (this->playerState == PLAYER_STATE_DEAD)
        {
            if (this->respawnTimer != 0)
            {
                BreakBorder(0);
                return;
            }
            this->hasBorder = BORDER_READY;
            return;
        }
        if (this->playerState != PLAYER_STATE_INVULNERABLE)
        {
            this->invulnerabilityTimer.Initialize(0x21c);
            this->borderTimer = this->invulnerabilityTimer;
            this->hasBorder = BORDER_ACTIVE;
            this->playerState = PLAYER_STATE_BORDER;
            if (this->borderEffect != NULL)
            {
                this->borderEffect->inUseFlag = 0;
            }
            if (this->effect != NULL)
            {
                this->effect->inUseFlag = 0;
                this->effect = NULL;
            }
            pEVar3 = g_EffectManager.SpawnEffect(0x1c, &this->positionCenter, 4, 1,
                                                 0xffffffff);
            pEVar3->vm.interpStartTimes[4].Initialize(0);
            pEVar3->vm.interpEndTimes[4].Initialize(
                this->invulnerabilityTimer.current);
            pEVar3->vm.interpModes[4] = 0;
            pEVar3->vm.scaleInterpInitial.y = 1.0f;
            pEVar3->vm.scaleInterpInitial.x = 1.0f;
            pEVar3->vm.scaleInterpFinal.x = 0.25f;
            pEVar3->vm.scaleInterpFinal.y = 0.25f;
            pEVar3->vm.intVars1[0] = this->invulnerabilityTimer.current;
            pEVar3->vm.angleVel.z *= -1.0f;
            this->borderEffect = pEVar3;
            g_Gui.ShowFullPowerMode(0, 2);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BORDER_ACTIVATE, 0);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BORDER_ACTIVATE2, 0);
            g_ReplayManager->replayEventFlags = g_ReplayManager->replayEventFlags | 8;
            return;
        }
    }
    this->hasBorder = BORDER_READY;
}

#pragma var_order(effect, local_c, local_10)
// FUNCTION: TH07 0x00441bd0
void Player::BreakBorder(u32 unused)
{
    Effect *effect;
    f32 local_10;
    i32 local_c;

    if (this->borderEffect != NULL)
    {
        this->borderEffect->inUseFlag = 0;
        this->borderEffect = NULL;
    }
    effect = g_EffectManager.SpawnEffect(0x1c, &this->positionCenter, 4, 1,
                                         0xffffffff);
    effect->vm.interpStartTimes[4].Initialize2(0);
    effect->vm.interpEndTimes[4].Initialize2(0x1e);
    effect->vm.interpModes[4] = 0;
    effect->vm.scaleInterpInitial.x = 0.0625f;
    effect->vm.scaleInterpInitial.y = 0.0625f;
    effect->vm.scaleInterpFinal.x = 1.3f;
    effect->vm.scaleInterpFinal.y = 1.3f;
    effect->vm.interpStartTimes[2].Initialize2(0);
    effect->vm.interpEndTimes[2].Initialize2(0x1e);
    effect->vm.interpModes[2] = 1;
    effect->vm.colorInterpInitialColor.bytes.a = effect->vm.color.bytes.a;
    effect->vm.colorInterpFinalColor.bytes.a = 0;
    effect->vm.intVars1[0] = 0x1e;
    this->borderEffect = effect;
    g_EnemyManager.spellcardInfo.captureScore = 0;
    g_EnemyManager.spellcardInfo.isCapturing = 0;
    this->hasBorder = BORDER_NONE;
    this->playerState = PLAYER_STATE_INVULNERABLE;
    this->invulnerabilityTimer.Initialize2(0x28);
    this->borderInvulnerabilityTime = 0x28;
    g_GameManager.cherryPlus = g_GameManager.globals->cherryStart;
    SpawnBombEffect(&this->positionCenter, 32.0f, 16.0f, 0x32, 8);
    local_10 = -ZUN_PI;
    for (local_c = 0; local_c < 0x20; local_c += 1, local_10 += 0.19634955f)
    {
        effect = g_EffectManager.SpawnParticles(0x1d, &this->positionCenter, 1,
                                                0xffffffff);
        effect->direction.x = cosf(local_10);
        effect->direction.y = sinf(local_10);
    }
    g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB_MARISA_A_FOCUS, 0);
    g_SoundPlayer.PlaySoundByIdx(SOUND_BORDER_BREAK, 0);
    g_ReplayManager->replayEventFlags = g_ReplayManager->replayEventFlags | 0x10;
}

// FUNCTION: TH07 0x00441e80
void Player::UpdateUI()
{
    (this->positionOfLastEnemyHit).x = -999.0f;
    (this->positionOfLastEnemyHit).y = -999.0f;
    (this->positionOfLastEnemyHit).z = 0.0f;
    (this->sakuyaTargetPosition).x = -999.0f;
    (this->sakuyaTargetPosition).y = -999.0f;
    (this->sakuyaTargetPosition).z = 0.0f;
    this->targetingEnemy = 0;
    if (this->positionCenter.y < 400.0f)
    {
        if (g_AsciiManager.uiFadeState == 2)
        {
            g_AsciiManager.otherVms[0].pendingInterrupt = 3;
            g_AsciiManager.uiFadeState = 3;
        }
    }
    else if ((g_AsciiManager.uiFadeState == 2) ||
             (160.0f <= this->positionCenter.x))
    {
        if ((g_AsciiManager.uiFadeState == 2) &&
            (160.0f < this->positionCenter.x))
        {
            g_AsciiManager.otherVms[0].pendingInterrupt = 3;
            g_AsciiManager.uiFadeState = 3;
        }
    }
    else
    {
        g_AsciiManager.otherVms[0].pendingInterrupt = 2;
        g_AsciiManager.uiFadeState = 2;
    }
}

// FUNCTION: TH07 0x00441fb0
u32 Player::OnUpdate(Player *arg)
{
    if (g_GameManager.isTimeStopped != 0)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    arg->UpdateBombProjectiles();
    arg->UpdateBorderAndBombState();
    if (arg->playerState == PLAYER_STATE_DEAD)
    {
        if (arg->UpdateDeath() == 0)
            goto LAB_00442012;
    }
    else if (arg->playerState != PLAYER_STATE_SPAWNING)
        goto LAB_00442012;
    arg->Respawn();
LAB_00442012:
    arg->UpdateState();
    if ((arg->playerState != PLAYER_STATE_DEAD) &&
        (arg->playerState != PLAYER_STATE_SPAWNING))
    {
        arg->HandlePlayerInputs();
    }
    g_AnmManager->ExecuteScript(&arg->playerSprite);
    if (arg->orbState != ORB_HIDDEN)
    {
        g_AnmManager->ExecuteScript(&arg->orbsSprite[0]);
        g_AnmManager->ExecuteScript(&arg->orbsSprite[1]);
    }
    arg->UpdateShots();
    arg->UpdateFireBulletTimer();
    arg->UpdateUI();
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x004420b0
u32 Player::OnDrawHighPrio(Player *arg)
{
    ZunColor local_8;

    arg->DrawBullets();
    if (arg->bombInfo.isInUse != 0)
    {
        if (arg->bombInfo.isFocus == 0)
        {
            arg->bombInfo.draw(arg);
        }
        else
        {
            arg->bombInfo.drawFocus(arg);
        }
    }
    if (g_GameManager.isInPauseMenu == 0)
    {
        (arg->playerSprite).pos.x =
            g_GameManager.arcadeRegionTopLeftPos.x + arg->positionCenter.x;
        (arg->playerSprite).pos.y =
            g_GameManager.arcadeRegionTopLeftPos.y + arg->positionCenter.y;
        (arg->playerSprite).pos.z = 0.0f;
        g_AnmManager->DrawNoRotation(&arg->playerSprite);
        if ((arg->orbState != ORB_HIDDEN) &&
            (((arg->playerState == PLAYER_STATE_ALIVE ||
               (arg->playerState == PLAYER_STATE_BORDER)) ||
              (arg->playerState == PLAYER_STATE_INVULNERABLE))))
        {
            arg->orbsSprite[0].pos.x =
                g_GameManager.arcadeRegionTopLeftPos.x + arg->orbsPosition[0].x;
            arg->orbsSprite[0].pos.y =
                g_GameManager.arcadeRegionTopLeftPos.y + arg->orbsPosition[0].y;
            arg->orbsSprite[0].pos.z = 0.0f;
            arg->orbsSprite[1].pos.x =
                g_GameManager.arcadeRegionTopLeftPos.x + arg->orbsPosition[1].x;
            arg->orbsSprite[1].pos.y =
                g_GameManager.arcadeRegionTopLeftPos.y + arg->orbsPosition[1].y;
            arg->orbsSprite[1].pos.z = 0.0f;
            g_AnmManager->Draw(&arg->orbsSprite[0]);
            g_AnmManager->Draw(&arg->orbsSprite[1]);
        }
    }
    if ((arg->playerState == PLAYER_STATE_BORDER) &&
        (0 < arg->invulnerabilityTimer.current))
    {
        if (arg->invulnerabilityTimer.current % 4 < 2)
        {
            (arg->playerSprite).color.color = 0xffff0000;
        }
        else
        {
            (arg->playerSprite).color.color = 0xffffffff;
        }
        if (g_Player.invulnerabilityTimer.current < 0x1fe)
        {
            if (g_Player.invulnerabilityTimer.current < 0x1e)
            {
                local_8.bytes.g =
                    -0x80 -
                    (char)((g_Player.invulnerabilityTimer.current * 0x50) / 0x1e);
                local_8.bytes.b = local_8.bytes.g;
                local_8.bytes.r = local_8.bytes.g;
                local_8.bytes.a = 0x80;
            }
            else
            {
                local_8.color = 0x80303030;
            }
        }
        else
        {
            local_8.bytes.g =
                -0x80 -
                (char)(((0x21c - g_Player.invulnerabilityTimer.current) * 0x50) /
                       0x1e);
            local_8.bytes.b = local_8.bytes.g;
            local_8.bytes.r = local_8.bytes.g;
            local_8.bytes.a = 0x80;
        }
        g_Stage.SmoothBlendColor(local_8);
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00442350
u32 Player::OnDrawLowPrio(Player *arg)
{
    arg->DrawBulletExplosions();
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(fVar1, fVar2)
// FUNCTION: TH07 0x00442370
f32 Player::AngleToPlayer(D3DXVECTOR3 *pos)
{
    f32 fVar1;
    f32 fVar2;

    fVar2 = this->positionCenter.x - pos->x;
    fVar1 = this->positionCenter.y - pos->y;
    if (fVar1 == 0.0f && fVar2 == 0.0f)
    {
        return 1.5707964f;
    }
    else
    {
        return atan2f(fVar1, fVar2);
    }
}

// FUNCTION: TH07 0x004423e0
ZunResult Player::AddedCallback(Player *arg)
{
    bool bVar1;
    PlayerBullet *bullet;
    i32 i;

    if (ShtData::LoadShtData(
            &arg->shooterData,
            g_ShooterTable[g_GameManager.shotTypeAndCharacter]) == ZUN_SUCCESS)
    {
        if (ShtData::LoadShtData(
                &arg->shooterData2,
                g_ShooterTable2[g_GameManager.shotTypeAndCharacter]) ==
            ZUN_SUCCESS)
        {
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
                if (g_GameManager.character == CHAR_REIMU)
                {
                    // STRING: TH07 0x00496ad8
                    if (g_AnmManager->LoadAnms(10, "data/player00.anm", 0x400) !=
                        ZUN_SUCCESS)
                    {
                        return ZUN_ERROR;
                    }
                }
                else if (g_GameManager.character == CHAR_MARISA)
                {
                    // STRING: TH07 0x00496ac4
                    if (g_AnmManager->LoadAnms(10, "data/player01.anm", 0x400) !=
                        ZUN_SUCCESS)
                    {
                        return ZUN_ERROR;
                    }
                }
                else if ((g_GameManager.character == CHAR_SAKUYA) &&
                         // STRING: TH07 0x00496ab0
                         (g_AnmManager->LoadAnms(10, "data/player02.anm", 0x400) !=
                          ZUN_SUCCESS))
                {
                    return ZUN_ERROR;
                }
            }
            (arg->playerSprite).anmFileIdx = 0x400;
            g_AnmManager->SetAndExecuteScript(&arg->playerSprite,
                                              g_AnmManager->scripts[0x400]);
            arg->positionCenter.x = g_GameManager.arcadeRegionSize.x / 2.0f;
            arg->positionCenter.y = g_GameManager.arcadeRegionSize.y - 64.0f;
            arg->positionCenter.z = 0.49f;
            arg->orbsPosition[0].z = 0.49f;
            arg->orbsPosition[1].z = 0.49f;
            for (i = 0; i < 0x80; i++)
            {
                arg->bombProjectiles[i].size.x = 0.0f;
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
            arg->invulnerabilityTimer.Initialize(0x78);
            arg->orbState = ORB_UNFOCUSED;
            arg->orbsSprite[0].anmFileIdx = 0x480;
            g_AnmManager->SetAndExecuteScript(arg->orbsSprite,
                                              g_AnmManager->scripts[0x480]);
            arg->orbsSprite[1].anmFileIdx = 0x481;
            g_AnmManager->SetAndExecuteScript(arg->orbsSprite + 1,
                                              g_AnmManager->scripts[0x481]);
            bullet = arg->bullets;
            for (i = 0; i < 0x60; i++)
            {
                bullet->bulletState = 0;
                bullet = bullet + 1;
            }
            arg->fireBulletTimer.Initialize(-1);
            arg->bombInfo.bombCalc =
                g_BombData[g_GameManager.shotTypeAndCharacter].calc;
            arg->bombInfo.draw = g_BombData[g_GameManager.shotTypeAndCharacter].draw;
            arg->bombInfo.bombFocusCalc =
                g_BombData[g_GameManager.shotTypeAndCharacter].calcFocus;
            arg->bombInfo.drawFocus =
                g_BombData[g_GameManager.shotTypeAndCharacter].drawFocus;
            arg->bombInfo.isInUse = 0;
            arg->orbAngle = -1.5707964f;
            arg->verticalMovementSpeedMultiplierDuringBomb = 1.0f;
            arg->horizontalMovementSpeedMultiplierDuringBomb = 1.0f;
            arg->respawnTimer = g_Player.shooterData->initialRespawnTimer;
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
                g_AsciiManager.otherVms[0].pendingInterrupt = 1;
                g_AsciiManager.uiFadeState = 1;
            }
            g_AsciiManager.otherVms[3].pendingInterrupt = 2;
            g_AsciiManager.otherVms[4].pendingInterrupt = 2;
            g_AsciiManager.otherVms[5].pendingInterrupt = 2;
            if (g_GameManager.globals->cherryStart + 50000 <=
                g_GameManager.cherryPlus)
            {
                g_GameManager.cherryPlus = g_GameManager.globals->cherryStart + 50000;
                g_Player.ActivateBorder();
            }
            return ZUN_SUCCESS;
        }
        else
        {
            return ZUN_ERROR;
        }
    }
    else
    {
        return ZUN_ERROR;
    }
}

// FUNCTION: TH07 0x004428e0
ZunResult Player::DeletedCallback(Player *arg)
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
        g_AnmManager->ReleaseAnm(10);
        g_AsciiManager.otherVms[0].pendingInterrupt = 99;
        g_AsciiManager.uiFadeState = 99;
        g_AsciiManager.otherVms[3].pendingInterrupt = 99;
        g_AsciiManager.otherVms[4].pendingInterrupt = 99;
        g_AsciiManager.otherVms[5].pendingInterrupt = 99;
    }
    SAFE_FREE(g_Player.shooterData);
    SAFE_FREE(g_Player.shooterData2);
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x004429d0
ZunResult Player::RegisterChain(u32 param_1)
{
    Player *mgr = &g_Player;
    memset(mgr, 0, sizeof(Player));
    mgr->invulnerabilityTimer.Initialize2(0);
    mgr->initParam = param_1;
    mgr->calcChain = g_Chain.CreateElem((ChainCallback)OnUpdate);
    mgr->drawChain1 = g_Chain.CreateElem((ChainCallback)OnDrawHighPrio);
    mgr->drawChain2 = g_Chain.CreateElem((ChainCallback)OnDrawLowPrio);
    mgr->calcChain->arg = mgr;
    mgr->drawChain1->arg = mgr;
    mgr->drawChain2->arg = mgr;
    mgr->calcChain->addedCallback = (ChainLifecycleCallback)AddedCallback;
    mgr->calcChain->deletedCallback = (ChainLifecycleCallback)DeletedCallback;
    if (g_Chain.AddToCalcChain(mgr->calcChain, 8) != 0)
        return ZUN_ERROR;

    g_Chain.AddToDrawChain(mgr->drawChain1, 6);
    g_Chain.AddToDrawChain(mgr->drawChain2, 8);
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00442b10
void Player::CutChain()
{
    g_Chain.Cut(g_Player.calcChain);
    g_Player.calcChain = NULL;
    g_Chain.Cut(g_Player.drawChain1);
    g_Player.drawChain1 = NULL;
    g_Chain.Cut(g_Player.drawChain2);
    g_Player.drawChain2 = NULL;
}

// FUNCTION: TH07 0x00442b70
ZunResult ShtData::LoadShtData(ShtData **data, const char *shtPath)
{
    ShtEntry *local_c;

    *data = (ShtData *)FileSystem::OpenFile(shtPath, 0);
    if (*data == NULL)
    {
        return ZUN_ERROR;
    }
    else
    {
        for (i32 i = 0; i < (i32)(u32)(*data)->entryCount; i++)
        {
            (&(*data)->levels)[i].entry =
                (ShtEntry *)((u8 *)&(*data)->numLevels +
                             (i32)(&(*data)->levels)[i].entry);
            for (local_c = (&(*data)->levels)[i].entry; -1 < local_c->fireInterval;
                 local_c = local_c + 1)
            {
                local_c->fireCallback = g_ShtFireFuncs[(i32)local_c->fireCallback];
                local_c->updateCallback =
                    g_ShtUpdateFuncs[(i32)local_c->updateCallback];
                local_c->drawCallback = g_ShtDrawFuncs[(i32)local_c->drawCallback];
                local_c->hitCallback = g_ShtHitFuncs[(i32)local_c->hitCallback];
            }
        }
        return ZUN_SUCCESS;
    }
}
