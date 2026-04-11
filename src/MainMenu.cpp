#include "MainMenu.hpp"

#include <direct.h>
#include <stdio.h>

#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameManager.hpp"
#include "ReplayManager.hpp"
#include "ScreenEffect.hpp"
#include "SoundPlayer.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"

// GLOBAL: TH07 0x0049f474
i16 g_LastJoystickInput;

// GLOBAL: TH07 0x0049f40c
const char *g_StagePracticeStrings[6] = {
    // STRING: TH07 0x00495520
    "Stage1",
    // STRING: TH07 0x00495518
    "Stage2",
    // STRING: TH07 0x00495510
    "Stage3",
    // STRING: TH07 0x00495508
    "Stage4",
    // STRING: TH07 0x00495500
    "Stage5",
    // STRING: TH07 0x004954f8
    "Stage6",
};

// GLOBAL: TH07 0x0049f424
const char *g_StageReplayStrings[7] = {
    // STRING: TH07 0x004955e0
    "Stage1  ",
    // STRING: TH07 0x004955d4
    "Stage2  ",
    // STRING: TH07 0x004955c8
    "Stage3  ",
    // STRING: TH07 0x004955bc
    "Stage4  ",
    // STRING: TH07 0x004955b0
    "Stage5  ",
    // STRING: TH07 0x004955a4
    "Stage6  ",
    // STRING: TH07 0x004955f8
    "Extra   ",
};

// GLOBAL: TH07 0x0049f440
// STRING: TH07 0x004955ec
const char *g_PhantasmReplayString = "Phantasm";

// GLOBAL: TH07 0x0049f444
const char *g_DifficultyStrings[6] = {
    "Easy    ",
    "Normal  ",
    "Hard    ",
    "Lunatic ",
    "Extra   ",
    "Phantasm",
};

// GLOBAL: TH07 0x0049f45c
const char *g_CharacterAndShottypeReplayStrings[6] = {
    // STRING: TH07 0x00495b28
    "ReimuA ",
    // STRING: TH07 0x00495b20
    "ReimuB ",
    // STRING: TH07 0x00495b18
    "MarisaA",
    // STRING: TH07 0x00495b10
    "MarisaB",
    // STRING: TH07 0x00495b08
    "SakuyaA",
    // STRING: TH07 0x00495b00
    "SakuyaB",
};

// GLOBAL: TH07 0x0049f478
const char *g_KeyConfigStrings[12] = {
    // STRING: TH07 0x004957e8
    "ショット、決定ボタンを設定します",
    // STRING: TH07 0x004957c4
    "ボム、キャンセルボタンを設定します",
    // STRING: TH07 0x004957a8
    "低速移動ボタンを設定します",
    // STRING: TH07 0x00495780
    "メッセージスキップボタンを設定します",
    // STRING: TH07 0x00495764
    "ポーズボタンを設定します",
    // STRING: TH07 0x00495748
    "上移動ボタンを設定します",
    // STRING: TH07 0x0049572c
    "下移動ボタンを設定します",
    // STRING: TH07 0x00495710
    "左移動ボタンを設定します",
    // STRING: TH07 0x004956f4
    "右移動ボタンを設定します",
    // STRING: TH07 0x004956c0
    "ショット押しっぱなしで低速移動になるようにします",
    // STRING: TH07 0x004956ac
    "初期設定に戻します",
    // STRING: TH07 0x00495698
    "おおよそ終了します",
};

// GLOBAL: TH07 0x0049f4a8
const char *g_OptionsStrings[9] = {
    // STRING: TH07 0x0049596c
    "プレイヤーの初期数を変更します。（初期設定　３）",
    // STRING: TH07 0x0049592c
    "画面の色数を変更します。３２ＢＩＴだと最も綺麗に表示されます。",
    // STRING: TH07 0x004958f8
    "ＢＧＭの再生方法を変更します。（初期設定　ＷＡＶ）",
    // STRING: TH07 0x004958d8
    "効果音を再生するか選択します",
    // STRING: TH07 0x004958b0
    "ウィンドウかフルスクリーンか選択します",
    // STRING: TH07 0x00495870
    "弾が多い場面でわざと処理落ちさせます(スコア、リプレイ記録不可)",
    // STRING: TH07 0x00495858
    "全て初期設定にします",
    // STRING: TH07 0x00495834
    "パッド操作のボタン配置を変更します",
    // STRING: TH07 0x0049581c
    "おいそれと終了します",
};

// GLOBAL: TH07 0x0049f4cc
const char *g_MainMenuStrings[8] = {
    // STRING: TH07 0x00495aa4
    "ゲームを開始します",
    // STRING: TH07 0x00495a84
    "エキストラステージを開始します",
    // STRING: TH07 0x00495a60
    "ステージを選択し、練習を開始します",
    // STRING: TH07 0x00495a48
    "リプレイを鑑賞できます",
    // STRING: TH07 0x00495a18
    "過去のスコアやスペルカードの取得歴を見られます",
    // STRING: TH07 0x00495a08
    "音楽を聴けます",
    // STRING: TH07 0x004959f4
    "各種設定できます",
    // STRING: TH07 0x004959dc
    "いろいろと終了します",
};

// GLOBAL: TH07 0x0049ea7c
const char *g_DemoReplayPaths[3] = {
    // STRING: TH07 0x00495ae8
    "data/demo/demorpy0.rpy",
    // STRING: TH07 0x00495ad0
    "data/demo/demorpy1.rpy",
    // STRING: TH07 0x00495ab8
    "data/demo/demorpy2.rpy"};

// FUNCTION: TH07 0x004553fa
void InitializeTimingVars(Supervisor *arg)

{
  arg->timingErrorCount = 0;
  arg->maxTimingError = 0;
  arg->checkTiming = 0;
  arg->timingSpikeAccumulator = 0;
  arg->timingBadCount = 0;
}

// FUNCTION: TH07 0x00455435
void MainMenu::SetGameState(GameState gameState)

{
  this->prevGameState = this->gameState;
  this->gameState = gameState;
  this->inputDelayTimer = 0;
  this->stateTimer = 0;
  this->menuSubState = 0;
  this->idleFrames = 0;
}

// FUNCTION: TH07 0x0045547d
MainMenu::MainMenu() { memset(this, 0, sizeof(MainMenu)); }

// FUNCTION: TH07 0x004554d6
u32 MainMenu::OnUpdate(MainMenu *arg)

{
  u32 result;

  switch (arg->gameState) {
  case STATE_PRE_INPUT:
    result = arg->OnUpdatePreInput();
    break;
  case STATE_OPTIONS:
    result = arg->OnUpdateOptionsMenu();
    break;
  case STATE_KEY_CONFIG:
    result = arg->OnUpdateKeyConfig();
    break;
  case STATE_NORMAL_SELECT_DIFFICULTY:
  case STATE_PRACTICE_SELECT_DIFFICULTY:
  case STATE_EXTRA_SELECT_DIFFICULTY:
    result = arg->OnUpdateSelectDifficulty();
    break;
  case STATE_NORMAL_SELECT_CHARACTER:
  case STATE_PRACTICE_SELECT_CHARACTER:
  case STATE_EXTRA_SELECT_CHARACTER:
    result = arg->OnUpdateSelectCharacter();
    break;
  case STATE_NORMAL_SELECT_SHOTTYPE:
  case STATE_PRACTICE_SELECT_SHOTTYPE:
  case STATE_EXTRA_SELECT_SHOTTYPE:
    result = arg->OnUpdateSelectShotType();
    break;
  case STATE_SELECT_REPLAY:
    result = arg->OnUpdateSelectReplay();
    break;
  case STATE_SELECT_PRACTICE_STAGE:
    result = arg->OnUpdateSelectPracticeStage();
  }
  AnmManager::ExecuteScripts(arg->vmHead, arg->vmCount);
  if (arg->cursorVm != NULL) {
    g_AnmManager->ExecuteScript(arg->cursorVm);
  }

  return result;
}

// FUNCTION: TH07 0x004555dd
u32 MainMenu::OnUpdatePreInput()

{
  i32 iVar7;

  iVar7 = this->menuSubState;
  if (iVar7 != 0) {
    if (iVar7 == 1)
      goto LAB_0045599d;
    if (iVar7 == 2) {
      if (0x3b < this->inputDelayTimer) {
        free(this->vmHead);
        this->vmHead = NULL;
        this->vmHead = NULL;
        this->vmCount = 0;
        this->stateTimer = 0;
        g_Supervisor.curState = -1;
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
      }
    } else if ((iVar7 == 3) && (0x1d < this->inputDelayTimer)) {
      this->prevGameState = this->gameState;
      this->gameState = STATE_OPTIONS;
      this->inputDelayTimer = 0;
      this->stateTimer = 0;
      this->menuSubState = 0;
      this->idleFrames = 0;
      this->cursor = 0;
      this->cfg = g_Supervisor.cfg;
      return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    goto LAB_004561e9;
  }
  if (this->prevGameState == STATE_PRE_INPUT &&
      g_Supervisor.wantedState2 != 5) {
    Supervisor::PlayLoadedAudio(8);
  }
  if (((((this->prevGameState == STATE_PRE_INPUT) ||
         (this->prevGameState == 4)) ||
        (this->prevGameState == STATE_SELECT_REPLAY)) ||
       ((this->prevGameState == 8 ||
         (this->prevGameState == STATE_EXTRA_SELECT_DIFFICULTY)))) &&
      // STRING: TH07 0x004959c4
      (g_AnmManager->LoadSurface(0, "data/title/title00.jpg") != ZUN_SUCCESS)) {
    return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
  }
  if (this->vmCount == 0) {
    this->vmCount = 0xa4;
    this->vmHead = new AnmVm[this->vmCount];
    AnmManager::ExecuteVmsAnms(this->vmHead, 0x900, this->vmCount);
  }
  AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 2);
  for (i32 i = 0; i < 8; i += 1) {
    g_AnmManager->SetActiveSprite(this->vmHead + i + 1,
                                  this->vmHead[i + 1].baseSpriteIdx + 1);
  }
  g_AnmManager->SetActiveSprite(
      this->vmHead + this->cursor + 1,
      (i32)this->vmHead[this->cursor + 1].baseSpriteIdx);
  this->menuSubState = 0;
  this->inputDelayTimer = 0;
  this->selected = -1;
  this->menuSubState = 1;
  this->demoFramesCount = 0;
  if ((g_GameManager.flags >> 3 & 1) != 0) {
    this->prevGameState = this->gameState;
    this->gameState = STATE_SELECT_REPLAY;
    this->inputDelayTimer = 0;
    this->stateTimer = 0;
    this->menuSubState = 0;
    this->idleFrames = 0;
    AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 0xd);
    this->cursorVm->pendingInterrupt = 2;
    g_GameManager.flags = g_GameManager.flags & 0xfffffff7;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
  }
  if (this->isPracticeMode != 0) {
    this->prevGameState = this->gameState;
    this->gameState = STATE_PRACTICE_SELECT_DIFFICULTY;
    this->inputDelayTimer = 0;
    this->stateTimer = 0;
    this->menuSubState = 0;
    this->idleFrames = 0;
    AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 5);
    this->cursorVm->pendingInterrupt = 2;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
  }
  for (i32 i = 0; i < 8; i += 1) {
    g_AnmManager->DrawStringFormat2(this->vms + i, 0xfff0e0, 0x300000,
                                    g_MainMenuStrings[i]);
  }
