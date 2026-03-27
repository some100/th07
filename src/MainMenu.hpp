#pragma once

#include "AnmVm.hpp"
#include "Chain.hpp"
#include "ReplayManager.hpp"
#include "Supervisor.hpp"

typedef enum GameState {
    STATE_PRE_INPUT=0,
    STATE_OPTIONS=2,
    STATE_KEY_CONFIG=3,
    STATE_NORMAL_SELECT_DIFFICULTY=4,
    STATE_NORMAL_SELECT_CHARACTER=5,
    STATE_NORMAL_SELECT_SHOTTYPE=6,
    STATE_SELECT_REPLAY=7,
    STATE_PRACTICE_SELECT_DIFFICULTY=8,
    STATE_PRACTICE_SELECT_CHARACTER=9,
    STATE_PRACTICE_SELECT_SHOTTYPE=10,
    STATE_SELECT_PRACTICE_STAGE=11,
    STATE_EXTRA_SELECT_DIFFICULTY=12,
    STATE_EXTRA_SELECT_CHARACTER=13,
    STATE_EXTRA_SELECT_SHOTTYPE=14
} GameState;

struct MainMenu {
  MainMenu();

  static void InitializeTimingVars(Supervisor *arg);
  static ZunResult RegisterChain();

  static ZunResult AddedCallback(MainMenu *arg);
  static ZunResult DeletedCallback(MainMenu *arg);
  static u32 OnUpdate(MainMenu *arg);
  static u32 OnDraw(MainMenu *arg);

  u32 OnUpdatePreInput();
  u32 OnUpdateOptionsMenu();
  u32 OnUpdateKeyConfig();

  u32 OnUpdateSelectDifficulty();
  u32 OnUpdateSelectCharacter();
  u32 OnUpdateSelectShotType();

  u32 OnUpdateSelectReplay();
  u32 OnUpdateSelectPracticeStage();

  i32 DrawReplayMenu();
  i32 DrawPracticeMenu();

  ZunResult ActualAddedCallback();
  i32 MoveCursorHorizontal(i32 param_1);
  i32 MoveCursorVertical(i32 param_1);
  ZunResult Release();
  void SetGameState(GameState gameState);
  void SwapMapping(i16 btnPressed,i16 oldMapping);
  static ZunResult UpdateMenuDigits(AnmVm *param_1, i16 param_2);

  i32 cursor;
  i32 selected;
  i32 menuSubState;
  i32 inputDelayTimer;
  i32 unused_10[21];
  i32 prevGameState;
  i32 isPracticeMode;
  char replayFilenames[60][512];
  char replayLabels[60][8];
  ReplayHeaderAndData replays[60];
  ReplayHeaderAndData *currentReplay;
  i32 replayPage;
  i32 replayFilesNum;
  i32 chosenReplay;
  i32 selectedStage;
  i32 idleFrames;
  AnmVm *vmHead;
  AnmVm *cursorVm;
  AnmVm vms[14];
  i32 vmCount;
  i32 gameState;
  i32 stateTimer;
  i32 demoFramesCount;
  ChainElem *calcChain;
  ChainElem *drawChain;
  i16 controlMapping[9];
  // pad 2
  GameConfiguration cfg;
};
C_ASSERT(sizeof(MainMenu) == 0xd158);
