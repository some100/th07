#include "EnemyManager.hpp"

#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "EclManager.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "Player.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "ZunResult.hpp"
#include "d3dx8.h"
#include "utils.hpp"

// GLOBAL: TH07 0x009a9b00
EnemyManager g_EnemyManager;

// GLOBAL: TH07 0x012fe210
ChainElem g_EnemyManagerCalcChain;

// GLOBAL: TH07 0x009a9adc
ChainElem g_EnemyManagerDrawChain1;

// GLOBAL: TH07 0x012fe230
ChainElem g_EnemyManagerDrawChain2;

// GLOBAL: TH07 0x0049f1b8
u32 g_SpellcardScore[141] = {
    0x1E8480, 0x1E8480, 0x2191C0, 0x2191C0, 0x249F00, 0x249F00, 0x249F00,
    0x249F00, 0x249F00, 0x249F00, 0x27AC40, 0x27AC40, 0x27AC40, 0x27AC40,
    0x27AC40, 0x27AC40, 0x27AC40, 0x27AC40, 0x27AC40, 0x27AC40, 0x27AC40,
    0x27AC40, 0x27AC40, 0x27AC40, 0x27AC40, 0x27AC40, 0x2DC6C0, 0x2DC6C0,
    0x2DC6C0, 0x2DC6C0, 0x2DC6C0, 0x2DC6C0, 0x2DC6C0, 0x2DC6C0, 0x2DC6C0,
    0x2DC6C0, 0x2DC6C0, 0x2DC6C0, 0x2DC6C0, 0x2DC6C0, 0x2DC6C0, 0x2DC6C0,
    0x2DC6C0, 0x2DC6C0, 0x3567E0, 0x3567E0, 0x3567E0, 0x3567E0, 0x3567E0,
    0x3567E0, 0x3567E0, 0x3567E0, 0x3567E0, 0x3567E0, 0x3567E0, 0x3567E0,
    0x3567E0, 0x3567E0, 0x3567E0, 0x3567E0, 0x3567E0, 0x3567E0, 0x3567E0,
    0x3567E0, 0x3567E0, 0x3567E0, 0x3567E0, 0x3567E0, 0x3D0900, 0x3D0900,
    0x3D0900, 0x3D0900, 0x3D0900, 0x3D0900, 0x3D0900, 0x3D0900, 0x3D0900,
    0x3D0900, 0x3D0900, 0x3D0900, 0x3D0900, 0x3D0900, 0x3D0900, 0x3D0900,
    0x3D0900, 0x3D0900, 0x3D0900, 0x3D0900, 0x4C4B40, 0x4C4B40, 0x4C4B40,
    0x4C4B40, 0x4C4B40, 0x4C4B40, 0x4C4B40, 0x4C4B40, 0x4C4B40, 0x4C4B40,
    0x4C4B40, 0x4C4B40, 0x4C4B40, 0x4C4B40, 0x4C4B40, 0x4C4B40, 0x4C4B40,
    0x4C4B40, 0x4C4B40, 0x4C4B40, 0x4C4B40, 0x4C4B40, 0x4C4B40, 0x4C4B40,
    0x2DC6C0, 0x2DC6C0, 0x2DC6C0, 0x2DC6C0, 0x5B8D80, 0x5B8D80, 0x6ACFC0,
    0x6ACFC0, 0x6ACFC0, 0x6ACFC0, 0x6ACFC0, 0x6ACFC0, 0x6ACFC0, 0x6ACFC0,
    0x3D0900, 0x6ACFC0, 0x6ACFC0, 0x6ACFC0, 0x7A1200, 0x7A1200, 0x7A1200,
    0x7A1200, 0x7A1200, 0x7A1200, 0x7A1200, 0x7A1200, 0x3D0900, 0x7A1200,
    0x3D0900};

// FUNCTION: TH07 0x0041e920
void Enemy::Move()
{
    this->finalPos = this->position - this->prevPos;
    this->prevPos = this->position;
    if ((this->flags1 >> 6 & 1) == 0)
    {
        this->position.x +=
            g_Supervisor.effectiveFramerateMultiplier * this->axisSpeed.x;
    }
    else
    {
        this->position.x -=
            g_Supervisor.effectiveFramerateMultiplier * this->axisSpeed.x;
    }
    this->position.y +=
        g_Supervisor.effectiveFramerateMultiplier * this->axisSpeed.y;
    this->position.z +=
        g_Supervisor.effectiveFramerateMultiplier * this->axisSpeed.z;
}

// FUNCTION: TH07 0x0041ea60
void EnemyManager::Initialize()
{
    memset(this, 0, sizeof(EnemyManager));
    memset(&this->enemyTemplate, 0, sizeof(Enemy));
    for (i32 i = 0; i < 2; i++)
    {
        this->enemyTemplate.vms[i].anmFileIdx = -1;
    }
    for (i32 i = 0; i < 0x60; i++)
    {
        this->enemyTemplate.enemyHistory[i].position.x = -999.0f;
    }
    this->enemyTemplate.flags1 = this->enemyTemplate.flags1 | 0x80;
    this->enemyTemplate.timer.Initialize(0);
    this->enemyTemplate.flags3 = this->enemyTemplate.flags3 & 0xf7;
    this->enemyTemplate.hitboxSize.x = 12.0f;
    this->enemyTemplate.hitboxSize.y = 12.0f;
    this->enemyTemplate.hitboxSize.z = 12.0f;
    this->enemyTemplate.axisSpeed.x = 0.0f;
    this->enemyTemplate.axisSpeed.y = 0.0f;
    this->enemyTemplate.axisSpeed.z = 0.0f;
    this->enemyTemplate.angularVelocity = 0.0f;
    this->enemyTemplate.angle = 0.0f;
    this->enemyTemplate.moveAcceleration = 0.0f;
    this->enemyTemplate.moveSpeed = 0.0f;
    this->enemyTemplate.flags1 = this->enemyTemplate.flags1 & 0xfc;
    this->enemyTemplate.flags1 = this->enemyTemplate.flags1 & 0xdf;
    this->enemyTemplate.flags1 = this->enemyTemplate.flags1 & 0xbf;
    this->enemyTemplate.flags2 = this->enemyTemplate.flags2 & 0xbf;
    this->enemyTemplate.stackDepth = 0;
    this->enemyTemplate.life = 1;
    this->enemyTemplate.score = 100;
    this->enemyTemplate.deathAnm1 = 0;
    this->enemyTemplate.deathAnm2 = 0;
    this->enemyTemplate.deathAnm3 = 0;
    this->enemyTemplate.shootInterval = 0;
    this->enemyTemplate.shootIntervalTimer.Initialize(0);
    this->enemyTemplate.shootOffset.x = 0.0f;
    this->enemyTemplate.shootOffset.y = 0.0f;
    this->enemyTemplate.shootOffset.z = 0.0f;
    this->enemyTemplate.anmExLeft = -1;
    this->enemyTemplate.anmExRight = -1;
    this->enemyTemplate.anmExDefaults = -1;
    this->enemyTemplate.flags2 = this->enemyTemplate.flags2 | 1;
    this->enemyTemplate.flags2 = this->enemyTemplate.flags2 | 2;
    this->enemyTemplate.flags2 = this->enemyTemplate.flags2 | 4;
    this->enemyTemplate.flags2 = this->enemyTemplate.flags2 & 0xf7;
    this->enemyTemplate.flags2 = this->enemyTemplate.flags2 | 0x10;
    this->enemyTemplate.flags2 = this->enemyTemplate.flags2 & 0xdf;
    this->enemyTemplate.flags3 = this->enemyTemplate.flags3 & 0xf8;
    this->enemyTemplate.deathCallbackSub = -1;
    this->enemyTemplate.flags2 = this->enemyTemplate.flags2 & 0x7f;
    this->enemyTemplate.effectsNum = 0;
    this->enemyTemplate.runInterrupt = -1;
    for (i32 i = 0; i < 4; i++)
    {
        this->enemyTemplate.lifeCallbackThreshold[i] = -1;
    }
    this->enemyTemplate.timerCallbackThreshold = -1;
    this->enemyTemplate.periodicCallbackSub = -1;
    this->enemyTemplate.laserIdx = 0;
    this->enemyTemplate.damageTintTimer = 0;
    this->enemyTemplate.flags3 = this->enemyTemplate.flags3 & 0xef;
    this->enemyTemplate.bulletRankSpeedLow = -0.15f;
    this->enemyTemplate.bulletRankSpeedHigh = 0.15f;
    this->enemyTemplate.bulletProps.soundIdx = SOUND_BOMB_MARISA_A_FOCUS;
    this->enemyTemplate.bulletProps.soundOverride = SOUND_25;
}

