#include "GameWindow.hpp"

#include <d3d8.h>
#include <direct.h>
#include <io.h>
#include <math.h>
#include <stdio.h>

typedef __w64 long SHANDLE_PTR; // i dont know anymore bro

#include <shlobj.h>

#include "AnmManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "ScreenEffect.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"

// GLOBAL: TH07 0x00575c20
GameWindow g_GameWindow;

// GLOBAL: TH07 0x0135e1f4
HANDLE g_Mutex;

// GLOBAL: TH07 0x0135e1f8
i32 g_TickCountToEffectiveFramerate;

// GLOBAL: TH07 0x0135e200
f64 g_LastFrameTime;

// GLOBAL: TH07 0x0135e208
LARGE_INTEGER g_LastPerfCounter;

// winmain should probably be here

// FUNCTION: TH07 0x00434490
LRESULT __stdcall GameWindow::WindowProc(HWND hWnd, u32 uMsg, WPARAM wParam,
                                         LPARAM lParam)
{
    if (uMsg < 0x1d)
    {
        if (uMsg == WM_ACTIVATEAPP)
        {
            g_GameWindow.lastActiveAppValue = wParam;
            g_GameWindow.isAppActive = (i32)(wParam == 0);
        }
        else
        {
            if (uMsg == WM_CLOSE)
            {
                g_GameWindow.isAppClosing = 1;
                return 1;
            }
            if (uMsg == WM_ERASEBKGND)
            {
                return 1;
            }
        }
    }
    else
    {
        if (uMsg == WM_SETCURSOR)
        {
            if (g_Supervisor.cfg.windowed == 0)
            {
                if (g_GameWindow.isAppActive == 0)
                {
                    ShowCursor(0);
                    SetCursor(NULL);
                }
                else
                {
                    SetCursor(LoadCursorA(NULL, IDC_ARROW));
                    ShowCursor(1);
                }
            }
            else
            {
                SetCursor(LoadCursorA(NULL, IDC_ARROW));
                ShowCursor(1);
            }
            return 1;
        }
        if ((uMsg == 609) && (g_Supervisor.midiOutput != NULL))
        {
            g_Supervisor.midiOutput->UnprepareHeader((LPMIDIHDR)lParam);
        }
    }
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

#pragma var_order(i, local_10c)
// FUNCTION: TH07 0x004345c0
void GameWindow::Present()
{
    char local_10c[252];
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
            sprintf(local_10c, "snapshot/th%.3d.bmp", i);
            if (FileSystem::CheckFileExists(local_10c) == 0)
            {
                break;
            }
        }
        if (i < 1000)
        {
            g_Supervisor.SnapshotScreen(local_10c);
        }
    }
    if (g_Supervisor.renderSkipFrames != 0)
    {
        g_Supervisor.renderSkipFrames = g_Supervisor.renderSkipFrames - 1;
    }
}

