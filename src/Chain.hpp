#pragma once

#include "ZunResult.hpp"
#include "inttypes.hpp"

enum ChainCallbackResult
{
    CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB,
    CHAIN_CALLBACK_RESULT_CONTINUE,
    CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN,
    CHAIN_CALLBACK_RESULT_BREAK,
    CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS,
    CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR,
    CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB
};

enum
{
    CHAIN_PRIO_CALC_SUPERVISOR = 0,
    CHAIN_PRIO_CALC_ASCIIMANAGER = 1,
    CHAIN_PRIO_CALC_GAMEMANAGER = 2,
    CHAIN_PRIO_CALC_MAINMENU = 3,
    CHAIN_PRIO_CALC_MUSICROOM = 3,
    CHAIN_PRIO_CALC_ENDING = 4,
    CHAIN_PRIO_CALC_REPLAYMANAGER_PLAYBACK_HIGH_PRIO = 5,
    CHAIN_PRIO_CALC_REPLAYMANAGER_PLAYBACK_LOW_PRIO = 6,
    CHAIN_PRIO_CALC_STAGE = 7,
    CHAIN_PRIO_CALC_PLAYER = 8,
    CHAIN_PRIO_CALC_ENEMYMANAGER = 10,
    CHAIN_PRIO_CALC_EFFECTMANAGER = 11,
    CHAIN_PRIO_CALC_BULLETMANAGER = 12,
    CHAIN_PRIO_CALC_GUI = 13,
    CHAIN_PRIO_CALC_RESULTSCREEN = 14,
    CHAIN_PRIO_CALC_SCREENEFFECT = 15,
    CHAIN_PRIO_CALC_REPLAYMANAGER_RECORD_HIGH_PRIO = 16,
    CHAIN_PRIO_CALC_REPLAYMANAGER_LOW_PRIO = 17,
};

enum
{
    CHAIN_PRIO_DRAW_MAINMENU = 0,
    CHAIN_PRIO_DRAW_MUSICROOM = 0,
    CHAIN_PRIO_DRAW_ENDING = 1,
    CHAIN_PRIO_DRAW_GAMEMANAGER = 2,
    CHAIN_PRIO_DRAW_STAGE_HIGH_PRIO = 3,
    CHAIN_PRIO_DRAW_STAGE_LOW_PRIO = 4,
    CHAIN_PRIO_DRAW_ENEMYMANAGER_HIGH_PRIO = 5,
    CHAIN_PRIO_DRAW_PLAYER_HIGH_PRIO = 6,
    CHAIN_PRIO_DRAW_ENEMYMANAGER_LOW_PRIO = 7,
    CHAIN_PRIO_DRAW_PLAYER_LOW_PRIO = 8,
    CHAIN_PRIO_DRAW_EFFECTMANAGER = 9,
    CHAIN_PRIO_DRAW_BULLETMANAGER = 10,
    CHAIN_PRIO_DRAW_ASCIIMANAGER_POPUPS = 11,
    CHAIN_PRIO_DRAW_GUI = 12,
    CHAIN_PRIO_DRAW_RESULTSCREEN = 13,
    CHAIN_PRIO_DRAW_REPLAYMANAGER = 14,
    CHAIN_PRIO_DRAW_SUPERVISOR = 15,
    CHAIN_PRIO_DRAW_ASCIIMANAGER_MENUS = 16,
    CHAIN_PRIO_DRAW_SCREENEFFECT = 17
};

typedef u32 (*ChainCallback)(void *);
typedef ZunResult (*ChainLifecycleCallback)(void *);

struct ChainElem
{
    ChainElem();
    ~ChainElem();

    i16 priority;
    u16 isAllocated : 1;
    ChainCallback callback;
    ChainLifecycleCallback addedCallback;
    ChainLifecycleCallback deletedCallback;
    ChainElem *prev;
    ChainElem *next;
    ChainElem *unkPtr;
    void *arg;
};

struct Chain
{
    Chain();
    ~Chain();

    ZunResult AddToCalcChain(ChainElem *elem, i32 priority);
    ZunResult AddToDrawChain(ChainElem *elem, i32 priority);
    ChainElem *CreateElem(ChainCallback callback);
    void Cut(ChainElem *toRemove);
    void Release();
    void ReleaseSingleChain(ChainElem *root);
    i32 RunCalcChain();
    i32 RunDrawChain();

    ChainElem calcChain;
    ChainElem drawChain;
};

extern Chain g_Chain;
