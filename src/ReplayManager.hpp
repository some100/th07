#pragma once

#include "Chain.hpp"
#include "Supervisor.hpp"
#include "inttypes.hpp"

struct ReplayDataInput {
  u16 frameNum;
  u16 inputKey;
};

struct StageReplayData {
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

struct ReplayHeader {
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
  StageReplayDataUnion stageReplayData[7];
  StageReplayDataUnion stageEndData[7];
};
C_ASSERT(sizeof(ReplayHeader) == 0x54);

struct ReplayData {
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

struct ReplayHeaderAndData {
  ReplayHeader head;
  ReplayData data;
};
C_ASSERT(sizeof(ReplayHeaderAndData) == 0xe8);

struct ReplayManager {
  static ZunResult RegisterChain(i32 isDemo, const char *replayFilename);

  static ZunResult AddedCallback(ReplayManager *arg);
  static ZunResult AddedCallbackDemo(ReplayManager *arg);
  static ZunResult DeletedCallback(ReplayManager *arg);
  static u32 OnUpdate(ReplayManager *arg);
  static u32 OnUpdateDemoHighPrio(ReplayManager *arg);
  static u32 OnUpdateDemoLowPrio(ReplayManager *arg);
  static u32 OnUpdateRng(ReplayManager *arg);

  static i32 CanSaveReplay();
  static void SaveReplay(const char *param_1, char *param_2);
  static void SaveReplay2(const char *param_1);
  static void StopRecording();
  static ReplayHeaderAndData *ValidateReplayData(ReplayHeaderAndData *data,
                                                 i32 size);

  i32 frameId;
  ReplayHeaderAndData *data;
  i32 stageReplayDataSize[7];
  i32 stageEndDataSize[7];
  void *unused_40;
  i32 isDemo;
  const char *replayFilename;
  u8 unused_4c[54];
  i16 unused_82;
  ReplayDataInput *replayInputs;
  ReplayDataInput *replayInputsByStage[7];
  StageReplayData *stageReplayData;
  i32 replayDataEndPointers[7];
  ChainElem *calcChain;
  ChainElem *drawChain;
  ChainElem *demoCalcChain;
  ChainElem *rngCalcChain;
  u16 rngSeed;
  u16 replayEventFlags;
};
C_ASSERT(sizeof(ReplayManager) == 0xd8);

extern ReplayManager *g_ReplayManager;