// FUNCTION: TH07 0x004346e0
RenderResult GameWindow::Render()
{
    DWORD curTimeReg;
    f64 local_2c;
    f64 local_1c;
    LARGE_INTEGER local_10;
    i32 local_8;
    f64 slowDown;

    if (this->lastActiveAppValue != 0)
    {
        if (this->curFrame != 0)
        {
            goto LAB_00434820;
        }
    LAB_00434708:
        do
        {
            while (true)
            {
                if ((i32)g_Supervisor.cfg.frameskipConfig <= (i32)this->curFrame)
                {
                    g_Supervisor.d3dDevice->BeginScene();
                    g_AnmManager->ResetVertexBuffer();
                    g_Supervisor.fogEnabled = 0xff;
                    g_Supervisor.DisableFog();
                    g_Chain.RunDrawChain();
                    g_AnmManager->Flush();
                    g_Supervisor.d3dDevice->SetTexture(0, NULL);
                    g_Supervisor.d3dDevice->EndScene();
                }
                g_AnmManager->Flush();
                g_Supervisor.viewport.X = 0;
                g_Supervisor.viewport.Y = 0;
                g_Supervisor.viewport.Width = 640;
                g_Supervisor.viewport.Height = 480;
                g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
                local_8 = g_Chain.RunCalcChain();
                g_SoundPlayer.ProcessQueues();
                if (local_8 == 0)
                {
                    return RENDER_RESULT_EXIT_SUCCESS;
                }
                if (local_8 == -1)
                {
                    return RENDER_RESULT_EXIT_ERROR;
                }
                this->curFrame = this->curFrame + 1;
            LAB_00434820:
                if (((g_Supervisor.cfg.windowed != 0) ||
                     (g_Supervisor.vsyncEnabled != 0)) &&
                    (this->curFrame != 0))
                {
                    break;
                }
            LAB_004349e2:
                if (g_Supervisor.cfg.windowed != 0)
                {
                    return RENDER_RESULT_KEEP_RUNNING;
                }
                if (g_Supervisor.vsyncEnabled != 0)
                {
                    return RENDER_RESULT_KEEP_RUNNING;
                }
                if ((i32)g_Supervisor.cfg.frameskipConfig < (i32)this->curFrame)
                {
                    goto LAB_00434a18;
                }
                Present();
            }
            if (g_GameWindow.lpFrequency.LowPart != 0)
            {
                QueryPerformanceCounter(&local_10);
                local_1c = (f64)(local_10.LowPart - g_LastPerfCounter.LowPart) /
                           (f64)g_GameWindow.lpFrequency.LowPart;
                if (local_1c < 0.0)
                {
                    g_LastPerfCounter.LowPart = local_10.LowPart;
                    g_LastPerfCounter.HighPart = local_10.HighPart;
                }
                if ((local_1c < 1.0 / 60.0) && (g_GameWindow.usesRelativePath == false))
                {
                    goto LAB_004349e2;
                }
                while (1.0 / 60.0 <= local_1c)
                {
                    g_LastPerfCounter.LowPart += g_GameWindow.lpFrequency.LowPart / 60;
                    local_1c -= 1.0 / 60.0;
                }
                if ((i32)g_Supervisor.cfg.frameskipConfig < (i32)this->curFrame)
                {
                    break;
                }
                goto LAB_00434708;
            }
            timeBeginPeriod(1);
            curTimeReg = timeGetTime();
            slowDown = (f64)curTimeReg;
            if (slowDown < g_LastFrameTime)
            {
                g_LastFrameTime = slowDown;
            }
            local_2c = fabs(slowDown - g_LastFrameTime);
            timeEndPeriod(1);
            if ((local_2c < 50.0 / 3.0) && (g_GameWindow.usesRelativePath == false))
            {
                goto LAB_004349e2;
            }
            while (50.0 / 3.0 <= local_2c)
            {
                g_LastFrameTime += 50.0 / 3.0;
                local_2c -= 50.0 / 3.0;
            }
        } while ((i32)this->curFrame <= (i32)g_Supervisor.cfg.frameskipConfig);
    LAB_00434a18:
        Present();
        this->curFrame = 0;
        g_TickCountToEffectiveFramerate += 1;
    }
    return RENDER_RESULT_KEEP_RUNNING;
}

// FUNCTION: TH07 0x00434a40
i32 GameWindow::InitD3dInterface()
{
    g_Supervisor.d3dIface = Direct3DCreate8(0x78);
    if (g_Supervisor.d3dIface == NULL)
    {
        // STRING: TH07 0x00497bd8
        g_GameErrorContext.Fatal("Direct3D オブジェクトは何故か作成出来なかった\r\n");
        return true;
    }

    return false;
}

