#include <direct.h>
#include <shlobj.h>
#include <stdio.h>
#include <windows.h>
#include <winnls32.h>

// clang-format keeps on ordering gameerrorcontext below anmmanager
// clang-format off
#include "GameErrorContext.hpp"
#include "AnmManager.hpp"
#include "BulletManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameManager.hpp"
#include "ResultScreen.hpp"
#include "ScreenEffect.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"
#include "i18n.hpp"
// clang-format on

enum RenderResult
{
    RENDER_RESULT_EXIT_SUCCESS_2 = -1,
    RENDER_RESULT_KEEP_RUNNING = 0,
    RENDER_RESULT_EXIT_SUCCESS = 1,
    RENDER_RESULT_EXIT_ERROR = 2
};

#pragma pack(4)
struct GameWindow
{
    static ZunResult CheckForRunningGameInstance(HINSTANCE hInstance);
    static i32 ChecksumExecutable();
    static i32 CreateGameWindow(HINSTANCE hInstance);
    static char *FormatCapability(const char *capabilityName, u32 capabilityFlags,
                                  u32 mask, char *buf);
    static void FormatD3DCapabilities(D3DCAPS8 *caps, char *buf);
    static i32 InitD3dInterface();
    static i32 InitD3dRendering();
    static void Present();
    RenderResult Render();
    static void ResetRenderState();
    static i32 ResolveIt(const char *shortcutPath, char *dstPath, i32 maxPathLen);
    static void SetWindowActive(HWND window);
    static LRESULT __stdcall WindowProc(HWND hWnd, u32 uMsg, WPARAM wParam,
                                        LPARAM lParam);

    HWND window;
    i32 isAppClosing;
    i32 isAppActive;
    i32 isAppInactive;
    i8 curFrame;
    // pad 3
    LARGE_INTEGER lpFrequency;
    bool usesRelativePath;
    // pad 3
    u32 screenSaveActive;
    u32 lowPowerActive;
    u32 powerOffActive;
};
C_ASSERT(sizeof(GameWindow) == 0x2c);

// GLOBAL: TH07 0x00575c20
GameWindow g_GameWindow;

// GLOBAL: TH07 0x0135e1f4
HANDLE g_Mutex;

// GLOBAL: TH07 0x0135e1f8
i32 g_FrameCount;

// GLOBAL: TH07 0x0135e200
f64 g_LastFrameTime;

// GLOBAL: TH07 0x0135e208
LARGE_INTEGER g_LastPerfCounter;

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
                          &g_GameWindow.screenSaveActive, 0);
    SystemParametersInfoA(SPI_GETLOWPOWERACTIVE, 0,
                          &g_GameWindow.lowPowerActive, 0);
    SystemParametersInfoA(SPI_GETPOWEROFFACTIVE, 0,
                          &g_GameWindow.powerOffActive, 0);
    SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, 0, NULL, 2);
    SystemParametersInfoA(SPI_SETLOWPOWERACTIVE, 0, NULL, 2);
    SystemParametersInfoA(SPI_SETPOWEROFFACTIVE, 0, NULL, 2);
    if (GameWindow::CheckForRunningGameInstance(hInstance) == ZUN_ERROR)
    {
        goto stop;
    }

    // STRING: TH07 0x00497c60
    if (g_Supervisor.LoadConfig("th07.cfg") != ZUN_SUCCESS)
    {
        goto stop;
    }

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
    if (!g_Supervisor.cfg.windowed)
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
    g_GameWindow.curFrame = -30;
    while (!g_GameWindow.isAppClosing)
    {
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
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
                {
                    break;
                }
                g_Supervisor.deviceNotReset = 0;
            }
            else if (d3dDeviceStatus == D3DERR_DEVICENOTRESET)
            {
                g_AnmManager->ReleaseSurfaces();
                if (g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters) !=
                    0)
                {
                    break;
                }
                GameWindow::ResetRenderState();
                g_Supervisor.renderSkipFrames = 3;
                g_Supervisor.deviceNotReset = 1;
            }
        }
    }
cleanup:
    if (g_GameManager.plst.magic != 0)
    {
        ResultScreen::RegisterChain(2);
    }
    g_Chain.Release();
    while (g_SoundPlayer.ProcessQueues())
        ;

