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
#include "ZunMemory.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"
#include "utils.hpp"

// GLOBAL: TH07 0x0049ea7c
const char *g_DemoReplayPaths[3] = {
    // STRING: TH07 0x00495ae8
    "data/demo/demorpy0.rpy",
    // STRING: TH07 0x00495ad0
    "data/demo/demorpy1.rpy",
    // STRING: TH07 0x00495ab8
    "data/demo/demorpy2.rpy",
};

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
    // STRING: TH07 0x00495628
    "Easy    ",
    // STRING: TH07 0x0049561c
    "Normal  ",
    // STRING: TH07 0x00495610
    "Hard    ",
    // STRING: TH07 0x00495604
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

// GLOBAL: TH07 0x0049f474
i16 g_LastJoystickInput = 32;

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

// FUNCTION: TH07 0x004554d6
u32 MainMenu::OnUpdate(MainMenu *arg)
{
    u32 result;

    switch (arg->gameState)
    {
    case STATE_PRE_INPUT:
        result = arg->OnUpdatePreInput();
        break;
    case STATE_SELECT_REPLAY:
        result = arg->OnUpdateSelectReplay();
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
    case STATE_SELECT_PRACTICE_STAGE:
        result = arg->OnUpdateSelectPracticeStage();
    }
    g_AnmManager->ExecuteScripts(arg->vmHead, arg->vmCount);
    if (arg->cursorVm)
    {
        g_AnmManager->ExecuteScript(arg->cursorVm);
    }

    return result;
}

