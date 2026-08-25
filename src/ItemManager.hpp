#pragma once

#include "AnmVm.hpp"
#include "Player.hpp"

extern u8 g_ItemDropTable[32];

enum ItemType
{
    ITEM_POWER_SMALL,
    ITEM_POINT,
    ITEM_POWER_BIG,
    ITEM_BOMB,
    ITEM_FULL_POWER,
    ITEM_LIFE,
    ITEM_POINT_BULLET,
    ITEM_CHERRY,
    ITEM_CHERRY_SMALL,
    ITEM_STAR,
    ITEM_NO_ITEM = 255,
};

struct Item
{
    Item();

    i32 IsBelowPoc()
    {
        return this->pos.y < g_Player.shooterData->pocY;
    }

    i32 OffsetFromPoc()
    {
        return this->pos.y - g_Player.shooterData->pocY;
    }

    i32 ShouldAwardMaxScore()
    {
        return this->pos.y < g_Player.shooterData->pocY || this->autoCollect;
    }

    AnmVm sprite;
    ZunVec3 pos;
    ZunVec3 prevPos;
    ZunVec3 velocity;
    ZunVec3 targetPos;
    ZunTimer timer;
    i8 itemType;
    i8 isInUse;
    i8 isOnscreen;
    i8 state;
    i8 autoCollect;
    // pad 3
    struct Item *next;
};

#define MAX_ITEMS 1100

struct ItemManager
{
    ItemManager();

    void ActivateAllItems();
    void DespawnAllItems(i32 param_1);
    void OnUpdate();
    void OnDraw();
    void RemoveAllItems();
    Item *SpawnItem(ZunVec3 *heading, i32 itemType, i32 state);

    Item items[MAX_ITEMS + 1];
    i32 nextIndex;
    i32 activeItemCount;
    Item listHead;
    Item *listTail;
};
extern ItemManager g_ItemManager;
