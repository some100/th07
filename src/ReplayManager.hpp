#pragma once

#include "Chain.hpp"
#include "Supervisor.hpp"
#include "inttypes.hpp"

struct ReplayDataInput
{
    u16 frameNum;
    u16 inputKey;
};
static_assert(sizeof(ReplayDataInput) == 0x4);

struct StageReplayData
{
    u32 score;
    i32 pointItemsCollectedForExtend;
    i32 cherry;
    i32 cherryMax;
    i32 cherryPlus;
    i32 grazeInTotal;
    i32 extendsFromPointItems;
    i32 nextNeededPointItemsForExtend;
    i16 stageRngSeed;
    u8 currentPower;
    u8 livesRemaining;
    u8 bombsRemaining;
    u8 rank;
    u8 powerItemCountForScore;
    u8 spellCardsCaptured;
    i32 unused_28;
    ReplayDataInput replayInputs[115189];
};
static_assert(sizeof(StageReplayData) == 0x70800);

#define REPLAY_STAGE_COUNT 7

struct ReplayHeader
{
    u32 magic;
    u16 version;
    u8 pad1[2];
    i32 checksum;
    u8 rngValue1;
    u8 key;
    u8 pad2[2];
    i32 replaySize;
    i32 compressedSize;
    i32 sizeWithoutHeader;
    u32 stageReplayDataOffsets[REPLAY_STAGE_COUNT];
    u32 stageEndDataOffsets[REPLAY_STAGE_COUNT];
};
static_assert(sizeof(ReplayHeader) == 0x54);

struct ReplayData
{
    u8 rngValue3;
    char versionChar1;
    u8 shotType;
    u8 difficulty;
    char date[6];
    char name[12];
    u16 replayVersion;
    i32 score;
    GameConfiguration cfg;
    i32 unused_a8[8];
    f32 slowdownRate2;
    f32 slowdownRate;
    f32 slowdownRate3;
    i32 magic30;
    i32 exeSize;
    i32 exeChecksum;
    char replayStr[4];
    i16 versionChar2;
};
static_assert(sizeof(ReplayData) == 0x94);

struct ReplayFile
{
    ReplayHeader head;
    ReplayData data;
    StageReplayData *stageReplayData[7];
    StageReplayData *stageEndData[7];
    u8 *rawData;
};

struct ReplayManager
{
    ReplayManager()
    {
    }

    static ZunResult RegisterChain(i32 isDemo, const char *replayFilename);

    static ZunResult AddedCallback(ReplayManager *arg);
    static ZunResult AddedCallbackDemo(ReplayManager *arg);
    static ZunResult DeletedCallback(ReplayManager *arg);
    static u32 OnUpdate(ReplayManager *arg);
    static u32 OnUpdateDemoHighPrio(ReplayManager *arg);
    static u32 OnUpdateDemoLowPrio(ReplayManager *arg);
    static u32 OnUpdateRng(ReplayManager *arg);

    static void SaveReplay(const char *filename, char *replayName);
    static void SaveReplay2(const char *filename);
    static void StopRecording();
    static ReplayFile *ValidateReplayData(ReplayFile *data, i32 size);
    static void FreeReplay(ReplayFile *replay);

    i32 StageReplayExists(i32 stage)
    {
        return this->data->stageReplayData[stage] != NULL;
    }

    i32 IsDemo()
    {
        return this->isDemo;
    }

    i32 frameId;
    ReplayFile *data;
    i32 stageReplayDataSize[REPLAY_STAGE_COUNT];
    i32 stageEndDataSize[REPLAY_STAGE_COUNT];
    void *unused_40;
    i32 isDemo;
    const char *replayFilename;
    u8 unused_4c[54];
    i16 unused_82;
    ReplayDataInput *replayInputs;
    ReplayDataInput *replayInputsByStage[REPLAY_STAGE_COUNT];
    u8 *fpsCursor;
    StageReplayData *stageReplayData;
    uintptr_t replayDataEndPointers[REPLAY_STAGE_COUNT];
    ChainElem *calcChain;
    ChainElem *drawChain;
    ChainElem *demoCalcChain;
    ChainElem *rngCalcChain;
    u16 rngSeed;
    u16 replayEventFlags;
};

extern ReplayManager *g_ReplayManager;