// FUNCTION: TH07 0x004555dd
u32 MainMenu::OnUpdatePreInput()
{
    i32 i;

    switch (this->menuSubState)
    {
    case 0:
        if (this->prevGameState == STATE_PRE_INPUT &&
            g_Supervisor.prevState != 5)
        {
            g_Supervisor.PlayLoadedAudio(8);
        }
        if ((this->prevGameState == STATE_PRE_INPUT ||
             this->prevGameState == 4 ||
             this->prevGameState == STATE_SELECT_REPLAY ||
             (this->prevGameState == 8 ||
              this->prevGameState == STATE_EXTRA_SELECT_DIFFICULTY)) &&
            g_AnmManager->LoadSurface(0, "data/title/title00.jpg") != ZUN_SUCCESS)
        {
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
        if (this->vmCount == 0)
        {
            this->vmCount = 164;
            this->vmHead = new AnmVm[this->vmCount];
            g_AnmManager->ExecuteVmsAnms(this->vmHead, 2304, this->vmCount);
        }
        g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 2);
        for (i = 0; i < 8; i++)
        {
            g_AnmManager->SetActiveSprite(&this->vmHead[i + 1],
                                          this->vmHead[i + 1].baseSpriteIdx + 1);
        }
        g_AnmManager->SetActiveSprite(
            &this->vmHead[this->cursor + 1],
            (i32)this->vmHead[this->cursor + 1].baseSpriteIdx);
        this->menuSubState = 0;
        this->inputDelayTimer = 0;
        this->selected = -1;
        this->menuSubState = 1;
        this->demoFramesCount = 0;
        if (g_GameManager.replay)
        {
            this->prevGameState = this->gameState;
            this->gameState = STATE_SELECT_REPLAY;
            this->inputDelayTimer = 0;
            this->stateTimer = 0;
            this->menuSubState = 0;
            this->idleFrames = 0;
            g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 13);
            this->cursorVm->SetInterrupt(2);
            g_GameManager.SetReplay(0);
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        if (this->isPracticeMode)
        {
            this->prevGameState = this->gameState;
            this->gameState = STATE_PRACTICE_SELECT_DIFFICULTY;
            this->inputDelayTimer = 0;
            this->stateTimer = 0;
            this->menuSubState = 0;
            this->idleFrames = 0;
            g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 5);
            this->cursorVm->SetInterrupt(2);
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        for (i = 0; (u32)i < 8; i++)
        {
            g_AnmManager->DrawStringFormat2(&this->vms[i], 0xfff0e0, 0x300000,
                                            g_MainMenuStrings[i]);
        }
    case 1: {
        i = MoveCursorVertical(8);
        if (i != 0)
        {
            while (g_GameManager.HasReachedMaxClearsAllShotTypes() == 0 &&
                   this->cursor == 1)
            {
                this->cursor += i;
            }
            for (i = 0; i < 8; i++)
            {
                g_AnmManager->SetActiveSprite(&this->vmHead[i + 1],
                                              this->vmHead[i + 1].baseSpriteIdx + 1);
            }
            g_AnmManager->SetActiveSprite(
                &this->vmHead[this->cursor + 1],
                (i32)this->vmHead[this->cursor + 1].baseSpriteIdx);
        }
        this->demoFramesCount++;
        if (g_CurFrameRawInput != 0)
        {
            this->demoFramesCount = 0;
        }
        if (900 < this->demoFramesCount)
        {
            g_GameManager.demoIdx++;
            g_GameManager.demoIdx %= 3;
            strcpy(g_GameManager.replayFilename,
                   g_DemoReplayPaths[g_GameManager.demoIdx]);
            this->currentReplay = (ReplayFile *)FileSystem::OpenFile(
                g_GameManager.replayFilename, 0);
            this->currentReplay =
                ReplayManager::ValidateReplayData(this->currentReplay, g_LastFileSize);
            if (!this->currentReplay)
            {
                Supervisor::DebugPrint2("error : Demo Play is not ready\r\n");
                this->demoFramesCount = 0;
            }
            else
            {
                g_GameManager.SetReplay(1);
                g_GameManager.flags |= 2;
                g_GameManager.demoFrames = 0;
                g_GameManager.difficulty = this->currentReplay->data.difficulty;
                g_GameManager.character = this->currentReplay->data.shotType / 2;
                g_GameManager.shotType = this->currentReplay->data.shotType % 2;
                g_GameManager.shotTypeAndCharacter = this->currentReplay->data.shotType;
                i = 0;
                while (!this->currentReplay->head.stageReplayData[i].data)
                {
                    i++;
                }

                g_GameManager.currentStage = i;
                ZunMemory::Free(this->currentReplay);
                this->currentReplay = NULL;
                g_Supervisor.curState = 2;
                g_GameManager.replayStage = 0;
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            }
        }
        if (this->selected != this->cursor)
        {
            this->cursorVm = &this->vms[this->cursor];
            this->cursorVm->SetInterrupt(1);
        }
        this->selected = this->cursor;
        if (this->stateTimer < 10)
        {
            break;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            g_SoundPlayer.ProcessQueues();
            switch (this->cursor)
            {
            case 0:
                g_GameManager.practice = 0;
                this->cursor = g_Supervisor.cfg.defaultDifficulty;
                if (this->cursor >= 4)
                {
                    this->cursor = 2;
                }
                this->prevGameState = this->gameState;
                this->gameState = STATE_NORMAL_SELECT_DIFFICULTY;
                this->inputDelayTimer = 0;
                this->stateTimer = 0;
                this->menuSubState = 0;
                this->idleFrames = 0;
                g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 5);
                this->cursorVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE;
            case 2:
                g_GameManager.practice = 1;
                this->cursor = g_Supervisor.cfg.defaultDifficulty;
                if (this->cursor >= 4)
                {
                    this->cursor = 2;
                }
                this->prevGameState = this->gameState;
                this->gameState = STATE_PRACTICE_SELECT_DIFFICULTY;
                this->inputDelayTimer = 0;
                this->stateTimer = 0;
                this->menuSubState = 0;
                this->idleFrames = 0;
                g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 5);
                this->cursorVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE;
            case 1:
                if (g_GameManager.HasReachedMaxClearsAllShotTypes())
                {
                    g_GameManager.practice = 0;
                    this->cursor = g_Supervisor.cfg.defaultDifficulty == 5;
                    this->prevGameState = this->gameState;
                    this->gameState = STATE_EXTRA_SELECT_DIFFICULTY;
                    this->inputDelayTimer = 0;
                    this->stateTimer = 0;
                    this->menuSubState = 0;
                    this->idleFrames = 0;
                    g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 5);
                    this->cursorVm->SetInterrupt(2);
                    return CHAIN_CALLBACK_RESULT_CONTINUE;
                }
            case 3:
                g_GameManager.practice = 0;
                this->prevGameState = this->gameState;
                this->gameState = STATE_SELECT_REPLAY;
                this->inputDelayTimer = 0;
                this->stateTimer = 0;
                this->menuSubState = 0;
                this->idleFrames = 0;
                g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 13);
                this->cursorVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE;
            case 5:
                g_Supervisor.curState = 8;
                this->cursorVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            case 4:
                g_Supervisor.curState = 5;
                this->cursorVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            case 6:
                this->menuSubState = 0;
                this->cursor = 0;
                this->stateTimer = 0;
                this->inputDelayTimer = 0;
                this->menuSubState = 3;
                this->inputDelayTimer = 0;
                OnUpdateOptionsMenu();
                this->cursor = 0;
                break;
            case 7:
                this->menuSubState = 2;
                this->inputDelayTimer = 0;
                g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 1);
                if (g_Supervisor.cfg.musicMode == 2)
                {
                    g_Supervisor.midiOutput->PlayLoaded(30);
                }
                break;
            }
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            g_AnmManager->SetActiveSprite(
                &this->vmHead[this->cursor + 1],
                this->vmHead[this->cursor + 1].baseSpriteIdx + 1);
            this->cursor = 7;
            g_AnmManager->SetActiveSprite(
                &this->vmHead[this->cursor + 1],
                (i32)this->vmHead[this->cursor + 1].baseSpriteIdx);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            g_SoundPlayer.ProcessQueues();
        }
        break;
    }
    case 2:
        if (this->inputDelayTimer >= 60)
        {
            delete[] this->vmHead;
            this->vmHead = NULL;
            this->vmHead = NULL;
            this->vmCount = 0;
            this->stateTimer = 0;
            g_Supervisor.curState = -1;
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
        break;
    case 3:
        if (this->inputDelayTimer >= 30)
        {
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
        break;
    }
    this->idleFrames++;
    this->inputDelayTimer++;
    this->stateTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0045624d
u32 MainMenu::OnUpdateOptionsMenu()
{
    i32 i;

    switch (this->menuSubState)
    {
    default:
        goto LAB_00456e08;
    case 0:
        if (this->stateTimer == 0)
        {
            g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 3);
            for (i = 0; i < 9; i++)
            {
                g_AnmManager->SetActiveSprite(&this->vmHead[i + 9],
                                              this->vmHead[i + 9].baseSpriteIdx + 1);
            }
            g_AnmManager->SetActiveSprite(
                &this->vmHead[this->cursor + 9],
                (i32)this->vmHead[this->cursor + 9].baseSpriteIdx);
            this->menuSubState = 0;
            this->inputDelayTimer = 0;
            this->selected = -1;
        }
        this->menuSubState = 1;
        for (i = 0; (u32)i < 9; i++)
        {
            g_AnmManager->DrawStringFormat2(&this->vms[i], 0xfff0e0, 0x300000,
                                            g_OptionsStrings[i]);
        }
    case 1:
        break;
    }

    if (MoveCursorVertical(9))
    {
        for (i = 0; i < 9; i++)
        {
            g_AnmManager->SetActiveSprite(&this->vmHead[i + 9],
                                          this->vmHead[i + 9].baseSpriteIdx + 1);
        }
        g_AnmManager->SetActiveSprite(
            &this->vmHead[this->cursor + 9],
            (i32)this->vmHead[this->cursor + 9].baseSpriteIdx);
    }

    if (this->selected != this->cursor)
    {
        this->cursorVm = &this->vms[this->cursor];
        this->cursorVm->SetInterrupt(1);
    }
    this->selected = this->cursor;

    for (i = 18; i <= 22; i++)
    {
        g_AnmManager->SetActiveSprite(&this->vmHead[i],
                                      this->vmHead[i].baseSpriteIdx + 1);
    }
    i = g_Supervisor.cfg.lifeCount + 18;
    g_AnmManager->SetActiveSprite(
        &this->vmHead[i],
        (i32)this->vmHead[i].baseSpriteIdx);

    for (i = 23; i <= 24; i++)
    {
        g_AnmManager->SetActiveSprite(&this->vmHead[i],
                                      this->vmHead[i].baseSpriteIdx + 1);
    }
    i = g_Supervisor.cfg.colorMode16bit + 23;
    g_AnmManager->SetActiveSprite(
        &this->vmHead[i],
        (i32)this->vmHead[i].baseSpriteIdx);

    for (i = 25; i <= 27; i++)
    {
        g_AnmManager->SetActiveSprite(&this->vmHead[i],
                                      this->vmHead[i].baseSpriteIdx + 1);
    }
    i = g_Supervisor.cfg.musicMode + 25;
    g_AnmManager->SetActiveSprite(
        &this->vmHead[i],
        (i32)this->vmHead[i].baseSpriteIdx);

    for (i = 28; i <= 29; i++)
    {
        g_AnmManager->SetActiveSprite(&this->vmHead[i],
                                      this->vmHead[i].baseSpriteIdx + 1);
    }
    i = g_Supervisor.cfg.playSounds + 28;
    g_AnmManager->SetActiveSprite(
        &this->vmHead[i],
        (i32)this->vmHead[i].baseSpriteIdx);

    for (i = 30; i <= 31; i++)
    {
        g_AnmManager->SetActiveSprite(&this->vmHead[i],
                                      this->vmHead[i].baseSpriteIdx + 1);
    }
    i = g_Supervisor.cfg.windowed + 30;
    g_AnmManager->SetActiveSprite(
        &this->vmHead[i],
        (i32)this->vmHead[i].baseSpriteIdx);

    for (i = 32; i <= 33; i++)
    {
        g_AnmManager->SetActiveSprite(&this->vmHead[i],
                                      this->vmHead[i].baseSpriteIdx + 1);
    }
    i = g_Supervisor.cfg.slowMode + 32;
    g_AnmManager->SetActiveSprite(
        &this->vmHead[i],
        (i32)this->vmHead[i].baseSpriteIdx);

    if (this->stateTimer < 4)
    {
        goto LAB_00456e08;
    }

    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_LEFT))
    {
        switch (this->cursor)
        {
        case 0:
            if (g_Supervisor.cfg.lifeCount == 0)
            {
                g_Supervisor.cfg.lifeCount = 4;
            }
            else
            {
                g_Supervisor.cfg.lifeCount--;
            }
            break;
        case 1:
            if (!g_Supervisor.cfg.colorMode16bit)
            {
                g_Supervisor.cfg.colorMode16bit = 1;
            }
            else
            {
                g_Supervisor.cfg.colorMode16bit--;
            }
            break;
        case 2:
            g_Supervisor.StopAudio();
            if (g_Supervisor.cfg.musicMode == MUSIC_MIDI)
            {
                g_Supervisor.midiOutput->PlayLoaded(30);
            }
            if (g_Supervisor.cfg.musicMode == MUSIC_OFF)
            {
                g_Supervisor.cfg.musicMode = MUSIC_MIDI;
            }
            else
            {
                g_Supervisor.cfg.musicMode--;
            }
            if (!g_Supervisor.cfg.preloadBgm &&
                g_Supervisor.cfg.musicMode == MUSIC_MIDI)
            {
                g_SoundPlayer.StartBGM("thbgm.dat");
            }
            g_Supervisor.LoadAudio(8, "bgm/th07_01.mid");
            g_Supervisor.PlayLoadedAudio(8);
            break;
        case 3:
            if (!g_Supervisor.cfg.playSounds)
            {
                g_Supervisor.cfg.playSounds = 1;
            }
            else
            {
                g_Supervisor.cfg.playSounds--;
            }
            break;
        case 4:
            if (!g_Supervisor.cfg.windowed)
            {
                g_Supervisor.cfg.windowed = 1;
            }
            else
            {
                g_Supervisor.cfg.windowed--;
            }
            break;
        case 5:
            if (!g_Supervisor.cfg.slowMode)
            {
                g_Supervisor.cfg.slowMode = 1;
            }
            else
            {
                g_Supervisor.cfg.slowMode--;
            }
            break;
        default:
            goto skip_left_sound;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        g_SoundPlayer.ProcessQueues();
    }

skip_left_sound:
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_RIGHT))
    {
        switch (this->cursor)
        {
        case 0:
            if (g_Supervisor.cfg.lifeCount >= 4)
            {
                g_Supervisor.cfg.lifeCount = 0;
            }
            else
            {
                g_Supervisor.cfg.lifeCount++;
            }
            break;
        case 1:
            if (g_Supervisor.cfg.colorMode16bit >= 1)
            {
                g_Supervisor.cfg.colorMode16bit = 0;
            }
            else
            {
                g_Supervisor.cfg.colorMode16bit++;
            }
            break;
        case 2:
            g_Supervisor.StopAudio();
            if (g_Supervisor.cfg.musicMode >= MUSIC_MIDI)
            {
                g_Supervisor.cfg.musicMode = MUSIC_OFF;
            }
            else
            {
                g_Supervisor.cfg.musicMode++;
            }
            g_Supervisor.LoadAudio(8, "bgm/th07_01.mid");
            g_Supervisor.PlayLoadedAudio(8);
            break;
        case 3:
            if (g_Supervisor.cfg.playSounds >= 1)
            {
                g_Supervisor.cfg.playSounds = 0;
            }
            else
            {
                g_Supervisor.cfg.playSounds++;
            }
            break;
        case 4:
            if (g_Supervisor.cfg.windowed >= 1)
            {
                g_Supervisor.cfg.windowed = 0;
            }
            else
            {
                g_Supervisor.cfg.windowed++;
            }
            break;
        case 5:
            if (g_Supervisor.cfg.slowMode >= 1)
            {
                g_Supervisor.cfg.slowMode = 0;
            }
            else
            {
                g_Supervisor.cfg.slowMode++;
            }
            break;
        default:
            goto skip_right_sound;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        g_SoundPlayer.ProcessQueues();
    }

skip_right_sound:
    if (g_CurFrameRawInput)
    {
        this->idleFrames = 0;
    }

    if (this->idleFrames >= 3600)
    {
        goto LAB_00456cc0;
    }

    if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
    {
        switch (this->cursor)
        {
        case 6:
            g_Supervisor.cfg.lifeCount = 2;
            g_Supervisor.cfg.bombCount = 3;
            g_Supervisor.cfg.musicMode = MUSIC_WAV;
            g_Supervisor.cfg.playSounds = 1;
            g_Supervisor.cfg.slowMode = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            g_SoundPlayer.ProcessQueues();
            break;
        case 7:
            this->cursor = 0;
            SetGameState(STATE_KEY_CONFIG);
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            g_SoundPlayer.ProcessQueues();
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        case 8:
        LAB_00456cc0:
            this->cursor = 6;
            SetGameState(STATE_PRE_INPUT);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            g_SoundPlayer.ProcessQueues();
            if (this->cfg.colorMode16bit != g_Supervisor.cfg.colorMode16bit ||
                this->cfg.windowed != g_Supervisor.cfg.windowed)
            {
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR;
            }
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
    }

    if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
    {
        if (this->cursor == 8)
        {
            goto LAB_00456cc0;
        }
        g_AnmManager->SetActiveSprite(
            &this->vmHead[this->cursor + 9],
            this->vmHead[this->cursor + 9].baseSpriteIdx + 1);
        this->cursor = 8;
        g_AnmManager->SetActiveSprite(
            &this->vmHead[this->cursor + 9],
            (i32)this->vmHead[this->cursor + 9].baseSpriteIdx);
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
        g_SoundPlayer.ProcessQueues();
    }

LAB_00456e08:
    this->idleFrames++;
    this->inputDelayTimer++;
    this->stateTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00456e40
void MainMenu::SwapMapping(i16 btnPressed, i16 oldMapping, i16 idk)
{
    if (this->controlMapping.shootButton == btnPressed)
    {
        this->controlMapping.shootButton = oldMapping;
    }
    if (this->controlMapping.bombButton == btnPressed)
    {
        this->controlMapping.bombButton = oldMapping;
    }
    if (this->controlMapping.focusButton == btnPressed)
    {
        this->controlMapping.focusButton = oldMapping;
    }
    if (this->controlMapping.upButton == btnPressed)
    {
        this->controlMapping.upButton = oldMapping;
    }
    if (this->controlMapping.downButton == btnPressed)
    {
        this->controlMapping.downButton = oldMapping;
    }
    if (this->controlMapping.leftButton == btnPressed)
    {
        this->controlMapping.leftButton = oldMapping;
    }
    if (this->controlMapping.rightButton == btnPressed)
    {
        this->controlMapping.rightButton = oldMapping;
    }
    if (this->controlMapping.menuButton == btnPressed)
    {
        this->controlMapping.menuButton = oldMapping;
    }
    if (this->controlMapping.skipButton == btnPressed)
    {
        this->controlMapping.skipButton = oldMapping;
    }
}

#pragma var_order(vm, i, btnPressed, controllerState, cursorVmTmp)
// FUNCTION: TH07 0x00456f6b
u32 MainMenu::OnUpdateKeyConfig()
{
    AnmVm *vm;
    i32 i;
    i16 btnPressed;
    u8 *controllerState;
    AnmVm *cursorVmTmp;

    switch (this->menuSubState)
    {
    case 0:
        if (this->stateTimer == 0)
        {
            g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 4);
            for (i = 0; i < 12; i++)
            {
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[i + 35],
                    this->vmHead[i + 35].baseSpriteIdx + 1);
            }
            g_AnmManager->SetActiveSprite(
                &this->vmHead[this->cursor + 35],
                (i32)this->vmHead[this->cursor + 35].baseSpriteIdx);
            this->menuSubState = 0;
            this->inputDelayTimer = 0;
            this->controlMapping = g_Supervisor.cfg.controllerMapping;
            g_Supervisor.cfg.controllerMapping.upButton = -1;
            g_Supervisor.cfg.controllerMapping.downButton = -1;

            vm = &this->vmHead[47];
            UpdateMenuDigits(vm, this->controlMapping.shootButton);
            vm += 2;
            UpdateMenuDigits(vm, this->controlMapping.bombButton);
            vm += 2;
            UpdateMenuDigits(vm, this->controlMapping.focusButton);
            vm += 2;
            UpdateMenuDigits(vm, this->controlMapping.skipButton);
            vm += 2;
            UpdateMenuDigits(vm, this->controlMapping.menuButton);
            vm += 2;
            UpdateMenuDigits(vm, this->controlMapping.upButton);
            vm += 2;
            UpdateMenuDigits(vm, this->controlMapping.downButton);
            vm += 2;
            UpdateMenuDigits(vm, this->controlMapping.leftButton);
            vm += 2;
            UpdateMenuDigits(vm, this->controlMapping.rightButton);

            this->selected = -1;
        }
        this->menuSubState = 1;
        for (i = 0; (u32)i < 12; i++)
        {
            g_AnmManager->DrawStringFormat2(&this->vms[i], 0xfff0e0, 0x300000,
                                            g_KeyConfigStrings[i]);
        }
    case 1:
        if (MoveCursorVertical(12))
        {
            for (i = 0; i < 12; i++)
            {
                g_AnmManager->SetActiveSprite(&this->vmHead[i + 35],
                                              this->vmHead[i + 35].baseSpriteIdx +
                                                  1);
            }
            g_AnmManager->SetActiveSprite(
                &this->vmHead[this->cursor + 35],
                (i32)this->vmHead[this->cursor + 35].baseSpriteIdx);
        }
        if (this->selected != this->cursor)
        {
            this->cursorVm = &this->vms[this->cursor];
            // this should be using SetInterrupt?
            cursorVmTmp = this->cursorVm;
            cursorVmTmp->pendingInterrupt = 1;
        }
        this->selected = this->cursor;

        vm = &this->vmHead[47];
        UpdateMenuDigits(vm, this->controlMapping.shootButton);
        vm += 2;
        UpdateMenuDigits(vm, this->controlMapping.bombButton);
        vm += 2;
        UpdateMenuDigits(vm, this->controlMapping.focusButton);
        vm += 2;
        UpdateMenuDigits(vm, this->controlMapping.skipButton);
        vm += 2;
        UpdateMenuDigits(vm, this->controlMapping.menuButton);
        vm += 2;
        UpdateMenuDigits(vm, this->controlMapping.upButton);
        vm += 2;
        UpdateMenuDigits(vm, this->controlMapping.downButton);
        vm += 2;
        UpdateMenuDigits(vm, this->controlMapping.leftButton);
        vm += 2;
        UpdateMenuDigits(vm, this->controlMapping.rightButton);

        for (i = 65; i <= 66; i++)
        {
            g_AnmManager->SetActiveSprite(&this->vmHead[i],
                                          this->vmHead[i].baseSpriteIdx + 1);
        }
        i = g_Supervisor.cfg.shotSlow + 65;
        g_AnmManager->SetActiveSprite(
            &this->vmHead[i],
            (i32)this->vmHead[i].baseSpriteIdx);

        controllerState = Controller::GetControllerState();
        for (btnPressed = 0; btnPressed < 32; btnPressed++)
        {
            if (controllerState[btnPressed] & 0x80)
            {
                break;
            }
        }
        if (btnPressed < 32 && g_LastJoystickInput != btnPressed)
        {
            switch (this->cursor)
            {
            case 0:
                SwapMapping(btnPressed, this->controlMapping.shootButton, 1);
                this->controlMapping.shootButton = btnPressed;
                break;
            case 1:
                SwapMapping(btnPressed, this->controlMapping.bombButton, 0);
                this->controlMapping.bombButton = btnPressed;
                break;
            case 2:
                SwapMapping(btnPressed, this->controlMapping.focusButton, 1);
                this->controlMapping.focusButton = btnPressed;
                break;
            case 4:
                SwapMapping(btnPressed, this->controlMapping.menuButton, 0);
                this->controlMapping.menuButton = btnPressed;
                break;
            case 5:
                SwapMapping(btnPressed, this->controlMapping.upButton, 0);
                this->controlMapping.upButton = btnPressed;
                break;
            case 6:
                SwapMapping(btnPressed, this->controlMapping.downButton, 0);
                this->controlMapping.downButton = btnPressed;
                break;
            case 7:
                SwapMapping(btnPressed, this->controlMapping.leftButton, 0);
                this->controlMapping.leftButton = btnPressed;
                break;
            case 8:
                SwapMapping(btnPressed, this->controlMapping.rightButton, 0);
                this->controlMapping.rightButton = btnPressed;
                break;
            case 3:
                SwapMapping(btnPressed, this->controlMapping.skipButton, 0);
                this->controlMapping.skipButton = btnPressed;
                break;
            default:
                goto switchD_00457548_default;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            g_SoundPlayer.ProcessQueues();
        }
    switchD_00457548_default:
        g_LastJoystickInput = btnPressed;

        if (WAS_PRESSED_RAW(TH_BUTTON_LEFT))
        {
            switch (this->cursor)
            {
            case 9:
                g_Supervisor.cfg.shotSlow = 1 - g_Supervisor.cfg.shotSlow;
            }
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RIGHT))
        {
            switch (this->cursor)
            {
            case 9:
                g_Supervisor.cfg.shotSlow = 1 - g_Supervisor.cfg.shotSlow;
            }
        }
        if (g_CurFrameRawInput)
        {
            this->idleFrames = 0;
        }
        if (this->idleFrames >= 3600)
        {
            goto exit_config;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            switch (this->cursor)
            {
            case 10:
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
                g_SoundPlayer.ProcessQueues();
                this->controlMapping = g_ControllerMapping;
                g_Supervisor.cfg.shotSlow = 1;
                break;
            case 11:
            exit_config:
                g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
                g_SoundPlayer.ProcessQueues();
                SetGameState(STATE_OPTIONS);
                g_Supervisor.cfg.controllerMapping = this->controlMapping;
                this->cursor = 7;
                return CHAIN_CALLBACK_RESULT_CONTINUE;
            }
        }
        break;
    }
    this->idleFrames++;
    this->inputDelayTimer++;
    this->stateTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x004578cc
ZunResult MainMenu::UpdateMenuDigits(AnmVm *param_1, i16 param_2)
{
    if (param_2 < 0)
    {
        param_1->active = 0;
        param_1[1].active = 0;
    }
    else
    {
        g_AnmManager->SetActiveSprite(param_1, (i32)param_1->baseSpriteIdx +
                                                   (i32)param_2 / 10 * 2);
        g_AnmManager->SetActiveSprite(param_1 + 1, (i32)param_1[1].baseSpriteIdx +
                                                       (i32)param_2 % 10 * 2);
        param_1->active = 1;
        param_1[1].active = 1;
    }
    return ZUN_SUCCESS;
}

#pragma var_order(numDifficulties, i, oldGameState)
// FUNCTION: TH07 0x0045798b
u32 MainMenu::OnUpdateSelectDifficulty()
{
    i32 oldGameState;
    i32 numDifficulties;
    i32 i;

    switch (this->menuSubState)
    {
    case 0:
        if (this->stateTimer == 0)
        {
            if (this->prevGameState != 5 && this->prevGameState != 9 &&
                this->prevGameState != STATE_EXTRA_SELECT_CHARACTER &&
                g_AnmManager->LoadSurface(0, "data/title/select00.jpg") !=
                    ZUN_SUCCESS)
            {
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            }
            this->cursor = g_Supervisor.cfg.defaultDifficulty;
            if (this->gameState != STATE_EXTRA_SELECT_DIFFICULTY)
            {
                g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 7);
            }
            else if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0)
            {
                g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 12);
                this->cursor = 4;
            }
            else
            {
                g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 22);
            }
            if (this->gameState != STATE_EXTRA_SELECT_DIFFICULTY)
            {
                if (this->cursor >= 4)
                {
                    this->cursor = 1;
                }
                for (i = 0; i < 4; i++)
                {
                    g_AnmManager->SetActiveSprite(
                        &this->vmHead[i + 67],
                        this->vmHead[i + 67].baseSpriteIdx + 1);
                }
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[this->cursor + 67],
                    (i32)this->vmHead[this->cursor + 67].baseSpriteIdx);
            }
            else
            {
                this->cursor -= 4;
                if (this->cursor < 0)
                {
                    this->cursor = 0;
                }
                for (i = 0; i < 2; i++)
                {
                    g_AnmManager->SetActiveSprite(
                        &this->vmHead[i + 162],
                        this->vmHead[i + 162].baseSpriteIdx + 1);
                }
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[this->cursor + 162],
                    (i32)this->vmHead[this->cursor + 162].baseSpriteIdx);
            }
            this->menuSubState = 0;
            this->inputDelayTimer = 0;
            this->cursorVm = NULL;
        }
        if (this->isPracticeMode)
        {
            SetGameState(STATE_PRACTICE_SELECT_CHARACTER);
            this->cursor = 0;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        if (this->stateTimer == 30)
        {
            this->menuSubState = 1;
        }
        break;
    case 1:
        numDifficulties = this->gameState != STATE_EXTRA_SELECT_DIFFICULTY
                              ? 4
                          : g_GameManager.HasUnlockedPhantomAndMaxClears()
                              ? 2
                              : 1;
        if (MoveCursorVertical(numDifficulties))
        {
            if (this->gameState != STATE_EXTRA_SELECT_DIFFICULTY)
            {
                for (i = 0; i < 4; i++)
                {
                    g_AnmManager->SetActiveSprite(
                        &this->vmHead[i + 67],
                        this->vmHead[i + 67].baseSpriteIdx + 1);
                }
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[this->cursor + 67],
                    (i32)this->vmHead[this->cursor + 67].baseSpriteIdx);
            }
            else if (numDifficulties == 2)
            {
                for (i = 0; i < 2; i++)
                {
                    g_AnmManager->SetActiveSprite(
                        &this->vmHead[i + 162],
                        this->vmHead[i + 162].baseSpriteIdx + 1);
                }
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[this->cursor + 162],
                    (i32)this->vmHead[this->cursor + 162].baseSpriteIdx);
            }
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            if (this->gameState != STATE_EXTRA_SELECT_DIFFICULTY)
            {
                g_Supervisor.cfg.defaultDifficulty = this->cursor;
            }
            else
            {
                g_Supervisor.cfg.defaultDifficulty = this->cursor + 4;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            g_SoundPlayer.ProcessQueues();
            if (this->gameState != STATE_EXTRA_SELECT_DIFFICULTY)
            {
                if (!g_GameManager.practice)
                {
                    SetGameState(STATE_NORMAL_SELECT_CHARACTER);
                }
                else
                {
                    SetGameState(STATE_PRACTICE_SELECT_CHARACTER);
                }
            }
            else
            {
                SetGameState(STATE_EXTRA_SELECT_CHARACTER);
            }

            this->cursor = 0;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            if (this->gameState != STATE_EXTRA_SELECT_DIFFICULTY)
            {
                g_Supervisor.cfg.defaultDifficulty = this->cursor;
            }
            else
            {
                g_Supervisor.cfg.defaultDifficulty = this->cursor + 4;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            g_SoundPlayer.ProcessQueues();
            this->menuSubState = 3;
            this->inputDelayTimer = 0;
            g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 6);
        }
        break;
    case 3:
        if (this->inputDelayTimer >= 30)
        {
            oldGameState = this->gameState;
            SetGameState(STATE_PRE_INPUT);
            if (oldGameState != STATE_EXTRA_SELECT_DIFFICULTY)
            {
                if (!g_GameManager.practice)
                {
                    this->cursor = 0;
                }
                else
                {
                    this->cursor = 2;
                }
            }
            else
            {
                this->cursor = 1;
            }
            g_GameManager.practice = 0;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
    }
    this->inputDelayTimer++;
    this->idleFrames++;
    this->stateTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00457fe5