// FUNCTION: TH07 0x00434a80
i32 GameWindow::CreateGameWindow(HINSTANCE hInstance) // GameWindow.cpp:275
{
    WNDCLASSA base_class;
    i32 width;
    i32 height;

    memset(&base_class, 0, sizeof(WNDCLASSA));
    base_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    base_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    base_class.hInstance = hInstance;
    base_class.lpfnWndProc = WindowProc;
    g_GameWindow.lastActiveAppValue = 1;
    g_GameWindow.isAppActive = 0;
    // STRING: TH07 0x00497bd0
    base_class.lpszClassName = "BASE";
    RegisterClassA(&base_class);
    if (g_Supervisor.cfg.windowed == 0)
    {
        width = 640;
        height = 480;
        g_GameWindow.window = CreateWindowExA(
            0, "BASE",
            // STRING: TH07 0x00497b9c
            "東方妖々夢　～ Perfect Cherry Blossom. ver 1.00b", WS_OVERLAPPEDWINDOW,
            0, 0, width, height, NULL, NULL, hInstance, NULL);
    }
    else
    {
        width = GetSystemMetrics(SM_CXFIXEDFRAME) * 2 + 640;
        height = 480 + GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);
        g_GameWindow.window = CreateWindowExA(
            0, "BASE", "東方妖々夢　～ Perfect Cherry Blossom. ver 1.00b",
            WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL,
            hInstance, NULL);
    }
    g_Supervisor.hwndGameWindow = g_GameWindow.window;
    if (g_GameWindow.window == NULL)
    {
        return true;
    }

    SetWindowActive(g_GameWindow.window);
    return false;
}

