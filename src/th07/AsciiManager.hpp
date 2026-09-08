#pragma once

#include <d3d8.h>

#include "AnmManager.hpp"
#include "ZunResult.hpp"
#include "ZunTimer.hpp"
#include "inttypes.hpp"

enum PauseMenuState
{
    PAUSE_MENU_STATE_INIT,
    PAUSE_MENU_STATE_SELECTING_UNPAUSE,
    PAUSE_MENU_STATE_SELECTING_RETURN,
    PAUSE_MENU_STATE_SELECTING_RESTART,
    PAUSE_MENU_STATE_UNPAUSING,
    PAUSE_MENU_STATE_CONFIRM_RETURN_SELECTING_YES,
    PAUSE_MENU_STATE_CONFIRM_RETURN_SELECTING_NO,
    PAUSE_MENU_STATE_CONFIRM_RESTART_SELECTING_YES,
    PAUSE_MENU_STATE_CONFIRM_RESTART_SELECTING_NO,
    PAUSE_MENU_STATE_RETURN_TO_MENU,
    PAUSE_MENU_STATE_RESTART_STAGE,
};

enum RetryMenuState
{
    RETRY_MENU_STATE_INIT,
    RETRY_MENU_STATE_SELECTING_CONTINUE,
    RETRY_MENU_STATE_SELECTING_RETURN,
    RETRY_MENU_STATE_CONTINUE_GAME,
    RETRY_MENU_STATE_RETURN_TO_MENU,
};

struct PauseMenu
{
    void OnDraw();
    i32 OnUpdate();

    i32 curState;
    i32 numFrames;
    AnmVm menuSprites[10];
    AnmVm menuBackground;
};
C_ASSERT(sizeof(PauseMenu) == 0x194c);

#define RETRY_MENU_SPRITES 5

struct RetryMenu
{
    i32 OnUpdate();
    void OnDraw();

    i32 curState;
    i32 numFrames;
    AnmVm menuSprites[RETRY_MENU_SPRITES + 1];
    AnmVm menuBackground;
};
C_ASSERT(sizeof(RetryMenu) == 0x101c);

struct AsciiManagerPopup
{
    u8 digits[8];
    Float3 pos;
    D3DCOLOR color;
    ZunTimer timer;
    u8 isInUse;
    u8 characterCount;
    // pad 2
};
C_ASSERT(sizeof(AsciiManagerPopup) == 0x28);

struct AsciiManagerString
{
    char text[64];
    Float3 pos;
    D3DCOLOR color;
    Float2 scale;
    ZunBool isSelected;
    ZunBool isGui;
};

#define MAX_POPUP1 720
#define MAX_POPUP2 3

struct AsciiManager
{
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

    void UpdateScripts()
    {
        g_AnmManager->ExecuteScript(&this->cherryGauge);
        g_AnmManager->ExecuteScript(&this->cherryDigit);
        g_AnmManager->ExecuteScript(&this->bossMarkers[0]);
        g_AnmManager->ExecuteScript(&this->bossMarkers[1]);
        g_AnmManager->ExecuteScript(&this->bossMarkers[2]);
        g_AnmManager->ExecuteScript(&this->bossMarkers[3]);
        g_AnmManager->ExecuteScript(&this->cherryBorderActive);
    }

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

    AnmVm smallScorePopupVm;
    AnmVm largeTextVm;
    AnmVm cherryGauge;
    AnmVm cherryDigit;
    AnmVm cherryBorderActive;
    AnmVm bossMarkers[4];
    i32 bossDamageTint[4];
    AsciiManagerString strings[256];
    i32 numStrings;
    D3DCOLOR color;
    Float2 scale;
    ZunBool isGui;
    ZunBool isSelected;
    i32 uiFadeState;
    i32 fontSpacing;
    i32 nextPopupIndex1;
    i32 nextPopupIndex2;
    i32 unused_74e4;
    PauseMenu pauseMenu;
    RetryMenu retryMenu;
    AnmVm vm;
    AsciiManagerPopup popups[MAX_POPUP1 + MAX_POPUP2];
};
C_ASSERT(sizeof(AsciiManager) == 0x11194);
extern AsciiManager g_AsciiManager;
