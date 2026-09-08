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
#include "dsutil.hpp"
#include "pbg4/Lzss.hpp"

// GLOBAL: TH07 0x004b9e48
ReplayManager *g_ReplayManager;

// FUNCTION: TH07 0x00442c60
u32 ReplayManager::OnUpdateRng(ReplayManager *arg)
{
    arg->replayEventFlags = 0;
    arg->rngSeed = g_Rng.seed;
    g_Rng.generationCount = 0;
    if (g_GameManager.isPaused)
    {
        arg->replayEventFlags |= 256;
    }
    g_GameManager.isPaused = FALSE;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00442cd0
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
    if (stage >= REPLAY_STAGE_COUNT) // PHANTASMSTAGE
    {
        stage = 6; // EXTRASTAGE
    }
    g_CurFrameGameInput = curInput = g_CurFrameRawInput;
    arg->replayInputs++;
    arg->replayInputsByStage[stage] = arg->replayInputs + 1;
    arg->replayInputs->frameNum = curInput;
    arg->replayInputs->inputKey = arg->replayEventFlags;
    if (arg->frameId % 30 == 0)
    {
        *(u8 *)&arg->stageReplayData->score =
            (u8)g_Supervisor.curFps |
            ((g_Supervisor.timingErrorCount != 0) ? 128 : 0);
        *((u8 *)&arg->stageReplayData->score + 1) = (u8)g_Supervisor.curFps;
        arg->replayDataEndPointers[stage] =
            (i32)((i32)&arg->stageReplayData->score + 2);
        arg->stageReplayData =
            (StageReplayData *)((i32)&arg->stageReplayData->score + 1);
    }
    arg->frameId++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00442e50
u32 ReplayManager::OnUpdateDemoLowPrio(ReplayManager *arg)
{
    if (!g_GameManager.notInMenu)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    if (g_Gui.HasCurrentMsgIdx() &&
        g_Gui.IsDialogueSkippable() &&
        arg->frameId % 3 != 2)
    {
        return CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB;
    }
    if (g_GameManager.replayStage == 2 &&
        !g_EnemyManager.HasActiveBoss() &&
        arg->frameId % 5 != 4)
    {
        return CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB;
    }

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00442ee0
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

    i32 idk = 0;
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
        g_Supervisor.curFps =
            (i16) * (char *)((i32)&arg->stageReplayData->score + 1) & 0x7f;
        g_Supervisor.isFpsBad =
            (i32) * (char *)((i32)&arg->stageReplayData->score + 1) >> 7;
        arg->stageReplayData =
            (StageReplayData *)((i32)&arg->stageReplayData->score + 1);
    }
    arg->frameId = arg->frameId + 1;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(replayData, i, endData, prevData)
// FUNCTION: TH07 0x00443040
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
        // STRING: TH07 0x00496aa8
        arg->data->head.magic = *(u32 *)&"T7RP";
        arg->data->data.shotType = g_GameManager.shotTypeAndCharacter;
        arg->data->head.version = 0x1100;
        arg->data->data.replayVersion = 256;
        arg->data->data.versionChar1 = 'b';
        // STRING: TH07 0x00497228
        strcpy(arg->data->data.replayStr, "0100b");
        arg->data->data.exeSize = g_Supervisor.exeSize;
        arg->data->data.exeChecksum = g_Supervisor.exeChecksum;
        arg->data->data.difficulty = g_GameManager.difficulty;
        // STRING: TH07 0x00496aa0
        memcpy(arg->data->data.name, "NO NAME", 4);
        arg->data->data.cfg = *g_GameManager.defaultCfg;
        for (i = 0; i < REPLAY_STAGE_COUNT; i++)
        {
            arg->data->head.stageReplayData[i].data = NULL;
            arg->data->head.stageEndData[i].data = NULL;
        }
    }
    else if (g_GameManager.currentStage - 2 >= 0)
    {
        prevData = arg->data->head.stageReplayData[g_GameManager.currentStage - 2].data;
        if (prevData)
        {
            prevData->score = g_GameManager.globals->score;
        }
    }
    i = g_GameManager.currentStage - 1;
    if (i >= REPLAY_STAGE_COUNT) // PHANTASMSTAGE
    {
        i = 6; // EXTRASTAGE
    }
    if (arg->data->head.stageReplayData[i].data)
    {
        ZunMemory::Free(arg->data->head.stageReplayData[i].data);
    }
    if (arg->data->head.stageEndData[i].data)
    {
        ZunMemory::Free(arg->data->head.stageEndData[i].data);
    }
    arg->data->head.stageReplayData[i].data =
        (StageReplayData *)ZunMemory::Alloc2(sizeof(StageReplayData));
    arg->data->head.stageEndData[i].data =
        (StageReplayData *)ZunMemory::Alloc2(sizeof(StageReplayData));

    replayData = arg->data->head.stageReplayData[i].data;
    endData = arg->data->head.stageEndData[i].data;

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
    replayData->nextNeededPointItemsForExtend = g_GameManager.globals->nextNeededPointItemsForExtend;

    arg->replayInputs = replayData->replayInputs;
    arg->stageReplayData = endData;
    arg->replayInputs->frameNum = 0;
    arg->unused_82 = 0;
    return ZUN_SUCCESS;
}

