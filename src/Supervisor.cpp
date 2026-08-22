#include "Supervisor.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>
#include <chrono>
#include <cstdio>

#include "AnmIdx.hpp"
#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "Ending.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameManager.hpp"
#include "GameWindow.hpp"
#include "MainMenu.hpp"
#include "MidiOutput.hpp"
#include "MusicRoom.hpp"
#include "ResultScreen.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "TextHelper.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"
#include "graphics/ZunGraphics.hpp"
#include "pbg4/Pbg4Archive.hpp"

ControllerMapping g_ControllerMapping = {0, 1, 2, 4, -1, -1, -1, -1, 3};

u16 g_CurFrameRawInput;
u16 g_CurFrameGameInput;
u16 g_LastFrameRawInput;
u16 g_LastFrameGameInput;
u16 g_IsEighthFrameOfHeldInput;
u16 g_NumOfFramesInputsWereHeld;
Supervisor g_Supervisor;
u32 g_FpsUpdateCounter;
char g_ReplayFpsBuffer[256];
char g_FpsCounterBuffer[256];
u32 g_NumFramesSinceLastTime;
u64 g_PerformanceCounter;

void Supervisor::DebugPrint(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, args);
    va_end(args);
}

void Supervisor::CheckTiming()
{
    f64 timeDiff;
    f64 perfDiff;

    if (!this->checkTiming)
    {
        return;
    }

    this->curPerfCounter = SDL_GetPerformanceCounter();

    this->curTime = std::chrono::system_clock::now();

    timeDiff = std::chrono::duration<f64>(this->curTime - this->prevTime).count();
    perfDiff = (f64)(this->curPerfCounter - this->prevPerfCounter) / (f64)this->perfFrequency;

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
            Supervisor::DebugPrint("alq チェック %f / %f = %f\n", timeDiff, perfDiff,
                                   timeDiff / perfDiff);
        }
        else if (this->timingErrorCount != 0)
        {
            this->timingErrorCount--;
        }
        this->prevTime = this->curTime;
        this->prevPerfCounter = this->curPerfCounter;

        this->checkTiming = 0;
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

void AnmManager::ReleaseVertexBuffer()
{
}

u32 Supervisor::OnUpdate(Supervisor *arg)
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
    g_AnmManager->offset.y = 0.0f;
    g_AnmManager->offset.x = 0.0f;
    g_Supervisor.fogEnabled = 255;
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
        g_GameWindow.ResetAccumulator();
        Supervisor::DebugPrint("scene %d -> %d\n", arg->wantedState, arg->curState);
        switch (arg->wantedState)
        {
        case SUPERVISOR_STATE_INIT:
        CASE_SUPERVISOR_STATE_INIT:
            arg->curState = SUPERVISOR_STATE_MAINMENU;
            if (MainMenu::RegisterChain() != ZUN_SUCCESS)
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
                if (MainMenu::RegisterChain() != ZUN_SUCCESS)
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
        g_CurFrameRawInput = g_LastFrameRawInput = g_IsEighthFrameOfHeldInput = 0;
    }
    arg->wantedState = arg->curState;
    arg->calcCount = arg->calcCount + 1;

    if (g_Supervisor.renderSkipFrames != 0)
    {
        g_Supervisor.renderSkipFrames--;
    }

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