// FUNCTION: TH07 0x0041ee70
EnemyManager::EnemyManager()
{
    Initialize();
}

// FUNCTION: TH07 0x0041ef70
Enemy::Enemy()
{
}

// FUNCTION: TH07 0x0041f220
EnemyEclContext::EnemyEclContext()
{
}

// FUNCTION: TH07 0x0041f2e0
Enemy *EnemyManager::SpawnEnemy(i16 eclSubId, D3DXVECTOR3 *pos, i32 life,
                                char itemDrop, i32 score, u8 param_6)
{
    Enemy *enemy;
    i32 local_8;

    enemy = this->enemies;
    local_8 = 0;
    while (true)
    {
        if (0x1df < local_8)
        {
            return enemy;
        }
        if (-1 < enemy->flags1)
            break;
        local_8 += 1;
        enemy = enemy + 1;
    }

    *enemy = this->enemyTemplate;
    enemy->flags1 = (enemy->flags1 & 0xbf) | (param_6 & 1) << 6;
    if (-1 < life)
    {
        enemy->life = life;
    }
    enemy->position = *pos;
    g_EclManager.CallEclSub(&enemy->currentContext, eclSubId);
    if (g_EclManager.RunEcl(enemy) == ZUN_ERROR)
    {
        enemy->flags1 &= 0x7f;
    }
    else
    {
        enemy->color = enemy->primaryVm.color;
        enemy->itemDrop = itemDrop;
        if (-1 < score)
        {
            enemy->score = score;
        }
        enemy->maxLife = enemy->life;
    }
    return enemy;
}

// FUNCTION: TH07 0x0041f430
Enemy *EnemyManager::SpawnEnemyEx(i32 eclSubId, D3DXVECTOR3 *pos, i32 life,
                                  i32 itemDrop, i32 score, EclContextArgs *args)
{
    Enemy *enemy;
    i32 local_8;

    enemy = this->enemies;
    local_8 = 0;
    while (true)
    {
        if (0x1df < local_8)
        {
            return enemy;
        }
        if (-1 < enemy->flags1)
            break;
        local_8 += 1;
        enemy = enemy + 1;
    }
    *enemy = this->enemyTemplate;
    if (-1 < life)
    {
        enemy->life = life;
    }
    enemy->position = *pos;
    g_EclManager.CallEclSub(&enemy->currentContext, eclSubId);
    enemy->currentContext.eclContextArgs = *args;
    if (g_EclManager.RunEcl(enemy) == ZUN_ERROR)
    {
        enemy->flags1 &= 0x7f;
    }
    else
    {
        enemy->color = enemy->primaryVm.color;
        enemy->itemDrop = itemDrop;
        if (-1 < life)
        {
            enemy->life = life;
        }
        if (-1 < score)
        {
            enemy->score = score;
        }
        enemy->maxLife = enemy->life;
    }
    return enemy;
}

// FUNCTION: TH07 0x0041f580
void Enemy::UpdateEffects()
{
    Effect *effect;

    for (i32 i = 0; i < this->effectsNum; i++)
    {
        effect = this->effects[i];
        if (effect == NULL)
            continue;

        effect->vm.active = (this->flags2 >> 3 & 1) == 0;
        effect->emitterPosition = this->position;
        if (effect->radius < this->effectDistance)
        {
            effect->radius = effect->radius + 0.3f;
        }
        effect->angularVelocity =
            utils::AddNormalizeAngle(effect->angularVelocity, 0.03141593f);
    }
}

// FUNCTION: TH07 0x0041f670
void Enemy::ResetEffectArray()
{
    for (i32 i = 0; i < this->effectsNum; i++)
    {
        if (this->effects[i] == NULL)
            continue;

        this->effects[i]->isFadingOut = 1;
        this->effects[i] = NULL;
    }
    this->effectsNum = 0;
}