u32 MainMenu::OnUpdateSelectCharacter()
{
    switch (this->menuSubState)
    {
    case 0:
        if (this->stateTimer == 0)
        {
            g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 8);
            if (g_Supervisor.cfg.defaultDifficulty < 4)
            {
                this->vmHead[g_Supervisor.cfg.defaultDifficulty + 67]
                    .SetInterrupt(9);
            }
            else
            {
                if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0)
                {
                    this->vmHead[161].SetInterrupt(9);
                }
                else
                {
                    this->vmHead[g_Supervisor.cfg.defaultDifficulty + 158]
                        .SetInterrupt(9);
                }
            }
            this->cursor = g_GameManager.character;
            if (g_Supervisor.cfg.defaultDifficulty == 4)
            {
                while (
                    g_GameManager.HasReachedMaxClears(this->cursor << 1) == 0 &&
                    g_GameManager.HasReachedMaxClears(this->cursor * 2 + 1) == 0)
                {
                    this->cursor++;
                    if (this->cursor >= 3)
                    {
                        this->cursor -= 3;
                    }
                }
            }
            else if (g_Supervisor.cfg.defaultDifficulty == 5)
            {
                while (
                    g_GameManager.HasUnlockedPhantom(this->cursor << 1) == 0 &&
                    g_GameManager.HasUnlockedPhantom(this->cursor * 2 + 1) == 0)
                {
                    this->cursor++;
                    if (this->cursor >= 3)
                    {
                        this->cursor -= 3;
                    }
                }
            }
            this->vmHead[72].active = 0;
            this->vmHead[73].active = 0;
            this->vmHead[71].active = 0;
            this->vmHead[80].active = 0;
            this->vmHead[83].active = 0;
            this->vmHead[75].active = 0;
            this->vmHead[76].active = 0;
            this->vmHead[74].active = 0;
            this->vmHead[81].active = 0;
            this->vmHead[84].active = 0;
            this->vmHead[78].active = 0;
            this->vmHead[79].active = 0;
            this->vmHead[77].active = 0;
            this->vmHead[82].active = 0;
            this->vmHead[85].active = 0;
            switch (this->cursor)
            {
            case 0:
                this->vmHead[72].active = 1;
                this->vmHead[73].active = 1;
                this->vmHead[71].active = 1;
                this->vmHead[80].active = 1;
                this->vmHead[83].active = 1;
                break;
            case 1:
                this->vmHead[75].active = 1;
                this->vmHead[76].active = 1;
                this->vmHead[74].active = 1;
                this->vmHead[81].active = 1;
                this->vmHead[84].active = 1;
                break;
            case 2:
                this->vmHead[78].active = 1;
                this->vmHead[79].active = 1;
                this->vmHead[77].active = 1;
                this->vmHead[82].active = 1;
                this->vmHead[85].active = 1;
                break;
            }
            switch (this->cursor)
            {
            case 0:
                this->vmHead[71].SetInterrupt(9);
                this->vmHead[74].SetInterrupt(8);
                this->vmHead[77].SetInterrupt(8);
                this->vmHead[74].color.bytes.a = 0;
                this->vmHead[77].color.bytes.a = 0;
                this->vmHead[80].SetInterrupt(9);
                this->vmHead[81].SetInterrupt(8);
                this->vmHead[82].SetInterrupt(8);
                this->vmHead[81].color.bytes.a = 0;
                this->vmHead[82].color.bytes.a = 0;
                this->vmHead[83].SetInterrupt(9);
                this->vmHead[84].SetInterrupt(8);
                this->vmHead[85].SetInterrupt(8);
                this->vmHead[84].color.bytes.a = 0;
                this->vmHead[85].color.bytes.a = 0;
                break;
            case 1:
                this->vmHead[71].SetInterrupt(8);
                this->vmHead[74].SetInterrupt(9);
                this->vmHead[77].SetInterrupt(8);
                this->vmHead[71].color.bytes.a = 0;
                this->vmHead[77].color.bytes.a = 0;
                this->vmHead[80].SetInterrupt(8);
                this->vmHead[81].SetInterrupt(9);
                this->vmHead[82].SetInterrupt(8);
                this->vmHead[80].color.bytes.a = 0;
                this->vmHead[82].color.bytes.a = 0;
                this->vmHead[83].SetInterrupt(8);
                this->vmHead[84].SetInterrupt(9);
                this->vmHead[85].SetInterrupt(8);
                this->vmHead[83].color.bytes.a = 0;
                this->vmHead[85].color.bytes.a = 0;
                break;
            case 2:
                this->vmHead[71].SetInterrupt(8);
                this->vmHead[74].SetInterrupt(8);
                this->vmHead[77].SetInterrupt(9);
                this->vmHead[74].color.bytes.a = 0;
                this->vmHead[71].color.bytes.a = 0;
                this->vmHead[80].SetInterrupt(8);
                this->vmHead[81].SetInterrupt(8);
                this->vmHead[82].SetInterrupt(9);
                this->vmHead[80].color.bytes.a = 0;
                this->vmHead[81].color.bytes.a = 0;
                this->vmHead[83].SetInterrupt(8);
                this->vmHead[84].SetInterrupt(8);
                this->vmHead[85].SetInterrupt(9);
                this->vmHead[83].color.bytes.a = 0;
                this->vmHead[84].color.bytes.a = 0;
                break;
            }
            this->menuSubState = 0;
            this->inputDelayTimer = 0;
        }
        if (this->isPracticeMode)
        {
            SetGameState(STATE_PRACTICE_SELECT_SHOTTYPE);
            this->cursor = 0;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        if (this->stateTimer == 30)
        {
            this->menuSubState = 1;
        }
        break;
    case 1:
        if (MoveCursorHorizontal(3) != ZUN_SUCCESS)
        {
            if (g_Supervisor.cfg.defaultDifficulty == 4)
            {
                while (
                    g_GameManager.HasReachedMaxClears(this->cursor << 1) == 0 &&
                    g_GameManager.HasReachedMaxClears(this->cursor * 2 + 1) == 0)
                {
                    this->cursor++;
                    if (this->cursor >= 3)
                    {
                        this->cursor -= 3;
                    }
                }
            }
            else if (g_Supervisor.cfg.defaultDifficulty == 5)
            {
                while (
                    g_GameManager.HasUnlockedPhantom(this->cursor << 1) == 0 &&
                    g_GameManager.HasUnlockedPhantom(this->cursor * 2 + 1) == 0)
                {
                    this->cursor++;
                    if (this->cursor >= 3)
                    {
                        this->cursor -= 3;
                    }
                }
            }
            this->vmHead[72].flags = this->vmHead[72].flags | 2;
            this->vmHead[73].flags = this->vmHead[73].flags | 2;
            this->vmHead[71].flags = this->vmHead[71].flags | 2;
            this->vmHead[80].flags = this->vmHead[80].flags | 2;
            this->vmHead[83].flags = this->vmHead[83].flags | 2;
            this->vmHead[75].flags = this->vmHead[75].flags | 2;
            this->vmHead[76].flags = this->vmHead[76].flags | 2;
            this->vmHead[74].flags = this->vmHead[74].flags | 2;
            this->vmHead[81].flags = this->vmHead[81].flags | 2;
            this->vmHead[84].flags = this->vmHead[84].flags | 2;
            this->vmHead[78].flags = this->vmHead[78].flags | 2;
            this->vmHead[79].flags = this->vmHead[79].flags | 2;
            this->vmHead[77].flags = this->vmHead[77].flags | 2;
            this->vmHead[82].flags = this->vmHead[82].flags | 2;
            this->vmHead[85].flags = this->vmHead[85].flags | 2;
            switch (this->cursor)
            {
            case 0:
                this->vmHead[71].SetInterrupt(9);
                this->vmHead[74].SetInterrupt(8);
                this->vmHead[77].SetInterrupt(8);
                this->vmHead[80].SetInterrupt(9);
                this->vmHead[81].SetInterrupt(8);
                this->vmHead[82].SetInterrupt(8);
                this->vmHead[83].SetInterrupt(9);
                this->vmHead[84].SetInterrupt(8);
                this->vmHead[85].SetInterrupt(8);
                break;
            case 1:
                this->vmHead[71].SetInterrupt(8);
                this->vmHead[74].SetInterrupt(9);
                this->vmHead[77].SetInterrupt(8);
                this->vmHead[80].SetInterrupt(8);
                this->vmHead[81].SetInterrupt(9);
                this->vmHead[82].SetInterrupt(8);
                this->vmHead[83].SetInterrupt(8);
                this->vmHead[84].SetInterrupt(9);
                this->vmHead[85].SetInterrupt(8);
                break;
            case 2:
                this->vmHead[71].SetInterrupt(8);
                this->vmHead[74].SetInterrupt(8);
                this->vmHead[77].SetInterrupt(9);
                this->vmHead[80].SetInterrupt(8);
                this->vmHead[81].SetInterrupt(8);
                this->vmHead[82].SetInterrupt(9);
                this->vmHead[83].SetInterrupt(8);
                this->vmHead[84].SetInterrupt(8);
                this->vmHead[85].SetInterrupt(9);
                break;
            }
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            g_GameManager.character = this->cursor;
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            g_SoundPlayer.ProcessQueues();
            if (this->gameState != STATE_EXTRA_SELECT_CHARACTER)
            {
                if (!g_GameManager.practice)
                {
                    SetGameState(STATE_NORMAL_SELECT_SHOTTYPE);
                }
                else
                {
                    SetGameState(STATE_PRACTICE_SELECT_SHOTTYPE);
                }
            }
            else
            {
                SetGameState(STATE_EXTRA_SELECT_SHOTTYPE);
            }
            this->cursor = 0;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            g_SoundPlayer.ProcessQueues();
            g_GameManager.character = this->cursor;
            if (this->gameState != STATE_EXTRA_SELECT_CHARACTER)
            {
                if (!g_GameManager.practice)
                {
                    SetGameState(STATE_NORMAL_SELECT_DIFFICULTY);
                }
                else
                {
                    SetGameState(STATE_PRACTICE_SELECT_DIFFICULTY);
                }
            }
            else
            {
                SetGameState(STATE_EXTRA_SELECT_DIFFICULTY);
            }
            this->cursor = 0;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        break;
    }
    this->idleFrames++;
    this->inputDelayTimer++;
    this->stateTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00459518
u32 MainMenu::OnUpdateSelectShotType()
{
    switch (this->menuSubState)
    {
    case 0:
        if (this->stateTimer == 0)
        {
            g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 10);
            if (g_Supervisor.cfg.defaultDifficulty < 4)
            {
                this->vmHead[g_Supervisor.cfg.defaultDifficulty + 67]
                    .SetInterrupt(9);
            }
            else
            {
                if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0)
                {
                    this->vmHead[161].SetInterrupt(9);
                }
                else
                {
                    this->vmHead[g_Supervisor.cfg.defaultDifficulty + 158]
                        .SetInterrupt(9);
                }
            }
            this->vmHead[72].active = 0;
            this->vmHead[73].active = 0;
            this->vmHead[71].active = 0;
            this->vmHead[80].active = 0;
            this->vmHead[83].active = 0;
            this->vmHead[75].active = 0;
            this->vmHead[76].active = 0;
            this->vmHead[74].active = 0;
            this->vmHead[81].active = 0;
            this->vmHead[84].active = 0;
            this->vmHead[78].active = 0;
            this->vmHead[79].active = 0;
            this->vmHead[77].active = 0;
            this->vmHead[82].active = 0;
            this->vmHead[85].active = 0;
            this->cursor = g_GameManager.shotType;
            if (g_Supervisor.cfg.defaultDifficulty == 4)
            {
                while (g_GameManager.HasReachedMaxClears(
                           this->cursor + (u32)g_GameManager.character * 2) == 0)
                {
                    this->cursor++;
                    if (this->cursor >= 2)
                    {
                        this->cursor = this->cursor - 2;
                    }
                }
            }
            else if (g_Supervisor.cfg.defaultDifficulty == 5)
            {
                while (g_GameManager.HasUnlockedPhantom(
                           this->cursor + (u32)g_GameManager.character * 2) == 0)
                {
                    this->cursor++;
                    if (this->cursor >= 2)
                    {
                        this->cursor = this->cursor - 2;
                    }
                }
            }
            switch (g_GameManager.character)
            {
            case CHAR_REIMU:
                this->vmHead[72].active = 1;
                this->vmHead[73].active = 1;
                this->vmHead[71].active = 1;
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[73 - this->cursor],
                    this->vmHead[73 - this->cursor].baseSpriteIdx + 1);
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[this->cursor + 72],
                    (i32)this->vmHead[this->cursor + 72].baseSpriteIdx);
                break;
            case CHAR_MARISA:
                this->vmHead[75].active = 1;
                this->vmHead[76].active = 1;
                this->vmHead[74].active = 1;
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[76 - this->cursor],
                    this->vmHead[76 - this->cursor].baseSpriteIdx + 1);
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[this->cursor + 75],
                    (i32)this->vmHead[this->cursor + 75].baseSpriteIdx);
                break;
            case CHAR_SAKUYA:
                this->vmHead[78].active = 1;
                this->vmHead[79].active = 1;
                this->vmHead[77].active = 1;
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[79 - this->cursor],
                    this->vmHead[79 - this->cursor].baseSpriteIdx + 1);
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[this->cursor + 78],
                    (i32)this->vmHead[this->cursor + 78].baseSpriteIdx);
                break;
            }
            this->menuSubState = 0;
            this->inputDelayTimer = 0;
        }
        if (this->isPracticeMode)
        {
            SetGameState(STATE_SELECT_PRACTICE_STAGE);
            this->isPracticeMode = 0;
            this->cursor = g_GameManager.currentStage - 1;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        if (this->stateTimer == 30)
        {
            this->menuSubState = 1;
        }
        break;
    case 1:
        if (MoveCursorVertical(2))
        {
            if (g_Supervisor.cfg.defaultDifficulty == 4)
            {
                while (g_GameManager.HasReachedMaxClears(
                           this->cursor + (u32)g_GameManager.character * 2) == 0)
                {
                    this->cursor++;
                    if (this->cursor >= 2)
                    {
                        this->cursor = this->cursor - 2;
                    }
                }
            }
            else if (g_Supervisor.cfg.defaultDifficulty == 5)
            {
                while (g_GameManager.HasUnlockedPhantom(
                           this->cursor + (u32)g_GameManager.character * 2) == 0)
                {
                    this->cursor++;
                    if (this->cursor >= 2)
                    {
                        this->cursor = this->cursor - 2;
                    }
                }
            }
            switch (g_GameManager.character)
            {
            case CHAR_REIMU:
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[73 - this->cursor],
                    this->vmHead[73 - this->cursor].baseSpriteIdx + 1);
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[this->cursor + 72],
                    (i32)this->vmHead[this->cursor + 72].baseSpriteIdx);
                break;
            case CHAR_MARISA:
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[76 - this->cursor],
                    this->vmHead[76 - this->cursor].baseSpriteIdx + 1);
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[this->cursor + 75],
                    (i32)this->vmHead[this->cursor + 75].baseSpriteIdx);
                break;
            case CHAR_SAKUYA:
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[79 - this->cursor],
                    this->vmHead[79 - this->cursor].baseSpriteIdx + 1);
                g_AnmManager->SetActiveSprite(
                    &this->vmHead[this->cursor + 78],
                    (i32)this->vmHead[this->cursor + 78].baseSpriteIdx);
                break;
            }
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            g_GameManager.shotType = this->cursor;
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            g_SoundPlayer.ProcessQueues();
            if (!g_GameManager.practice)
            {
                g_GameManager.difficulty = g_Supervisor.cfg.defaultDifficulty;
                if (g_GameManager.difficulty < DIFF_EXTRA)
                {
                    g_GameManager.currentStage = 0;
                }
                else
                {
                    g_GameManager.currentStage = g_GameManager.difficulty + DIFF_HARD;
                }
                g_Supervisor.curState = 2;
                g_GameManager.SetReplay(0);
                g_Supervisor.StopAudio();
                while (g_SoundPlayer.ProcessQueues())
                    ;
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            }
            this->cursor = 0;
            SetGameState(STATE_SELECT_PRACTICE_STAGE);
            return CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            g_GameManager.shotType = this->cursor;
            if (this->gameState != STATE_EXTRA_SELECT_SHOTTYPE)
            {
                if (!g_GameManager.practice)
                {
                    SetGameState(STATE_NORMAL_SELECT_CHARACTER);
                }
                else
                {
                    SetGameState(STATE_PRACTICE_SELECT_CHARACTER);
                }
            }
            else
            {
                SetGameState(STATE_EXTRA_SELECT_CHARACTER);
            }
            this->vmHead[72].active = 1;
            this->vmHead[73].active = 1;
            this->vmHead[71].active = 1;
            this->vmHead[80].active = 1;
            this->vmHead[83].active = 1;
            this->vmHead[75].active = 1;
            this->vmHead[76].active = 1;
            this->vmHead[74].active = 1;
            this->vmHead[81].active = 1;
            this->vmHead[84].active = 1;
            this->vmHead[78].active = 1;
            this->vmHead[79].active = 1;
            this->vmHead[77].active = 1;
            this->vmHead[82].active = 1;
            this->vmHead[85].active = 1;
            return CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN;
        }
        break;
    }
    this->idleFrames++;
    this->inputDelayTimer++;
    this->stateTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0045a1dd