LAB_0045599d:
  if (iVar7 = MoveCursorVertical(8), iVar7 != 0) {
    while ((g_GameManager.HasReachedMaxClearsAllShotTypes() == 0 &&
            (this->cursor == 1))) {
      this->cursor += iVar7;
    }
    for (i32 i = 0; i < 8; i += 1) {
      g_AnmManager->SetActiveSprite(this->vmHead + i + 1,
                                    this->vmHead[i + 1].baseSpriteIdx + 1);
    }
    g_AnmManager->SetActiveSprite(
        this->vmHead + this->cursor + 1,
        (i32)this->vmHead[this->cursor + 1].baseSpriteIdx);
  }
  this->demoFramesCount = this->demoFramesCount + 1;
  if (g_CurFrameRawInput != 0) {
    this->demoFramesCount = 0;
  }
  if (900 < this->demoFramesCount) {
    g_GameManager.demoIdx = (u8)(g_GameManager.demoIdx + 1) % 3;
    strcpy(g_GameManager.replayFilename,
           g_DemoReplayPaths[g_GameManager.demoIdx]);
    this->currentReplay = (ReplayHeaderAndData *)FileSystem::OpenFile(
        g_GameManager.replayFilename, 0);
    this->currentReplay =
        ReplayManager::ValidateReplayData(this->currentReplay, g_LastFileSize);
    if (this->currentReplay != NULL) {
      g_GameManager.flags = (g_GameManager.flags & 0xfffffff7) | 10;
      g_GameManager.demoFrames = 0;
      g_GameManager.difficulty = this->currentReplay->data.difficulty;
      g_GameManager.character = this->currentReplay->data.shotType / 2;
      g_GameManager.shotType = this->currentReplay->data.shotType % 2;
      g_GameManager.shotTypeAndCharacter = this->currentReplay->data.shotType;
      i32 i = 0;
      while (!this->currentReplay->head.stageReplayData[i].data)
        ++i;

      g_GameManager.currentStage = i;
      free(this->currentReplay);
      this->currentReplay = NULL;
      g_Supervisor.curState = 2;
      g_GameManager.replayStage = 0;
      return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    }
    // STRING: TH07 0x004959a0
    Supervisor::DebugPrint2("error : Demo Play is not ready\r\n");
    this->demoFramesCount = 0;
  }
  if (this->selected != this->cursor) {
    this->cursorVm = this->vms + this->cursor;
    this->cursorVm->pendingInterrupt = 1;
  }
  this->selected = this->cursor;
  if (this->stateTimer < 10)
    goto LAB_004561e9;
  if (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) == 0) ||
      ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) ==
       (g_LastFrameInput & TH_BUTTON_SELECTMENU)))
    goto switchD_00455cdd_default;
  g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
  g_SoundPlayer.ProcessQueues();
  switch (this->cursor) {
  case 0:
    g_GameManager.flags &= 0xfffffffe;
    this->cursor = g_Supervisor.cfg.defaultDifficulty;
    if (3 < this->cursor) {
      this->cursor = 2;
    }
    this->prevGameState = this->gameState;
    this->gameState = STATE_NORMAL_SELECT_DIFFICULTY;
    this->inputDelayTimer = 0;
    this->stateTimer = 0;
    this->menuSubState = 0;
    this->idleFrames = 0;
    AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 5);
    this->cursorVm->pendingInterrupt = 2;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
    break;
  case 1:
    if (g_GameManager.HasReachedMaxClearsAllShotTypes() != 0) {
      g_GameManager.flags &= 0xfffffffe;
      this->cursor = g_Supervisor.cfg.defaultDifficulty == 5;
      this->prevGameState = this->gameState;
      this->gameState = STATE_EXTRA_SELECT_DIFFICULTY;
      this->inputDelayTimer = 0;
      this->stateTimer = 0;
      this->menuSubState = 0;
      this->idleFrames = 0;
      AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 5);
      this->cursorVm->pendingInterrupt = 2;
      return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
  case 2:
    g_GameManager.flags |= 1;
    this->cursor = g_Supervisor.cfg.defaultDifficulty;
    if (3 < this->cursor) {
      this->cursor = 2;
    }
    this->prevGameState = this->gameState;
    this->gameState = STATE_PRACTICE_SELECT_DIFFICULTY;
    this->inputDelayTimer = 0;
    this->stateTimer = 0;
    this->menuSubState = 0;
    this->idleFrames = 0;
    AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 5);
    this->cursorVm->pendingInterrupt = 2;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
    break;
  case 3:
    g_GameManager.flags &= 0xfffffffe;
    this->prevGameState = this->gameState;
    this->gameState = STATE_SELECT_REPLAY;
    this->inputDelayTimer = 0;
    this->stateTimer = 0;
    this->menuSubState = 0;
    this->idleFrames = 0;
    AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 0xd);
    this->cursorVm->pendingInterrupt = 2;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
    break;
  case 4:
    g_Supervisor.curState = 5;
    this->cursorVm->pendingInterrupt = 2;
    return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    break;
  case 5:
    g_Supervisor.curState = 8;
    this->cursorVm->pendingInterrupt = 2;
    return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    break;
  case 6:
    this->menuSubState = 0;
    this->cursor = 0;
    this->stateTimer = 0;
    this->inputDelayTimer = 0;
    this->menuSubState = 3;
    this->inputDelayTimer = 0;
    OnUpdateOptionsMenu();
    this->cursor = 0;
  default:
  switchD_00455cdd_default:
    if (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) != 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) !=
         (g_LastFrameInput & TH_BUTTON_RETURNMENU))) {
      g_AnmManager->SetActiveSprite(
          this->vmHead + this->cursor + 1,
          this->vmHead[this->cursor + 1].baseSpriteIdx + 1);
      this->cursor = 7;
      g_AnmManager->SetActiveSprite(
          this->vmHead + this->cursor + 1,
          (i32)this->vmHead[this->cursor + 1].baseSpriteIdx);
      g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
      g_SoundPlayer.ProcessQueues();
    }
  LAB_004561e9:
    this->idleFrames = this->idleFrames + 1;
    this->inputDelayTimer = this->inputDelayTimer + 1;
    this->stateTimer = this->stateTimer + 1;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
    break;
  case 7:
    this->menuSubState = 2;
    this->inputDelayTimer = 0;
    AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 1);
    if (g_Supervisor.cfg.musicMode == MUSIC_MIDI) {
      g_Supervisor.midiOutput->StopPlayback();
      g_Supervisor.midiOutput->ParseFile(0x1e);
      g_Supervisor.midiOutput->Play();
    }
    goto switchD_00455cdd_default;
  }
}

// FUNCTION: TH07 0x0045624d
u32 MainMenu::OnUpdateOptionsMenu()

{
  if (this->menuSubState == 0) {
    if (this->stateTimer == 0) {
      AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 3);
      for (i32 i = 0; i < 9; i += 1) {
        g_AnmManager->SetActiveSprite(this->vmHead + i + 9,
                                      this->vmHead[i + 9].baseSpriteIdx + 1);
      }
      g_AnmManager->SetActiveSprite(
          this->vmHead + this->cursor + 9,
          (i32)this->vmHead[this->cursor + 9].baseSpriteIdx);
      this->menuSubState = 0;
      this->inputDelayTimer = 0;
      this->selected = -1;
    }
    this->menuSubState = 1;
    for (i32 i = 0; i < 9; i += 1) {
      g_AnmManager->DrawStringFormat2(this->vms + i, 0xfff0e0, 0x300000,
                                      g_OptionsStrings[i]);
    }
  } else if (this->menuSubState != 1)
    goto LAB_00456e08;
  if (MoveCursorVertical(9) != 0) {
    for (i32 i = 0; i < 9; i += 1) {
      g_AnmManager->SetActiveSprite(this->vmHead + i + 9,
                                    this->vmHead[i + 9].baseSpriteIdx + 1);
    }
    g_AnmManager->SetActiveSprite(
        this->vmHead + this->cursor + 9,
        (i32)this->vmHead[this->cursor + 9].baseSpriteIdx);
  }
  if (this->selected != this->cursor) {
    this->cursorVm = this->vms + this->cursor;
    this->cursorVm->pendingInterrupt = 1;
  }
  this->selected = this->cursor;
  for (i32 i = 0x12; i < 0x17; i += 1) {
    g_AnmManager->SetActiveSprite(this->vmHead + i,
                                  this->vmHead[i].baseSpriteIdx + 1);
  }
  g_AnmManager->SetActiveSprite(
      this->vmHead + g_Supervisor.cfg.lifeCount + 0x12,
      (i32)this->vmHead[g_Supervisor.cfg.lifeCount + 0x12].baseSpriteIdx);
  for (i32 i = 0x17; i < 0x19; i += 1) {
    g_AnmManager->SetActiveSprite(this->vmHead + i,
                                  this->vmHead[i].baseSpriteIdx + 1);
  }
  g_AnmManager->SetActiveSprite(
      this->vmHead + g_Supervisor.cfg.colorMode16bit + 0x17,
      (i32)this->vmHead[g_Supervisor.cfg.colorMode16bit + 0x17].baseSpriteIdx);
  for (i32 i = 0x19; i < 0x1c; i += 1) {
    g_AnmManager->SetActiveSprite(this->vmHead + i,
                                  this->vmHead[i].baseSpriteIdx + 1);
  }
  g_AnmManager->SetActiveSprite(
      this->vmHead + g_Supervisor.cfg.musicMode + 0x19,
      (i32)this->vmHead[g_Supervisor.cfg.musicMode + 0x19].baseSpriteIdx);
  for (i32 i = 0x1c; i < 0x1e; i += 1) {
    g_AnmManager->SetActiveSprite(this->vmHead + i,
                                  this->vmHead[i].baseSpriteIdx + 1);
  }
  g_AnmManager->SetActiveSprite(
      this->vmHead + g_Supervisor.cfg.playSounds + 0x1c,
      (i32)this->vmHead[g_Supervisor.cfg.playSounds + 0x1c].baseSpriteIdx);
  for (i32 i = 0x1e; i < 0x20; i += 1) {
    g_AnmManager->SetActiveSprite(this->vmHead + i,
                                  this->vmHead[i].baseSpriteIdx + 1);
  }
  g_AnmManager->SetActiveSprite(
      this->vmHead + g_Supervisor.cfg.windowed + 0x1e,
      (i32)this->vmHead[g_Supervisor.cfg.windowed + 0x1e].baseSpriteIdx);
  for (i32 i = 0x20; i < 0x22; i += 1) {
    g_AnmManager->SetActiveSprite(this->vmHead + i,
                                  this->vmHead[i].baseSpriteIdx + 1);
  }
  g_AnmManager->SetActiveSprite(
      this->vmHead + g_Supervisor.cfg.slowMode + 0x20,
      (i32)this->vmHead[g_Supervisor.cfg.slowMode + 0x20].baseSpriteIdx);
  if (this->stateTimer < 4)
    goto LAB_00456e08;
  if ((((g_CurFrameRawInput & TH_BUTTON_LEFT) != 0) &&
       ((g_CurFrameRawInput & TH_BUTTON_LEFT) !=
        (g_LastFrameInput & TH_BUTTON_LEFT))) ||
      (((g_CurFrameRawInput & TH_BUTTON_LEFT) != 0 &&
        (g_IsEighthFrameOfHeldInput != 0)))) {
    if (this->cursor == 0) {
      if (g_Supervisor.cfg.lifeCount == 0) {
        g_Supervisor.cfg.lifeCount = 4;
      } else {
        g_Supervisor.cfg.lifeCount -= 1;
      }
    } else if (this->cursor == 1) {
      if (g_Supervisor.cfg.colorMode16bit == 0) {
        g_Supervisor.cfg.colorMode16bit = 1;
      } else {
        g_Supervisor.cfg.colorMode16bit -= 1;
      }
    } else if (this->cursor == 2) {
      Supervisor::StopAudio();
      if (g_Supervisor.cfg.musicMode == MUSIC_MIDI) {
        g_Supervisor.midiOutput->StopPlayback();
        g_Supervisor.midiOutput->ParseFile(0x1e);
        g_Supervisor.midiOutput->Play();
      }
      if (g_Supervisor.cfg.musicMode == MUSIC_OFF) {
        g_Supervisor.cfg.musicMode = MUSIC_MIDI;
      } else {
        *(i32 *)&g_Supervisor.cfg.musicMode -= 1;
      }
      if (((g_Supervisor.cfg.opts >> 0xd & 1) == 0) &&
          (g_Supervisor.cfg.musicMode == MUSIC_MIDI)) {
        g_SoundPlayer.StartBGM("thbgm.dat");
      }
      // STRING: TH07 0x0049580c
      Supervisor::LoadAudio(8, "bgm/th07_01.mid");
      Supervisor::PlayLoadedAudio(8);
    } else if (this->cursor == 3) {
      if (g_Supervisor.cfg.playSounds == 0) {
        g_Supervisor.cfg.playSounds = 1;
      } else {
        g_Supervisor.cfg.playSounds -= 1;
      }
    } else if (this->cursor == 4) {
      if (g_Supervisor.cfg.windowed == 0) {
        g_Supervisor.cfg.windowed = 1;
      } else {
        g_Supervisor.cfg.windowed -= 1;
      }
    } else {
      if (this->cursor != 5)
        goto LAB_00456a2d;
      if (g_Supervisor.cfg.slowMode == 0) {
        g_Supervisor.cfg.slowMode = 1;
      } else {
        g_Supervisor.cfg.slowMode -= 1;
      }
    }
    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
    g_SoundPlayer.ProcessQueues();
  }