#pragma var_order(dataDecompressed, i, curData, obfOffset, curByte, \
                  csum, csumPtr)
// FUNCTION: TH07 0x004433b0
ReplayFile *
ReplayManager::ValidateReplayData(ReplayFile *data, i32 size)
{
    ReplayFile *dataDecompressed;
    ReplayFile *curData = data;
    u8 *csumPtr;
    i32 csum;
    u8 *curByte;
    u8 obfOffset;
    i32 i;

    if (!curData)
    {
        goto bad;
    }

    if (curData->head.magic != *(u32 *)&"T7RP")
    {
        goto bad;
    }

    if (curData->head.version != 0x1100)
    {
        goto bad;
    }

    curByte = (u8 *)&curData->head.replaySize;
    obfOffset = curData->head.key;
    for (i = 0; i < size - 16; i++, curByte++)
    {
        *curByte -= obfOffset;
        obfOffset += 7;
    }
    csumPtr = &curData->head.key;
    csum = 0x3f000318;
    for (i = 0; i < size - 13; i++, csumPtr++)
    {
        csum += (u32)*csumPtr;
    }
    if (csum != curData->head.checksum)
    {
        goto bad;
    }
    dataDecompressed = (ReplayFile *)ZunMemory::Alloc(curData->head.sizeWithoutHeader +
                                                      sizeof(ReplayHeader));
    memcpy(dataDecompressed, data, sizeof(ReplayHeader));
    Lzss::Decompress(&curData->data.rngValue3, curData->head.compressedSize,
                     &dataDecompressed->data.rngValue3, curData->head.sizeWithoutHeader);

    curData = dataDecompressed;

    if (curData->data.cfg.slowMode)
    {
        goto bad;
    }

    if (g_Supervisor.CheckIntegrity(
            curData->data.replayStr, curData->data.exeSize,
            curData->data.exeChecksum) != ZUN_SUCCESS)
    {
        goto bad;
    }
    ZunMemory::Free(data);
    return dataDecompressed;

bad:
    ZunMemory::Free(data);
    return NULL;
}