// FUNCTION: TH07 0x00434bd0
i32 GameWindow::InitD3dRendering()
{
    bool bVar1;
    D3DXVECTOR3 pEye;
    D3DXVECTOR3 pAt;
    D3DXVECTOR3 pUp;
    char local_2064[8192];
    f32 fov;
    f32 aspectRatio;
    f32 halfWidth;
    f32 halfHeight;
    f32 halfCameraDistance;
    D3DPRESENT_PARAMETERS presentParams;
    D3DDISPLAYMODE display_mode;
    bool usingD3dHal;

    usingD3dHal = true;
    memset(&presentParams, 0, sizeof(D3DPRESENT_PARAMETERS));
    g_Supervisor.d3dIface->GetAdapterDisplayMode(0, &display_mode);
    if (g_Supervisor.cfg.windowed == 0)
    {
        if ((g_Supervisor.cfg.opts >> 2 & 1) == 1)
        {
            presentParams.BackBufferFormat = D3DFMT_R5G6B5;
            g_Supervisor.cfg.colorMode16bit = 1;
        }
        else if (g_Supervisor.cfg.colorMode16bit == 0xff)
        {
            presentParams.BackBufferFormat = D3DFMT_X8R8G8B8;
            g_Supervisor.cfg.colorMode16bit = 0;
            // STRING: TH07 0x00497b70
            g_GameErrorContext.Log("初回起動、画面を 32Bits で初期化しました\r\n");
        }
        else if (g_Supervisor.cfg.colorMode16bit == 0)
        {
            presentParams.BackBufferFormat = D3DFMT_X8R8G8B8;
        }
        else
        {
            presentParams.BackBufferFormat = D3DFMT_R5G6B5;
        }
        if (g_GameWindow.usesRelativePath != false)
        {
            g_Supervisor.vsyncEnabled = 1;
        }
        if (g_Supervisor.vsyncEnabled == 0)
        {
            presentParams.FullScreen_RefreshRateInHz = 60;
            presentParams.FullScreen_PresentationInterval = 1;
            // STRING: TH07 0x00497b44
            g_GameErrorContext.Log("リフレッシュレートを60Hzに変更を試みます\r\n");
            if (g_Supervisor.cfg.frameskipConfig == 0)
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
            // STRING: TH07 0x00497b20
            g_GameErrorContext.Log("VSync非同期可能かどうかを試みます\r\n");
        }
    }
    else
    {
        presentParams.BackBufferFormat = display_mode.Format;
        presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
        presentParams.Windowed = 1;
    }
    presentParams.BackBufferWidth = 640;
    presentParams.BackBufferHeight = 480;
    presentParams.EnableAutoDepthStencil = 1;
    presentParams.AutoDepthStencilFormat = D3DFMT_D16;
    presentParams.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    g_Supervisor.flags = g_Supervisor.flags | 2;
    g_Supervisor.lockableBackBuffer = 1;
    bVar1 = false;
    while (true)
    {
        if ((g_Supervisor.cfg.opts >> 9 & 1) == 0)
        {
            if (SUCCEEDED(g_Supervisor.d3dIface->CreateDevice(
                    0, D3DDEVTYPE_HAL, g_GameWindow.window,
                    D3DCREATE_HARDWARE_VERTEXPROCESSING, &presentParams,
                    &g_Supervisor.d3dDevice)))
            {
                // STRING: TH07 0x00497998
                g_GameErrorContext.Log("T&L HAL で動作しま～す\r\n");
                g_Supervisor.flags = g_Supervisor.flags | 1;
                goto LAB_00434f4e;
            }
            if (bVar1)
            {
                // STRING: TH07 0x00497afc
                g_GameErrorContext.Log("T&L HAL は使用できないようです\r\n");
            }
            if (FAILED(g_Supervisor.d3dIface->CreateDevice(
                    0, D3DDEVTYPE_HAL, g_GameWindow.window, 0x20, &presentParams,
                    &g_Supervisor.d3dDevice)))
            {
                if (bVar1)
                {
                    // STRING: TH07 0x00497adc
                    g_GameErrorContext.Log("HAL も使用できないようです\r\n");
                }
                goto LAB_00434dff;
            }
            // STRING: TH07 0x004979b4
            g_GameErrorContext.Log("HAL で動作します\r\n");
        LAB_00434f2d:
            g_Supervisor.flags = g_Supervisor.flags & 0xfffffffe;
        LAB_00434f4e:
            g_Supervisor.presentParameters = presentParams;
            halfWidth = 320.0f;
            halfHeight = 240.0f;
            aspectRatio = 1.3333334f;
            fov = 0.5235988f;
            halfCameraDistance = tanf(fov / 2.0f);
            halfCameraDistance = halfHeight / halfCameraDistance;
            pUp.x = 0.0f;
            pUp.y = 1.0f;
            pUp.z = 0.0f;
            pAt.x = halfWidth;
            pAt.y = -halfHeight;
            pAt.z = 0.0f;
            pEye.x = halfWidth;
            pEye.y = -halfHeight;
            pEye.z = -halfCameraDistance;
            D3DXMatrixLookAtLH(&g_Supervisor.viewMatrix, &pEye, &pAt, &pUp);
            D3DXMatrixPerspectiveFovLH(&g_Supervisor.projectionMatrix, fov,
                                       aspectRatio, 100.0f, 10000.0f);

            g_Supervisor.d3dDevice->SetTransform(D3DTS_VIEW,
                                                 &g_Supervisor.viewMatrix);
            g_Supervisor.d3dDevice->SetTransform(D3DTS_PROJECTION,
                                                 &g_Supervisor.projectionMatrix);
            g_Supervisor.d3dDevice->GetViewport(&g_Supervisor.viewport);
            g_Supervisor.d3dDevice->GetDeviceCaps(&g_Supervisor.d3dCaps);
            if (((g_Supervisor.cfg.opts & 1) == 0) &&
                ((g_Supervisor.d3dCaps.TextureOpCaps & 0x40) == 0))
            {
                // STRING: TH07 0x00497948
                g_GameErrorContext.Log("D3DTEXOPCAPS_ADD をサポートしていません、色加算エミュレートモードで動作します\r\n");
                g_Supervisor.cfg.opts = g_Supervisor.cfg.opts | 1;
            }
            if (g_Supervisor.d3dCaps.MaxTextureWidth < 0x101)
            {
                // STRING: TH07 0x004978f8
                g_GameErrorContext.Log("512 以上のテクスチャをサポートしていません。殆どの絵がボケて表示されます。\r\n");
            }
            FormatD3DCapabilities(&g_Supervisor.d3dCaps, local_2064);
            g_GameErrorContext.Log(local_2064);
            if (((g_Supervisor.cfg.opts >> 2 & 1) == 0) && (usingD3dHal))
            {
                if (g_Supervisor.d3dIface->CheckDeviceFormat(
                        0, D3DDEVTYPE_HAL, presentParams.BackBufferFormat, 0,
                        D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8) == 0)
                {
                    g_Supervisor.flags |= 4;
                }
                else
                {
                    g_Supervisor.flags &= 0xfffffffb;
                    g_Supervisor.cfg.opts |= 4;
                    // STRING: TH07 0x004978b0
                    g_GameErrorContext.Log("D3DFMT_A8R8G8B8 をサポートしていません、減色モードで動作します\r\n");
                }
            }
            ResetRenderState();
            ScreenEffect::SetViewport(0xff000000);
            g_Supervisor.lastFrameTime = 0;
            g_GameWindow.isAppClosing = 0;
            return 0;
        }
    LAB_00434dff:
        if (SUCCEEDED(g_Supervisor.d3dIface->CreateDevice(
                0, D3DDEVTYPE_REF, g_GameWindow.window,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParams,
                &g_Supervisor.d3dDevice)))
        {
            // STRING: TH07 0x004979c8
            g_GameErrorContext.Log("REF で動作しますが、重すぎて恐らくゲームになりません...\r\n");
            usingD3dHal = false;
            goto LAB_00434f2d;
        }
        if (g_Supervisor.vsyncEnabled == 0)
        {
            // STRING: TH07 0x00497ab4
            g_GameErrorContext.Log("リフレッシュレートが変更できません\r\n");
            presentParams.FullScreen_RefreshRateInHz = 0;
            g_Supervisor.lockableBackBuffer = 0;
            bVar1 = true;
        }
        else
        {
            if (presentParams.FullScreen_PresentationInterval !=
                D3DPRESENT_INTERVAL_IMMEDIATE)
            {
                // STRING: TH07 0x00497a04
                g_GameErrorContext.Fatal("Direct3D の初期化に失敗、これではゲームは出来ません\r\n");
                SAFE_RELEASE(g_Supervisor.d3dIface);
                return 1;
            }
            // STRING: TH07 0x00497a7c
            g_GameErrorContext.Log("非同期更新も行えません。一番汚いモードに変更します\r\n");
            // STRING: TH07 0x00497a3c
            g_GameErrorContext.Fatal("*** リフレッシュレートを60Hzに変更することを推奨します ***\r\n");
            presentParams.FullScreen_PresentationInterval = 1;
            presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
        }
    }
}