LAB_00456a2d:
  if ((((g_CurFrameRawInput & TH_BUTTON_RIGHT) != 0) &&
       ((g_CurFrameRawInput & TH_BUTTON_RIGHT) !=
        (g_LastFrameInput & TH_BUTTON_RIGHT))) ||
      (((g_CurFrameRawInput & TH_BUTTON_RIGHT) != 0 &&
        (g_IsEighthFrameOfHeldInput != 0)))) {
    if (this->cursor == 0) {
      if (g_Supervisor.cfg.lifeCount < 4) {
        g_Supervisor.cfg.lifeCount += 1;
      } else {
        g_Supervisor.cfg.lifeCount = 0;
      }
    } else if (this->cursor == 1) {
      g_Supervisor.cfg.colorMode16bit = g_Supervisor.cfg.colorMode16bit == 0;
    } else if (this->cursor == 2) {
      Supervisor::StopAudio();
      if (g_Supervisor.cfg.musicMode < MUSIC_MIDI) {
        *(i32 *)&g_Supervisor.cfg.musicMode += 1;
      } else {
        g_Supervisor.cfg.musicMode = MUSIC_OFF;
      }
      Supervisor::LoadAudio(8, "bgm/th07_01.mid");
      Supervisor::PlayLoadedAudio(8);
    } else if (this->cursor == 3) {
      g_Supervisor.cfg.playSounds = g_Supervisor.cfg.playSounds == 0;
    } else if (this->cursor == 4) {
      g_Supervisor.cfg.windowed = g_Supervisor.cfg.windowed == 0;
    } else {
      if (this->cursor != 5)
        goto LAB_00456bd3;
      g_Supervisor.cfg.slowMode = g_Supervisor.cfg.slowMode == 0;
    }
    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
    g_SoundPlayer.ProcessQueues();
  }
LAB_00456bd3:
  if (g_CurFrameRawInput != 0) {
    this->idleFrames = 0;
  }
  if (this->idleFrames < 0xe10) {
    if (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) != 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) !=
         (g_LastFrameInput & TH_BUTTON_SELECTMENU))) {
      if (this->cursor == 6) {
        g_Supervisor.cfg.lifeCount = 2;
        g_Supervisor.cfg.bombCount = 3;
        g_Supervisor.cfg.musicMode = MUSIC_WAV;
        g_Supervisor.cfg.playSounds = 1;
        g_Supervisor.cfg.slowMode = 0;
        g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
        g_SoundPlayer.ProcessQueues();
      } else {
        if (this->cursor == 7) {
          this->cursor = 0;
          SetGameState(STATE_KEY_CONFIG);
          g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
          g_SoundPlayer.ProcessQueues();
          return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        if (this->cursor == 8)
          goto LAB_00456cc0;
      }
    }
    if (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) != 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) !=
         (g_LastFrameInput & TH_BUTTON_RETURNMENU))) {
      if (this->cursor == 8)
        goto LAB_00456cc0;
      g_AnmManager->SetActiveSprite(
          this->vmHead + this->cursor + 9,
          this->vmHead[this->cursor + 9].baseSpriteIdx + 1);
      this->cursor = 8;
      g_AnmManager->SetActiveSprite(
          this->vmHead + this->cursor + 9,
          (i32)this->vmHead[this->cursor + 9].baseSpriteIdx);
      g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
      g_SoundPlayer.ProcessQueues();
    }
  LAB_00456e08:
    this->idleFrames = this->idleFrames + 1;
    this->inputDelayTimer = this->inputDelayTimer + 1;
    this->stateTimer = this->stateTimer + 1;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
  }
LAB_00456cc0:
  this->cursor = 6;
  SetGameState(STATE_PRE_INPUT);
  g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
  g_SoundPlayer.ProcessQueues();
  if (((this->cfg).colorMode16bit == g_Supervisor.cfg.colorMode16bit) &&
      ((this->cfg).windowed == g_Supervisor.cfg.windowed)) {
    return CHAIN_CALLBACK_RESULT_CONTINUE;
  }
  return CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR;
}

// FUNCTION: TH07 0x00456e40
void MainMenu::SwapMapping(i16 btnPressed, i16 oldMapping)

{
  if (this->controlMapping[0] == btnPressed) {
    this->controlMapping[0] = oldMapping;
  }
  if (this->controlMapping[1] == btnPressed) {
    this->controlMapping[1] = oldMapping;
  }
  if (this->controlMapping[2] == btnPressed) {
    this->controlMapping[2] = oldMapping;
  }
  if (this->controlMapping[4] == btnPressed) {
    this->controlMapping[4] = oldMapping;
  }
  if (this->controlMapping[5] == btnPressed) {
    this->controlMapping[5] = oldMapping;
  }
  if (this->controlMapping[6] == btnPressed) {
    this->controlMapping[6] = oldMapping;
  }
  if (this->controlMapping[7] == btnPressed) {
    this->controlMapping[7] = oldMapping;
  }
  if (this->controlMapping[3] == btnPressed) {
    this->controlMapping[3] = oldMapping;
  }
  if (this->controlMapping[8] == btnPressed) {
    this->controlMapping[8] = oldMapping;
  }
}

// FUNCTION: TH07 0x00456f6b
u32 MainMenu::OnUpdateKeyConfig()

{
  i16 btnPressed;

  if (this->menuSubState == 0) {
    if (this->stateTimer == 0) {
      AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 4);
      for (i32 local_c = 0; local_c < 0xc; local_c += 1) {
        g_AnmManager->SetActiveSprite(
            this->vmHead + local_c + 0x23,
            this->vmHead[local_c + 0x23].baseSpriteIdx + 1);
      }
      g_AnmManager->SetActiveSprite(
          this->vmHead + this->cursor + 0x23,
          (i32)this->vmHead[this->cursor + 0x23].baseSpriteIdx);
      this->menuSubState = 0;
      this->inputDelayTimer = 0;
      this->controlMapping[0] = g_Supervisor.cfg.controllerMapping.shootButton;
      this->controlMapping[1] = g_Supervisor.cfg.controllerMapping.bombButton;
      this->controlMapping[2] = g_Supervisor.cfg.controllerMapping.focusButton;
      this->controlMapping[3] = g_Supervisor.cfg.controllerMapping.menuButton;
      this->controlMapping[4] = g_Supervisor.cfg.controllerMapping.upButton;
      this->controlMapping[5] = g_Supervisor.cfg.controllerMapping.downButton;
      this->controlMapping[6] = g_Supervisor.cfg.controllerMapping.leftButton;
      this->controlMapping[7] = g_Supervisor.cfg.controllerMapping.rightButton;
      this->controlMapping[8] = g_Supervisor.cfg.controllerMapping.skipButton;
      g_Supervisor.cfg.controllerMapping.upButton = 0xffff;
      g_Supervisor.cfg.controllerMapping.downButton = 0xffff;
      UpdateMenuDigits(this->vmHead + 0x2f, this->controlMapping[0]);
      UpdateMenuDigits(this->vmHead + 0x31, this->controlMapping[1]);
      UpdateMenuDigits(this->vmHead + 0x33, this->controlMapping[2]);
      UpdateMenuDigits(this->vmHead + 0x35, this->controlMapping[8]);
      UpdateMenuDigits(this->vmHead + 0x37, this->controlMapping[3]);
      UpdateMenuDigits(this->vmHead + 0x39, this->controlMapping[4]);
      UpdateMenuDigits(this->vmHead + 0x3b, this->controlMapping[5]);
      UpdateMenuDigits(this->vmHead + 0x3d, this->controlMapping[6]);
      UpdateMenuDigits(this->vmHead + 0x3f, this->controlMapping[7]);
      this->selected = -1;
    }
    this->menuSubState = 1;
    for (i32 local_c = 0; local_c < 0xc; local_c += 1) {
      g_AnmManager->DrawStringFormat2(this->vms + local_c, 0xfff0e0, 0x300000,
                                      g_KeyConfigStrings[local_c]);
    }
  } else if (this->menuSubState != 1)
    goto LAB_0045786e;
  if (MoveCursorVertical(0xc) != 0) {
    for (i32 local_c = 0; local_c < 0xc; local_c += 1) {
      g_AnmManager->SetActiveSprite(this->vmHead + local_c + 0x23,
                                    this->vmHead[local_c + 0x23].baseSpriteIdx +
                                        1);
    }
    g_AnmManager->SetActiveSprite(
        this->vmHead + this->cursor + 0x23,
        (i32)this->vmHead[this->cursor + 0x23].baseSpriteIdx);
  }
  if (this->selected != this->cursor) {
    this->cursorVm = this->vms + this->cursor;
    this->cursorVm->pendingInterrupt = 1;
  }
  this->selected = this->cursor;
  UpdateMenuDigits(this->vmHead + 0x2f, this->controlMapping[0]);
  UpdateMenuDigits(this->vmHead + 0x31, this->controlMapping[1]);
  UpdateMenuDigits(this->vmHead + 0x33, this->controlMapping[2]);
  UpdateMenuDigits(this->vmHead + 0x35, this->controlMapping[8]);
  UpdateMenuDigits(this->vmHead + 0x37, this->controlMapping[3]);
  UpdateMenuDigits(this->vmHead + 0x39, this->controlMapping[4]);
  UpdateMenuDigits(this->vmHead + 0x3b, this->controlMapping[5]);
  UpdateMenuDigits(this->vmHead + 0x3d, this->controlMapping[6]);
  UpdateMenuDigits(this->vmHead + 0x3f, this->controlMapping[7]);
  for (i32 local_c = 0x41; local_c < 0x43; local_c += 1) {
    g_AnmManager->SetActiveSprite(this->vmHead + local_c,
                                  this->vmHead[local_c].baseSpriteIdx + 1);
  }
  g_AnmManager->SetActiveSprite(
      this->vmHead + g_Supervisor.cfg.shotSlow + 0x41,
      (i32)this->vmHead[g_Supervisor.cfg.shotSlow + 0x41].baseSpriteIdx);
  btnPressed = 0;
  while ((btnPressed < 0x20 &&
          ((Controller::GetControllerState()[btnPressed] & 0x80) == 0))) {
    btnPressed += 1;
  }
  if ((btnPressed < 0x20) && (g_LastJoystickInput != btnPressed)) {
    switch (this->cursor) {
    case 0:
      SwapMapping(btnPressed, this->controlMapping[0]);
      this->controlMapping[0] = btnPressed;
      break;
    case 1:
      SwapMapping(btnPressed, this->controlMapping[1]);
      this->controlMapping[1] = btnPressed;
      break;
    case 2:
      SwapMapping(btnPressed, this->controlMapping[2]);
      this->controlMapping[2] = btnPressed;
      break;
    case 3:
      SwapMapping(btnPressed, this->controlMapping[8]);
      this->controlMapping[8] = btnPressed;
      break;
    case 4:
      SwapMapping(btnPressed, this->controlMapping[3]);
      this->controlMapping[3] = btnPressed;
      break;
    case 5:
      SwapMapping(btnPressed, this->controlMapping[4]);
      this->controlMapping[4] = btnPressed;
      break;
    case 6:
      SwapMapping(btnPressed, this->controlMapping[5]);
      this->controlMapping[5] = btnPressed;
      break;
    case 7:
      SwapMapping(btnPressed, this->controlMapping[6]);
      this->controlMapping[6] = btnPressed;
      break;
    case 8:
      SwapMapping(btnPressed, this->controlMapping[7]);
      this->controlMapping[7] = btnPressed;
      break;
    default:
      goto switchD_00457548_default;
    }
    g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
    g_SoundPlayer.ProcessQueues();
  }
switchD_00457548_default:
  if ((((g_CurFrameRawInput & TH_BUTTON_LEFT) != 0) &&
       ((g_CurFrameRawInput & TH_BUTTON_LEFT) !=
        (g_LastFrameInput & TH_BUTTON_LEFT))) &&
      (this->cursor == 9)) {
    g_Supervisor.cfg.shotSlow = 1 - g_Supervisor.cfg.shotSlow;
  }
  if ((((g_CurFrameRawInput & TH_BUTTON_RIGHT) != 0) &&
       ((g_CurFrameRawInput & TH_BUTTON_RIGHT) !=
        (g_LastFrameInput & TH_BUTTON_RIGHT))) &&
      (this->cursor == 9)) {
    g_Supervisor.cfg.shotSlow = 1 - g_Supervisor.cfg.shotSlow;
  }
  g_LastJoystickInput = btnPressed;
  if (g_CurFrameRawInput != 0) {
    this->idleFrames = 0;
  }
  if (this->idleFrames < 0xe10) {
    if (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) != 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) !=
         (g_LastFrameInput & TH_BUTTON_SELECTMENU))) {
      if (this->cursor == 10) {
        g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
        g_SoundPlayer.ProcessQueues();
        this->controlMapping[0] = g_ControllerMapping.shootButton;
        this->controlMapping[1] = g_ControllerMapping.bombButton;
        this->controlMapping[2] = g_ControllerMapping.focusButton;
        this->controlMapping[3] = g_ControllerMapping.menuButton;
        this->controlMapping[4] = g_ControllerMapping.upButton;
        this->controlMapping[5] = g_ControllerMapping.downButton;
        this->controlMapping[6] = g_ControllerMapping.leftButton;
        this->controlMapping[7] = g_ControllerMapping.rightButton;
        this->controlMapping[8] = g_ControllerMapping.skipButton;
        g_Supervisor.cfg.shotSlow = 1;
      } else if (this->cursor == 0xb)
        goto LAB_0045782a;
    }
  LAB_0045786e:
    this->idleFrames = this->idleFrames + 1;
    this->inputDelayTimer = this->inputDelayTimer + 1;
    this->stateTimer = this->stateTimer + 1;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
  }
