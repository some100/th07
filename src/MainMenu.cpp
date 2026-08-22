#include "MainMenu.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "AnmIdx.hpp"
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

namespace fs = std::filesystem;

const char *g_DemoReplayPaths[3] = {
    "data/demo/demorpy0.rpy",
    "data/demo/demorpy1.rpy",
    "data/demo/demorpy2.rpy",
};

const char *g_StagePracticeStrings[6] = {
    "Stage1", "Stage2", "Stage3", "Stage4", "Stage5", "Stage6",
};

const char *g_StageReplayStrings[7] = {
    "Stage1  ", "Stage2  ", "Stage3  ", "Stage4  ", "Stage5  ", "Stage6  ", "Extra   ",
};

const char *g_PhantasmReplayString = "Phantasm";

const char *g_DifficultyStrings[6] = {
    "Easy    ", "Normal  ", "Hard    ", "Lunatic ", "Extra   ", "Phantasm",
};

const char *g_CharacterAndShottypeReplayStrings[6] = {
    "ReimuA ", "ReimuB ", "MarisaA", "MarisaB", "SakuyaA", "SakuyaB",
};

i16 g_LastJoystickInput = 32;

const char *g_KeyConfigStrings[12] = {
    "ショット、決定ボタンを設定します",
    "ボム、キャンセルボタンを設定します",
    "低速移動ボタンを設定します",
    "メッセージスキップボタンを設定します",
    "ポーズボタンを設定します",
    "上移動ボタンを設定します",
    "下移動ボタンを設定します",
    "左移動ボタンを設定します",
    "右移動ボタンを設定します",
    "ショット押しっぱなしで低速移動になるようにします",
    "初期設定に戻します",
    "おおよそ終了します",
};

const char *g_OptionsStrings[9] = {
    "プレイヤーの初期数を変更します。（初期設定　３）",
    "画面の色数を変更します。３２ＢＩＴだと最も綺麗に表示されます。",
    "ＢＧＭの再生方法を変更します。（初期設定　ＷＡＶ）",
    "効果音を再生するか選択します",
    "ウィンドウかフルスクリーンか選択します",
    "弾が多い場面でわざと処理落ちさせます(スコア、リプレイ記録不可)",
    "全て初期設定にします",
    "パッド操作のボタン配置を変更します",
    "おいそれと終了します",
};

const char *g_MainMenuStrings[8] = {
    "ゲームを開始します",
    "エキストラステージを開始します",
    "ステージを選択し、練習を開始します",
    "リプレイを鑑賞できます",
    "過去のスコアやスペルカードの取得歴を見られます",
    "音楽を聴けます",
    "各種設定できます",
    "いろいろと終了します",
};

u32 MainMenu::OnUpdate(MainMenu *arg)
{
    u32 result;

    arg->UpdatePrev();

    switch (arg->menuState)
    {
    case MENU_STATE_PRE_INPUT:
        result = arg->OnUpdatePreInput();
        break;
    case MENU_STATE_SELECT_REPLAY:
        result = arg->OnUpdateSelectReplay();
        break;
    case MENU_STATE_OPTIONS:
        result = arg->OnUpdateOptionsMenu();
        break;
    case MENU_STATE_KEY_CONFIG:
        result = arg->OnUpdateKeyConfig();
        break;
    case MENU_STATE_NORMAL_SELECT_DIFFICULTY:
    case MENU_STATE_PRACTICE_SELECT_DIFFICULTY:
    case MENU_STATE_EXTRA_SELECT_DIFFICULTY:
        result = arg->OnUpdateSelectDifficulty();
        break;
    case MENU_STATE_NORMAL_SELECT_CHARACTER:
    case MENU_STATE_PRACTICE_SELECT_CHARACTER:
    case MENU_STATE_EXTRA_SELECT_CHARACTER:
        result = arg->OnUpdateSelectCharacter();
        break;
    case MENU_STATE_NORMAL_SELECT_SHOTTYPE:
    case MENU_STATE_PRACTICE_SELECT_SHOTTYPE:
    case MENU_STATE_EXTRA_SELECT_SHOTTYPE:
        result = arg->OnUpdateSelectShotType();
        break;
    case MENU_STATE_SELECT_PRACTICE_STAGE:
        result = arg->OnUpdateSelectPracticeStage();
    }
    g_AnmManager->ExecuteScripts(arg->vms, arg->vmCount);
    if (arg->curDescriptionVm)
    {
        g_AnmManager->ExecuteScript(arg->curDescriptionVm);
    }

    return result;
}

