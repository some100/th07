#include "ItemManager.hpp"

#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "BulletManager.hpp"
#include "EffectManager.hpp"
#include "EnemyManager.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "Player.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "d3dx8.h"

// GLOBAL: TH07 0x0049ecf8
i32 g_CherryBonusFullPower[30] = {10, 20, 30, 40, 50, 60, 70, 80,
                                  90, 100, 200, 300, 400, 500, 600, 700,
                                  800, 900, 1000, 2000, 3000, 4000, 5000,
                                  6000, 7000, 8000, 9000, 10000, 11000, 12000};

// GLOBAL: TH07 0x0049ed74
i32 g_PowerLevels[9] = {8, 16, 32, 48, 64, 80, 96, 128, 999};

// GLOBAL: TH07 0x0049efa0
u8 g_ItemDropTable[32] = {
    0,
    0,
    1,
    0,
    1,
    0,
    0,
    7,
    1,
    1,
    0,
    0,
    7,
    1,
    1,
    0,
    1,
    0,
    1,
    0,
    1,
    0,
    1,
    0,
    1,
    0,
    7,
    1,
    1,
    1,
    0,
    2,
};

// GLOBAL: TH07 0x00575c70
ItemManager g_ItemManager;

// FUNCTION: TH07 0x004325c0
void AngleToVector(D3DXVECTOR3 *out, f32 angle, f32 speed)
{
    /* out->x = cosf(angle) * speed;
     * out->y = sinf(angle) * speed;
     */
    __asm {
        mov eax, out
        fld [angle]
        fsincos
        fmul [speed]
        fstp float ptr [eax]
        fmul [speed]
        fstp float ptr [eax + 4]
    }
}

// FUNCTION: TH07 0x004325e0
void GameManager::AddCurrentPower(i32 amount)
{
    if (CheckGameIntegrity() != 0)
    {
        NUKE_SUPERVISOR();
    }
    this->globals->currentPower += (f32)amount;
    RegenerateGameIntegrityCsum();
}

// FUNCTION: TH07 0x00432630
ItemManager::ItemManager()
{
}

// FUNCTION: TH07 0x00432690
Item::Item()
{
}

#pragma var_order(i, item)
// FUNCTION: TH07 0x004326f0
Item *ItemManager::SpawnItem(D3DXVECTOR3 *heading, i32 itemType, i32 state)
{
    Item *item;
    i32 i;

    item = &this->items[this->nextIndex];
    if ((i32)g_GameManager.globals->currentPower >= 128)
    {
        if (itemType == ITEM_POWER_SMALL || itemType == ITEM_POWER_BIG)
        {
            itemType = ITEM_CHERRY;
        }
    }
    for (i = 0; i < 1100; i++)
    {
        this->nextIndex++;

        if (item->isInUse != 0)
        {
            if (this->nextIndex >= 0x44c)
            {
                this->nextIndex = 0;
                item = this->items;
            }
            else
            {
                item++;
            }
            continue;
        }
        if (this->nextIndex >= 1100)
        {
            this->nextIndex = 0;
        }
        item->isInUse = 1;
        item->currentPosition = *heading;
        item->startPosition.x = 0.0f;
        item->startPosition.y = -2.2f;
        item->startPosition.z = 0.0f;
        item->itemType = (u8)itemType;
        item->state = (u8)state;
        item->timer = 0;
        if (state == 2)
        {
            item->targetPosition.x = g_Rng.GetRandomFloatInRange(288.0f) + 48.0f;
            item->targetPosition.y = g_Rng.GetRandomFloatInRange(192.0f) - 64.0f;
            item->targetPosition.z = 0.0f;
            item->startPosition = item->currentPosition;
        }
        else if (state == 3)
        {
            item->state = 1;
        }
        else if (state == 4)
        {
            item->state = 0;
        }
        g_AnmManager->SetAnmIdxAndExecuteScript(&item->sprite, itemType + 0x2c4);
        item->sprite.color.color = 0xffffffff;
        item->sprite.zWriteDisable = 1;
        item->autoCollect = 0;
        item->isArrowSprite = 1;
        break;
    }

    return i < 0x44c ? item : &this->items[0x44c];
}

#pragma var_order(i, itemTimerSecs, itemScore, playerAngle, local_20, itemAcquired, \
                  item, j, prevPowerIdx, k, prevPowerLevel2)