LAB_0045782a:
  g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
  g_SoundPlayer.ProcessQueues();
  SetGameState(STATE_OPTIONS);
  g_Supervisor.cfg.controllerMapping.shootButton = this->controlMapping[0];
  g_Supervisor.cfg.controllerMapping.bombButton = this->controlMapping[1];
  g_Supervisor.cfg.controllerMapping.focusButton = this->controlMapping[2];
  g_Supervisor.cfg.controllerMapping.menuButton = this->controlMapping[3];
  g_Supervisor.cfg.controllerMapping.upButton = this->controlMapping[4];
  g_Supervisor.cfg.controllerMapping.downButton = this->controlMapping[5];
  g_Supervisor.cfg.controllerMapping.leftButton = this->controlMapping[6];
  g_Supervisor.cfg.controllerMapping.rightButton = this->controlMapping[7];
  g_Supervisor.cfg.controllerMapping.skipButton = this->controlMapping[8];
  this->cursor = 7;
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x004578cc
ZunResult MainMenu::UpdateMenuDigits(AnmVm *param_1, i16 param_2)

{
  if (param_2 < 0) {
    param_1->flags = param_1->flags & 0xfffffffd;
    param_1[1].flags = param_1[1].flags & 0xfffffffd;
  } else {
    g_AnmManager->SetActiveSprite(param_1, (i32)param_1->baseSpriteIdx +
                                               ((i32)param_2 / 10) * 2);
    g_AnmManager->SetActiveSprite(param_1 + 1, (i32)param_1[1].baseSpriteIdx +
                                                   ((i32)param_2 % 10) * 2);
    param_1->flags = param_1->flags | 2;
    param_1[1].flags = param_1[1].flags | 2;
  }
  return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0045798b
u32 MainMenu::OnUpdateSelectDifficulty()

{
  i32 local_1c;

  if (this->menuSubState == 0) {
    if (this->stateTimer == 0) {
      if ((((this->prevGameState != 5) && (this->prevGameState != 9)) &&
           (this->prevGameState != STATE_EXTRA_SELECT_CHARACTER)) &&
          (g_AnmManager->LoadSurface(0, "data/title/select00.jpg") !=
           ZUN_SUCCESS)) {
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
      }
      this->cursor = g_Supervisor.cfg.defaultDifficulty;
      if (this->gameState == STATE_EXTRA_SELECT_DIFFICULTY) {
        if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0) {
          AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 0xc);
          this->cursor = 4;
        } else {
          AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 0x16);
        }
      } else {
        AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 7);
      }
      if (this->gameState == STATE_EXTRA_SELECT_DIFFICULTY) {
        this->cursor -= 4;
        if (this->cursor < 0) {
          this->cursor = 0;
        }
        for (i32 local_c = 0; local_c < 2; local_c += 1) {
          g_AnmManager->SetActiveSprite(
              this->vmHead + local_c + 0xa2,
              this->vmHead[local_c + 0xa2].baseSpriteIdx + 1);
        }
        g_AnmManager->SetActiveSprite(
            this->vmHead + this->cursor + 0xa2,
            (i32)this->vmHead[this->cursor + 0xa2].baseSpriteIdx);
      } else {
        if (3 < this->cursor) {
          this->cursor = 1;
        }
        for (i32 local_c = 0; local_c < 4; local_c += 1) {
          g_AnmManager->SetActiveSprite(
              this->vmHead + local_c + 0x43,
              this->vmHead[local_c + 0x43].baseSpriteIdx + 1);
        }
        g_AnmManager->SetActiveSprite(
            this->vmHead + this->cursor + 0x43,
            (i32)this->vmHead[this->cursor + 0x43].baseSpriteIdx);
      }
      this->menuSubState = 0;
      this->inputDelayTimer = 0;
      this->cursorVm = NULL;
    }
    if (this->isPracticeMode != 0) {
      SetGameState(STATE_PRACTICE_SELECT_CHARACTER);
      this->cursor = 0;
      return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    if (this->stateTimer == 0x1e) {
      this->menuSubState = 1;
    }
  } else if (this->menuSubState == 1) {
    if (this->gameState == STATE_EXTRA_SELECT_DIFFICULTY) {
      local_1c = (g_GameManager.HasUnlockedPhantomAndMaxClears() != 0) + 1;
    } else {
      local_1c = 4;
    }
    if (MoveCursorVertical(local_1c) != 0) {
      if (this->gameState == STATE_EXTRA_SELECT_DIFFICULTY) {
        if (local_1c == 2) {
          for (i32 local_c = 0; local_c < 2; local_c += 1) {
            g_AnmManager->SetActiveSprite(
                this->vmHead + local_c + 0xa2,
                this->vmHead[local_c + 0xa2].baseSpriteIdx + 1);
          }
          g_AnmManager->SetActiveSprite(
              this->vmHead + this->cursor + 0xa2,
              (i32)this->vmHead[this->cursor + 0xa2].baseSpriteIdx);
        }
      } else {
        for (i32 local_c = 0; local_c < 4; local_c += 1) {
          g_AnmManager->SetActiveSprite(
              this->vmHead + local_c + 0x43,
              this->vmHead[local_c + 0x43].baseSpriteIdx + 1);
        }
        g_AnmManager->SetActiveSprite(
            this->vmHead + this->cursor + 0x43,
            (i32)this->vmHead[this->cursor + 0x43].baseSpriteIdx);
      }
    }
    if (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) != 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) !=
         (g_LastFrameInput & TH_BUTTON_SELECTMENU))) {
      if (this->gameState == STATE_EXTRA_SELECT_DIFFICULTY) {
        g_Supervisor.cfg.defaultDifficulty = this->cursor + 4;
      } else {
        g_Supervisor.cfg.defaultDifficulty = this->cursor;
      }
      g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
      g_SoundPlayer.ProcessQueues();
      if (this->gameState == STATE_EXTRA_SELECT_DIFFICULTY) {
        SetGameState(STATE_EXTRA_SELECT_CHARACTER);
      } else if ((g_GameManager.flags & 1) == 0) {
        SetGameState(STATE_NORMAL_SELECT_CHARACTER);
      } else {
        SetGameState(STATE_PRACTICE_SELECT_CHARACTER);
      }
      this->cursor = 0;
      return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    if (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) != 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) !=
         (g_LastFrameInput & TH_BUTTON_RETURNMENU))) {
      if (this->gameState == STATE_EXTRA_SELECT_DIFFICULTY) {
        g_Supervisor.cfg.defaultDifficulty = this->cursor + 4;
      } else {
        g_Supervisor.cfg.defaultDifficulty = this->cursor;
      }
      g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
      g_SoundPlayer.ProcessQueues();
      this->menuSubState = 3;
      this->inputDelayTimer = 0;
      AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 6);
    }
  } else if ((this->menuSubState == 3) && (0x1d < this->inputDelayTimer)) {
    SetGameState(STATE_PRE_INPUT);
    if (this->gameState == STATE_EXTRA_SELECT_DIFFICULTY) {
      this->cursor = 1;
    } else if ((g_GameManager.flags & 1) == 0) {
      this->cursor = 0;
    } else {
      this->cursor = 2;
    }
    g_GameManager.flags = g_GameManager.flags & 0xfffffffe;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
  }
  this->inputDelayTimer = this->inputDelayTimer + 1;
  this->idleFrames = this->idleFrames + 1;
  this->stateTimer = this->stateTimer + 1;
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00457fe5
u32 MainMenu::OnUpdateSelectCharacter()