// FUNCTION: TH07 0x0041f6f0
void EnemyManager::RunEclTimeline(EclTimeline *timeline)
{
    EclTimelineInstr *pEVar1;
    Enemy *pEVar3;
    f32 fVar5;
    D3DXVECTOR3 local_48;
    D3DXVECTOR3 local_3c;
    D3DXVECTOR3 local_2c;
    D3DXVECTOR3 local_20;

    while (true)
    {
        if (timeline->timelineInstr->time < 0)
        {
        LAB_0041fd08:
            timeline->timelineTime.Tick();
            return;
        }
        if ((timeline->timelineTime).current ==
            (i32)timeline->timelineInstr->time)
        {
            switch (timeline->timelineInstr->opcode)
            {
            case 0:
                if (g_Gui.bossPresent == 0)
                {
                    pEVar1 = timeline->timelineInstr;
                    g_EnemyManager.SpawnEnemy(
                        timeline->timelineInstr->arg0, (D3DXVECTOR3 *)&pEVar1->args,
                        pEVar1->args[3].i, (char)pEVar1->args[4].i, pEVar1->args[5].i, 0);
                }
                break;
            case 1:
                if (g_Gui.bossPresent == 0)
                {
                    g_EnemyManager.SpawnEnemy(
                        timeline->timelineInstr->arg0,
                        (D3DXVECTOR3 *)&timeline->timelineInstr->args[0].f, -1, -1, -1,
                        0);
                }
                break;
            case 2:
                if (g_Gui.bossPresent == 0)
                {
                    pEVar1 = timeline->timelineInstr;
                    g_EnemyManager.SpawnEnemy(
                        timeline->timelineInstr->arg0, (D3DXVECTOR3 *)&pEVar1->args[0].f,
                        pEVar1->args[3].i, (char)pEVar1->args[4].i, pEVar1->args[5].i, 1);
                }
                break;
            case 3:
                if (g_Gui.bossPresent == 0)
                {
                    g_EnemyManager.SpawnEnemy(
                        timeline->timelineInstr->arg0,
                        (D3DXVECTOR3 *)&timeline->timelineInstr->args[0].f, -1, -1, -1,
                        1);
                }
                break;
            case 4:
                if (g_Gui.bossPresent == 0)
                {
                    pEVar1 = timeline->timelineInstr;
                    local_20.x = pEVar1->args[0].f;
                    local_20.y = pEVar1->args[1].f;
                    local_20.z = pEVar1->args[2].f;
                    fVar5 = pEVar1->args[0].f;
                    if (fVar5 < -990.0f != (fVar5 == -990.0f))
                    {
                        local_20.x =
                            g_Rng.GetRandomFloatInRange(g_GameManager.playerMovementAreaSize.x);
                    }
                    if (pEVar1->args[1].f < -990.0f != (pEVar1->args[1].f == -990.0f))
                    {
                        local_20.y =
                            g_Rng.GetRandomFloatInRange(g_GameManager.playerMovementAreaSize.y);
                    }
                    if (pEVar1->args[2].f < -990.0f != (pEVar1->args[2].f == -990.0f))
                    {
                        local_20.z = g_Rng.GetRandomFloatInRange(800.0f);
                    }
                    g_EnemyManager.SpawnEnemy(timeline->timelineInstr->arg0, &local_20,
                                              pEVar1->args[3].i, (char)pEVar1->args[4].i,
                                              pEVar1->args[5].i, 0);
                }
                break;
            case 5:
                if (g_Gui.bossPresent == 0)
                {
                    pEVar1 = timeline->timelineInstr;
                    local_2c.x = pEVar1->args[0].f;
                    local_2c.y = pEVar1->args[1].f;
                    local_2c.z = pEVar1->args[2].f;
                    if (local_2c.x < -990.0f != (local_2c.x == -990.0f))
                    {
                        local_2c.x =
                            g_Rng.GetRandomFloatInRange(g_GameManager.playerMovementAreaSize.x);
                    }
                    if (local_2c.y < -990.0f != (local_2c.y == -990.0f))
                    {
                        local_2c.y =
                            g_Rng.GetRandomFloatInRange(g_GameManager.playerMovementAreaSize.y);
                    }
                    if (local_2c.z < -990.0f != (local_2c.z == -990.0f))
                    {
                        local_2c.z = g_Rng.GetRandomFloatInRange(800.0f);
                    }
                    g_EnemyManager.SpawnEnemy(timeline->timelineInstr->arg0, &local_2c,
                                              -1, -1, -1, 0);
                }
                break;
            case 6:
                if (g_Gui.bossPresent == 0)
                {
                    pEVar1 = timeline->timelineInstr;
                    local_3c.x = pEVar1->args[0].f;
                    local_3c.y = pEVar1->args[1].f;
                    local_3c.z = pEVar1->args[2].f;
                    fVar5 = pEVar1->args[0].f;
                    if (fVar5 < -990.0f != (fVar5 == -990.0f))
                    {
                        local_3c.x =
                            g_Rng.GetRandomFloatInRange(g_GameManager.playerMovementAreaSize.x);
                    }
                    if (pEVar1->args[1].f < -990.0f != (pEVar1->args[1].f == -990.0f))
                    {
                        local_3c.y =
                            g_Rng.GetRandomFloatInRange(g_GameManager.playerMovementAreaSize.y);
                    }
                    if (pEVar1->args[2].f < -990.0f != (pEVar1->args[2].f == -990.0f))
                    {
                        local_3c.z = g_Rng.GetRandomFloatInRange(800.0f);
                    }
                    pEVar3 = g_EnemyManager.SpawnEnemy(
                        timeline->timelineInstr->arg0, &local_3c, pEVar1->args[3].i,
                        (char)pEVar1->args[4].i, pEVar1->args[5].i, 0);
                    pEVar3->flags1 = pEVar3->flags1 | 0x40;
                }
                break;
            case 7:
                if (g_Gui.bossPresent == 0)
                {
                    pEVar1 = timeline->timelineInstr;
                    local_48.x = pEVar1->args[0].f;
                    local_48.y = pEVar1->args[1].f;
                    local_48.z = pEVar1->args[2].f;
                    if (local_48.x < -990.0f != (local_48.x == -990.0f))
                    {
                        local_48.x =
                            g_Rng.GetRandomFloatInRange(g_GameManager.playerMovementAreaSize.x);
                    }
                    if (local_48.y < -990.0f != (local_48.y == -990.0f))
                    {
                        local_48.y =
                            g_Rng.GetRandomFloatInRange(g_GameManager.playerMovementAreaSize.y);
                    }
                    if (local_48.z < -990.0f != (local_48.z == -990.0f))
                    {
                        local_48.z = g_Rng.GetRandomFloatInRange(800.0f);
                    }
                    pEVar3 = g_EnemyManager.SpawnEnemy(timeline->timelineInstr->arg0,
                                                       &local_48, -1, -1, -1, 0);
                    pEVar3->flags1 = pEVar3->flags1 | 0x40;
                }
                break;
            case 8:
                g_Gui.MsgRead((i32)timeline->timelineInstr->arg0 +
                              (u32)g_GameManager.character * 10);
                break;
            case 9:
                if (g_Gui.MsgWait() != 0)
                {
                    timeline->timelineTime.Decrement(1);
                    goto LAB_0041fd08;
                }
                break;
            case 10:
                g_EnemyManager.bosses[timeline->timelineInstr->args[0].i]
                    ->runInterrupt = timeline->timelineInstr->args[1].i;
                break;
            case 0xb:
                g_GameManager.globals->currentPower =
                    (f32)(i32)timeline->timelineInstr->arg0;
                g_GameManager.RegenerateGameIntegrityCsum();
                break;
            case 0xc:
                if ((g_EnemyManager.bosses[timeline->timelineInstr->arg0] != NULL) &&
                    (g_EnemyManager.bosses[timeline->timelineInstr->arg0]->flags1 <
                     0))
                {
                    timeline->timelineTime.Decrement(1);
                    goto LAB_0041fd08;
                }
            }
        }
        else if ((timeline->timelineTime).current <
                 (i32)timeline->timelineInstr->time)
            goto LAB_0041fd08;
        timeline->timelineInstr =
            (EclTimelineInstr *)((u8 *)timeline->timelineInstr +
                                 timeline->timelineInstr->size);
    }
}

// FUNCTION: TH07 0x0041fd70
i32 Enemy::HandleLifeCallback()
{
    i32 local_10;
    i32 local_c;
    Enemy *enemy;

    enemy = g_EnemyManager.enemies;
    local_c = 0;
    while (true)
    {
        if (3 < local_c)
        {
            return 0;
        }
        if ((-1 < this->lifeCallbackThreshold[local_c]) &&
            (this->life < this->lifeCallbackThreshold[local_c]))
            break;
        local_c += 1;
    }
    this->life = this->lifeCallbackThreshold[local_c];
    g_EclManager.CallEclSub(&this->currentContext,
                            (i16)this->lifeCallbackSub[local_c]);
    this->lifeCallbackThreshold[local_c] = -1;
    this->timerCallbackThreshold = -1;
    this->periodicCallbackSub = -1;
    this->bulletRankSpeedLow = -0.5f;
    this->bulletRankSpeedHigh = 0.5f;
    this->bulletRankAmount1Low = 0;
    this->bulletRankAmount1High = 0;
    this->bulletRankAmount2Low = 0;
    this->bulletRankAmount2High = 0;
    this->stackDepth = 0;
    this->bulletProps = g_EnemyManager.enemyTemplate.bulletProps;
    this->shootInterval = 0;
    for (local_10 = 0; local_10 < 0x1e0; local_10 += 1)
    {
        if ((((enemy->flags1 < 0) && ((enemy->flags2 >> 6 & 1) == 0)) &&
             (enemy->life = 0, (enemy->flags2 & 1) == 0)) &&
            (-1 < enemy->deathCallbackSub))
        {
            g_EclManager.CallEclSub(&enemy->currentContext,
                                    (i16)enemy->deathCallbackSub);
            enemy->deathCallbackSub = -1;
        }
        enemy = enemy + 1;
    }
    return 1;
}

// FUNCTION: TH07 0x0041ff80
i32 Enemy::HandleTimerCallback()
{
    u32 uVar2;
    i32 local_1c;
    Enemy *enemy;
    i32 local_10;
    i32 local_c;
    i32 i;

    if (((this->flags2 >> 6 & 1) != 0) && (this->bossId == 0))
    {
        g_Gui.spellcardSecondsRemaining =
            (this->timerCallbackThreshold - (this->timer).current) / 60;
    }
    if ((this->timer).current < this->timerCallbackThreshold)
    {
        return 0;
    }
    else
    {
        local_c = 0;
        for (i = 0; i < 4; i++)
        {
            if ((-1 < this->lifeCallbackThreshold[i]) &&
                (local_c < this->lifeCallbackThreshold[i]))
            {
                local_c = this->lifeCallbackThreshold[i];
                local_10 = i;
            }
        }
        if (0 < local_c)
        {
            this->life = this->lifeCallbackThreshold[local_10];
            this->lifeCallbackThreshold[local_10] = -1;
        }
        g_EclManager.CallEclSub(&this->currentContext, (i16)this->timerCallbackSub);
        this->timerCallbackThreshold = -1;
        this->timerCallbackSub = this->deathCallbackSub;
        (this->timer).Initialize(0);
        if ((this->flags3 >> 6 & 1) == 0)
        {
            g_EnemyManager.spellcardInfo.captureScore = 0;
            g_EnemyManager.spellcardInfo.isCapturing = 0;
            if (g_EnemyManager.spellcardInfo.isActive != 0)
            {
                g_EnemyManager.spellcardInfo.isActive += 1;
            }
            g_BulletManager.RemoveAllBullets(10);
            uVar2 =
                ((f32)(g_GameManager.cherry - g_GameManager.globals->cherryStart) *
                 0.25f);
            g_GameManager.cherry -= uVar2 - (i32)uVar2 % 10;
        }
        enemy = g_EnemyManager.enemies;
        for (local_1c = 0; local_1c < 0x1e0; local_1c += 1)
        {
            if ((((enemy->flags1 < 0) && ((enemy->flags2 >> 6 & 1) == 0)) &&
                 (enemy->life = 0, (enemy->flags2 & 1) == 0)) &&
                (-1 < enemy->deathCallbackSub))
            {
                g_EclManager.CallEclSub(&enemy->currentContext,
                                        (i16)enemy->deathCallbackSub);
                enemy->deathCallbackSub = -1;
            }
            enemy = enemy + 1;
        }
        this->periodicCallbackSub = -1;
        this->bulletProps = g_EnemyManager.enemyTemplate.bulletProps;
        this->shootInterval = 0;
        this->bulletRankSpeedLow = -0.5f;
        this->bulletRankSpeedHigh = 0.5f;
        this->bulletRankAmount1Low = 0;
        this->bulletRankAmount1High = 0;
        this->bulletRankAmount2Low = 0;
        this->bulletRankAmount2High = 0;
        this->stackDepth = 0;
        return 1;
    }
}