// FUNCTION: TH07 0x00432990
void ItemManager::OnUpdate()
{
    i32 prevPowerLevel2;
    i32 k;
    i32 prevPowerIdx;
    i32 j;
    Item *item;
    i32 itemAcquired;
    f32 playerAngle;
    i32 itemScore;
    f32 itemTimerSecs;
    i32 i;

    item = this->items;
    D3DXVECTOR3 local_20(g_Player.shooterData->itemCollectRadius,
                         g_Player.shooterData->itemCollectRadius, 16.0f);
    itemAcquired = 0;
    this->activeItemCount = 0;
    this->listTail = &this->listHead;
    this->listHead.next = NULL;

    for (i = 0; i < 1100; i++, item++)
    {
        if (item->isInUse == 0)
        {
            continue;
        }

        this->activeItemCount++;
        if (item->state == 2)
        {
            if (60 < item->timer)
            {
                itemTimerSecs = item->timer.AsFloat() / 60.0f;
                item->currentPosition = itemTimerSecs * item->targetPosition +
                                        item->startPosition *
                                            (1.0f - itemTimerSecs);
                goto check_collision;
            }
            else
            {
                if (item->timer == 60)
                {
                    item->startPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
                    item->state = 0;
                }
            }
        }
        else
        {
            if (item->state == 1 || ((128.0 <= (f64)(i32)g_GameManager.globals->currentPower || g_GameManager.difficulty >= 4) && g_Player.positionCenter.y < g_Player.shooterData->pocY) || g_Player.hasBorder == 1)
            {
                if (g_Player.playerState != 1)
                {
                    playerAngle = g_Player.AngleToPlayer(&item->currentPosition);
                    AngleToVector(&item->startPosition, playerAngle, g_Player.shooterData->itemCollectSpeed);
                    item->state = 1;
                    if (g_Player.hasBorder == 1)
                    {
                        item->autoCollect = 1;
                    }
                }
                else
                {
                    item->startPosition.y = -0.5f;
                    item->state = 0;
                }
            }
            else
            {
                item->startPosition.x = 0.0f;
                item->startPosition.z = 0.0f;
                if (item->startPosition.y < -2.2f)
                {
                    item->startPosition.y = -2.2f;
                }
            }
        }
        item->currentPosition += item->startPosition * g_Supervisor.effectiveFramerateMultiplier;
        if (g_GameManager.arcadeRegionSize.y + 16.0f <= item->currentPosition.y)
        {
            item->isInUse = 0;
            g_GameManager.DecreaseSubrank(3);
            continue;
        }
        if (item->startPosition.y < 3.0f)
        {
            item->startPosition.y += 0.03f * g_Supervisor.effectiveFramerateMultiplier;
        }
        else
        {
            item->startPosition.y = 3.0f;
        }
    check_collision:
        if (g_Player.CalcItemBoxCollision(&item->currentPosition, &local_20))
        {
            g_ReplayManager->replayEventFlags |= 0x40;
            switch (item->itemType)
            {
            case ITEM_POWER_SMALL:
                if ((i32)g_GameManager.globals->currentPower >= 128)
                {
                    g_GameManager.powerItemCountForScore++;
                    if ((u32)g_GameManager.powerItemCountForScore >= 31)
                    {
                        g_GameManager.powerItemCountForScore = 30;
                    }
                    itemScore = g_CherryBonusFullPower[g_GameManager.powerItemCountForScore];
                    g_GameManager.AddScore(itemScore);
                    g_AsciiManager.CreatePopup1(&item->currentPosition, itemScore, itemScore >= 12800 ? 0xffffff00 : 0xffffffff);
                }
                else
                {
                    j = 0;
                    while ((i32)g_GameManager.globals->currentPower >= g_PowerLevels[j])
                    {
                        j++;
                    }
                    prevPowerIdx = j;
                    g_GameManager.powerItemCountForScore = 0;
                    g_GameManager.AddCurrentPower(1);
                    if ((i32)g_GameManager.globals->currentPower >= 128)
                    {
                        g_GameManager.globals->currentPower = 128.0f;
                        g_GameManager.RegenerateGameIntegrityCsum();
                        if (!g_EnemyManager.spellcardInfo.isActive)
                        {
                            g_BulletManager.RemoveAllBullets(1);
                        }
                        g_Gui.ShowFullPowerMode(0, 1);
                        this->DespawnAllItems(i);
                    }
                    g_GameManager.AddScore(10);
                    g_Gui.flags = (g_Gui.flags & 0xffffffcf) | 0x20;
                    while ((i32)g_GameManager.globals->currentPower >= g_PowerLevels[j])
                    {
                        j++;
                    }
                    if (j != prevPowerIdx)
                    {
                        g_AsciiManager.CreatePopup1(&item->currentPosition, -1, 0xffffc0a0);
                        g_SoundPlayer.PlaySoundByIdx(SOUND_POWERUP, 0);
                    }
                    else
                    {
                        g_AsciiManager.CreatePopup1(&item->currentPosition, 10, 0xffffffff);
                    }
                }
                g_GameManager.IncreaseSubrank(1);
                break;
            case ITEM_POINT:
                itemScore =
                    item->IsBelowPoc() ? 50000
                                       : 50000 - item->OffsetFromPoc() * 100;
                if (item->autoCollect == 1)
                {
                    itemScore = 50000;
                }
                if (itemScore >= 50000)
                {
                    if (g_GameManager.cherry - g_GameManager.globals->cherryStart > 50000)
                    {
                        itemScore = g_GameManager.cherry - g_GameManager.globals->cherryStart;
                    }
                }
                else if (g_GameManager.cherry - g_GameManager.globals->cherryStart > 50000)
                {
                    itemScore += (g_GameManager.cherry - g_GameManager.globals->cherryStart - 50000) / 5;
                }
                itemScore -= itemScore % 10;
                g_AsciiManager.CreatePopup1(&item->currentPosition, itemScore, (item->currentPosition.y < g_Player.shooterData->pocY || item->autoCollect == 1) ? 0xffffff00 : 0xffffffff);
                g_GameManager.AddScore(itemScore);
                g_GameManager.globals->pointItemsCollectedThisStage++;
                g_GameManager.globals->pointItemsCollectedForExtend++;
                g_Gui.flags = (g_Gui.flags & 0xfffffcff) | 0x200;
                if (item->currentPosition.y < 128.0f)
                {
                    g_GameManager.IncreaseSubrank(10);
                }
                else
                {
                    g_GameManager.IncreaseSubrank(3);
                }
                if (g_GameManager.globals->extendsFromPointItems >= 0)
                {
                    for (;;)
                    {
                        if (g_GameManager.difficulty < 4)
                        {
                            if (g_GameManager.globals->extendsFromPointItems < 3)
                            {
                                g_GameManager.globals->nextNeededPointItemsForExtend = g_GameManager.globals->extendsFromPointItems * 75 + 50;
                            }
                            else if (g_GameManager.globals->extendsFromPointItems < 5)
                            {
                                g_GameManager.globals->nextNeededPointItemsForExtend = (g_GameManager.globals->extendsFromPointItems - 3) * 150 + 300;
                            }
                            else
                            {
                                g_GameManager.globals->nextNeededPointItemsForExtend = (g_GameManager.globals->extendsFromPointItems - 5) * 200 + 800;
                            }
                        }
                        else if (g_GameManager.globals->extendsFromPointItems == 0)
                        {
                            g_GameManager.globals->nextNeededPointItemsForExtend = 200;
                        }
                        else if (g_GameManager.globals->extendsFromPointItems == 1)
                        {
                            g_GameManager.globals->nextNeededPointItemsForExtend = 500;
                        }
                        else
                        {
                            g_GameManager.globals->nextNeededPointItemsForExtend = (g_GameManager.globals->extendsFromPointItems - 2) * 500 + 800;
                        }

                        if (g_GameManager.globals->pointItemsCollectedForExtend >= g_GameManager.globals->nextNeededPointItemsForExtend)
                        {
                            g_GameManager.ExtendFromPoints();
                            g_GameManager.globals->extendsFromPointItems++;
                            continue;
                        }
                        break;
                    }
                }
                break;
            case ITEM_POWER_BIG:
                if ((i32)g_GameManager.globals->currentPower >= 128)
                {
                    g_AsciiManager.CreatePopup1(&item->currentPosition, itemScore, itemScore >= 1000 ? 0xffffff00 : 0xffffffff);
                }
                else
                {
                    k = 0;
                    while ((i32)g_GameManager.globals->currentPower >= g_PowerLevels[k])
                    {
                        k++;
                    }
                    prevPowerLevel2 = k;
                    g_GameManager.AddCurrentPower(8);
                    if ((i32)g_GameManager.globals->currentPower >= 128)
                    {
                        g_GameManager.globals->currentPower = 128.0f;
                        g_GameManager.RegenerateGameIntegrityCsum();
                        if (!g_EnemyManager.spellcardInfo.isActive)
                        {
                            g_BulletManager.RemoveAllBullets(1);
                        }
                        g_Gui.ShowFullPowerMode(0, 1);
                        this->DespawnAllItems(i);
                    }
                    g_Gui.flags = (g_Gui.flags & 0xffffffcf) | 0x20;
                    g_GameManager.AddScore(10);
                    while ((i32)g_GameManager.globals->currentPower >= g_PowerLevels[k])
                    {
                        k++;
                    }
                    if (k != prevPowerLevel2)
                    {
                        g_AsciiManager.CreatePopup1(&item->currentPosition, -1, 0xffffc0a0);
                        g_SoundPlayer.PlaySoundByIdx(SOUND_POWERUP, 0);
                    }
                    else
                    {
                        g_AsciiManager.CreatePopup1(&item->currentPosition, 10, 0xffffffff);
                    }
                }
                break;
            case ITEM_BOMB:
                if ((i32)g_GameManager.globals->bombsRemaining < 8)
                {
                    g_GameManager.AddBombsRemaining(1);
                    g_Gui.flags = (g_Gui.flags & 0xfffffff3) | 8;
                }
                g_GameManager.IncreaseSubrank(5);
                break;
            case ITEM_LIFE:
                g_GameManager.ExtendFromPoints();
                break;
            case ITEM_FULL_POWER:
                if ((i32)g_GameManager.globals->currentPower < 128)
                {
                    g_BulletManager.RemoveAllBullets(1);
                    g_Gui.ShowFullPowerMode(0, 1);
                    g_SoundPlayer.PlaySoundByIdx(SOUND_POWERUP, 0);
                    g_AsciiManager.CreatePopup1(&item->currentPosition, -1, 0xffffc0a0);
                    this->DespawnAllItems(i);
                }
                g_GameManager.globals->currentPower = 128.0f;
                g_GameManager.RegenerateGameIntegrityCsum();
                g_GameManager.AddScore(1000);
                g_AsciiManager.CreatePopup1(&item->currentPosition, 1000, 0xffffffff);
                g_Gui.flags = (g_Gui.flags & 0xffffffcf) | 0x20;
                break;
            case ITEM_POINT_BULLET:
                if (!g_Player.isBombing)
                {
                    itemScore = (g_GameManager.globals->grazeInTotal / 40) * 10 + 300;
                    if (itemScore <= 0)
                    {
                        itemScore = 10;
                    }
                }
                else
                {
                    itemScore = 100;
                }
                g_AsciiManager.CreatePopup2(&item->currentPosition, itemScore, -1);
                g_GameManager.AddScore(itemScore);
                if (!g_Player.bombInfo.isInUse)
                {
                    g_GameManager.AddCherryPlus(20);
                }
                else if ((i & 1) == 0)
                {
                    g_GameManager.AddCherryPlus(10);
                }
                else
                {
                    g_GameManager.AddCherry(10);
                }
                break;
            case ITEM_CHERRY_SMALL:
                g_GameManager.AddCherryPlus(30);
                g_GameManager.AddCherry(70);
                break;
            case ITEM_CHERRY:
                if (g_GameManager.IsCherryAtMax())
                {
                    itemScore = item->ShouldAwardMaxScore()
                                    ? 50000
                                    : 50000 - item->OffsetFromPoc() * 100;
                    itemScore -= itemScore % 10;
                    g_AsciiManager.CreatePopup1(&item->currentPosition, itemScore, (item->currentPosition.y < g_Player.shooterData->pocY || item->autoCollect != 0) ? 0xffffff00 : 0xffffffff);
                    g_GameManager.AddScore(itemScore);
                }
                itemScore = 1000;
                itemScore += g_GameManager.globals->spellCardsCaptured * 100;
                if (!g_GameManager.IsCherryAtMax())
                {
                    g_AsciiManager.CreatePopup1(&item->currentPosition, itemScore, 0xffff4040);
                }
                g_GameManager.AddCherryPlus(itemScore);
                break;
            case ITEM_STAR:
                itemScore = (g_GameManager.globals->grazeInTotal / 40) * 10 + 300;
                if (itemScore <= 0)
                {
                    itemScore = 10;
                }
                if (g_GameManager.IsCherryAtMax())
                {
                    g_AsciiManager.CreatePopup1(&item->currentPosition, itemScore, 0xffffffff);
                }
                g_GameManager.AddScore(itemScore);
                itemScore = 100;
                if (!g_GameManager.IsCherryAtMax())
                {
                    g_AsciiManager.CreatePopup1(&item->currentPosition, itemScore, 0xffff4040);
                }
                g_GameManager.AddCherryPlus(itemScore);
                break;
            }
            item->isInUse = 0;
            itemAcquired = 1;
            continue;
        }
        else
        {
            item->timer++;
            if (item->sprite.currentInstruction != NULL)
            {
                g_AnmManager->ExecuteScript(&item->sprite);
            }
            this->listTail->next = item;
            item->next = NULL;
            this->listTail = item;
        }
    }
    if (itemAcquired)
    {
        g_SoundPlayer.PlaySoundByIdx(SOUND_21, 0);
    }
}