#pragma var_order(replayData, i, endData)
// FUNCTION: TH07 0x00443550
ZunResult ReplayManager::AddedCallbackDemo(ReplayManager *arg)
{
    StageReplayData *endData;
    i32 i;
    StageReplayData *replayData;

    arg->frameId = 0;
    if (!arg->data)
    {
        arg->data = (ReplayFile *)FileSystem::OpenFile(
            arg->replayFilename, !g_GameManager.demo);
        arg->data = ValidateReplayData(arg->data, g_LastFileSize);
        if (!arg->data)
        {
            return ZUN_ERROR;
        }
        arg->unused_40 = NULL;
        for (i = 0; i < REPLAY_STAGE_COUNT; i++)
        {
            arg->stageReplayDataSize[i] = 0;
            arg->stageEndDataSize[i] = 0;
            if (arg->data->head.stageReplayData[i].offset != 0)
            {
                if (i < 6 && arg->data->head.stageReplayData[i + 1].offset != 0)
                {
                    arg->stageReplayDataSize[i] =
                        arg->data->head.stageReplayData[i + 1].offset -
                        arg->data->head.stageReplayData[i].offset;
                }
                else
                {
                    arg->stageReplayDataSize[i] =
                        arg->data->head.stageEndData[i].offset -
                        arg->data->head.stageReplayData[i].offset;
                }
                if (i < 6 && arg->data->head.stageEndData[i + 1].offset != 0)
                {
                    arg->stageEndDataSize[i] =
                        arg->data->head.stageEndData[i + 1].offset -
                        arg->data->head.stageEndData[i].offset;
                }
                else
                {
                    arg->stageEndDataSize[i] =
                        arg->data->head.sizeWithoutHeader + sizeof(ReplayHeader) -
                        arg->data->head.stageEndData[i].offset;
                }
            }

            if (arg->data->head.stageReplayData[i].offset != 0)
            {
                arg->data->head.stageReplayData[i].data =
                    (StageReplayData *)(arg->data->head.stageReplayData[i].offset +
                                        (i32)arg->data);
            }
            if (arg->data->head.stageEndData[i].offset != 0)
            {
                arg->data->head.stageEndData[i].data =
                    (StageReplayData *)(arg->data->head.stageEndData[i].offset +
                                        (i32)arg->data);
            }
        }
    }
    i = g_GameManager.currentStage - 1;
    if (i >= REPLAY_STAGE_COUNT) // PHANTASMSTAGE
    {
        i = 6; // EXTRASTAGE
    }
    if (!arg->data->head.stageReplayData[i].data)
    {
        return ZUN_ERROR;
    }

    replayData = arg->data->head.stageReplayData[i].data;
    endData = arg->data->head.stageEndData[i].data;

    g_GameManager.character = arg->data->data.shotType / 2;
    g_GameManager.shotType = arg->data->data.shotType % 2;
    g_GameManager.shotTypeAndCharacter = arg->data->data.shotType;
    g_GameManager.difficulty = arg->data->data.difficulty;
    g_GameManager.globals->pointItemsCollectedForExtend =
        replayData->pointItemsCollectedForExtend;
    g_GameManager.rank.rank = replayData->rank;
    g_GameManager.SetLivesRemaining(replayData->livesRemaining);
    g_GameManager.RegenerateGameIntegrityCsum();
    g_GameManager.SetBombsRemainingAndComputeCsum(replayData->bombsRemaining);
    g_GameManager.SetCurrentPower(replayData->currentPower);
    g_GameManager.RegenerateGameIntegrityCsum();
    g_GameManager.globals->grazeInTotal = replayData->grazeInTotal;
    arg->replayInputs = replayData->replayInputs;
    g_GameManager.powerItemCountForScore = replayData->powerItemCountForScore;
    g_GameManager.cherry = replayData->cherry +
                           g_GameManager.globals->cherryStart;
    g_GameManager.cherryMax = replayData->cherryMax +
                              g_GameManager.globals->cherryStart;
    g_GameManager.cherryPlus = replayData->cherryPlus +
                               g_GameManager.globals->cherryStart;
    if (g_GameManager.cherryPlus >= g_GameManager.globals->cherryStart + 50000)
    {
        g_GameManager.cherryPlus = g_GameManager.globals->cherryStart + 50000;
        g_Player.ActivateBorder();
    }
    *g_GameManager.defaultCfg = arg->data->data.cfg;
    g_Rng.SetSeed(replayData->stageRngSeed);
    g_GameManager.globals->spellCardsCaptured =
        replayData->spellCardsCaptured;
    g_GameManager.globals->extendsFromPointItems =
        replayData->extendsFromPointItems;
    g_GameManager.globals->nextNeededPointItemsForExtend =
        replayData->nextNeededPointItemsForExtend;
    arg->stageReplayData = endData;
    if (g_GameManager.currentStage >= STAGE2 &&
        g_GameManager.currentStage <= STAGE6 &&
        arg->data->head.stageReplayData[g_GameManager.currentStage - 2].data)
    {
        g_GameManager.globals->guiScore = g_GameManager.globals->score =
            arg->data->head.stageReplayData[g_GameManager.currentStage - 2]
                .data->score;
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x004439b0
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
    ZunMemory::Free(g_ReplayManager->data);
    if (arg->unused_40)
    {
        ZunMemory::Free(arg->unused_40);
    }

    delete g_ReplayManager;

    // ZUN bloat: This is doing the exact same thing twice
    g_ReplayManager = NULL;
    g_ReplayManager = NULL;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00443aa0
ZunResult ReplayManager::RegisterChain(ZunBool isDemo, const char *replayFilename)
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
            // ReplayManager::OnDraw is almost certainly folded with
            // EffectManager::UpdateNoOp, but I couldn't get it to fold
            // from the (admittedly very little) attempts I put on it, so just
            // do this
            mgr->drawChain = g_Chain.CreateElem(
                (ChainCallback)EffectManager::UpdateNoOp);
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
            mgr->drawChain =
                g_Chain.CreateElem((ChainCallback)EffectManager::UpdateNoOp);
            mgr->calcChain->arg = mgr;
            if (g_Chain.AddToCalcChain(mgr->calcChain, 5))
            {
                return ZUN_ERROR;
            }

            mgr->demoCalcChain =
                g_Chain.CreateElem((ChainCallback)OnUpdateDemoLowPrio);
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

// FUNCTION: TH07 0x00443d30
void ReplayManager::StopRecording()
{
    ReplayManager *mgr = g_ReplayManager;

    if (mgr)
    {
        mgr->replayInputs++;
        mgr->replayInputs->frameNum = 0;
        i32 stage = g_GameManager.currentStage - 1;
        if (stage >= REPLAY_STAGE_COUNT) // PHANTASMSTAGE
        {
            stage = 6; // EXTRASTAGE
        }
        mgr->replayInputsByStage[stage] = mgr->replayInputs + 1;
    }
}

#pragma var_order(i, mgr, bytesWritten, lpBuffer, slowdown, compressedSize, \
                  stageSize, replayData, replayCopy, hFile, replaySize,     \
                  csum, csumPtr, obfOffset, curByte)
// FUNCTION: TH07 0x00443da0
void ReplayManager::SaveReplay(const char *filename, char *replayName)
{
    u8 *curByte;
    u8 obfOffset;
    u8 *csumPtr;
    i32 csum;
    i32 replaySize;
    HANDLE hFile;
    ReplayFile replayCopy;
    u8 *replayData;
    i32 stageSize;
    i32 compressedSize;
    f32 slowdown;
    u8 *lpBuffer;
    DWORD bytesWritten;
    ReplayManager *mgr;
    i32 i;

    if (g_ReplayManager)
    {
        mgr = g_ReplayManager;
        if (!mgr->IsDemo())
        {
            if (!g_GameManager.practice &&
                g_GameManager.difficulty < 4 &&
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
                // STRING: TH07 0x00496a80
                DebugPrint("info : Replay File write %s\r\n", filename);
                replayData = (u8 *)ZunMemory::Alloc2(0x100000);
                replayCopy = *mgr->data;
                StopRecording();
                i = g_GameManager.currentStage - 1;
                if (i >= REPLAY_STAGE_COUNT) // PHANTASMSTAGE
                {
                    i = 6; // EXTRASTAGE
                }
                mgr->data->head.stageReplayData[i].data->score =
                    g_GameManager.globals->score;
                replaySize = sizeof(ReplayHeader);
                replaySize += sizeof(ReplayData);
                for (i = 0; i < REPLAY_STAGE_COUNT; i++)
                {
                    if (mgr->data->head.stageReplayData[i].data)
                    {
                        stageSize = (u32)mgr->replayInputsByStage[i] -
                                    (u32)mgr->data->head.stageReplayData[i].data;
                        memcpy((StageReplayData *)(replayData + replaySize -
                                                   sizeof(ReplayHeader)),
                               mgr->data->head.stageReplayData[i].data,
                               stageSize);
                        replayCopy.head.stageReplayData[i].offset = replaySize;
                        replaySize += stageSize;
                    }
                }
                for (i = 0; i < REPLAY_STAGE_COUNT; i++)
                {
                    if (mgr->data->head.stageEndData[i].data)
                    {
                        stageSize = (u32)mgr->replayDataEndPointers[i] -
                                    (u32)mgr->data->head.stageEndData[i].data;
                        memcpy((StageReplayData *)(replayData + replaySize -
                                                   sizeof(ReplayHeader)),
                               mgr->data->head.stageEndData[i].data,
                               stageSize);
                        replayCopy.head.stageEndData[i].offset = replaySize;
                        replaySize += stageSize;
                    }
                }
                replayCopy.data.score = g_GameManager.globals->guiScore;
                slowdown =
                    (g_Supervisor.framerateMultiplier /
                         g_Supervisor.fpsAccumulator -
                     0.5f) *
                    2.0f;
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
                // STRING: TH07 0x00496a64
                DebugPrint("info : original size %d\r\n", replaySize);
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
                hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, 2, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile == INVALID_HANDLE_VALUE)
                {
                    // empty branch
                }
                else
                {
                    WriteFile(hFile, &replayCopy, sizeof(ReplayHeader), &bytesWritten, NULL);
                    WriteFile(hFile, lpBuffer, compressedSize, &bytesWritten, NULL);
                    CloseHandle(hFile);
                    // STRING: TH07 0x00496a4c
                    DebugPrint("info : Size %d -> %d\r\n", replaySize,
                               compressedSize + sizeof(ReplayHeader));
                    GlobalFree(lpBuffer);
                }
            }
        SKIP_WRITE:
            for (i = 0; i < REPLAY_STAGE_COUNT; i++)
            {
                if (g_ReplayManager->data->head.stageReplayData[i].data)
                {
                    ZunMemory::Free(g_ReplayManager->data->head.stageReplayData[i].data);
                }
                if (mgr->data->head.stageEndData[i].data)
                {
                    ZunMemory::Free(g_ReplayManager->data->head.stageEndData[i].data);
                }
            }
        }
        g_Chain.Cut(g_ReplayManager->calcChain);
    }
}