u32 MainMenu::OnUpdateSelectPracticeStage()
{
    i32 local_8;

    switch (this->menuSubState)
    {
    case 0:
        if (this->stateTimer == 0)
        {
            g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 18);
            this->vmHead[72].active = 0;
            this->vmHead[73].active = 0;
            this->vmHead[71].active = 0;
            this->vmHead[80].active = 0;
            this->vmHead[83].active = 0;
            this->vmHead[75].active = 0;
            this->vmHead[76].active = 0;
            this->vmHead[74].active = 0;
            this->vmHead[81].active = 0;
            this->vmHead[84].active = 0;
            this->vmHead[78].active = 0;
            this->vmHead[79].active = 0;
            this->vmHead[77].active = 0;
            this->vmHead[82].active = 0;
            this->vmHead[85].active = 0;
            switch (g_GameManager.character)
            {
            case CHAR_REIMU:
                this->vmHead[72].active = 1;
                this->vmHead[73].active = 1;
                this->vmHead[71].active = 1;
                break;
            case CHAR_MARISA:
                this->vmHead[75].active = 1;
                this->vmHead[76].active = 1;
                this->vmHead[74].active = 1;
                break;
            case CHAR_SAKUYA:
                this->vmHead[78].active = 1;
                this->vmHead[79].active = 1;
                this->vmHead[77].active = 1;
                break;
            }
            this->menuSubState = 0;
            this->inputDelayTimer = 0;
            g_GameManager.practice = 1;
        }
        if (this->stateTimer == 30)
        {
            this->menuSubState = 1;
        }
        break;
    case 1:
        local_8 =
            g_GameManager.clrd[g_GameManager.character * 2 + g_GameManager.shotType]
                .difficultyClearedWithoutRetries[g_Supervisor.cfg.defaultDifficulty];
        if (local_8 < 0)
        {
            local_8 = 1;
        }
        else if (local_8 >= 99)
        {
            local_8 = 6;
        }
        if (this->cursor >= local_8)
        {
            this->cursor = 0;
        }
        MoveCursorVertical(local_8);
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            g_GameManager.difficulty = g_Supervisor.cfg.defaultDifficulty;
            g_GameManager.currentStage = this->cursor;
            g_Supervisor.curState = 2;

            i32 idk = 0;
            g_GameManager.replay = idk;
            g_Supervisor.StopAudio();
            while (g_SoundPlayer.ProcessQueues())
                ;
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->cursor = g_GameManager.shotType;
            SetGameState(STATE_NORMAL_SELECT_SHOTTYPE);
            this->vmHead[72].active = 1;
            this->vmHead[73].active = 1;
            this->vmHead[71].active = 1;
            this->vmHead[80].active = 1;
            this->vmHead[83].active = 1;
            this->vmHead[75].active = 1;
            this->vmHead[76].active = 1;
            this->vmHead[74].active = 1;
            this->vmHead[81].active = 1;
            this->vmHead[84].active = 1;
            this->vmHead[78].active = 1;
            this->vmHead[79].active = 1;
            this->vmHead[77].active = 1;
            this->vmHead[82].active = 1;
            this->vmHead[85].active = 1;
            return CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN;
        }
        break;
    }
    this->idleFrames++;
    this->inputDelayTimer++;
    this->stateTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(i, local_c, local_10, file, local_54, local_194)