// FUNCTION: TH07 0x004351c0
char *GameWindow::FormatCapability(const char *capabilityName,
                                   u32 capabilityFlags, u32 mask, char *buf)
{
    buf += sprintf(buf, capabilityName);
    if ((capabilityFlags & mask) == 0)
    {
        // STRING: TH07 0x004978a4
        buf += sprintf(buf, "不可\r\n");
    }
    else
    {
        // STRING: TH07 0x0049789c
        buf += sprintf(buf, "可\r\n");
    }
    return buf;
}

// FUNCTION: TH07 0x00435230
void GameWindow::FormatD3DCapabilities(D3DCAPS8 *caps, char *buf)
{
    char *strPos;

    strPos = buf;
    // STRING: TH07 0x0049786c
    strPos += sprintf(strPos, "現在のビデオカード、及びドライバの能力詳細\r\n");
    // STRING: TH07 0x00497858
    strPos = FormatCapability("　走査線取得能力 : ", caps->Caps,
                              D3DCAPS_READ_SCANLINE, strPos);
    // STRING: TH07 0x00497834
    strPos = FormatCapability("　ウィンドウモードのレンダリング : ", caps->Caps2,
                              D3DCAPS2_CANRENDERWINDOWED, strPos);
    strPos = FormatCapability(
        // STRING: TH07 0x00497810
        "　プレゼンテーション間隔（直接）: ", caps->PresentationIntervals,
        D3DPRESENT_INTERVAL_IMMEDIATE, strPos);
    strPos = FormatCapability(
        // STRING: TH07 0x004977e8
        "　プレゼンテーション間隔（垂直同期）: ", caps->PresentationIntervals,
        D3DPRESENT_INTERVAL_ONE, strPos);
    // STRING: TH07 0x004977b4
    strPos += sprintf(strPos, "　-- デバイス能力 ------------------------------\r\n");
    strPos =
        // STRING: TH07 0x0049778c
        FormatCapability("　System -> 非ローカルVRAMブリット : ", caps->DevCaps,
                         D3DDEVCAPS_CANBLTSYSTONONLOCAL, strPos);
    // STRING: TH07 0x00497774
    strPos = FormatCapability("　ハードウェア T&L : ", caps->DevCaps,
                              D3DDEVCAPS_HWTRANSFORMANDLIGHT, strPos);
    strPos =
        // STRING: TH07 0x0049774c
        FormatCapability("　非ローカルVRAMからテクスチャ取得 : ", caps->DevCaps,
                         D3DDEVCAPS_TEXTURENONLOCALVIDMEM, strPos);
    strPos =
        // STRING: TH07 0x00497724
        FormatCapability("　システムメモリからテクスチャ取得 : ", caps->DevCaps,
                         D3DDEVCAPS_TEXTURESYSTEMMEMORY, strPos);
    // STRING: TH07 0x00497704
    strPos = FormatCapability("　VRAM からテクスチャ取得 : ", caps->DevCaps,
                              D3DDEVCAPS_TEXTUREVIDEOMEMORY, strPos);
    strPos =
        // STRING: TH07 0x004976dc
        FormatCapability("　頂点バッファにシステムメモリを使用 : ", caps->DevCaps,
                         D3DDEVCAPS_TLVERTEXSYSTEMMEMORY, strPos);
    strPos =
        // STRING: TH07 0x004976b4
        FormatCapability("　頂点バッファにビデオメモリを使用 : ", caps->DevCaps,
                         D3DDEVCAPS_TLVERTEXVIDEOMEMORY, strPos);
    strPos +=
        // STRING: TH07 0x00497680
        sprintf(strPos, "　-- プリミティブ能力 ---------------------------\r\n");
    // STRING: TH07 0x00497670
    strPos = FormatCapability("　半透明処理 : ", caps->PrimitiveMiscCaps,
                              D3DPMISCCAPS_BLENDOP, strPos);
    strPos = FormatCapability(
        // STRING: TH07 0x00497650
        "　ポイントのクリッピング処理 : ", caps->PrimitiveMiscCaps,
        D3DPMISCCAPS_CLIPPLANESCALEDPOINTS, strPos);
    strPos = FormatCapability(
        // STRING: TH07 0x0049762c
        "　プリミティブのクリッピング処理 : ", caps->PrimitiveMiscCaps,
        D3DPMISCCAPS_CLIPTLVERTS, strPos);
    strPos = FormatCapability(
        // STRING: TH07 0x0049760c
        "　法線クリップ（反時計周り） : ", caps->PrimitiveMiscCaps,
        D3DPMISCCAPS_CULLCCW, strPos);
    strPos =
        // STRING: TH07 0x004975ec
        FormatCapability("　法線クリップ（時計周り） : ", caps->PrimitiveMiscCaps,
                         D3DPMISCCAPS_CULLCW, strPos);
    // STRING: TH07 0x004975d4
    strPos = FormatCapability("　法線クリップ無し : ", caps->PrimitiveMiscCaps,
                              D3DPMISCCAPS_CULLNONE, strPos);
    strPos = FormatCapability(
        // STRING: TH07 0x004975b4
        "　デプステストON/OFF切り替え : ", caps->PrimitiveMiscCaps,
        D3DPMISCCAPS_MASKZ, strPos);
    // STRING: TH07 0x00497580
    strPos += sprintf(strPos, "　-- ラスタ能力 --------------------------------\r\n");
    // STRING: TH07 0x00497564
    strPos = FormatCapability("　異方性フィルタリング : ", caps->RasterCaps,
                              D3DPRASTERCAPS_ANISOTROPY, strPos);
    // STRING: TH07 0x00497548
    strPos = FormatCapability("　アンチエイリアシング : ", caps->RasterCaps,
                              D3DPRASTERCAPS_ANTIALIASEDGES, strPos);
    // STRING: TH07 0x00497538
    strPos = FormatCapability("　ディザ処理 : ", caps->RasterCaps,
                              D3DPRASTERCAPS_DITHER, strPos);
    // STRING: TH07 0x00497520
    strPos = FormatCapability("　範囲ベースのフォグ : ", caps->RasterCaps,
                              D3DPRASTERCAPS_FOGRANGE, strPos);
    // STRING: TH07 0x00497508
    strPos = FormatCapability("　Zベースのフォグ : ", caps->RasterCaps,
                              D3DPRASTERCAPS_ZFOG, strPos);
    // STRING: TH07 0x004974f4
    strPos = FormatCapability("　テーブルフォグ : ", caps->RasterCaps,
                              D3DPRASTERCAPS_FOGTABLE, strPos);
    // STRING: TH07 0x004974e4
    strPos = FormatCapability("　頂点フォグ : ", caps->RasterCaps,
                              D3DPRASTERCAPS_FOGVERTEX, strPos);
    // STRING: TH07 0x004974d0
    strPos = FormatCapability("　デプステスト : ", caps->RasterCaps,
                              D3DPRASTERCAPS_ZTEST, strPos);
    // STRING: TH07 0x0049749c
    strPos += sprintf(strPos, "　-- シェーディング能力 -----------------------\r\n");
    // STRING: TH07 0x00497480
    strPos = FormatCapability("　グーローシェーディング : ", caps->ShadeCaps,
                              D3DPSHADECAPS_COLORGOURAUDRGB, strPos);
    // STRING: TH07 0x0049745c
    strPos = FormatCapability("　α成分のグーローシェーディング : ", caps->ShadeCaps,
                              D3DPSHADECAPS_ALPHAGOURAUDBLEND, strPos);
    // STRING: TH07 0x00497438
    strPos = FormatCapability("　グーローシェーディングでフォグ : ", caps->ShadeCaps,
                              D3DPSHADECAPS_FOGGOURAUD, strPos);
    // STRING: TH07 0x00497404
    strPos += sprintf(strPos, "　-- テクスチャ能力 ---------------------------\r\n");
    // STRING: TH07 0x004973e0
    strPos += sprintf(strPos, "　最大テクスチャサイズ : (%d, %d)\r\n",
                    caps->MaxTextureWidth, caps->MaxTextureHeight);
    // STRING: TH07 0x004973c8
    strPos = FormatCapability("　α付きテクスチャ : ", caps->TextureCaps,
                              D3DPTEXTURECAPS_ALPHA, strPos);
    // STRING: TH07 0x004973a8
    strPos = FormatCapability("　テクスチャトランスフォーム : ", caps->TextureCaps,
                              D3DPTEXTURECAPS_PROJECTED, strPos);
    // STRING: TH07 0x0049738c
    strPos = FormatCapability("　バイリニア補間（拡大） : ", caps->TextureFilterCaps,
                              D3DPTFILTERCAPS_MAGFLINEAR, strPos);
    // STRING: TH07 0x00497370
    strPos = FormatCapability("　バイリニア補間（縮小） : ", caps->TextureFilterCaps,
                              D3DPTFILTERCAPS_MINFLINEAR, strPos);
    // STRING: TH07 0x00497340
    strPos += sprintf(strPos, "--------------------------------------------\r\n");
}