u32 Supervisor::OnDraw(Supervisor *arg)
{
    (void)arg;

    DrawFpsCounter(1);
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult Supervisor::SetupInput()
{
    if (this->cfg.disableDinput)
    {
        return ZUN_ERROR;
    }

    if (SDL_Init(SDL_INIT_GAMEPAD))
    {
        i32 numGamepads;
        SDL_JoystickID *gamepads = SDL_GetGamepads(&numGamepads);
        if (gamepads)
        {
            if (numGamepads > 0)
            {
                g_Supervisor.controller = SDL_OpenGamepad(gamepads[0]);
            }
            SDL_free(gamepads);
        }
        if (g_Supervisor.controller)
        {
            g_GameErrorContext.Log("有効なパッドを発見しました\n");
        }
    }

    return ZUN_SUCCESS;
}

ZunResult Supervisor::LoadGameData()
{
    char verFile[128];
    if (g_Pbg4Archive.Load(FileSystem::GetBasePath("th07.dat").c_str()))
    {
        sprintf(verFile, "th07_%.4x%c.ver", 256, 98);
        g_Supervisor.version = (char *)FileSystem::OpenFile(verFile, 0);
        g_Supervisor.versionTableSize = g_LastFileSize;
        if (!g_Supervisor.version)
        {
            g_GameErrorContext.Fatal("error : データのバージョンが違います\n");
            return ZUN_ERROR;
        }
    }
    else
    {
        g_GameErrorContext.Fatal("error : データファイルが存在しません\n");
        return ZUN_ERROR;
    }
    return ZUN_SUCCESS;
}

i32 Supervisor::CheckVSync()
{
#ifdef __EMSCRIPTEN__
    return 0;
#endif

    // the previous way of doing this does not work at all with the gles renderer, since swapbuffers
    // would just return immediately. invariably this meant that every system (well, besides
    // emscripten) would have software vsync on regardless of if the display was 60 hz. so instead
    // we just query the refresh rate and go off of that.
    //
    // there's a delay of 3 seconds since thats about how long the previous way took to complete i
    // think, and also to keep the logo displayed for a bit. you can skip it though

    u64 start = SDL_GetTicks();
    while (SDL_GetTicks() - start < 3000)
    {
        SDL_Event event;
        bool skip = false;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                SDL_PushEvent(&event);
                skip = true;
            }

            if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ||
                event.type == SDL_EVENT_FINGER_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                skip = true;
            }
        }
        if (skip)
        {
            break;
        }

        g_Supervisor.gfxDevice->BeginFrame();
        g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
        g_Supervisor.gfxDevice->EndFrame();
        g_Supervisor.gfxDevice->SwapBuffers();
        SDL_Delay(16);
    }

#if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IPHONE)
    return 0;
#endif

    if (g_GameWindow.window)
    {
        i32 swapInterval = 0;
        SDL_GL_GetSwapInterval(&swapInterval);

        // we have hardware vsync, so we can probably be reasonably sure that the game won't run at
        // a ridiculously high framerate
        if (swapInterval != 0)
        {
            g_Supervisor.vsyncEnabled = 0;
            return 0;
        }
    }

    g_GameErrorContext.Log("垂直同期が取れてないか、リフレッシュレートが高すぎます\n");
    g_GameErrorContext.Log("強制６０フレームモードで動作します\n");
    g_Supervisor.vsyncEnabled = 1;
    return 0;
}