stop:
    g_SoundPlayer.Release();
    delete g_AnmManager;
    g_AnmManager = NULL;

    if (g_Supervisor.d3dDevice)
    {
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
    }
    SAFE_RELEASE(g_Supervisor.d3dDevice);
    SAFE_RELEASE(g_Supervisor.d3dIface);
    if (g_GameWindow.window)
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
        g_GameErrorContext.Log(TH_LOG_RESTARTING);
        if (!g_Supervisor.cfg.windowed)
        {
            WINNLSEnableIME(0, 1);
        }
        i32 i = 0;
        while (i < 60)
        {
            if (PeekMessageA(&msg, NULL, 0, 0, 1))
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
                          g_GameWindow.screenSaveActive, NULL, 2);
    SystemParametersInfoA(SPI_SETLOWPOWERACTIVE, g_GameWindow.lowPowerActive,
                          NULL, 2);
    SystemParametersInfoA(SPI_SETPOWEROFFACTIVE, g_GameWindow.powerOffActive,
                          NULL, 2);
    WINNLSEnableIME(0, 1);
    g_GameErrorContext.Flush();
    return 0;
}

// FUNCTION: TH07 0x00434490
LRESULT __stdcall GameWindow::WindowProc(HWND hWnd, u32 uMsg, WPARAM wParam,
                                         LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_ERASEBKGND:
        return 1;
    case MM_MOM_DONE:
        if (g_Supervisor.midiOutput)
        {
            g_Supervisor.midiOutput->UnprepareHeader((LPMIDIHDR)lParam);
        }
        break;
    case WM_ACTIVATEAPP:
        g_GameWindow.isAppActive = wParam;
        if (g_GameWindow.isAppActive)
        {
            g_GameWindow.isAppInactive = 0;
        }
        else
        {
            g_GameWindow.isAppInactive = 1;
        }
        break;
    case WM_SETCURSOR:
        if (!g_Supervisor.cfg.windowed)
        {
            if (g_GameWindow.isAppInactive)
            {
                SetCursor(LoadCursorA(NULL, IDC_ARROW));
                ShowCursor(1);
            }
            else
            {
                ShowCursor(0);
                SetCursor(NULL);
            }
        }
        else
        {
            SetCursor(LoadCursorA(NULL, IDC_ARROW));
            ShowCursor(1);
        }
        return 1;
    case WM_CLOSE:
        g_GameWindow.isAppClosing = 1;
        return 1;
    }
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

#pragma var_order(i, snapshotPath)
// FUNCTION: TH07 0x004345c0
void GameWindow::Present()
{
    char snapshotPath[252];
    i32 i;

    if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
    {
        g_AnmManager->ReleaseSurfaces();
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
        ResetRenderState();
        g_Supervisor.renderSkipFrames = 2;
    }
    g_AnmManager->TakeScreenshotIfRequested();
    if (WAS_PRESSED_RAW(TH_BUTTON_HOME))
    {
        // STRING: TH07 0x00497c1c
        _mkdir("snapshot");
        for (i = 0; i < 1000; i++)
        {
            // STRING: TH07 0x00497c08
            sprintf(snapshotPath, "snapshot/th%.3d.bmp", i);
            if (FileSystem::CheckFileExists(snapshotPath) == 0)
            {
                break;
            }
        }
        if (i < 1000)
        {
            g_Supervisor.SnapshotScreen(snapshotPath);
        }
    }
    if (g_Supervisor.renderSkipFrames != 0)
    {
        g_Supervisor.renderSkipFrames--;
    }
}