// FUNCTION: TH07 0x0045a924
u32 MainMenu::OnUpdateSelectReplay()
{
    _WIN32_FIND_DATAA local_194;
    char local_54[64];
    ReplayFile *file;
    i32 local_10;
    HANDLE local_c;
    i32 i;

    switch (this->menuSubState)
    {
    case 0:
        if (this->stateTimer == 0)
        {
            if (this->prevGameState != STATE_SELECT_REPLAY &&
                // STRING: TH07 0x00495680
                g_AnmManager->LoadSurface(0, "data/title/select00.jpg") !=
                    ZUN_SUCCESS)
            {
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            }
            g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 14);
            this->cursor = 0;
            this->menuSubState = 0;
            this->inputDelayTimer = 0;
            this->cursorVm = NULL;
            local_10 = 0;
            for (i = 0; i < 15; i++)
            {
                // STRING: TH07 0x004967bc
                sprintf(local_54, "./replay/th7_%.2d.rpy", i + 1);
                file = (ReplayFile *)FileSystem::OpenFile(local_54, 1);
                if (!file)
                {
                    continue;
                }

                file =
                    ReplayManager::ValidateReplayData(file, g_LastFileSize);
                if (file)
                {
                    this->replays[local_10] = *file;
                    strcpy(this->replayFilenames[local_10], local_54);
                    // STRING: TH07 0x00496460
                    sprintf(this->replayLabels[local_10], "No.%.2d", i + 1);
                    local_10++;
                    free(file);
                }
            }
            // STRING: TH07 0x00495674
            _mkdir("./replay");
            _chdir("./replay");
            // STRING: TH07 0x00495664
            local_c = FindFirstFileA("th7_ud????.rpy", &local_194);
            if (local_c != INVALID_HANDLE_VALUE)
            {
                for (i = 0; i < 45; i++)
                {
                    file = (ReplayFile *)FileSystem::OpenFile(
                        local_194.cFileName, 1);
                    if (!file)
                    {
                        continue;
                    }
                    else
                    {
                        file =
                            ReplayManager::ValidateReplayData(file, g_LastFileSize);
                        if (file)
                        {
                            this->replays[local_10] = *file;
                            // STRING: TH07 0x00495658
                            sprintf(this->replayFilenames[local_10], "./replay/%s",
                                    local_194.cFileName);
                            // STRING: TH07 0x00495650
                            sprintf(this->replayLabels[local_10], "User ");
                            free(file);
                            local_10++;
                        }
                        if (FindNextFileA(local_c, &local_194) == 0)
                        {
                            break;
                        }
                    }
                }
            }
            FindClose(local_c);
            // STRING: TH07 0x0049564c
            _chdir("../");
            this->replayFilesNum = local_10;
            this->replayPage = 0;
        }
        if (this->stateTimer >= 30)
        {
            this->menuSubState = 1;
            this->inputDelayTimer = 0;
        }
        break;
    case 1:
        MoveCursorVertical(this->replayFilesNum);
        if (15 < this->replayFilesNum)
        {
            if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_LEFT))
            {
                this->cursor = this->cursor - 15;
                if (this->cursor < 0)
                {
                    this->cursor = this->cursor + this->replayFilesNum;
                }
                g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
            }
            if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_RIGHT))
            {
                this->cursor = this->cursor + 15;
                if (this->cursor >= this->replayFilesNum)
                {
                    this->cursor = this->cursor - this->replayFilesNum;
                }
                g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
            }
        }
        this->chosenReplay = this->cursor;
        if (this->inputDelayTimer < 10)
        {
            break;
        }

        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            if (this->replayFilesNum == 0)
            {
                break;
            }

            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            this->menuSubState = 2;
            g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 15);
            this->vmHead[this->chosenReplay % 15 + 135].SetInterrupt(17);
            this->currentReplay = (ReplayFile *)FileSystem::OpenFile(
                this->replayFilenames[this->chosenReplay], 1);
            this->currentReplay = ReplayManager::ValidateReplayData(
                this->currentReplay, g_LastFileSize);
            for (i = 0; i < 7; i++)
            {
                if (this->currentReplay->head.stageReplayData[i].offset != 0)
                {
                    this->currentReplay->head.stageReplayData[i].data =
                        (StageReplayData *)((u8 *)this->currentReplay +
                                            this->currentReplay->head.stageReplayData[i]
                                                .offset);
                }
            }
            this->cursor = 0;
            while (this->replays[this->chosenReplay]
                       .head.stageReplayData[this->cursor]
                       .data == NULL)
            {
                this->cursor++;
                if (this->cursor >= 7)
                {
                    // STRING: TH07 0x00495634
                    g_GameErrorContext.Fatal("リプレイデータが異常\r\n");
                    return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
                }
            }
            break;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->menuSubState = 4;
            this->inputDelayTimer = 0;
            g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 16);
        }
        break;
    case 2:
        i = MoveCursorVertical(7);
        if (i < 0)
        {
            while (this->replays[this->chosenReplay]
                       .head.stageReplayData[this->cursor]
                       .data == NULL)
            {
                this->cursor--;
                if (this->cursor < 0)
                {
                    this->cursor = 6;
                }
            }
        }
        else if (0 < i)
        {
            while (this->replays[this->chosenReplay]
                       .head.stageReplayData[this->cursor]
                       .data == NULL)
            {
                this->cursor++;
                if (this->cursor >= 7)
                {
                    this->cursor = 0;
                }
            }
        }
        this->selectedStage = this->cursor;
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 19);
            this->vmHead[this->chosenReplay % 15 + 135].SetInterrupt(17);
            this->menuSubState = 3;
            this->cursor = 0;
            this->vmHead[158].pendingInterrupt = 21;
            this->vmHead[159].pendingInterrupt = 21;
            this->vmHead[160].pendingInterrupt = 21;
            this->vmHead[this->cursor + 158].pendingInterrupt = 20;
            break;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            ZunMemory::Free(this->currentReplay);
            this->currentReplay = NULL;
            this->menuSubState = 1;
            this->stateTimer = 0;
            g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 14);
            this->cursor = this->chosenReplay;
            break;
        }
        break;
    case 3:
        i = MoveCursorVertical(3);
        if (i != 0)
        {
            this->vmHead[158].pendingInterrupt = 21;
            this->vmHead[159].pendingInterrupt = 21;
            this->vmHead[160].pendingInterrupt = 21;
            this->vmHead[this->cursor + 158].pendingInterrupt = 20;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            g_GameManager.SetReplay(1);
            strcpy(g_GameManager.replayFilename,
                   this->replayFilenames[this->chosenReplay]);
            g_GameManager.difficulty = this->currentReplay->data.difficulty;
            g_GameManager.character = this->currentReplay->data.shotType / 2;
            g_GameManager.shotType = this->currentReplay->data.shotType % 2;
            g_GameManager.shotTypeAndCharacter = this->currentReplay->data.shotType;
            ZunMemory::Free(this->currentReplay);
            this->currentReplay = NULL;
            g_GameManager.currentStage =
                g_GameManager.difficulty >= 5 ? 7 : this->selectedStage;
            g_Supervisor.curState = 2;
            g_GameManager.replayStage = (u8)this->cursor;
            g_Supervisor.StopAudio();
            while (g_SoundPlayer.ProcessQueues())
                ;
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            this->menuSubState = 2;
            this->stateTimer = 0;
            this->cursor = this->selectedStage;
            g_AnmManager->SetInterruptActiveVms(this->vmHead, this->vmCount, 15);
            this->vmHead[this->chosenReplay % 15 + 135].SetInterrupt(17);
            break;
        }
        break;
    case 4:
        if (this->inputDelayTimer >= 30)
        {
            SetGameState(STATE_PRE_INPUT);
            this->cursor = 3;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        break;
    }
    this->idleFrames++;
    this->inputDelayTimer++;
    this->stateTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(vm, i, replayAmount)