#pragma var_order(i, mgr, bytesWritten, lpBuffer, compressedSize, stageSize, \
                  replayData, replayCopy, hFile, replaySize, csum,           \
                  csumPtr, obfOffset, curByte)
// FUNCTION: TH07 0x004444d0
void ReplayManager::SaveReplay2(const char *filename)
{
    u8 *curByte;
    u8 obfOffset;
    u8 *csumPtr;
    u32 csum;
    i32 replaySize;
    HANDLE hFile;
    ReplayFile replayCopy;
    u8 *replayData;
    i32 stageSize;
    i32 compressedSize;
    u8 *lpBuffer;
    DWORD bytesWritten;
    ReplayManager *mgr;
    i32 i;

    if (g_ReplayManager)
    {
        mgr = g_ReplayManager;
        if (!g_GameManager.practice &&
            g_GameManager.difficulty < 4 &&
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
            // STRING: TH07 0x00496a2c
            DebugPrint("info : Replay File rewrite %s\r\n", filename);
            replayData = (u8 *)ZunMemory::Alloc2(0x100000);
            replayCopy = *mgr->data;
            i = g_GameManager.currentStage - 1;
            if (i >= REPLAY_STAGE_COUNT) // PHANTASMSTAGE
            {
                i = 6; // EXTRASTAGE
            }
            mgr->data->head.stageReplayData[i].data->score =
                g_GameManager.globals->score;
            replaySize = sizeof(ReplayHeader);
            replaySize += sizeof(ReplayData);
            for (i = 0; i < REPLAY_STAGE_COUNT; i++)
            {
                if (mgr->data->head.stageReplayData[i].data)
                {
                    stageSize = mgr->stageReplayDataSize[i];
                    memcpy((StageReplayData *)(replayData +
                                               replaySize -
                                               sizeof(ReplayHeader)),
                           mgr->data->head.stageReplayData[i].data,
                           stageSize);
                    replayCopy.head.stageReplayData[i].offset = replaySize;
                    replaySize += stageSize;
                }
            }
            for (i = 0; i < REPLAY_STAGE_COUNT; i++)
            {
                if (mgr->data->head.stageEndData[i].data)
                {
                    stageSize = mgr->stageEndDataSize[i];
                    memcpy((StageReplayData *)(replayData +
                                               replaySize -
                                               sizeof(ReplayHeader)),
                           mgr->data->head.stageEndData[i].data,
                           stageSize);
                    replayCopy.head.stageEndData[i].offset = replaySize;
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
            DebugPrint("info : original size %d\r\n", replaySize);
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
            hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, 2, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                // empty branch
            }
            else
            {
                WriteFile(hFile, &replayCopy, sizeof(ReplayHeader), &bytesWritten, NULL);
                WriteFile(hFile, lpBuffer, compressedSize, &bytesWritten, NULL);
                CloseHandle(hFile);
                DebugPrint("info : Size %d -> %d\r\n", replaySize,
                           compressedSize + sizeof(ReplayHeader));
                GlobalFree(lpBuffer);
            }
        }
    SKIP_WRITE:
        g_Chain.Cut(g_ReplayManager->calcChain);
    }
}
