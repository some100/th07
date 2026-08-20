#pragma once

#include "AnmVm.hpp"
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
    PauseMenu();

    void OnDraw();
    i32 OnUpdate();

    void UpdatePrev()
    {
        for (i32 i = 0; i < 10; i++)
        {
            menuSprites[i].UpdatePrev();
        }
        menuBackground.UpdatePrev();
    }

    i32 curState;
    i32 numFrames;
    AnmVm menuSprites[10];
    AnmVm menuBackground;
};

#define RETRY_MENU_SPRITES 5

struct RetryMenu
{
    RetryMenu();

    i32 OnUpdate();
    void OnDraw();

    void UpdatePrev()
    {
        for (i32 i = 0; i < 6; i++)
        {
            menuSprites[i].UpdatePrev();
        }
        menuBackground.UpdatePrev();
    }

    i32 curState;
    i32 numFrames;
    AnmVm menuSprites[RETRY_MENU_SPRITES + 1];
    AnmVm menuBackground;
};

struct AsciiManagerPopup
{
    u8 digits[8];
    ZunVec3 pos;
    ZunVec3 prevPos;
    u32 color;
    ZunTimer timer;
    u8 inUse;
    u8 characterCount;
    // pad 2
};

struct AsciiManagerString
{
    char text[64];
    ZunVec3 pos;
    u32 color;
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

    static void AddFormatText(AsciiManager *manager, ZunVec3 *pos, const char *fmt, ...);
    void AddString(ZunVec3 *pos, const char *text);
    void CreatePopup1(ZunVec3 *pos, i32 value, u32 color);
    void CreatePopup2(ZunVec3 *pos, i32 value, u32 color);
    void DrawPopups();
    void DrawStrings();
    void InitializeVms();
    void InitializeOtherVms();
    void UpdateScripts();

    void SetColor(u32 color)
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

    void SetBossMarkerPos(i32 idx, ZunVec3 *pos)
    {
        this->bossMarkers[idx].pos = *pos;
    }

    void SetBossDamageTint(i32 idx, u32 color)
    {
        this->bossDamageTint[idx] = color;
    }

    void SetBossMarkerInterrupt(i32 idx, i32 interrupt)
    {
        this->bossMarkers[idx].pendingInterrupt = interrupt;
    }

    void UpdatePrev()
    {
        this->cherryGauge.UpdatePrev();
        this->cherryDigit.UpdatePrev();
        this->cherryBorderActive.UpdatePrev();
        for (i32 i = 0; i < 4; i++)
        {
            this->bossMarkers[i].UpdatePrev();
        }
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
    u32 color;
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

extern AsciiManager g_AsciiManager;