ZunResult Supervisor::AddedCallback(Supervisor *arg)
{
    ScoreDat *scoreDat;
    i32 i;

    arg->perfFrequency = SDL_GetPerformanceFrequency();
    g_Supervisor.gfxDevice->BeginFrame();
    g_Supervisor.gfxDevice->SetClearColor({0xff000000});
    g_Supervisor.gfxDevice->Clear(CLEAR_COLOR_BUFFER);
    g_Supervisor.gfxDevice->EndFrame();
    g_Supervisor.gfxDevice->SwapBuffers();
    if (LoadGameData() != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }
    g_AnmManager->LoadSurface(0, "data/title/th07logo.jpg");
    g_Supervisor.isInEnding = 1;
    if (!g_Supervisor.vsyncEnabled)
    {
        CheckVSync();
    }
    else
    {
        i = 0;
        while (i < 4)
        {
            g_Supervisor.gfxDevice->BeginFrame();
            g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
            g_Supervisor.gfxDevice->EndFrame();
            g_Supervisor.gfxDevice->SwapBuffers();
            i++;
        }
    }
    g_AnmManager->ReleaseSurface(0);
    arg->isInEnding = 0;
    arg->renderSkipFrames = 0;
    arg->lastTotalPlayTimeUpdate = SDL_GetTicks();
    g_Rng.SetSeed(arg->lastTotalPlayTimeUpdate);
    arg->SetupInput();
    if (!arg->midiOutput)
    {
        arg->midiOutput = new MidiOutput;
    }
    if (arg->midiOutput)
    {
        arg->midiOutput->ReadFileData(30, "bgm/init.mid");
    }
    g_SoundPlayer.InitSoundBuffers();
    if (g_AnmManager->LoadAnms(ANM_FILE_TEXT, "data/text.anm", ANM_OFFSET_TEXT) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    if (AsciiManager::RegisterChain() != ZUN_SUCCESS)
    {
        g_GameErrorContext.Log("error : 文字の初期化に失敗しました\n");
        return ZUN_ERROR;
    }

    g_AnmManager->SetupVertexBuffer();
    if (TextHelper::CreateTextBuffer() != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }
    if (g_SoundPlayer.LoadFmt("bgm/thbgm.fmt"))
    {
        g_GameErrorContext.Log("error : BGM の初期化に失敗しました\n");
        return ZUN_ERROR;
    }

    if (g_SoundPlayer.bgmSeekOffset == 0)
    {
        if (!g_Supervisor.cfg.preloadBgm)
        {
            g_SoundPlayer.StartBGM("thbgm.dat");
        }
        else
        {
            SDL_strlcpy(g_SoundPlayer.bgmArchivePath, FileSystem::GetBasePath("thbgm.dat").c_str(),
                        sizeof(g_SoundPlayer.bgmArchivePath));
        }
    }
    else if (!g_Supervisor.cfg.preloadBgm)
    {
        g_SoundPlayer.StartBGM("th07.dat");
    }
    else
    {
        SDL_strlcpy(g_SoundPlayer.bgmArchivePath, FileSystem::GetBasePath("th07.dat").c_str(),
                    sizeof(g_SoundPlayer.bgmArchivePath));
    }
    scoreDat = ResultScreen::OpenScore(FileSystem::GetPrefPath("score.dat").c_str());
    memset(&g_GameManager.plst, 0, sizeof(g_GameManager.plst));
    g_GameManager.plst.base.th7kLen2 = g_GameManager.plst.base.th7kLen = sizeof(Plst);
    g_GameManager.plst.base.magic = PLST_MAGIC;
    g_GameManager.plst.base.version = 1;
    ResultScreen::ParsePlst(scoreDat, &g_GameManager.plst);
    ResultScreen::ReleaseScoreDat(scoreDat);
    g_Supervisor.midiTimer = new DummyMidiTimer;
    if (g_Supervisor.midiTimer)
    {
        g_Supervisor.midiTimer->StartTimerDefault();
    }
    return ZUN_SUCCESS;
}

ZunResult Supervisor::DeletedCallback(Supervisor *arg)
{
    SAFE_FREE(g_Supervisor.version);
    g_AnmManager->ReleaseVertexBuffer();
    g_AnmManager->ReleaseAnm(ANM_FILE_TEXT);
    AsciiManager::CutChain();
    g_SoundPlayer.PushCommand(AUDIO_SHUTDOWN, 0, "dummy");
    if (arg->midiOutput)
    {
        arg->midiOutput->StopPlayback();
        delete arg->midiOutput;
        arg->midiOutput = NULL;
    }
    ReplayManager::SaveReplay(NULL, NULL);
    TextHelper::ReleaseTextBuffer();
    if (arg->controller)
    {
        SDL_CloseGamepad(arg->controller);
    }
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

void Supervisor::DrawFpsCounter(i32 param_1)
{
    ZunVec3 replayFpsCounterPos;
    ZunVec3 fpsCounterPos;
    u64 curPerfCounter;
    f32 targetFps;
    u64 curTime;
    f32 elapsedTimeInSecs;
    f32 fps;

    if (!g_GameManager.slowModeSlowActive)
    {
        g_NumFramesSinceLastTime += 1 + (u32)g_Supervisor.cfg.frameskipConfig;

        if (g_Supervisor.perfFrequency == 0)
        {
            static u64 g_LastTime = SDL_GetTicks();

            curTime = SDL_GetTicks();
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
                        sprintf(g_ReplayFpsBuffer, "%2d", (i32)g_Supervisor.curFps);
                    }
                }
            }
            goto LAB_00439350;
        }

        if (g_PerformanceCounter == 0)
        {
            g_PerformanceCounter = SDL_GetPerformanceCounter();
        }
        curPerfCounter = SDL_GetPerformanceCounter();
        if (curPerfCounter < g_PerformanceCounter)
        {
            g_PerformanceCounter = curPerfCounter;
            g_NumFramesSinceLastTime = 0;
        }
        if (curPerfCounter - g_PerformanceCounter >= g_Supervisor.perfFrequency / 2)
        {
            elapsedTimeInSecs =
                (f32)(curPerfCounter - g_PerformanceCounter) / (f32)g_Supervisor.perfFrequency;
            g_PerformanceCounter = curPerfCounter;
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
        if (g_GameManager.replay && g_GameManager.notInMenu)
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
            this->subFrame =
                (f32)value * g_Supervisor.effectiveFramerateMultiplier + this->subFrame;
            while (this->subFrame >= 1.0f)
            {
                this->current = this->current + 1;
                this->subFrame = this->subFrame - 1.0f;
            }
        }
    }
}

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
            this->subFrame =
                this->subFrame - (f32)value * g_Supervisor.effectiveFramerateMultiplier;
            while (this->subFrame < 0.0f)
            {
                this->current--;
                this->subFrame = this->subFrame + 1.0f;
            }
        }
    }
}

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
i32 Supervisor::SnapshotScreen(const char *param_1)
{
    u8 *pixels = new u8[640 * 480 * 4];
    this->gfxDevice->ReadPixels(0, 0, 640, 480, pixels);

    SDL_Surface *surf = SDL_CreateSurfaceFrom(640, 480, SDL_PIXELFORMAT_RGBA32, pixels, 640 * 4);

    SDL_SaveBMP(surf, FileSystem::GetPrefPath(param_1).c_str());
    SDL_DestroySurface(surf);
    delete[] pixels;
    return 0;
}