// FUNCTION: TH07 0x004202d0
void Enemy::Despawn()
{
    if ((this->flags3 & 7) == 0)
    {
        this->flags1 = this->flags1 & 0x7f;
    }
    else
    {
        this->flags2 = this->flags2 & 0xfe;
    }
    if (((this->flags2 >> 6 & 1) != 0) && (this->bossId < 4))
    {
        g_Gui.bossPresent = 0;
    }
    if (this->effectsNum != 0)
    {
        ResetEffectArray();
    }
    if ((this->flags2 >> 6 & 1) != 0)
    {
        g_EnemyManager.bosses[this->bossId] = NULL;
    }
    g_ReplayManager->replayEventFlags |= 0x20;
}

// FUNCTION: TH07 0x004203b0
void Enemy::ClampPos()
{
    if (this->flags2 < 0)
    {
        if ((this->lowerMoveLimit).x <= this->position.x)
        {
            if ((this->upperMoveLimit).x < this->position.x)
            {
                this->position.x = (this->upperMoveLimit).x;
            }
        }
        else
        {
            this->position.x = (this->lowerMoveLimit).x;
        }
        if ((this->lowerMoveLimit).y <= this->position.y)
        {
            if ((this->upperMoveLimit).y < this->position.y)
            {
                this->position.y = (this->upperMoveLimit).y;
            }
        }
        else
        {
            this->position.y = (this->lowerMoveLimit).y;
        }
    }
}

// FUNCTION: TH07 0x00420490
void Enemy::CheckBulletPlayerCollision(D3DXVECTOR3 *bulletCenter,
                                       D3DXVECTOR3 *bulletSize)
{
    D3DXVECTOR3 local_10;

    local_10.z = bulletSize->z * 1.4285715f;
    local_10.y = bulletSize->y * 1.4285715f;
    local_10.x = bulletSize->x * 1.4285715f;
    if ((((this->flags2 >> 5 & 1) != 0) &&
         ((this->timer).current != (this->timer).previous)) &&
        ((this->timer).current % 6 == 0))
    {
        g_Player.CheckGraze(bulletCenter, &local_10);
    }
    local_10.z = bulletSize->z * 0.6666667f;
    local_10.y = bulletSize->y * 0.6666667f;
    local_10.x = bulletSize->x * 0.6666667f;
    if (((g_Player.CalcKillboxCollision(bulletCenter, &local_10) == 1) &&
         ((this->flags2 & 1) != 0)) &&
        (((this->flags2 >> 6 & 1) == 0 && ((this->flags2 >> 5 & 1) == 0))))
    {
        this->life = this->life - 10;
    }
}

