#pragma once

#include <d3d8.h>

#include "AnmVm.hpp"
#include "ZunResult.hpp"
#include "ZunTimer.hpp"
#include "inttypes.hpp"

struct PauseMenu
{
    PauseMenu();

    void OnDraw();
    i32 OnUpdate();

    i32 curState;
    i32 numFrames;
    AnmVm menuSprites[10];
    AnmVm menuBackground;
};
C_ASSERT(sizeof(PauseMenu) == 0x194c);

struct RetryMenu
{
    RetryMenu();

    i32 OnUpdate();
    void OnDraw();

    i32 curState;
    i32 numFrames;
    AnmVm menuSprites[6];
    AnmVm menuBackground;
};
C_ASSERT(sizeof(RetryMenu) == 0x101c);

struct AsciiManagerPopup
{
    u8 digits[8];
    Float3 pos;
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

    // This should seriously just be a normal Float3, but for some reason if
    // a Float3 is used here instead the AsciiManager constructor opts to
    // construct popups with a vector constructor iterator rather than just
    // doing it in a loop, which ruins the matching percentage even more than
    // it already is.
    PodFloat3 pos;
    D3DCOLOR color;
    Float2 scale;
    i32 isSelected;
    i32 isGui;
};

struct AsciiManager
{
    AsciiManager();

    static ZunResult RegisterChain();
    static void CutChain();

    static ZunResult AddedCallback(AsciiManager *arg);
    static ZunResult DeletedCallback(AsciiManager *arg);
    static u32 OnUpdate(AsciiManager *arg);
    static u32 OnDrawMenus(AsciiManager *arg);
    static u32 OnDrawPopups(AsciiManager *arg);

    static void AddFormatText(AsciiManager *manager, Float3 *pos,
                              const char *fmt, ...);
    void AddString(Float3 *pos, const char *text);
    void CreatePopup1(Float3 *pos, i32 value, D3DCOLOR color);
    void CreatePopup2(Float3 *pos, i32 value, D3DCOLOR color);
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
        this->cherryGauge.pendingInterrupt = fadeState;
        this->uiFadeState = fadeState;
    }

    i32 GetFadeState()
    {
        return this->uiFadeState;
    }

    AnmVm *GetBossMarker(i32 idx)
    {
        return &this->bossMarkers[idx];
    }

    void SetBossMarkerPos(i32 idx, Float3 *pos)
    {
        this->bossMarkers[idx].pos = *pos;
    }

    void SetBossDamageTint(i32 idx, D3DCOLOR color)
    {
        this->bossDamageTint[idx] = color;
    }

    void SetBossMarkerInterrupt(i32 idx, i32 interrupt)
    {
        this->bossMarkers[idx].pendingInterrupt = interrupt;
    }

    AnmVm vm0;
    AnmVm vm1;
    AnmVm cherryGauge;
    AnmVm cherryDigit;
    AnmVm cherryBorderActive;
    AnmVm bossMarkers[4];
    i32 bossDamageTint[4];
    AsciiManagerString strings[256];
    i32 numStrings;
    D3DCOLOR color;
    Float2 scale;
    i32 isGui;
    i32 isSelected;
    i32 uiFadeState;
    i32 fontSpacing;
    i32 nextPopupIndex1;
    i32 nextPopupIndex2;
    i32 unused_74e4;
    PauseMenu pauseMenu;
    RetryMenu retryMenu;
    AnmVm vm;
    AsciiManagerPopup popups[723];
};
C_ASSERT(sizeof(AsciiManager) == 0x11194);
extern AsciiManager g_AsciiManager;