// FUNCTION: TH07 0x004356a0
void GameWindow::ResetRenderState()
{
    if ((g_Supervisor.cfg.opts >> 6 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZENABLE, 1);
    }
    else
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZENABLE, 0);
    }
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_LIGHTING, 0);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_CULLMODE, 1);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
    if ((g_Supervisor.cfg.opts >> 5 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_SHADEMODE, 2);
    }
    else
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_SHADEMODE, 1);
    }
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_SRCBLEND, 5);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_DESTBLEND, 6);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZFUNC, 8);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, 1);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ALPHAREF, 4);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_ALPHAFUNC, 7);
    if ((g_Supervisor.cfg.opts >> 10 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGENABLE, 1);
    }
    else
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGENABLE, 0);
    }
    f32 fogDensity = 1.0f;
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGDENSITY, *(DWORD *)&fogDensity);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGTABLEMODE, 0);
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_FOGVERTEXMODE, 3);
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
    if ((g_Supervisor.cfg.opts >> 8 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 4);
    }
    else
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 2);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 2);
    if ((g_Supervisor.cfg.opts >> 1 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, 3);
    }
    else
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, 0);
    }
    if ((g_Supervisor.cfg.opts >> 8 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 4);
    }
    else
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 2);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 2);
    if ((g_Supervisor.cfg.opts >> 1 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, 3);
    }
    else
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, 0);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, 0);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, 2);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, 2);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS,
                                                 2);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ADDRESSW, 3);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, 1);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, 1);
    if (g_AnmManager != NULL)
    {
        g_AnmManager->SetBlendMode(0xff);
        g_AnmManager->SetColorOp(0xff);
        g_AnmManager->SetVertexShader(0xff);
        g_AnmManager->SetTexture(NULL);
        g_AnmManager->SetCameraMode(0xff);
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
        // STRING: TH07 0x00497314
        g_GameErrorContext.Fatal("二つは起動できません\r\n");
        return ZUN_ERROR;
    }

    startupInfo.cb = sizeof(startupInfo);
    memset(&startupInfo.lpReserved, 0, sizeof(startupInfo) - 4);
    GetModuleFileNameA(NULL, exePath, 0x105);
    GetConsoleTitleA(resolvedPath, 0x105);
    GetStartupInfoA(&startupInfo);
    if (startupInfo.lpTitle != NULL)
    {
        ext = strrchr(startupInfo.lpTitle, '.');
        if ((FileSystem::CheckFileExists(startupInfo.lpTitle) != 0) &&
            (ext != NULL))
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
    if (g_Mutex == NULL)
    {
        return ZUN_ERROR;
    }

    return ZUN_SUCCESS;
}