#pragma var_order(chainRes, perfCounter, perfDiff, curTime, timeDiff)
// FUNCTION: TH07 0x004346e0
RenderResult GameWindow::Render()
{
    f64 timeDiff;
    f64 curTime;
    f64 perfDiff;
    LARGE_INTEGER perfCounter;
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
            g_Supervisor.d3dDevice->BeginScene();
            g_AnmManager->ResetVertexBuffer();
            g_Supervisor.fogEnabled = 255;
            g_Supervisor.DisableFog();
            g_Chain.RunDrawChain();
            g_AnmManager->Flush();
            g_Supervisor.d3dDevice->SetTexture(0, NULL);
            g_Supervisor.d3dDevice->EndScene();
        }

        g_AnmManager->Flush();
        g_Supervisor.viewport.X = 0;
        g_Supervisor.viewport.Y = 0;
        g_Supervisor.viewport.Width = GAME_WINDOW_WIDTH;
        g_Supervisor.viewport.Height = GAME_WINDOW_HEIGHT;
        g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);

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

    if (g_Supervisor.cfg.windowed || g_Supervisor.VsyncDisabled())
    {
        if (this->curFrame != 0)
        {
            if (g_GameWindow.lpFrequency.LowPart != 0)
            {
                QueryPerformanceCounter(&perfCounter);
                perfDiff = (f64)(perfCounter.LowPart - g_LastPerfCounter.LowPart) / (f64)g_GameWindow.lpFrequency.LowPart;

                if (perfDiff < 0.0)
                {
                    g_LastPerfCounter.LowPart = perfCounter.LowPart;
                    g_LastPerfCounter.HighPart = perfCounter.HighPart;
                }

                if (perfDiff >= 1.0 / 60.0 || g_GameWindow.usesRelativePath)
                {
                    while (perfDiff >= 1.0 / 60.0)
                    {
                        g_LastPerfCounter.LowPart += g_GameWindow.lpFrequency.LowPart / 60;
                        perfDiff -= 1.0 / 60.0;
                    }
                    if ((i32)g_Supervisor.cfg.frameskipConfig < (i32)this->curFrame)
                    {
                        goto LAB_00434a18;
                    }
                    goto begin_loop;
                }
            }
            else
            {
                timeBeginPeriod(1);
                curTime = (f64)timeGetTime();

                if (curTime < g_LastFrameTime)
                {
                    g_LastFrameTime = curTime;
                }

                timeDiff = fabs(curTime - g_LastFrameTime);
                timeEndPeriod(1);

                if (timeDiff >= 50.0 / 3.0 || g_GameWindow.usesRelativePath)
                {
                    while (timeDiff >= 50.0 / 3.0)
                    {
                        g_LastFrameTime += 50.0 / 3.0;
                        timeDiff -= 50.0 / 3.0;
                    }
                    if ((i32)g_Supervisor.cfg.frameskipConfig < (i32)this->curFrame)
                    {
                        goto LAB_00434a18;
                    }
                    goto begin_loop;
                }
            }
        }
    }

    if (!g_Supervisor.cfg.windowed && !g_Supervisor.VsyncDisabled())
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

// FUNCTION: TH07 0x00434a40
i32 GameWindow::InitD3dInterface()
{
    g_Supervisor.d3dIface = Direct3DCreate8(D3D_SDK_VERSION);
    if (!g_Supervisor.d3dIface)
    {
        g_GameErrorContext.Fatal(TH_ERR_D3D_CREATE_FAIL);
        return true;
    }

    return false;
}

// FUNCTION: TH07 0x00434a80
i32 GameWindow::CreateGameWindow(HINSTANCE hInstance)
{
    WNDCLASSA base_class;
    i32 width;
    i32 height;

    memset(&base_class, 0, sizeof(WNDCLASSA));
    base_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    base_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    base_class.hInstance = hInstance;
    base_class.lpfnWndProc = WindowProc;
    g_GameWindow.isAppActive = 1;
    g_GameWindow.isAppInactive = 0;
    // STRING: TH07 0x00497bd0
    base_class.lpszClassName = "BASE";
    RegisterClassA(&base_class);
    if (!g_Supervisor.cfg.windowed)
    {
        width = GAME_WINDOW_WIDTH;
        height = GAME_WINDOW_HEIGHT;
        g_GameWindow.window = CreateWindowExA(
            0, "BASE", TH_WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
            0, 0, width, height, NULL, NULL, hInstance, NULL);
    }
    else
    {
        width = GetSystemMetrics(SM_CXFIXEDFRAME) * 2 + GAME_WINDOW_WIDTH;
        height = GAME_WINDOW_HEIGHT + GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);
        g_GameWindow.window = CreateWindowExA(
            0, "BASE", TH_WINDOW_TITLE,
            WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL,
            hInstance, NULL);
    }
    g_Supervisor.hwndGameWindow = g_GameWindow.window;
    if (!g_GameWindow.window)
    {
        return true;
    }

    SetWindowActive(g_GameWindow.window);
    return false;
}

#pragma var_order(retryWithoutRefreshRate, usingD3dHal, displayMode, presentParams, halfCameraDistance, \
                  halfHeight, halfWidth, aspectRatio, fov, capsBuffer, pUp,                             \
                  pAt, pEye)
