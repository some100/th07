#include "Chain.hpp"

#include <stddef.h>

// GLOBAL: TH07 0x00626218
Chain g_Chain;

// FUNCTION: TH07 0x0042fa60
Chain::~Chain()
{
}

// FUNCTION: TH07 0x0042fab0
ChainElem::ChainElem()
{
    this->prev = NULL;
    this->next = NULL;
    this->callback = NULL;
    this->unkPtr = this;
    this->addedCallback = NULL;
    this->deletedCallback = NULL;
    this->priority = 0;
    this->flags &= (u16)~1;
}

// FUNCTION: TH07 0x0042fb20
ChainElem::~ChainElem()
{
    if (this->deletedCallback != NULL)
    {
        (*this->deletedCallback)(this->arg);
    }
    this->prev = NULL;
    this->next = NULL;
    this->callback = NULL;
    this->addedCallback = NULL;
    this->deletedCallback = NULL;
}

// FUNCTION: TH07 0x0042fb80
Chain::Chain()
{
}

// FUNCTION: TH07 0x0042fbd0
ZunResult Chain::AddToCalcChain(ChainElem *elem, i32 priority)
{
    ZunResult uVar1;
    ChainElem *curElem;

    curElem = &this->calcChain;
    elem->priority = priority;
    while (curElem->next != NULL)
    {
        if (curElem->priority > priority)
            break;
        curElem = curElem->next;
    }
    if (curElem->priority > priority)
    {
        elem->next = curElem;
        elem->prev = curElem->prev;
        if (elem->prev != NULL)
        {
            elem->prev->next = elem;
        }
        curElem->prev = elem;
    }
    else
    {
        elem->next = NULL;
        elem->prev = curElem;
        curElem->next = elem;
    }
    if (elem->addedCallback != NULL)
    {
        uVar1 = elem->addedCallback(elem->arg);
        elem->addedCallback = NULL;
        return uVar1;
    }
    else
    {
        return ZUN_SUCCESS;
    }
}

// FUNCTION: TH07 0x0042fca0
ZunResult Chain::AddToDrawChain(ChainElem *elem, i32 priority)
{
    ChainElem *curElem;

    curElem = &this->drawChain;
    elem->priority = priority;
    while (curElem->next != NULL)
    {
        if (curElem->priority > priority)
            break;
        curElem = curElem->next;
    }
    if (curElem->priority > priority)
    {
        elem->next = curElem;
        elem->prev = curElem->prev;
        if (elem->prev != NULL)
        {
            elem->prev->next = elem;
        }
        curElem->prev = elem;
    }
    else
    {
        elem->next = NULL;
        elem->prev = curElem;
        curElem->next = elem;
    }
    if (elem->addedCallback != NULL)
    {
        return elem->addedCallback(elem->arg);
    }
    else
    {
        return ZUN_SUCCESS;
    }
}

// FUNCTION: TH07 0x0042fd60
i32 Chain::RunCalcChain()
{
    ChainElem *next;
    ChainElem *current;
    i32 updateCount;

restart_from_first_job:
    updateCount = 0;
    current = &this->calcChain;
    while (current != NULL)
    {
        if (current->callback != NULL)
        {
        execute_again:
            switch (current->callback(current->arg))
            {
            case CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB:
                next = current;
                current = current->next;
                Cut(next);
                updateCount++;
                continue;
            case CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN:
                goto execute_again;
            case CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS:
                return 0;
            case CHAIN_CALLBACK_RESULT_BREAK:
                return 1;
            case CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR:
                return -1;
            case CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB:
                goto restart_from_first_job;
            default:
                break;
            }
            updateCount += 1;
        }
        current = current->next;
    }
    return updateCount;
}

// FUNCTION: TH07 0x0042fe20
i32 Chain::RunDrawChain()
{
    ChainElem *next;
    ChainElem *current;
    i32 updateCount;

    updateCount = 0;
    current = &this->drawChain;
    while (current != NULL)
    {
        if (current->callback != NULL)
        {
        execute_again:
            switch (current->callback(current->arg))
            {
            case CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB:
                next = current;
                current = current->next;
                Cut(next);
                updateCount++;
                continue;
            case CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN:
                goto execute_again;
            case CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS:
                return 0;
            case CHAIN_CALLBACK_RESULT_BREAK:
                return 1;
            case CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR:
                return -1;
            default:
                break;
            }
            updateCount += 1;
        }
        current = current->next;
    }
    return updateCount;
}

#pragma var_order(curElem, tmp2, tmp, nextRootElem)
// FUNCTION: TH07 0x0042fee0
void Chain::ReleaseSingleChain(ChainElem *root)
{
    ChainElem nextRootElem;
    ChainElem *tmp2;
    ChainElem *tmp;
    ChainElem *curElem;

    tmp = new ChainElem;
    nextRootElem.next = tmp;
    curElem = root;
    while (curElem != NULL)
    {
        tmp->unkPtr = curElem;
        tmp->next = new ChainElem;
        tmp = tmp->next;
        curElem = curElem->next;
    }
    curElem = &nextRootElem;
    while (curElem != NULL)
    {
        Cut(curElem->unkPtr);
        curElem = curElem->next;
    }
    tmp = nextRootElem.next;
    while (tmp != NULL)
    {
        tmp2 = tmp->next;
        delete tmp;
        tmp = NULL;
        tmp = tmp2;
    }
}

// FUNCTION: TH07 0x00430060
void Chain::Release()
{
    ReleaseSingleChain(&this->calcChain);
    ReleaseSingleChain(&this->drawChain);
}

// FUNCTION: TH07 0x00430090
ChainElem *Chain::CreateElem(ChainCallback callback)
{
    ChainElem *elem = new ChainElem;
    elem->callback = callback;
    elem->addedCallback = NULL;
    elem->deletedCallback = NULL;
    elem->flags |= (u16)1;
    return elem;
}

// FUNCTION: TH07 0x00430140
void Chain::Cut(ChainElem *toRemove)
{
    BOOL isDrawChain;
    ChainElem *curElem;

    isDrawChain = FALSE;

    if (toRemove == NULL)
        return;

    curElem = &this->calcChain;
    while (curElem != NULL)
    {
        if (curElem == toRemove)
            goto destroy_elem;
        curElem = curElem->next;
    }
    isDrawChain = TRUE;
    curElem = &this->drawChain;
    while (curElem != NULL)
    {
        if (curElem == toRemove)
            goto destroy_elem;
        curElem = curElem->next;
    }

    return;

destroy_elem:
    if (toRemove->prev != NULL)
    {
        toRemove->callback = NULL;
        toRemove->prev->next = toRemove->next;
        if (toRemove->next != NULL)
        {
            toRemove->next->prev = toRemove->prev;
        }
        toRemove->prev = NULL;
        toRemove->next = NULL;

        if (toRemove->flags & 1)
        {
            delete toRemove;
            toRemove = NULL;
        }
        else
        {
            if (toRemove->deletedCallback != NULL)
            {
                ChainLifecycleCallback deletedCallback = toRemove->deletedCallback;
                deletedCallback(toRemove->arg);
                toRemove->deletedCallback = NULL;
            }
        }
    }
}