u32 MainMenu::OnUpdatePreInput()
{
    i32 i;

    switch (this->menuSubState)
    {
    case MENU_SUBSTATE_PREINPUT_INIT:
        if (this->prevMenuState == MENU_STATE_PRE_INPUT &&
            g_Supervisor.prevState != SUPERVISOR_STATE_RESULTSCREEN)
        {
            g_Supervisor.PlayLoadedAudio(8);
        }
        if ((this->prevMenuState == MENU_STATE_PRE_INPUT ||
             this->prevMenuState == MENU_STATE_NORMAL_SELECT_DIFFICULTY ||
             this->prevMenuState == MENU_STATE_SELECT_REPLAY ||
             (this->prevMenuState == MENU_STATE_PRACTICE_SELECT_DIFFICULTY ||
              this->prevMenuState == MENU_STATE_EXTRA_SELECT_DIFFICULTY)) &&
            g_AnmManager->LoadSurface(0, "data/title/title00.jpg") != ZUN_SUCCESS)
        {
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
        if (this->vmCount == 0)
        {
            this->vmCount = 164;
            this->vms = new AnmVm[this->vmCount];
            g_AnmManager->ExecuteVmsAnms(this->vms, ANM_OFFSET_TITLE, this->vmCount);
        }
        g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 2);
        for (i = 0; i < ARRAY_SIZE_SIGNED(g_MainMenuStrings); i++)
        {
            g_AnmManager->SetActiveSprite(&this->vms[i + 1], this->vms[i + 1].baseSpriteIdx + 1);
        }
        g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 1],
                                      (i32)this->vms[this->cursor + 1].baseSpriteIdx);
        this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
        this->inputDelayTimer = 0;
        this->selected = -1;
        this->menuSubState = MENU_SUBSTATE_PREINPUT_INPUT;
        this->demoFramesCount = 0;
        if (g_GameManager.replay)
        {
            this->prevMenuState = this->menuState;
            this->menuState = MENU_STATE_SELECT_REPLAY;
            this->inputDelayTimer = 0;
            this->stateTimer = 0;
            this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
            this->idleFrames = 0;
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 13);
            this->curDescriptionVm->SetInterrupt(2);
            g_GameManager.SetReplay(0);
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        if (this->isPracticeMode)
        {
            this->prevMenuState = this->menuState;
            this->menuState = MENU_STATE_PRACTICE_SELECT_DIFFICULTY;
            this->inputDelayTimer = 0;
            this->stateTimer = 0;
            this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
            this->idleFrames = 0;
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 5);
            this->curDescriptionVm->SetInterrupt(2);
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        for (i = 0; i < ARRAY_SIZE(g_MainMenuStrings); i++)
        {
            g_AnmManager->DrawStringFormat2(&this->descriptionVms[i], 0xfff0e0, 0x300000,
                                            g_MainMenuStrings[i]);
        }
    case MENU_SUBSTATE_PREINPUT_INPUT: {
        i = MoveCursorVertical(ARRAY_SIZE_SIGNED(g_MainMenuStrings));
        if (i != 0)
        {
            while (g_GameManager.HasReachedMaxClearsAllShotTypes() == 0 && this->cursor == 1)
            {
                this->cursor += i;
            }
            for (i = 0; i < ARRAY_SIZE_SIGNED(g_MainMenuStrings); i++)
            {
                g_AnmManager->SetActiveSprite(&this->vms[i + 1],
                                              this->vms[i + 1].baseSpriteIdx + 1);
            }
            g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 1],
                                          (i32)this->vms[this->cursor + 1].baseSpriteIdx);
        }
        this->demoFramesCount++;
        if (g_CurFrameRawInput != 0)
        {
            this->demoFramesCount = 0;
        }
        if (this->demoFramesCount > 900)
        {
            g_GameManager.demoIdx++;
            g_GameManager.demoIdx %= 3;
            SDL_strlcpy(g_GameManager.replayFilename, g_DemoReplayPaths[g_GameManager.demoIdx],
                        sizeof(g_GameManager.replayFilename));
            this->currentReplay =
                (ReplayFile *)FileSystem::OpenFile(g_GameManager.replayFilename, 0);
            this->currentReplay =
                ReplayManager::ValidateReplayData(this->currentReplay, g_LastFileSize);
            if (!this->currentReplay)
            {
                Supervisor::DebugPrint("error : Demo Play is not ready\n");
                this->demoFramesCount = 0;
            }
            else
            {
                g_GameManager.SetReplay(1);
                g_GameManager.demo = 1;
                g_GameManager.demoFrames = 0;
                g_GameManager.difficulty = this->currentReplay->data.difficulty;
                g_GameManager.character = this->currentReplay->data.shotType / 2;
                g_GameManager.shotType = this->currentReplay->data.shotType % 2;
                g_GameManager.shotTypeAndCharacter = this->currentReplay->data.shotType;
                i = 0;
                while (!this->currentReplay->stageReplayData[i])
                {
                    i++;
                }

                g_GameManager.currentStage = i;
                ReplayManager::FreeReplay(this->currentReplay);
                this->currentReplay = NULL;
                g_Supervisor.curState = SUPERVISOR_STATE_GAMEMANAGER;
                g_GameManager.replayStage = 0;
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            }
        }
        if (this->selected != this->cursor)
        {
            this->curDescriptionVm = &this->descriptionVms[this->cursor];
            this->curDescriptionVm->SetInterrupt(1);
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
            case MENU_CURSOR_PREINPUT_START:
                g_GameManager.practice = 0;
                this->cursor = g_Supervisor.cfg.defaultDifficulty;
                if (this->cursor >= 4)
                {
                    this->cursor = 2;
                }
                this->prevMenuState = this->menuState;
                this->menuState = MENU_STATE_NORMAL_SELECT_DIFFICULTY;
                this->inputDelayTimer = 0;
                this->stateTimer = 0;
                this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
                this->idleFrames = 0;
                g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 5);
                this->curDescriptionVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE;
            case MENU_CURSOR_PREINPUT_PRACTICE_START:
                g_GameManager.practice = 1;
                this->cursor = g_Supervisor.cfg.defaultDifficulty;
                if (this->cursor >= 4)
                {
                    this->cursor = 2;
                }
                this->prevMenuState = this->menuState;
                this->menuState = MENU_STATE_PRACTICE_SELECT_DIFFICULTY;
                this->inputDelayTimer = 0;
                this->stateTimer = 0;
                this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
                this->idleFrames = 0;
                g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 5);
                this->curDescriptionVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE;
            case MENU_CURSOR_PREINPUT_EXTRA_START:
                if (g_GameManager.HasReachedMaxClearsAllShotTypes())
                {
                    g_GameManager.practice = 0;
                    this->cursor = g_Supervisor.cfg.defaultDifficulty == 5;
                    this->prevMenuState = this->menuState;
                    this->menuState = MENU_STATE_EXTRA_SELECT_DIFFICULTY;
                    this->inputDelayTimer = 0;
                    this->stateTimer = 0;
                    this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
                    this->idleFrames = 0;
                    g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 5);
                    this->curDescriptionVm->SetInterrupt(2);
                    return CHAIN_CALLBACK_RESULT_CONTINUE;
                }
            case MENU_CURSOR_PREINPUT_REPLAY:
                g_GameManager.practice = 0;
                this->prevMenuState = this->menuState;
                this->menuState = MENU_STATE_SELECT_REPLAY;
                this->inputDelayTimer = 0;
                this->stateTimer = 0;
                this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
                this->idleFrames = 0;
                g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 13);
                this->curDescriptionVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE;
            case MENU_CURSOR_PREINPUT_MUSICROOM:
                g_Supervisor.curState = SUPERVISOR_STATE_MUSICROOM;
                this->curDescriptionVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            case MENU_CURSOR_PREINPUT_RESULTS:
                g_Supervisor.curState = SUPERVISOR_STATE_RESULTSCREEN;
                this->curDescriptionVm->SetInterrupt(2);
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            case MENU_CURSOR_PREINPUT_OPTIONS:
                this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
                this->cursor = 0;
                this->stateTimer = 0;
                this->inputDelayTimer = 0;
                this->menuSubState = 3;
                this->inputDelayTimer = 0;
                OnUpdateOptionsMenu();
                this->cursor = 0;
                break;
            case MENU_CURSOR_PREINPUT_EXIT:
                this->menuSubState = MENU_SUBSTATE_PREINPUT_EXIT;
                this->inputDelayTimer = 0;
                g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 1);
                if (g_Supervisor.cfg.musicMode == 2)
                {
                    g_Supervisor.midiOutput->PlayLoaded(30);
                }
                break;
            }
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 1],
                                          this->vms[this->cursor + 1].baseSpriteIdx + 1);
            this->cursor = 7;
            g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 1],
                                          (i32)this->vms[this->cursor + 1].baseSpriteIdx);
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            g_SoundPlayer.ProcessQueues();
        }
        break;
    }
    case MENU_SUBSTATE_PREINPUT_EXIT:
        if (this->inputDelayTimer >= 60)
        {
            delete[] this->vms;
            this->vms = NULL;
            this->vms = NULL;
            this->vmCount = 0;
            this->stateTimer = 0;
            g_Supervisor.curState = SUPERVISOR_STATE_EXIT;
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
        break;
    case MENU_SUBSTATE_PREINPUT_OPTIONS:
        if (this->inputDelayTimer >= 30)
        {
            this->prevMenuState = this->menuState;
            this->menuState = MENU_STATE_OPTIONS;
            this->inputDelayTimer = 0;
            this->stateTimer = 0;
            this->menuSubState = MENU_SUBSTATE_PREINPUT_INIT;
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

u32 MainMenu::OnUpdateOptionsMenu()
{
    i32 i;

    switch (this->menuSubState)
    {
    default:
        goto LAB_00456e08;
    case MENU_SUBSTATE_SELECT_INIT:
        if (this->stateTimer == 0)
        {
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 3);
            for (i = 0; i < ARRAY_SIZE_SIGNED(g_OptionsStrings); i++)
            {
                g_AnmManager->SetActiveSprite(&this->vms[i + 9],
                                              this->vms[i + 9].baseSpriteIdx + 1);
            }
            g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 9],
                                          (i32)this->vms[this->cursor + 9].baseSpriteIdx);
            this->menuSubState = MENU_SUBSTATE_SELECT_INIT;
            this->inputDelayTimer = 0;
            this->selected = -1;
        }
        this->menuSubState = MENU_SUBSTATE_SELECT_INPUT;
        for (i = 0; i < ARRAY_SIZE(g_OptionsStrings); i++)
        {
            g_AnmManager->DrawStringFormat2(&this->descriptionVms[i], 0xfff0e0, 0x300000,
                                            g_OptionsStrings[i]);
        }
    case MENU_SUBSTATE_SELECT_INPUT:
        break;
    }

    if (MoveCursorVertical(ARRAY_SIZE_SIGNED(g_OptionsStrings)))
    {
        for (i = 0; i < ARRAY_SIZE_SIGNED(g_OptionsStrings); i++)
        {
            g_AnmManager->SetActiveSprite(&this->vms[i + 9], this->vms[i + 9].baseSpriteIdx + 1);
        }
        g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 9],
                                      (i32)this->vms[this->cursor + 9].baseSpriteIdx);
    }

    if (this->selected != this->cursor)
    {
        this->curDescriptionVm = &this->descriptionVms[this->cursor];
        this->curDescriptionVm->SetInterrupt(1);
    }
    this->selected = this->cursor;

    for (i = 18; i <= 22; i++)
    {
        g_AnmManager->SetActiveSprite(&this->vms[i], this->vms[i].baseSpriteIdx + 1);
    }
    i = g_Supervisor.cfg.lifeCount + 18;
    g_AnmManager->SetActiveSprite(&this->vms[i], (i32)this->vms[i].baseSpriteIdx);

    for (i = 23; i <= 24; i++)
    {
        g_AnmManager->SetActiveSprite(&this->vms[i], this->vms[i].baseSpriteIdx + 1);
    }
    i = g_Supervisor.cfg.colorMode16bit + 23;
    g_AnmManager->SetActiveSprite(&this->vms[i], (i32)this->vms[i].baseSpriteIdx);

    for (i = 25; i <= 27; i++)
    {
        g_AnmManager->SetActiveSprite(&this->vms[i], this->vms[i].baseSpriteIdx + 1);
    }
    i = g_Supervisor.cfg.musicMode + 25;
    g_AnmManager->SetActiveSprite(&this->vms[i], (i32)this->vms[i].baseSpriteIdx);

    for (i = 28; i <= 29; i++)
    {
        g_AnmManager->SetActiveSprite(&this->vms[i], this->vms[i].baseSpriteIdx + 1);
    }
    i = g_Supervisor.cfg.playSounds + 28;
    g_AnmManager->SetActiveSprite(&this->vms[i], (i32)this->vms[i].baseSpriteIdx);

    for (i = 30; i <= 31; i++)
    {
        g_AnmManager->SetActiveSprite(&this->vms[i], this->vms[i].baseSpriteIdx + 1);
    }
    i = g_Supervisor.cfg.windowed + 30;
    g_AnmManager->SetActiveSprite(&this->vms[i], (i32)this->vms[i].baseSpriteIdx);

    for (i = 32; i <= 33; i++)
    {
        g_AnmManager->SetActiveSprite(&this->vms[i], this->vms[i].baseSpriteIdx + 1);
    }
    i = g_Supervisor.cfg.slowMode + 32;
    g_AnmManager->SetActiveSprite(&this->vms[i], (i32)this->vms[i].baseSpriteIdx);

    if (this->stateTimer < 4)
    {
        goto LAB_00456e08;
    }

    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_LEFT))
    {
        switch (this->cursor)
        {
        case MENU_CURSOR_OPTIONS_MENU_LIVES:
            if (g_Supervisor.cfg.lifeCount == 0)
            {
                g_Supervisor.cfg.lifeCount = 4;
            }
            else
            {
                g_Supervisor.cfg.lifeCount--;
            }
            break;
        case MENU_CURSOR_OPTIONS_COLOR_MODE:
            if (!g_Supervisor.cfg.colorMode16bit)
            {
                g_Supervisor.cfg.colorMode16bit = 1;
            }
            else
            {
                g_Supervisor.cfg.colorMode16bit--;
            }
            break;
        case MENU_CURSOR_OPTIONS_MUSIC_MODE:
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
            if (!g_Supervisor.cfg.preloadBgm && g_Supervisor.cfg.musicMode == MUSIC_MIDI)
            {
                g_SoundPlayer.StartBGM("thbgm.dat");
            }
            g_Supervisor.LoadAudio(8, "bgm/th07_01.mid");
            g_Supervisor.PlayLoadedAudio(8);
            break;
        case MENU_CURSOR_OPTIONS_PLAY_SFX:
            if (!g_Supervisor.cfg.playSounds)
            {
                g_Supervisor.cfg.playSounds = 1;
            }
            else
            {
                g_Supervisor.cfg.playSounds--;
            }
            break;
        case MENU_CURSOR_OPTIONS_WINDOW_MODE:
            if (!g_Supervisor.cfg.windowed)
            {
                g_Supervisor.cfg.windowed = 1;
            }
            else
            {
                g_Supervisor.cfg.windowed--;
            }
            break;
        case MENU_CURSOR_OPTIONS_SLOW_MODE:
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
        case MENU_CURSOR_OPTIONS_MENU_LIVES:
            if (g_Supervisor.cfg.lifeCount >= 4)
            {
                g_Supervisor.cfg.lifeCount = 0;
            }
            else
            {
                g_Supervisor.cfg.lifeCount++;
            }
            break;
        case MENU_CURSOR_OPTIONS_COLOR_MODE:
            if (g_Supervisor.cfg.colorMode16bit >= 1)
            {
                g_Supervisor.cfg.colorMode16bit = 0;
            }
            else
            {
                g_Supervisor.cfg.colorMode16bit++;
            }
            break;
        case MENU_CURSOR_OPTIONS_MUSIC_MODE:
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
        case MENU_CURSOR_OPTIONS_PLAY_SFX:
            if (g_Supervisor.cfg.playSounds >= 1)
            {
                g_Supervisor.cfg.playSounds = 0;
            }
            else
            {
                g_Supervisor.cfg.playSounds++;
            }
            break;
        case MENU_CURSOR_OPTIONS_WINDOW_MODE:
            if (g_Supervisor.cfg.windowed >= 1)
            {
                g_Supervisor.cfg.windowed = 0;
            }
            else
            {
                g_Supervisor.cfg.windowed++;
            }
            break;
        case MENU_CURSOR_OPTIONS_SLOW_MODE:
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
        goto RETURN_TO_PREINPUT;
    }

    if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
    {
        switch (this->cursor)
        {
        case MENU_CURSOR_OPTIONS_RESET:
            g_Supervisor.cfg.lifeCount = 2;
            g_Supervisor.cfg.bombCount = 3;
            g_Supervisor.cfg.musicMode = MUSIC_WAV;
            g_Supervisor.cfg.playSounds = 1;
            g_Supervisor.cfg.slowMode = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            g_SoundPlayer.ProcessQueues();
            break;
        case MENU_CURSOR_OPTIONS_KEY_CONFIG:
            this->cursor = 0;
            SetMenuState(MENU_STATE_KEY_CONFIG);
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            g_SoundPlayer.ProcessQueues();
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        case MENU_CURSOR_OPTIONS_EXIT:
        RETURN_TO_PREINPUT:
            this->cursor = MENU_CURSOR_PREINPUT_OPTIONS;
            SetMenuState(MENU_STATE_PRE_INPUT);
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
        if (this->cursor == MENU_CURSOR_OPTIONS_EXIT)
        {
            goto RETURN_TO_PREINPUT;
        }
        g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 9],
                                      this->vms[this->cursor + 9].baseSpriteIdx + 1);
        this->cursor = MENU_CURSOR_OPTIONS_EXIT;
        g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 9],
                                      (i32)this->vms[this->cursor + 9].baseSpriteIdx);
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
        g_SoundPlayer.ProcessQueues();
    }