// FUNCTION: TH07 0x00434bd0
i32 GameWindow::InitD3dRendering()
{
    Float3 pEye;
    Float3 pAt;
    Float3 pUp;
    char capsBuffer[8192];
    f32 fov;
    f32 aspectRatio;
    f32 halfWidth;
    f32 halfHeight;
    f32 halfCameraDistance;
    D3DPRESENT_PARAMETERS presentParams;
    D3DDISPLAYMODE displayMode;
    bool usingD3dHal;
    i32 retryWithoutRefreshRate;

    usingD3dHal = true;
    memset(&presentParams, 0, sizeof(D3DPRESENT_PARAMETERS));
    g_Supervisor.d3dIface->GetAdapterDisplayMode(0, &displayMode);
    if (!g_Supervisor.cfg.windowed)
    {
        if (g_Supervisor.cfg.use16BitTextures == 1)
        {
            presentParams.BackBufferFormat = D3DFMT_R5G6B5;
            g_Supervisor.cfg.colorMode16bit = 1;
        }
        else if (g_Supervisor.cfg.colorMode16bit == 255)
        {
            presentParams.BackBufferFormat = D3DFMT_X8R8G8B8;
            g_Supervisor.cfg.colorMode16bit = 0;
            g_GameErrorContext.Log(TH_LOG_DISPLAY_INIT_32BITS);
        }
        else if (!g_Supervisor.cfg.colorMode16bit)
        {
            presentParams.BackBufferFormat = D3DFMT_X8R8G8B8;
        }
        else
        {
            presentParams.BackBufferFormat = D3DFMT_R5G6B5;
        }
        if (g_GameWindow.usesRelativePath != false)
        {
            g_Supervisor.vsyncDisabled = 1;
        }
        if (!g_Supervisor.vsyncDisabled)
        {
            presentParams.FullScreen_RefreshRateInHz = 60;
            presentParams.FullScreen_PresentationInterval = 1;
            g_GameErrorContext.Log(TH_LOG_DISPLAY_CHANGE_60_HZ);
            if (!g_Supervisor.cfg.frameskipConfig)
            {
                presentParams.SwapEffect = D3DSWAPEFFECT_FLIP;
            }
            else
            {
                presentParams.SwapEffect = D3DSWAPEFFECT_COPY_VSYNC;
            }
        }
        else
        {
            presentParams.FullScreen_RefreshRateInHz = 0;
            presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
            presentParams.FullScreen_PresentationInterval =
                D3DPRESENT_INTERVAL_IMMEDIATE;
            g_GameErrorContext.Log(TH_LOG_TRY_ASYNC_VSYNC);
        }
    }
    else
    {
        presentParams.BackBufferFormat = displayMode.Format;
        presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
        presentParams.Windowed = 1;
    }
    presentParams.BackBufferWidth = GAME_WINDOW_WIDTH;
    presentParams.BackBufferHeight = GAME_WINDOW_HEIGHT;
    presentParams.EnableAutoDepthStencil = 1;
    presentParams.AutoDepthStencilFormat = D3DFMT_D16;
    presentParams.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    g_Supervisor.hasLockableBackbuffer = 1;
    g_Supervisor.lockableBackBuffer = 1;
    retryWithoutRefreshRate = 0;
    for (;;)
    {
        if (g_Supervisor.cfg.forceReferenceRender)
        {
            goto fallback_to_software;
        }
        if (FAILED(g_Supervisor.d3dIface->CreateDevice(
                0, D3DDEVTYPE_HAL, g_GameWindow.window,
                D3DCREATE_HARDWARE_VERTEXPROCESSING, &presentParams,
                &g_Supervisor.d3dDevice)))
        {
            if (retryWithoutRefreshRate)
            {
                g_GameErrorContext.Log(TH_LOG_TL_HAL_UNUSABLE);
            }
            if (FAILED(g_Supervisor.d3dIface->CreateDevice(
                    0, D3DDEVTYPE_HAL, g_GameWindow.window, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParams,
                    &g_Supervisor.d3dDevice)))
            {
                if (retryWithoutRefreshRate)
                {
                    g_GameErrorContext.Log(TH_LOG_HAL_UNUSABLE);
                }
            fallback_to_software:
                if (FAILED(g_Supervisor.d3dIface->CreateDevice(
                        0, D3DDEVTYPE_REF, g_GameWindow.window,
                        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParams,
                        &g_Supervisor.d3dDevice)))
                {
                    if (!g_Supervisor.vsyncDisabled)
                    {
                        g_GameErrorContext.Log(TH_LOG_REFRESH_RATE_UNCHANGED);
                        presentParams.FullScreen_RefreshRateInHz = 0;
                        g_Supervisor.lockableBackBuffer = 0;
                        retryWithoutRefreshRate = 1;
                        continue;
                    }

                    if (presentParams.FullScreen_PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE)
                    {
                        g_GameErrorContext.Log(TH_LOG_ASYNC_VSYNC_UNAVAILABLE);
                        g_GameErrorContext.Fatal(TH_ERR_CHANGE_YO_REFRESH_RATE);
                        presentParams.FullScreen_PresentationInterval = 1;
                        presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
                        continue;
                    }
                    else
                    {
                        g_GameErrorContext.Fatal(TH_ERR_D3D_INIT_FAIL);
                        SAFE_RELEASE(g_Supervisor.d3dIface);
                        return 1;
                    }
                }
                else
                {
                    // STRING: TH07 0x004979c8
                    g_GameErrorContext.Log(TH_LOG_USING_REF_RENDER);
                    g_Supervisor.usingTnLHal = 0;
                    usingD3dHal = false;
                }
            }
            else
            {
                g_GameErrorContext.Log(TH_LOG_USING_HAL);
                g_Supervisor.usingTnLHal = 0;
            }
        }
        else
        {
            g_GameErrorContext.Log(TH_LOG_USING_TL_HAL);
            g_Supervisor.usingTnLHal = 1;
        }
        break;
    }

    g_Supervisor.presentParameters = presentParams;
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
    D3DXMatrixLookAtLH(&g_Supervisor.viewMatrix, pEye.asD3DX(), pAt.asD3DX(), pUp.asD3DX());
    D3DXMatrixPerspectiveFovLH(&g_Supervisor.projectionMatrix, fov,
                               aspectRatio, 100.0f, 10000.0f);

    g_Supervisor.d3dDevice->SetTransform(D3DTS_VIEW,
                                         &g_Supervisor.viewMatrix);
    g_Supervisor.d3dDevice->SetTransform(D3DTS_PROJECTION,
                                         &g_Supervisor.projectionMatrix);
    g_Supervisor.d3dDevice->GetViewport(&g_Supervisor.viewport);
    g_Supervisor.d3dDevice->GetDeviceCaps(&g_Supervisor.d3dCaps);
    if (!g_Supervisor.cfg.colorAddEmulation &&
        (g_Supervisor.d3dCaps.TextureOpCaps & 0x40) == 0)
    {
        g_GameErrorContext.Log(TH_LOG_USING_COLOR_ADD_EMU);
        g_Supervisor.cfg.colorAddEmulation = 1;
    }
    if (g_Supervisor.d3dCaps.MaxTextureWidth <= 256)
    {
        g_GameErrorContext.Log(TH_LOG_LARGE_TEX_UNSUPPORTED);
    }
    FormatD3DCapabilities(&g_Supervisor.d3dCaps, capsBuffer);
    g_GameErrorContext.Log(capsBuffer);
    if (!g_Supervisor.cfg.use16BitTextures && usingD3dHal)
    {
        if (g_Supervisor.d3dIface->CheckDeviceFormat(
                0, D3DDEVTYPE_HAL, presentParams.BackBufferFormat, 0,
                D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8) == 0)
        {
            g_Supervisor.supports32BitTex = 1;
        }
        else
        {
            g_Supervisor.supports32BitTex = 0;
            g_Supervisor.cfg.use16BitTextures = 1;
            g_GameErrorContext.Log(TH_LOG_USING_REDUCED_COLOR);
        }
    }
    ResetRenderState();
    ScreenEffect::SetViewport(0xff000000);
    g_GameWindow.isAppClosing = 0;
    g_Supervisor.lastFrameTime = 0;
    return 0;
}