// FUNCTION: TH07 0x0045b5ef
i32 MainMenu::DrawReplayMenu()
{
    i32 replayAmount;
    i32 i;
    AnmVm *vm;

    vm = &this->vmHead[134];
    AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos,
                                // STRING: TH07 0x0049557c
                                "No.   Name       Date  Player   Rank");
    replayAmount = this->chosenReplay - this->chosenReplay % 15;
    for (i = replayAmount + 15; replayAmount < i; replayAmount++)
    {
        if (replayAmount >= this->replayFilesNum)
        {
            break;
        }
        vm++;
        g_AsciiManager.isSelected = IsReplaySelected(replayAmount);
        if (replayAmount == this->chosenReplay)
        {
            g_AsciiManager.color = 0xffffffff;
        }
        else
        {
            g_AsciiManager.color = 0xff808080;
        }
        AsciiManager::AddFormatText(
            // STRING: TH07 0x00495564
            &g_AsciiManager, &vm->pos, "%s %8s  %6s %7s  %8s",
            this->replayLabels + replayAmount,
            this->replays[replayAmount].data.name,
            this->replays[replayAmount].data.date,
            g_CharacterAndShottypeReplayStrings[this->replays[replayAmount]
                                                    .data.shotType],
            g_DifficultyStrings[this->replays[replayAmount].data.difficulty]);
    }
    if ((this->menuSubState == 2 || this->menuSubState == 3) && this->currentReplay != NULL)
    {
        g_AsciiManager.color = 0xffffffff;
        g_AsciiManager.isSelected = 0;
        vm = &this->vmHead[133];
        AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos,
                                    // STRING: TH07 0x00495554
                                    "       %2.3f%%",
                                    (double)this->currentReplay->data.slowdownRate);
        vm = &this->vmHead[150];
        AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos,
                                    // STRING: TH07 0x00495540
                                    "Stage    LastScore");
        replayAmount = this->chosenReplay - this->chosenReplay % 15;
        for (i = 0; i < 7; i++, replayAmount++)
        {
            vm++;
            if (this->menuSubState != 3)
            {
                g_AsciiManager.isSelected = IsStageSelected(i);
                if (i == this->selectedStage)
                {
                    g_AsciiManager.color = 0xffffffff;
                }
                else
                {
                    g_AsciiManager.color = 0xff808080;
                }
            }
            else
            {
                if (i == this->selectedStage)
                {
                    g_AsciiManager.color = 0x60ffffff;
                }
                else
                {
                    g_AsciiManager.color = 0x60808080;
                }
            }
            if (this->currentReplay->head.stageReplayData[i].data)
            {
                if (i < 6 || this->currentReplay->data.difficulty <= 4)
                {
                    // STRING: TH07 0x00495538
                    AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos, "%s %9d0",
                                                g_StageReplayStrings[i],
                                                this->currentReplay->head.stageReplayData[i].data->score);
                }
                else
                {
                    AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos, "%s %9d0",
                                                g_PhantasmReplayString,
                                                this->currentReplay->head.stageReplayData[i].data->score);
                }
            }
            else
            {
                if (i < 6 || this->currentReplay->data.difficulty <= 4)
                {
                    // STRING: TH07 0x00495528
                    AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos,
                                                "%s ----------",
                                                g_StageReplayStrings[i]);
                }
                else
                {
                    AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos,
                                                "%s ----------",
                                                g_PhantasmReplayString);
                }
            }
        }
    }
    g_AsciiManager.color = 0xffffffff;
    g_AsciiManager.isSelected = 0;
    return 1;
}