#pragma var_order(local_8, local_c, idAttachTo)
// FUNCTION: TH07 0x00435e30
void GameWindow::SetWindowActive(HWND window)
{
    DWORD idAttachTo;
    void *local_c;
    DWORD local_8;

    idAttachTo = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    local_8 = GetWindowThreadProcessId(window, NULL);
    AttachThreadInput(local_8, idAttachTo, 1);
    SystemParametersInfoA(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &local_c, 0);
    SystemParametersInfoA(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, 0);
#ifdef NON_MATCHING
    // since the game's input is completely broken on wine when using
    // setactivewindow, this so i can actually load ingame. this happens in the
    // original executable as well though
    SetForegroundWindow(window);
#else
    SetActiveWindow(window);
#endif
    SystemParametersInfoA(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &local_c, 0);
    AttachThreadInput(local_8, idAttachTo, 0);
}

// FUNCTION: TH07 0x00435ec0
i32 GameWindow::ChecksumExecutable()
{
    u8 *dataBase;
    i32 checksum;
    u8 *dataCursor;
    CHAR filename[264];

    if (GetModuleFileNameA(NULL, filename, 0x105) == 0)
    {
        checksum = -1;
    }
    else
    {
        checksum = 0;
        dataBase = FileSystem::OpenFile(filename, 1);
        if (dataBase == NULL)
        {
            checksum = -1;
        }
        else
        {
            dataCursor = dataBase;
            for (u32 i = 0; i < (g_LastFileSize >> 2) - 1; i++)
            {
                checksum = checksum + *(i32 *)dataCursor;
                dataCursor = dataCursor + 4;
            }
            // STRING: TH07 0x004972fc
            DebugPrint("main sum %d\r\n", checksum);
            free(dataBase);
            g_Supervisor.exeChecksum = checksum;
            g_Supervisor.exeSize = g_LastFileSize;
        }
    }
    return checksum;
}