{
  if (this->menuSubState == 0) {
    if (this->stateTimer == 0) {
      AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 8);
      if (g_Supervisor.cfg.defaultDifficulty < 4) {
        this->vmHead[g_Supervisor.cfg.defaultDifficulty + 0x43]
            .pendingInterrupt = 9;
      } else {
        if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0) {
          this->vmHead[0xa1].pendingInterrupt = 9;
        } else {
          this->vmHead[g_Supervisor.cfg.defaultDifficulty + 0x9e]
              .pendingInterrupt = 9;
        }
      }
      this->cursor = g_GameManager.character;
      if (g_Supervisor.cfg.defaultDifficulty == 4) {
        while (
            (g_GameManager.HasReachedMaxClears(this->cursor << 1) == 0 &&
             (g_GameManager.HasReachedMaxClears(this->cursor * 2 + 1) == 0))) {
          this->cursor = this->cursor + 1;
          if (2 < this->cursor) {
            this->cursor = this->cursor - 3;
          }
        }
      } else if (g_Supervisor.cfg.defaultDifficulty == 5) {
        while (
            (g_GameManager.HasUnlockedPhantom(this->cursor << 1) == 0 &&
             (g_GameManager.HasUnlockedPhantom(this->cursor * 2 + 1) == 0))) {
          this->cursor = this->cursor + 1;
          if (2 < this->cursor) {
            this->cursor = this->cursor - 3;
          }
        }
      }
      this->vmHead[0x48].flags = this->vmHead[0x48].flags & 0xfffffffd;
      this->vmHead[0x49].flags = this->vmHead[0x49].flags & 0xfffffffd;
      this->vmHead[0x47].flags = this->vmHead[0x47].flags & 0xfffffffd;
      this->vmHead[0x50].flags = this->vmHead[0x50].flags & 0xfffffffd;
      this->vmHead[0x53].flags = this->vmHead[0x53].flags & 0xfffffffd;
      this->vmHead[0x4b].flags = this->vmHead[0x4b].flags & 0xfffffffd;
      this->vmHead[0x4c].flags = this->vmHead[0x4c].flags & 0xfffffffd;
      this->vmHead[0x4a].flags = this->vmHead[0x4a].flags & 0xfffffffd;
      this->vmHead[0x51].flags = this->vmHead[0x51].flags & 0xfffffffd;
      this->vmHead[0x54].flags = this->vmHead[0x54].flags & 0xfffffffd;
      this->vmHead[0x4e].flags = this->vmHead[0x4e].flags & 0xfffffffd;
      this->vmHead[0x4f].flags = this->vmHead[0x4f].flags & 0xfffffffd;
      this->vmHead[0x4d].flags = this->vmHead[0x4d].flags & 0xfffffffd;
      this->vmHead[0x52].flags = this->vmHead[0x52].flags & 0xfffffffd;
      this->vmHead[0x55].flags = this->vmHead[0x55].flags & 0xfffffffd;
      if (this->cursor == 0) {
        this->vmHead[0x48].flags = this->vmHead[0x48].flags | 2;
        this->vmHead[0x49].flags = this->vmHead[0x49].flags | 2;
        this->vmHead[0x47].flags = this->vmHead[0x47].flags | 2;
        this->vmHead[0x50].flags = this->vmHead[0x50].flags | 2;
        this->vmHead[0x53].flags = this->vmHead[0x53].flags | 2;
      } else if (this->cursor == 1) {
        this->vmHead[0x4b].flags = this->vmHead[0x4b].flags | 2;
        this->vmHead[0x4c].flags = this->vmHead[0x4c].flags | 2;
        this->vmHead[0x4a].flags = this->vmHead[0x4a].flags | 2;
        this->vmHead[0x51].flags = this->vmHead[0x51].flags | 2;
        this->vmHead[0x54].flags = this->vmHead[0x54].flags | 2;
      } else if (this->cursor == 2) {
        this->vmHead[0x4e].flags = this->vmHead[0x4e].flags | 2;
        this->vmHead[0x4f].flags = this->vmHead[0x4f].flags | 2;
        this->vmHead[0x4d].flags = this->vmHead[0x4d].flags | 2;
        this->vmHead[0x52].flags = this->vmHead[0x52].flags | 2;
        this->vmHead[0x55].flags = this->vmHead[0x55].flags | 2;
      }
      if (this->cursor == 0) {
        this->vmHead[0x47].pendingInterrupt = 9;
        this->vmHead[0x4a].pendingInterrupt = 8;
        this->vmHead[0x4d].pendingInterrupt = 8;
        this->vmHead[0x4a].color.bytes.a = 0;
        this->vmHead[0x4d].color.bytes.a = 0;
        this->vmHead[0x50].pendingInterrupt = 9;
        this->vmHead[0x51].pendingInterrupt = 8;
        this->vmHead[0x52].pendingInterrupt = 8;
        this->vmHead[0x51].color.bytes.a = 0;
        this->vmHead[0x52].color.bytes.a = 0;
        this->vmHead[0x53].pendingInterrupt = 9;
        this->vmHead[0x54].pendingInterrupt = 8;
        this->vmHead[0x55].pendingInterrupt = 8;
        this->vmHead[0x54].color.bytes.a = 0;
        this->vmHead[0x55].color.bytes.a = 0;
      } else if (this->cursor == 1) {
        this->vmHead[0x47].pendingInterrupt = 8;
        this->vmHead[0x4a].pendingInterrupt = 9;
        this->vmHead[0x4d].pendingInterrupt = 8;
        this->vmHead[0x47].color.bytes.a = 0;
        this->vmHead[0x4d].color.bytes.a = 0;
        this->vmHead[0x50].pendingInterrupt = 8;
        this->vmHead[0x51].pendingInterrupt = 9;
        this->vmHead[0x52].pendingInterrupt = 8;
        this->vmHead[0x50].color.bytes.a = 0;
        this->vmHead[0x52].color.bytes.a = 0;
        this->vmHead[0x53].pendingInterrupt = 8;
        this->vmHead[0x54].pendingInterrupt = 9;
        this->vmHead[0x55].pendingInterrupt = 8;
        this->vmHead[0x53].color.bytes.a = 0;
        this->vmHead[0x55].color.bytes.a = 0;
      } else if (this->cursor == 2) {
        this->vmHead[0x47].pendingInterrupt = 8;
        this->vmHead[0x4a].pendingInterrupt = 8;
        this->vmHead[0x4d].pendingInterrupt = 9;
        this->vmHead[0x4a].color.bytes.a = 0;
        this->vmHead[0x47].color.bytes.a = 0;
        this->vmHead[0x50].pendingInterrupt = 8;
        this->vmHead[0x51].pendingInterrupt = 8;
        this->vmHead[0x52].pendingInterrupt = 9;
        this->vmHead[0x50].color.bytes.a = 0;
        this->vmHead[0x51].color.bytes.a = 0;
        this->vmHead[0x53].pendingInterrupt = 8;
        this->vmHead[0x54].pendingInterrupt = 8;
        this->vmHead[0x55].pendingInterrupt = 9;
        this->vmHead[0x53].color.bytes.a = 0;
        this->vmHead[0x54].color.bytes.a = 0;
      }
      this->menuSubState = 0;
      this->inputDelayTimer = 0;
    }
    if (this->isPracticeMode != 0) {
      SetGameState(STATE_PRACTICE_SELECT_SHOTTYPE);
      this->cursor = 0;
      return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    if (this->stateTimer == 0x1e) {
      this->menuSubState = 1;
    }
  } else if (this->menuSubState == 1) {
    if (MoveCursorHorizontal(3) != ZUN_SUCCESS) {
      if (g_Supervisor.cfg.defaultDifficulty == 4) {
        while (
            (g_GameManager.HasReachedMaxClears(this->cursor << 1) == 0 &&
             (g_GameManager.HasReachedMaxClears(this->cursor * 2 + 1) == 0))) {
          this->cursor = this->cursor + 1;
          if (2 < this->cursor) {
            this->cursor = this->cursor - 3;
          }
        }
      } else if (g_Supervisor.cfg.defaultDifficulty == 5) {
        while (
            (g_GameManager.HasUnlockedPhantom(this->cursor << 1) == 0 &&
             (g_GameManager.HasUnlockedPhantom(this->cursor * 2 + 1) == 0))) {
          this->cursor = this->cursor + 1;
          if (2 < this->cursor) {
            this->cursor = this->cursor - 3;
          }
        }
      }
      this->vmHead[0x48].flags = this->vmHead[0x48].flags | 2;
      this->vmHead[0x49].flags = this->vmHead[0x49].flags | 2;
      this->vmHead[0x47].flags = this->vmHead[0x47].flags | 2;
      this->vmHead[0x50].flags = this->vmHead[0x50].flags | 2;
      this->vmHead[0x53].flags = this->vmHead[0x53].flags | 2;
      this->vmHead[0x4b].flags = this->vmHead[0x4b].flags | 2;
      this->vmHead[0x4c].flags = this->vmHead[0x4c].flags | 2;
      this->vmHead[0x4a].flags = this->vmHead[0x4a].flags | 2;
      this->vmHead[0x51].flags = this->vmHead[0x51].flags | 2;
      this->vmHead[0x54].flags = this->vmHead[0x54].flags | 2;
      this->vmHead[0x4e].flags = this->vmHead[0x4e].flags | 2;
      this->vmHead[0x4f].flags = this->vmHead[0x4f].flags | 2;
      this->vmHead[0x4d].flags = this->vmHead[0x4d].flags | 2;
      this->vmHead[0x52].flags = this->vmHead[0x52].flags | 2;
      this->vmHead[0x55].flags = this->vmHead[0x55].flags | 2;
      if (this->cursor == 0) {
        this->vmHead[0x47].pendingInterrupt = 9;
        this->vmHead[0x4a].pendingInterrupt = 8;
        this->vmHead[0x4d].pendingInterrupt = 8;
        this->vmHead[0x50].pendingInterrupt = 9;
        this->vmHead[0x51].pendingInterrupt = 8;
        this->vmHead[0x52].pendingInterrupt = 8;
        this->vmHead[0x53].pendingInterrupt = 9;
        this->vmHead[0x54].pendingInterrupt = 8;
        this->vmHead[0x55].pendingInterrupt = 8;
      } else if (this->cursor == 1) {
        this->vmHead[0x47].pendingInterrupt = 8;
        this->vmHead[0x4a].pendingInterrupt = 9;
        this->vmHead[0x4d].pendingInterrupt = 8;
        this->vmHead[0x50].pendingInterrupt = 8;
        this->vmHead[0x51].pendingInterrupt = 9;
        this->vmHead[0x52].pendingInterrupt = 8;
        this->vmHead[0x53].pendingInterrupt = 8;
        this->vmHead[0x54].pendingInterrupt = 9;
        this->vmHead[0x55].pendingInterrupt = 8;
      } else if (this->cursor == 2) {
        this->vmHead[0x47].pendingInterrupt = 8;
        this->vmHead[0x4a].pendingInterrupt = 8;
        this->vmHead[0x4d].pendingInterrupt = 9;
        this->vmHead[0x50].pendingInterrupt = 8;
        this->vmHead[0x51].pendingInterrupt = 8;
        this->vmHead[0x52].pendingInterrupt = 9;
        this->vmHead[0x53].pendingInterrupt = 8;
        this->vmHead[0x54].pendingInterrupt = 8;
        this->vmHead[0x55].pendingInterrupt = 9;
      }
    }
    if (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) != 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) !=
         (g_LastFrameInput & TH_BUTTON_SELECTMENU))) {
      g_GameManager.character = this->cursor;
      g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
      g_SoundPlayer.ProcessQueues();
      if (this->gameState == STATE_EXTRA_SELECT_CHARACTER) {
        SetGameState(STATE_EXTRA_SELECT_SHOTTYPE);
      } else if ((g_GameManager.flags & 1) == 0) {
        SetGameState(STATE_NORMAL_SELECT_SHOTTYPE);
      } else {
        SetGameState(STATE_PRACTICE_SELECT_SHOTTYPE);
      }
      this->cursor = 0;
      return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    if (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) != 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) !=
         (g_LastFrameInput & TH_BUTTON_RETURNMENU))) {
      g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
      g_SoundPlayer.ProcessQueues();
      g_GameManager.character = this->cursor;
      if (this->gameState == STATE_EXTRA_SELECT_CHARACTER) {
        SetGameState(STATE_EXTRA_SELECT_DIFFICULTY);
      } else if ((g_GameManager.flags & 1) == 0) {
        SetGameState(STATE_NORMAL_SELECT_DIFFICULTY);
      } else {
        SetGameState(STATE_PRACTICE_SELECT_DIFFICULTY);
      }
      this->cursor = 0;
      return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
  }
  this->idleFrames = this->idleFrames + 1;
  this->inputDelayTimer = this->inputDelayTimer + 1;
  this->stateTimer = this->stateTimer + 1;
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00459518
u32 MainMenu::OnUpdateSelectShotType()

{
  if (this->menuSubState == 0) {
    if (this->stateTimer == 0) {
      AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 10);
      if (g_Supervisor.cfg.defaultDifficulty < 4) {
        this->vmHead[g_Supervisor.cfg.defaultDifficulty + 0x43]
            .pendingInterrupt = 9;
      } else {
        if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0) {
          this->vmHead[0xa1].pendingInterrupt = 9;
        } else {
          this->vmHead[g_Supervisor.cfg.defaultDifficulty + 0x9e]
              .pendingInterrupt = 9;
        }
      }
      this->vmHead[0x48].flags = this->vmHead[0x48].flags & 0xfffffffd;
      this->vmHead[0x49].flags = this->vmHead[0x49].flags & 0xfffffffd;
      this->vmHead[0x47].flags = this->vmHead[0x47].flags & 0xfffffffd;
      this->vmHead[0x50].flags = this->vmHead[0x50].flags & 0xfffffffd;
      this->vmHead[0x53].flags = this->vmHead[0x53].flags & 0xfffffffd;
      this->vmHead[0x4b].flags = this->vmHead[0x4b].flags & 0xfffffffd;
      this->vmHead[0x4c].flags = this->vmHead[0x4c].flags & 0xfffffffd;
      this->vmHead[0x4a].flags = this->vmHead[0x4a].flags & 0xfffffffd;
      this->vmHead[0x51].flags = this->vmHead[0x51].flags & 0xfffffffd;
      this->vmHead[0x54].flags = this->vmHead[0x54].flags & 0xfffffffd;
      this->vmHead[0x4e].flags = this->vmHead[0x4e].flags & 0xfffffffd;
      this->vmHead[0x4f].flags = this->vmHead[0x4f].flags & 0xfffffffd;
      this->vmHead[0x4d].flags = this->vmHead[0x4d].flags & 0xfffffffd;
      this->vmHead[0x52].flags = this->vmHead[0x52].flags & 0xfffffffd;
      this->vmHead[0x55].flags = this->vmHead[0x55].flags & 0xfffffffd;
      this->cursor = g_GameManager.shotType;
      if (g_Supervisor.cfg.defaultDifficulty == 4) {
        while (g_GameManager.HasReachedMaxClears(
                   this->cursor + (u32)g_GameManager.character * 2) == 0) {
          this->cursor = this->cursor + 1;
          if (1 < this->cursor) {
            this->cursor = this->cursor - 2;
          }
        }
      } else if (g_Supervisor.cfg.defaultDifficulty == 5) {
        while (g_GameManager.HasReachedMaxClears(
                   this->cursor + (u32)g_GameManager.character * 2) == 0) {
          this->cursor = this->cursor + 1;
          if (1 < this->cursor) {
            this->cursor = this->cursor - 2;
          }
        }
      }
      if (g_GameManager.character == CHAR_REIMU) {
        this->vmHead[0x48].flags = this->vmHead[0x48].flags | 2;
        this->vmHead[0x49].flags = this->vmHead[0x49].flags | 2;
        this->vmHead[0x47].flags = this->vmHead[0x47].flags | 2;
        g_AnmManager->SetActiveSprite(
            this->vmHead + (0x49 - this->cursor),
            this->vmHead[0x49 - this->cursor].baseSpriteIdx + 1);
        g_AnmManager->SetActiveSprite(
            this->vmHead + this->cursor + 0x48,
            (i32)this->vmHead[this->cursor + 0x48].baseSpriteIdx);
      } else if (g_GameManager.character == CHAR_MARISA) {
        this->vmHead[0x4b].flags = this->vmHead[0x4b].flags | 2;
        this->vmHead[0x4c].flags = this->vmHead[0x4c].flags | 2;
        this->vmHead[0x4a].flags = this->vmHead[0x4a].flags | 2;
        g_AnmManager->SetActiveSprite(
            this->vmHead + (0x4c - this->cursor),
            this->vmHead[0x4c - this->cursor].baseSpriteIdx + 1);
        g_AnmManager->SetActiveSprite(
            this->vmHead + this->cursor + 0x4b,
            (i32)this->vmHead[this->cursor + 0x4b].baseSpriteIdx);
      } else if (g_GameManager.character == CHAR_SAKUYA) {
        this->vmHead[0x4e].flags = this->vmHead[0x4e].flags | 2;
        this->vmHead[0x4f].flags = this->vmHead[0x4f].flags | 2;
        this->vmHead[0x4d].flags = this->vmHead[0x4d].flags | 2;
        g_AnmManager->SetActiveSprite(
            this->vmHead + (0x4f - this->cursor),
            this->vmHead[0x4f - this->cursor].baseSpriteIdx + 1);
        g_AnmManager->SetActiveSprite(
            this->vmHead + this->cursor + 0x4e,
            (i32)this->vmHead[this->cursor + 0x4e].baseSpriteIdx);
      }
      this->menuSubState = 0;
      this->inputDelayTimer = 0;
    }
    if (this->isPracticeMode != 0) {
      SetGameState(STATE_SELECT_PRACTICE_STAGE);
      this->isPracticeMode = 0;
      this->cursor = g_GameManager.currentStage - 1;
      return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    if (this->stateTimer == 0x1e) {
      this->menuSubState = 1;
    }
  } else if (this->menuSubState == 1) {
    if (MoveCursorVertical(2) != 0) {
      if (g_Supervisor.cfg.defaultDifficulty == 4) {
        while (g_GameManager.HasReachedMaxClears(
                   this->cursor + (u32)g_GameManager.character * 2) == 0) {
          this->cursor = this->cursor + 1;
          if (1 < this->cursor) {
            this->cursor = this->cursor - 2;
          }
        }
      } else if (g_Supervisor.cfg.defaultDifficulty == 5) {
        while (g_GameManager.HasReachedMaxClears(
                   this->cursor + (u32)g_GameManager.character * 2) == 0) {
          this->cursor = this->cursor + 1;
          if (1 < this->cursor) {
            this->cursor = this->cursor - 2;
          }
        }
      }
      if (g_GameManager.character == CHAR_REIMU) {
        g_AnmManager->SetActiveSprite(
            this->vmHead + (0x49 - this->cursor),
            this->vmHead[0x49 - this->cursor].baseSpriteIdx + 1);
        g_AnmManager->SetActiveSprite(
            this->vmHead + this->cursor + 0x48,
            (i32)this->vmHead[this->cursor + 0x48].baseSpriteIdx);
      } else if (g_GameManager.character == CHAR_MARISA) {
        g_AnmManager->SetActiveSprite(
            this->vmHead + (0x4c - this->cursor),
            this->vmHead[0x4c - this->cursor].baseSpriteIdx + 1);
        g_AnmManager->SetActiveSprite(
            this->vmHead + this->cursor + 0x4b,
            (i32)this->vmHead[this->cursor + 0x4b].baseSpriteIdx);
      } else if (g_GameManager.character == CHAR_SAKUYA) {
        g_AnmManager->SetActiveSprite(
            this->vmHead + (0x4f - this->cursor),
            this->vmHead[0x4f - this->cursor].baseSpriteIdx + 1);
        g_AnmManager->SetActiveSprite(
            this->vmHead + this->cursor + 0x4e,
            (i32)this->vmHead[this->cursor + 0x4e].baseSpriteIdx);
      }
    }
    if (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) != 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) !=
         (g_LastFrameInput & TH_BUTTON_SELECTMENU))) {
      g_GameManager.shotType = this->cursor;
      g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
      g_SoundPlayer.ProcessQueues();
      if ((g_GameManager.flags & 1) == 0) {
        g_GameManager.difficulty = g_Supervisor.cfg.defaultDifficulty;
        if (g_GameManager.difficulty < DIFF_EXTRA) {
          g_GameManager.currentStage = 0;
        } else {
          g_GameManager.currentStage = g_GameManager.difficulty + DIFF_HARD;
        }
        g_Supervisor.curState = 2;
        g_GameManager.flags &= 0xfffffff7;
        Supervisor::StopAudio();
        while (g_SoundPlayer.ProcessQueues() != 0)
          ;
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
      }
      this->cursor = 0;
      SetGameState(STATE_SELECT_PRACTICE_STAGE);
      return CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN;
    }
    if (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) != 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) !=
         (g_LastFrameInput & TH_BUTTON_RETURNMENU))) {
      g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
      g_GameManager.shotType = this->cursor;
      if (this->gameState == STATE_EXTRA_SELECT_SHOTTYPE) {
        SetGameState(STATE_EXTRA_SELECT_CHARACTER);
      } else if ((g_GameManager.flags & 1) == 0) {
        SetGameState(STATE_NORMAL_SELECT_CHARACTER);
      } else {
        SetGameState(STATE_PRACTICE_SELECT_CHARACTER);
      }
      this->vmHead[0x48].flags = this->vmHead[0x48].flags | 2;
      this->vmHead[0x49].flags = this->vmHead[0x49].flags | 2;
      this->vmHead[0x47].flags = this->vmHead[0x47].flags | 2;
      this->vmHead[0x50].flags = this->vmHead[0x50].flags | 2;
      this->vmHead[0x53].flags = this->vmHead[0x53].flags | 2;
      this->vmHead[0x4b].flags = this->vmHead[0x4b].flags | 2;
      this->vmHead[0x4c].flags = this->vmHead[0x4c].flags | 2;
      this->vmHead[0x4a].flags = this->vmHead[0x4a].flags | 2;
      this->vmHead[0x51].flags = this->vmHead[0x51].flags | 2;
      this->vmHead[0x54].flags = this->vmHead[0x54].flags | 2;
      this->vmHead[0x4e].flags = this->vmHead[0x4e].flags | 2;
      this->vmHead[0x4f].flags = this->vmHead[0x4f].flags | 2;
      this->vmHead[0x4d].flags = this->vmHead[0x4d].flags | 2;
      this->vmHead[0x52].flags = this->vmHead[0x52].flags | 2;
      this->vmHead[0x55].flags = this->vmHead[0x55].flags | 2;
      return CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN;
    }
  }
  this->idleFrames = this->idleFrames + 1;
  this->inputDelayTimer = this->inputDelayTimer + 1;
  this->stateTimer = this->stateTimer + 1;
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0045a1dd
u32 MainMenu::OnUpdateSelectPracticeStage()