LAB_00456e08:
    this->idleFrames++;
    this->inputDelayTimer++;
    this->stateTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

void MainMenu::SwapMapping(i16 btnPressed, i16 oldMapping)
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

u32 MainMenu::OnUpdateKeyConfig()
{
    AnmVm *vm;
    i32 i;
    i16 btnPressed;
    u8 *controllerState;
    AnmVm *cursorVmTmp;

    switch (this->menuSubState)
    {
    case MENU_SUBSTATE_SELECT_INIT:
        if (this->stateTimer == 0)
        {
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 4);
            for (i = 0; i < ARRAY_SIZE_SIGNED(g_KeyConfigStrings); i++)
            {
                g_AnmManager->SetActiveSprite(&this->vms[i + 35],
                                              this->vms[i + 35].baseSpriteIdx + 1);
            }
            g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 35],
                                          (i32)this->vms[this->cursor + 35].baseSpriteIdx);
            this->menuSubState = MENU_SUBSTATE_SELECT_INIT;
            this->inputDelayTimer = 0;
            this->controlMapping = g_Supervisor.cfg.controllerMapping;
            g_Supervisor.cfg.controllerMapping.upButton = -1;
            g_Supervisor.cfg.controllerMapping.downButton = -1;

            vm = &this->vms[47];
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
        this->menuSubState = MENU_SUBSTATE_SELECT_INPUT;
        for (i = 0; i < ARRAY_SIZE(g_KeyConfigStrings); i++)
        {
            g_AnmManager->DrawStringFormat2(&this->descriptionVms[i], 0xfff0e0, 0x300000,
                                            g_KeyConfigStrings[i]);
        }
    case MENU_SUBSTATE_SELECT_INPUT:
        if (MoveCursorVertical(ARRAY_SIZE_SIGNED(g_KeyConfigStrings)))
        {
            for (i = 0; i < ARRAY_SIZE_SIGNED(g_KeyConfigStrings); i++)
            {
                g_AnmManager->SetActiveSprite(&this->vms[i + 35],
                                              this->vms[i + 35].baseSpriteIdx + 1);
            }
            g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 35],
                                          (i32)this->vms[this->cursor + 35].baseSpriteIdx);
        }
        if (this->selected != this->cursor)
        {
            this->curDescriptionVm = &this->descriptionVms[this->cursor];
            // this should be using SetInterrupt?
            cursorVmTmp = this->curDescriptionVm;
            cursorVmTmp->pendingInterrupt = 1;
        }
        this->selected = this->cursor;

        vm = &this->vms[47];
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
            g_AnmManager->SetActiveSprite(&this->vms[i], this->vms[i].baseSpriteIdx + 1);
        }
        i = g_Supervisor.cfg.shotSlow + 65;
        g_AnmManager->SetActiveSprite(&this->vms[i], (i32)this->vms[i].baseSpriteIdx);

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
            case MENU_CURSOR_KEYCONFIG_SHOOT:
                SwapMapping(btnPressed, this->controlMapping.shootButton);
                this->controlMapping.shootButton = btnPressed;
                break;
            case MENU_CURSOR_KEYCONFIG_BOMB:
                SwapMapping(btnPressed, this->controlMapping.bombButton);
                this->controlMapping.bombButton = btnPressed;
                break;
            case MENU_CURSOR_KEYCONFIG_FOCUS:
                SwapMapping(btnPressed, this->controlMapping.focusButton);
                this->controlMapping.focusButton = btnPressed;
                break;
            case MENU_CURSOR_KEYCONFIG_MENU:
                SwapMapping(btnPressed, this->controlMapping.menuButton);
                this->controlMapping.menuButton = btnPressed;
                break;
            case MENU_CURSOR_KEYCONFIG_UP:
                SwapMapping(btnPressed, this->controlMapping.upButton);
                this->controlMapping.upButton = btnPressed;
                break;
            case MENU_CURSOR_KEYCONFIG_DOWN:
                SwapMapping(btnPressed, this->controlMapping.downButton);
                this->controlMapping.downButton = btnPressed;
                break;
            case MENU_CURSOR_KEYCONFIG_LEFT:
                SwapMapping(btnPressed, this->controlMapping.leftButton);
                this->controlMapping.leftButton = btnPressed;
                break;
            case MENU_CURSOR_KEYCONFIG_RIGHT:
                SwapMapping(btnPressed, this->controlMapping.rightButton);
                this->controlMapping.rightButton = btnPressed;
                break;
            case MENU_CURSOR_KEYCONFIG_SKIP:
                SwapMapping(btnPressed, this->controlMapping.skipButton);
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
            case MENU_CURSOR_KEYCONFIG_SHOTSLOW:
                g_Supervisor.cfg.shotSlow = 1 - g_Supervisor.cfg.shotSlow;
            }
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RIGHT))
        {
            switch (this->cursor)
            {
            case MENU_CURSOR_KEYCONFIG_SHOTSLOW:
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
            case MENU_CURSOR_KEYCONFIG_RESET:
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
                g_SoundPlayer.ProcessQueues();
                this->controlMapping = g_ControllerMapping;
                g_Supervisor.cfg.shotSlow = 1;
                break;
            case MENU_CURSOR_KEYCONFIG_EXIT:
            exit_config:
                g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
                g_SoundPlayer.ProcessQueues();
                SetMenuState(MENU_STATE_OPTIONS);
                g_Supervisor.cfg.controllerMapping = this->controlMapping;
                this->cursor = MENU_CURSOR_OPTIONS_KEY_CONFIG;
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

ZunResult MainMenu::UpdateMenuDigits(AnmVm *vm, i16 number)
{
    if (number < 0)
    {
        vm->active = 0;
        vm[1].active = 0;
    }
    else
    {
        g_AnmManager->SetActiveSprite(vm, (i32)vm->baseSpriteIdx + (i32)number / 10 * 2);
        g_AnmManager->SetActiveSprite(vm + 1, (i32)vm[1].baseSpriteIdx + (i32)number % 10 * 2);
        vm->active = 1;
        vm[1].active = 1;
    }
    return ZUN_SUCCESS;
}

u32 MainMenu::OnUpdateSelectDifficulty()
{
    i32 oldMenuState;
    i32 numDifficulties;
    i32 i;

    switch (this->menuSubState)
    {
    case MENU_SUBSTATE_SELECT_INIT:
        if (this->stateTimer == 0)
        {
            if (this->prevMenuState != MENU_STATE_NORMAL_SELECT_CHARACTER &&
                this->prevMenuState != MENU_STATE_PRACTICE_SELECT_CHARACTER &&
                this->prevMenuState != MENU_STATE_EXTRA_SELECT_CHARACTER &&
                g_AnmManager->LoadSurface(0, "data/title/select00.jpg") != ZUN_SUCCESS)
            {
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            }
            this->cursor = g_Supervisor.cfg.defaultDifficulty;
            if (this->menuState != MENU_STATE_EXTRA_SELECT_DIFFICULTY)
            {
                g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 7);
            }
            else if (!g_GameManager.HasUnlockedPhantomAndMaxClears())
            {
                g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 12);
                this->cursor = MENU_CURSOR_SELECTDIFFICULTY_EXTRA;
            }
            else
            {
                g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 22);
            }
            if (this->menuState != MENU_STATE_EXTRA_SELECT_DIFFICULTY)
            {
                if (this->cursor >= MENU_CURSOR_SELECTDIFFICULTY_EXTRA)
                {
                    this->cursor = MENU_CURSOR_SELECTDIFFICULTY_NORMAL;
                }
                for (i = 0; i < 4; i++)
                {
                    g_AnmManager->SetActiveSprite(&this->vms[i + 67],
                                                  this->vms[i + 67].baseSpriteIdx + 1);
                }
                g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 67],
                                              (i32)this->vms[this->cursor + 67].baseSpriteIdx);
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
                    g_AnmManager->SetActiveSprite(&this->vms[i + 162],
                                                  this->vms[i + 162].baseSpriteIdx + 1);
                }
                g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 162],
                                              (i32)this->vms[this->cursor + 162].baseSpriteIdx);
            }
            this->menuSubState = MENU_SUBSTATE_SELECT_INIT;
            this->inputDelayTimer = 0;
            this->curDescriptionVm = NULL;
        }
        if (this->isPracticeMode)
        {
            SetMenuState(MENU_STATE_PRACTICE_SELECT_CHARACTER);
            this->cursor = 0;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        if (this->stateTimer == 30)
        {
            this->menuSubState = MENU_SUBSTATE_SELECT_INPUT;
        }
        break;
    case MENU_SUBSTATE_SELECT_INPUT:
        numDifficulties = this->menuState != MENU_STATE_EXTRA_SELECT_DIFFICULTY ? 4
                          : g_GameManager.HasUnlockedPhantomAndMaxClears()      ? 2
                                                                                : 1;
        if (MoveCursorVertical(numDifficulties))
        {
            if (this->menuState != MENU_STATE_EXTRA_SELECT_DIFFICULTY)
            {
                for (i = 0; i < 4; i++)
                {
                    g_AnmManager->SetActiveSprite(&this->vms[i + 67],
                                                  this->vms[i + 67].baseSpriteIdx + 1);
                }
                g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 67],
                                              (i32)this->vms[this->cursor + 67].baseSpriteIdx);
            }
            else if (numDifficulties == 2)
            {
                for (i = 0; i < 2; i++)
                {
                    g_AnmManager->SetActiveSprite(&this->vms[i + 162],
                                                  this->vms[i + 162].baseSpriteIdx + 1);
                }
                g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 162],
                                              (i32)this->vms[this->cursor + 162].baseSpriteIdx);
            }
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            if (this->menuState != MENU_STATE_EXTRA_SELECT_DIFFICULTY)
            {
                g_Supervisor.cfg.defaultDifficulty = this->cursor;
            }
            else
            {
                g_Supervisor.cfg.defaultDifficulty = this->cursor + 4;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            g_SoundPlayer.ProcessQueues();
            if (this->menuState != MENU_STATE_EXTRA_SELECT_DIFFICULTY)
            {
                if (!g_GameManager.practice)
                {
                    SetMenuState(MENU_STATE_NORMAL_SELECT_CHARACTER);
                }
                else
                {
                    SetMenuState(MENU_STATE_PRACTICE_SELECT_CHARACTER);
                }
            }
            else
            {
                SetMenuState(MENU_STATE_EXTRA_SELECT_CHARACTER);
            }

            this->cursor = 0;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            if (this->menuState != MENU_STATE_EXTRA_SELECT_DIFFICULTY)
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
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 6);
        }
        break;
    case 3:
        if (this->inputDelayTimer >= 30)
        {
            oldMenuState = this->menuState;
            SetMenuState(MENU_STATE_PRE_INPUT);
            if (oldMenuState != MENU_STATE_EXTRA_SELECT_DIFFICULTY)
            {
                if (!g_GameManager.practice)
                {
                    this->cursor = MENU_CURSOR_PREINPUT_START;
                }
                else
                {
                    this->cursor = MENU_CURSOR_PREINPUT_PRACTICE_START;
                }
            }
            else
            {
                this->cursor = MENU_CURSOR_PREINPUT_EXTRA_START;
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

u32 MainMenu::OnUpdateSelectCharacter()
{
    switch (this->menuSubState)
    {
    case MENU_SUBSTATE_SELECT_INIT:
        if (this->stateTimer == 0)
        {
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 8);
            if (g_Supervisor.cfg.defaultDifficulty < DIFF_EXTRA)
            {
                this->vms[g_Supervisor.cfg.defaultDifficulty + 67].SetInterrupt(9);
            }
            else
            {
                if (!g_GameManager.HasUnlockedPhantomAndMaxClears())
                {
                    this->vms[161].SetInterrupt(9);
                }
                else
                {
                    this->vms[g_Supervisor.cfg.defaultDifficulty + 158].SetInterrupt(9);
                }
            }
            this->cursor = g_GameManager.character;
            if (g_Supervisor.cfg.defaultDifficulty == DIFF_EXTRA)
            {
                while (!g_GameManager.HasReachedMaxClears(this->cursor * 2) &&
                       !g_GameManager.HasReachedMaxClears(this->cursor * 2 + 1))
                {
                    this->cursor++;
                    if (this->cursor >= MENU_CURSOR_SELECTCHARACTER_COUNT)
                    {
                        this->cursor -= MENU_CURSOR_SELECTCHARACTER_COUNT;
                    }
                }
            }
            else if (g_Supervisor.cfg.defaultDifficulty == DIFF_PHANTASM)
            {
                while (g_GameManager.HasUnlockedPhantom(this->cursor << 1) == 0 &&
                       g_GameManager.HasUnlockedPhantom(this->cursor * 2 + 1) == 0)
                {
                    this->cursor++;
                    if (this->cursor >= MENU_CURSOR_SELECTCHARACTER_COUNT)
                    {
                        this->cursor -= MENU_CURSOR_SELECTCHARACTER_COUNT;
                    }
                }
            }
            this->vms[72].active = 0;
            this->vms[73].active = 0;
            this->vms[71].active = 0;
            this->vms[80].active = 0;
            this->vms[83].active = 0;
            this->vms[75].active = 0;
            this->vms[76].active = 0;
            this->vms[74].active = 0;
            this->vms[81].active = 0;
            this->vms[84].active = 0;
            this->vms[78].active = 0;
            this->vms[79].active = 0;
            this->vms[77].active = 0;
            this->vms[82].active = 0;
            this->vms[85].active = 0;
            switch (this->cursor)
            {
            case MENU_CURSOR_SELECTCHARACTER_REIMU:
                this->vms[72].active = 1;
                this->vms[73].active = 1;
                this->vms[71].active = 1;
                this->vms[80].active = 1;
                this->vms[83].active = 1;
                break;
            case MENU_CURSOR_SELECTCHARACTER_MARISA:
                this->vms[75].active = 1;
                this->vms[76].active = 1;
                this->vms[74].active = 1;
                this->vms[81].active = 1;
                this->vms[84].active = 1;
                break;
            case MENU_CURSOR_SELECTCHARACTER_SAKUYA:
                this->vms[78].active = 1;
                this->vms[79].active = 1;
                this->vms[77].active = 1;
                this->vms[82].active = 1;
                this->vms[85].active = 1;
                break;
            }
            switch (this->cursor)
            {
            case MENU_CURSOR_SELECTCHARACTER_REIMU:
                this->vms[71].SetInterrupt(9);
                this->vms[74].SetInterrupt(8);
                this->vms[77].SetInterrupt(8);
                this->vms[74].color.bytes.a = 0;
                this->vms[77].color.bytes.a = 0;
                this->vms[80].SetInterrupt(9);
                this->vms[81].SetInterrupt(8);
                this->vms[82].SetInterrupt(8);
                this->vms[81].color.bytes.a = 0;
                this->vms[82].color.bytes.a = 0;
                this->vms[83].SetInterrupt(9);
                this->vms[84].SetInterrupt(8);
                this->vms[85].SetInterrupt(8);
                this->vms[84].color.bytes.a = 0;
                this->vms[85].color.bytes.a = 0;
                break;
            case MENU_CURSOR_SELECTCHARACTER_MARISA:
                this->vms[71].SetInterrupt(8);
                this->vms[74].SetInterrupt(9);
                this->vms[77].SetInterrupt(8);
                this->vms[71].color.bytes.a = 0;
                this->vms[77].color.bytes.a = 0;
                this->vms[80].SetInterrupt(8);
                this->vms[81].SetInterrupt(9);
                this->vms[82].SetInterrupt(8);
                this->vms[80].color.bytes.a = 0;
                this->vms[82].color.bytes.a = 0;
                this->vms[83].SetInterrupt(8);
                this->vms[84].SetInterrupt(9);
                this->vms[85].SetInterrupt(8);
                this->vms[83].color.bytes.a = 0;
                this->vms[85].color.bytes.a = 0;
                break;
            case MENU_CURSOR_SELECTCHARACTER_SAKUYA:
                this->vms[71].SetInterrupt(8);
                this->vms[74].SetInterrupt(8);
                this->vms[77].SetInterrupt(9);
                this->vms[74].color.bytes.a = 0;
                this->vms[71].color.bytes.a = 0;
                this->vms[80].SetInterrupt(8);
                this->vms[81].SetInterrupt(8);
                this->vms[82].SetInterrupt(9);
                this->vms[80].color.bytes.a = 0;
                this->vms[81].color.bytes.a = 0;
                this->vms[83].SetInterrupt(8);
                this->vms[84].SetInterrupt(8);
                this->vms[85].SetInterrupt(9);
                this->vms[83].color.bytes.a = 0;
                this->vms[84].color.bytes.a = 0;
                break;
            }
            this->menuSubState = MENU_SUBSTATE_SELECT_INIT;
            this->inputDelayTimer = 0;
        }
        if (this->isPracticeMode)
        {
            SetMenuState(MENU_STATE_PRACTICE_SELECT_SHOTTYPE);
            this->cursor = 0;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        if (this->stateTimer == 30)
        {
            this->menuSubState = MENU_SUBSTATE_SELECT_INPUT;
        }
        break;
    case MENU_SUBSTATE_SELECT_INPUT:
        if (MoveCursorHorizontal(3) != ZUN_SUCCESS)
        {
            if (g_Supervisor.cfg.defaultDifficulty == DIFF_EXTRA)
            {
                while (!g_GameManager.HasReachedMaxClears(this->cursor * 2) &&
                       !g_GameManager.HasReachedMaxClears(this->cursor * 2 + 1))
                {
                    this->cursor++;
                    if (this->cursor >= MENU_CURSOR_SELECTCHARACTER_COUNT)
                    {
                        this->cursor -= MENU_CURSOR_SELECTCHARACTER_COUNT;
                    }
                }
            }
            else if (g_Supervisor.cfg.defaultDifficulty == 5)
            {
                while (g_GameManager.HasUnlockedPhantom(this->cursor << 1) == 0 &&
                       g_GameManager.HasUnlockedPhantom(this->cursor * 2 + 1) == 0)
                {
                    this->cursor++;
                    if (this->cursor >= MENU_CURSOR_SELECTCHARACTER_COUNT)
                    {
                        this->cursor -= MENU_CURSOR_SELECTCHARACTER_COUNT;
                    }
                }
            }
            this->vms[72].active = 1;
            this->vms[73].active = 1;
            this->vms[71].active = 1;
            this->vms[80].active = 1;
            this->vms[83].active = 1;
            this->vms[75].active = 1;
            this->vms[76].active = 1;
            this->vms[74].active = 1;
            this->vms[81].active = 1;
            this->vms[84].active = 1;
            this->vms[78].active = 1;
            this->vms[79].active = 1;
            this->vms[77].active = 1;
            this->vms[82].active = 1;
            this->vms[85].active = 1;
            switch (this->cursor)
            {
            case MENU_CURSOR_SELECTCHARACTER_REIMU:
                this->vms[71].SetInterrupt(9);
                this->vms[74].SetInterrupt(8);
                this->vms[77].SetInterrupt(8);
                this->vms[80].SetInterrupt(9);
                this->vms[81].SetInterrupt(8);
                this->vms[82].SetInterrupt(8);
                this->vms[83].SetInterrupt(9);
                this->vms[84].SetInterrupt(8);
                this->vms[85].SetInterrupt(8);
                break;
            case MENU_CURSOR_SELECTCHARACTER_MARISA:
                this->vms[71].SetInterrupt(8);
                this->vms[74].SetInterrupt(9);
                this->vms[77].SetInterrupt(8);
                this->vms[80].SetInterrupt(8);
                this->vms[81].SetInterrupt(9);
                this->vms[82].SetInterrupt(8);
                this->vms[83].SetInterrupt(8);
                this->vms[84].SetInterrupt(9);
                this->vms[85].SetInterrupt(8);
                break;
            case MENU_CURSOR_SELECTCHARACTER_SAKUYA:
                this->vms[71].SetInterrupt(8);
                this->vms[74].SetInterrupt(8);
                this->vms[77].SetInterrupt(9);
                this->vms[80].SetInterrupt(8);
                this->vms[81].SetInterrupt(8);
                this->vms[82].SetInterrupt(9);
                this->vms[83].SetInterrupt(8);
                this->vms[84].SetInterrupt(8);
                this->vms[85].SetInterrupt(9);
                break;
            }
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            g_GameManager.character = this->cursor;
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            g_SoundPlayer.ProcessQueues();
            if (this->menuState != MENU_STATE_EXTRA_SELECT_CHARACTER)
            {
                if (!g_GameManager.practice)
                {
                    SetMenuState(MENU_STATE_NORMAL_SELECT_SHOTTYPE);
                }
                else
                {
                    SetMenuState(MENU_STATE_PRACTICE_SELECT_SHOTTYPE);
                }
            }
            else
            {
                SetMenuState(MENU_STATE_EXTRA_SELECT_SHOTTYPE);
            }
            this->cursor = 0;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            g_SoundPlayer.ProcessQueues();
            g_GameManager.character = this->cursor;
            if (this->menuState != MENU_STATE_EXTRA_SELECT_CHARACTER)
            {
                if (!g_GameManager.practice)
                {
                    SetMenuState(MENU_STATE_NORMAL_SELECT_DIFFICULTY);
                }
                else
                {
                    SetMenuState(MENU_STATE_PRACTICE_SELECT_DIFFICULTY);
                }
            }
            else
            {
                SetMenuState(MENU_STATE_EXTRA_SELECT_DIFFICULTY);
            }
            this->cursor = MENU_CURSOR_SELECTDIFFICULTY_EASY;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        break;
    }
    this->idleFrames++;
    this->inputDelayTimer++;
    this->stateTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

u32 MainMenu::OnUpdateSelectShotType()
{
    switch (this->menuSubState)
    {
    case MENU_SUBSTATE_SELECT_INIT:
        if (this->stateTimer == 0)
        {
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 10);
            if (g_Supervisor.cfg.defaultDifficulty < DIFF_EXTRA)
            {
                this->vms[g_Supervisor.cfg.defaultDifficulty + 67].SetInterrupt(9);
            }
            else
            {
                if (!g_GameManager.HasUnlockedPhantomAndMaxClears())
                {
                    this->vms[161].SetInterrupt(9);
                }
                else
                {
                    this->vms[g_Supervisor.cfg.defaultDifficulty + 158].SetInterrupt(9);
                }
            }
            this->vms[72].active = 0;
            this->vms[73].active = 0;
            this->vms[71].active = 0;
            this->vms[80].active = 0;
            this->vms[83].active = 0;
            this->vms[75].active = 0;
            this->vms[76].active = 0;
            this->vms[74].active = 0;
            this->vms[81].active = 0;
            this->vms[84].active = 0;
            this->vms[78].active = 0;
            this->vms[79].active = 0;
            this->vms[77].active = 0;
            this->vms[82].active = 0;
            this->vms[85].active = 0;
            this->cursor = g_GameManager.shotType;
            if (g_Supervisor.cfg.defaultDifficulty == DIFF_EXTRA)
            {
                while (!g_GameManager.HasReachedMaxClears(this->cursor +
                                                          (u32)g_GameManager.character * 2))
                {
                    this->cursor++;
                    if (this->cursor >= MENU_CURSOR_SELECTSHOTTYPE_COUNT)
                    {
                        this->cursor -= MENU_CURSOR_SELECTSHOTTYPE_COUNT;
                    }
                }
            }
            else if (g_Supervisor.cfg.defaultDifficulty == DIFF_PHANTASM)
            {
                while (!g_GameManager.HasUnlockedPhantom(this->cursor +
                                                         (u32)g_GameManager.character * 2))
                {
                    this->cursor++;
                    if (this->cursor >= MENU_CURSOR_SELECTSHOTTYPE_COUNT)
                    {
                        this->cursor -= MENU_CURSOR_SELECTSHOTTYPE_COUNT;
                    }
                }
            }
            switch (g_GameManager.character)
            {
            case CHAR_REIMU:
                this->vms[72].active = 1;
                this->vms[73].active = 1;
                this->vms[71].active = 1;
                g_AnmManager->SetActiveSprite(&this->vms[73 - this->cursor],
                                              this->vms[73 - this->cursor].baseSpriteIdx + 1);
                g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 72],
                                              (i32)this->vms[this->cursor + 72].baseSpriteIdx);
                break;
            case CHAR_MARISA:
                this->vms[75].active = 1;
                this->vms[76].active = 1;
                this->vms[74].active = 1;
                g_AnmManager->SetActiveSprite(&this->vms[76 - this->cursor],
                                              this->vms[76 - this->cursor].baseSpriteIdx + 1);
                g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 75],
                                              (i32)this->vms[this->cursor + 75].baseSpriteIdx);
                break;
            case CHAR_SAKUYA:
                this->vms[78].active = 1;
                this->vms[79].active = 1;
                this->vms[77].active = 1;
                g_AnmManager->SetActiveSprite(&this->vms[79 - this->cursor],
                                              this->vms[79 - this->cursor].baseSpriteIdx + 1);
                g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 78],
                                              (i32)this->vms[this->cursor + 78].baseSpriteIdx);
                break;
            }
            this->menuSubState = MENU_SUBSTATE_SELECT_INIT;
            this->inputDelayTimer = 0;
        }
        if (this->isPracticeMode)
        {
            SetMenuState(MENU_STATE_SELECT_PRACTICE_STAGE);
            this->isPracticeMode = 0;
            this->cursor = g_GameManager.currentStage - 1;
            return CHAIN_CALLBACK_RESULT_CONTINUE;
        }
        if (this->stateTimer == 30)
        {
            this->menuSubState = MENU_SUBSTATE_SELECT_INPUT;
        }
        break;
    case MENU_SUBSTATE_SELECT_INPUT:
        if (MoveCursorVertical(MENU_CURSOR_SELECTSHOTTYPE_COUNT))
        {
            if (g_Supervisor.cfg.defaultDifficulty == DIFF_EXTRA)
            {
                while (!g_GameManager.HasReachedMaxClears(this->cursor +
                                                          (u32)g_GameManager.character * 2))
                {
                    this->cursor++;
                    if (this->cursor >= MENU_CURSOR_SELECTSHOTTYPE_COUNT)
                    {
                        this->cursor -= MENU_CURSOR_SELECTSHOTTYPE_COUNT;
                    }
                }
            }
            else if (g_Supervisor.cfg.defaultDifficulty == DIFF_PHANTASM)
            {
                while (g_GameManager.HasUnlockedPhantom(this->cursor +
                                                        (u32)g_GameManager.character * 2) == 0)
                {
                    this->cursor++;
                    if (this->cursor >= MENU_CURSOR_SELECTSHOTTYPE_COUNT)
                    {
                        this->cursor -= MENU_CURSOR_SELECTSHOTTYPE_COUNT;
                    }
                }
            }
            switch (g_GameManager.character)
            {
            case CHAR_REIMU:
                g_AnmManager->SetActiveSprite(&this->vms[73 - this->cursor],
                                              this->vms[73 - this->cursor].baseSpriteIdx + 1);
                g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 72],
                                              (i32)this->vms[this->cursor + 72].baseSpriteIdx);
                break;
            case CHAR_MARISA:
                g_AnmManager->SetActiveSprite(&this->vms[76 - this->cursor],
                                              this->vms[76 - this->cursor].baseSpriteIdx + 1);
                g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 75],
                                              (i32)this->vms[this->cursor + 75].baseSpriteIdx);
                break;
            case CHAR_SAKUYA:
                g_AnmManager->SetActiveSprite(&this->vms[79 - this->cursor],
                                              this->vms[79 - this->cursor].baseSpriteIdx + 1);
                g_AnmManager->SetActiveSprite(&this->vms[this->cursor + 78],
                                              (i32)this->vms[this->cursor + 78].baseSpriteIdx);
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
                    g_GameManager.currentStage = g_GameManager.difficulty + 2;
                }
                g_Supervisor.curState = SUPERVISOR_STATE_GAMEMANAGER;
                g_GameManager.SetReplay(0);
                g_Supervisor.StopAudio();
                while (g_SoundPlayer.ProcessQueues())
                    ;
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            }
            this->cursor = 0;
            SetMenuState(MENU_STATE_SELECT_PRACTICE_STAGE);
            return CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            g_GameManager.shotType = this->cursor;
            if (this->menuState != MENU_STATE_EXTRA_SELECT_SHOTTYPE)
            {
                if (!g_GameManager.practice)
                {
                    SetMenuState(MENU_STATE_NORMAL_SELECT_CHARACTER);
                }
                else
                {
                    SetMenuState(MENU_STATE_PRACTICE_SELECT_CHARACTER);
                }
            }
            else
            {
                SetMenuState(MENU_STATE_EXTRA_SELECT_CHARACTER);
            }
            this->vms[72].active = 1;
            this->vms[73].active = 1;
            this->vms[71].active = 1;
            this->vms[80].active = 1;
            this->vms[83].active = 1;
            this->vms[75].active = 1;
            this->vms[76].active = 1;
            this->vms[74].active = 1;
            this->vms[81].active = 1;
            this->vms[84].active = 1;
            this->vms[78].active = 1;
            this->vms[79].active = 1;
            this->vms[77].active = 1;
            this->vms[82].active = 1;
            this->vms[85].active = 1;
            return CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN;
        }
        break;
    }
    this->idleFrames++;
    this->inputDelayTimer++;
    this->stateTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