// FUNCTION: TH07 0x004351c0
char *GameWindow::FormatCapability(const char *capabilityName,
                                   u32 capabilityFlags, u32 mask, char *buf)
{
    buf += sprintf(buf, capabilityName);
    if ((capabilityFlags & mask) == 0)
    {
        buf += sprintf(buf, TH_CAP_UNSUPPORTED);
    }
    else
    {
        buf += sprintf(buf, TH_CAP_SUPPORTED);
    }
    return buf;
}

// FUNCTION: TH07 0x00435230
void GameWindow::FormatD3DCapabilities(D3DCAPS8 *caps, char *buf)
{
    char *strPos;

    strPos = buf;
    strPos += sprintf(strPos, TH_CAP_VIDEO_SUMMARY);
    strPos = FormatCapability(TH_CAP_VIDEO_SCANLINE, caps->Caps,
                              D3DCAPS_READ_SCANLINE, strPos);
    strPos = FormatCapability(TH_CAP_VIDEO_WINDOWED, caps->Caps2,
                              D3DCAPS2_CANRENDERWINDOWED, strPos);
    strPos = FormatCapability(
        TH_CAP_VIDEO_PRESENT_INTERVAL_IMMEDIATE, caps->PresentationIntervals,
        D3DPRESENT_INTERVAL_IMMEDIATE, strPos);
    strPos = FormatCapability(
        TH_CAP_VIDEO_PRESENT_INTERVAL_VSYNC, caps->PresentationIntervals,
        D3DPRESENT_INTERVAL_ONE, strPos);
    strPos += sprintf(strPos, TH_CAP_DEV_SUMMARY);
    strPos =
        FormatCapability(TH_CAP_DEV_NON_LOCAL_VRAM_BLIT, caps->DevCaps,
                         D3DDEVCAPS_CANBLTSYSTONONLOCAL, strPos);
    strPos = FormatCapability(TH_CAP_DEV_HARDWARE_TL, caps->DevCaps,
                              D3DDEVCAPS_HWTRANSFORMANDLIGHT, strPos);
    strPos =
        FormatCapability(TH_CAP_DEV_NON_LOCAL_VRAM_TEX, caps->DevCaps,
                         D3DDEVCAPS_TEXTURENONLOCALVIDMEM, strPos);
    strPos =
        FormatCapability(TH_CAP_DEV_SYS_MEM_TEX, caps->DevCaps,
                         D3DDEVCAPS_TEXTURESYSTEMMEMORY, strPos);
    strPos = FormatCapability(TH_CAP_DEV_VRAM_TEX, caps->DevCaps,
                              D3DDEVCAPS_TEXTUREVIDEOMEMORY, strPos);
    strPos =
        FormatCapability(TH_CAP_DEV_SYS_MEM_VERTEX_BUF, caps->DevCaps,
                         D3DDEVCAPS_TLVERTEXSYSTEMMEMORY, strPos);
    strPos =
        FormatCapability(TH_CAP_DEV_VMEM_VERTEX_BUF, caps->DevCaps,
                         D3DDEVCAPS_TLVERTEXVIDEOMEMORY, strPos);
    strPos += sprintf(strPos, TH_CAP_PRIMITIVE_SUMMARY);
    strPos = FormatCapability(TH_CAP_PRIMITIVE_TRANSPARENCY,
                              caps->PrimitiveMiscCaps,
                              D3DPMISCCAPS_BLENDOP, strPos);
    strPos = FormatCapability(
        TH_CAP_PRIMITIVE_POINT_CLIPPING, caps->PrimitiveMiscCaps,
        D3DPMISCCAPS_CLIPPLANESCALEDPOINTS, strPos);
    strPos = FormatCapability(
        TH_CAP_PRIMITIVE_TL_CLIPPING, caps->PrimitiveMiscCaps,
        D3DPMISCCAPS_CLIPTLVERTS, strPos);
    strPos = FormatCapability(
        TH_CAP_PRIMITIVE_CLIP_CCW, caps->PrimitiveMiscCaps,
        D3DPMISCCAPS_CULLCCW, strPos);
    strPos =
        FormatCapability(TH_CAP_PRIMITIVE_CLIP_CW, caps->PrimitiveMiscCaps,
                         D3DPMISCCAPS_CULLCW, strPos);
    strPos = FormatCapability(TH_CAP_PRIMITIVE_CLIP_NONE, caps->PrimitiveMiscCaps,
                              D3DPMISCCAPS_CULLNONE, strPos);
    strPos = FormatCapability(
        TH_CAP_PRIMITIVE_DEPTH_TEST, caps->PrimitiveMiscCaps,
        D3DPMISCCAPS_MASKZ, strPos);
    strPos += sprintf(strPos, TH_CAP_RASTER_SUMMARY);
    strPos = FormatCapability(TH_CAP_RASTER_ANISOTROPIC_FILTER, caps->RasterCaps,
                              D3DPRASTERCAPS_ANISOTROPY, strPos);
    strPos = FormatCapability(TH_CAP_RASTER_ANTIALIASING, caps->RasterCaps,
                              D3DPRASTERCAPS_ANTIALIASEDGES, strPos);
    strPos = FormatCapability(TH_CAP_RASTER_DITHER, caps->RasterCaps,
                              D3DPRASTERCAPS_DITHER, strPos);
    strPos = FormatCapability(TH_CAP_RASTER_RANGE_FOG, caps->RasterCaps,
                              D3DPRASTERCAPS_FOGRANGE, strPos);
    strPos = FormatCapability(TH_CAP_RASTER_Z_FOG, caps->RasterCaps,
                              D3DPRASTERCAPS_ZFOG, strPos);
    strPos = FormatCapability(TH_CAP_RASTER_TABLE_FOG, caps->RasterCaps,
                              D3DPRASTERCAPS_FOGTABLE, strPos);
    strPos = FormatCapability(TH_CAP_RASTER_VERTEX_FOG, caps->RasterCaps,
                              D3DPRASTERCAPS_FOGVERTEX, strPos);
    strPos = FormatCapability(TH_CAP_RASTER_DEPTH_TEST, caps->RasterCaps,
                              D3DPRASTERCAPS_ZTEST, strPos);
    strPos += sprintf(strPos, TH_CAP_SHADE_SUMMARY);
    strPos = FormatCapability(TH_CAP_SHADE_GOURAUD, caps->ShadeCaps,
                              D3DPSHADECAPS_COLORGOURAUDRGB, strPos);
    strPos = FormatCapability(TH_CAP_SHADE_ALPHA_GOURAUD, caps->ShadeCaps,
                              D3DPSHADECAPS_ALPHAGOURAUDBLEND, strPos);
    strPos = FormatCapability(TH_CAP_SHADE_FOG_GOURAUD, caps->ShadeCaps,
                              D3DPSHADECAPS_FOGGOURAUD, strPos);
    strPos += sprintf(strPos, TH_CAP_TEX_SUMMARY);
    strPos += sprintf(strPos, TH_CAP_TEX_MAX_SIZE,
                      caps->MaxTextureWidth, caps->MaxTextureHeight);
    strPos = FormatCapability(TH_CAP_TEX_ALPHA, caps->TextureCaps,
                              D3DPTEXTURECAPS_ALPHA, strPos);
    strPos = FormatCapability(TH_CAP_TEX_TRANSFORM, caps->TextureCaps,
                              D3DPTEXTURECAPS_PROJECTED, strPos);
    strPos = FormatCapability(TH_CAP_BILINEAR_UPSCALE, caps->TextureFilterCaps,
                              D3DPTFILTERCAPS_MAGFLINEAR, strPos);
    strPos = FormatCapability(TH_CAP_BILINEAR_DOWNSCALE, caps->TextureFilterCaps,
                              D3DPTFILTERCAPS_MINFLINEAR, strPos);
    // STRING: TH07 0x00497340
    strPos += sprintf(strPos, "--------------------------------------------\r\n");
}