// FUNCTION: TH07 0x00420620
u32 EnemyManager::OnUpdate(EnemyManager *arg)
{
    f32 fVar1;
    f32 fVar2;
    i32 iVar11;
    u32 uVar14;
    i32 local_1f0;
    f32 local_6c;
    i32 local_60;
    i32 local_58;
    i32 local_44;
    Enemy *enemy;
    i32 local_3c;
    i32 local_38;
    D3DXVECTOR3 local_30;
    f32 local_24;
    i32 i;
    u32 local_1c;
    i32 local_18;
    i32 local_14;
    f32 local_10;
    f32 local_c;
    f32 local_8;

    local_18 = 0;
    if (g_GameManager.currentStage < 5)
    {
        local_1f0 = g_GameManager.currentStage << 1;
    }
    else
    {
        local_1f0 = 10;
    }
    local_14 = local_1f0;
    if (g_Gui.HasCurrentMsgIdx() == 0)
    {
        if ((arg->timelineTime.current != arg->timelineTime.previous) &&
            (arg->timelineTime.current %
                 (i32)(g_GameManager.globals->livesRemaining * -0xf0 + 0x960) ==
             0))
        {
            g_GameManager.IncreaseSubrank(100);
        }
        g_GameManager.activeFrameCounter += 1;
    }
    for (i = 0; i < 4; i++)
    {
        arg->enemyHead[i] = NULL;
    }
    for (i = 0; i < g_EclManager.eclFile->timelineCount; i++)
    {
        if (arg->timelines[i].timelineInstr == NULL)
        {
            arg->timelines[i].timelineInstr = g_EclManager.eclFile->timelinePtr[i];
        }
        RunEclTimeline(arg->timelines + i);
    }
    enemy = arg->enemies;
    arg->enemyCountReal = 0;
    i = 0;
    while (true)
    {
        if (0x1df < i)
        {
            if ((arg->timelineTime.current % 200 == 0) &&
                (g_GameManager.CheckGameIntegrity() != 0))
            {
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            }
            else
            {
                arg->timelineTime.Tick();
                return CHAIN_CALLBACK_RESULT_CONTINUE;
            }
        }
        if (-1 < enemy->flags1)
            goto LAB_004207d0;
        arg->enemyCountReal = arg->enemyCountReal + 1;
        if (((enemy->flags4 >> 3 & 1) != 0) &&
            ((g_Player.bombInfo.isInUse != 0 ||
              (g_Player.playerState != PLAYER_STATE_ALIVE))))
        {
            enemy->timer.Decrement(1);
            goto LAB_00421da7;
        }
        do
        {
            if (g_EclManager.RunEcl(enemy) == ZUN_ERROR)
            {
                enemy->flags1 &= 0x7f;
                enemy->Despawn();
                goto LAB_004207d0;
            }
            if ((enemy->flags4 & 1) == 0)
            {
                enemy->ClampPos();
                enemy->Move();
                enemy->ClampPos();
                if ((enemy->specialEffect != NULL) && ((enemy->flags4 >> 1 & 1) == 0))
                {
                    (enemy->specialEffect->pos1) +=
                        (enemy->position - (enemy->specialEffect->pos1)) * 0.0625f;
                }
            }
            if (enemy->trailFlags != 0)
            {
                iVar11 = (i32)enemy->trailCount;
                while (local_38 = iVar11 - 1, 0 < local_38)
                {
                    enemy->enemyHistory[iVar11 - 1].position =
                        enemy->enemyHistory[iVar11 - 2].position;
                    enemy->enemyHistory[iVar11 - 1].axisSpeed =
                        enemy->enemyHistory[iVar11 - 2].axisSpeed;
                    enemy->enemyHistory[iVar11 - 1].angle =
                        enemy->enemyHistory[iVar11 - 2].angle;
                    iVar11 = local_38;
                }
                enemy->enemyHistory[0].position = enemy->position;
                enemy->enemyHistory[0].axisSpeed = enemy->axisSpeed;
                enemy->enemyHistory[0].angle = enemy->angle;
            }
            if (enemy->primaryVm.sprite == NULL)
            {
                enemy->flags2 = enemy->flags2 | 8;
            }
            if ((((enemy->flags2 >> 3 & 1) == 0) &&
                 ((enemy->flags3 >> 3 & 1) == 0)) &&
                (g_GameManager.IsInBounds(enemy->position.x, enemy->position.y,
                                         (enemy->primaryVm.sprite)->widthPx,
                                         (enemy->primaryVm.sprite)->heightPx) !=
                 0))
            {
                enemy->flags3 = enemy->flags3 | 8;
            }
            if (((enemy->flags3 >> 3 & 1) == 1) &&
                ((((enemy->trailFlags == 0 &&
                    (g_GameManager.IsInBounds(enemy->position.x, enemy->position.y,
                                             (enemy->primaryVm.sprite)->widthPx,
                                             (enemy->primaryVm.sprite)->heightPx) ==
                     0)) ||
                   ((enemy->trailFlags != 0 &&
                     ((g_GameManager.IsInBounds(
                           enemy->position.x, enemy->position.y,
                           (enemy->primaryVm.sprite)->widthPx,
                           (enemy->primaryVm.sprite)->heightPx) == 0 &&
                       (g_GameManager.IsInBounds(
                            enemy->enemyHistory[enemy->trailCount - 1].position.x,
                            enemy->enemyHistory[enemy->trailCount - 1].position.y,
                            (enemy->primaryVm.sprite)->widthPx,
                            (enemy->primaryVm.sprite)->heightPx) == 0)))))) &&
                  (-1 < enemy->flags3))))
            {
                enemy->flags1 &= 0x7f;
                enemy->Despawn();
                goto LAB_004207d0;
            }
        } while ((enemy->HandleLifeCallback() != 0) ||
                 ((-1 < enemy->timerCallbackThreshold &&
                   (enemy->HandleTimerCallback() != 0))));
        enemy->primaryVm.color = enemy->color;
        g_AnmManager->ExecuteScript(&enemy->primaryVm);
        enemy->color = enemy->primaryVm.color;
        for (local_38 = 0; local_38 < 2; local_38 += 1)
        {
            if ((-1 < enemy->vms[local_38].anmFileIdx) &&
                (g_AnmManager->ExecuteScript(enemy->vms + local_38) != 0))
            {
                enemy->vms[local_38].anmFileIdx = -1;
            }
        }
        local_18 = 0;
        local_3c = 0;
        if (((enemy->flags2 >> 3 & 1) == 0) && ((enemy->flags4 >> 2 & 1) == 0))
        {
            if (((enemy->flags2 & 1) != 0) &&
                (((enemy->flags2 >> 1 & 1) != 0 &&
                  (enemy->CheckBulletPlayerCollision(&enemy->position,
                                                     &enemy->hitboxSize),
                   enemy->trailFlags != 0))))
            {
                local_30 = (enemy->hitboxSize);
                for (local_38 = 1; local_38 < enemy->trailInterval; local_38 += 6)
                {
                    if ((enemy->trailFlags & 2) != 0)
                    {
                        fVar1 = (f32)local_38;
                        fVar2 = 1.0f / (f32)(i32)enemy->trailInterval;
                        local_30.z =
                            (enemy->hitboxSize).z - fVar1 * (enemy->hitboxSize).z * fVar2;
                        local_30.y =
                            (enemy->hitboxSize).y - fVar1 * (enemy->hitboxSize).y * fVar2;
                        local_30.x =
                            (enemy->hitboxSize).x - fVar1 * (enemy->hitboxSize).x * fVar2;
                    }
                    enemy->CheckBulletPlayerCollision(
                        &enemy->enemyHistory[local_38].position, &local_30);
                }
            }
            enemy->lastDamage = 0;
            if (((enemy->flags2 & 1) != 0) && ((enemy->flags2 >> 4 & 1) != 0))
            {
                local_1c = g_Player.CheckCollisionWithEnemy(
                    &enemy->position, &enemy->hitboxSize, &local_18);
                if ((0.0f < (enemy->grazeSize).x) &&
                    (iVar11 = g_Player.CheckCollisionWithEnemy(
                         &enemy->position, &enemy->grazeSize, &local_18),
                     local_18 == 0))
                {
                    local_1c = (f32)iVar11 / 2.5f + (f32)(i32)local_1c;
                }
                if (0 < (i32)local_1c)
                {
                    if ((((enemy->flags2 >> 6 & 1) != 0) || (g_Player.isFocus == 0)) &&
                        (g_Player.bombInfo.isInUse == 0))
                    {
                        if (((enemy->flags2 >> 6 & 1) == 0) || (g_Player.isFocus != 0))
                        {
                            iVar11 = 0x1e - local_14;
                        }
                        else
                        {
                            iVar11 = 10 - local_14 / 3;
                        }
                        local_44 = ((i32)local_1c / iVar11) * 10;
                        if (0x46 < local_44)
                        {
                            local_44 = 0x46;
                        }
                        if ((local_44 == 0) && ((g_Player.isFocus == 0 ||
                                                 (((enemy->timer).current & 1U) != 0))))
                        {
                            local_44 = 10;
                        }
                        if (g_GameManager.shotTypeAndCharacter == SHOT_REIMU_A)
                        {
                            if (((local_44 == 0x14) || (local_44 == 0x1e)) &&
                                (((enemy->timer).current & 1U) != 0))
                            {
                                local_44 += -10;
                            }
                            if (((4 < g_GameManager.currentStage) &&
                                 (g_GameManager.currentStage < 7)) &&
                                ((enemy->flags2 >> 6 & 1) == 0))
                            {
                                local_1c = (i32)local_1c / 2;
                            }
                            if ((g_GameManager.currentStage == 4) &&
                                ((enemy->flags2 >> 6 & 1) == 0))
                            {
                                local_1c -=
                                    ((i32)(local_1c + ((i32)local_1c >> 0x1f & 3U)) >> 2) +
                                    ((i32)(local_1c + ((i32)local_1c >> 0x1f & 0xfU)) >> 4);
                            }
                        }
                        if (local_44 != 0)
                        {
                            g_GameManager.AddCherryPlus(local_44);
                        }
                    }
                    if (0x45 < (i32)local_1c)
                    {
                        local_1c = 0x46;
                    }
                    g_GameManager.globals->score =
                        (((i32)local_1c / 5) * 10) / 10 + g_GameManager.globals->score;
                    if ((enemy->flags2 >> 2 & 1) != 0)
                    {
                        if (arg->spellcardInfo.isActive != 0)
                        {
                            if (local_18 == 0)
                            {
                                if ((i32)local_1c < 8)
                                {
                                    if (local_1c != 0)
                                    {
                                        local_1c = 1;
                                    }
                                }
                                else
                                {
                                    local_1c = (i32)local_1c / 7;
                                }
                            }
                            else if (arg->spellcardInfo.usedBomb == 0)
                            {
                                local_1c = 0;
                            }
                            else if ((i32)local_1c < 3)
                            {
                                if (local_1c != 0)
                                {
                                    local_1c = 1;
                                }
                            }
                            else
                            {
                                local_1c = (f32)(i32)local_1c / 2.5f;
                            }
                        }
                        if (0 < enemy->invincibilityTimer.current)
                        {
                            if ((enemy->flags2 >> 6 & 1) == 0)
                            {
                                local_1c = 0;
                            }
                            else
                            {
                                local_1c = (i32)local_1c / 9;
                            }
                        }
                        enemy->life = enemy->life - local_1c;
                        enemy->lastDamage = local_1c;
                    }
                    local_3c = 1;
                }
                if ((enemy->flags2 >> 6 & 1) != 0)
                {
                    local_8 = enemy->position.z - g_Player.positionCenter.z;
                    local_c = enemy->position.y - g_Player.positionCenter.y;
                    local_10 = enemy->position.x - g_Player.positionCenter.x;
                    if ((g_Player.targetingEnemy == 0) ||
                        (fabsf(local_10) < fabsf(g_Player.positionOfLastEnemyHit.x -
                                                 g_Player.positionCenter.x)))
                    {
                        g_Player.positionOfLastEnemyHit = enemy->position;
                    }
                    if (g_GameManager.character == CHAR_SAKUYA)
                    {
                        fVar1 = g_Player.sakuyaTargetPosition.x - g_Player.positionCenter.x;
                        local_24 = atan2f((enemy->position.y - g_Player.positionCenter.y),
                                          (enemy->position.x - g_Player.positionCenter.x));
                        if (((-2.0943952f <= local_24) &&
                             (local_24 < -1.0471976f != (local_24 == -1.0471976f))) &&
                            ((g_Player.targetingEnemy == 0 ||
                              (fabsf(local_10) < fabsf(fVar1)))))
                        {
                            g_Player.sakuyaTargetPosition = enemy->position;
                            g_Player.targetingEnemy = 1;
                        }
                    }
                    else
                    {
                        g_Player.targetingEnemy = 1;
                    }
                }
                if (g_Player.targetingEnemy == 0)
                {
                    if (g_Player.positionOfLastEnemyHit.y < enemy->position.y)
                    {
                        g_Player.positionOfLastEnemyHit = enemy->position;
                    }
                    if ((((g_GameManager.character == CHAR_SAKUYA) &&
                          (g_Player.sakuyaTargetPosition.y < -900.0f)) &&
                         (local_24 =
                              atan2f((enemy->position.y - g_Player.positionCenter.y),
                                     (enemy->position.x - g_Player.positionCenter.x)),
                          -2.0943952f <= local_24)) &&
                        (local_24 < -1.0471976f != (local_24 == -1.0471976f)))
                    {
                        g_Player.sakuyaTargetPosition = enemy->position;
                    }
                }
            }
        }
        if ((0 < enemy->life) || ((enemy->flags2 & 1) == 0))
            goto LAB_00421da7;
        for (local_58 = 0; local_58 < 4; local_58 += 1)
        {
            enemy->lifeCallbackThreshold[local_58] = -1;
        }
        enemy->timerCallbackThreshold = -1;
        enemy->periodicCallbackSub = -1;
        switch (enemy->flags3 & 7)
        {
        case 0:
            g_GameManager.globals->score =
                enemy->score / 10 + g_GameManager.globals->score;
            enemy->flags1 &= 0x7f;
            goto LAB_00421a39;
        case 1:
            g_GameManager.globals->score =
                enemy->score / 10 + g_GameManager.globals->score;
            enemy->flags2 = enemy->flags2 & 0xfe;
        LAB_00421a39:
            if ((enemy->flags2 >> 6 & 1) != 0)
            {
                g_Gui.bossPresent = 0;
                enemy->ResetEffectArray();
            }
        switchD_004218d5_caseD_2:
            if (enemy->itemDrop < ITEM_POWER_SMALL)
            {
                if (enemy->itemDrop == -1)
                {
                    if ((u32)arg->randomItemSpawnIdx % 3 == 0)
                    {
                        g_EffectManager.SpawnParticles(enemy->deathAnm2 + 4,
                                                       &enemy->position, 6, 0xffffffff);
                        g_ItemManager.SpawnItem(
                            &enemy->position,
                            (ItemType)g_ItemDropTable[arg->randomItemTableIdx], local_18);
                        arg->randomItemTableIdx = arg->randomItemTableIdx + 1;
                        if (0x1f < arg->randomItemTableIdx)
                        {
                            arg->randomItemTableIdx = 0;
                        }
                    }
                    arg->randomItemSpawnIdx = arg->randomItemSpawnIdx + 1;
                }
            }
            else
            {
                g_EffectManager.SpawnParticles(enemy->deathAnm2 + 4, &enemy->position,
                                               3, 0xffffffff);
                g_ItemManager.SpawnItem(&enemy->position, enemy->itemDrop, local_18);
            }
            if (((enemy->flags2 >> 6 & 1) != 0) &&
                (g_EnemyManager.spellcardInfo.isActive == 0))
            {
                iVar11 = g_EnemyManager.RemoveAllEnemies(
                    8000, g_BulletManager.DespawnBullets(8000, 1));
                if (iVar11 != 0)
                {
                    g_GameManager.globals->score =
                        iVar11 / 10 + g_GameManager.globals->score;
                    g_Gui.ShowBonusScore(iVar11);
                }
            }
            enemy->life = 0;
            g_ReplayManager->replayEventFlags =
                g_ReplayManager->replayEventFlags | 0x20;
            break;
        case 2:
            goto switchD_004218d5_caseD_2;
        case 3:
            enemy->life = 1;
            enemy->flags2 = enemy->flags2 & 0xfb;
            enemy->flags3 = enemy->flags3 & 0xf8;
            g_Gui.bossPresent = 0;
            g_ReplayManager->replayEventFlags =
                g_ReplayManager->replayEventFlags | 0x20;
            if (-1 < enemy->deathAnm1)
            {
                g_EffectManager.SpawnParticles((i32)enemy->deathAnm1, &enemy->position,
                                               1, 0xffffffff);
                g_EffectManager.SpawnParticles((i32)enemy->deathAnm1, &enemy->position,
                                               1, 0xffffffff);
                g_EffectManager.SpawnParticles((i32)enemy->deathAnm1, &enemy->position,
                                               1, 0xffffffff);
            }
        }
        g_SoundPlayer.PlaySoundByIdx((i % 2) + 2, 0);
        if (-1 < enemy->deathAnm1)
        {
            g_EffectManager.SpawnParticles((i32)enemy->deathAnm1, &enemy->position, 1,
                                           0xffffffff);
            g_EffectManager.SpawnParticles(enemy->deathAnm2 + 4, &enemy->position, 4,
                                           0xffffffff);
        }
        if (-1 < enemy->deathCallbackSub)
        {
            enemy->bulletRankSpeedLow = -0.5f;
            enemy->bulletRankSpeedHigh = 0.5f;
            enemy->bulletRankAmount1Low = 0;
            enemy->bulletRankAmount1High = 0;
            enemy->bulletRankAmount2Low = 0;
            enemy->bulletRankAmount2High = 0;
            enemy->stackDepth = 0;
            for (local_60 = 0; local_60 < 4; local_60 += 1)
            {
                enemy->lifeCallbackThreshold[local_60] = -1;
            }
            enemy->timerCallbackThreshold = -1;
            enemy->periodicCallbackSub = -1;
            enemy->bulletProps = g_EnemyManager.enemyTemplate.bulletProps;
            enemy->shootInterval = 0;
            g_EclManager.CallEclSub(&enemy->currentContext,
                                    (i16)enemy->deathCallbackSub);
            enemy->deathCallbackSub = -1;
        }
    LAB_00421da7:
        if (enemy->damageTintTimer == 0)
        {
            if (local_3c == 0)
            {
                enemy->primaryVm.useColor2 = 0;
            }
            else
            {
                g_SoundPlayer.PlaySoundByIdx(SOUND_20, 0);
                enemy->primaryVm.color2.bytes.r = 0xff;
                enemy->primaryVm.color2.bytes.g = 0x80;
                enemy->primaryVm.color2.bytes.b = 0xc0;
                enemy->primaryVm.color2.bytes.a = enemy->primaryVm.color.bytes.a;
                enemy->primaryVm.useColor2 = 1;
                enemy->damageTintTimer = 1;
            }
        }
        else
        {
            enemy->damageTintTimer = enemy->damageTintTimer - 1;
            enemy->primaryVm.useColor2 = 0;
        }
        if ((enemy->flags2 >> 6 & 1) != 0)
        {
            if ((g_Gui.HasCurrentMsgIdx() == 0) && (enemy->bossId == 0))
            {
                g_Gui.bossHealthBar = (f32)enemy->life / (f32)enemy->maxLife;
            }
            if ((enemy->flags2 >> 6 & 1) < 4)
            {
                if ((enemy->flags2 >> 3 & 1) == 0)
                {
                    local_6c = enemy->position.x + 32.0f;
                }
                else
                {
                    local_6c = -999.0f;
                }
                uVar14 = enemy->bossId;
                g_AsciiManager.otherVms[uVar14 + 3].pos.x = local_6c;
                g_AsciiManager.otherVms[uVar14 + 3].pos.y = 472.0;
                g_AsciiManager.otherVms[uVar14 + 3].pos.z = 0.0f;
                g_AsciiManager.bossDamageTint[enemy->bossId] =
                    enemy->primaryVm.useColor2;
            }
        }
        enemy->UpdateEffects();
        if (g_GameManager.isTimeStopped == 0)
        {
            enemy->timer.Tick();
        }
        if (0 < enemy->invincibilityTimer.current)
        {
            enemy->invincibilityTimer.Decrement(1);
        }
        if (((enemy->flags2 >> 3 & 1) == 0) && (enemy->flags1 < 0))
        {
            enemy->next = arg->enemyHead[enemy->zLayer];
            arg->enemyHead[enemy->zLayer] = enemy;
        }
    LAB_004207d0:
        i++;
        enemy = enemy + 1;
    }
}

