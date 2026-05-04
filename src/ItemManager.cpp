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

// FUNCTION: TH07 0x004326f0
void ItemManager::SpawnItem(D3DXVECTOR3 *heading, i32 itemType, i32 state)
{
    i16 local_14;
    Item *item;

    item = this->items + this->nextIndex;
    if ((127 < (i32)g_GameManager.globals->currentPower) &&
        (itemType == ITEM_POWER_SMALL || (itemType == ITEM_POWER_BIG)))
    {
        itemType = ITEM_CHERRY;
    }
    for (i32 i = 0; i <= 1099; ++i)
    {
        this->nextIndex = this->nextIndex + 1;
        if (item->isInUse == 0)
        {
            break;
        }
        if (this->nextIndex < 0x44c)
        {
            item = item + 1;
        }
        else
        {
            this->nextIndex = 0;
            item = this->items;
        }
        i++;
    }
    if (1099 < this->nextIndex)
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
        item->startPosition.x = item->currentPosition.x;
        item->startPosition.y = item->currentPosition.y;
        item->startPosition.z = item->currentPosition.z;
    }
    else if (state == 3)
    {
        item->state = 1;
    }
    else if (state == 4)
    {
        item->state = 0;
    }
    local_14 = (i16)itemType + 0x2c4;
    item->sprite.anmFileIdx = local_14;
    g_AnmManager->SetAndExecuteScript(&item->sprite,
                                      g_AnmManager->scripts[itemType + 0x2c4]);
    item->sprite.color.color = 0xffffffff;
    item->sprite.zWriteDisable = 1;
    item->autoCollect = 0;
    item->isArrowSprite = 1;
}

