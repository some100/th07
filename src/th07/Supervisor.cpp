#include "Supervisor.hpp"

#include <dinput.h>
#include <stdio.h>

#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "Ending.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameManager.hpp"
#include "MainMenu.hpp"
#include "MidiOutput.hpp"
#include "MusicRoom.hpp"
#include "ResultScreen.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "TextHelper.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"
#include "i18n.hpp"
#include "pbg4/Pbg4Archive.hpp"

// GLOBAL: TH07 0x0049ee40
ControllerMapping g_ControllerMapping = {0, 1, 2, 4, -1, -1, -1, -1, 3};

// GLOBAL: TH07 0x004b9e4c
u16 g_CurFrameRawInput;

// GLOBAL: TH07 0x004b9e50
u16 g_CurFrameGameInput;

// GLOBAL: TH07 0x004b9e54
u16 g_LastFrameRawInput;

// GLOBAL: TH07 0x004b9e58
u16 g_LastFrameGameInput;

// GLOBAL: TH07 0x004b9e5c
u16 g_IsEighthFrameOfHeldInput;

// GLOBAL: TH07 0x004b9e60
u16 g_NumOfFramesInputsWereHeld;

// GLOBAL: TH07 0x00575950
Supervisor g_Supervisor;

// GLOBAL: TH07 0x0135dfec
u32 g_FpsUpdateCounter;

// GLOBAL: TH07 0x0135dff0
char g_ReplayFpsBuffer[256];

// GLOBAL: TH07 0x0135e0f0
char g_FpsCounterBuffer[256];

// GLOBAL: TH07 0x0135e1f0
u32 g_NumFramesSinceLastTime;

// GLOBAL: TH07 0x0135e298
LARGE_INTEGER g_PerformanceCounter;

// FUNCTION: TH07 0x00437903
void Supervisor::DebugPrint2(const char *fmt, ...)
{
}

// FUNCTION: TH07 0x00437908
void Supervisor::CheckTiming()
{
    f64 timeDiff;
    f64 perfDiff;

    if (!this->checkTiming)
    {
        return;
    }

    QueryPerformanceCounter(&this->curPerfCounter);
    GetLocalTime(&this->curTime);

    timeDiff = (f64)this->curTime.wDay * 24.0 * 60.0 * 60.0 +
               (f64)(this->curTime.wHour * 60 * 60) +
               (f64)(this->curTime.wMinute * 60) +
               (f64)this->curTime.wSecond;

    perfDiff = (f64)this->prevTime.wDay * 24.0 * 60.0 * 60.0 +
               (f64)(this->prevTime.wHour * 60 * 60) +
               (f64)(this->prevTime.wMinute * 60) +
               (f64)this->prevTime.wSecond;

    if (timeDiff < perfDiff)
    {
        timeDiff = (f64)(this->prevTime.wDay + 1) * 24.0 * 60.0 * 60.0 +
                   (f64)(this->curTime.wHour * 60 * 60) +
                   (f64)(this->curTime.wMinute * 60) +
                   (f64)this->curTime.wSecond;
    }

    timeDiff -= perfDiff;
    timeDiff = timeDiff * 1000.0 + (f64)this->curTime.wMilliseconds -
               (f64)this->prevTime.wMilliseconds;
    timeDiff /= 1000.0;

    perfDiff = (f64)(this->curPerfCounter.LowPart - this->prevPerfCounter.LowPart) /
               (f64)this->perfFrequency.LowPart;

    if (perfDiff >= 1.0)
    {
        if (timeDiff / perfDiff > 2.5)
        {
            this->timingErrorCount++;
            if (this->maxTimingError < this->timingErrorCount)
            {
                this->maxTimingError = this->timingErrorCount;
            }
            if (this->timingSpikeAccumulator < this->timingErrorCount)
            {
                this->timingSpikeAccumulator = this->timingErrorCount;
            }
            if (this->timingSpikeAccumulator >= 10)
            {
                this->timingBadCount++;
                this->timingSpikeAccumulator = 0;
            }
            Supervisor::DebugPrint2(TH_LOG_ALQ_CHECK, timeDiff, perfDiff,
                                    timeDiff / perfDiff);
        }
        else if (this->timingErrorCount != 0)
        {
            this->timingErrorCount--;
        }
        this->checkTiming = FALSE;
    }

    if (this->maxTimingError >= 40 || this->timingBadCount >= 16)
    {
        this->timingBad = 1;
    }
    else
    {
        this->timingBad = 0;
    }
}

// FUNCTION: TH07 0x00437c39
void AnmManager::ReleaseVertexBuffer()
{
    SAFE_RELEASE(this->vertexBuffer);
}