{
  i32 local_8;

  if (this->menuSubState == 0) {
    if (this->stateTimer == 0) {
      AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 0x12);
      this->vmHead[0x48].flags = this->vmHead[0x48].flags & 0xfffffffd;
      this->vmHead[0x49].flags = this->vmHead[0x49].flags & 0xfffffffd;
      this->vmHead[0x47].flags = this->vmHead[0x47].flags & 0xfffffffd;
      this->vmHead[0x50].flags = this->vmHead[0x50].flags & 0xfffffffd;
      this->vmHead[0x53].flags = this->vmHead[0x53].flags & 0xfffffffd;
      this->vmHead[0x4b].flags = this->vmHead[0x4b].flags & 0xfffffffd;
      this->vmHead[0x4c].flags = this->vmHead[0x4c].flags & 0xfffffffd;
      this->vmHead[0x4a].flags = this->vmHead[0x4a].flags & 0xfffffffd;
      this->vmHead[0x51].flags = this->vmHead[0x51].flags & 0xfffffffd;
      this->vmHead[0x54].flags = this->vmHead[0x54].flags & 0xfffffffd;
      this->vmHead[0x4e].flags = this->vmHead[0x4e].flags & 0xfffffffd;
      this->vmHead[0x4f].flags = this->vmHead[0x4f].flags & 0xfffffffd;
      this->vmHead[0x4d].flags = this->vmHead[0x4d].flags & 0xfffffffd;
      this->vmHead[0x52].flags = this->vmHead[0x52].flags & 0xfffffffd;
      this->vmHead[0x55].flags = this->vmHead[0x55].flags & 0xfffffffd;
      if (g_GameManager.character == CHAR_REIMU) {
        this->vmHead[0x48].flags = this->vmHead[0x48].flags | 2;
        this->vmHead[0x49].flags = this->vmHead[0x49].flags | 2;
        this->vmHead[0x47].flags = this->vmHead[0x47].flags | 2;
      } else if (g_GameManager.character == CHAR_MARISA) {
        this->vmHead[0x4b].flags = this->vmHead[0x4b].flags | 2;
        this->vmHead[0x4c].flags = this->vmHead[0x4c].flags | 2;
        this->vmHead[0x4a].flags = this->vmHead[0x4a].flags | 2;
      } else if (g_GameManager.character == CHAR_SAKUYA) {
        this->vmHead[0x4e].flags = this->vmHead[0x4e].flags | 2;
        this->vmHead[0x4f].flags = this->vmHead[0x4f].flags | 2;
        this->vmHead[0x4d].flags = this->vmHead[0x4d].flags | 2;
      }
      this->menuSubState = 0;
      this->inputDelayTimer = 0;
      g_GameManager.flags |= 1;
    }
    if (this->stateTimer == 0x1e) {
      this->menuSubState = 1;
    }
  } else if (this->menuSubState == 1) {
    local_8 =
        g_GameManager.clrd[g_GameManager.shotType + g_GameManager.character * 2]
            .difficultyClearedWithRetries[g_Supervisor.cfg.defaultDifficulty];
    if (local_8 < 0) {
      local_8 = 1;
    } else if (0x62 < local_8) {
      local_8 = 6;
    }
    if ((i32)local_8 <= this->cursor) {
      this->cursor = 0;
    }
    MoveCursorVertical(local_8);
    if (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) != 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) !=
         (g_LastFrameInput & TH_BUTTON_SELECTMENU))) {
      g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
      g_GameManager.difficulty = g_Supervisor.cfg.defaultDifficulty;
      g_GameManager.currentStage = this->cursor;
      g_Supervisor.curState = 2;
      g_GameManager.flags &= 0xfffffff7;
      Supervisor::StopAudio();
      while (g_SoundPlayer.ProcessQueues() != 0)
        ;
      return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    }
    if (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) != 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) !=
         (g_LastFrameInput & TH_BUTTON_RETURNMENU))) {
      g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
      this->cursor = g_GameManager.shotType;
      SetGameState(STATE_NORMAL_SELECT_SHOTTYPE);
      this->vmHead[0x48].flags = this->vmHead[0x48].flags | 2;
      this->vmHead[0x49].flags = this->vmHead[0x49].flags | 2;
      this->vmHead[0x47].flags = this->vmHead[0x47].flags | 2;
      this->vmHead[0x50].flags = this->vmHead[0x50].flags | 2;
      this->vmHead[0x53].flags = this->vmHead[0x53].flags | 2;
      this->vmHead[0x4b].flags = this->vmHead[0x4b].flags | 2;
      this->vmHead[0x4c].flags = this->vmHead[0x4c].flags | 2;
      this->vmHead[0x4a].flags = this->vmHead[0x4a].flags | 2;
      this->vmHead[0x51].flags = this->vmHead[0x51].flags | 2;
      this->vmHead[0x54].flags = this->vmHead[0x54].flags | 2;
      this->vmHead[0x4e].flags = this->vmHead[0x4e].flags | 2;
      this->vmHead[0x4f].flags = this->vmHead[0x4f].flags | 2;
      this->vmHead[0x4d].flags = this->vmHead[0x4d].flags | 2;
      this->vmHead[0x52].flags = this->vmHead[0x52].flags | 2;
      this->vmHead[0x55].flags = this->vmHead[0x55].flags | 2;
      return CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN;
    }
  }
  this->idleFrames = this->idleFrames + 1;
  this->inputDelayTimer = this->inputDelayTimer + 1;
  this->stateTimer = this->stateTimer + 1;
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0045a924
u32 MainMenu::OnUpdateSelectReplay()

