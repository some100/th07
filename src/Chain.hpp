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
