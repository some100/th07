#include "Chain.hpp"

#include <stddef.h>

Chain g_Chain;

Chain::~Chain() {}

ChainElem::ChainElem() {
  this->prev = NULL;
  this->next = NULL;
  this->callback = NULL;
  this->unkPtr = this;
  this->addedCallback = NULL;
  this->deletedCallback = NULL;
  this->priority = 0;
  this->flags &= ~1;
}

ChainElem::~ChainElem() {
  if (this->deletedCallback != NULL) {
    (*this->deletedCallback)(this->arg);
  }
  this->prev = NULL;
  this->next = NULL;
  this->callback = NULL;
  this->addedCallback = NULL;
  this->deletedCallback = NULL;
}

Chain::Chain() {}

ZunResult Chain::AddToCalcChain(ChainElem *elem, i32 priority) {
  ZunResult uVar1;
  ChainElem *calcChain;

  elem->priority = (i16)priority;
  for (calcChain = &this->calcChain;
       (calcChain->next != NULL && (calcChain->priority <= priority));
       calcChain = calcChain->next) {
  }
  if (priority < calcChain->priority) {
    elem->next = calcChain;
    elem->prev = calcChain->prev;
    if (elem->prev != NULL) {
      elem->prev->next = elem;
    }
    calcChain->prev = elem;
  } else {
    elem->next = NULL;
    elem->prev = calcChain;
    calcChain->next = elem;
  }
  if (elem->addedCallback == NULL) {
    uVar1 = ZUN_SUCCESS;
  } else {
    uVar1 = elem->addedCallback(elem->arg);
    elem->addedCallback = NULL;
  }
  return uVar1;
}

ZunResult Chain::AddToDrawChain(ChainElem *elem, i32 priority) {
  ZunResult uVar1;
  ChainElem *drawChain;

  elem->priority = (i16)priority;
  for (drawChain = &this->drawChain;
       (drawChain->next != NULL && (drawChain->priority <= priority));
       drawChain = drawChain->next) {
  }
  if (priority < drawChain->priority) {
    elem->next = drawChain;
    elem->prev = drawChain->prev;
    if (elem->prev != NULL) {
      elem->prev->next = elem;
    }
    drawChain->prev = elem;
  } else {
    elem->next = NULL;
    elem->prev = drawChain;
    drawChain->next = elem;
  }
  if (elem->addedCallback == NULL) {
    uVar1 = ZUN_SUCCESS;
  } else {
    uVar1 = (*elem->addedCallback)(elem->arg);
    elem->addedCallback = NULL;
  }
  return uVar1;
}

i32 Chain::RunCalcChain()

{
  i32 updateCount;
  ChainElem *current;
  ChainElem *tmp;

restart:
  updateCount = 0;
  current = &this->calcChain;
LAB_0042fd76:
  while (true) {
    if (current == NULL) {
      return updateCount;
    }
    if (current->callback != NULL)
      break;
  LAB_0042fde7:
    current = current->next;
  }
  while (true) {
    switch (current->callback(current->arg)) {
    case CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB:
      goto switchD_0042fd9d_caseD_0;
    default:
      updateCount += 1;
      goto LAB_0042fde7;
    case CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN:
      break;
    case CHAIN_CALLBACK_RESULT_BREAK:
      return 1;
    case CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS:
      return 0;
    case CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR:
      return -1;
    case CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB:
      goto restart;
    }
  }
switchD_0042fd9d_caseD_0:
  tmp = current->next;
  Cut(current);
  updateCount = updateCount + 1;
  current = tmp;
  goto LAB_0042fd76;
}

i32 Chain::RunDrawChain()

{
  i32 updateCount;
  ChainElem *current;
  ChainElem *next;

  updateCount = 0;
  current = &this->drawChain;
LAB_0042fe39:
  while (true) {
    if (current == NULL) {
      return updateCount;
    }
    if (current->callback != NULL)
      break;
  LAB_0042fea8:
    current = current->next;
  }
  while (true) {
    switch (current->callback(current->arg)) {
    case CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB:
      goto switchD_0042fe60_caseD_0;
    default:
      updateCount += 1;
      goto LAB_0042fea8;
    case CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN:
      break;
    case CHAIN_CALLBACK_RESULT_BREAK:
      return 1;
    case CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS:
      return 0;
    case CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR:
      return -1;
    }
  }
switchD_0042fe60_caseD_0:
  next = current->next;
  Cut(current);
  updateCount = updateCount + 1;
  current = next;
  goto LAB_0042fe39;
}

void Chain::ReleaseSingleChain(ChainElem *root)

{
  ChainElem nextRootElem;
  ChainElem *tmp;
  ChainElem *tmp2;
  ChainElem *curElem;

  tmp = new ChainElem;
  nextRootElem.next = tmp;
  for (curElem = root; curElem != NULL; curElem = curElem->next) {
    tmp->unkPtr = curElem;
    tmp->next = new ChainElem;
    tmp = tmp->next;
  }
  for (curElem = &nextRootElem; curElem != NULL; curElem = curElem->next) {
    Cut(curElem->unkPtr);
  }
  tmp = nextRootElem.next;
  while (tmp != NULL) {
    tmp2 = tmp->next;
    delete tmp;
    tmp = NULL;
    tmp = tmp2;
  }
}

void Chain::Release()

{
  ReleaseSingleChain(&this->calcChain);
  ReleaseSingleChain(&this->drawChain);
}

ChainElem *Chain::CreateElem(ChainCallback callback) {
  ChainElem *elem = new ChainElem;
  elem->callback = callback;
  elem->addedCallback = NULL;
  elem->deletedCallback = NULL;
  elem->flags = elem->flags | 1;
  return elem;
}

void Chain::Cut(ChainElem *toRemove) {
  ChainElem *curElem = &this->calcChain;
  if (toRemove != NULL) {
    while (curElem != NULL) {
      if (curElem == toRemove)
        goto destroy_elem;
      curElem = curElem->next;
    }
    for (curElem = &this->drawChain; curElem != NULL; curElem = curElem->next) {
      if (curElem == toRemove) {
      destroy_elem:
        if (toRemove->prev == NULL) {
          return;
        }
        toRemove->callback = NULL;
        toRemove->prev->next = toRemove->next;
        if (toRemove->next != NULL) {
          toRemove->next->prev = toRemove->prev;
        }
        toRemove->prev = NULL;
        toRemove->next = NULL;
        if ((toRemove->flags & 1) != 0) {
          delete toRemove;
          return;
        }
        if (toRemove->deletedCallback == NULL) {
          return;
        }
        ChainLifecycleCallback deletedCallback = toRemove->deletedCallback;
        toRemove->deletedCallback = NULL;
        (*deletedCallback)(toRemove->arg);
        return;
      }
    }
  }
}