// FUNCTION: TH07 0x00437c70
u32 Supervisor::OnUpdate(Supervisor *arg)
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
    g_AnmManager->offset.y = 0.0f;
    g_AnmManager->offset.x = 0.0f;
    g_Supervisor.fogEnabled = 255;
    if (g_SoundPlayer.backgroundMusic)
    {
        g_SoundPlayer.backgroundMusic->UpdateFadeOut();
    }
    if (!g_GameManager.slowModeSlowActive)
    {
        g_LastFrameRawInput = g_CurFrameRawInput;
        g_CurFrameRawInput = Controller::GetInput();
        g_IsEighthFrameOfHeldInput = 0;
        if (g_LastFrameRawInput == g_CurFrameRawInput)
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
    }
    else
    {
        g_CurFrameRawInput |= Controller::GetInput();
    }
    if (arg->wantedState != arg->curState)
    {
        arg->prevState = arg->wantedState;
        // STRING: TH07 0x00497230
        Supervisor::DebugPrint2("scene %d -> %d\r\n", arg->wantedState,
                                arg->curState);
        switch (arg->wantedState)
        {
        case SUPERVISOR_STATE_INIT:
        CASE_SUPERVISOR_STATE_INIT:
            arg->curState = SUPERVISOR_STATE_MAINMENU;
            g_Supervisor.d3dDevice->ResourceManagerDiscardBytes(0);
            if (MainMenu::RegisterChain(0) != ZUN_SUCCESS)
            {
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            }
            break;
        case SUPERVISOR_STATE_MAINMENU:
            switch (arg->curState)
            {
            case SUPERVISOR_STATE_EXIT:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case SUPERVISOR_STATE_GAMEMANAGER:
                if (GameManager::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            case SUPERVISOR_STATE_EXIT_ERROR:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR;
            case SUPERVISOR_STATE_RESULTSCREEN:
                if (ResultScreen::RegisterChain(0) != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            case SUPERVISOR_STATE_MUSICROOM:
                if (MusicRoom::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            case SUPERVISOR_STATE_ENDING:
                GameManager::CutChain();
                if (Ending::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            }
            break;
        case SUPERVISOR_STATE_RESULTSCREEN:
            switch (arg->curState)
            {
            case SUPERVISOR_STATE_EXIT:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case SUPERVISOR_STATE_MAINMENU:
                arg->curState = 0;
                goto CASE_SUPERVISOR_STATE_INIT;
            }
            break;
        case SUPERVISOR_STATE_GAMEMANAGER:
            switch (arg->curState)
            {
            case SUPERVISOR_STATE_EXIT:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case SUPERVISOR_STATE_MAINMENU:
                GameManager::CutChain();
                arg->curState = SUPERVISOR_STATE_INIT;
                ReplayManager::SaveReplay(NULL, NULL);
                goto CASE_SUPERVISOR_STATE_INIT;
                break;
            case SUPERVISOR_STATE_RESULTSCREEN_FROM_GAME:
                GameManager::CutChain();
                if (ResultScreen::RegisterChain(1) != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            case SUPERVISOR_STATE_RESTART_FROM_BEGINNING:
                GameManager::CutChain();
                if (!g_GameManager.practice && g_GameManager.difficulty < 4)
                {
                    g_GameManager.currentStage = DUMMYSTAGE;
                }
                else
                {
                    g_GameManager.currentStage--;
                }
                ReplayManager::SaveReplay(NULL, NULL);
                if (GameManager::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                arg->curState = SUPERVISOR_STATE_GAMEMANAGER;
                break;
            case SUPERVISOR_STATE_RESTART_STAGE:
                g_Supervisor.curState = SUPERVISOR_STATE_NEXT_STAGE;
                GameManager::CutChain();
                g_GameManager.currentStage--;
                if (GameManager::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                arg->curState = SUPERVISOR_STATE_GAMEMANAGER;
                break;
            case SUPERVISOR_STATE_NEXT_STAGE_USELESS:
                // ZUN bloat: The idea was likely to start the next stage
                // with all stats reset to initial values, but the curState
                // assignment literally right after makes it the exact same as
                // SUPERVISOR_STATE_NEXT_STAGE for all intents and purposes.
                g_Supervisor.curState = SUPERVISOR_STATE_NEXT_STAGE;
                GameManager::CutChain();
                if (GameManager::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                arg->curState = SUPERVISOR_STATE_GAMEMANAGER;
                break;
            case SUPERVISOR_STATE_NEXT_STAGE:
                GameManager::CutChain();
                if (GameManager::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                arg->curState = SUPERVISOR_STATE_GAMEMANAGER;
                break;
            case SUPERVISOR_STATE_REPLAY_END:
                GameManager::CutChain();
                arg->curState = SUPERVISOR_STATE_INIT;
                ReplayManager::SaveReplay(NULL, NULL);
                arg->curState = SUPERVISOR_STATE_MAINMENU;
                g_Supervisor.d3dDevice->ResourceManagerDiscardBytes(0);
                if (MainMenu::RegisterChain(1) != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            case SUPERVISOR_STATE_ENDING:
                g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty]
                    .noContinueClearCount =
                    g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty]
                        .noContinueClearCount +
                    1;
                GameManager::CutChain();
                if (Ending::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            }
            break;
        case SUPERVISOR_STATE_RESULTSCREEN_FROM_GAME:
            switch (arg->curState)
            {
            case SUPERVISOR_STATE_EXIT:
                ReplayManager::SaveReplay(NULL, NULL);
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case SUPERVISOR_STATE_MAINMENU:
                arg->curState = SUPERVISOR_STATE_INIT;
                ReplayManager::SaveReplay(NULL, NULL);
                goto CASE_SUPERVISOR_STATE_INIT;
            }
            break;
        case SUPERVISOR_STATE_MUSICROOM:
            switch (arg->curState)
            {
            case SUPERVISOR_STATE_EXIT:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case SUPERVISOR_STATE_MAINMENU:
                arg->curState = SUPERVISOR_STATE_INIT;
                goto CASE_SUPERVISOR_STATE_INIT;
            }
            break;
        case SUPERVISOR_STATE_ENDING:
            switch (arg->curState)
            {
            case SUPERVISOR_STATE_EXIT:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case SUPERVISOR_STATE_MAINMENU:
                arg->curState = SUPERVISOR_STATE_INIT;
                goto CASE_SUPERVISOR_STATE_INIT;
            case SUPERVISOR_STATE_RESULTSCREEN_FROM_GAME:
                if (ResultScreen::RegisterChain(1) != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            }
            break;
        }
        g_CurFrameRawInput = g_LastFrameRawInput =
            g_IsEighthFrameOfHeldInput = 0;
    }
    arg->wantedState = arg->curState;
    arg->calcCount = arg->calcCount + 1;
    if (arg->calcCount % 4000 == 3999 &&
        g_Supervisor.CheckIntegrity("0100b", g_Supervisor.exeSize,
                                    g_Supervisor.exeChecksum) != ZUN_SUCCESS)
    {
        return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
    }
    else
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
}

// FUNCTION: TH07 0x0043831b
u32 Supervisor::OnDraw(Supervisor *arg)
{
    DrawFpsCounter(1);
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0043832f
i32 __stdcall Supervisor::EnumGameControllersCb(LPCDIDEVICEINSTANCEA param_1,
                                                void *param_2)
{
    HRESULT hr;
    if (!g_Supervisor.controller)
    {
        hr = g_Supervisor.directInput->CreateDevice(param_1->guidInstance,
                                                    &g_Supervisor.controller, NULL);
        if (FAILED(hr))
        {
            return 1;
        }
    }
    return 0;
}

#pragma var_order(dipr, idk)
// FUNCTION: TH07 0x0043836e
i32 __stdcall Supervisor::ControllerCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi,
                                             LPVOID pvRef)
{
    DIPROPRANGE dipr;
    void *idk = pvRef;

    if (lpddoi->dwType & DIDFT_AXIS)
    {
        dipr.diph.dwSize = sizeof(DIPROPRANGE);
        dipr.diph.dwHeaderSize = 16;
        dipr.diph.dwHow = 2;
        dipr.diph.dwObj = lpddoi->dwType;
        dipr.lMin = -1000;
        dipr.lMax = 1000;
        if (g_Supervisor.controller->SetProperty(DIPROP_RANGE, &dipr.diph) <
            0)
        {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: TH07 0x004383d8
ZunResult Supervisor::SetupDInput()
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongA(this->hwndGameWindow, -6);
    if (this->cfg.disableDinput)
    {
        return ZUN_ERROR;
    }

    if (FAILED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8A,
                                  (LPVOID *)&this->directInput, NULL)))
    {
        this->directInput = NULL;
        g_GameErrorContext.Log(TH_ERR_DINPUT_INIT_FAIL);
        return ZUN_ERROR;
    }
    else
    {
        if (FAILED(this->directInput->CreateDevice(GUID_SysKeyboard,
                                                   &this->keyboard, NULL)))
        {
            SAFE_RELEASE(this->directInput);
            g_GameErrorContext.Log(TH_ERR_DINPUT_INIT_FAIL);
            return ZUN_ERROR;
        }
        else
        {
            if (FAILED(this->keyboard->SetDataFormat(&c_dfDIKeyboard)))
            {
                SAFE_RELEASE(this->keyboard);
                SAFE_RELEASE(this->directInput);
                // STRING: TH07 0x004971d8
                g_GameErrorContext.Log(TH_ERR_DINPUT_SETDATAFORMAT_FAIL);
                return ZUN_ERROR;
            }
            else
            {
                if (FAILED(this->keyboard->SetCooperativeLevel(
                        this->hwndGameWindow,
                        DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY)))
                {
                    SAFE_RELEASE(this->keyboard);
                    SAFE_RELEASE(this->directInput);
                    g_GameErrorContext.Log(TH_ERR_DINPUT_SETCOOPERATIVELEVEL_FAIL);
                    return ZUN_ERROR;
                }
                else
                {
                    this->keyboard->Acquire();
                    g_GameErrorContext.Log(TH_LOG_DINPUT_INIT_SUCCESS);
                    this->directInput->EnumDevices(4, EnumGameControllersCb, NULL, 1);
                    if (this->controller)
                    {
                        this->controller->SetDataFormat(&c_dfDIJoystick2);
                        this->controller->SetCooperativeLevel(this->hwndGameWindow, 10);
                        g_Supervisor.controllerCaps.dwSize = sizeof(DIDEVCAPS);
                        this->controller->GetCapabilities(&g_Supervisor.controllerCaps);
                        this->controller->EnumObjects(ControllerCallback, NULL, 0);
                        g_GameErrorContext.Log(TH_LOG_FOUND_PAD);
                    }
                    return ZUN_SUCCESS;
                }
            }
        }
    }
}

// FUNCTION: TH07 0x00438668
ZunResult Supervisor::LoadGameData()
{
    char verFile[128];

    // STRING: TH07 0x00497150
    if (g_Pbg4Archive.Load("th07.dat"))
    {
        // STRING: TH07 0x00497140
        sprintf(verFile, "th07_%.4x%c.ver", 256, 98);
        g_Supervisor.version = (char *)FileSystem::OpenFile(verFile, 0);
        g_Supervisor.versionTableSize = g_LastFileSize;
        if (!g_Supervisor.version)
        {
            g_GameErrorContext.Fatal(TH_ERR_DATA_VER_MISMATCH);
            return ZUN_ERROR;
        }
    }
    else
    {
        g_GameErrorContext.Fatal(TH_ERR_DAT_NOT_FOUND);
        return ZUN_ERROR;
    }
    return ZUN_SUCCESS;
}

#pragma var_order(i, frameCount, timeStart, fpsArray, fpsCount, timeEnd, \
                  timeDiff, fps, unused, j, fpsSum)
// FUNCTION: TH07 0x004386f3
i32 Supervisor::CheckVSync()
{
    f32 fpsSum;
    i32 j;
    f32 unused;
    f32 fps;
    i32 timeDiff;
    DWORD timeEnd;
    i32 fpsCount;
    f32 fpsArray[29];
    DWORD timeStart;
    i32 frameCount;
    i32 i;

    i = 0;
    frameCount = 0;
    fpsCount = 0;
    timeStart = 0;

    timeBeginPeriod(1);
    timeStart = timeGetTime();
    timeEndPeriod(1);

    while (i < 1800 && fpsCount < 8)
    {
        g_Supervisor.d3dDevice->BeginScene();
        g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
        g_Supervisor.d3dDevice->EndScene();
        if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
        {
            g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
        }
        i++;
        timeBeginPeriod(1);
        timeEnd = timeGetTime();
        timeEndPeriod(1);
        frameCount++;
        timeDiff = timeEnd - timeStart;

        if (timeDiff >= 700)
        {
            timeStart = timeEnd;
            frameCount = 0;
        }
        else if (timeDiff >= 500)
        {
            unused = (f32)timeDiff / 1000.0f;
            fps = frameCount * 1000.0f / (f32)timeDiff;
            if (fps >= 57.0f)
            {
                fpsArray[fpsCount] = fps;
                fpsCount++;
            }
            timeStart = timeEnd;
            frameCount = 0;
        }
    }

    if (!g_Supervisor.cfg.disableVsync)
    {
        fpsSum = 0.0f;
        if (fpsCount >= 2)
        {
            for (j = 0; j < fpsCount; j++)
            {
                fpsSum += fpsArray[j];
            }
            fpsSum /= (f32)j;
        }
        else
        {
            fpsSum = 1000.0f;
        }

        if (fpsSum > 160.0f)
        {
            g_GameErrorContext.Log(TH_LOG_VSYNC_FAIL);
            g_GameErrorContext.Log(TH_LOG_FORCE_60FPS);
            g_Supervisor.vsyncDisabled = TRUE;
        }
        else if (fpsSum >= 65.0f)
        {
            g_GameErrorContext.Log(TH_LOG_VSYNC_FAIL_2);
            g_GameErrorContext.Log(TH_LOG_FORCE_60FPS);
            g_Supervisor.vsyncDisabled = TRUE;
            return -2;
        }
    }
    return 0;
}

// FUNCTION: TH07 0x00438986
ZunResult Supervisor::AddedCallback(Supervisor *arg)
{
    ScoreDat *scoreDat;
    i32 i;

    QueryPerformanceFrequency(&arg->perfFrequency);
    g_Supervisor.d3dDevice->BeginScene();
    g_Supervisor.d3dDevice->Clear(0, NULL, 1, 0xff000000, 1.0f, 0);
    g_Supervisor.d3dDevice->EndScene();
    if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
    {
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
    }
    g_Supervisor.d3dDevice->BeginScene();
    g_Supervisor.d3dDevice->Clear(0, NULL, 1, 0xff000000, 1.0f, 0);
    g_Supervisor.d3dDevice->EndScene();
    if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
    {
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
    }
    if (LoadGameData() != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    // STRING: TH07 0x00497038
    g_AnmManager->LoadSurface(0, "data/title/th07logo.jpg");
    g_Supervisor.isInEnding = TRUE;
    if (!g_Supervisor.vsyncDisabled)
    {
        if (CheckVSync())
        {
            g_AnmManager->ReleaseSurface(0);
            return (ZunResult)-2;
        }
    }
    else
    {
        i = 0;
        while (i < 4)
        {
            g_Supervisor.d3dDevice->BeginScene();
            g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
            g_Supervisor.d3dDevice->EndScene();
            if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
            {
                g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
            }
            i++;
        }
    }
    g_AnmManager->ReleaseSurface(0);
    arg->isInEnding = FALSE;
    arg->renderSkipFrames = 0;
    arg->lastTotalPlayTimeUpdate = timeGetTime();
    g_Rng.SetSeed(arg->lastTotalPlayTimeUpdate);
    arg->SetupDInput();
    if (!arg->midiOutput)
    {
        arg->midiOutput = new MidiOutput;
    }
    if (arg->midiOutput)
    {
        // STRING: TH07 0x00497028
        arg->midiOutput->ReadFileData(30, "bgm/init.mid");
    }
    g_SoundPlayer.InitSoundBuffers();
    // STRING: TH07 0x00497018
    if (g_AnmManager->LoadAnms(ANM_FILE_TEXT, "data/text.anm", ANM_OFFSET_TEXT) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    if (AsciiManager::RegisterChain() != ZUN_SUCCESS)
    {
        g_GameErrorContext.Log(TH_BGM_ASCII_INIT_FAIL);
        return ZUN_ERROR;
    }

    g_AnmManager->SetupVertexBuffer();
    TextHelper::CreateTextBuffer();
    // STRING: TH07 0x00496fe0
    if (g_SoundPlayer.LoadFmt("bgm/thbgm.fmt"))
    {
        g_GameErrorContext.Log(TH_ERR_BGM_INIT_FAIL);
        return ZUN_ERROR;
    }

    if (g_SoundPlayer.bgmSeekOffset == 0)
    {
        if (!g_Supervisor.cfg.preloadBgm)
        {
            // STRING: TH07 0x00496fac
            g_SoundPlayer.StartBGM("thbgm.dat");
        }
        else
        {
            memcpy(g_SoundPlayer.bgmArchivePath, "thbgm.dat", 10);
        }
    }
    else if (!g_Supervisor.cfg.preloadBgm)
    {
        g_SoundPlayer.StartBGM("th07.dat");
    }
    else
    {
        memcpy(g_SoundPlayer.bgmArchivePath, "th07.dat", 9);
    }
    scoreDat = ResultScreen::OpenScore("score.dat");
    memset(&g_GameManager.plst, 0, sizeof(g_GameManager.plst));
    g_GameManager.plst.th7kLen2 = g_GameManager.plst.th7kLen = sizeof(Plst);
    g_GameManager.plst.magic = PLST_MAGIC;
    g_GameManager.plst.version = 1;
    ResultScreen::ParsePlst(scoreDat, &g_GameManager.plst);
    ResultScreen::ReleaseScoreDat(scoreDat);
    g_Supervisor.midiTimer = new DummyMidiTimer;
    if (g_Supervisor.midiTimer)
    {
        g_Supervisor.midiTimer->StartTimerDefault();
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00438de2
ZunResult Supervisor::DeletedCallback(Supervisor *arg)
{
    SAFE_FREE(g_Supervisor.version);
    g_AnmManager->ReleaseVertexBuffer();
    g_AnmManager->ReleaseAnm(ANM_FILE_TEXT);
    AsciiManager::CutChain();
    // STRING: TH07 0x004980d0
    g_SoundPlayer.PushCommand(AUDIO_SHUTDOWN, 0, "dummy");
    if (arg->midiOutput)
    {
        arg->midiOutput->StopPlayback();
        delete arg->midiOutput;
        arg->midiOutput = NULL;
    }
    ReplayManager::SaveReplay(NULL, NULL);
    TextHelper::ReleaseTextBuffer();
    if (arg->keyboard)
    {
        arg->keyboard->Unacquire();
    }
    SAFE_RELEASE(arg->keyboard);
    if (arg->controller)
    {
        arg->controller->Unacquire();
    }
    SAFE_RELEASE(arg->controller);
    SAFE_RELEASE(arg->directInput);
    SAFE_DELETE(g_GameManager.globals);
    SAFE_DELETE(g_GameManager.defaultCfg);
    g_Pbg4Archive.Release();
    if (g_Supervisor.midiTimer)
    {
        StopMidiTimer(g_Supervisor.midiTimer);
        delete g_Supervisor.midiTimer;
        g_Supervisor.midiTimer = NULL;
    }
    return ZUN_SUCCESS;
}

#pragma var_order(chain, res, mgr)
// FUNCTION: TH07 0x00439000
ZunResult Supervisor::RegisterChain()
{
    ZunResult res;

    Supervisor *mgr = &g_Supervisor;
    mgr->wantedState = SUPERVISOR_STATE_INIT;
    mgr->curState = SUPERVISOR_STATE_EXIT;
    mgr->calcCount = 0;
    ChainElem *chain = g_Chain.CreateElem((ChainCallback)OnUpdate);
    chain->arg = mgr;
    chain->addedCallback = (ChainLifecycleCallback)AddedCallback;
    chain->deletedCallback = (ChainLifecycleCallback)DeletedCallback;
    res = g_Chain.AddToCalcChain(chain, 0);
    if (res)
    {
        return res;
    }

    chain = g_Chain.CreateElem((ChainCallback)OnDraw);
    chain->arg = mgr;
    g_Chain.AddToDrawChain(chain, 15);
    return ZUN_SUCCESS;
}

#pragma var_order(fps, elapsedTimeInSecs, curTime, targetFps, curPerfCounter, \
                  fpsCounterPos, replayFpsCounterPos)
// FUNCTION: TH07 0x004390a5
void Supervisor::DrawFpsCounter(i32 param_1)
{
    Float3 replayFpsCounterPos;
    Float3 fpsCounterPos;
    LARGE_INTEGER curPerfCounter;
    f32 targetFps;
    DWORD curTime;
    f32 elapsedTimeInSecs;
    f32 fps;

    if (!g_GameManager.slowModeSlowActive)
    {
        g_NumFramesSinceLastTime =
            g_NumFramesSinceLastTime + 1 + (u32)g_Supervisor.cfg.frameskipConfig;

        if (g_Supervisor.perfFrequency.LowPart == 0)
        {
            static DWORD g_LastTime = timeGetTime();

            curTime = timeGetTime();
            if (curTime < g_LastTime)
            {
                g_LastTime = curTime;
                g_NumFramesSinceLastTime = 0;
            }
            if (curTime - g_LastTime >= 500)
            {
                elapsedTimeInSecs = (f32)(curTime - g_LastTime) / 1000.0f;
                g_LastTime = curTime;

            MERGE:
                fps = (f32)g_NumFramesSinceLastTime / elapsedTimeInSecs;
                g_NumFramesSinceLastTime = 0;
                // STRING: TH07 0x00496fa0
                sprintf(g_FpsCounterBuffer, "%.02ffps", (f64)fps);
                if (g_GameManager.notInMenu && param_1 != 0)
                {
                    targetFps = 60.0f;
                    g_Supervisor.fpsAccumulator = g_Supervisor.fpsAccumulator + targetFps;
                    if (targetFps * 0.9f < fps)
                    {
                        g_Supervisor.framerateMultiplier =
                            g_Supervisor.framerateMultiplier + targetFps;
                    }
                    else if (targetFps * 0.7f < fps)
                    {
                        g_Supervisor.framerateMultiplier =
                            g_Supervisor.framerateMultiplier + targetFps * 0.8f;
                    }
                    else if (targetFps * 0.5f < fps)
                    {
                        g_Supervisor.framerateMultiplier =
                            g_Supervisor.framerateMultiplier + targetFps * 0.6f;
                    }
                    else
                    {
                        g_Supervisor.framerateMultiplier =
                            g_Supervisor.framerateMultiplier + targetFps * 0.5f;
                    }

                    if (!g_GameManager.replay)
                    {
                        g_Supervisor.curFps = fps + 0.5f;
                    }
                    else
                    {
                        // STRING: TH07 0x00496f9c
                        sprintf(g_ReplayFpsBuffer, "%2d", (i32)g_Supervisor.curFps);
                    }
                }
            }
            goto LAB_00439350;
        }

        if (g_PerformanceCounter.LowPart == 0)
        {
            QueryPerformanceCounter(&g_PerformanceCounter);
        }
        QueryPerformanceCounter(&curPerfCounter);
        if (curPerfCounter.LowPart < g_PerformanceCounter.LowPart)
        {
            g_PerformanceCounter.LowPart = curPerfCounter.LowPart;
            g_PerformanceCounter.HighPart = curPerfCounter.HighPart;
            g_NumFramesSinceLastTime = 0;
        }
        if (curPerfCounter.LowPart >= g_PerformanceCounter.LowPart +
                                          (g_Supervisor.perfFrequency.LowPart >> 1))
        {
            elapsedTimeInSecs =
                (f32)(curPerfCounter.LowPart - g_PerformanceCounter.LowPart) /
                (f32)g_Supervisor.perfFrequency.LowPart;
            g_PerformanceCounter.LowPart = curPerfCounter.LowPart;
            g_PerformanceCounter.HighPart = curPerfCounter.HighPart;
            g_FpsUpdateCounter++;
            if (g_FpsUpdateCounter % 8 == 0)
            {
                g_Supervisor.CheckTiming();
            }
            goto MERGE;
        }
    }

LAB_00439350:
    if (!g_Supervisor.isInEnding && param_1 != 0)
    {
        fpsCounterPos.x = 512.0f;
        fpsCounterPos.y = 464.0f;
        fpsCounterPos.z = 0.0f;
        g_AsciiManager.AddString(&fpsCounterPos, g_FpsCounterBuffer);
        if (g_GameManager.replay &&
            g_GameManager.notInMenu)
        {
            replayFpsCounterPos.x = 384.0f;
            replayFpsCounterPos.y = 448.0f;
            replayFpsCounterPos.z = 0.0f;
            if (g_Supervisor.isFpsBad)
            {
                g_AsciiManager.color = 0xffff4040;
            }
            else
            {
                g_AsciiManager.color = 0xffffffd0;
            }
            g_AsciiManager.AddString(&replayFpsCounterPos, g_ReplayFpsBuffer);
            g_AsciiManager.color = 0xffffffff;
        }
    }
}

// FUNCTION: TH07 0x00439401
void ZunTimer::Increment(i32 value)
{
    if (g_Supervisor.forceIntegerTimer)
    {
        this->current++;
        this->subFrame = 0.0f;
        this->previous = -999;
    }
    if (g_Supervisor.effectiveFramerateMultiplier > 0.99f)
    {
        this->current = this->current + value;
    }
    else
    {
        if (value < 0)
        {
            Decrement(-value);
        }
        else
        {
            this->previous = this->current;
            this->subFrame = (f32)value * g_Supervisor.effectiveFramerateMultiplier +
                             this->subFrame;
            while (this->subFrame >= 1.0f)
            {
                this->current = this->current + 1;
                this->subFrame = this->subFrame - 1.0f;
            }
        }
    }
}

// FUNCTION: TH07 0x004394c7
void ZunTimer::Decrement(i32 value)
{
    if (g_Supervisor.forceIntegerTimer)
    {
        this->current--;
        this->subFrame = 0.0f;
        this->previous = -999;
    }
    if (g_Supervisor.effectiveFramerateMultiplier > 0.99f)
    {
        this->current = this->current - value;
    }
    else
    {
        if (value < 0)
        {
            Increment(-value);
        }
        else
        {
            this->previous = this->current;
            this->subFrame = this->subFrame -
                             (f32)value * g_Supervisor.effectiveFramerateMultiplier;
            while (this->subFrame < 0.0f)
            {
                this->current--;
                this->subFrame = this->subFrame + 1.0f;
            }
        }
    }
}

// FUNCTION: TH07 0x0043958d
void Supervisor::TickTimer(i32 *frames, f32 *subframes)
{
    if (this->effectiveFramerateMultiplier <= 0.99f)
    {
        *subframes = *subframes + this->effectiveFramerateMultiplier;
        if (*subframes >= 1.0f)
        {
            *frames = *frames + 1;
            *subframes = *subframes - 1.0f;
        }
    }
    else
    {
        *frames = *frames + 1;
    }
}

// ZUN name: snapShotScreen
#pragma var_order(bmfh, local_18, local_1c, backBuffer, stride,                    \
                  srcPixel, dstPixel, y, x, bytesPerRow, lockedRect, bytesWritten, \
                  bitmapFile)
// FUNCTION: TH07 0x004395fb
i32 Supervisor::SnapshotScreen(const char *filename)
{
    HANDLE bitmapFile;
    DWORD bytesWritten;
    D3DLOCKED_RECT lockedRect;
    i32 bytesPerRow;
    i32 x;
    i32 y;
    u8 *dstPixel;
    u8 *srcPixel;
    i32 stride;
    IDirect3DSurface8 *backBuffer;
    BITMAPINFO *bitmapInfo;
    void *bitmapData;
    BITMAPFILEHEADER bmfh;

    bitmapInfo = NULL;
    bitmapData = NULL;
    backBuffer = NULL;
    this->d3dDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
    memset(&bmfh, 0, sizeof(BITMAPFILEHEADER));

    // STRING: TH07 0x00496f98
    bmfh.bfType = *(WORD *)&"BM";
    bmfh.bfSize = bmfh.bfOffBits = 54;
    switch (this->presentParameters.BackBufferFormat)
    {
    case D3DFMT_R5G6B5:
        g_GameErrorContext.Log(TH_LOG_16BIT_NOT_SUPPORTED);
        break;
    case D3DFMT_X8R8G8B8:
        bitmapInfo = (BITMAPINFO *)ZunMemory::Alloc2(sizeof(BITMAPINFO));
        if (!bitmapInfo)
        {
            g_GameErrorContext.Log(TH_LOG_BITMAP_ALLOC_FAIL);
            break;
        }

        memset(bitmapInfo, 0, sizeof(BITMAPINFO));
        stride = 1920;
        bitmapData = malloc(stride * GAME_WINDOW_HEIGHT);
        if (!bitmapData)
        {
            g_GameErrorContext.Log(TH_LOG_BITMAP_ALLOC_FAIL);
            break;
        }

        bmfh.bfSize += stride * GAME_WINDOW_HEIGHT;
        bitmapInfo->bmiHeader.biBitCount = 24;
        bitmapInfo->bmiHeader.biSize = 40;
        bitmapInfo->bmiHeader.biWidth = GAME_WINDOW_WIDTH;
        bitmapInfo->bmiHeader.biHeight = GAME_WINDOW_HEIGHT;
        bitmapInfo->bmiHeader.biPlanes = 1;
        bitmapInfo->bmiHeader.biCompression = 0;
        backBuffer->LockRect(&lockedRect, NULL, 0);
        bytesPerRow = 0;
        for (y = 479; -1 < y; y--, bytesPerRow++)
        {
            dstPixel = (u8 *)((u8 *)bitmapData + stride * bytesPerRow);
            srcPixel = (u8 *)((u8 *)lockedRect.pBits + lockedRect.Pitch * y);
            for (x = 0; x < GAME_WINDOW_WIDTH; x++)
            {
                *dstPixel = *srcPixel;
                srcPixel++;
                dstPixel++;
                *dstPixel = *srcPixel;
                srcPixel++;
                dstPixel++;
                *dstPixel = *srcPixel;
                srcPixel += 2;
                dstPixel++;
            }
        }
        backBuffer->UnlockRect();
        bitmapFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, 2, FILE_ATTRIBUTE_NORMAL, NULL);
        if (bitmapFile == INVALID_HANDLE_VALUE)
        {
            break;
        }

        WriteFile(bitmapFile, &bmfh, 14, &bytesWritten, NULL);
        WriteFile(bitmapFile, bitmapInfo, 40, &bytesWritten, NULL);
        WriteFile(bitmapFile, bitmapData, stride * GAME_WINDOW_HEIGHT, &bytesWritten, NULL);
        CloseHandle(bitmapFile);
        break;
    default:
        // STRING: TH07 0x00496f48
        g_GameErrorContext.Log("error ? mother.cpp\r\n");
        return 1;
    }
    SAFE_RELEASE(backBuffer);
    free(bitmapInfo);
    free(bitmapData);
    return 0;
}

#pragma var_order(configFile, bgm2, bytesRead2, bgm2Data, bgm, bytesRead, bgmData)
// FUNCTION: TH07 0x004398b6
ZunResult Supervisor::LoadConfig(const char *configFilename)
{
    i32 bgmData[4];
    DWORD bytesRead;
    HANDLE bgm;
    i32 bgm2Data[4];
    DWORD bytesRead2;
    HANDLE bgm2;
    u32 *configFile;

    memset(&g_Supervisor.cfg, 0, sizeof(GameConfiguration));
    configFile = (u32 *)FileSystem::OpenFile((char *)configFilename, 1);
    if (!configFile)
    {
        g_GameErrorContext.Log(TH_LOG_CONFIG_INIT);
    init:
        g_Supervisor.cfg.lifeCount = 2;
        g_Supervisor.cfg.bombCount = 3;
        g_Supervisor.cfg.colorMode16bit = 255;
        g_Supervisor.cfg.version = 0x70002;
        g_Supervisor.cfg.padAxisX = 600;
        g_Supervisor.cfg.padAxisY = 600;
        bgm2 = CreateFileA("./thbgm.dat", GENERIC_READ, 1, NULL, 3, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, NULL);
        if (bgm2 != INVALID_HANDLE_VALUE)
        {
            ReadFile(bgm2, bgm2Data, 16, &bytesRead2, NULL);
            CloseHandle(bgm2);
            if (bgm2Data[0] != 0x5641575a || bgm2Data[1] != 1 ||
                bgm2Data[2] != 0x700)
            {
                g_GameErrorContext.Fatal(TH_ERR_BGM_VER_MISMATCH);
                return ZUN_ERROR;
            }
            g_Supervisor.cfg.musicMode = MUSIC_WAV;
        }
        else
        {
            g_Supervisor.cfg.musicMode = MUSIC_MIDI;
            Supervisor::DebugPrint2(TH_LOG_WAV_UNAVAILABLE);
        }
        g_Supervisor.cfg.playSounds = 1;
        g_Supervisor.cfg.defaultDifficulty = (u8)DIFF_NORMAL;
        g_Supervisor.cfg.windowed = 0;
        g_Supervisor.cfg.frameskipConfig = 0;
        g_Supervisor.cfg.controllerMapping = g_ControllerMapping;
        g_Supervisor.cfg.effectQuality = QUALITY_BEAUTIFUL;
        g_Supervisor.cfg.slowMode = 0;
        g_Supervisor.cfg.shotSlow = 1;
    }
    else
    {
        g_Supervisor.cfg = *(GameConfiguration *)configFile;
        free(configFile);

        bgm = CreateFileA("./thbgm.dat", GENERIC_READ, 1, NULL, 3, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, NULL);
        if (bgm != INVALID_HANDLE_VALUE)
        {
            ReadFile(bgm, bgmData, 16, &bytesRead, NULL);
            CloseHandle(bgm);
            if (bgmData[0] != 0x5641575a || bgmData[1] != 1 ||
                bgmData[2] != 0x700)
            {
                g_GameErrorContext.Fatal(TH_ERR_BGM_VER_MISMATCH);
                return ZUN_ERROR;
            }
        }
        if (!(g_Supervisor.cfg.lifeCount < 5 &&
              g_Supervisor.cfg.bombCount < 4 &&
              g_Supervisor.cfg.colorMode16bit < 2 &&
              g_Supervisor.cfg.musicMode < 3 &&
              g_Supervisor.cfg.defaultDifficulty < 6 &&
              g_Supervisor.cfg.playSounds < 2 &&
              g_Supervisor.cfg.windowed < 2 &&
              g_Supervisor.cfg.frameskipConfig < 3 &&
              g_Supervisor.cfg.effectQuality < 3 &&
              g_Supervisor.cfg.slowMode < 2 &&
              g_Supervisor.cfg.shotSlow < 2 &&
              g_Supervisor.cfg.version == 0x70002 &&
              g_LastFileSize == sizeof(GameConfiguration)))
        {
            g_GameErrorContext.Log(TH_LOG_CONFIG_REINIT);
            goto init;
        }
        g_ControllerMapping = g_Supervisor.cfg.controllerMapping;
    }
    g_Supervisor.cfg.colorAddEmulation = 1;
    if (this->cfg.noVertexBuffers)
    {
        g_GameErrorContext.Log(TH_CONFIG_NO_VERTEX_BUFFERS);
    }
    if (this->cfg.disableFog)
    {
        g_GameErrorContext.Log(TH_CONFIG_NO_FOG);
    }
    if (this->cfg.use16BitTextures)
    {
        g_GameErrorContext.Log(TH_CONFIG_FORCE_16BIT);
    }
    if (this->IsClearingBackbuffer())
    {
        g_GameErrorContext.Log(TH_CONFIG_FORCE_BACKBUFFER_CLEAR);
    }
    if (this->cfg.disableItemDrawAroundPlayfield)
    {
        g_GameErrorContext.Log(TH_CONFIG_DISABLE_ITEM_DRAW_AROUND_PLAYFIELD);
    }
    if (this->cfg.disableGouraud)
    {
        g_GameErrorContext.Log(TH_CONFIG_DISABLE_GOURAUD);
    }
    if (this->cfg.disableZBuffer)
    {
        g_GameErrorContext.Log(TH_CONFIG_DISABLE_DEPTH_TEST);
    }
    this->vsyncDisabled = FALSE;
    this->cfg.unused = 0;
    if (this->cfg.disableTextureBlend)
    {
        g_GameErrorContext.Log(TH_CONFIG_DISABLE_TEXTURE_BLEND);
    }
    if (this->cfg.windowed)
    {
        g_GameErrorContext.Log(TH_CONFIG_WINDOWED);
    }
    if (this->cfg.forceReferenceRender)
    {
        g_GameErrorContext.Log(TH_CONFIG_FORCE_REFERENCE_RENDER);
    }
    if (this->cfg.disableDinput)
    {
        g_GameErrorContext.Log(TH_CONFIG_DISABLE_DINPUT);
    }
    if (this->cfg.redrawEveryFrame)
    {
        g_GameErrorContext.Log(TH_CONFIG_REDRAW_EVERY_FRAME);
    }
    if (this->cfg.preloadBgm)
    {
        g_GameErrorContext.Log(TH_CONFIG_PRELOAD_BGM);
    }
    if (this->cfg.disableVsync)
    {
        g_GameErrorContext.Log(TH_CONFIG_DISABLE_VSYNC);
        g_Supervisor.vsyncDisabled = TRUE;
    }
    if (FileSystem::WriteDataToFile(configFilename, &g_Supervisor.cfg,
                                    sizeof(GameConfiguration)))
    {
        g_GameErrorContext.Fatal(TH_ERR_WRITE_CONFIG_FAIL, configFilename);
        g_GameErrorContext.Fatal(TH_ERR_WRITE_CONFIG_FAIL_HELP);
        return ZUN_ERROR;
    }

    return ZUN_SUCCESS;
}

#pragma var_order(pathext, pathbuf)
// FUNCTION: TH07 0x00439dd0
i32 Supervisor::LoadAudio(i32 idx, const char *path)
{
    char pathbuf[256];
    char *pathext;

    if (g_Supervisor.cfg.musicMode == MUSIC_MIDI)
    {
        if (g_Supervisor.midiOutput)
        {
            g_Supervisor.midiOutput->ReadFileData(idx, path);
        }
        return 0;
    }
    else
    {
        if (g_Supervisor.cfg.musicMode == MUSIC_WAV)
        {
            strcpy(pathbuf, path);

            // ZUN landmine: the result of strrchr is not checked for NULL.
            pathext = strrchr(pathbuf, '.');
            pathext[1] = 'w';
            pathext[2] = 'a';
            pathext[3] = 'v';
            g_SoundPlayer.PushCommand(AUDIO_PRELOAD, idx, pathbuf);
        }
        return 1;
    }
}

// FUNCTION: TH07 0x00439ec1
ZunResult Supervisor::PlayLoadedAudio(i32 idx)
{
    if (g_Supervisor.cfg.musicMode == MUSIC_MIDI)
    {
        if (g_Supervisor.midiOutput)
        {
            g_Supervisor.midiOutput->PlayLoaded(idx);
        }
        return ZUN_SUCCESS;
    }
    if (g_Supervisor.cfg.musicMode == MUSIC_WAV)
    {
        if (g_Supervisor.cfg.preloadBgm)
        {
            g_SoundPlayer.PushCommand(AUDIO_SHUTDOWN, 0, "dummy");
        }
        g_SoundPlayer.PushCommand(AUDIO_START, idx, "dummy");
    }
    return ZUN_SUCCESS;
}

#pragma var_order(pathExt, pathBuf)
// FUNCTION: TH07 0x00439f4d
ZunResult Supervisor::PlayAudio(const char *path)
{
    char pathBuf[256];
    char *pathExt;

    if (g_Supervisor.cfg.musicMode == MUSIC_MIDI)
    {
        if (g_Supervisor.midiOutput)
        {
            g_Supervisor.midiOutput->Play(path);
        }
    }
    else
    {
        if (g_Supervisor.cfg.musicMode == MUSIC_WAV)
        {
            // ZUN landmine: the result of strrchr is not checked for NULL.
            strcpy(pathBuf, path);
            pathExt = strrchr(pathBuf, '.');
            pathExt[1] = 'w';
            pathExt[2] = 'a';
            pathExt[3] = 'v';
            g_SoundPlayer.PushCommand(AUDIO_START, -1, pathBuf);
        }
        else
        {
            return ZUN_ERROR;
        }
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0043a05f
ZunResult Supervisor::StopAudio()
{
    if (g_Supervisor.cfg.musicMode == MUSIC_MIDI)
    {
        if (g_Supervisor.midiOutput)
        {
            g_Supervisor.midiOutput->StopPlayback();
        }
    }
    else
    {
        if (g_Supervisor.cfg.musicMode == MUSIC_WAV)
        {
            if (g_Supervisor.cfg.preloadBgm)
            {
                g_SoundPlayer.PushCommand(AUDIO_SHUTDOWN, 0, "dummy");
            }
            else
            {
                g_SoundPlayer.PushCommand(AUDIO_STOP, 0, "dummy");
            }
        }
        else
        {
            return ZUN_ERROR;
        }
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0043a0d6
i32 Supervisor::FadeOutMusic(f32 musicFadeFrames)
{
    f32 effectiveFadeFrames;

    if (g_Supervisor.cfg.musicMode == MUSIC_MIDI)
    {
        if (g_Supervisor.midiOutput)
        {
            g_Supervisor.midiOutput->SetFadeOut(1000.0f * musicFadeFrames);
        }
    }
    else
    {
        if (g_Supervisor.cfg.musicMode == MUSIC_WAV)
        {
            if (this->effectiveFramerateMultiplier == 0.0f)
            {
                effectiveFadeFrames = musicFadeFrames;
            }
            else if (this->effectiveFramerateMultiplier > 1.0f)
            {
                effectiveFadeFrames = musicFadeFrames;
            }
            else
            {
                effectiveFadeFrames = musicFadeFrames / this->effectiveFramerateMultiplier;
            }
            // STRING: TH07 0x00496c1e
            g_SoundPlayer.PushCommand(AUDIO_FADEOUT, effectiveFadeFrames, "");
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

// FUNCTION: TH07 0x0043a18d
i32 Supervisor::IsSlowMode()
{
    return g_GameManager.defaultCfg != NULL &&
           g_GameManager.defaultCfg->slowMode;
}

// FUNCTION: TH07 0x0043a1bd
HRESULT Supervisor::EnableFog()
{
    g_AnmManager->Flush();
    if (this->fogEnabled != TRUE)
    {
        this->fogEnabled = TRUE;
        return this->d3dDevice->SetRenderState(D3DRS_FOGENABLE, TRUE);
    }

    return 0;
}

// FUNCTION: TH07 0x0043a207
HRESULT Supervisor::DisableFog()
{
    g_AnmManager->Flush();
    if (this->fogEnabled)
    {
        this->fogEnabled = FALSE;
        return this->d3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }

    return 0;
}

// FUNCTION: TH07 0x0043a24e
void Supervisor::SetRenderState(D3DRENDERSTATETYPE stateType, DWORD param_2)
{
    g_AnmManager->Flush();
    this->d3dDevice->SetRenderState(stateType, param_2);
}

#pragma var_order(time, timeSinceStartup)
// FUNCTION: TH07 0x0043a27f
void Supervisor::UpdateStartupTime()
{
    u32 timeSinceStartup;
    DWORD time;

    time = timeGetTime();
    if (time < this->lastTotalPlayTimeUpdate)
    {
        this->lastTotalPlayTimeUpdate = 0;
    }
    timeSinceStartup = time - this->lastTotalPlayTimeUpdate;
    g_GameManager.plst.totalHours += timeSinceStartup / 3600000;
    timeSinceStartup %= 3600000;
    g_GameManager.plst.totalMinutes += timeSinceStartup / 60000;
    timeSinceStartup %= 60000;
    g_GameManager.plst.totalSeconds += timeSinceStartup / 1000;
    timeSinceStartup %= 1000;
    g_GameManager.plst.totalMilliseconds += timeSinceStartup;
    if (g_GameManager.plst.totalMilliseconds >= 1000)
    {
        g_GameManager.plst.totalSeconds +=
            g_GameManager.plst.totalMilliseconds / 1000;
        g_GameManager.plst.totalMilliseconds %= 1000;
    }
    if (g_GameManager.plst.totalSeconds >= 60)
    {
        g_GameManager.plst.totalMinutes += g_GameManager.plst.totalSeconds / 60;
        g_GameManager.plst.totalSeconds %= 60;
    }
    if (g_GameManager.plst.totalMinutes >= 60)
    {
        g_GameManager.plst.totalHours += g_GameManager.plst.totalMinutes / 60;
        g_GameManager.plst.totalMinutes %= 60;
    }
    this->lastTotalPlayTimeUpdate = time;
}

#pragma var_order(time, timeSinceLastTime)
// FUNCTION: TH07 0x0043a3f4
void Supervisor::UpdateTime()
{
    u32 timeSinceLastTime;
    DWORD time;

    time = timeGetTime();
    if (time < this->currentTime)
    {
        this->currentTime = 0;
    }
    timeSinceLastTime = time - this->currentTime;
    g_GameManager.plst.gameHours += timeSinceLastTime / 3600000;
    timeSinceLastTime %= 3600000;
    g_GameManager.plst.gameMinutes += timeSinceLastTime / 60000;
    timeSinceLastTime %= 60000;
    g_GameManager.plst.gameSeconds += timeSinceLastTime / 1000;
    timeSinceLastTime %= 1000;
    g_GameManager.plst.gameMilliseconds += timeSinceLastTime;
    if (g_GameManager.plst.gameMilliseconds >= 1000)
    {
        g_GameManager.plst.gameSeconds +=
            g_GameManager.plst.gameMilliseconds / 1000;
        g_GameManager.plst.gameMilliseconds %= 1000;
    }
    if (g_GameManager.plst.gameSeconds >= 60)
    {
        g_GameManager.plst.gameMinutes += g_GameManager.plst.gameSeconds / 60;
        g_GameManager.plst.gameSeconds %= 60;
    }
    if (g_GameManager.plst.gameMinutes >= 60)
    {
        g_GameManager.plst.gameHours += g_GameManager.plst.gameMinutes / 60;
        g_GameManager.plst.gameMinutes %= 60;
    }
    this->currentTime = time;
}

#pragma var_order(local_8, local_c, local_10, local_14, local_18)
// FUNCTION: TH07 0x0043a569
ZunResult Supervisor::CheckIntegrity(const char *version, i32 exeSize,
                                     i32 exeChecksum)
{
#ifdef NON_MATCHING
    return ZUN_SUCCESS;
#else
    i32 local_18;
    char *local_14;
    i32 local_10;
    i32 local_c;
    char *local_8;

    if (!this->version)
    {
        return ZUN_SUCCESS;
    }
    else
    {
        local_8 = this->version;
        local_10 = this->versionTableSize;
        // STRING: TH07 0x00496c18
        if (strncmp(version, "debug", 5) == 0)
        {
            return ZUN_SUCCESS;
        }
        else
        {
            if (strcmp("0100b", "debug") == 0)
            {
                return ZUN_SUCCESS;
            }
            else
            {
                while ((u32)local_10 > 0)
                {
                    if (strncmp(version, local_8, 5) == 0)
                    {
                        local_8 = local_8 + 6;
                        // STRING: TH07 0x00496c10
                        sscanf(local_8, "%d %d", &local_18, &local_c);
                        if (local_18 == exeSize && local_c == exeChecksum)
                        {
                            return ZUN_SUCCESS;
                        }
                        return ZUN_ERROR;
                    }
                    local_14 = local_8;
                    local_8 = strchr(local_8, 10) + 1;
                    local_10 -= (u8 *)local_8 - (u8 *)local_14;
                }
                return ZUN_ERROR;
            }
        }
    }
#endif
}
