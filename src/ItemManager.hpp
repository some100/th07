#pragma once

#include "AnmVm.hpp"

extern u8 g_ItemDropTable[32];

void AngleToVector(D3DXVECTOR3 *out, f32 angle, f32 speed);

typedef enum ItemType
{
    ITEM_POWER_SMALL = 0,
    ITEM_POINT = 1,
    ITEM_POWER_BIG = 2,
    ITEM_BOMB = 3,
    ITEM_FULL_POWER = 4,
    ITEM_LIFE = 5,
    ITEM_POINT_BULLET = 6,
    ITEM_CHERRY = 7,
    ITEM_CHERRY_SMALL = 8,
    ITEM_STAR = 9,
    ITEM_NO_ITEM = 255
} ItemType;

struct Item
{
    Item();

    AnmVm sprite;
    D3DXVECTOR3 currentPosition;
    D3DXVECTOR3 startPosition;
    D3DXVECTOR3 targetPosition;
    ZunTimer timer;
    i8 itemType;
    i8 isInUse;
    u8 isArrowSprite;
    i8 state;
    u8 autoCollect;
    // pad 3
    struct Item *next;
};
C_ASSERT(sizeof(Item) == 0x288);

struct ItemManager
{
    ItemManager();

    void ActivateAllItems();
    void DespawnAllItems(i32 param_1);
    void OnUpdate();
    void OnDraw();
    void RemoveAllItems();
    void SpawnItem(D3DXVECTOR3 *heading, i32 itemType, i32 state);

    struct Item items[1101];
    i32 nextIndex;
    i32 activeItemCount;
    struct Item listHead;
    struct Item *listTail;
};
extern ItemManager g_ItemManager;