// FUNCTION: TH07 0x004356a0
void GameWindow::ResetRenderState()
{
    if (!g_Supervisor.cfg.disableZBuffer)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    }
    else
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    }
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    if (!g_Supervisor.cfg.disableGouraud)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    }
    else
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
    }
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ALPHAREF, 4);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    if (!g_Supervisor.cfg.disableFog)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGENABLE, TRUE);
    }
    else
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }
    f32 fogDensity = 1.0f;
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGDENSITY, *(DWORD *)&fogDensity);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGCOLOR, 0xffa0a0a0);

    f32 fog = 1000.0f;
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGSTART, *(DWORD *)&fog);
    fog = 5000.0f;
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGEND, *(DWORD *)&fog);
    if ((g_Supervisor.d3dCaps.RasterCaps | 0x1000) != 0)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_EDGEANTIALIAS, 0);
    }
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, 0);
    if (!g_Supervisor.cfg.disableTextureBlend)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    }
    else
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    if (!g_Supervisor.cfg.noVertexBuffers)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
    }
    else
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    }
    if (!g_Supervisor.cfg.disableTextureBlend)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    }
    else
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    if (!g_Supervisor.cfg.noVertexBuffers)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    }
    else
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS,
                                                 D3DTTFF_COUNT2);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    if (g_AnmManager)
    {
        g_AnmManager->SetBlendMode(255);
        g_AnmManager->SetColorOp(255);
        g_AnmManager->SetVertexShader(255);
        g_AnmManager->SetTexture(NULL);
        g_AnmManager->SetCameraMode(255);
    }
    g_Stage.renderStateWasReset = 1;
}

