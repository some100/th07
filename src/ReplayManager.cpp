#include "ReplayManager.hpp"

#include "Chain.hpp"
#include "EffectManager.hpp"
#include "EnemyManager.hpp"
#include "FileSystem.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "Player.hpp"
#include "Rng.hpp"
#include "Supervisor.hpp"
#include "pbg4/Lzss.hpp"
#include "dsutil.hpp"

ReplayManager *g_ReplayManager;

u32 ReplayManager::OnUpdateRng(ReplayManager *arg)

{
  arg->replayEventFlags = 0;
  arg->rngSeed = g_Rng.seed;
  g_Rng.generationCount = 0;
  if (g_GameManager.isPaused != 0) {
    arg->replayEventFlags |= 0x100;
  }
  g_GameManager.isPaused = 0;
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

u32 ReplayManager::OnUpdate(ReplayManager *arg)

{
  i32 stage;

  u16 uVar1 = g_CurFrameRawInput;
  if ((g_GameManager.flags >> 2 & 1) != 0) {
    g_LastFrameGameInput = g_CurFrameGameInput;
    g_CurFrameGameInput = g_CurFrameRawInput;
    if ((g_GameManager.defaultCfg->slowMode == 0) &&
        ((g_Supervisor.flags >> 3 & 1) == 0)) {
      stage = g_GameManager.currentStage - 1;
      if (6 < stage) {
        stage = 6;
      }
      arg->replayInputs = arg->replayInputs + 1;
      arg->replayInputsByStage[stage] = arg->replayInputs + 1;
      arg->replayInputs->frameNum = uVar1;
      arg->replayInputs->inputKey = arg->replayEventFlags;
      if (arg->frameId % 0x1e == 0) {
        *(u8 *)&arg->stageReplayData->score =
            (u8)g_Supervisor.curFps |
            (-(g_Supervisor.timingErrorCount != 0) & 0x80U);
        *((u8 *)&arg->stageReplayData->score + 1) = (u8)g_Supervisor.curFps;
        arg->replayDataEndPointers[stage] =
            (i32)((i32)&arg->stageReplayData->score + 2);
        arg->stageReplayData =
            (StageReplayData *)((i32)&arg->stageReplayData->score + 1);
      }
      arg->frameId = arg->frameId + 1;
    }
  }
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

u32 ReplayManager::OnUpdateDemoLowPrio(ReplayManager *arg)

{
  if ((g_GameManager.flags >> 2 & 1) != 0) {
    if (((g_Gui.HasCurrentMsgIdx() != 0) &&
         (g_Gui.IsDialogueSkippable() != 0)) &&
        (arg->frameId % 3 != 2)) {
      return CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB;
    }
    if (((g_GameManager.replayStage == 2) &&
         (g_EnemyManager.HasActiveBoss() == 0)) &&
        (arg->frameId % 5 != 4)) {
      return CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB;
    }
  }
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

u32 ReplayManager::OnUpdateDemoHighPrio(ReplayManager *arg)

{
  if (((g_GameManager.flags >> 2 & 1) != 0) &&
      (g_GameManager.defaultCfg->slowMode == 0)) {
    g_LastFrameGameInput = g_CurFrameGameInput;
    g_CurFrameGameInput = arg->replayInputs->frameNum;
    arg->replayInputs = arg->replayInputs + 1;
    g_IsEighthFrameOfHeldInput = 0;
    if (g_LastFrameGameInput == g_CurFrameGameInput) {
      if (0x1d < g_NumOfFramesInputsWereHeld) {
        g_IsEighthFrameOfHeldInput =
            (g_NumOfFramesInputsWereHeld & 0x80000007) == 0;
        if (0x25 < g_NumOfFramesInputsWereHeld) {
          g_NumOfFramesInputsWereHeld = 0x1e;
        }
      }
      g_NumOfFramesInputsWereHeld += 1;
    } else {
      g_NumOfFramesInputsWereHeld = 0;
    }
    if (arg->frameId % 0x1e == 0) {
      g_Supervisor.curFps =
          (i16) * (char *)((i32)&arg->stageReplayData->score + 1) & 0x7f;
      g_Supervisor.isFpsBad =
          (i32) * (char *)((i32)&arg->stageReplayData->score + 1) >> 7;
      arg->stageReplayData =
          (StageReplayData *)((i32)&arg->stageReplayData->score + 1);
    }
    arg->frameId = arg->frameId + 1;
  }
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult ReplayManager::AddedCallback(ReplayManager *arg)

{
  i32 i;

  arg->frameId = 0;
  arg->unused_40 = NULL;
  if (arg->data == NULL) {
    arg->data = new ReplayHeaderAndData;
    arg->data->head.magic = 0x50523754;
    arg->data->data.shotType = g_GameManager.shotTypeAndCharacter;
    arg->data->head.version = 0x1100;
    arg->data->data.replayVersion = 0x100;
    arg->data->data.versionChar1 = 'b';
    strncpy(arg->data->data.replayStr, "0100", 4);
    arg->data->data.versionChar2 = 'b';
    arg->data->data.exeSize = g_Supervisor.exeSize;
    arg->data->data.exeChecksum = g_Supervisor.exeChecksum;
    arg->data->data.difficulty = g_GameManager.difficulty;
    strncpy(arg->data->data.name, "NO N", 4);
    arg->data->data.cfg = *g_GameManager.defaultCfg;
    for (i = 0; i < 7; i += 1) {
      arg->data->head.stageReplayData[i].data = NULL;
      arg->data->head.stageEndData[i].data = NULL;
    }
  } else if ((-1 < g_GameManager.currentStage - 2) &&
             (arg->data->head.stageReplayData[g_GameManager.currentStage - 2]
                  .data != NULL)) {
    arg->data->head.stageReplayData[g_GameManager.currentStage - 2]
        .data->score = g_GameManager.globals->score;
  }
  i = g_GameManager.currentStage - 1;
  if (6 < i) {
    i = 6;
  }
  if (arg->data->head.stageReplayData[i].data != NULL) {
    free(arg->data->head.stageReplayData[i].data);
  }
  if (arg->data->head.stageEndData[i].data != NULL) {
    free(arg->data->head.stageEndData[i].data);
  }
  arg->data->head.stageReplayData[i].data =
      (StageReplayData *)malloc(sizeof(StageReplayData));
  arg->data->head.stageEndData[i].data =
      (StageReplayData *)malloc(sizeof(StageReplayData));
  arg->data->head.stageReplayData[i].data->grazeInTotal =
      g_GameManager.globals->grazeInTotal;
  arg->data->head.stageReplayData[i].data->bombsRemaining =
      g_GameManager.globals->bombsRemaining;
  arg->data->head.stageReplayData[i].data->livesRemaining =
      g_GameManager.globals->livesRemaining;
  arg->data->head.stageReplayData[i].data->currentPower =
      g_GameManager.globals->currentPower;
  arg->data->head.stageReplayData[i].data->rank = g_GameManager.rank.rank;
  arg->data->head.stageReplayData[i].data->pointItemsCollectedForExtend =
      g_GameManager.globals->pointItemsCollectedForExtend;
  arg->data->head.stageReplayData[i].data->stageRngSeed =
      g_GameManager.stageRngSeed;
  arg->data->head.stageReplayData[i].data->powerItemCountForScore =
      g_GameManager.powerItemCountForScore;
  arg->data->head.stageReplayData[i].data->cherry =
      g_GameManager.cherry - g_GameManager.globals->cherryStart;
  arg->data->head.stageReplayData[i].data->cherryMax =
      g_GameManager.cherryMax - g_GameManager.globals->cherryStart;
  arg->data->head.stageReplayData[i].data->cherryPlus =
      g_GameManager.cherryPlus - g_GameManager.globals->cherryStart;
  arg->data->head.stageReplayData[i].data->spellCardsCaptured =
      (u8)g_GameManager.globals->spellCardsCaptured;
  arg->data->head.stageReplayData[i].data->extendsFromPointItems =
      g_GameManager.globals->extendsFromPointItems;
  arg->data->head.stageReplayData[i].data->nextNeededPointItemsForExtend =
      g_GameManager.globals->nextNeededPointItemsForExtend;
  arg->replayInputs = arg->data->head.stageReplayData[i].data->replayInputs;
  arg->stageReplayData = arg->data->head.stageEndData[i].data;
  arg->replayInputs->frameNum = 0;
  arg->unused_82 = 0;
  return ZUN_SUCCESS;
}

ReplayHeaderAndData *
ReplayManager::ValidateReplayData(ReplayHeaderAndData *data, i32 size)

{
  ReplayHeaderAndData *puVar1;
  u8 *local_20;
  i32 checksumBase;
  i32 *curByte;
  u8 obfOffset;
  i32 i;

  if (((data != NULL) && (data->head.magic == 0x50523754)) &&
      (data->head.version == 0x1100)) {
    curByte = &data->head.replaySize;
    obfOffset = data->head.key;
    for (i = 0; i < size - 0x10; i += 1) {
      *(u8 *)curByte = (char)*curByte - obfOffset;
      obfOffset += 7;
      curByte = (i32 *)((i32)curByte + 1);
    }
    local_20 = &data->head.key;
    checksumBase = 0x3f000318;
    for (i = 0; i < size - 0xd; i += 1) {
      checksumBase += (u32)*local_20;
      local_20 = local_20 + 1;
    }
    if (checksumBase == data->head.checksum) {
      puVar1 = (ReplayHeaderAndData *)malloc(data->head.sizeWithoutHeader +
                                             sizeof(ReplayHeader));
      memcpy(puVar1, data, sizeof(ReplayHeader));
      Lzss::Decompress(&data->data.rngValue3, data->head.compressedSize,
                       &puVar1->data.rngValue3, data->head.sizeWithoutHeader);
      if (((puVar1->data.cfg).slowMode == 0) &&
          (g_Supervisor.CheckIntegrity(
               puVar1->data.replayStr, puVar1->data.exeSize,
               puVar1->data.exeChecksum) == ZUN_SUCCESS)) {
        free(data);
        return puVar1;
      }
    }
  }
  free(data);
  return NULL;
}

ZunResult ReplayManager::AddedCallbackDemo(ReplayManager *arg)

{
  i32 i;

  arg->frameId = 0;
  if (arg->data == NULL) {
    arg->data = (ReplayHeaderAndData *)FileSystem::OpenFile(
        arg->replayFilename, ((g_GameManager.flags >> 1 & 1) == 0));
    arg->data = ValidateReplayData(arg->data, g_LastFileSize);
    if (arg->data == NULL) {
      return ZUN_ERROR;
    }
    arg->unused_40 = NULL;
    for (i = 0; i < 7; i += 1) {
      arg->stageReplayDataSize[i] = 0;
      arg->stageEndDataSize[i] = 0;
      if (arg->data->head.stageReplayData[i].offset != 0) {
        if ((i < 6) && (arg->data->head.stageReplayData[i + 1].offset != 0)) {
          arg->stageReplayDataSize[i] =
              arg->data->head.stageReplayData[i + 1].offset -
              arg->data->head.stageReplayData[i].offset;
        } else {
          arg->stageReplayDataSize[i] =
              arg->data->head.stageEndData[i].offset -
              arg->data->head.stageReplayData[i].offset;
        }
        if ((i < 6) && (arg->data->head.stageEndData[i + 1].offset != 0)) {
          arg->stageEndDataSize[i] =
              arg->data->head.stageEndData[i + 1].offset -
              arg->data->head.stageEndData[i].offset;
        } else {
          arg->stageEndDataSize[i] =
              (arg->data->head.sizeWithoutHeader + sizeof(ReplayHeader)) -
              arg->data->head.stageEndData[i].offset;
        }
      }

      if (arg->data->head.stageReplayData[i].offset != 0) {
        arg->data->head.stageReplayData[i].data =
            (StageReplayData *)((u8 *)arg->data +
                                arg->data->head.stageReplayData[i].offset);
      }
      if (arg->data->head.stageEndData[i].offset != 0) {
        arg->data->head.stageEndData[i].data =
            (StageReplayData *)((u8 *)arg->data +
                                arg->data->head.stageEndData[i].offset);
      }
    }
  }
  i = g_GameManager.currentStage - 1;
  if (6 < i) {
    i = 6;
  }
  if (arg->data->head.stageReplayData[i].data == NULL) {
    return ZUN_ERROR;
  } else {
    g_GameManager.character = arg->data->data.shotType / 2;
    g_GameManager.shotType = arg->data->data.shotType & 1;
    g_GameManager.shotTypeAndCharacter = arg->data->data.shotType;
    g_GameManager.difficulty = arg->data->data.difficulty;
    g_GameManager.globals->pointItemsCollectedForExtend =
        arg->data->head.stageReplayData[i].data->pointItemsCollectedForExtend;
    g_GameManager.rank.rank = arg->data->head.stageReplayData[i].data->rank;
    g_GameManager.globals->livesRemaining =
        arg->data->head.stageReplayData[i].data->livesRemaining;
    g_GameManager.RegenerateGameIntegrityCsum();
    g_GameManager.SetBombsRemainingAndComputeCsum(
        arg->data->head.stageReplayData[i].data->bombsRemaining);
    g_GameManager.globals->currentPower =
        arg->data->head.stageReplayData[i].data->currentPower;
    g_GameManager.RegenerateGameIntegrityCsum();
    g_GameManager.globals->grazeInTotal =
        arg->data->head.stageReplayData[i].data->grazeInTotal;
    arg->replayInputs = arg->data->head.stageReplayData[i].data->replayInputs;
    g_GameManager.powerItemCountForScore =
        arg->data->head.stageReplayData[i].data->powerItemCountForScore;
    g_GameManager.cherry = arg->data->head.stageReplayData[i].data->cherry +
                           g_GameManager.globals->cherryStart;
    g_GameManager.cherryMax =
        arg->data->head.stageReplayData[i].data->cherryMax +
        g_GameManager.globals->cherryStart;
    g_GameManager.cherryPlus =
        arg->data->head.stageReplayData[i].data->cherryPlus +
        g_GameManager.globals->cherryStart;
    if (g_GameManager.globals->cherryStart + 50000 <=
        g_GameManager.cherryPlus) {
      g_GameManager.cherryPlus = g_GameManager.globals->cherryStart + 50000;
      g_Player.ActivateBorder();
    }
    g_GameManager.defaultCfg->controllerMapping =
        arg->data->data.cfg.controllerMapping;
    g_Rng.seed = arg->data->head.stageReplayData[i].data->stageRngSeed;
    g_GameManager.globals->spellCardsCaptured =
        arg->data->head.stageReplayData[i].data->spellCardsCaptured;
    g_GameManager.globals->extendsFromPointItems =
        arg->data->head.stageReplayData[i].data->extendsFromPointItems;
    g_GameManager.globals->nextNeededPointItemsForExtend =
        arg->data->head.stageReplayData[i].data->nextNeededPointItemsForExtend;
    arg->stageReplayData = arg->data->head.stageEndData[i].data;
    if (((1 < g_GameManager.currentStage) &&
         (g_GameManager.currentStage < 7)) &&
        (arg->data->head.stageReplayData[g_GameManager.currentStage - 2].data !=
         NULL)) {
      g_GameManager.globals->score =
          arg->data->head.stageReplayData[g_GameManager.currentStage - 2]
              .data->score;
      g_GameManager.globals->guiScore = g_GameManager.globals->score;
    }
    return ZUN_SUCCESS;
  }
}

ZunResult ReplayManager::DeletedCallback(ReplayManager *arg)

{
  g_Chain.Cut(arg->drawChain);
  arg->drawChain = NULL;
  if (arg->demoCalcChain != NULL) {
    g_Chain.Cut(arg->demoCalcChain);
    arg->demoCalcChain = NULL;
  }
  if (arg->rngCalcChain != NULL) {
    g_Chain.Cut(arg->rngCalcChain);
    arg->rngCalcChain = NULL;
  }
  free(g_ReplayManager->data);
  if (arg->unused_40 != NULL) {
    free(arg->unused_40);
  }
  free(g_ReplayManager);
  g_ReplayManager = NULL;
  return ZUN_SUCCESS;
}

ZunResult ReplayManager::RegisterChain(i32 isDemo, const char *replayFilename)

{
  g_LastFrameGameInput = 0;
  g_CurFrameGameInput = 0;
  if (g_ReplayManager == NULL) {
    ReplayManager *replayManager = new ReplayManager;
    g_ReplayManager = replayManager;
    replayManager->data = NULL;
    replayManager->isDemo = isDemo;
    replayManager->replayFilename = replayFilename;
    if (isDemo == 0) {
      replayManager->calcChain = Chain::CreateElem((ChainCallback)OnUpdate);
      replayManager->calcChain->addedCallback =
          (ChainLifecycleCallback)AddedCallback;
      replayManager->calcChain->deletedCallback =
          (ChainLifecycleCallback)DeletedCallback;
      replayManager->drawChain = Chain::CreateElem(
          (ChainCallback)EffectManager::UpdateNoOp); // idk either bro
      replayManager->calcChain->arg = replayManager;
      if (g_Chain.AddToCalcChain(replayManager->calcChain, 0x10) != 0) {
        return ZUN_ERROR;
      }
      replayManager->demoCalcChain = NULL;
      replayManager->rngCalcChain =
          Chain::CreateElem((ChainCallback)OnUpdateRng);
      replayManager->rngCalcChain->arg = replayManager;
      g_Chain.AddToCalcChain(replayManager->rngCalcChain, 6);
    } else if (isDemo == 1) {
      replayManager->calcChain =
          Chain::CreateElem((ChainCallback)OnUpdateDemoHighPrio);
      replayManager->calcChain->addedCallback =
          (ChainLifecycleCallback)AddedCallbackDemo;
      replayManager->calcChain->deletedCallback =
          (ChainLifecycleCallback)DeletedCallback;
      replayManager->drawChain =
          Chain::CreateElem((ChainCallback)EffectManager::UpdateNoOp);
      replayManager->calcChain->arg = replayManager;
      if (g_Chain.AddToCalcChain(replayManager->calcChain, 5) != 0) {
        return ZUN_ERROR;
      }
      replayManager->demoCalcChain =
          Chain::CreateElem((ChainCallback)OnUpdateDemoLowPrio);
      replayManager->demoCalcChain->arg = replayManager;
      g_Chain.AddToCalcChain(replayManager->demoCalcChain, 0x11);
      replayManager->rngCalcChain = NULL;
    }
    replayManager->drawChain->arg = replayManager;
    g_Chain.AddToDrawChain(replayManager->drawChain, 0xe);
  } else if (isDemo == 0) {
    AddedCallback(g_ReplayManager);
  } else if (isDemo == 1) {
    AddedCallbackDemo(g_ReplayManager);
  }
  return ZUN_SUCCESS;
}

void ReplayManager::StopRecording()

{
  if (g_ReplayManager != NULL) {
    g_ReplayManager->replayInputs = g_ReplayManager->replayInputs + 1;
    g_ReplayManager->replayInputs->frameNum = 0;
    i32 stage = g_GameManager.currentStage - 1;
    if (6 < stage) {
      stage = 6;
    }
    g_ReplayManager->replayInputsByStage[stage] =
        g_ReplayManager->replayInputs + 1;
  }
}

void ReplayManager::SaveReplay(const char *param_1, char *param_2)

{
  u8 *replayData;
  u8 *lpBuffer;
  HANDLE hFile;
  u8 local_158;
  u8 local_154;
  u8 *local_124;
  u8 local_11d;
  u8 *local_11c;
  i32 local_118;
  i32 replaySize;
  i32 compressedSize;
  ReplayHeaderAndData replayCopy;
  f32 slowdown;
  DWORD local_10;
  ReplayManager *local_c;
  i32 i;

  if (g_ReplayManager == NULL) {
    return;
  }
  local_c = g_ReplayManager;
  if (g_ReplayManager->isDemo != 0)
    goto LAB_004444a9;
  if (((g_GameManager.flags & 1) == 0) && ((i32)g_GameManager.difficulty < 4)) {
    bool bVar9 = true;

    for (i32 i = 0; i < 14; i++) {
      if (((u32 *)&g_Supervisor.cfg)[i] !=
          ((u32 *)&g_ReplayManager->data->data.cfg)[i]) {
        bVar9 = false;
        break;
      }
    }

    if (bVar9)
      goto LAB_00443e18;
  } else {
  LAB_00443e18:
    if ((g_ReplayManager->data->data.cfg.slowMode == 0) &&
        (param_1 != NULL)) {
      DebugPrint("info : Replay File write %s\r\n", param_1);
      replayData = (u8 *)malloc(0x100000);
      replayCopy = *local_c->data;
      StopRecording();
      i = g_GameManager.currentStage - 1;
      if (6 < i) {
        i = 6;
      }
      local_c->data->head.stageReplayData[i].data->score =
          g_GameManager.globals->score;
      replaySize = sizeof(ReplayHeaderAndData);
      for (i = 0; i < 7; i += 1) {
        if (local_c->data->head.stageReplayData[i].data != NULL) {
          memcpy((StageReplayData *)(replayData + replaySize -
                                     sizeof(ReplayHeader)),
                 local_c->data->head.stageReplayData[i].data,
                 (i32)((u8 *)local_c->replayInputsByStage[i] -
                       local_c->data->head.stageReplayData[i].offset));
          replayCopy.head.stageReplayData[i].offset = replaySize;
          replaySize += (i32)((u8 *)local_c->replayInputsByStage[i] -
                              local_c->data->head.stageReplayData[i].offset);
        }
      }
      for (i = 0; i < 7; i += 1) {
        if (local_c->data->head.stageEndData[i].data != NULL) {
          memcpy((StageReplayData *)(replayData + replaySize -
                                     sizeof(ReplayHeader)),
                 local_c->data->head.stageEndData[i].data,
                 local_c->replayDataEndPointers[i] -
                     local_c->data->head.stageEndData[i].offset);
          replayCopy.head.stageEndData[i].offset = replaySize;
          replaySize += local_c->replayDataEndPointers[i] -
                        local_c->data->head.stageEndData[i].offset;
        }
      }
      replayCopy.data.score = g_GameManager.globals->guiScore;
      slowdown =
          g_Supervisor.framerateMultiplier / g_Supervisor.fpsAccumulator - 0.5f;
      slowdown = slowdown + slowdown;
      if (0.0f <= slowdown) {
        if (1.0f <= slowdown) {
          slowdown = 1.0f;
        }
      } else {
        slowdown = 0.0f;
      }
      replayCopy.data.slowdownRate = (1.0f - slowdown) * 100.0f;
      replayCopy.head.replaySize = replaySize;
      strcpy(replayCopy.data.name, param_2);
      ResultScreen::GetDate(replayCopy.data.date);
      replayCopy.head.key = ((u32)g_Rng.GetRandomU16() % 0x80) + 0x40;
      local_154 = (u8)((u32)g_Rng.GetRandomU16() % 0x100);
      replayCopy.data.rngValue3 = local_154;
      local_158 = (u8)((u32)g_Rng.GetRandomU16() % 0x100);
      replayCopy.head.rngValue1 = local_158;
      replayCopy.data.slowdownRate2 = replayCopy.data.slowdownRate + 1.12f;
      replayCopy.data.slowdownRate3 = replayCopy.data.slowdownRate + 2.34f;
      replayCopy.data.magic30 = 0x1e;
      memcpy(replayData, &replayCopy.data.rngValue3, sizeof(ReplayData));
      DebugPrint("info : original size %d\r\n", replaySize);
      replayCopy.head.sizeWithoutHeader = replaySize - sizeof(ReplayHeader);
      lpBuffer = Lzss::Compress(replayData, replayCopy.head.sizeWithoutHeader,
                                &compressedSize);
      replayCopy.head.compressedSize = compressedSize;
      free(replayData);
      local_11c = &replayCopy.head.key;
      local_118 = 0x3f000318;
      for (i = 0; i < 0x47; i += 1) {
        local_118 += (u32)*local_11c;
        local_11c = local_11c + 1;
      }
      local_11c = lpBuffer;
      for (i = 0; i < replayCopy.head.compressedSize; i += 1) {
        local_118 += (u32)*local_11c;
        local_11c = local_11c + 1;
      }
      replayCopy.head.checksum = local_118;
      local_124 = (u8 *)&replayCopy.head.replaySize;
      local_11d = replayCopy.head.key;
      for (i = 0; i < 0x44; i += 1) {
        *local_124 = *local_124 + local_11d;
        local_11d += 7;
        local_124 = local_124 + 1;
      }
      local_124 = lpBuffer;
      for (i = 0; i < compressedSize; i += 1) {
        *local_124 = *local_124 + local_11d;
        local_11d += 7;
        local_124 = local_124 + 1;
      }
      hFile = CreateFileA(param_1, GENERIC_WRITE, 0, NULL, 2, 0x80, NULL);
      if (hFile != INVALID_HANDLE_VALUE) {
        WriteFile(hFile, &replayCopy, sizeof(ReplayHeader), &local_10, NULL);
        WriteFile(hFile, lpBuffer, compressedSize, &local_10, NULL);
        CloseHandle(hFile);
        DebugPrint("info : Size %d -> %d\r\n", replaySize,
                          compressedSize + sizeof(ReplayHeader));
        GlobalFree(lpBuffer);
      }
    }
  }
  for (i = 0; i < 7; i += 1) {
    if (g_ReplayManager->data->head.stageReplayData[i].data != NULL) {
      free(g_ReplayManager->data->head.stageReplayData[i].data);
    }
    if (local_c->data->head.stageEndData[i].data != NULL) {
      free(g_ReplayManager->data->head.stageEndData[i].data);
    }
  }
LAB_004444a9:
  g_Chain.Cut(g_ReplayManager->calcChain);
}

void ReplayManager::SaveReplay2(const char *param_1)

{
  u8 *src;
  u8 *pbVar5;
  HANDLE hFile;
  u8 local_138;
  u8 local_134;
  i32 *local_124;
  u8 local_11d;
  u8 *local_11c;
  i32 local_118;
  i32 local_114;
  ReplayHeaderAndData replayCopy;
  DWORD local_10;
  ReplayManager *local_c;
  u32 i;

  if (g_ReplayManager == NULL) {
    return;
  }
  local_c = g_ReplayManager;
  if (((g_GameManager.flags & 1) == 0) && (g_GameManager.difficulty < 4)) {
    if (memcmp(&g_Supervisor.cfg, &g_ReplayManager->data->data.cfg,
               sizeof(GameConfiguration)) != 0)
      goto LAB_00444a3e;
  }
  if ((g_ReplayManager->data->data.cfg.slowMode == 0) && (param_1 != NULL)) {
    DebugPrint("info : Replay File rewrite %s\r\n", param_1);
    src = (u8 *)malloc(0x100000);
    replayCopy = *local_c->data;
    i = g_GameManager.currentStage - 1;
    if (6 < (i32)i) {
      i = 6;
    }
    local_c->data->head.stageReplayData[i].data->score =
        g_GameManager.globals->score;
    local_114 = sizeof(ReplayHeaderAndData);
    for (i = 0; (i32)i < 7; i += 1) {
      if (local_c->data->head.stageReplayData[i].data != NULL) {
        memcpy((StageReplayData *)(src - sizeof(ReplayHeader) + local_114),
               local_c->data->head.stageReplayData[i].data,
               local_c->stageReplayDataSize[i]);
        replayCopy.head.stageReplayData[i].offset = local_114;
        local_114 += local_c->stageReplayDataSize[i];
      }
    }
    for (i = 0; (i32)i < 7; i += 1) {
      if (local_c->data->head.stageEndData[i].data != NULL) {
        memcpy((StageReplayData *)(src - sizeof(ReplayHeader) + local_114),
               local_c->data->head.stageEndData[i].data,
               local_c->stageEndDataSize[i]);
        replayCopy.head.stageEndData[i].offset = local_114;
        local_114 += local_c->stageEndDataSize[i];
      }
    }
    replayCopy.data.score = g_GameManager.globals->guiScore;
    replayCopy.head.replaySize = local_114;
    replayCopy.head.key = ((u32)g_Rng.GetRandomU16() % 0x80) + 0x40;
    local_134 = (u8)((u32)g_Rng.GetRandomU16() % 0x100);
    replayCopy.data.rngValue3 = local_134;
    local_138 = (u8)((u32)g_Rng.GetRandomU16() % 0x100);
    replayCopy.head.rngValue1 = local_138;
    replayCopy.data.slowdownRate2 = replayCopy.data.slowdownRate + 1.12f;
    replayCopy.data.slowdownRate3 = replayCopy.data.slowdownRate + 2.34f;
    replayCopy.data.magic30 = 0x1e;
    memcpy(src, &replayCopy.data.rngValue3, sizeof(ReplayData));
    DebugPrint("info : original size %d\r\n", local_114);
    replayCopy.head.sizeWithoutHeader = local_114 - sizeof(ReplayHeader);
    pbVar5 = Lzss::Compress(src, replayCopy.head.sizeWithoutHeader,
                            &replayCopy.head.compressedSize);
    free(src);
    local_11c = &replayCopy.head.key;
    local_118 = 0x3f000318;
    for (i = 0; i < 0x47; i += 1) {
      local_118 += (u32)*local_11c;
      local_11c = local_11c + 1;
    }
    local_11c = pbVar5;
    for (i = 0; (i32)i < replayCopy.head.compressedSize; i += 1) {
      local_118 += (u32)*local_11c;
      local_11c = local_11c + 1;
    }
    replayCopy.head.checksum = local_118;
    local_124 = &replayCopy.head.replaySize;
    local_11d = replayCopy.head.key;
    for (i = 0; i < 0x44; i += 1) {
      *(u8 *)local_124 = (char)*local_124 + local_11d;
      local_11d += 7;
      local_124 = (i32 *)((i32)local_124 + 1);
    }
    local_124 = (i32 *)pbVar5;
    for (i = 0; (i32)i < replayCopy.head.compressedSize; i += 1) {
      *(u8 *)local_124 = (u8)*local_124 + local_11d;
      local_11d += 7;
      local_124 = (i32 *)((i32)local_124 + 1);
    }
    hFile = CreateFileA(param_1, GENERIC_WRITE, 0, NULL, 2, 0x80, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
      WriteFile(hFile, &replayCopy, sizeof(ReplayHeader), &local_10, NULL);
      WriteFile(hFile, pbVar5, replayCopy.head.compressedSize, &local_10, NULL);
      CloseHandle(hFile);
      DebugPrint("info : Size %d -> %d\r\n", local_114,
                        replayCopy.head.compressedSize + sizeof(ReplayHeader));
      GlobalFree(pbVar5);
    }
  }
LAB_00444a3e:
  g_Chain.Cut(g_ReplayManager->calcChain);
}
