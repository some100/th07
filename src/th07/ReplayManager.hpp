#pragma once

#include "Chain.hpp"
#include "Supervisor.hpp"
#include "inttypes.hpp"

struct ReplayDataInput
{
    u16 frameNum;
    u16 inputKey;
};

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
C_ASSERT(sizeof(StageReplayData) == 0x70800);

union StageReplayDataUnion {
    StageReplayData *data;
    i32 offset;
};

#define REPLAY_STAGE_COUNT 7

struct ReplayHeader
{
    u32 magic;
    u16 version;
    // pad 2
    i32 checksum;
    u8 rngValue1;
    u8 key;
    // pad 2
    i32 replaySize;
    i32 compressedSize;
    i32 sizeWithoutHeader;
    StageReplayDataUnion stageReplayData[REPLAY_STAGE_COUNT];
    StageReplayDataUnion stageEndData[REPLAY_STAGE_COUNT];
};
C_ASSERT(sizeof(ReplayHeader) == 0x54);

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
    // pad 3
};
C_ASSERT(sizeof(ReplayData) == 0x94);

struct ReplayFile
{
    ReplayHeader head;
    ReplayData data;
};
C_ASSERT(sizeof(ReplayFile) == 0xe8);

struct ReplayManager
{
    ReplayManager()
    {
    }

    static ZunResult RegisterChain(ZunBool isDemo, const char *replayFilename);

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
    static ReplayFile *ValidateReplayData(ReplayFile *data,
                                          i32 size);

    ZunBool StageReplayExists(i32 stage)
    {
        return this->data->head.stageReplayData[stage].data != NULL;
    }

    ZunBool IsDemo()
    {
        return this->isDemo;
    }

    i32 frameId;
    ReplayFile *data;
    i32 stageReplayDataSize[REPLAY_STAGE_COUNT];
    i32 stageEndDataSize[REPLAY_STAGE_COUNT];
    void *unused_40;
    ZunBool isDemo;
    const char *replayFilename;
    u8 unused_4c[54];
    i16 unused_82;
    ReplayDataInput *replayInputs;
    ReplayDataInput *replayInputsByStage[REPLAY_STAGE_COUNT];
    StageReplayData *stageReplayData;
    i32 replayDataEndPointers[REPLAY_STAGE_COUNT];
    ChainElem *calcChain;
    ChainElem *drawChain;
    ChainElem *demoCalcChain;
    ChainElem *rngCalcChain;
    u16 rngSeed;
    u16 replayEventFlags;
};
C_ASSERT(sizeof(ReplayManager) == 0xd8);
extern ReplayManager *g_ReplayManager;