#pragma var_order(exePath, startupInfo, resolvedPath, ext)
// FUNCTION: TH07 0x00435bd0
ZunResult GameWindow::CheckForRunningGameInstance(HINSTANCE hInstance)
{
    char *ext;
    char resolvedPath[264];
    STARTUPINFO startupInfo;
    char exePath[264];

    // STRING: TH07 0x0049732c
    g_Mutex = CreateMutexA(NULL, 1, "Touhou YouYouMu App");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        g_GameErrorContext.Fatal(TH_ERR_ALREADY_OPEN);
        return ZUN_ERROR;
    }

    startupInfo.cb = sizeof(startupInfo);
    memset(&startupInfo.lpReserved, 0, sizeof(startupInfo) - 4);
    GetModuleFileNameA(NULL, exePath, 0x105);
    GetConsoleTitleA(resolvedPath, 0x105);
    GetStartupInfoA(&startupInfo);
    if (startupInfo.lpTitle)
    {
        ext = strrchr(startupInfo.lpTitle, '.');
        if (FileSystem::CheckFileExists(startupInfo.lpTitle) &&
            ext)
        {
            // STRING: TH07 0x0049730c
            if (_stricmp(ext, ".lnk") == 0)
            {
                do
                {
                    ResolveIt(startupInfo.lpTitle, resolvedPath, 0x104);
                    ext = strrchr(resolvedPath, '.');
                } while (_stricmp(ext, ".lnk") == 0);
            }
            else
            {
                strcpy(resolvedPath, startupInfo.lpTitle);
            }

            if (strcmp(exePath, resolvedPath) != 0)
            {
                g_GameWindow.usesRelativePath = true;
            }
        }
    }
    if (!g_Mutex)
    {
        return ZUN_ERROR;
    }

    return ZUN_SUCCESS;
}