u32 MainMenu::OnUpdateSelectPracticeStage()
{
    i32 allowedStages;

    switch (this->menuSubState)
    {
    case MENU_SUBSTATE_SELECT_INIT:
        if (this->stateTimer == 0)
        {
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 18);
            this->vms[72].active = 0;
            this->vms[73].active = 0;
            this->vms[71].active = 0;
            this->vms[80].active = 0;
            this->vms[83].active = 0;
            this->vms[75].active = 0;
            this->vms[76].active = 0;
            this->vms[74].active = 0;
            this->vms[81].active = 0;
            this->vms[84].active = 0;
            this->vms[78].active = 0;
            this->vms[79].active = 0;
            this->vms[77].active = 0;
            this->vms[82].active = 0;
            this->vms[85].active = 0;
            switch (g_GameManager.character)
            {
            case CHAR_REIMU:
                this->vms[72].active = 1;
                this->vms[73].active = 1;
                this->vms[71].active = 1;
                break;
            case CHAR_MARISA:
                this->vms[75].active = 1;
                this->vms[76].active = 1;
                this->vms[74].active = 1;
                break;
            case CHAR_SAKUYA:
                this->vms[78].active = 1;
                this->vms[79].active = 1;
                this->vms[77].active = 1;
                break;
            }
            this->menuSubState = MENU_SUBSTATE_SELECT_INIT;
            this->inputDelayTimer = 0;
            g_GameManager.practice = 1;
        }
        if (this->stateTimer == 30)
        {
            this->menuSubState = MENU_SUBSTATE_SELECT_INPUT;
        }
        break;
    case MENU_SUBSTATE_SELECT_INPUT:
        allowedStages = g_GameManager.clrd[g_GameManager.character * 2 + g_GameManager.shotType]
                            .difficultyClearedWithoutRetries[g_Supervisor.cfg.defaultDifficulty];
        if (allowedStages < 0)
        {
            allowedStages = 1;
        }
        else if (allowedStages >= 99)
        {
            allowedStages = 6;
        }
        if (this->cursor >= allowedStages)
        {
            this->cursor = 0;
        }
        MoveCursorVertical(allowedStages);
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            g_GameManager.difficulty = g_Supervisor.cfg.defaultDifficulty;
            g_GameManager.currentStage = this->cursor;
            g_Supervisor.curState = SUPERVISOR_STATE_GAMEMANAGER;

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
            SetMenuState(MENU_STATE_NORMAL_SELECT_SHOTTYPE);
            this->vms[72].active = 1;
            this->vms[73].active = 1;
            this->vms[71].active = 1;
            this->vms[80].active = 1;
            this->vms[83].active = 1;
            this->vms[75].active = 1;
            this->vms[76].active = 1;
            this->vms[74].active = 1;
            this->vms[81].active = 1;
            this->vms[84].active = 1;
            this->vms[78].active = 1;
            this->vms[79].active = 1;
            this->vms[77].active = 1;
            this->vms[82].active = 1;
            this->vms[85].active = 1;
            return CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN;
        }
        break;
    }
    this->idleFrames++;
    this->inputDelayTimer++;
    this->stateTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