#pragma var_order(i, item)
// FUNCTION: TH07 0x00433a90
void ItemManager::RemoveAllItems()
{
    Item *item;
    i32 i;

    item = this->items;
    for (i = 0; i < 0x44c; i++, item++)
    {
        if (item->isInUse == 0)
        {
            continue;
        }

        item->state = 1;
        item->startPosition = D3DXVECTOR3(0.0f, -0.5f, 0.0f);
    }
}

#pragma var_order(i, item)
// FUNCTION: TH07 0x00433b20
void ItemManager::DespawnAllItems(i32 param_1)
{
    Item *item;
    i32 i;

    item = this->items;
    for (i = 0; i < 0x44c; i++, item++)
    {
        if (item->isInUse == 0 || i == param_1)
        {
            continue;
        }

        if (item->itemType == 0 || item->itemType == 2)
        {
            if (item->startPosition.y > -0.5f)
            {
                item->startPosition.x = 0.0f;
                item->startPosition.y = -0.5f;
                item->startPosition.z = 0.0f;
            }
            g_EffectManager.SpawnParticles(0, &item->currentPosition, 1, 0xffffffff);
            item->itemType = 7;
            g_AnmManager->SetAnmIdxAndExecuteScript(&item->sprite, 0x2cb);
        }
    }
}

