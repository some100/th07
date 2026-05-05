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
#include "pbg4/Pbg4Archive.hpp"

#pragma optimize("s", on)

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
char g_FpsCounterBuffer2[256];

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

    if (this->checkTiming == 0)
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
    timeDiff = (timeDiff * 1000.0 + (f64)this->curTime.wMilliseconds -
                (f64)this->prevTime.wMilliseconds);
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
            // STRING: TH07 0x00497244
            Supervisor::DebugPrint2("alq チェック %f / %f = %f\r\n", timeDiff, perfDiff,
                                    timeDiff / perfDiff);
        }
        else if (this->timingErrorCount != 0)
        {
            this->timingErrorCount--;
        }
        this->checkTiming = 0;
    }

    if (this->maxTimingError >= 0x28 || this->timingBadCount >= 0x10)
    {
        this->flags |= 8;
    }
    else
    {
        this->flags &= 0xfffffff7;
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
    g_AnmManager->SetVertexShader(0xff);
    g_AnmManager->SetSprite(NULL);
    g_AnmManager->SetTexture(NULL);
    g_AnmManager->SetColorOp(0xff);
    g_AnmManager->SetBlendMode(0xff);
    g_AnmManager->SetZWriteDisable(0xff);
    g_AnmManager->ClearFrameState();
    g_AnmManager->SetCameraMode(0xff);
    g_AnmManager->SetColor(0x80808080);
    g_AnmManager->offset.y = 0.0f;
    g_AnmManager->offset.x = 0.0f;
    g_Supervisor.fogEnabled = 0xff;
    if (g_SoundPlayer.backgroundMusic != NULL)
    {
        g_SoundPlayer.backgroundMusic->UpdateFadeOut();
    }
    if (g_GameManager.slowModeSlowActive == 0)
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
        arg->wantedState2 = arg->wantedState;
        // STRING: TH07 0x00497230
        Supervisor::DebugPrint2("scene %d -> %d\r\n", arg->wantedState,
                                arg->curState);
        switch (arg->wantedState)
        {
        case 0:
        CASE_0:
            arg->curState = 1;
            g_Supervisor.d3dDevice->ResourceManagerDiscardBytes(0);
            if (MainMenu::RegisterChain(0) != ZUN_SUCCESS)
            {
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            }
            break;
        case 1:
            switch (arg->curState)
            {
            case -1:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case 2:
                if (GameManager::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            case 4:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR;
            case 5:
                if (ResultScreen::RegisterChain(0) != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            case 8:
                if (MusicRoom::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            case 9:
                GameManager::CutChain();
                if (Ending::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            }
            break;
        case 5:
            switch (arg->curState)
            {
            case -1:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case 1:
                arg->curState = 0;
                goto CASE_0;
            }
            break;
        case 2:
            switch (arg->curState)
            {
            case -1:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case 1:
                GameManager::CutChain();
                arg->curState = 0;
                ReplayManager::SaveReplay(NULL, NULL);
                goto CASE_0;
                break;
            case 6:
                GameManager::CutChain();
                if (ResultScreen::RegisterChain(1) != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            case 10:
                GameManager::CutChain();
                if ((g_GameManager.practice == 0) && (g_GameManager.difficulty < 4))
                {
                    g_GameManager.currentStage = 0;
                }
                else
                {
                    g_GameManager.currentStage -= 1;
                }
                ReplayManager::SaveReplay(NULL, NULL);
                if (GameManager::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                arg->curState = 2;
                break;
            case 11:
                g_Supervisor.curState = 3;
                GameManager::CutChain();
                g_GameManager.currentStage -= 1;
                if (GameManager::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                arg->curState = 2;
                break;
            case 12:
                g_Supervisor.curState = 3;
                GameManager::CutChain();
                if (GameManager::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                arg->curState = 2;
                break;
            case 3:
                GameManager::CutChain();
                if (GameManager::RegisterChain() != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                arg->curState = 2;
                break;
            case 7:
                GameManager::CutChain();
                arg->curState = 0;
                ReplayManager::SaveReplay(NULL, NULL);
                arg->curState = 1;
                g_Supervisor.d3dDevice->ResourceManagerDiscardBytes(0);
                if (MainMenu::RegisterChain(1) != ZUN_SUCCESS)
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                break;
            case 9:
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
        case 6:
            switch (arg->curState)
            {
            case -1:
                ReplayManager::SaveReplay(NULL, NULL);
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case 1:
                arg->curState = 0;
                ReplayManager::SaveReplay(NULL, NULL);
                goto CASE_0;
            }
            break;
        case 8:
            switch (arg->curState)
            {
            case -1:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case 1:
                arg->curState = 0;
                goto CASE_0;
            }
            break;
        case 9:
            switch (arg->curState)
            {
            case -1:
                return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
            case 1:
                arg->curState = 0;
                goto CASE_0;
            case 6:
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
    if ((arg->calcCount % 4000 == 3999) &&
        (g_Supervisor.CheckIntegrity("0100b", g_Supervisor.exeSize,
                                     g_Supervisor.exeChecksum) != ZUN_SUCCESS))
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
    if (g_Supervisor.controller == NULL)
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

#pragma var_order(local_1c, idk)
// FUNCTION: TH07 0x0043836e
i32 __stdcall Supervisor::ControllerCallback(LPCDIDEVICEOBJECTINSTANCE param_1,
                                             void *param_2)
{
    DIPROPRANGE local_1c;
    void *idk = param_2;

    if (param_1->dwType & DIDFT_AXIS)
    {
        local_1c.diph.dwSize = 0x18;
        local_1c.diph.dwHeaderSize = 0x10;
        local_1c.diph.dwHow = 2;
        local_1c.diph.dwObj = param_1->dwType;
        local_1c.lMin = -1000;
        local_1c.lMax = 1000;
        if (g_Supervisor.controller->SetProperty(DIPROP_RANGE, &local_1c.diph) <
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
    if ((this->cfg.opts >> 0xb & 1) != 0)
    {
        return ZUN_ERROR;
    }

    if (FAILED(DirectInput8Create(hInstance, 0x800, IID_IDirectInput8A,
                                  (LPVOID *)&this->directInput, NULL)))
    {
        this->directInput = NULL;
        // STRING: TH07 0x00497208
        g_GameErrorContext.Log("DirectInput が使用できません\r\n");
        return ZUN_ERROR;
    }
    else
    {
        if (FAILED(this->directInput->CreateDevice(GUID_SysKeyboard,
                                                   &this->keyboard, NULL)))
        {
            SAFE_RELEASE(this->directInput);
            g_GameErrorContext.Log("DirectInput が使用できません\r\n");
            return ZUN_ERROR;
        }
        else
        {
            if (FAILED(this->keyboard->SetDataFormat(&c_dfDIKeyboard)))
            {
                SAFE_RELEASE(this->keyboard);
                SAFE_RELEASE(this->directInput);
                // STRING: TH07 0x004971d8
                g_GameErrorContext.Log("DirectInput SetDataFormat が使用できません\r\n");
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
                    // STRING: TH07 0x004971a4
                    g_GameErrorContext.Log("DirectInput SetCooperativeLevel が使用できません\r\n");
                    return ZUN_ERROR;
                }
                else
                {
                    this->keyboard->Acquire();
                    // STRING: TH07 0x0049717c
                    g_GameErrorContext.Log("DirectInput は正常に初期化されました\r\n");
                    this->directInput->EnumDevices(4, EnumGameControllersCb, NULL, 1);
                    if (this->controller != NULL)
                    {
                        this->controller->SetDataFormat(&c_dfDIJoystick2);
                        this->controller->SetCooperativeLevel(this->hwndGameWindow, 10);
                        g_Supervisor.controllerCaps.dwSize = 0x2c;
                        this->controller->GetCapabilities(&g_Supervisor.controllerCaps);
                        this->controller->EnumObjects(ControllerCallback, NULL, 0);
                        // STRING: TH07 0x0049715c
                        g_GameErrorContext.Log("有効なパッドを発見しました\r\n");
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
        if (g_Supervisor.version == NULL)
        {
            // STRING: TH07 0x00497118
            g_GameErrorContext.Fatal("error : データのバージョンが違います\r\n");
            return ZUN_ERROR;
        }
    }
    else
    {
        // STRING: TH07 0x004970f0
        g_GameErrorContext.Fatal("error : データファイルが存在しません\r\n");
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

    while (i < 0x708 && fpsCount < 8)
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

    if ((g_Supervisor.cfg.opts >> 0xe & 1) == 0)
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
            g_GameErrorContext.Log("垂直同期が取れてないか、リフレッシュレートが高すぎます\r\n");
            g_GameErrorContext.Log("強制６０フレームモードで動作します\r\n");
            g_Supervisor.vsyncEnabled = 1;
        }
        else if (fpsSum >= 65.0f)
        {
            g_GameErrorContext.Log("垂直同期が取れてないか、リフレッシュレートが高すぎます。\r\n");
            g_GameErrorContext.Log("強制６０フレームモードで動作します\r\n");
            g_Supervisor.vsyncEnabled = 1;
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
    g_Supervisor.isInEnding = 1;
    if (g_Supervisor.vsyncEnabled == 0)
    {
        if (CheckVSync() != 0)
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
    arg->isInEnding = 0;
    arg->renderSkipFrames = 0;
    arg->startupTimeForMenuMusic = timeGetTime();
    g_Rng.SetSeed(arg->startupTimeForMenuMusic);
    arg->SetupDInput();
    if (arg->midiOutput == NULL)
    {
        arg->midiOutput = new MidiOutput;
    }
    if (arg->midiOutput != NULL)
    {
        // STRING: TH07 0x00497028
        arg->midiOutput->ReadFileData(0x1e, "bgm/init.mid");
    }
    g_SoundPlayer.InitSoundBuffers();
    // STRING: TH07 0x00497018
    if (g_AnmManager->LoadAnms(0, "data/text.anm", 0x700) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    if (AsciiManager::RegisterChain() != ZUN_SUCCESS)
    {
        // STRING: TH07 0x00496ff0
        g_GameErrorContext.Log("error : 文字の初期化に失敗しました\r\n");
        return ZUN_ERROR;
    }

    g_AnmManager->SetupVertexBuffer();
    TextHelper::CreateTextBuffer();
    // STRING: TH07 0x00496fe0
    if (g_SoundPlayer.LoadFmt("bgm/thbgm.fmt") != 0)
    {
        // STRING: TH07 0x00496fb8
        g_GameErrorContext.Log("error : BGM の初期化に失敗しました\r\n");
        return ZUN_ERROR;
    }

    if (g_SoundPlayer.bgmSeekOffset == 0)
    {
        if ((g_Supervisor.cfg.opts >> 0xd & 1) == 0)
        {
            // STRING: TH07 0x00496fac
            g_SoundPlayer.StartBGM("thbgm.dat");
        }
        else
        {
            memcpy(g_SoundPlayer.bgmArchivePath, "thbgm.dat", 10);
        }
    }
    else if ((g_Supervisor.cfg.opts >> 0xd & 1) == 0)
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
    g_GameManager.plst.magic = 0x54534c50;
    g_GameManager.plst.version = 1;
    ResultScreen::ParsePlst(scoreDat, &g_GameManager.plst);
    ResultScreen::ReleaseScoreDat(scoreDat);
    g_Supervisor.midiTimer = new DummyMidiTimer;
    if (g_Supervisor.midiTimer != NULL)
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
    g_AnmManager->ReleaseAnm(0);
    AsciiManager::CutChain();
    // STRING: TH07 0x004980d0
    g_SoundPlayer.PushCommand(AUDIO_SHUTDOWN, 0, "dummy");
    if (arg->midiOutput != NULL)
    {
        arg->midiOutput->StopPlayback();
        delete arg->midiOutput;
        arg->midiOutput = NULL;
    }
    ReplayManager::SaveReplay(NULL, NULL);
    TextHelper::ReleaseTextBuffer();
    if (arg->keyboard != NULL)
    {
        arg->keyboard->Unacquire();
    }
    SAFE_RELEASE(arg->keyboard);
    if (arg->controller != NULL)
    {
        arg->controller->Unacquire();
    }
    SAFE_RELEASE(arg->controller);
    SAFE_RELEASE(arg->directInput);
    SAFE_DELETE(g_GameManager.globals);
    SAFE_DELETE(g_GameManager.defaultCfg);
    g_Pbg4Archive.Release();
    if (g_Supervisor.midiTimer != NULL)
    {
        StopMidiTimer(g_Supervisor.midiTimer);
        delete g_Supervisor.midiTimer;
        g_Supervisor.midiTimer = NULL;
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00438fef
DummyMidiTimer::~DummyMidiTimer()
{
}

#pragma var_order(chain, res, mgr)
// FUNCTION: TH07 0x00439000
ZunResult Supervisor::RegisterChain()
{
    ZunResult res;

    Supervisor *mgr = &g_Supervisor;
    mgr->wantedState = 0;
    mgr->curState = -1;
    mgr->calcCount = 0;
    ChainElem *chain = g_Chain.CreateElem((ChainCallback)OnUpdate);
    chain->arg = mgr;
    chain->addedCallback = (ChainLifecycleCallback)AddedCallback;
    chain->deletedCallback = (ChainLifecycleCallback)DeletedCallback;
    res = g_Chain.AddToCalcChain(chain, 0);
    if (res != 0)
    {
        return res;
    }

    chain = g_Chain.CreateElem((ChainCallback)OnDraw);
    chain->arg = mgr;
    g_Chain.AddToDrawChain(chain, 0xf);
    return ZUN_SUCCESS;
}

#pragma var_order(fps, elapsedTimeInSecs, curTime, targetFps, local_1c, local_28, local_34)
// FUNCTION: TH07 0x004390a5
void Supervisor::DrawFpsCounter(i32 param_1)
{
    D3DXVECTOR3 local_30;
    D3DXVECTOR3 local_24;
    LARGE_INTEGER local_18;
    f32 targetFps;
    DWORD curTime;
    f32 elapsedTimeInSecs;
    f32 fps;

    if (g_GameManager.slowModeSlowActive == 0)
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
                if ((g_GameManager.notInMenu != 0) && (param_1 != 0))
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

                    if (g_GameManager.replay == 0)
                    {
                        g_Supervisor.curFps = (fps + 0.5f);
                    }
                    else
                    {
                        // STRING: TH07 0x00496f9c
                        sprintf(g_FpsCounterBuffer2, "%2d", (i32)g_Supervisor.curFps);
                    }
                }
                goto LAB_00439350;
            }
        }

        if (g_PerformanceCounter.LowPart == 0)
        {
            QueryPerformanceCounter(&g_PerformanceCounter);
        }
        QueryPerformanceCounter(&local_18);
        if (local_18.LowPart < g_PerformanceCounter.LowPart)
        {
            g_PerformanceCounter.LowPart = local_18.LowPart;
            g_PerformanceCounter.HighPart = local_18.HighPart;
            g_NumFramesSinceLastTime = 0;
        }
        if (local_18.LowPart >= g_PerformanceCounter.LowPart +
                                    (g_Supervisor.perfFrequency.LowPart >> 1))
        {
            elapsedTimeInSecs =
                ((f32)(local_18.LowPart - g_PerformanceCounter.LowPart) /
                 (f32)g_Supervisor.perfFrequency.LowPart);
            g_PerformanceCounter.LowPart = local_18.LowPart;
            g_PerformanceCounter.HighPart = local_18.HighPart;
            g_FpsUpdateCounter = g_FpsUpdateCounter + 1;
            if (g_FpsUpdateCounter % 8 == 0)
            {
                g_Supervisor.CheckTiming();
            }
            goto MERGE;
        }
    }

LAB_00439350:
    if ((g_Supervisor.isInEnding == 0) && (param_1 != 0))
    {
        local_24.x = 512.0f;
        local_24.y = 464.0f;
        local_24.z = 0.0f;
        g_AsciiManager.AddString(&local_24, g_FpsCounterBuffer);
        if ((g_GameManager.replay != 0) &&
            (g_GameManager.notInMenu != 0))
        {
            local_30.x = 384.0f;
            local_30.y = 448.0f;
            local_30.z = 0.0f;
            if (g_Supervisor.isFpsBad != 0)
            {
                g_AsciiManager.color = 0xffff4040;
            }
            else
            {
                g_AsciiManager.color = 0xffffffd0;
            }
            g_AsciiManager.AddString(&local_30, g_FpsCounterBuffer2);
            g_AsciiManager.color = 0xffffffff;
        }
    }
}

// FUNCTION: TH07 0x00439401
void ZunTimer::Increment(i32 value)
{
    if ((g_Supervisor.flags >> 5 & 1) != 0)
    {
        ++this->current;
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
    if ((g_Supervisor.flags >> 5 & 1) != 0)
    {
        --this->current;
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
                this->current = this->current - 1;
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

#pragma var_order(local_14, local_18, local_1c, backBuffer, local_24,        \
                  local_28, local_2c, y, x, bytesPerRow, local_40, local_44, \
                  hFile)
// FUNCTION: TH07 0x004395fb
i32 Supervisor::SnapshotScreen(const char *param_1)
{
    HANDLE hFile;
    DWORD local_44;
    D3DLOCKED_RECT local_40;
    i32 bytesPerRow;
    i32 x;
    i32 y;
    u8 *local_2c;
    u8 *local_28;
    i32 local_24;
    IDirect3DSurface8 *backBuffer;
    BITMAPINFO *local_1c;
    void *local_18;
    BITMAPFILEHEADER local_14;

    local_1c = NULL;
    local_18 = NULL;
    backBuffer = NULL;
    this->d3dDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
    memset(&local_14, 0, sizeof(BITMAPFILEHEADER));

    // STRING: TH07 0x00496f98
    local_14.bfType = *(WORD *)&"BM";
    local_14.bfSize = local_14.bfOffBits = 0x36;
    switch (this->presentParameters.BackBufferFormat)
    {
    case D3DFMT_R5G6B5:
        // STRING: TH07 0x00496f80
        g_GameErrorContext.Log("16bit は取り込めない\r\n");
        break;
    case D3DFMT_X8R8G8B8:
        local_1c = (BITMAPINFO *)ZunMemory::Alloc(sizeof(BITMAPINFO));
        if (local_1c == NULL)
        {
            // STRING: TH07 0x00496f60
            g_GameErrorContext.Log("snapShotScreen : 確保しくり\r\n");
        }
        else
        {
            memset(local_1c, 0, sizeof(BITMAPINFO));
            local_24 = 1920;
            local_18 = malloc(local_24 * 480);
            if (local_18 == NULL)
            {
                g_GameErrorContext.Log("snapShotScreen : 確保しくり\r\n");
            }
            else
            {
                local_14.bfSize += local_24 * 0x1e0;
                local_1c->bmiHeader.biBitCount = 0x18;
                local_1c->bmiHeader.biSize = 0x28;
                local_1c->bmiHeader.biWidth = 640;
                local_1c->bmiHeader.biHeight = 480;
                local_1c->bmiHeader.biPlanes = 1;
                local_1c->bmiHeader.biCompression = 0;
                backBuffer->LockRect(&local_40, NULL, 0);
                bytesPerRow = 0;
                for (y = 479; -1 < y; y--, bytesPerRow++)
                {
                    local_2c = (u8 *)((u8 *)local_18 + local_24 * bytesPerRow);
                    local_28 = (u8 *)((u8 *)local_40.pBits + local_40.Pitch * y);
                    for (x = 0; x < 640; x++)
                    {
                        *local_2c = *local_28;
                        local_28++;
                        local_2c++;
                        *local_2c = *local_28;
                        local_28++;
                        local_2c++;
                        *local_2c = *local_28;
                        local_28 += 2;
                        local_2c++;
                    }
                }
                backBuffer->UnlockRect();
                hFile = CreateFileA(param_1, GENERIC_WRITE, 0, NULL, 2, 0x80, NULL);
                if (hFile == INVALID_HANDLE_VALUE)
                {
                    break;
                }

                WriteFile(hFile, &local_14, 0xe, &local_44, NULL);
                WriteFile(hFile, local_1c, 0x28, &local_44, NULL);
                WriteFile(hFile, local_18, local_24 * 0x1e0, &local_44, NULL);
                CloseHandle(hFile);
            }
        }
        break;
    default:
        // STRING: TH07 0x00496f48
        g_GameErrorContext.Log("error ? mother.cpp\r\n");
        return 1;
    }
    SAFE_RELEASE(backBuffer);
    free(local_1c);
    free(local_18);
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
    if (configFile == NULL)
    {
        // STRING: TH07 0x00496f14
        g_GameErrorContext.Log("コンフィグデータが見つからないので初期化しました\r\n");
    init:
        g_Supervisor.cfg.lifeCount = 2;
        g_Supervisor.cfg.bombCount = 3;
        g_Supervisor.cfg.colorMode16bit = 0xff;
        g_Supervisor.cfg.version = 0x70002;
        g_Supervisor.cfg.padAxisX = 600;
        g_Supervisor.cfg.padAxisY = 600;
        bgm2 = CreateFileA("./thbgm.dat", GENERIC_READ, 1, NULL, 3, 0x8000080, NULL);
        if (bgm2 != INVALID_HANDLE_VALUE)
        {
            ReadFile(bgm2, bgm2Data, 0x10, &bytesRead2, NULL);
            CloseHandle(bgm2);
            if (((bgm2Data[0] != 0x5641575a) || (bgm2Data[1] != 1)) ||
                (bgm2Data[2] != 0x700))
            {
                // STRING: TH07 0x00496ee4
                g_GameErrorContext.Fatal("BGM データのバージョンが違います\r\n");
                return ZUN_ERROR;
            }
            g_Supervisor.cfg.musicMode = MUSIC_WAV;
        }
        else
        {
            g_Supervisor.cfg.musicMode = MUSIC_MIDI;
            // STRING: TH07 0x00496ebc
            Supervisor::DebugPrint2("wave データが無いので、midi にします\r\n");
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

        bgm = CreateFileA("./thbgm.dat", GENERIC_READ, 1, NULL, 3, 0x8000080, NULL);
        if (bgm != INVALID_HANDLE_VALUE)
        {
            ReadFile(bgm, bgmData, 0x10, &bytesRead, NULL);
            CloseHandle(bgm);
            if (((bgmData[0] != 0x5641575a) || (bgmData[1] != 1)) ||
                (bgmData[2] != 0x700))
            {
                g_GameErrorContext.Fatal("BGM データのバージョンが違います\r\n");
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
            // STRING: TH07 0x00496e88
            g_GameErrorContext.Log("コンフィグデータが異常でしたので再初期化しました\r\n");
            goto init;
        }
    }
    g_ControllerMapping = g_Supervisor.cfg.controllerMapping;
    g_Supervisor.cfg.opts = g_Supervisor.cfg.opts | 1;
    if ((this->cfg.opts >> 1 & 1) != 0)
    {
        // STRING: TH07 0x00496e64
        g_GameErrorContext.Log("頂点バッファの使用を抑制します\r\n");
    }
    if ((this->cfg.opts >> 10 & 1) != 0)
    {
        // STRING: TH07 0x00496e48
        g_GameErrorContext.Log("フォグの使用を抑制します\r\n");
    }
    if ((this->cfg.opts >> 2 & 1) != 0)
    {
        // STRING: TH07 0x00496e20
        g_GameErrorContext.Log("16Bit のテクスチャの使用を強制します\r\n");
    }
    if ((this->cfg.opts >> 4 & 1) | (this->cfg.opts >> 3 & 1))
    {
        // STRING: TH07 0x00496dfc
        g_GameErrorContext.Log("バックバッファの消去を強制します\r\n");
    }
    if ((this->cfg.opts >> 4 & 1) != 0)
    {
        // STRING: TH07 0x00496dd0
        g_GameErrorContext.Log("ゲーム周りのアイテムの描画を抑制します\r\n");
    }
    if ((this->cfg.opts >> 5 & 1) != 0)
    {
        // STRING: TH07 0x00496da8
        g_GameErrorContext.Log("グーローシェーディングを抑制します\r\n");
    }
    if ((this->cfg.opts >> 6 & 1) != 0)
    {
        // STRING: TH07 0x00496d8c
        g_GameErrorContext.Log("デプステストを抑制します\r\n");
    }
    this->vsyncEnabled = 0;
    this->cfg.opts = this->cfg.opts & 0xffffff7f;
    if ((this->cfg.opts >> 8 & 1) != 0)
    {
        // STRING: TH07 0x00496d6c
        g_GameErrorContext.Log("テクスチャの色合成を抑制しますn");
    }
    if (this->cfg.windowed != 0)
    {
        // STRING: TH07 0x00496d4c
        g_GameErrorContext.Log("ウィンドウモードで起動します\r\n");
    }
    if ((this->cfg.opts >> 9 & 1) != 0)
    {
        // STRING: TH07 0x00496d24
        g_GameErrorContext.Log("リファレンスラスタライザを強制します\r\n");
    }
    if ((this->cfg.opts >> 0xb & 1) != 0)
    {
        // STRING: TH07 0x00496cec
        g_GameErrorContext.Log("パッド、キーボードの入力に DirectInput を使用しません\r\n");
    }
    if ((this->cfg.opts >> 0xc & 1) != 0)
    {
        // STRING: TH07 0x00496cd0
        g_GameErrorContext.Log("画面周りを毎回描画します\r\n");
    }
    if ((this->cfg.opts >> 0xd & 1) != 0)
    {
        // STRING: TH07 0x00496cb0
        g_GameErrorContext.Log("ＢＧＭをメモリに読み込みます\r\n");
    }
    if ((this->cfg.opts >> 0xe & 1) != 0)
    {
        // STRING: TH07 0x00496c98
        g_GameErrorContext.Log("垂直同期を取りません\r\n");
        g_Supervisor.vsyncEnabled = 1;
    }
    if (FileSystem::WriteDataToFile(configFilename, &g_Supervisor.cfg,
                                    sizeof(GameConfiguration)) != 0)
    {
        // STRING: TH07 0x00496c78
        g_GameErrorContext.Fatal("ファイルが書き出せません %s\r\n", configFilename);
        // STRING: TH07 0x00496c20
        g_GameErrorContext.Fatal("フォルダが書込み禁止属性になっているか、ディスクがいっぱいいっぱいになってませんか？\r\n");
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
        if (g_Supervisor.midiOutput != NULL)
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
        if (g_Supervisor.midiOutput != NULL)
        {
            g_Supervisor.midiOutput->PlayLoaded(idx);
        }
        return ZUN_SUCCESS;
    }
    if (g_Supervisor.cfg.musicMode == MUSIC_WAV)
    {
        if ((g_Supervisor.cfg.opts >> 0xd & 1) != 0)
        {
            g_SoundPlayer.PushCommand(AUDIO_SHUTDOWN, 0, "dummy");
        }
        g_SoundPlayer.PushCommand(AUDIO_START, idx, "dummy");
    }
    return ZUN_SUCCESS;
}

#pragma var_order(local_8, local_10c)
// FUNCTION: TH07 0x00439f4d
ZunResult Supervisor::PlayAudio(const char *path)
{
    char local_10c[256];
    char *local_8;

    if (g_Supervisor.cfg.musicMode == MUSIC_MIDI)
    {
        if (g_Supervisor.midiOutput != NULL)
        {
            g_Supervisor.midiOutput->Play(path);
        }
    }
    else
    {
        if (g_Supervisor.cfg.musicMode == MUSIC_WAV)
        {
            strcpy(local_10c, path);
            local_8 = strrchr(local_10c, 0x2e);
            local_8[1] = 'w';
            local_8[2] = 'a';
            local_8[3] = 'v';
            g_SoundPlayer.PushCommand(AUDIO_START, -1, local_10c);
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
        if (g_Supervisor.midiOutput != NULL)
        {
            g_Supervisor.midiOutput->StopPlayback();
        }
    }
    else
    {
        if (g_Supervisor.cfg.musicMode == MUSIC_WAV)
        {
            if ((g_Supervisor.cfg.opts >> 0xd & 1) != 0)
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
    f32 local_8;

    if (g_Supervisor.cfg.musicMode == MUSIC_MIDI)
    {
        if (g_Supervisor.midiOutput != NULL)
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
                local_8 = musicFadeFrames;
            }
            else if (this->effectiveFramerateMultiplier > 1.0f)
            {
                local_8 = musicFadeFrames;
            }
            else
            {
                local_8 = musicFadeFrames / this->effectiveFramerateMultiplier;
            }
            // STRING: TH07 0x00496c1e
            g_SoundPlayer.PushCommand(AUDIO_FADEOUT, local_8, "");
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

// FUNCTION: TH07 0x0043a18d
i32 Supervisor::CanSaveReplay()
{
    return g_GameManager.defaultCfg != NULL &&
           g_GameManager.defaultCfg->slowMode != 0;
}

// FUNCTION: TH07 0x0043a1bd
HRESULT Supervisor::EnableFog()
{
    g_AnmManager->Flush();
    if (this->fogEnabled != 1)
    {
        this->fogEnabled = 1;
        return this->d3dDevice->SetRenderState(D3DRS_FOGENABLE, 1);
    }

    return 0;
}

// FUNCTION: TH07 0x0043a207
HRESULT Supervisor::DisableFog()
{
    g_AnmManager->Flush();
    if (this->fogEnabled != 0)
    {
        this->fogEnabled = 0;
        return this->d3dDevice->SetRenderState(D3DRS_FOGENABLE, 0);
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
    if (time < this->startupTimeForMenuMusic)
    {
        this->startupTimeForMenuMusic = 0;
    }
    timeSinceStartup = time - this->startupTimeForMenuMusic;
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
    this->startupTimeForMenuMusic = time;
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

    if (this->version == NULL)
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
                        if ((local_18 == exeSize) && (local_c == exeChecksum))
                        {
                            return ZUN_SUCCESS;
                        }
                        return ZUN_ERROR;
                    }
                    local_14 = local_8;
                    local_8 = strchr(local_8, 10) + 1;
                    local_10 -= ((u8 *)local_8 - (u8 *)local_14);
                }
                return ZUN_ERROR;
            }
        }
    }
#endif
}

#pragma optimize("s", off)