bool ReplayFileMatches(const std::string &name)
{
    return name.size() == 14 && name.compare(0, 6, "th7_ud") == 0 &&
           name.compare(10, 4, ".rpy") == 0;
}

u32 MainMenu::OnUpdateSelectReplay()
{
    ReplayFile *file;
    i32 local_10;
    i32 i;

    switch (this->menuSubState)
    {
    case MENU_SUBSTATE_SELECT_INIT:
        if (this->stateTimer == 0)
        {
            if (this->prevMenuState != MENU_STATE_SELECT_REPLAY &&
                g_AnmManager->LoadSurface(0, "data/title/select00.jpg") != ZUN_SUCCESS)
            {
                return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
            }
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 14);
            this->cursor = 0;
            this->menuSubState = MENU_SUBSTATE_SELECT_INIT;
            this->inputDelayTimer = 0;
            this->curDescriptionVm = NULL;
            local_10 = 0;
            for (i = 0; i < 15; i++)
            {
                char filename[32];
                snprintf(filename, sizeof(filename), "th7_%.2d.rpy", i + 1);
                std::string replayPath = fs::path(FileSystem::GetPrefPath("replay")) / filename;
                file = (ReplayFile *)FileSystem::OpenFile(replayPath.c_str(), 1);
                if (!file)
                {
                    continue;
                }

                file = ReplayManager::ValidateReplayData(file, g_LastFileSize);
                if (file)
                {
                    this->replays[local_10] = *file;
                    SDL_strlcpy(this->replayFilenames[local_10], replayPath.c_str(),
                                sizeof(this->replayFilenames[local_10]));
                    sprintf(this->replayLabels[local_10], "No.%.2d", i + 1);
                    local_10++;
                    ReplayManager::FreeReplay(file);
                }
            }

            const fs::path replay = FileSystem::GetPrefPath("replay");
            fs::create_directory(replay);

            std::vector<fs::directory_entry> entries(fs::directory_iterator(replay),
                                                     fs::directory_iterator{});
            std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
                return a.path().filename() < b.path().filename();
            });
            for (const auto &entry : entries)
            {
                const std::string filename = entry.path().filename().string();
                if (!ReplayFileMatches(filename))
                {
                    continue;
                }
                if (local_10 >= 45)
                {
                    break;
                }
                file = (ReplayFile *)FileSystem::OpenFile(
                    (FileSystem::GetPrefPath("replay") + "/" + filename).c_str(), 1);
                if (!file)
                {
                    continue;
                }
                file = ReplayManager::ValidateReplayData(file, g_LastFileSize);
                if (file)
                {
                    this->replays[local_10] = *file;
                    SDL_strlcpy(this->replayFilenames[local_10],
                                (FileSystem::GetPrefPath("replay") + "/" + filename).c_str(),
                                sizeof(this->replayFilenames[local_10]));
                    sprintf(this->replayLabels[local_10], "User ");
                    ReplayManager::FreeReplay(file);
                    local_10++;
                }
            }
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
        if (this->replayFilesNum > 15)
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
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 15);
            this->vms[this->chosenReplay % 15 + 135].SetInterrupt(17);
            this->currentReplay =
                (ReplayFile *)FileSystem::OpenFile(this->replayFilenames[this->chosenReplay], 1);
            this->currentReplay =
                ReplayManager::ValidateReplayData(this->currentReplay, g_LastFileSize);
            this->cursor = 0;
            while (!this->currentReplay->head.stageReplayDataOffsets[this->cursor])
            {
                this->cursor++;
                if (this->cursor >= 7)
                {
                    g_GameErrorContext.Fatal("リプレイデータが異常\n");
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
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 16);
        }
        break;
    case 2:
        i = MoveCursorVertical(7);
        if (i < 0)
        {
            while (!this->replays[this->chosenReplay].head.stageReplayDataOffsets[this->cursor])
            {
                this->cursor--;
                if (this->cursor < 0)
                {
                    this->cursor = 6;
                }
            }
        }
        else if (i > 0)
        {
            while (!this->replays[this->chosenReplay].head.stageReplayDataOffsets[this->cursor])
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
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 19);
            this->vms[this->chosenReplay % 15 + 135].SetInterrupt(17);
            this->menuSubState = 3;
            this->cursor = 0;
            this->vms[158].pendingInterrupt = 21;
            this->vms[159].pendingInterrupt = 21;
            this->vms[160].pendingInterrupt = 21;
            this->vms[this->cursor + 158].pendingInterrupt = 20;
            break;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            ReplayManager::FreeReplay(this->currentReplay);
            this->currentReplay = NULL;
            this->menuSubState = 1;
            this->stateTimer = 0;
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 14);
            this->cursor = this->chosenReplay;
            break;
        }
        break;
    case 3:
        i = MoveCursorVertical(3);
        if (i != 0)
        {
            this->vms[158].pendingInterrupt = 21;
            this->vms[159].pendingInterrupt = 21;
            this->vms[160].pendingInterrupt = 21;
            this->vms[this->cursor + 158].pendingInterrupt = 20;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            g_GameManager.SetReplay(1);
            SDL_strlcpy(g_GameManager.replayFilename, this->replayFilenames[this->chosenReplay],
                        sizeof(g_GameManager.replayFilename));
            g_GameManager.difficulty = this->currentReplay->data.difficulty;
            g_GameManager.character = this->currentReplay->data.shotType / 2;
            g_GameManager.shotType = this->currentReplay->data.shotType % 2;
            g_GameManager.shotTypeAndCharacter = this->currentReplay->data.shotType;
            ReplayManager::FreeReplay(this->currentReplay);
            this->currentReplay = NULL;
            g_GameManager.currentStage =
                g_GameManager.difficulty >= DIFF_PHANTASM ? 7 : this->selectedStage;
            g_Supervisor.curState = SUPERVISOR_STATE_GAMEMANAGER;
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
            g_AnmManager->SetInterruptActiveVms(this->vms, this->vmCount, 15);
            this->vms[this->chosenReplay % 15 + 135].SetInterrupt(17);
            break;
        }
        break;
    case 4:
        if (this->inputDelayTimer >= 30)
        {
            SetMenuState(MENU_STATE_PRE_INPUT);
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

i32 MainMenu::DrawReplayMenu()
{
    i32 replayAmount;
    i32 i;
    AnmVm *vm;

    vm = &this->vms[134];
    AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos, "No.   Name       Date  Player   Rank");
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
            &g_AsciiManager, &vm->pos, "%s %8s  %6s %7s  %8s", this->replayLabels + replayAmount,
            this->replays[replayAmount].data.name, this->replays[replayAmount].data.date,
            g_CharacterAndShottypeReplayStrings[this->replays[replayAmount].data.shotType],
            g_DifficultyStrings[this->replays[replayAmount].data.difficulty]);
    }
    if ((this->menuSubState == 2 || this->menuSubState == 3) && this->currentReplay != NULL)
    {
        g_AsciiManager.color = 0xffffffff;
        g_AsciiManager.isSelected = 0;
        vm = &this->vms[133];
        AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos, "       %2.3f%%",
                                    (f64)this->currentReplay->data.slowdownRate);
        vm = &this->vms[150];
        AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos, "Stage    LastScore");
        replayAmount = this->chosenReplay - this->chosenReplay % 15;
        for (i = 0; i < ARRAY_SIZE_SIGNED(g_StageReplayStrings); i++, replayAmount++)
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
            if (this->currentReplay->stageReplayData[i])
            {
                if (i < 6 || this->currentReplay->data.difficulty <= 4)
                {
                    AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos, "%s %9d0",
                                                g_StageReplayStrings[i],
                                                this->currentReplay->stageReplayData[i]->score);
                }
                else
                {
                    AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos, "%s %9d0",
                                                g_PhantasmReplayString,
                                                this->currentReplay->stageReplayData[i]->score);
                }
            }
            else
            {
                if (i < 6 || this->currentReplay->data.difficulty <= 4)
                {
                    AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos, "%s ----------",
                                                g_StageReplayStrings[i]);
                }
                else
                {
                    AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos, "%s ----------",
                                                g_PhantasmReplayString);
                }
            }
        }
    }
    g_AsciiManager.color = 0xffffffff;
    g_AsciiManager.isSelected = 0;
    return 1;
}