// FUNCTION: TH07 0x00432990
void ItemManager::OnUpdate()
{
    bool bVar6;
    i32 iVar8;
    i32 local_f0;
    i32 local_ec;
    i32 local_e4;
    i32 local_e0;
    i32 local_34;
    i32 local_2c;
    Item *item;
    D3DXVECTOR3 local_20;
    f32 local_14;
    i32 local_10;
    f32 local_c;
    u32 i;

    local_20.y = g_Player.shooterData->itemCollectRadius;
    local_20.x = g_Player.shooterData->itemCollectRadius;
    local_20.z = 16.0f;
    bVar6 = false;
    this->activeItemCount = 0;
    this->listTail = &this->listHead;
    (this->listHead).next = NULL;
    i = 0;
    item = this->items;
    while (true)
    {
        if (1099 < (i32)i)
        {
            if (bVar6)
            {
                g_SoundPlayer.PlaySoundByIdx(SOUND_21, 0);
            }
            return;
        }
        if (item->isInUse != 0)
        {
            this->activeItemCount = this->activeItemCount + 1;
            if (item->state == 2)
            {
                if (item->timer > 60)
                {
                    if (item->timer == 60)
                    {
                        item->startPosition.x = 0.0f;
                        item->startPosition.y = 0.0f;
                        item->startPosition.z = 0.0f;
                        item->state = 0;
                    }
                    goto LAB_00432d47;
                }
                local_c = ((f32)item->timer.current + item->timer.subFrame) / 60.0f;
                item->currentPosition = local_c * item->targetPosition +
                                        (1.0f - local_c) * item->startPosition;
            }
            else
            {
                if ((item->state == 1) ||
                    // double intentionally used here
                    (((128.0 <= (f64)(i32)g_GameManager.globals->currentPower ||
                       (3 < g_GameManager.difficulty)) &&
                      (g_Player.positionCenter.y < g_Player.shooterData->pocY)) ||
                     (g_Player.hasBorder == BORDER_ACTIVE)))
                {
                    if (g_Player.playerState == PLAYER_STATE_SPAWNING)
                    {
                        item->startPosition.y = -0.5f;
                        item->state = 0;
                    }
                    else
                    {
                        local_14 = g_Player.AngleToPlayer(&item->currentPosition);
                        AngleToVector(&item->startPosition, local_14,
                                      g_Player.shooterData->itemCollectSpeed);
                        item->state = 1;
                        if (g_Player.hasBorder == BORDER_ACTIVE)
                        {
                            item->autoCollect = 1;
                        }
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
            LAB_00432d47:
                item->currentPosition +=
                    g_Supervisor.effectiveFramerateMultiplier * item->startPosition;
                if (g_GameManager.arcadeRegionSize.y + 16.0f <
                    item->currentPosition.y)
                {
                    item->isInUse = 0;
                    g_GameManager.DecreaseSubrank(3);
                    goto LAB_00432a1b;
                }
                if (3.0f <= item->startPosition.y)
                {
                    item->startPosition.y = 3.0f;
                }
                else
                {
                    item->startPosition.y =
                        g_Supervisor.effectiveFramerateMultiplier * 0.03f +
                        item->startPosition.y;
                }
            }
            if (g_Player.CalcItemBoxCollision(&item->currentPosition, &local_20) ==
                0)
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
            else
            {
                g_ReplayManager->replayEventFlags |= 0x40;
                switch (item->itemType)
                {
                case ITEM_POWER_SMALL:
                    if (g_GameManager.globals->currentPower < 128)
                    {
                        for (local_2c = 0; iVar8 = local_2c,
                            g_PowerLevels[local_2c] <= g_GameManager.globals->currentPower;
                             local_2c += 1)
                        {
                        }
                        g_GameManager.powerItemCountForScore = 0;
                        g_GameManager.AddCurrentPower(1);
                        if (0x7f < g_GameManager.globals->currentPower)
                        {
                            g_GameManager.globals->currentPower = 128.0f;
                            g_GameManager.RegenerateGameIntegrityCsum();
                            if (g_EnemyManager.spellcardInfo.isActive == 0)
                            {
                                g_BulletManager.RemoveAllBullets(1);
                            }
                            g_Gui.ShowFullPowerMode(0, 1);
                            DespawnAllItems(i);
                        }
                        g_GameManager.globals->score = g_GameManager.globals->score + 1;
                        g_Gui.flags = (g_Gui.flags & 0xffffffcf) | 0x20;
                        while (g_PowerLevels[local_2c] <=
                               g_GameManager.globals->currentPower)
                        {
                            local_2c += 1;
                        }
                        if (local_2c == iVar8)
                        {
                            g_AsciiManager.CreatePopup1(&item->currentPosition, 10,
                                                        0xffffffff);
                        }
                        else
                        {
                            g_AsciiManager.CreatePopup1(&item->currentPosition, -1,
                                                        0xffffc0a0);
                            g_SoundPlayer.PlaySoundByIdx(SOUND_POWERUP, 0);
                        }
                    }
                    else
                    {
                        g_GameManager.powerItemCountForScore += 1;
                        if (0x1e < g_GameManager.powerItemCountForScore)
                        {
                            g_GameManager.powerItemCountForScore = 0x1e;
                        }
                        local_10 =
                            g_CherryBonusFullPower[g_GameManager.powerItemCountForScore];
                        g_GameManager.globals->score =
                            local_10 / 10 + g_GameManager.globals->score;
                        g_AsciiManager.CreatePopup1(&item->currentPosition, local_10,
                                                    ((local_10 < 0x3200) - 1 & 0xffffff01) -
                                                        1);
                    }
                    g_GameManager.IncreaseSubrank(1);
                    break;
                case ITEM_POINT:
                    if (g_Player.shooterData->pocY <= item->currentPosition.y)
                    {
                        local_e0 =
                            (item->currentPosition.y - g_Player.shooterData->pocY) * -100 +
                            50000;
                    }
                    else
                    {
                        local_e0 = 50000;
                    }
                    local_10 = local_e0;
                    if (item->autoCollect == 1)
                    {
                        local_10 = 50000;
                    }
                    if (local_10 < 50000)
                    {
                        if (50000 <
                            g_GameManager.cherry - g_GameManager.globals->cherryStart)
                        {
                            local_10 +=
                                ((g_GameManager.cherry - g_GameManager.globals->cherryStart) +
                                 -50000) /
                                5;
                        }
                    }
                    else if (50000 < g_GameManager.cherry -
                                         g_GameManager.globals->cherryStart)
                    {
                        local_10 =
                            g_GameManager.cherry - g_GameManager.globals->cherryStart;
                    }
                    local_10 -= local_10 % 10;
                    if ((item->currentPosition.y < g_Player.shooterData->pocY) ||
                        (item->autoCollect == 1))
                    {
                        local_e4 = 0xffffff00;
                    }
                    else
                    {
                        local_e4 = 0xffffffff;
                    }
                    g_AsciiManager.CreatePopup1(&item->currentPosition, local_10,
                                                local_e4);
                    g_GameManager.globals->score =
                        local_10 / 10 + g_GameManager.globals->score;
                    g_GameManager.globals->pointItemsCollectedThisStage =
                        g_GameManager.globals->pointItemsCollectedThisStage + 1;
                    g_GameManager.globals->pointItemsCollectedForExtend =
                        g_GameManager.globals->pointItemsCollectedForExtend + 1;
                    g_Gui.flags = (g_Gui.flags & 0xfffffcff) | 0x200;
                    if (128.0f <= item->currentPosition.y)
                    {
                        g_GameManager.IncreaseSubrank(3);
                    }
                    else
                    {
                        g_GameManager.IncreaseSubrank(10);
                    }
                    if (-1 < g_GameManager.globals->extendsFromPointItems)
                    {
                        while (true)
                        {
                            if (g_GameManager.difficulty < 4)
                            {
                                if (g_GameManager.globals->extendsFromPointItems < 3)
                                {
                                    g_GameManager.globals->nextNeededPointItemsForExtend =
                                        g_GameManager.globals->extendsFromPointItems * 0x4b +
                                        0x32;
                                }
                                else if (g_GameManager.globals->extendsFromPointItems < 5)
                                {
                                    g_GameManager.globals->nextNeededPointItemsForExtend =
                                        (g_GameManager.globals->extendsFromPointItems - 3) *
                                            0x96 +
                                        300;
                                }
                                else
                                {
                                    g_GameManager.globals->nextNeededPointItemsForExtend =
                                        (g_GameManager.globals->extendsFromPointItems - 5) * 200 +
                                        800;
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
                                g_GameManager.globals->nextNeededPointItemsForExtend =
                                    (g_GameManager.globals->extendsFromPointItems - 2) * 500 +
                                    800;
                            }
                            if (g_GameManager.globals->pointItemsCollectedForExtend <
                                g_GameManager.globals->nextNeededPointItemsForExtend)
                            {
                                break;
                            }
                            g_GameManager.ExtendFromPoints();
                            g_GameManager.globals->extendsFromPointItems =
                                g_GameManager.globals->extendsFromPointItems + 1;
                        }
                    }
                    break;
                case ITEM_POWER_BIG:
                    if (g_GameManager.globals->currentPower < 0x80)
                    {
                        for (local_34 = 0; iVar8 = local_34,
                            g_PowerLevels[local_34] <= g_GameManager.globals->currentPower;
                             local_34 += 1)
                        {
                        }
                        g_GameManager.AddCurrentPower(8);
                        if (0x7f < (i32)g_GameManager.globals->currentPower)
                        {
                            g_GameManager.globals->currentPower = 128.0f;
                            g_GameManager.RegenerateGameIntegrityCsum();
                            if (g_EnemyManager.spellcardInfo.isActive == 0)
                            {
                                g_BulletManager.RemoveAllBullets(1);
                            }
                            g_Gui.ShowFullPowerMode(0, 1);
                            DespawnAllItems(i);
                        }
                        g_Gui.flags = (g_Gui.flags & 0xffffffcf) | 0x20;
                        g_GameManager.globals->score = g_GameManager.globals->score + 1;
                        while (g_PowerLevels[local_34] <=
                               g_GameManager.globals->currentPower)
                        {
                            local_34 += 1;
                        }
                        if (local_34 == iVar8)
                        {
                            g_AsciiManager.CreatePopup1(&item->currentPosition, 10,
                                                        0xffffffff);
                        }
                        else
                        {
                            g_AsciiManager.CreatePopup1(&item->currentPosition, -1,
                                                        0xffffc0a0);
                            g_SoundPlayer.PlaySoundByIdx(SOUND_POWERUP, 0);
                        }
                    }
                    else
                    {
                        g_AsciiManager.CreatePopup1(&item->currentPosition, local_10,
                                                    ((local_10 < 1000) - 1 & 0xffffff01) -
                                                        1);
                    }
                    break;
                case ITEM_BOMB:
                    if (g_GameManager.globals->bombsRemaining < 8)
                    {
                        g_GameManager.AddBombsRemaining(1);
                        g_Gui.flags = (g_Gui.flags & 0xfffffff3) | 8;
                    }
                    g_GameManager.IncreaseSubrank(5);
                    break;
                case ITEM_FULL_POWER:
                    if (g_GameManager.globals->currentPower < 128)
                    {
                        g_BulletManager.RemoveAllBullets(1);
                        g_Gui.ShowFullPowerMode(0, 1);
                        g_SoundPlayer.PlaySoundByIdx(SOUND_POWERUP, 0);
                        g_AsciiManager.CreatePopup1(&item->currentPosition, -1, 0xffffc0a0);
                        DespawnAllItems(i);
                    }
                    g_GameManager.globals->currentPower = 128.0f;
                    g_GameManager.RegenerateGameIntegrityCsum();
                    g_GameManager.globals->score = g_GameManager.globals->score + 100;
                    g_AsciiManager.CreatePopup1(&item->currentPosition, 1000, 0xffffffff);
                    g_Gui.flags = (g_Gui.flags & 0xffffffcf) | 0x20;
                    break;
                case ITEM_LIFE:
                    g_GameManager.ExtendFromPoints();
                    break;
                case ITEM_POINT_BULLET:
                    if (g_Player.isBombing == 0)
                    {
                        local_10 = (g_GameManager.globals->grazeInTotal / 0x28) * 10 + 300;
                        if (local_10 < 1)
                        {
                            local_10 = 10;
                        }
                    }
                    else
                    {
                        local_10 = 100;
                    }
                    g_AsciiManager.CreatePopup2(&item->currentPosition, local_10,
                                                0xffffffff);
                    g_GameManager.globals->score =
                        local_10 / 10 + g_GameManager.globals->score;
                    if (g_Player.bombInfo.isInUse == 0)
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
                case ITEM_CHERRY:
                    if (g_GameManager.cherryMax <= g_GameManager.cherry)
                    {
                        if ((item->currentPosition.y < g_Player.shooterData->pocY) ||
                            (item->autoCollect != 0))
                        {
                            bVar6 = true;
                        }
                        else
                        {
                            bVar6 = false;
                        }
                        if (bVar6)
                        {
                            local_ec = 50000;
                        }
                        else
                        {
                            local_ec =
                                (item->currentPosition.y - g_Player.shooterData->pocY) *
                                    -100 +
                                50000;
                        }
                        local_10 = local_ec - local_ec % 10;
                        if ((item->currentPosition.y < g_Player.shooterData->pocY) ||
                            (item->autoCollect != 0))
                        {
                            local_f0 = 0xffffff00;
                        }
                        else
                        {
                            local_f0 = 0xffffffff;
                        }
                        g_AsciiManager.CreatePopup1(&item->currentPosition, local_10,
                                                    local_f0);
                        g_GameManager.globals->score =
                            local_10 / 10 + g_GameManager.globals->score;
                    }
                    local_10 = g_GameManager.globals->spellCardsCaptured * 100 + 1000;
                    if (g_GameManager.cherry < g_GameManager.cherryMax)
                    {
                        g_AsciiManager.CreatePopup1(&item->currentPosition, local_10,
                                                    0xffff4040);
                    }
                    g_GameManager.AddCherryPlus(local_10);
                    break;
                case ITEM_CHERRY_SMALL:
                    g_GameManager.AddCherryPlus(30);
                    g_GameManager.AddCherry(70);
                    break;
                case ITEM_STAR:
                    local_10 = (g_GameManager.globals->grazeInTotal / 40) * 10 + 300;
                    if (local_10 < 1)
                    {
                        local_10 = 10;
                    }
                    if (g_GameManager.cherryMax <= g_GameManager.cherry)
                    {
                        g_AsciiManager.CreatePopup1(&item->currentPosition, local_10,
                                                    0xffffffff);
                    }
                    g_GameManager.globals->score =
                        local_10 / 10 + g_GameManager.globals->score;
                    local_10 = 100;
                    if (g_GameManager.cherry < g_GameManager.cherryMax)
                    {
                        g_AsciiManager.CreatePopup1(&item->currentPosition, 100,
                                                    0xffff4040);
                    }
                    g_GameManager.AddCherryPlus(local_10);
                }
                item->isInUse = 0;
                bVar6 = true;
            }
        }
    LAB_00432a1b:
        i++;
        item = item + 1;
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

// FUNCTION: TH07 0x00433cd0
void ItemManager::OnDraw()
{
    for (Item *item = (this->listHead).next; item != NULL; item = item->next)
    {
        item->sprite.pos.x =
            g_GameManager.arcadeRegionTopLeftPos.x + item->currentPosition.x;
        item->sprite.pos.y =
            g_GameManager.arcadeRegionTopLeftPos.y + item->currentPosition.y;
        item->sprite.pos.z = 0.01f;
        if (-8.0f <= item->currentPosition.y)
        {
            if (item->isArrowSprite == 0)
            {
                g_AnmManager->SetActiveSprite(&item->sprite, item->itemType + 0x2ac);
                item->isArrowSprite = 1;
                item->sprite.color.color = 0xffffffff;
                item->sprite.zWriteDisable = 1;
            }
        }
        else
        {
            item->sprite.pos.y = g_GameManager.arcadeRegionTopLeftPos.y + 8.0f;
            if (item->isArrowSprite != 0)
            {
                g_AnmManager->SetActiveSprite(&item->sprite, item->itemType + 0x2b6);
                item->isArrowSprite = 0;
                item->sprite.zWriteDisable = 1;
            }
            i32 local_8 = 0xff - ((8.0f - item->currentPosition.y) * 255.0f) / 128.0f;
            if (local_8 < 0x40)
            {
                local_8 = 0x40;
            }
            item->sprite.color.color =
                (item->sprite.color.color & 0xffffff) | local_8 << 0x18;
        }
        g_AnmManager->Draw(&item->sprite);
    }
}
