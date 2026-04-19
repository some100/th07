#include <stdio.h>
#include <windows.h>
#include <winnls32.h>

// pull in gameerrorcontext::flush before anmmanager::releasesurfaces
#include "AnmManager.hpp"
#include "BulletManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameManager.hpp"
#include "GameWindow.hpp"
#include "ResultScreen.hpp"
#include "SoundPlayer.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"

// FUNCTION: TH07 0x00433f90
void AnmManager::TakeScreenshotIfRequested()
{
    if (this->screenshotTextureId >= 0)
    {
        TakeScreenshot(this->screenshotTextureId, this->screenshotSrcLeft,
                       this->screenshotSrcTop, this->screenshotSrcWidth,
                       this->screenshotSrcHeight, this->screenshotDstLeft,
                       this->screenshotDstTop, this->screenshotDstWidth,
                       this->screenshotDstHeight);
        this->screenshotTextureId = -1;
    }
}

#pragma var_order(d3dDeviceStatus, msg, res, i)
// FUNCTION: TH07 0x00434020
i32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdline, i32 nCmdShow)
{
    HRESULT d3dDeviceStatus;
    i32 res;
    tagMSG msg;

    res = RENDER_RESULT_KEEP_RUNNING;
    g_Supervisor.hInstance = hInstance;
    SystemParametersInfoA(SPI_GETSCREENSAVEACTIVE, 0,
                          &g_GameWindow.screen_save_active, 0);
    SystemParametersInfoA(SPI_GETLOWPOWERACTIVE, 0,
                          &g_GameWindow.low_power_active, 0);
    SystemParametersInfoA(SPI_GETPOWEROFFACTIVE, 0,
                          &g_GameWindow.power_off_active, 0);
    SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, 0, NULL, 2);
    SystemParametersInfoA(SPI_SETLOWPOWERACTIVE, 0, NULL, 2);
    SystemParametersInfoA(SPI_SETPOWEROFFACTIVE, 0, NULL, 2);
    if (GameWindow::CheckForRunningGameInstance(hInstance) == ZUN_ERROR)
        goto stop;

    // STRING: TH07 0x00497c60
    if (g_Supervisor.LoadConfig("th07.cfg") != ZUN_SUCCESS)
        goto stop;

    GameWindow::ChecksumExecutable();
    QueryPerformanceFrequency(&g_GameWindow.lpFrequency);

start:
    if (GameWindow::InitD3dInterface())
    {
        goto stop;
    }

    if (GameWindow::CreateGameWindow(hInstance))
    {
        goto stop;
    }

    if (GameWindow::InitD3dRendering())
    {
        goto stop;
    }

    g_SoundPlayer.InitializeDSound(g_GameWindow.window);
    Controller::GetJoystickCaps();
    Controller::ResetKeyboard();
    g_AnmManager = new AnmManager();
    if (g_Supervisor.cfg.windowed == 0)
    {
        WINNLSEnableIME(0, 0);
        ShowCursor(0);
    }
    res = g_Supervisor.RegisterChain();
    if (res != ZUN_SUCCESS)
    {
        if (res == ZUN_ERROR)
        {
            goto cleanup;
        }
        res = RENDER_RESULT_EXIT_ERROR;
        goto cleanup;
    }
    res = RENDER_RESULT_KEEP_RUNNING;
    g_GameWindow.curFrame = 0xe2;
    while (g_GameWindow.isAppClosing == 0)
    {
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE) != 0)
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        else
        {
            d3dDeviceStatus = g_Supervisor.d3dDevice->TestCooperativeLevel();
            if (d3dDeviceStatus == D3D_OK)
            {
                res = g_GameWindow.Render();
                if (res != RENDER_RESULT_KEEP_RUNNING)
                    break;
                g_Supervisor.flags = g_Supervisor.flags & 0xffffffef;
            }
            else if (d3dDeviceStatus == D3DERR_DEVICENOTRESET)
            {
                g_AnmManager->ReleaseSurfaces();
                if (g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters) !=
                    0)
                    break;
                GameWindow::ResetRenderState();
                g_Supervisor.renderSkipFrames = 3;
                g_Supervisor.flags = g_Supervisor.flags | 0x10;
            }
        }
    }
cleanup:
    if (g_GameManager.plst.magic != 0)
    {
        ResultScreen::RegisterChain(2);
    }
    g_Chain.Release();
    while (g_SoundPlayer.ProcessQueues() != 0)
        ;

stop:
    g_SoundPlayer.Release();
    delete g_AnmManager;
    g_AnmManager = NULL;

    if (g_Supervisor.d3dDevice != NULL)
    {
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
    }
    SAFE_RELEASE(g_Supervisor.d3dDevice);
    SAFE_RELEASE(g_Supervisor.d3dIface);
    if (g_GameWindow.window != NULL)
    {
        ShowWindow(g_GameWindow.window, 0);
        MoveWindow(g_GameWindow.window, 0, 0, 0, 0, 0);
        DestroyWindow(g_GameWindow.window);
        g_GameWindow.window = NULL;
    }
    ShowCursor(1);
    if (res == RENDER_RESULT_EXIT_ERROR)
    {
        g_GameErrorContext.m_BufferEnd = g_GameErrorContext.m_Buffer;
        *g_GameErrorContext.m_BufferEnd = NULL;
        // STRING: TH07 0x00497c28
        g_GameErrorContext.Log("再起動を要するオプションが変更されたので再起動します\r\n");
        if (g_Supervisor.cfg.windowed == 0)
        {
            WINNLSEnableIME(0, 1);
        }
        i32 i = 0;
        while (i < 60)
        {
            if (PeekMessageA(&msg, NULL, 0, 0, 1) != 0)
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
            i++;
        }
        goto start;
    }
    FileSystem::WriteDataToFile("th07.cfg", &g_Supervisor.cfg,
                                sizeof(GameConfiguration));
    SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE,
                          g_GameWindow.screen_save_active, NULL, 2);
    SystemParametersInfoA(SPI_SETLOWPOWERACTIVE, g_GameWindow.low_power_active,
                          NULL, 2);
    SystemParametersInfoA(SPI_SETPOWEROFFACTIVE, g_GameWindow.power_off_active,
                          NULL, 2);
    WINNLSEnableIME(0, 1);
    g_GameErrorContext.Flush();
    return 0;
}
