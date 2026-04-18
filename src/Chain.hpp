#pragma once

#include <windows.h>

#include "ZunResult.hpp"
#include "inttypes.hpp"

typedef enum ChainCallbackResult
{
    CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB = 0,
    CHAIN_CALLBACK_RESULT_CONTINUE = 1,
    CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN = 2,
    CHAIN_CALLBACK_RESULT_BREAK = 3,
    CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS = 4,
    CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR = 5,
    CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB = 6
} ChainCallbackResult;

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
C_ASSERT(sizeof(Chain) == 0x40);

extern Chain g_Chain;
