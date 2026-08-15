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
#include "dxutil.hpp"
#include "pbg4/Lzss.hpp"
#include <cstdio>

ReplayManager *g_ReplayManager;

u32 ReplayManager::OnUpdateRng(ReplayManager *arg)
{
    arg->replayEventFlags = 0;
    arg->rngSeed = g_Rng.seed;
    g_Rng.generationCount = 0;
    if (g_GameManager.isPaused)
    {
        arg->replayEventFlags |= 256;
    }
    g_GameManager.isPaused = 0;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

u32 ReplayManager::OnUpdate(ReplayManager *arg)
{
    u16 curInput;
    i32 stage;

    if (!g_GameManager.notInMenu)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    g_LastFrameGameInput = g_CurFrameGameInput;
    g_CurFrameGameInput = g_CurFrameRawInput;
    if (g_GameManager.defaultCfg->slowMode)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    if (g_Supervisor.timingBad)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    stage = g_GameManager.currentStage - 1;
    if (stage >= 7)
    {
        stage = 6;
    }
    g_CurFrameGameInput = curInput = g_CurFrameRawInput;
    arg->replayInputs++;
    arg->replayInputsByStage[stage] = arg->replayInputs + 1;
    arg->replayInputs->frameNum = curInput;
    arg->replayInputs->inputKey = arg->replayEventFlags;
    if (arg->frameId % 30 == 0)
    {
        *arg->fpsCursor = (u8)(g_Supervisor.curFps > 60 ? 60 : g_Supervisor.curFps) |
                          ((g_Supervisor.timingErrorCount != 0) ? 128 : 0);
        *(arg->fpsCursor + 1) = (u8)(g_Supervisor.curFps > 60 ? 60 : g_Supervisor.curFps);
        arg->replayDataEndPointers[stage] = (uintptr_t)(arg->fpsCursor + 2);
        arg->fpsCursor++;
    }
    arg->frameId++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

u32 ReplayManager::OnUpdateDemoLowPrio(ReplayManager *arg)
{
    if (!g_GameManager.notInMenu)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    if (g_Gui.HasCurrentMsgIdx() && g_Gui.IsDialogueSkippable() && arg->frameId % 3 != 2)
    {
        return CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB;
    }
    if (g_GameManager.replayStage == 2 && !g_EnemyManager.HasActiveBoss() && arg->frameId % 5 != 4)
    {
        return CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB;
    }

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

u32 ReplayManager::OnUpdateDemoHighPrio(ReplayManager *arg)
{
    if (!g_GameManager.notInMenu)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    if (g_GameManager.defaultCfg->slowMode)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    g_LastFrameGameInput = g_CurFrameGameInput;
    g_CurFrameGameInput = arg->replayInputs->frameNum;
    arg->replayInputs = arg->replayInputs + 1;
    g_IsEighthFrameOfHeldInput = 0;
    if (g_LastFrameGameInput == g_CurFrameGameInput)
    {
        if (g_NumOfFramesInputsWereHeld >= 30)
        {
            if (g_NumOfFramesInputsWereHeld % 8 == 0)
            {
                g_IsEighthFrameOfHeldInput = 1;
            }
            if (g_NumOfFramesInputsWereHeld >= 38)
            {
                g_NumOfFramesInputsWereHeld = 30;
            }
        }
        g_NumOfFramesInputsWereHeld++;
    }
    else
    {
        g_NumOfFramesInputsWereHeld = 0;
    }
    if (arg->frameId % 30 == 0)
    {
        g_Supervisor.curFps = (i16) * (arg->fpsCursor + 1) & 0x7f;
        g_Supervisor.isFpsBad = (i32) * (arg->fpsCursor + 1) >> 7;
        arg->fpsCursor++;
    }
    arg->frameId = arg->frameId + 1;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult ReplayManager::AddedCallback(ReplayManager *arg)
{
    StageReplayData *prevData;
    StageReplayData *endData;
    i32 i;
    StageReplayData *replayData;

    arg->frameId = 0;
    arg->unused_40 = NULL;
    if (!arg->data)
    {
        arg->data = new ReplayFile;
        memset(arg->data, 0, sizeof(ReplayFile));
        memcpy(&arg->data->head.magic, "T7RP", 4);
        arg->data->data.shotType = g_GameManager.shotTypeAndCharacter;
        arg->data->head.version = 0x1100;
        arg->data->data.replayVersion = 256;
        arg->data->data.versionChar1 = 'b';
        memcpy(arg->data->data.replayStr, "0100", 4);
        arg->data->data.versionChar2 = 'b';
        arg->data->data.exeSize = g_Supervisor.exeSize;
        arg->data->data.exeChecksum = g_Supervisor.exeChecksum;
        arg->data->data.difficulty = g_GameManager.difficulty;
        memcpy(arg->data->data.name, "NO NAME", 4);
        arg->data->data.cfg = *g_GameManager.defaultCfg;
        for (i = 0; i < 7; i++)
        {
            arg->data->stageReplayData[i] = NULL;
            arg->data->stageEndData[i] = NULL;
        }
    }
    else if (g_GameManager.currentStage - 2 >= 0)
    {
        prevData = arg->data->stageReplayData[g_GameManager.currentStage - 2];
        if (prevData)
        {
            prevData->score = g_GameManager.globals->score;
        }
    }
    i = g_GameManager.currentStage - 1;
    if (i >= 7)
    {
        i = 6;
    }
    SAFE_FREE(arg->data->stageReplayData[i]);
    SAFE_FREE(arg->data->stageEndData[i]);
    arg->data->stageReplayData[i] = (StageReplayData *)malloc(sizeof(StageReplayData));
    arg->data->stageEndData[i] = (StageReplayData *)malloc(sizeof(StageReplayData));

    replayData = arg->data->stageReplayData[i];
    endData = arg->data->stageEndData[i];

    replayData->grazeInTotal = g_GameManager.globals->grazeInTotal;
    replayData->bombsRemaining = g_GameManager.globals->bombsRemaining;
    replayData->livesRemaining = g_GameManager.globals->livesRemaining;
    replayData->currentPower = g_GameManager.globals->currentPower;
    replayData->rank = g_GameManager.rank.rank;
    replayData->pointItemsCollectedForExtend = g_GameManager.globals->pointItemsCollectedForExtend;
    replayData->stageRngSeed = g_GameManager.stageRngSeed;
    replayData->powerItemCountForScore = g_GameManager.powerItemCountForScore;
    replayData->cherry = g_GameManager.cherry - g_GameManager.globals->cherryStart;
    replayData->cherryMax = g_GameManager.cherryMax - g_GameManager.globals->cherryStart;
    replayData->cherryPlus = g_GameManager.cherryPlus - g_GameManager.globals->cherryStart;
    replayData->spellCardsCaptured = (u8)g_GameManager.globals->spellCardsCaptured;
    replayData->extendsFromPointItems = g_GameManager.globals->extendsFromPointItems;
    replayData->nextNeededPointItemsForExtend =
        g_GameManager.globals->nextNeededPointItemsForExtend;

    arg->replayInputs = replayData->replayInputs;
    arg->stageReplayData = endData;
    arg->fpsCursor = (u8 *)&endData->score;
    arg->replayInputs->frameNum = 0;
    arg->unused_82 = 0;
    return ZUN_SUCCESS;
}

void ReplayManager::FreeReplay(ReplayFile *replay)
{
    if (replay)
    {
        free(replay->rawData);
        delete replay;
    }
}

ReplayFile *ReplayManager::ValidateReplayData(ReplayFile *data, i32 size)
{
    ReplayFile *parsed;
    u8 *dataDecompressed;
    u8 *csumPtr;
    i32 csum;
    u8 *curByte;
    u8 obfOffset;
    i32 i;

    u8 *rawFile = (u8 *)data;
    ReplayHeader *rawHead = (ReplayHeader *)rawFile;

    if (!rawFile)
    {
        return NULL;
    }

    u32 magicT7RP;
    memcpy(&magicT7RP, "T7RP", 4);

    if (rawHead->magic != magicT7RP)
    {
        goto bad;
    }

    if (rawHead->version != 0x1100)
    {
        goto bad;
    }

    curByte = rawFile + offsetof(ReplayHeader, replaySize);
    obfOffset = rawHead->key;
    for (i32 i = 0; i < size - 16; i++, curByte++)
    {
        *curByte -= obfOffset;
        obfOffset += 7;
    }

    csumPtr = &rawHead->key;
    csum = 0x3f000318;
    for (i32 i = 0; i < size - 13; i++, csumPtr++)
    {
        csum += (u32)*csumPtr;
    }
    if (csum != rawHead->checksum)
    {
        goto bad;
    }

    dataDecompressed = (u8 *)malloc(rawHead->sizeWithoutHeader + sizeof(ReplayHeader));
    memcpy(dataDecompressed, rawHead, sizeof(ReplayHeader));
    Lzss::Decompress(rawFile + sizeof(ReplayHeader), rawHead->compressedSize,
                     dataDecompressed + sizeof(ReplayHeader), rawHead->sizeWithoutHeader);

    parsed = new ReplayFile;
    parsed->head = *(ReplayHeader *)dataDecompressed;
    parsed->data = *(ReplayData *)(dataDecompressed + sizeof(ReplayHeader));
    parsed->rawData = dataDecompressed;

    for (i = 0; i < 7; i++)
    {
        if (parsed->head.stageReplayDataOffsets[i] != 0)
        {
            parsed->stageReplayData[i] =
                (StageReplayData *)(dataDecompressed + parsed->head.stageReplayDataOffsets[i]);
        }
        else
        {
            parsed->stageReplayData[i] = NULL;
        }

        if (parsed->head.stageEndDataOffsets[i] != 0)
        {
            parsed->stageEndData[i] =
                (StageReplayData *)(dataDecompressed + parsed->head.stageEndDataOffsets[i]);
        }
        else
        {
            parsed->stageEndData[i] = NULL;
        }
    }

    if (parsed->data.cfg.slowMode)
    {
        FreeReplay(parsed);
        goto bad;
    }

    free(rawFile);
    return parsed;

bad:
    free(rawFile);
    return NULL;
}

ZunResult ReplayManager::AddedCallbackDemo(ReplayManager *arg)
{
    StageReplayData *endData;
    i32 i;
    StageReplayData *replayData;

    arg->frameId = 0;
    if (!arg->data)
    {
        arg->data = (ReplayFile *)FileSystem::OpenFile(arg->replayFilename, !g_GameManager.demo);
        arg->data = ValidateReplayData(arg->data, g_LastFileSize);
        if (!arg->data)
        {
            return ZUN_ERROR;
        }
        arg->unused_40 = NULL;
        for (i = 0; i < 7; i++)
        {
            arg->stageReplayDataSize[i] = 0;
            arg->stageEndDataSize[i] = 0;
            if (arg->data->head.stageReplayDataOffsets[i] != 0)
            {
                if (i < 6 && arg->data->head.stageReplayDataOffsets[i + 1] != 0)
                {
                    arg->stageReplayDataSize[i] = arg->data->head.stageReplayDataOffsets[i + 1] -
                                                  arg->data->head.stageReplayDataOffsets[i];
                }
                else
                {
                    arg->stageReplayDataSize[i] = arg->data->head.stageEndDataOffsets[i] -
                                                  arg->data->head.stageReplayDataOffsets[i];
                }
                if (i < 6 && arg->data->head.stageEndDataOffsets[i + 1] != 0)
                {
                    arg->stageEndDataSize[i] = arg->data->head.stageEndDataOffsets[i + 1] -
                                               arg->data->head.stageEndDataOffsets[i];
                }
                else
                {
                    arg->stageEndDataSize[i] = arg->data->head.sizeWithoutHeader +
                                               sizeof(ReplayHeader) -
                                               arg->data->head.stageEndDataOffsets[i];
                }
            }

            if (arg->data->head.stageReplayDataOffsets[i] != 0)
            {
                arg->data->stageReplayData[i] =
                    (StageReplayData *)(arg->data->head.stageReplayDataOffsets[i] +
                                        arg->data->rawData);
            }
            if (arg->data->head.stageEndDataOffsets[i] != 0)
            {
                arg->data->stageEndData[i] =
                    (StageReplayData *)(arg->data->head.stageEndDataOffsets[i] +
                                        arg->data->rawData);
            }
        }
    }
    i = g_GameManager.currentStage - 1;
    if (i >= 7)
    {
        i = 6;
    }
    if (!arg->data->stageReplayData[i])
    {
        return ZUN_ERROR;
    }

    replayData = arg->data->stageReplayData[i];
    endData = arg->data->stageEndData[i];

    g_GameManager.character = arg->data->data.shotType / 2;
    g_GameManager.shotType = arg->data->data.shotType % 2;
    g_GameManager.shotTypeAndCharacter = arg->data->data.shotType;
    g_GameManager.difficulty = arg->data->data.difficulty;
    g_GameManager.globals->pointItemsCollectedForExtend = replayData->pointItemsCollectedForExtend;
    g_GameManager.rank.rank = replayData->rank;
    g_GameManager.SetLivesRemaining(replayData->livesRemaining);
    g_GameManager.RegenerateGameIntegrityCsum();
    g_GameManager.SetBombsRemainingAndComputeCsum(replayData->bombsRemaining);
    g_GameManager.SetCurrentPower(replayData->currentPower);
    g_GameManager.RegenerateGameIntegrityCsum();
    g_GameManager.globals->grazeInTotal = replayData->grazeInTotal;
    arg->replayInputs = replayData->replayInputs;
    g_GameManager.powerItemCountForScore = replayData->powerItemCountForScore;
    g_GameManager.cherry = replayData->cherry + g_GameManager.globals->cherryStart;
    g_GameManager.cherryMax = replayData->cherryMax + g_GameManager.globals->cherryStart;
    g_GameManager.cherryPlus = replayData->cherryPlus + g_GameManager.globals->cherryStart;
    if (g_GameManager.cherryPlus >= g_GameManager.globals->cherryStart + 50000)
    {
        g_GameManager.cherryPlus = g_GameManager.globals->cherryStart + 50000;
        g_Player.ActivateBorder();
    }
    *g_GameManager.defaultCfg = arg->data->data.cfg;
    g_Rng.SetSeed(replayData->stageRngSeed);
    g_GameManager.globals->spellCardsCaptured = replayData->spellCardsCaptured;
    g_GameManager.globals->extendsFromPointItems = replayData->extendsFromPointItems;
    g_GameManager.globals->nextNeededPointItemsForExtend =
        replayData->nextNeededPointItemsForExtend;
    arg->stageReplayData = endData;
    arg->fpsCursor = (u8 *)&endData->score;
    if (g_GameManager.currentStage >= 2 && g_GameManager.currentStage <= 6 &&
        arg->data->stageReplayData[g_GameManager.currentStage - 2])
    {
        g_GameManager.globals->guiScore = g_GameManager.globals->score =
            arg->data->stageReplayData[g_GameManager.currentStage - 2]->score;
    }
    return ZUN_SUCCESS;
}

ZunResult ReplayManager::DeletedCallback(ReplayManager *arg)
{
    g_Chain.Cut(arg->drawChain);
    arg->drawChain = NULL;
    if (arg->demoCalcChain)
    {
        g_Chain.Cut(arg->demoCalcChain);
        arg->demoCalcChain = NULL;
    }
    if (arg->rngCalcChain)
    {
        g_Chain.Cut(arg->rngCalcChain);
        arg->rngCalcChain = NULL;
    }
    if (g_ReplayManager->data && !g_ReplayManager->isDemo)
    {
        for (i32 i = 0; i < 7; i++)
        {
            if (g_ReplayManager->data->stageReplayData[i])
            {
                free(g_ReplayManager->data->stageReplayData[i]);
            }
            if (g_ReplayManager->data->stageEndData[i])
            {
                free(g_ReplayManager->data->stageEndData[i]);
            }
        }
    }
    FreeReplay(g_ReplayManager->data);

    if (arg->unused_40)
    {
        free(arg->unused_40);
    }

    delete g_ReplayManager;
    g_ReplayManager = NULL;

    return ZUN_SUCCESS;
}

ZunResult ReplayManager::RegisterChain(i32 isDemo, const char *replayFilename)
{
    g_LastFrameGameInput = 0;
    g_CurFrameGameInput = 0;
    if (!g_ReplayManager)
    {
        ReplayManager *mgr = new ReplayManager();
        g_ReplayManager = mgr;
        mgr->data = NULL;
        mgr->isDemo = isDemo;
        mgr->replayFilename = replayFilename;
        switch (isDemo)
        {
        case 0:
            mgr->calcChain = g_Chain.CreateElem((ChainCallback)OnUpdate);
            mgr->calcChain->addedCallback = (ChainLifecycleCallback)AddedCallback;
            mgr->calcChain->deletedCallback = (ChainLifecycleCallback)DeletedCallback;
            mgr->drawChain =
                g_Chain.CreateElem((ChainCallback)EffectManager::UpdateNoOp); // idk either bro
            mgr->calcChain->arg = mgr;
            if (g_Chain.AddToCalcChain(mgr->calcChain, 16))
            {
                return ZUN_ERROR;
            }

            mgr->demoCalcChain = NULL;
            mgr->rngCalcChain = g_Chain.CreateElem((ChainCallback)OnUpdateRng);
            mgr->rngCalcChain->arg = mgr;
            g_Chain.AddToCalcChain(mgr->rngCalcChain, 6);
            break;
        case 1:
            mgr->calcChain = g_Chain.CreateElem((ChainCallback)OnUpdateDemoHighPrio);
            mgr->calcChain->addedCallback = (ChainLifecycleCallback)AddedCallbackDemo;
            mgr->calcChain->deletedCallback = (ChainLifecycleCallback)DeletedCallback;
            mgr->drawChain = g_Chain.CreateElem((ChainCallback)EffectManager::UpdateNoOp);
            mgr->calcChain->arg = mgr;
            if (g_Chain.AddToCalcChain(mgr->calcChain, 5))
            {
                return ZUN_ERROR;
            }

            mgr->demoCalcChain = g_Chain.CreateElem((ChainCallback)OnUpdateDemoLowPrio);
            mgr->demoCalcChain->arg = mgr;
            g_Chain.AddToCalcChain(mgr->demoCalcChain, 17);
            mgr->rngCalcChain = NULL;
            break;
        }
        mgr->drawChain->arg = mgr;
        g_Chain.AddToDrawChain(mgr->drawChain, 14);
    }
    else
    {
        switch (isDemo)
        {
        case 0:
            AddedCallback(g_ReplayManager);
            break;
        case 1:
            AddedCallbackDemo(g_ReplayManager);
            break;
        }
    }
    return ZUN_SUCCESS;
}

void ReplayManager::StopRecording()
{
    ReplayManager *mgr = g_ReplayManager;

    if (mgr)
    {
        mgr->replayInputs++;
        mgr->replayInputs->frameNum = 0;
        i32 stage = g_GameManager.currentStage - 1;
        if (stage >= 7)
        {
            stage = 6;
        }
        mgr->replayInputsByStage[stage] = mgr->replayInputs + 1;
    }
}

void ReplayManager::SaveReplay(const char *filename, char *replayName)
{
    u8 *curByte;
    u8 obfOffset;
    u8 *csumPtr;
    i32 csum;
    i32 replaySize;
    SDL_IOStream *file;
    ReplayFile replayCopy;
    u8 *replayData;
    i32 stageSize;
    i32 compressedSize;
    f32 slowdown;
    u8 *lpBuffer;
    ReplayManager *mgr;
    i32 i;

    if (g_ReplayManager)
    {
        mgr = g_ReplayManager;
        if (!mgr->IsDemo())
        {
            if (!g_GameManager.practice && g_GameManager.difficulty < 4 &&
                memcmp(&g_Supervisor.cfg, &mgr->data->data.cfg, sizeof(g_Supervisor.cfg)) != 0)
            {
                goto SKIP_WRITE;
            }
            if (mgr->data->data.cfg.slowMode)
            {
                goto SKIP_WRITE;
            }
            if (filename)
            {
                Supervisor::DebugPrint("info : Replay File write %s\n", filename);
                replayData = (u8 *)malloc(0x100000);
                replayCopy = *mgr->data;
                StopRecording();
                i = g_GameManager.currentStage - 1;
                if (i >= 7)
                {
                    i = 6;
                }
                mgr->data->stageReplayData[i]->score = g_GameManager.globals->score;
                replaySize = sizeof(ReplayHeader);
                replaySize += sizeof(ReplayData);
                for (i = 0; i < 7; i++)
                {
                    if (mgr->data->stageReplayData[i])
                    {
                        stageSize =
                            (u8 *)mgr->replayInputsByStage[i] - (u8 *)mgr->data->stageReplayData[i];
                        memcpy((StageReplayData *)(replayData + replaySize - sizeof(ReplayHeader)),
                               mgr->data->stageReplayData[i], stageSize);
                        replayCopy.head.stageReplayDataOffsets[i] = replaySize;
                        replaySize += stageSize;
                    }
                }
                for (i = 0; i < 7; i++)
                {
                    if (mgr->data->stageEndData[i])
                    {
                        stageSize =
                            (u8 *)mgr->replayDataEndPointers[i] - (u8 *)mgr->data->stageEndData[i];
                        memcpy((StageReplayData *)(replayData + replaySize - sizeof(ReplayHeader)),
                               mgr->data->stageEndData[i], stageSize);
                        replayCopy.head.stageEndDataOffsets[i] = replaySize;
                        replaySize += stageSize;
                    }
                }
                replayCopy.data.score = g_GameManager.globals->guiScore;
                slowdown =
                    (g_Supervisor.framerateMultiplier / g_Supervisor.fpsAccumulator - 0.5f) * 2.0f;
                if (slowdown < 0.0f)
                {
                    slowdown = 0.0f;
                }
                else if (slowdown >= 1.0f)
                {
                    slowdown = 1.0f;
                }
                replayCopy.data.slowdownRate = (1.0f - slowdown) * 100.0f;
                replayCopy.head.replaySize = replaySize;
                strcpy(replayCopy.data.name, replayName);
                ResultScreen::GetDate(replayCopy.data.date);
                replayCopy.head.key = g_Rng.GetRandomU16InRange(128) + 64;
                replayCopy.data.rngValue3 = g_Rng.GetRandomU16InRange(256);
                replayCopy.head.rngValue1 = g_Rng.GetRandomU16InRange(256);
                replayCopy.data.slowdownRate2 = replayCopy.data.slowdownRate + 1.12f;
                replayCopy.data.slowdownRate3 = replayCopy.data.slowdownRate + 2.34f;
                replayCopy.data.magic30 = 30;
                memcpy(replayData, &replayCopy.data.rngValue3, sizeof(ReplayData));
                Supervisor::DebugPrint("info : original size %d\n", replaySize);
                replayCopy.head.sizeWithoutHeader = replaySize - sizeof(ReplayHeader);
                lpBuffer = Lzss::Compress(replayData, replayCopy.head.sizeWithoutHeader,
                                          &replayCopy.head.compressedSize);
                free(replayData);
                compressedSize = replayCopy.head.compressedSize;
                csumPtr = &replayCopy.head.key;
                csum = 0x3f000318;
                for (i = 0; (u32)i < 0x47; i++, csumPtr++)
                {
                    csum += (u32)*csumPtr;
                }
                csumPtr = lpBuffer;
                for (i = 0; i < compressedSize; i++, csumPtr++)
                {
                    csum += (u32)*csumPtr;
                }
                replayCopy.head.checksum = csum;
                curByte = (u8 *)&replayCopy.head.replaySize;
                obfOffset = replayCopy.head.key;
                for (i = 0; (u32)i < 0x44; i++, curByte++)
                {
                    *curByte += obfOffset;
                    obfOffset += 7;
                }
                curByte = lpBuffer;
                for (i = 0; i < compressedSize; i++, curByte++)
                {
                    *curByte += obfOffset;
                    obfOffset += 7;
                }
                file = SDL_IOFromFile(filename, "wb");
                if (file)
                {
                    SDL_WriteIO(file, &replayCopy, sizeof(ReplayHeader));
                    SDL_WriteIO(file, lpBuffer, compressedSize);
                    SDL_CloseIO(file);
                    Supervisor::DebugPrint("info : Size %d -> %d\n", replaySize,
                                           compressedSize + sizeof(ReplayHeader));
                    free(lpBuffer);
                }
            }
        SKIP_WRITE:
            for (i = 0; i < 7; i++)
            {
                SAFE_FREE(g_ReplayManager->data->stageReplayData[i]);
                SAFE_FREE(g_ReplayManager->data->stageEndData[i]);
            }
        }
        g_Chain.Cut(g_ReplayManager->calcChain);
    }
}

void ReplayManager::SaveReplay2(const char *filename)
{
    u8 *curByte;
    u8 obfOffset;
    u8 *csumPtr;
    u32 csum;
    i32 replaySize;
    SDL_IOStream *file;
    ReplayFile replayCopy;
    u8 *replayData;
    i32 stageSize;
    i32 compressedSize;
    u8 *lpBuffer;
    ReplayManager *mgr;
    i32 i;

    if (g_ReplayManager)
    {
        mgr = g_ReplayManager;
        if (!g_GameManager.practice && g_GameManager.difficulty < 4 &&
            memcmp(&g_Supervisor.cfg, &mgr->data->data.cfg, sizeof(g_Supervisor.cfg)) != 0)
        {
            goto SKIP_WRITE;
        }
        if (mgr->data->data.cfg.slowMode)
        {
            goto SKIP_WRITE;
        }
        if (filename)
        {
            Supervisor::DebugPrint("info : Replay File rewrite %s\n", filename);
            replayData = (u8 *)malloc(0x100000);
            replayCopy = *mgr->data;
            i = g_GameManager.currentStage - 1;
            if (i >= 7)
            {
                i = 6;
            }
            mgr->data->stageReplayData[i]->score = g_GameManager.globals->score;
            replaySize = sizeof(ReplayHeader);
            replaySize += sizeof(ReplayData);
            for (i = 0; i < 7; i++)
            {
                if (mgr->data->stageReplayData[i])
                {
                    stageSize = mgr->stageReplayDataSize[i];
                    memcpy((StageReplayData *)(replayData + replaySize - sizeof(ReplayHeader)),
                           mgr->data->stageReplayData[i], stageSize);
                    replayCopy.head.stageReplayDataOffsets[i] = replaySize;
                    replaySize += stageSize;
                }
            }
            for (i = 0; i < 7; i++)
            {
                if (mgr->data->stageEndData[i])
                {
                    stageSize = mgr->stageEndDataSize[i];
                    memcpy((StageReplayData *)(replayData + replaySize - sizeof(ReplayHeader)),
                           mgr->data->stageEndData[i], stageSize);
                    replayCopy.head.stageEndDataOffsets[i] = replaySize;
                    replaySize += stageSize;
                }
            }
            replayCopy.data.score = g_GameManager.globals->guiScore;
            replayCopy.head.replaySize = replaySize;
            replayCopy.head.key = g_Rng.GetRandomU16InRange(128) + 64;
            replayCopy.data.rngValue3 = g_Rng.GetRandomU16InRange(256);
            replayCopy.head.rngValue1 = g_Rng.GetRandomU16InRange(256);
            replayCopy.data.slowdownRate2 = replayCopy.data.slowdownRate + 1.12f;
            replayCopy.data.slowdownRate3 = replayCopy.data.slowdownRate + 2.34f;
            replayCopy.data.magic30 = 30;
            memcpy(replayData, &replayCopy.data.rngValue3, sizeof(ReplayData));
            Supervisor::DebugPrint("info : original size %d\n", replaySize);
            replayCopy.head.sizeWithoutHeader = replaySize - sizeof(ReplayHeader);
            lpBuffer = Lzss::Compress(replayData, replayCopy.head.sizeWithoutHeader,
                                      &replayCopy.head.compressedSize);
            free(replayData);
            compressedSize = replayCopy.head.compressedSize;
            csumPtr = &replayCopy.head.key;
            csum = 0x3f000318;
            for (i = 0; (u32)i < 0x47; i++, csumPtr++)
            {
                csum += (u32)*csumPtr;
            }
            csumPtr = lpBuffer;
            for (i = 0; i < compressedSize; i++, csumPtr++)
            {
                csum += (u32)*csumPtr;
            }
            replayCopy.head.checksum = csum;
            curByte = (u8 *)&replayCopy.head.replaySize;
            obfOffset = replayCopy.head.key;
            for (i = 0; (u32)i < 0x44; i++, curByte++)
            {
                *curByte += obfOffset;
                obfOffset += 7;
            }
            curByte = lpBuffer;
            for (i = 0; i < compressedSize; i++, curByte++)
            {
                *curByte += obfOffset;
                obfOffset += 7;
            }
            file = SDL_IOFromFile(filename, "wb");
            if (file)
            {
                SDL_WriteIO(file, &replayCopy, sizeof(ReplayHeader));
                SDL_WriteIO(file, lpBuffer, compressedSize);
                SDL_CloseIO(file);
                Supervisor::DebugPrint("info : Size %d -> %d\n", replaySize,
                                       compressedSize + sizeof(ReplayHeader));
                free(lpBuffer);
            }
        }
    SKIP_WRITE:
        g_Chain.Cut(g_ReplayManager->calcChain);
    }
}