ZunResult Supervisor::LoadConfig(const char *configFilename)
{
    i32 bgmData[4];
    SDL_IOStream *bgm;
    i32 bgm2Data[4];
    SDL_IOStream *bgm2;
    u32 *configFile;

    memset(&g_Supervisor.cfg, 0, sizeof(GameConfiguration));
    configFile = (u32 *)FileSystem::OpenFile(FileSystem::GetPrefPath(configFilename).c_str(), 1);
    if (!configFile)
    {
        g_GameErrorContext.Log("コンフィグデータが見つからないので初期化しました\n");
    init:
        g_Supervisor.cfg.lifeCount = 2;
        g_Supervisor.cfg.bombCount = 3;
        g_Supervisor.cfg.colorMode16bit = 255;
        g_Supervisor.cfg.version = 0x70002;
        g_Supervisor.cfg.padAxisX = 600;
        g_Supervisor.cfg.padAxisY = 600;
        bgm2 = SDL_IOFromFile(FileSystem::GetBasePath("thbgm.dat").c_str(), "rb");
        if (bgm2)
        {
            SDL_ReadIO(bgm2, bgm2Data, 16);
            SDL_CloseIO(bgm2);
            if (bgm2Data[0] != 0x5641575a || bgm2Data[1] != 1 || bgm2Data[2] != 0x700)
            {
                g_GameErrorContext.Fatal("BGM データのバージョンが違います\n");
                return ZUN_ERROR;
            }
            g_Supervisor.cfg.musicMode = MUSIC_WAV;
        }
        else
        {
            g_Supervisor.cfg.musicMode = MUSIC_MIDI;
            Supervisor::DebugPrint("wave データが無いので、midi にします\n");
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

        bgm = SDL_IOFromFile(FileSystem::GetBasePath("thbgm.dat").c_str(), "rb");
        if (bgm)
        {
            SDL_ReadIO(bgm, bgmData, 16);
            SDL_CloseIO(bgm);
            if (bgmData[0] != 0x5641575a || bgmData[1] != 1 || bgmData[2] != 0x700)
            {
                g_GameErrorContext.Fatal("BGM データのバージョンが違います\n");
                return ZUN_ERROR;
            }
        }
        if (!(g_Supervisor.cfg.lifeCount < 5 && g_Supervisor.cfg.bombCount < 4 &&
              g_Supervisor.cfg.colorMode16bit < 2 && g_Supervisor.cfg.musicMode < 3 &&
              g_Supervisor.cfg.defaultDifficulty < 6 && g_Supervisor.cfg.playSounds < 2 &&
              g_Supervisor.cfg.windowed < 2 && g_Supervisor.cfg.frameskipConfig < 3 &&
              g_Supervisor.cfg.effectQuality < 3 && g_Supervisor.cfg.slowMode < 2 &&
              g_Supervisor.cfg.shotSlow < 2 && g_Supervisor.cfg.version == 0x70002 &&
              g_LastFileSize == sizeof(GameConfiguration)))
        {
            g_GameErrorContext.Log("コンフィグデータが異常でしたので再初期化しました\n");
            goto init;
        }
        g_ControllerMapping = g_Supervisor.cfg.controllerMapping;
    }
    g_Supervisor.cfg.loaded = 1;
    if (this->cfg.noVertexBuffers)
    {
        g_GameErrorContext.Log("頂点バッファの使用を抑制します\n");
    }
    if (this->cfg.disableFog)
    {
        g_GameErrorContext.Log("フォグの使用を抑制します\n");
    }
    if (this->cfg.use16BitTextures)
    {
        g_GameErrorContext.Log("16Bit のテクスチャの使用を強制します\n");
    }
    if (this->IsClearingBackbuffer())
    {
        g_GameErrorContext.Log("バックバッファの消去を強制します\n");
    }
    if (this->cfg.disableItemDrawAroundPlayfield)
    {
        g_GameErrorContext.Log("ゲーム周りのアイテムの描画を抑制します\n");
    }
    if (this->cfg.disableGouraud)
    {
        g_GameErrorContext.Log("グーローシェーディングを抑制します\n");
    }
    if (this->cfg.disableZBuffer)
    {
        g_GameErrorContext.Log("デプステストを抑制します\n");
    }
    this->vsyncEnabled = 0;
    this->cfg.unused = 0;
    if (this->cfg.disableTextureBlend)
    {
        g_GameErrorContext.Log("テクスチャの色合成を抑制しますn");
    }
    if (this->cfg.windowed)
    {
        g_GameErrorContext.Log("ウィンドウモードで起動します\n");
    }
    if (this->cfg.forceReferenceRender)
    {
        g_GameErrorContext.Log("リファレンスラスタライザを強制します\n");
    }
    if (this->cfg.disableDinput)
    {
        g_GameErrorContext.Log("パッド、キーボードの入力に DirectInput を使用しません\n");
    }

    this->cfg.redrawEveryFrame = 1;
    if (this->cfg.redrawEveryFrame)
    {
        g_GameErrorContext.Log("画面周りを毎回描画します\n");
    }
    if (this->cfg.preloadBgm)
    {
        g_GameErrorContext.Log("ＢＧＭをメモリに読み込みます\n");
    }
    if (this->cfg.enableVsync)
    {
        g_GameErrorContext.Log("垂直同期を取りません\n");
        g_Supervisor.vsyncEnabled = 1;
    }
    if (FileSystem::WriteDataToFile(configFilename, &g_Supervisor.cfg, sizeof(GameConfiguration)))
    {
        g_GameErrorContext.Fatal("ファイルが書き出せません %s\n", configFilename);
        g_GameErrorContext.Fatal("フォルダが書込み禁止属性になっているか、ディスクがいっぱいいっぱ"
                                 "いになってませんか？\n");
        return ZUN_ERROR;
    }

    return ZUN_SUCCESS;
}

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

            pathext = strrchr(pathbuf, '.');
            if (!pathext)
            {
                return 1;
            }

            pathext[1] = 'w';
            pathext[2] = 'a';
            pathext[3] = 'v';
            g_SoundPlayer.PushCommand(AUDIO_PRELOAD, idx, pathbuf);
        }
        return 1;
    }
}

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
            strcpy(pathBuf, path);
            pathExt = strrchr(pathExt, '.');
            if (!pathExt)
            {
                return ZUN_ERROR;
            }

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
            g_SoundPlayer.PushCommand(AUDIO_FADEOUT, effectiveFadeFrames, "");
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

i32 Supervisor::IsSlowMode()
{
    return g_GameManager.defaultCfg != NULL && g_GameManager.defaultCfg->slowMode;
}

i32 Supervisor::EnableFog()
{
    g_AnmManager->Flush();
    if (this->fogEnabled != 1)
    {
        this->fogEnabled = 1;
        g_Supervisor.gfxDevice->Enable(CAPS_FOG);
        return 1;
    }

    return 0;
}

i32 Supervisor::DisableFog()
{
    g_AnmManager->Flush();
    if (this->fogEnabled)
    {
        this->fogEnabled = 0;
        g_Supervisor.gfxDevice->Disable(CAPS_FOG);
        return 1;
    }

    return 0;
}

void Supervisor::UpdateStartupTime()
{
    u32 timeSinceStartup;
    u64 time;

    time = SDL_GetTicks();
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
        g_GameManager.plst.totalSeconds += g_GameManager.plst.totalMilliseconds / 1000;
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

void Supervisor::UpdateTime()
{
    u32 timeSinceLastTime;
    u64 time;

    time = SDL_GetTicks();
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
        g_GameManager.plst.gameSeconds += g_GameManager.plst.gameMilliseconds / 1000;
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
