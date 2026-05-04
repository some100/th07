#pragma once

#include <d3d8.h>

#include "AnmVm.hpp"
#include "ZunResult.hpp"
#include "ZunTimer.hpp"
#include "inttypes.hpp"

struct RetryMenu
{
    void OnDraw();
    i32 OnUpdate();

    i32 curState;
    i32 numFrames;
    AnmVm menuSprites[10];
    AnmVm menuBackground;
};
C_ASSERT(sizeof(RetryMenu) == 0x194c);

struct PauseMenu
{
    i32 OnUpdate();
    void OnDraw();

    i32 curState;
    i32 numFrames;
    AnmVm menuSprites[6];
    AnmVm menuBackground;
};
C_ASSERT(sizeof(PauseMenu) == 0x101c);

struct AsciiManagerPopup
{
    char digits[8];
    D3DXVECTOR3 position;
    D3DCOLOR color;
    ZunTimer timer;
    u8 inUse;
    u8 characterCount;
    // pad 2
};
C_ASSERT(sizeof(AsciiManagerPopup) == 0x28);

struct AsciiManagerString
{
    char text[64];
    D3DXVECTOR3 position;
    D3DCOLOR color;
    D3DXVECTOR2 scale;
    i32 isSelected;
    i32 isGui;
};

struct AsciiManager
{
    static ZunResult RegisterChain();
    static void CutChain();

    static ZunResult AddedCallback(AsciiManager *arg);
    static ZunResult DeletedCallback(AsciiManager *arg);
    static u32 OnUpdate(AsciiManager *arg);
    static u32 OnDrawMenus(AsciiManager *arg);
    static u32 OnDrawPopups(AsciiManager *arg);

    static void AddFormatText(AsciiManager *manager, D3DXVECTOR3 *position,
                              const char *fmt, ...);
    void AddString(D3DXVECTOR3 *param_1, const char *text);
    void CreatePopup1(D3DXVECTOR3 *position, i32 value, D3DCOLOR color);
    void CreatePopup2(D3DXVECTOR3 *position, i32 value, D3DCOLOR color);
    void DrawPopups();
    void DrawStrings();
    void InitializeVms();
    void InitializeOtherVms();
    void UpdateScripts();

    void SetColor(D3DCOLOR color)
    {
        this->color = color;
    }

    void SetFadeState(i32 fadeState)
    {
        this->otherVms[0].pendingInterrupt = fadeState;
        this->uiFadeState = fadeState;
    }

    i32 GetFadeState()
    {
        return this->uiFadeState;
    }

    AnmVm vm0;
    AnmVm vm1;
    AnmVm otherVms[3];
    AnmVm otherOtherVms[4];
    i32 bossDamageTint[4];
    AsciiManagerString strings[256];
    i32 numStrings;
    D3DCOLOR color;
    D3DXVECTOR2 scale;
    i32 isGui;
    i32 isSelected;
    i32 uiFadeState;
    i32 fontSpacing;
    i32 nextPopupIndex1;
    i32 nextPopupIndex2;
    i32 unused_74e4;
    RetryMenu retryMenu;
    PauseMenu pauseMenu;
    AnmVm vm;
    AsciiManagerPopup popups[723];
};
C_ASSERT(sizeof(AsciiManager) == 0x11194);
extern AsciiManager g_AsciiManager;