{
  MainMenu *mainMenu;
  _WIN32_FIND_DATAA local_194;
  char local_54[64];
  ReplayHeaderAndData *local_14;
  i32 local_10;
  HANDLE local_c;
  i32 i;

  mainMenu = this;
  if (this->menuSubState == 0) {
    if (this->stateTimer == 0) {
      if ((this->prevGameState != STATE_SELECT_REPLAY) &&
          // STRING: TH07 0x00495680
          (g_AnmManager->LoadSurface(0, "data/title/select00.jpg") !=
           ZUN_SUCCESS)) {
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
      }
      AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 0xe);
      this->cursor = 0;
      this->menuSubState = 0;
      this->inputDelayTimer = 0;
      this->cursorVm = NULL;
      local_10 = 0;
      for (i = 0; i < 0xf; i += 1) {
        // STRING: TH07 0x004967bc
        sprintf(local_54, "./replay/th7_%.2d.rpy", i + 1);
        local_14 = (ReplayHeaderAndData *)FileSystem::OpenFile(local_54, 1);
        if (local_14 == NULL) {
          local_14 = NULL;
        } else {
          local_14 =
              ReplayManager::ValidateReplayData(local_14, g_LastFileSize);
          if (local_14 != NULL) {
            mainMenu->replays[local_10] = *local_14;
            strcpy(mainMenu->replayFilenames[local_10], local_54);
            // STRING: TH07 0x00496460
            sprintf(mainMenu->replayLabels[local_10], "No.%.2d", i + 1);
            local_10 += 1;
            mainMenu = this;
            free(local_14);
          }
        }
      }
      // STRING: TH07 0x00495674
      _mkdir("./replay");
      _chdir("./replay");
      // STRING: TH07 0x00495664
      local_c = FindFirstFileA("th7_ud????.rpy", &local_194);
      if (local_c != INVALID_HANDLE_VALUE) {
        for (i = 0; i < 0x2d; i += 1) {
          local_14 = (ReplayHeaderAndData *)FileSystem::OpenFile(
              local_194.cFileName, 1);
          if (local_14 == NULL) {
            local_14 = NULL;
          } else {
            local_14 =
                ReplayManager::ValidateReplayData(local_14, g_LastFileSize);
            if (local_14 != NULL) {
              mainMenu->replays[local_10] = *local_14;
              // STRING: TH07 0x00495658
              sprintf(mainMenu->replayFilenames[local_10], "./replay/%s",
                      local_194.cFileName);
              // STRING: TH07 0x00495650
              sprintf(mainMenu->replayLabels[local_10], "User ");
              free(local_14);
              local_10 += 1;
            }
            if (FindNextFileA(local_c, &local_194) == 0)
              break;
          }
        }
      }
      FindClose(local_c);
      // STRING: TH07 0x0049564c
      _chdir("../");
      mainMenu->replayFilesNum = local_10;
      mainMenu->replayPage = 0;
    }
    if (0x1d < mainMenu->stateTimer) {
      mainMenu->menuSubState = 1;
      mainMenu->inputDelayTimer = 0;
    }
  } else if (this->menuSubState == 1) {
    MoveCursorVertical(this->replayFilesNum);
    if (0xf < this->replayFilesNum) {
      if ((((g_CurFrameRawInput & TH_BUTTON_LEFT) != 0) &&
           ((g_CurFrameRawInput & TH_BUTTON_LEFT) !=
            (g_LastFrameInput & TH_BUTTON_LEFT))) ||
          (((g_CurFrameRawInput & TH_BUTTON_LEFT) != 0 &&
            (g_IsEighthFrameOfHeldInput != 0)))) {
        this->cursor = this->cursor - 0xf;
        if (this->cursor < 0) {
          this->cursor = this->cursor + this->replayFilesNum;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
      }
      if ((((g_CurFrameRawInput & TH_BUTTON_RIGHT) != 0) &&
           ((g_CurFrameRawInput & TH_BUTTON_RIGHT) !=
            (g_LastFrameInput & TH_BUTTON_RIGHT))) ||
          (((g_CurFrameRawInput & TH_BUTTON_RIGHT) != 0 &&
            (g_IsEighthFrameOfHeldInput != 0)))) {
        this->cursor = this->cursor + 0xf;
        if (this->replayFilesNum <= this->cursor) {
          this->cursor = this->cursor - this->replayFilesNum;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
      }
    }
    this->chosenReplay = this->cursor;
    if (9 < this->inputDelayTimer) {
      if (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) == 0) ||
          ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) ==
           (g_LastFrameInput & TH_BUTTON_SELECTMENU))) {
        if (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) != 0) &&
            ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) !=
             (g_LastFrameInput & TH_BUTTON_RETURNMENU))) {
          g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
          this->menuSubState = 4;
          this->inputDelayTimer = 0;
          AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 0x10);
        }
      } else if (this->replayFilesNum != 0) {
        g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
        this->menuSubState = 2;
        AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 0xf);
        this->vmHead[this->chosenReplay % 0xf + 0x87].pendingInterrupt = 0x11;
        this->currentReplay = (ReplayHeaderAndData *)FileSystem::OpenFile(
            this->replayFilenames[this->chosenReplay], 1);
        this->currentReplay = ReplayManager::ValidateReplayData(
            this->currentReplay, g_LastFileSize);
        for (i = 0; i < 7; i += 1) {
          if (this->currentReplay->head.stageReplayData[i].offset != 0) {
            this->currentReplay->head.stageReplayData[i].data =
                (StageReplayData *)((u8 *)this->currentReplay +
                                    this->currentReplay->head.stageReplayData[i]
                                        .offset);
          }
        }
        this->cursor = 0;
        while (this->replays[this->chosenReplay]
                   .head.stageReplayData[this->cursor]
                   .data == NULL) {
          this->cursor = this->cursor + 1;
          if (6 < this->cursor) {
            // STRING: TH07 0x00495634
            g_GameErrorContext.Fatal("リプレイデータが異常\r\n");
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
          }
        }
      }
    }
  } else if (this->menuSubState == 2) {
    i = MoveCursorVertical(7);
    if (i < 0) {
      while (this->replays[this->chosenReplay]
                 .head.stageReplayData[this->cursor]
                 .data == NULL) {
        this->cursor = this->cursor - 1;
        if (this->cursor < 0) {
          this->cursor = 6;
        }
      }
    } else if (0 < i) {
      while (this->replays[this->chosenReplay]
                 .head.stageReplayData[this->cursor]
                 .data == NULL) {
        this->cursor = this->cursor + 1;
        if (6 < this->cursor) {
          this->cursor = 0;
        }
      }
    }
    this->selectedStage = this->cursor;
    if (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) == 0) ||
        ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) ==
         (g_LastFrameInput & TH_BUTTON_SELECTMENU))) {
      if (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) != 0) &&
          ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) !=
           (g_LastFrameInput & TH_BUTTON_RETURNMENU))) {
        free(this->currentReplay);
        this->currentReplay = NULL;
        this->menuSubState = 1;
        this->stateTimer = 0;
        AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 0xe);
        this->cursor = this->chosenReplay;
      }
    } else {
      AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 0x13);
      this->vmHead[this->chosenReplay % 0xf + 0x87].pendingInterrupt = 0x11;
      this->menuSubState = 3;
      this->cursor = 0;
      this->vmHead[0x9e].pendingInterrupt = 0x15;
      this->vmHead[0x9f].pendingInterrupt = 0x15;
      this->vmHead[0xa0].pendingInterrupt = 0x15;
      this->vmHead[this->cursor + 0x9e].pendingInterrupt = 0x14;
    }
  } else if (this->menuSubState == 3) {
    i = MoveCursorVertical(3);
    if (i != 0) {
      this->vmHead[0x9e].pendingInterrupt = 0x15;
      this->vmHead[0x9f].pendingInterrupt = 0x15;
      this->vmHead[0xa0].pendingInterrupt = 0x15;
      this->vmHead[this->cursor + 0x9e].pendingInterrupt = 0x14;
    }
    if (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) != 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) !=
         (g_LastFrameInput & TH_BUTTON_SELECTMENU))) {
      g_GameManager.flags = (g_GameManager.flags & 0xfffffff7) | 8;
      strcpy(g_GameManager.replayFilename,
             this->replayFilenames[this->chosenReplay]);
      g_GameManager.difficulty = this->currentReplay->data.difficulty;
      g_GameManager.character = this->currentReplay->data.shotType / 2;
      g_GameManager.shotType = this->currentReplay->data.shotType % 2;
      g_GameManager.shotTypeAndCharacter = this->currentReplay->data.shotType;
      free(this->currentReplay);
      this->currentReplay = NULL;
      if (g_GameManager.difficulty < 5) {
        g_GameManager.currentStage = this->selectedStage;
      } else {
        g_GameManager.currentStage = 7;
      }
      g_Supervisor.curState = 2;
      g_GameManager.replayStage = (u8)this->cursor;
      Supervisor::StopAudio();
      while (g_SoundPlayer.ProcessQueues() != 0)
        ;
      return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    }
    if (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) != 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) !=
         (g_LastFrameInput & TH_BUTTON_RETURNMENU))) {
      this->menuSubState = 2;
      this->stateTimer = 0;
      this->cursor = this->selectedStage;
      AnmManager::SetInterruptActiveVms(this->vmHead, this->vmCount, 0xf);
      this->vmHead[this->chosenReplay % 0xf + 0x87].pendingInterrupt = 0x11;
    }
  } else if ((this->menuSubState == 4) && (0x1d < this->inputDelayTimer)) {
    SetGameState(STATE_PRE_INPUT);
    this->cursor = 3;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
  }
  mainMenu->idleFrames = mainMenu->idleFrames + 1;
  mainMenu->inputDelayTimer = mainMenu->inputDelayTimer + 1;
  mainMenu->stateTimer = mainMenu->stateTimer + 1;
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0045b5ef
i32 MainMenu::DrawReplayMenu()

{
  i32 iVar2;
  i32 replayAmount;
  i32 i;
  AnmVm *vm;

  AsciiManager::AddFormatText(&g_AsciiManager, &this->vmHead[0x86].pos,
  // STRING: TH07 0x0049557c
                              "No.   Name       Date  Player   Rank");
  replayAmount = this->chosenReplay - this->chosenReplay % 0xf;
  iVar2 = replayAmount + 0xf;
  vm = &this->vmHead[0x86];
  while (replayAmount < iVar2 && (replayAmount < this->replayFilesNum)) {
    g_AsciiManager.isSelected = replayAmount == this->chosenReplay;
    if (replayAmount == this->chosenReplay) {
      g_AsciiManager.color = 0xffffffff;
    } else {
      g_AsciiManager.color = 0xff808080;
    }
    AsciiManager::AddFormatText(
        // STRING: TH07 0x00495564
        &g_AsciiManager, &vm[1].pos, "%s %8s  %6s %7s  %8s",
        this->replayLabels + replayAmount,
        this->replays[replayAmount].data.name,
        this->replays[replayAmount].data.date,
        g_CharacterAndShottypeReplayStrings[this->replays[replayAmount]
                                                .data.shotType],
        g_DifficultyStrings[this->replays[replayAmount].data.difficulty]);
    vm = vm + 1;
    replayAmount += 1;
  }
  if (((this->menuSubState == 2) || (this->menuSubState == 3)) &&
      (this->currentReplay != NULL)) {
    g_AsciiManager.color = 0xffffffff;
    g_AsciiManager.isSelected = 0;
    AsciiManager::AddFormatText(&g_AsciiManager, &this->vmHead[0x85].pos,
    // STRING: TH07 0x00495554
                                "       %2.3f%%",
                                (double)this->currentReplay->data.slowdownRate);
    AsciiManager::AddFormatText(&g_AsciiManager, &this->vmHead[0x96].pos,
    // STRING: TH07 0x00495540
                                "Stage    LastScore");
    vm = &this->vmHead[0x96];
    for (i = 0; i < 7; i += 1) {
      if (this->menuSubState == 3) {
        if (i == this->selectedStage) {
          g_AsciiManager.color = 0x60ffffff;
        } else {
          g_AsciiManager.color = 0x60808080;
        }
      } else {
        g_AsciiManager.isSelected = (i32)(i == this->selectedStage);
        if (i == this->selectedStage) {
          g_AsciiManager.color = 0xffffffff;
        } else {
          g_AsciiManager.color = 0xff808080;
        }
      }
      if (this->currentReplay->head.stageReplayData[i].data == NULL) {
        if ((i < 6) || (this->currentReplay->data.difficulty < 5)) {
          // STRING: TH07 0x00495528
          AsciiManager::AddFormatText(&g_AsciiManager, &vm[1].pos, "%s ----------");
        } else {
          AsciiManager::AddFormatText(&g_AsciiManager, &vm[1].pos, "%s ----------");
        }
      } else if ((i < 6) || (this->currentReplay->data.difficulty < 5)) {
        // STRING: TH07 0x00495538
        AsciiManager::AddFormatText(&g_AsciiManager, &vm[1].pos, "%s %9d0",
                                    g_StageReplayStrings[i]);
      } else {
        AsciiManager::AddFormatText(&g_AsciiManager, &vm[1].pos, "%s %9d0",
                                    g_PhantasmReplayString);
      }
      vm = vm + 1;
    }
  }
  g_AsciiManager.color = 0xffffffff;
  g_AsciiManager.isSelected = 0;
  return 1;
}

// FUNCTION: TH07 0x0045b9ad
i32 MainMenu::DrawPracticeMenu()

{
  D3DXVECTOR3 local_1c;
  u32 local_10;
  i32 local_c;
  AnmVm *local_8;

  g_AsciiManager.color = 0xffffffff;
  g_AsciiManager.isSelected = 0;
  local_8 = this->vmHead + 0x83;
  AsciiManager::AddFormatText(&g_AsciiManager, &this->vmHead[0x83].pos,
  // STRING: TH07 0x004954e4
                              "Stage    HI-Score");
  local_1c = local_8->pos;
  local_10 =
      g_GameManager.clrd[g_GameManager.shotType + g_GameManager.character * 2]
          .difficultyClearedWithoutRetries[g_Supervisor.cfg.defaultDifficulty];
  for (local_c = 0; local_1c.y = local_1c.y + 16.0f, local_c < 6;
       local_c += 1) {
    g_AsciiManager.isSelected = (i32)(local_c == this->cursor);
    if (local_c == this->cursor) {
      g_AsciiManager.color = 0xffffffff;
    } else if (local_c < (i32)local_10) {
      g_AsciiManager.color = 0xffa0a0a0;
    } else {
      g_AsciiManager.color = 0xff404040;
    }
    AsciiManager::AddFormatText(
        // STRING: TH07 0x004954d4
        &g_AsciiManager, &local_1c, "%s %9d0 (%3d)",
        g_StagePracticeStrings[local_c],
        g_GameManager
            .pscr[g_GameManager.shotType + g_GameManager.character * 2][local_c]
                 [g_Supervisor.cfg.defaultDifficulty]
            .score,
        g_GameManager
            .pscr[g_GameManager.shotType + g_GameManager.character * 2][local_c]
                 [g_Supervisor.cfg.defaultDifficulty]
            .playCount);
  }
  g_AsciiManager.color = 0xffffffff;
  g_AsciiManager.isSelected = 0;
  return 1;
}