#pragma var_order(vm, i, local_10, local_1c)
// FUNCTION: TH07 0x0045b9ad
i32 MainMenu::DrawPracticeMenu()
{
    Float3 local_1c;
    i32 local_10;
    i32 i;
    AnmVm *vm;

    g_AsciiManager.color = 0xffffffff;
    g_AsciiManager.isSelected = 0;
    vm = &this->vmHead[131];
    AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos,
                                // STRING: TH07 0x004954e4
                                "Stage    HI-Score");
    local_1c = vm->pos;
    local_1c.y += 16.0f;
    local_10 =
        g_GameManager.clrd[g_GameManager.character * 2 + g_GameManager.shotType]
            .difficultyClearedWithoutRetries[g_Supervisor.cfg.defaultDifficulty];

    // ZUN bloat: this is always false, since difficultyClearedWithoutRetries is unsigned
    if (local_10 < 0)
    {
        local_10 = 1;
    }
    for (i = 0; i < 6; i++)
    {
        g_AsciiManager.isSelected = IsSelected(i);
        if (i == this->cursor)
        {
            g_AsciiManager.color = 0xffffffff;
        }
        else if (i < local_10)
        {
            g_AsciiManager.color = 0xffa0a0a0;
        }
        else
        {
            g_AsciiManager.color = 0xff404040;
        }
        AsciiManager::AddFormatText(
            // STRING: TH07 0x004954d4
            &g_AsciiManager, &local_1c, "%s %9d0 (%3d)",
            g_StagePracticeStrings[i],
            g_GameManager
                .pscr[g_GameManager.character * 2 + g_GameManager.shotType][i]
                     [g_Supervisor.cfg.defaultDifficulty]
                .score,
            g_GameManager
                .pscr[g_GameManager.character * 2 + g_GameManager.shotType][i]
                     [g_Supervisor.cfg.defaultDifficulty]
                .playCount);
        local_1c.y += 16.0f;
    }
    g_AsciiManager.color = 0xffffffff;
    g_AsciiManager.isSelected = 0;
    return 1;
}