i32 MainMenu::DrawPracticeMenu()
{
    ZunVec3 local_1c;
    i32 local_10;
    i32 i;
    AnmVm *vm;

    g_AsciiManager.color = 0xffffffff;
    g_AsciiManager.isSelected = 0;
    vm = &this->vms[131];
    AsciiManager::AddFormatText(&g_AsciiManager, &vm->pos, "Stage    HI-Score");
    local_1c = vm->pos;
    local_1c.y += 16.0f;
    local_10 = g_GameManager.clrd[g_GameManager.character * 2 + g_GameManager.shotType]
                   .difficultyClearedWithoutRetries[g_Supervisor.cfg.defaultDifficulty];

    for (i = 0; i < ARRAY_SIZE_SIGNED(g_StagePracticeStrings); i++)
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
        AsciiManager::AddFormatText(&g_AsciiManager, &local_1c, "%s %9d0 (%3d)",
                                    g_StagePracticeStrings[i],
                                    g_GameManager
                                        .pscr[g_GameManager.character * 2 + g_GameManager.shotType]
                                             [i][g_Supervisor.cfg.defaultDifficulty]
                                        .score,
                                    g_GameManager
                                        .pscr[g_GameManager.character * 2 + g_GameManager.shotType]
                                             [i][g_Supervisor.cfg.defaultDifficulty]
                                        .playCount);
        local_1c.y += 16.0f;
    }
    g_AsciiManager.color = 0xffffffff;
    g_AsciiManager.isSelected = 0;
    return 1;
}

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