// FUNCTION: TH07 0x00435fc0
i32 GameWindow::ResolveIt(const char *shortcutPath, char *dstPath,
                          i32 maxPathLen)
{
    WIN32_FIND_DATAA wfd;
    LPWSTR wPath;
    IPersistFile *ppf;
    IShellLinkA *psl;
    i32 ret;
    HRESULT hres;

    if (dstPath == NULL)
    {
        ret = 0;
    }
    else
    {
        ret = 0;
        CoInitialize(NULL);
        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                IID_IShellLink, (void **)&psl);
        if (SUCCEEDED(hres))
        {
            hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
            if (SUCCEEDED(hres))
            {
                wPath = (LPWSTR)malloc(maxPathLen * sizeof(WCHAR));
                MultiByteToWideChar(CP_ACP, 0, shortcutPath, -1, wPath, maxPathLen);
                if (SUCCEEDED(hres))
                {
                    hres = ppf->Load(wPath, STGM_READ);
                    if (SUCCEEDED(hres))
                    {
                        hres = psl->GetPath(dstPath, maxPathLen, &wfd, 0);
                        if (SUCCEEDED(hres))
                        {
                            ret = 1;
                        }
                    }
                }
                free(wPath);
                ppf->Release();
            }
            psl->Release();
        }
        CoUninitialize();
    }
    return ret;
}