// FUNCTION: TH07 0x0045bb4a
i32 MainMenu::MoveCursorVertical(i32 max)
{
    if (max == 0)
    {
        return 0;
    }
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_UP))
    {
        this->cursor--;
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        if (this->cursor < 0)
        {
            this->cursor = max - 1;
        }
        if (this->cursor >= max)
        {
            this->cursor = 0;
        }
        return -1;
    }
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_DOWN))
    {
        this->cursor++;
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        if (this->cursor < 0)
        {
            this->cursor = max - 1;
        }
        if (this->cursor >= max)
        {
            this->cursor = 0;
        }
        return 1;
    }
    return 0;
}

// FUNCTION: TH07 0x0045bc63
i32 MainMenu::MoveCursorHorizontal(i32 max)
{
    if (max == 0)
    {
        return 0;
    }
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_LEFT))
    {
        this->cursor = this->cursor - 1;
        if (this->cursor < 0)
        {
            this->cursor = this->cursor + max;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return -1;
    }
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_RIGHT))
    {
        this->cursor++;
        if (this->cursor >= max)
        {
            this->cursor = this->cursor - max;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return 1;
    }
    return 0;
}

#pragma var_order(i, local_c, savedPos)
// FUNCTION: TH07 0x0045bd6c
u32 MainMenu::OnDraw(MainMenu *arg)
{
    Float3 savedPos;
    AnmVm *local_c;
    i32 i;

    g_AnmManager->SetTexture(NULL);
    g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
    switch (arg->gameState)
    {
    case STATE_SELECT_REPLAY:
        arg->DrawReplayMenu();
        break;
    case STATE_SELECT_PRACTICE_STAGE:
        arg->DrawPracticeMenu();
        break;
    }
    local_c = arg->vmHead;
    for (i = 0; i < arg->vmCount; i++, local_c++)
    {
        if (g_AnmManager->ShouldDraw(local_c))
        {
            savedPos = local_c->pos;
            local_c->pos += local_c->offset;
            if (local_c->rotation.z != 0.0f)
            {
                g_AnmManager->Draw(local_c);
            }
            else
            {
                g_AnmManager->DrawNoRotation(local_c);
            }
            local_c->pos = savedPos;
        }
    }
    if (arg->cursorVm)
    {
        g_AnmManager->DrawNoRotation(arg->cursorVm);
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(local_8, frameCount, local_1c, local_20, local_24, local_34, i)
// FUNCTION: TH07 0x0045bf15
ZunResult MainMenu::ActualAddedCallback()
{
    i32 i;
    ZunRect local_34;
    ZunColor local_24;
    ZunColor local_20;
    ZunRect local_1c;
    i32 frameCount;
    ScoreDat *local_8;

    SAFE_DELETE(g_GameManager.defaultCfg);
    g_GameManager.defaultCfg = new GameConfiguration;
    SAFE_DELETE(g_GameManager.globals);
    g_GameManager.globals = new ZunGlobals;
    g_Supervisor.effectiveFramerateMultiplier = 1.0f;
    if (g_GameManager.replay)
    {
        g_GameManager.shotTypeAndCharacter = SHOT_REIMU_A;
        g_GameManager.character = g_GameManager.shotTypeAndCharacter;
    }
    if (g_GameManager.demo)
    {
        g_GameManager.replay = 0;
    }
    local_8 = ResultScreen::OpenScore("score.dat");
    ResultScreen::ParseClrd(local_8, g_GameManager.clrd);
    ResultScreen::ParsePscr(local_8, &g_GameManager.pscr[0][0][0]);
    ResultScreen::ParseCatk(local_8, g_GameManager.catk);
    ResultScreen::ReleaseScoreDat(local_8);
    if (g_GameManager.plst.gameHours < 7)
    {
        g_GameManager.maxRetries = 3;
    }
    else if (g_GameManager.plst.gameHours < 14)
    {
        g_GameManager.maxRetries = 4;
    }
    else
    {
        g_GameManager.maxRetries = 5;
    }
    if (!g_GameManager.phantasmUnlocked &&
        g_GameManager.HasUnlockedPhantomAndMaxClears())
    {
        frameCount = 0;
        // STRING: TH07 0x004954bc
        g_AnmManager->LoadSurface(0, "data/title/phantasm.jpg");
        while (frameCount < 900)
        {
            g_AnmManager->SetVertexShader(255);
            g_AnmManager->SetSprite(NULL);
            g_AnmManager->SetTexture(NULL);
            g_AnmManager->SetColorOp(255);
            g_AnmManager->SetBlendMode(255);
            g_AnmManager->SetZWriteDisable(255);
            g_AnmManager->ClearFrameState();
            g_AnmManager->SetCameraMode(255);
            g_AnmManager->SetColor(0x80808080);
            g_Supervisor.d3dDevice->BeginScene();
            g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
            if (frameCount < 60)
            {
                local_1c.left = 0.0f;
                local_1c.top = 0.0f;
                local_1c.right = 639.0f;
                local_1c.bottom = 479.0f;
                local_20.bytes.a = (60 - frameCount) * 255 / 60;
                local_20.bytes.r = local_20.bytes.g = local_20.bytes.b = 0;
                ScreenEffect::DrawSquare(&local_1c, local_20.color);
            }
            else if (frameCount > 840)
            {
                local_34.left = 0.0f;
                local_34.top = 0.0f;
                local_34.right = 639.0f;
                local_34.bottom = 479.0f;
                local_24.bytes.a = (frameCount - 840) * 255 / 60;
                local_24.bytes.r = local_24.bytes.g = local_24.bytes.b = 0;
                ScreenEffect::DrawSquare(&local_34, local_24.color);
            }
            g_CurFrameRawInput = Controller::GetInput();
            g_Supervisor.d3dDevice->EndScene();
            if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
            {
                g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
            }
            if (120 <= frameCount && frameCount < 840 &&
                WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU | TH_BUTTON_BOMB))
            {
                frameCount = 840;
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            }
            frameCount++;
            g_SoundPlayer.ProcessQueues();
        }
        g_AnmManager->ReleaseSurface(0);
    }
    g_GameManager.phantasmUnlocked = g_GameManager.HasUnlockedPhantomAndMaxClears();
    this->gameState = STATE_PRE_INPUT;
    InitializeTimingVars(&g_Supervisor);
    switch (g_Supervisor.prevState)
    {
    case 2:
    case 3:
    case 6:
        this->cursor = g_GameManager.difficulty >= 4;
        break;
    case 5:
        this->cursor = 4;
        break;
    case 8:
        this->cursor = 5;
        break;
    default:
        this->cursor = 0;
        break;
    }
    this->isPracticeMode = 0;
    if (g_GameManager.practice)
    {
        this->cursor = 2;
        this->isPracticeMode = 1;
    }
    g_GameManager.practice = 0;
    if (g_Supervisor.prevState)
    {
        GameManager::DrawLoadingSprite();
    }
    // STRING: TH07 0x004954a8
    if (g_AnmManager->LoadAnms(ANM_FILE_TITLE, "data/title01.anm", ANM_OFFSET_TITLE) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    if (!g_GameManager.demo)
    {
        if (g_Supervisor.prevState != 5)
        {
            g_Supervisor.LoadAudio(8, "bgm/th07_01.mid");
        }
        if (g_Supervisor.lastTotalPlayTimeUpdate == 0)
        {
            BombEffects::RegisterChain(0, 70, 0xffffff, 0, 0);
        }
        else
        {
            BombEffects::RegisterChain(0, 70, 0xffffff, 0, 0);
        }
    }
    for (i = 0; i < 14; i++)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(&this->vms[i], 1798);
        g_AnmManager->SetActiveSprite(&this->vms[i],
                                      this->vms[i].activeSpriteIdx + i);
    }
    this->cursorVm = this->vms;
    g_GameManager.demo = 0;
    g_GameManager.demoFrames = 0;
    return ZUN_SUCCESS;
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
    if (this->vmHead)
    {
        delete[] this->vmHead;
        this->vmHead = NULL;
        this->vmHead = NULL; // ZUN bloat: this is the same exact thing twice
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0045c546
ZunResult MainMenu::DeletedCallback(MainMenu *arg)
{
    g_Supervisor.d3dDevice->ResourceManagerDiscardBytes(0);
    for (i32 i = 32; i <= 41; i++)
    {
        g_AnmManager->ReleaseAnm(i);
    }
    g_AnmManager->ReleaseSurface(0);
    g_Chain.Cut(arg->drawChain);
    arg->drawChain = NULL;
    arg->Release();
    delete arg;
    arg = NULL;

    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0045c5d0
ZunResult MainMenu::RegisterChain(u32 param_1)
{
    MainMenu *mgr = new MainMenu;

    // ZUN bloat: memset it twice just to be nice
    memset(mgr, 0, sizeof(MainMenu));

    g_GameManager.isInPauseMenu = 0;
    mgr->calcChain = g_Chain.CreateElem((ChainCallback)OnUpdate);
    mgr->calcChain->arg = mgr;
    mgr->calcChain->addedCallback = (ChainLifecycleCallback)AddedCallback;
    mgr->calcChain->deletedCallback =
        (ChainLifecycleCallback)DeletedCallback;
    if (g_Chain.AddToCalcChain(mgr->calcChain, 3))
    {
        return ZUN_ERROR;
    }

    mgr->drawChain = g_Chain.CreateElem((ChainCallback)OnDraw);
    mgr->drawChain->arg = mgr;
    g_Chain.AddToDrawChain(mgr->drawChain, 0);

    UselessStack::EightBytes();
    return ZUN_SUCCESS;
}