// FUNCTION: TH07 0x004220f0
f32 AngleLerp(f32 start, f32 target, f32 t)
{
    f32 local_c;
    f32 local_8;

    if (start > target)
    {
        local_c = target - start;
        local_8 = (start + ZUN_2PI) - target;
    }
    else
    {
        local_c = start - target;
        local_8 = (target + ZUN_2PI) - start;
        start = target;
    }
    if (local_c < local_8)
    {
        local_8 = local_c;
    }
    return local_8 * t + start;
}

// FUNCTION: TH07 0x00422170
u32 EnemyManager::ActualOnDraw(EnemyManager *arg, i32 param_2, i32 param_3)
{
    f32 fVar2;
    f32 fVar3;
    f32 fVar4;
    f32 fVar5;
    f32 fVar6;
    ZunColor::ColorBytes ZVar7;
    f32 fVar8;
    i32 iVar10;
    f32 fVar12;
    f32 local_44;
    VertexTex1DiffuseXyzrwh *local_3c;
    f32 local_38;
    f32 local_34;
    i32 local_2c;
    f32 local_28;
    f32 local_24;
    Enemy *local_20;
    i32 local_1c;
    u8 bStack_11;
    i32 local_10;

    for (local_10 = param_2; local_10 < param_3; local_10 += 1)
    {
        for (local_20 = arg->enemyHead[local_10]; local_20 != NULL;
             local_20 = local_20->next)
        {
            if (local_20->vms[0].anmFileIdx >= 0)
            {
                if (local_20->vms[0].autoRotate)
                {
                    local_20->vms[0].rotation.z = local_20->angle;
                    local_20->vms[0].updateRotation = 1;
                }

                local_20->vms[0].pos = local_20->position + local_20->vms[0].offset;
                local_20->vms[0].pos.z = 0.3f;
                local_20->vms[0].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
                local_20->vms[0].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;

                g_AnmManager->Draw(&local_20->vms[0]);
            }
            if ((local_20->flags3 >> 4 & 1) != 0)
            {
                local_20->primaryVm.rotation.z = local_20->angle;
                local_20->primaryVm.updateRotation = 1;
            }
            local_20->primaryVm.pos = local_20->position + local_20->primaryVm.offset;
            local_20->primaryVm.pos.z = 0.29f;
            if (((local_20->trailFlags & 0x10) == 0) &&
                ((local_20->flags4 >> 2 & 1) == 0))
            {
                local_20->primaryVm.pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
                local_20->primaryVm.pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
                g_AnmManager->Draw(&local_20->primaryVm);
            }
            if (-1 < local_20->vms[1].anmFileIdx)
            {
                if (local_20->vms[1].autoRotate != 0)
                {
                    local_20->vms[1].rotation.z = -local_20->angle;
                    local_20->vms[1].updateRotation = 1;
                }
                local_20->vms[1].pos = local_20->position + local_20->vms[1].offset;
                local_20->vms[1].pos.z = 0.3f;
                local_20->vms[1].pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
                local_20->vms[1].pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
                g_AnmManager->Draw(&local_20->vms[1]);
            }
            if (local_20->trailFlags != 0)
            {
                fVar2 = local_20->primaryVm.scale.x;
                fVar3 = local_20->primaryVm.scale.y;
                ZVar7 = local_20->primaryVm.color.bytes;
                bStack_11 = ZVar7.a;
                if ((local_20->trailFlags & 8) == 0)
                {
                    for (local_1c = (i32)local_20->trailNodeStep;
                         local_1c < local_20->trailCount;
                         local_1c += local_20->trailNodeStep)
                    {
                        if (-990.0f <= local_20->enemyHistory[local_1c].position.x)
                        {
                            if ((local_20->flags3 >> 4 & 1) != 0)
                            {
                                local_20->primaryVm.rotation.z =
                                    local_20->enemyHistory[local_1c].angle;
                                local_20->primaryVm.updateRotation = 1;
                            }
                            if ((local_20->trailFlags & 2) != 0)
                            {
                                local_20->primaryVm.scale.x =
                                    fVar2 -
                                    ((f32)local_1c * fVar2) / (f32)(i32)local_20->trailCount;
                            }
                            if ((local_20->trailFlags & 4) != 0)
                            {
                                local_20->primaryVm.color.bytes.a =
                                    bStack_11 - (char)((i32)((u32)bStack_11 * local_1c) /
                                                       (i32)local_20->trailCount);
                            }
                            local_20->primaryVm.pos =
                                (local_20->enemyHistory[local_1c].position) +
                                local_20->primaryVm.offset;
                            local_20->primaryVm.pos.z = 0.3f;
                            local_20->primaryVm.pos.x +=
                                g_GameManager.arcadeRegionTopLeftPos.x;
                            local_20->primaryVm.pos.y +=
                                g_GameManager.arcadeRegionTopLeftPos.y;
                            g_AnmManager->Draw(&local_20->primaryVm);
                        }
                    }
                }
                else
                {
                    local_2c = 0;
                    local_1c = 0;
                    while ((local_1c < local_20->trailCount &&
                            (-990.0f <= local_20->enemyHistory[local_1c].position.x)))
                    {
                        local_2c += 2;
                        local_1c += local_20->trailNodeStep;
                    }
                    if (2 < local_2c)
                    {
                        fVar4 = (local_20->primaryVm.sprite->uvEnd).x;
                        fVar5 = (local_20->primaryVm.sprite->uvStart).x;
                        iVar10 = local_2c + 1;
                        local_34 = (local_20->primaryVm.sprite->uvEnd).x +
                                   local_20->primaryVm.uvScrollPos.x;
                        local_3c = local_20->trailVertices;
                        local_1c = 0;
                        while ((local_1c < local_20->trailCount &&
                                (-990.0f <= local_20->enemyHistory[local_1c].position.x)))
                        {
                            if (local_1c == 0)
                            {
                                local_44 = local_20->enemyHistory[0].angle;
                            }
                            else
                            {
                                local_44 =
                                    AngleLerp(local_20->enemyHistory[local_1c - 1].angle,
                                              local_20->enemyHistory[local_1c].angle, 0.5f);
                            }
                            if (((((local_20->trailFlags & 2) == 0) || (local_1c < 1)) ||
                                 ((i32)local_20->trailCount <=
                                  local_1c + local_20->trailNodeStep)) ||
                                ((fVar12 = AngleLerp(
                                      local_20
                                          ->enemyHistory[(i32)local_20->trailNodeStep +
                                                         local_1c - 1]
                                          .angle,
                                      local_20->enemyHistory[local_20->trailNodeStep].angle,
                                      0.5f),
                                  0.00001f <= fabsf(local_38 - local_44) ||
                                      (0.00001f <= fabsf(local_44 - fVar12)))))
                            {
                                local_38 = local_44;
                                fVar12 = sinf(local_44);
                                fVar6 = cosf(local_44);
                                local_28 = 0.0f;
                                local_24 =
                                    (fVar3 * local_20->primaryVm.sprite->heightPx) / 2.0f;
                                if ((local_20->trailFlags & 2) != 0)
                                {
                                    fVar8 = 1.0f - (f32)local_1c / (f32)(i32)local_20->trailCount;
                                    local_28 = fVar8 * 0.0f;
                                    local_24 = local_24 * fVar8;
                                }
                                local_3c[1].color = local_20->primaryVm.color;
                                local_3c->color = local_3c[1].color;
                                if ((local_20->trailFlags & 4) != 0)
                                {
                                    local_3c[1].color.bytes.a =
                                        bStack_11 - (char)((i32)((u32)bStack_11 * local_1c) /
                                                           (i32)local_20->trailCount);
                                    (local_3c->color).bytes.a = local_3c[1].color.bytes.a;
                                }
                                *(D3DXVECTOR3 *)&local_3c->pos =
                                    local_20->enemyHistory[local_1c].position;
                                local_3c->pos.x = (fVar6 * local_28 - fVar12 * local_24) +
                                                  32.0f + local_3c->pos.x;
                                local_3c->pos.y = fVar6 * local_24 + fVar12 * local_28 + 16.0f +
                                                  local_3c->pos.y;
                                local_3c->textureUV.x = local_34;
                                local_3c->textureUV.y =
                                    (local_20->primaryVm.sprite->uvStart).y +
                                    local_20->primaryVm.uvScrollPos.y;
                                *(D3DXVECTOR3 *)&local_3c[1].pos =
                                    local_20->enemyHistory[local_1c].position;
                                local_3c[1].pos.x = fVar12 * local_24 + fVar6 * local_28 +
                                                    32.0f + local_3c[1].pos.x;
                                local_3c[1].pos.y = (fVar12 * local_28 - fVar6 * local_24) +
                                                    16.0f + local_3c[1].pos.y;
                                local_3c[1].textureUV.x = local_34;
                                local_3c[1].textureUV.y =
                                    (local_20->primaryVm.sprite->uvEnd).y +
                                    local_20->primaryVm.uvScrollPos.y;
                                local_3c = local_3c + 2;
                            }
                            else
                            {
                                local_2c += -2;
                            }
                            local_1c += local_20->trailNodeStep;
                            local_34 = local_34 - (fVar4 - fVar5) / (f32)(iVar10 / 2 - 1);
                        }
                        if (2 < local_2c)
                        {
                            g_AnmManager->DrawTriangleStrip(
                                &local_20->primaryVm, local_20->trailVertices, local_2c);
                        }
                    }
                }
                local_20->primaryVm.scale.x = fVar2;
                local_20->primaryVm.scale.y = fVar3;
                local_20->primaryVm.color.bytes = ZVar7;
            }
        }
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00422ca0
u32 EnemyManager::OnDraw1(EnemyManager *arg)
{
    return ActualOnDraw(arg, 0, 2);
}

// FUNCTION: TH07 0x00422cc0
u32 EnemyManager::OnDraw2(EnemyManager *arg)
{
    return ActualOnDraw(arg, 2, 4);
}

// FUNCTION: TH07 0x00422ce0
ZunResult EnemyManager::AddedCallback(EnemyManager *arg)
{
    if ((arg->stgEnmAnmFilename != NULL) &&
        (g_AnmManager->LoadAnms(0xf, arg->stgEnmAnmFilename, 0x900) !=
         ZUN_SUCCESS))
    {
        return ZUN_ERROR;
    }
    if ((arg->stgEnm2AnmFilename != NULL) &&
        (g_AnmManager->LoadAnms(0x10, arg->stgEnm2AnmFilename, 0x900) !=
         ZUN_SUCCESS))
    {
        return ZUN_ERROR;
    }

    arg->randomItemSpawnIdx = g_Rng.GetRandomU16InRange(3);
    arg->randomItemTableIdx = g_Rng.GetRandomU16InRange(8);
    arg->spellcardInfo.isActive = 0;
    g_AsciiManager.otherVms[6].pos.z = -999.0f;
    g_AsciiManager.otherVms[6].pos.y = -999.0f;
    g_AsciiManager.otherVms[6].pos.x = -999.0f;
    g_AsciiManager.otherVms[5].pos.z = -999.0f;
    g_AsciiManager.otherVms[5].pos.y = -999.0f;
    g_AsciiManager.otherVms[5].pos.x = -999.0f;
    g_AsciiManager.otherVms[4].pos.z = -999.0f;
    g_AsciiManager.otherVms[4].pos.y = -999.0f;
    g_AsciiManager.otherVms[4].pos.x = -999.0f;
    g_AsciiManager.otherVms[3].pos.z = -999.0f;
    g_AsciiManager.otherVms[3].pos.y = -999.0f;
    g_AsciiManager.otherVms[3].pos.x = -999.0f;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00422e70
ZunResult EnemyManager::DeletedCallback(EnemyManager *arg)
{
    g_AnmManager->ReleaseAnm(0x10);
    g_AnmManager->ReleaseAnm(0xf);
    D3DXVECTOR3 vec = D3DXVECTOR3(-999.0f, -999.0f, -999.0f);
    g_AsciiManager.otherOtherVms[0].pos = vec;
    g_AsciiManager.otherOtherVms[1].pos = vec;
    g_AsciiManager.otherOtherVms[2].pos = vec;
    g_AsciiManager.otherOtherVms[3].pos = vec;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00422f40
ZunResult EnemyManager::RegisterChain(const char *stgEnm1, const char *stgEnm2)
{
    EnemyManager *mgr = &g_EnemyManager;
    mgr->Initialize();
    mgr->stgEnmAnmFilename = stgEnm1;
    mgr->stgEnm2AnmFilename = stgEnm2;
    g_EnemyManagerCalcChain.callback = (ChainCallback)OnUpdate;
    g_EnemyManagerCalcChain.addedCallback = NULL;
    g_EnemyManagerCalcChain.deletedCallback = NULL;
    g_EnemyManagerCalcChain.addedCallback = (ChainLifecycleCallback)AddedCallback;
    g_EnemyManagerCalcChain.deletedCallback =
        (ChainLifecycleCallback)DeletedCallback;
    g_EnemyManagerCalcChain.arg = mgr;
    if (g_Chain.AddToCalcChain(&g_EnemyManagerCalcChain, 10) != 0)
        return ZUN_ERROR;

    g_EnemyManagerDrawChain1.callback = (ChainCallback)OnDraw1;
    g_EnemyManagerDrawChain1.addedCallback = NULL;
    g_EnemyManagerDrawChain1.deletedCallback = NULL;
    g_EnemyManagerDrawChain1.arg = mgr;
    if (g_Chain.AddToDrawChain(&g_EnemyManagerDrawChain1, 5) != ZUN_SUCCESS)
        return ZUN_ERROR;

    g_EnemyManagerDrawChain2.callback = (ChainCallback)OnDraw2;
    g_EnemyManagerDrawChain2.addedCallback = NULL;
    g_EnemyManagerDrawChain2.deletedCallback = NULL;
    g_EnemyManagerDrawChain2.arg = mgr;
    if (g_Chain.AddToDrawChain(&g_EnemyManagerDrawChain2, 7) != ZUN_SUCCESS)
        return ZUN_ERROR;

    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00423050
void EnemyManager::CutChain()
{
    g_Chain.Cut(&g_EnemyManagerCalcChain);
    g_Chain.Cut(&g_EnemyManagerDrawChain1);
    g_Chain.Cut(&g_EnemyManagerDrawChain2);
}

// FUNCTION: TH07 0x00423090
i32 EnemyManager::RemoveAllEnemies(i32 scoreMax, i32 scoreMin)
{
    i32 local_18;
    i32 i;
    Enemy *local_10;
    i32 local_c;
    i32 local_8;

    local_10 = this->enemies;
    local_c = scoreMin;
    local_8 = 2000;
    for (i = 0; i < 0x1e0; i++)
    {
        if ((local_10->flags1 < 0) && ((local_10->flags2 >> 6 & 1) == 0))
        {
            local_10->life = 0;
            if ((local_10->flags2 >> 5 & 1) != 0)
            {
                g_ItemManager.SpawnItem(&local_10->position, ITEM_POINT_BULLET, 1);
                g_AsciiManager.CreatePopup1(&local_10->position, local_8,
                                            ((local_8 < scoreMax) - 1 & 0xffffff01) -
                                                1);
                local_c += local_8;
                local_8 += 0x1e;
                if (scoreMax < local_8)
                {
                    local_8 = scoreMax;
                }
                if (local_10->trailFlags != 0)
                {
                    for (local_18 = 0; local_18 < local_10->trailCount; local_18 += 6)
                    {
                        g_ItemManager.SpawnItem(&local_10->enemyHistory[local_18].position,
                                                ITEM_POINT_BULLET, 1);
                        g_AsciiManager.CreatePopup1(
                            &local_10->enemyHistory[local_18].position, local_8,
                            ((local_8 < scoreMax) - 1 & 0xffffff01) - 1);
                        local_c += local_8;
                        local_8 += 0x1e;
                        if (scoreMax < local_8)
                        {
                            local_8 = scoreMax;
                        }
                    }
                }
            }
            if (((local_10->flags2 & 1) == 0) && (-1 < local_10->deathCallbackSub))
            {
                g_EclManager.CallEclSub(&local_10->currentContext,
                                        (i16)local_10->deathCallbackSub);
                local_10->deathCallbackSub = -1;
            }
        }
        local_10 = local_10 + 1;
    }
    return local_c;
}

// FUNCTION: TH07 0x004232a0
i32 EnemyManager::HasActiveBoss()
{
    for (i32 i = 0; 7 < i; ++i)
    {
        if (this->bosses[i] != NULL)
            return 1;
    }
    return 0;
}
