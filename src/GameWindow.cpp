#include "GameWindow.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <cmath>
#include <cstdio>
#include <filesystem>

#include "AnmManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "ScreenEffect.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "graphics/Gles.hpp"
#include "graphics/ZunGraphics.hpp"

GameWindow g_GameWindow;
i32 g_FrameCount;
f64 g_LastFrameTime;
u64 g_LastPerfCounter;

static GfxInit g_RenderingBackends[] = {
    GlesGraphics::Init,
};

void GameWindow::Present()
{
    char snapshotPath[252];
    i32 i;

    g_Supervisor.gfxDevice->SwapBuffers();

    g_AnmManager->TakeScreenshotIfRequested();
    if (WAS_PRESSED_RAW(TH_BUTTON_HOME))
    {
        std::filesystem::create_directory("snapshot");
        for (i = 0; i < 1000; i++)
        {
            sprintf(snapshotPath, "snapshot/th%.3d.bmp", i);
            if (FileSystem::CheckFileExists(snapshotPath) == 0)
            {
                break;
            }
        }
        if (i < 1000)
        {
            g_Supervisor.TakeSnapshot(snapshotPath);
        }
    }
    if (g_Supervisor.renderSkipFrames != 0)
    {
        g_Supervisor.renderSkipFrames--;
    }
}

RenderResult GameWindow::Render()
{
    f64 perfDiff;
    u64 perfCounter;
    i32 chainRes;

    if (!this->isAppActive)
    {
        return RENDER_RESULT_KEEP_RUNNING;
    }

    if (this->curFrame == 0)
    {
    begin_loop:
        if ((i32)g_Supervisor.cfg.frameskipConfig <= (i32)this->curFrame)
        {
            g_AnmManager->ResetVertexBuffer();
            g_Supervisor.fogEnabled = 255;
            g_Supervisor.DisableFog();
            g_Chain.RunDrawChain();
            g_AnmManager->Flush();
            g_Supervisor.gfxDevice->BindTexture({0});
        }

        g_AnmManager->Flush();
        g_Supervisor.viewport.x = 0;
        g_Supervisor.viewport.y = 0;
        g_Supervisor.viewport.width = GAME_WINDOW_WIDTH;
        g_Supervisor.viewport.height = GAME_WINDOW_HEIGHT;
        g_Supervisor.gfxDevice->SetViewport(g_Supervisor.viewport);

        chainRes = g_Chain.RunCalcChain();
        g_SoundPlayer.ProcessQueues();

        if (!chainRes)
        {
            return RENDER_RESULT_EXIT_SUCCESS;
        }
        if (chainRes == -1)
        {
            return RENDER_RESULT_EXIT_ERROR;
        }

        this->curFrame++;
    }

    if (g_Supervisor.VsyncDisabled())
    {
        if (this->curFrame != 0)
        {
            perfCounter = SDL_GetPerformanceCounter();
            perfDiff = (f64)(perfCounter - g_LastPerfCounter) / (f64)g_GameWindow.frequency;

            if (perfDiff < 0.0)
            {
                g_LastPerfCounter = perfCounter;
            }

            if (perfDiff >= (1.0 / 60.0) || g_GameWindow.usesRelativePath)
            {
                u64 frameTicks = g_GameWindow.frequency / 60.0;

                while (perfDiff >= (1.0 / 60.0))
                {
                    g_LastPerfCounter += frameTicks;
                    perfDiff -= (1.0 / 60.0);
                }

                if ((i32)g_Supervisor.cfg.frameskipConfig < (i32)this->curFrame)
                {
                    goto LAB_00434a18;
                }

                goto begin_loop;
            }
        }
    }

    if (!g_Supervisor.VsyncDisabled())
    {
        if ((i32)g_Supervisor.cfg.frameskipConfig >= (i32)this->curFrame)
        {
            Present();
            goto begin_loop;
        }

    LAB_00434a18:
        Present();
        this->curFrame = 0;
        g_FrameCount++;
    }

    return RENDER_RESULT_KEEP_RUNNING;
}

ZunResult GameWindow::InitInterface()
{
    for (auto gfxInit : g_RenderingBackends)
    {
        g_Supervisor.gfxDevice = gfxInit();
        if (g_Supervisor.gfxDevice)
        {
            g_Supervisor.hasLockableBackbuffer = 1;
            g_Supervisor.lockableBackBuffer = 1;
            return ZUN_SUCCESS;
        }
    }

    g_GameErrorContext.Fatal("Direct3D オブジェクトは何故か作成出来なかった\n");
    return ZUN_ERROR;
}