// FUNCTION: TH07 0x0045bb4a
i32 MainMenu::MoveCursorVertical(i32 param_1)

{
  if (param_1 == 0) {
    return 0;
  } else if ((((g_CurFrameRawInput & TH_BUTTON_UP) == 0) ||
              ((g_CurFrameRawInput & TH_BUTTON_UP) ==
               (g_LastFrameInput & TH_BUTTON_UP))) &&
             (((g_CurFrameRawInput & TH_BUTTON_UP) == 0 ||
               (g_IsEighthFrameOfHeldInput == 0)))) {
    if ((((g_CurFrameRawInput & TH_BUTTON_DOWN) == 0) ||
         ((g_CurFrameRawInput & TH_BUTTON_DOWN) ==
          (g_LastFrameInput & TH_BUTTON_DOWN))) &&
        (((g_CurFrameRawInput & TH_BUTTON_DOWN) == 0 ||
          (g_IsEighthFrameOfHeldInput == 0)))) {
      return 0;
    } else {
      this->cursor += 1;
      g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
      if (this->cursor < 0) {
        this->cursor = param_1 - 1;
      }
      if (param_1 <= this->cursor) {
        this->cursor = 0;
      }
      return 1;
    }
  } else {
    this->cursor -= 1;
    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
    if (this->cursor < 0) {
      this->cursor = param_1 - 1;
    }
    if (param_1 <= this->cursor) {
      this->cursor = 0;
    }
    return -1;
  }
}

// FUNCTION: TH07 0x0045bc63
i32 MainMenu::MoveCursorHorizontal(i32 param_1)

{
  if (param_1 == 0) {
    return 0;
  } else if ((((g_CurFrameRawInput & TH_BUTTON_LEFT) == 0) ||
              ((g_CurFrameRawInput & TH_BUTTON_LEFT) ==
               (g_LastFrameInput & TH_BUTTON_LEFT))) &&
             (((g_CurFrameRawInput & TH_BUTTON_LEFT) == 0 ||
               (g_IsEighthFrameOfHeldInput == 0)))) {
    if ((((g_CurFrameRawInput & TH_BUTTON_RIGHT) == 0) ||
         ((g_CurFrameRawInput & TH_BUTTON_RIGHT) ==
          (g_LastFrameInput & TH_BUTTON_RIGHT))) &&
        (((g_CurFrameRawInput & TH_BUTTON_RIGHT) == 0 ||
          (g_IsEighthFrameOfHeldInput == 0)))) {
      return 0;
    } else {
      this->cursor = this->cursor + 1;
      if (param_1 <= this->cursor) {
        this->cursor = this->cursor - param_1;
      }
      g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
      return 1;
    }
  } else {
    this->cursor = this->cursor - 1;
    if (this->cursor < 0) {
      this->cursor = this->cursor + param_1;
    }
    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
    return -1;
  }
}

// FUNCTION: TH07 0x0045bd6c
u32 MainMenu::OnDraw(MainMenu *arg)

{
  f32 fVar1;
  f32 fVar2;
  f32 fVar3;
  bool bVar4;
  AnmVm *local_c;

  g_AnmManager->currentTexture = NULL;
  g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
  if (arg->gameState == STATE_SELECT_REPLAY) {
    arg->DrawReplayMenu();
  } else if (arg->gameState == STATE_SELECT_PRACTICE_STAGE) {
    arg->DrawPracticeMenu();
  }
  local_c = arg->vmHead;
  for (i32 i = 0; i < arg->vmCount; ++i) {
    if (local_c->sprite == NULL) {
      bVar4 = false;
    } else if (local_c->sprite->sourceFileIndex < 0) {
      bVar4 = false;
    } else {
      bVar4 = g_AnmManager->textures[local_c->sprite->sourceFileIndex] != NULL;
    }
    if (bVar4) {
      fVar1 = local_c->pos.x;
      fVar2 = local_c->pos.y;
      fVar3 = local_c->pos.z;
      local_c->pos += local_c->offset;
      if ((local_c->rotation).z == 0.0f) {
        g_AnmManager->DrawNoRotation(local_c);
      } else {
        g_AnmManager->Draw(local_c);
      }
      local_c->pos.x = fVar1;
      local_c->pos.y = fVar2;
      local_c->pos.z = fVar3;
    }
    local_c = local_c + 1;
  }
  if (arg->cursorVm != NULL) {
    g_AnmManager->DrawNoRotation(arg->cursorVm);
  }
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0045bf15
ZunResult MainMenu::ActualAddedCallback()

{
  ZunRect local_34;
  D3DCOLOR local_24;
  D3DCOLOR local_20;
  ZunRect local_1c;
  ScoreDat *local_8;

  SAFE_DELETE(g_GameManager.defaultCfg);
  g_GameManager.defaultCfg = new GameConfiguration;
  SAFE_DELETE(g_GameManager.globals);
  g_GameManager.globals = new ZunGlobals;
  g_Supervisor.effectiveFramerateMultiplier = 1.0f;
  if ((g_GameManager.flags >> 3 & 1) != 0) {
    g_GameManager.shotTypeAndCharacter = SHOT_REIMU_A;
    g_GameManager.character = CHAR_REIMU;
  }
  if ((g_GameManager.flags >> 1 & 1) != 0) {
    g_GameManager.flags = g_GameManager.flags & 0xfffffff7;
  }
  local_8 = ResultScreen::OpenScore("score.dat");
  ResultScreen::ParseClrd(local_8, g_GameManager.clrd);
  ResultScreen::ParsePscr(local_8, &g_GameManager.pscr[0][0][0]);
  ResultScreen::ParseCatk(local_8, g_GameManager.catk);
  ResultScreen::ReleaseScoreDat(local_8);
  if (g_GameManager.plst.gameHours < 7) {
    g_GameManager.maxRetries = 3;
  } else if (g_GameManager.plst.gameHours < 0xe) {
    g_GameManager.maxRetries = 4;
  } else {
    g_GameManager.maxRetries = 5;
  }
  if ((g_GameManager.isGameComplete == 0) &&
      (g_GameManager.HasUnlockedPhantomAndMaxClears() != 0)) {
    i32 local_c = 0;
    // STRING: TH07 0x004954bc
    g_AnmManager->LoadSurface(0, "data/title/phantasm.jpg");
    while (local_c < 900) {
      g_AnmManager->currentVertexShader = 0xff;
      g_AnmManager->currentSprite = NULL;
      g_AnmManager->currentTexture = NULL;
      g_AnmManager->currentColorOp = 0xff;
      g_AnmManager->currentBlendMode = 0xff;
      g_AnmManager->currentZWriteDisable = 0xff;
      g_AnmManager->scriptTicksThisFrame = 0;
      g_AnmManager->renderStateChangesThisFrame = 0;
      g_AnmManager->scriptsExecutedThisFrame = 0;
      g_AnmManager->flushesThisFrame = 0;
      g_AnmManager->currentCameraMode = 0xff;
      g_AnmManager->colorMulEnabled = 0;
      g_AnmManager->color.color = 0x80808080;
      g_Supervisor.d3dDevice->BeginScene();
      g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
      if (local_c < 0x3c) {
        local_1c.left = 0.0f;
        local_1c.top = 0.0f;
        local_1c.right = 639.0f;
        local_1c.bottom = 479.0f;
        local_20 = ((0x3c - local_c) * 0xff) / 0x3c << 0x18;
        ScreenEffect::DrawSquare(&local_1c, local_20);
      } else if (0x348 < local_c) {
        local_34.left = 0.0f;
        local_34.top = 0.0f;
        local_34.right = 639.0f;
        local_34.bottom = 479.0f;
        local_24 = ((local_c - 0x348) * 0xff) / 0x3c << 0x18;
        ScreenEffect::DrawSquare(&local_34, local_24);
      }
      g_CurFrameRawInput = Controller::GetInput();
      g_Supervisor.d3dDevice->EndScene();
      if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL))) {
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
      }
      if ((((0x77 < local_c) && (local_c < 0x348)) &&
           ((g_CurFrameRawInput & (TH_BUTTON_SELECTMENU | TH_BUTTON_BOMB)) !=
            0)) &&
          ((g_CurFrameRawInput & (TH_BUTTON_SELECTMENU | TH_BUTTON_BOMB)) !=
           (g_LastFrameInput & (TH_BUTTON_SELECTMENU | TH_BUTTON_BOMB)))) {
        local_c = 0x348;
        g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
      }
      local_c = local_c + 1;
      g_SoundPlayer.ProcessQueues();
    }
    g_AnmManager->ReleaseSurface(0);
  }
  g_GameManager.isGameComplete = g_GameManager.HasUnlockedPhantomAndMaxClears();
  this->gameState = STATE_PRE_INPUT;
  InitializeTimingVars(&g_Supervisor);
  if (g_Supervisor.wantedState2 < 2) {
  LAB_0045c342:
    this->cursor = 0;
  } else {
    if (3 < g_Supervisor.wantedState2) {
      if (g_Supervisor.wantedState2 == 5) {
        this->cursor = 4;
        goto LAB_0045c348;
      }
      if (g_Supervisor.wantedState2 != 6) {
        if (g_Supervisor.wantedState2 == 8) {
          this->cursor = 5;
          goto LAB_0045c348;
        }
        goto LAB_0045c342;
      }
    }
    this->cursor = 3 < g_GameManager.difficulty;
  }
LAB_0045c348:
  this->isPracticeMode = 0;
  if ((g_GameManager.flags & 1) != 0) {
    this->cursor = 2;
    this->isPracticeMode = 1;
  }
  g_GameManager.flags = g_GameManager.flags & 0xfffffffe;
  if (g_Supervisor.wantedState2 != 0) {
    GameManager::DrawLoadingSprite();
  }
  // STRING: TH07 0x004954a8
  if (g_AnmManager->LoadAnms(0x20, "data/title01.anm", 0x900) == ZUN_SUCCESS) {
    if ((g_GameManager.flags >> 1 & 1) == 0) {
      if (g_Supervisor.wantedState2 != 5) {
        Supervisor::LoadAudio(8, "bgm/th07_01.mid");
      }
      if (g_Supervisor.startupTimeForMenuMusic == 0) {
        BombEffects::RegisterChain(0, 0x46, 0xffffff, 0, 0);
      } else {
        BombEffects::RegisterChain(0, 0x46, 0xffffff, 0, 0);
      }
    }
    for (i32 i = 0; i < 0xe; i = i + 1) {
      this->vms[i].anmFileIdx = 0x706;
      g_AnmManager->SetAndExecuteScript(this->vms + i,
                                        g_AnmManager->scripts[0x706]);
      g_AnmManager->SetActiveSprite(this->vms + i,
                                    this->vms[i].activeSpriteIdx + i);
    }
    this->cursorVm = this->vms;
    g_GameManager.flags = g_GameManager.flags & 0xfffffffd;
    g_GameManager.demoFrames = 0;
    return ZUN_SUCCESS;
  } else {
    return ZUN_ERROR;
  }
}

// FUNCTION: TH07 0x0045c4c8
ZunResult MainMenu::AddedCallback(MainMenu *arg)

{
  return arg->ActualAddedCallback();
}

// FUNCTION: TH07 0x0045c4d9
ZunResult MainMenu::Release()

{
  SAFE_FREE(this->currentReplay);
  SAFE_FREE(this->vmHead);
  return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0045c546
ZunResult MainMenu::DeletedCallback(MainMenu *arg)

{
  g_Supervisor.d3dDevice->ResourceManagerDiscardBytes(0);
  for (i32 i = 0x20; i < 0x2a; ++i)
    g_AnmManager->ReleaseAnm(i);
  g_AnmManager->ReleaseSurface(0);
  g_Chain.Cut(arg->drawChain);
  arg->drawChain = NULL;
  arg->Release();
  free(arg);

  return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0045c5d0
ZunResult MainMenu::RegisterChain()

{
  MainMenu *mainMenu = new MainMenu;

  // memset it twice just for good measure i guess
  memset(mainMenu, 0, sizeof(MainMenu));

  g_GameManager.isInGameMenu = 0;
  mainMenu->calcChain = Chain::CreateElem((ChainCallback)OnUpdate);
  mainMenu->calcChain->arg = mainMenu;
  mainMenu->calcChain->addedCallback = (ChainLifecycleCallback)AddedCallback;
  mainMenu->calcChain->deletedCallback =
      (ChainLifecycleCallback)DeletedCallback;
  if (g_Chain.AddToCalcChain(mainMenu->calcChain, 3) == 0) {
    mainMenu->drawChain = Chain::CreateElem((ChainCallback)OnDraw);
    mainMenu->drawChain->arg = mainMenu;
    g_Chain.AddToDrawChain(mainMenu->drawChain, 0);
    return ZUN_SUCCESS;
  } else {
    return ZUN_ERROR;
  }
}
