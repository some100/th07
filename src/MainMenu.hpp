#pragma once

#include "AnmVm.hpp"
#include "Chain.hpp"
#include "ReplayManager.hpp"
#include "Supervisor.hpp"

enum MenuState
{
    MENU_STATE_PRE_INPUT = 0,
    MENU_STATE_OPTIONS = 2,
    MENU_STATE_KEY_CONFIG = 3,
    MENU_STATE_NORMAL_SELECT_DIFFICULTY = 4,
    MENU_STATE_NORMAL_SELECT_CHARACTER = 5,
    MENU_STATE_NORMAL_SELECT_SHOTTYPE = 6,
    MENU_STATE_SELECT_REPLAY = 7,
    MENU_STATE_PRACTICE_SELECT_DIFFICULTY = 8,
    MENU_STATE_PRACTICE_SELECT_CHARACTER = 9,
    MENU_STATE_PRACTICE_SELECT_SHOTTYPE = 10,
    MENU_STATE_SELECT_PRACTICE_STAGE = 11,
    MENU_STATE_EXTRA_SELECT_DIFFICULTY = 12,
    MENU_STATE_EXTRA_SELECT_CHARACTER = 13,
    MENU_STATE_EXTRA_SELECT_SHOTTYPE = 14
};

enum MenuSubStatePreInput
{
    MENU_SUBSTATE_PREINPUT_INIT,
    MENU_SUBSTATE_PREINPUT_INPUT,
    MENU_SUBSTATE_PREINPUT_EXIT,
    MENU_SUBSTATE_PREINPUT_OPTIONS,
};

enum MenuSubStateSelect
{
    MENU_SUBSTATE_SELECT_INIT,
    MENU_SUBSTATE_SELECT_INPUT,
};

enum MenuCursorPreInput
{
    MENU_CURSOR_PREINPUT_START,
    MENU_CURSOR_PREINPUT_EXTRA_START,
    MENU_CURSOR_PREINPUT_PRACTICE_START,
    MENU_CURSOR_PREINPUT_REPLAY,
    MENU_CURSOR_PREINPUT_RESULTS,
    MENU_CURSOR_PREINPUT_MUSICROOM,
    MENU_CURSOR_PREINPUT_OPTIONS,
    MENU_CURSOR_PREINPUT_EXIT,
};

enum MenuCursorOptionsMenu
{
    MENU_CURSOR_OPTIONS_MENU_LIVES,
    MENU_CURSOR_OPTIONS_COLOR_MODE,
    MENU_CURSOR_OPTIONS_MUSIC_MODE,
    MENU_CURSOR_OPTIONS_PLAY_SFX,
    MENU_CURSOR_OPTIONS_WINDOW_MODE,
    MENU_CURSOR_OPTIONS_SLOW_MODE,
    MENU_CURSOR_OPTIONS_RESET,
    MENU_CURSOR_OPTIONS_KEY_CONFIG,
    MENU_CURSOR_OPTIONS_EXIT,
};

enum MenuCursorKeyConfig
{
    MENU_CURSOR_KEYCONFIG_SHOOT,
    MENU_CURSOR_KEYCONFIG_BOMB,
    MENU_CURSOR_KEYCONFIG_FOCUS,
    MENU_CURSOR_KEYCONFIG_SKIP,
    MENU_CURSOR_KEYCONFIG_MENU,
    MENU_CURSOR_KEYCONFIG_UP,
    MENU_CURSOR_KEYCONFIG_DOWN,
    MENU_CURSOR_KEYCONFIG_LEFT,
    MENU_CURSOR_KEYCONFIG_RIGHT,
    MENU_CURSOR_KEYCONFIG_SHOTSLOW,
    MENU_CURSOR_KEYCONFIG_RESET,
    MENU_CURSOR_KEYCONFIG_EXIT,
};

enum MenuCursorSelectDifficulty
{
    MENU_CURSOR_SELECTDIFFICULTY_EASY,
    MENU_CURSOR_SELECTDIFFICULTY_NORMAL,
    MENU_CURSOR_SELECTDIFFICULTY_HARD,
    MENU_CURSOR_SELECTDIFFICULTY_LUNATIC,
    MENU_CURSOR_SELECTDIFFICULTY_EXTRA,
    MENU_CURSOR_SELECTDIFFICULTY_PHANTASM,
};