ZunResult GameWindow::CreateGameWindow()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        g_GameErrorContext.Fatal("Direct3D オブジェクトは何故か作成出来なかった\n");
        return ZUN_ERROR;
    }

    u32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
    if (!g_Supervisor.cfg.windowed)
    {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    g_GameWindow.isAppActive = 1;
    g_GameWindow.isAppInactive = 0;
    g_LastPerfCounter = SDL_GetPerformanceCounter();

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    g_GameWindow.window = SDL_CreateWindow("東方妖々夢　〜 Perfect Cherry Blossom. ver 1.00b",
                                           SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                           GAME_WINDOW_WIDTH, GAME_WINDOW_HEIGHT, flags);
    if (!g_GameWindow.window)
    {
        Supervisor::DebugPrint("sdl window create failed: %s\n", SDL_GetError());
        return ZUN_ERROR;
    }

    SDL_RaiseWindow(g_GameWindow.window);
    return ZUN_SUCCESS;
}

ZunResult GameWindow::InitRendering()
{
    ZunVec3 pEye;
    ZunVec3 pAt;
    ZunVec3 pUp;
    f32 fov;
    f32 aspectRatio;
    f32 halfWidth;
    f32 halfHeight;
    f32 halfCameraDistance;

    halfWidth = 320.0f;
    halfHeight = 240.0f;
    aspectRatio = 4.0f / 3.0f;
    fov = ZUN_PI / 6.0f;
    halfCameraDistance = halfHeight / tanf(fov / 2.0f);
    pUp.x = 0.0f;
    pUp.y = 1.0f;
    pUp.z = 0.0f;
    pAt.x = halfWidth;
    pAt.y = -halfHeight;
    pAt.z = 0.0f;
    pEye.x = halfWidth;
    pEye.y = -halfHeight;
    pEye.z = -halfCameraDistance;
    g_Supervisor.viewMatrix.LookAtLH(&pEye, &pAt, &pUp);
    g_Supervisor.projectionMatrix.PerspectiveFovLH(fov, aspectRatio, 100.0f, 10000.0f);

    g_Supervisor.gfxDevice->SetTransformMatrix(MATRIX_VIEW, g_Supervisor.viewMatrix);
    g_Supervisor.gfxDevice->SetTransformMatrix(MATRIX_PROJECTION, g_Supervisor.projectionMatrix);

    g_Supervisor.viewport.x = 0;
    g_Supervisor.viewport.y = 0;
    g_Supervisor.viewport.width = GAME_WINDOW_WIDTH;
    g_Supervisor.viewport.height = GAME_WINDOW_HEIGHT;
    g_Supervisor.viewport.minZ = 0.0f;
    g_Supervisor.viewport.maxZ = 1.0f;
    g_Supervisor.gfxDevice->SetViewport(g_Supervisor.viewport);

    ResetRenderState();
    ScreenEffect::SetViewport(0xff000000);
    g_GameWindow.isAppClosing = 0;
    g_Supervisor.lastFrameTime = 0;
    g_Supervisor.cfg.colorMode16bit = 0;

    return ZUN_SUCCESS;
}

void GameWindow::ResetRenderState()
{
    ZunColor fogColor;

    if (!g_Supervisor.cfg.disableZBuffer)
    {
        g_Supervisor.gfxDevice->Enable(CAPS_DEPTH_TEST);
    }
    else
    {
        g_Supervisor.gfxDevice->Disable(CAPS_DEPTH_TEST);
    }

    g_Supervisor.gfxDevice->Enable(CAPS_BLEND);
    g_Supervisor.gfxDevice->SetBlendMode(BLEND_ALPHA, BLEND_ALPHA);
    g_Supervisor.gfxDevice->SetDepthFunc(DEPTH_FUNC_ALWAYS);
    g_Supervisor.gfxDevice->Enable(CAPS_ALPHA_TEST);
    g_Supervisor.gfxDevice->SetAlphaTestRef(4);

    if (!g_Supervisor.cfg.disableFog)
    {
        g_Supervisor.gfxDevice->Enable(CAPS_FOG);
    }
    else
    {
        g_Supervisor.gfxDevice->Disable(CAPS_FOG);
    }

    fogColor.color = 0xffa0a0a0;
    g_Supervisor.gfxDevice->SetFogColor(fogColor);
    g_Supervisor.gfxDevice->SetFogRange(1000.0f, 5000.0f);

    g_Supervisor.gfxDevice->SetTextureFilter();
    if (g_AnmManager)
    {
        g_AnmManager->SetBlendMode(255);
        g_AnmManager->SetColorOp(255);
        g_AnmManager->SetVertexShader(255);
        g_AnmManager->SetTexture(0);
        g_AnmManager->SetCameraMode(255);
    }
    g_Stage.renderStateWasReset = 1;
}

i32 GameWindow::ChecksumExecutable()
{
    // the game uses exechecksum and exesize to write to replay and score files about the program
    // that produced that file, and in the original executable those are compared to values in the
    // verfile to check if they're "good" untampered files. obviously it's not gonna match, so we
    // just return these hardcoded values.
    g_Supervisor.exeSize = 650752;
    return g_Supervisor.exeChecksum = 0xaec5445c;
}