#pragma var_order(processId, param, idAttachTo)
// FUNCTION: TH07 0x00435e30
void GameWindow::SetWindowActive(HWND window)
{
    DWORD idAttachTo;
    void *param;
    DWORD processId;

    idAttachTo = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    processId = GetWindowThreadProcessId(window, NULL);
    AttachThreadInput(processId, idAttachTo, 1);
    SystemParametersInfoA(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &param, 0);
    SystemParametersInfoA(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, 0);

    // ZUN landmine: ZUN here opts to use SetActiveWindow to bring input focus
    // onto the game window rather than SetForegroundWindow. This is quite
    // unusual since it's intended to be used only for already foregrounded
    // windows, which might not always be the case if the application decides
    // to launch in the background.
    // This normally works fine since applications do launch in the foreground,
    // but for some reason on Wine it doesn't work at all, leading to broken
    // input.
    // This is fixed when you use vpatch btw
#ifdef NON_MATCHING
    SetForegroundWindow(window);
#else
    SetActiveWindow(window);
#endif
    SystemParametersInfoA(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &param, 0);
    AttachThreadInput(processId, idAttachTo, 0);
}

#pragma var_order(filename, i, dataCursor, checksum, dataBase)
// FUNCTION: TH07 0x00435ec0
i32 GameWindow::ChecksumExecutable()
{
    u32 *dataBase;
    u32 checksum;
    u32 *dataCursor;
    u32 i;
    char filename[MAX_PATH + 1];

    if (GetModuleFileNameA(NULL, filename, 0x105))
    {
        checksum = 0;
        dataBase = dataCursor = (u32 *)FileSystem::OpenFile(filename, 1);
        if (!dataCursor)
        {
            return -1;
        }

        for (i = 0; i < g_LastFileSize / 4 - 1; i++, dataCursor++)
        {
            checksum += *dataCursor;
        }
        // STRING: TH07 0x004972fc
        DebugPrint("main sum %d\r\n", checksum);
        free(dataBase);
        g_Supervisor.exeChecksum = checksum;
        g_Supervisor.exeSize = g_LastFileSize;
        return checksum;
    }

    return -1;
}

#pragma var_order(hr, ret, psl, ppf, wPath, wfd)
// FUNCTION: TH07 0x00435fc0
i32 GameWindow::ResolveIt(const char *shortcutPath, char *dstPath,
                          i32 maxPathLen)
{
    WIN32_FIND_DATAA wfd;
    LPWSTR wPath;
    IPersistFile *ppf;
    IShellLinkA *psl;
    i32 ret;
    HRESULT hr;

    if (!dstPath)
    {
        return 0;
    }

    ret = 0;
    CoInitialize(NULL);
    if (SUCCEEDED(hr = CoCreateInstance(CLSID_ShellLink, NULL,
                                        CLSCTX_INPROC_SERVER, IID_IShellLink,
                                        (void **)&psl)))
    {
        if (SUCCEEDED(hr = psl->QueryInterface(IID_IPersistFile,
                                               (void **)&ppf)))
        {
            wPath = new WCHAR[maxPathLen];
            if (SUCCEEDED(hr))
            {
                MultiByteToWideChar(CP_ACP, 0, shortcutPath, -1, wPath,
                                    maxPathLen);
                if (SUCCEEDED(hr = ppf->Load(wPath, STGM_READ)))
                {
                    if (SUCCEEDED(hr = psl->GetPath(dstPath, maxPathLen, &wfd,
                                                    0)))
                    {
                        ret = 1;
                    }
                }
            }
            delete wPath;
            ppf->Release();
        }
        psl->Release();
    }
    CoUninitialize();
    return ret;
}