u32 MainMenu::OnDraw(MainMenu *arg)
{
    ZunVec3 savedPos;
    AnmVm *local_c;
    i32 i;

    g_AnmManager->SetTexture(0);
    g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
    switch (arg->menuState)
    {
    case MENU_STATE_SELECT_REPLAY:
        arg->DrawReplayMenu();
        break;
    case MENU_STATE_SELECT_PRACTICE_STAGE:
        arg->DrawPracticeMenu();
        break;
    }
    local_c = arg->vms;
    for (i = 0; i < arg->vmCount; i++, local_c++)
    {
        if (g_AnmManager->ShouldDraw(local_c))
        {
            savedPos = local_c->pos;
            ZunVec3 drawPos = local_c->prevPos.Lerp(local_c->pos, g_RenderAlpha);
            local_c->pos = drawPos + local_c->offset;
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
    if (arg->curDescriptionVm)
    {
        g_AnmManager->DrawInterpNoRotation(arg->curDescriptionVm);
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

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
    local_8 = ResultScreen::OpenScore(FileSystem::GetPrefPath("score.dat").c_str());
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
    if (!g_GameManager.phantasmUnlocked && g_GameManager.HasUnlockedPhantomAndMaxClears())
    {
        frameCount = 0;
        g_AnmManager->LoadSurface(0, "data/title/phantasm.jpg");
        while (frameCount < 900)
        {
            g_AnmManager->SetVertexShader(255);
            g_AnmManager->SetSprite(NULL);
            g_AnmManager->SetTexture(0);
            g_AnmManager->SetColorOp(255);
            g_AnmManager->SetBlendMode(255);
            g_AnmManager->SetZWriteDisable(255);
            g_AnmManager->ClearFrameState();
            g_AnmManager->SetCameraMode(255);
            g_AnmManager->SetColor(0x80808080);
            g_Supervisor.gfxDevice->BeginFrame();
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
            g_Supervisor.gfxDevice->EndFrame();
            g_Supervisor.gfxDevice->SwapBuffers();
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
    this->menuState = MENU_STATE_PRE_INPUT;
    InitializeTimingVars(&g_Supervisor);
    switch (g_Supervisor.prevState)
    {
    case SUPERVISOR_STATE_GAMEMANAGER:
    case SUPERVISOR_STATE_NEXT_STAGE:
    case SUPERVISOR_STATE_RESULTSCREEN_FROM_GAME:
        this->cursor = g_GameManager.difficulty >= 4;
        break;
    case SUPERVISOR_STATE_RESULTSCREEN:
        this->cursor = MENU_CURSOR_PREINPUT_RESULTS;
        break;
    case SUPERVISOR_STATE_MUSICROOM:
        this->cursor = MENU_CURSOR_PREINPUT_MUSICROOM;
        break;
    default:
        this->cursor = MENU_CURSOR_PREINPUT_START;
        break;
    }
    this->isPracticeMode = 0;
    if (g_GameManager.practice)
    {
        this->cursor = MENU_CURSOR_PREINPUT_PRACTICE_START;
        this->isPracticeMode = 1;
    }
    g_GameManager.practice = 0;
    if (g_Supervisor.prevState != SUPERVISOR_STATE_INIT)
    {
        GameManager::DrawLoadingSprite();
    }
    if (g_AnmManager->LoadAnms(ANM_FILE_TITLE, "data/title01.anm", ANM_OFFSET_TITLE) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    if (!g_GameManager.demo)
    {
        if (g_Supervisor.prevState != SUPERVISOR_STATE_RESULTSCREEN)
        {
            g_Supervisor.LoadAudio(8, "bgm/th07_01.mid");
        }
        if (g_Supervisor.lastTotalPlayTimeUpdate == 0)
        {
            ScreenEffect::RegisterChain(SCREEN_EFFECT_FADE_OUT, 70, 0xffffff, 0, 0);
        }
        else
        {
            ScreenEffect::RegisterChain(SCREEN_EFFECT_FADE_OUT, 70, 0xffffff, 0, 0);
        }
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->descriptionVms); i++)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(&this->descriptionVms[i],
                                                ANM_SCRIPT_TEXT_MAINMENU_OPTION_DESC);
        g_AnmManager->SetActiveSprite(&this->descriptionVms[i],
                                      this->descriptionVms[i].activeSpriteIdx + i);
    }
    this->curDescriptionVm = this->descriptionVms;
    g_GameManager.demo = 0;
    g_GameManager.demoFrames = 0;
    return ZUN_SUCCESS;
}

ZunResult MainMenu::AddedCallback(MainMenu *arg)
{
    return arg->ActualAddedCallback();
}

ZunResult MainMenu::Release()
{
    SAFE_FREE(this->currentReplay);
    if (this->vms)
    {
        delete[] this->vms;
        this->vms = NULL;
    }
    return ZUN_SUCCESS;
}

ZunResult MainMenu::DeletedCallback(MainMenu *arg)
{
    for (i32 i = ANM_FILE_TITLE_0; i <= ANM_FILE_TITLE_9; i++)
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

ZunResult MainMenu::RegisterChain()
{
    MainMenu *mgr = new MainMenu;

    g_GameManager.isInPauseMenu = 0;
    mgr->calcChain = g_Chain.CreateElem((ChainCallback)OnUpdate);
    mgr->calcChain->arg = mgr;
    mgr->calcChain->addedCallback = (ChainLifecycleCallback)AddedCallback;
    mgr->calcChain->deletedCallback = (ChainLifecycleCallback)DeletedCallback;
    if (g_Chain.AddToCalcChain(mgr->calcChain, 3))
    {
        return ZUN_ERROR;
    }

    mgr->drawChain = g_Chain.CreateElem((ChainCallback)OnDraw);
    mgr->drawChain->arg = mgr;
    g_Chain.AddToDrawChain(mgr->drawChain, 0);

    return ZUN_SUCCESS;
}