enum MenuCursorSelectCharacter
{
    MENU_CURSOR_SELECTCHARACTER_REIMU,
    MENU_CURSOR_SELECTCHARACTER_MARISA,
    MENU_CURSOR_SELECTCHARACTER_SAKUYA,
    MENU_CURSOR_SELECTCHARACTER_COUNT,
};

enum MenuCursorSelectShotType
{
    MENU_CURSOR_SELECTSHOTTYPE_A,
    MENU_CURSOR_SELECTSHOTTYPE_B,
    MENU_CURSOR_SELECTSHOTTYPE_COUNT,
};

struct MainMenu
{
    MainMenu()
    {
        memset(this, 0, sizeof(MainMenu));
    }

    static ZunResult RegisterChain();

    static ZunResult AddedCallback(MainMenu *arg);
    static ZunResult DeletedCallback(MainMenu *arg);
    static u32 OnUpdate(MainMenu *arg);
    static u32 OnDraw(MainMenu *arg);

    u32 OnUpdatePreInput();
    u32 OnUpdateOptionsMenu();
    u32 OnUpdateKeyConfig();

    u32 OnUpdateSelectDifficulty();
    u32 OnUpdateSelectCharacter();
    u32 OnUpdateSelectShotType();

    u32 OnUpdateSelectReplay();
    u32 OnUpdateSelectPracticeStage();

    i32 DrawReplayMenu();
    i32 DrawPracticeMenu();

    ZunResult ActualAddedCallback();
    i32 MoveCursorHorizontal(i32 max);
    i32 MoveCursorVertical(i32 max);
    ZunResult Release();
    void SwapMapping(i16 btnPressed, i16 oldMapping);
    ZunResult UpdateMenuDigits(AnmVm *vm, i16 number);

    static void InitializeTimingVars(Supervisor *arg)
    {
        arg->timingErrorCount = 0;
        arg->maxTimingError = 0;
        arg->checkTiming = 0;
        arg->timingSpikeAccumulator = 0;
        arg->timingBadCount = 0;
    }

    void SetMenuState(MenuState menuState)
    {
        this->prevMenuState = this->menuState;
        this->menuState = menuState;
        this->inputDelayTimer = 0;
        this->stateTimer = 0;
        this->menuSubState = 0;
        this->idleFrames = 0;
    }

    i32 IsSelected(i32 idx)
    {
        i32 selected = idx == this->cursor;
        return selected;
    }

    i32 IsReplaySelected(i32 idx)
    {
        i32 selected = idx == this->chosenReplay;
        return selected;
    }

    i32 IsStageSelected(i32 idx)
    {
        i32 selected = idx == this->selectedStage;
        return selected;
    }

    void UpdatePrev()
    {
        if (this->vmHead)
        {
            for (i32 i = 0; i < this->vmCount; i++)
            {
                this->vmHead[i].UpdatePrev();
            }
        }
        for (i32 i = 0; i < 14; i++)
        {
            this->vms[i].UpdatePrev();
        }
    }

    i32 cursor;
    i32 selected;
    i32 menuSubState;
    i32 inputDelayTimer;
    i32 unused_10[21];
    i32 prevMenuState;
    i32 isPracticeMode;
    char replayFilenames[60][512];
    char replayLabels[60][8];
    ReplayFile replays[60];
    ReplayFile *currentReplay;
    i32 replayPage;
    i32 replayFilesNum;
    i32 chosenReplay;
    i32 selectedStage;
    i32 idleFrames;
    AnmVm *vmHead;
    AnmVm *cursorVm;
    AnmVm vms[14];
    i32 vmCount;
    i32 menuState;
    i32 stateTimer;
    i32 demoFramesCount;
    ChainElem *calcChain;
    ChainElem *drawChain;
    ControllerMapping controlMapping;
    // pad 2
    GameConfiguration cfg;
};