#pragma var_order(i, item)
// FUNCTION: TH07 0x00433c40
void ItemManager::ActivateAllItems()
{
    Item *item;
    i32 i;

    item = this->items;
    for (i = 0; i < 0x44c; i++, item++)
    {
        if (item->isInUse != 1)
        {
            continue;
        }

        if (item->state == 1)
        {
            item->state = 0;
            item->startPosition.x = 0.0f;
            item->startPosition.y = -0.9f;
            item->startPosition.z = 0.0f;
        }
    }
}

#pragma var_order(local_8, item)
// FUNCTION: TH07 0x00433cd0
void ItemManager::OnDraw()
{
    Item *item;
    i32 local_8;

    item = this->listHead.next;
    while (item != NULL)
    {
        item->sprite.pos.x =
            g_GameManager.arcadeRegionTopLeftPos.x + item->currentPosition.x;
        item->sprite.pos.y =
            g_GameManager.arcadeRegionTopLeftPos.y + item->currentPosition.y;
        item->sprite.pos.z = 0.01f;
        if (item->currentPosition.y < -8.0f)
        {
            item->sprite.pos.y = 8.0f + g_GameManager.arcadeRegionTopLeftPos.y;
            if (item->isArrowSprite != 0)
            {
                g_AnmManager->SetActiveSprite(&item->sprite, item->itemType + 0x2b6);
                item->isArrowSprite = 0;
                item->sprite.zWriteDisable = 1;
            }
            local_8 = 255 - (i32)((8.0f - item->currentPosition.y) * 255.0f / 128.0f);
            if (local_8 < 0x40)
            {
                local_8 = 0x40;
            }
            item->sprite.color.color =
                (item->sprite.color.color & 0xffffff) | local_8 << 0x18;
        }
        else
        {
            if (item->isArrowSprite == 0)
            {
                g_AnmManager->SetActiveSprite(&item->sprite, item->itemType + 0x2ac);
                item->isArrowSprite = 1;
                item->sprite.color.color = 0xffffffff;
                item->sprite.zWriteDisable = 1;
            }
        }
        g_AnmManager->Draw(&item->sprite);
        item = item->next;
    }
}
