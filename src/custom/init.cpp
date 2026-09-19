#include <cstdio>
#include <windows.h>

#include "Supervisor.hpp"
#include "i18n.hpp"
#include "resource.h"

// GLOBAL: CUSTOM 0x0040a040
ControllerMapping g_CustomControllerMapping = {0};

// GLOBAL: CUSTOM 0x0040a70c
IDirectInputDevice8 *g_Controller;

// GLOBAL: CUSTOM 0x0040a7e8
GameConfiguration g_Config;

// GLOBAL: CUSTOM 0x0040a820
WORD g_PadState;

// FUNCTION: CUSTOM 0x00401000
static i32 Initialize(HWND hWnd)
{
    char text[256];
    GameConfiguration cfg;
    FILE *file;

    memset(&g_Config, 0, sizeof(GameConfiguration));
    g_Config.colorAddEmulation = 1;
    file = fopen("th07.cfg", "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        if (ftell(file) != sizeof(GameConfiguration))
        {
            fclose(file);
        }
        else
        {
            fseek(file, 0, SEEK_SET);

            // ZUN landmine: fread is not checked for failure.
            fread(&cfg, 1, sizeof(cfg), file);
            fclose(file);
            if (cfg.lifeCount < 5 &&
                cfg.bombCount < 4 &&
                cfg.colorMode16bit < 2 &&
                cfg.musicMode < 3 &&
                cfg.defaultDifficulty < 6 &&
                cfg.playSounds < 2 &&
                cfg.windowed < 2 &&
                cfg.frameskipConfig < 3 &&
                cfg.effectQuality < 3 &&
                cfg.slowMode < 2 &&
                cfg.shotSlow < 2 &&
                cfg.version == 0x70002)
            {
                goto skip_init;
            }
        }
    }
    memset(&cfg, 0, sizeof(cfg));
    cfg.colorMode16bit = 0;
    cfg.lifeCount = 2;
    cfg.bombCount = 3;
    cfg.version = 0x70002;
    cfg.padAxisX = 600;
    cfg.padAxisY = 600;
    file = fopen("thbgm.dat", "rb");
    if (file)
    {
        cfg.musicMode = MUSIC_WAV;
        fclose(file);
    }
    else
    {
        cfg.musicMode = MUSIC_MIDI;
    }
    cfg.windowed = 0;
    cfg.frameskipConfig = 0;
    cfg.playSounds = 1;
    cfg.defaultDifficulty = 1;
    cfg.controllerMapping = g_CustomControllerMapping;
    cfg.slowMode = 0;
    cfg.effectQuality = QUALITY_BEAUTIFUL;
    cfg.shotSlow = 1;
    cfg.colorAddEmulation = 1;

skip_init:
    g_Config = cfg;
    SendMessageA(GetDlgItem(hWnd, IDC_DISABLE_VERTEX_BUFFERS), BM_SETCHECK, g_Config.noVertexBuffers, 0);
    SendMessageA(GetDlgItem(hWnd, IDC_USE_16BIT_TEX), BM_SETCHECK, g_Config.use16BitTextures, 0);
    SendMessageA(GetDlgItem(hWnd, IDC_DISABLE_DINPUT), BM_SETCHECK, g_Config.disableDinput, 0);
    SendMessageA(GetDlgItem(hWnd, IDC_DISABLE_TEXTURE_BLEND), BM_SETCHECK, g_Config.disableTextureBlend, 0);
    SendMessageA(GetDlgItem(hWnd, IDC_USE_REF_RASTERIZER), BM_SETCHECK, g_Config.forceReferenceRender, 0);
    SendMessageA(GetDlgItem(hWnd, IDC_DISABLE_DEPTH_TEST), BM_SETCHECK, g_Config.disableZBuffer, 0);
    SendMessageA(GetDlgItem(hWnd, IDC_DISABLE_FOG), BM_SETCHECK, g_Config.disableFog, 0);
    SendMessageA(GetDlgItem(hWnd, 1014), BM_SETCHECK, g_Config.unused, 0);
    SendMessageA(GetDlgItem(hWnd, IDC_REDRAW_EVERY_FRAME), BM_SETCHECK, g_Config.redrawEveryFrame, 0);
    SendMessageA(GetDlgItem(hWnd, IDC_PRELOAD_BGM), BM_SETCHECK, g_Config.preloadBgm, 0);
    SendMessageA(GetDlgItem(hWnd, IDC_DISABLE_VSYNC), BM_SETCHECK, g_Config.disableVsync, 0);

    if (!g_Config.windowed)
    {
        SendMessageA(GetDlgItem(hWnd, IDC_FULLSCREEN), BM_SETCHECK, BST_CHECKED, 0);
    }
    else
    {
        SendMessageA(GetDlgItem(hWnd, IDC_WINDOWED), BM_SETCHECK, BST_CHECKED, 0);
    }

    if (g_Config.frameskipConfig == 0)
    {
        SendMessageA(GetDlgItem(hWnd, IDC_FRAMESKIP_NONE), BM_SETCHECK, BST_CHECKED, 0);
    }
    else if (g_Config.frameskipConfig == 1)
    {
        SendMessageA(GetDlgItem(hWnd, IDC_FRAMESKIP_HALF), BM_SETCHECK, BST_CHECKED, 0);
    }
    else
    {
        SendMessageA(GetDlgItem(hWnd, IDC_FRAMESKIP_THIRD), BM_SETCHECK, BST_CHECKED, 0);
    }

    if (!g_Config.colorMode16bit)
    {
        SendMessageA(GetDlgItem(hWnd, IDC_COLOR_MODE_32BIT), BM_SETCHECK, BST_CHECKED, 0);
    }
    else
    {
        SendMessageA(GetDlgItem(hWnd, IDC_COLOR_MODE_16BIT), BM_SETCHECK, BST_CHECKED, 0);
    }

    if (g_Config.effectQuality == QUALITY_WORST)
    {
        SendMessageA(GetDlgItem(hWnd, IDC_QUALITY_WORST), BM_SETCHECK, BST_CHECKED, 0);
    }
    else if (g_Config.effectQuality == QUALITY_MEDIUM)
    {
        SendMessageA(GetDlgItem(hWnd, IDC_QUALITY_MEDIUM), BM_SETCHECK, BST_CHECKED, 0);
    }
    else
    {
        SendMessageA(GetDlgItem(hWnd, IDC_QUALITY_BEAUTIFUL), BM_SETCHECK, BST_CHECKED, 0);
    }

    sprintf(text, "%d", (i32)g_Config.padAxisX);
    SetDlgItemTextA(hWnd, IDC_JOY_DEADZONE_X, text);
    sprintf(text, "%d", (i32)g_Config.padAxisY);
    SetDlgItemTextA(hWnd, IDC_JOY_DEADZONE_Y, text);
    return 0;
}

// FUNCTION: CUSTOM 0x004013c1
u16 GetControllerState(u16 buttons)
{
    DIJOYSTATE2 js;
    i32 retryCount;
    HRESULT hr;

    if (!g_Controller)
    {
        return buttons;
    }

    if (FAILED(hr = g_Controller->Poll()))
    {
        retryCount = 0;
        hr = g_Controller->Acquire();
        while (hr == DIERR_INPUTLOST)
        {
            hr = g_Controller->Acquire();
            retryCount++;
            if (retryCount >= 400)
            {
                return buttons;
            }
        }
    }

    g_Controller->GetDeviceState(sizeof(DIJOYSTATE2), &js);

    // ZUN landmine: hr holds the result of Poll, not GetDeviceState
    //
    // This is basically exactly the same thing as in
    // Controller::GetControllerState
    if (FAILED(hr))
    {
        return buttons;
    }
    buttons |= js.lX > g_Config.padAxisX ? 0x80 : 0;
    buttons |= js.lX < -g_Config.padAxisX ? 0x40 : 0;
    buttons |= js.lY > g_Config.padAxisY ? 0x20 : 0;
    buttons |= js.lY < -g_Config.padAxisY ? 0x10 : 0;
    return buttons;
}

// FUNCTION: CUSTOM 0x004014b2
u16 GetJoystickState()
{
    return GetControllerState(0);
}

// FUNCTION: CUSTOM 0x004014bb
INT_PTR __stdcall DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    char str[256];

    switch (uMsg)
    {
    case WM_TIMER: {
        WORD oldPadState = g_PadState;

        GetDlgItemTextA(hWnd, IDC_JOY_DEADZONE_X, str, 6);
        long padAxisX = atol(str);
        if (padAxisX < 0)
        {
            padAxisX = 1;
        }
        else if (padAxisX > 1000)
        {
            padAxisX = 1000;
        }
        else
        {
            goto skip1;
        }

        // ZUN bloat: Why woudl you sprintf over it twice ??????
        sprintf(str, "%d", padAxisX);
        sprintf(str, "%d", padAxisX);
        SetDlgItemTextA(hWnd, IDC_JOY_DEADZONE_X, str);

    skip1:
        g_Config.padAxisX = (i16)padAxisX;

        GetDlgItemTextA(hWnd, IDC_JOY_DEADZONE_Y, str, 6);
        long padAxisY = atol(str);
        if (padAxisY < 0)
        {
            padAxisY = 1;
        }
        else if (padAxisY > 1000)
        {
            padAxisY = 1000;
        }
        else
        {
            goto skip2;
        }

        // ZUN bloat: AND YOU DI HTE SAME THING aGAIN????????
        sprintf(str, "%d", padAxisY);
        sprintf(str, "%d", padAxisY);
        SetDlgItemTextA(hWnd, IDC_JOY_DEADZONE_Y, str);

    skip2:
        str[0] = '\0';
        g_Config.padAxisY = (short)padAxisY;

        g_PadState = GetJoystickState();

        if (g_PadState != oldPadState)
        {
            if (g_PadState & 0x10)
            {
                strcat(str, TH_CUSTOM_PAD_UP);
            }
            if (g_PadState & 0x20)
            {
                strcat(str, TH_CUSTOM_PAD_DOWN);
            }
            if (g_PadState & 0x40)
            {
                strcat(str, TH_CUSTOM_PAD_LEFT);
            }
            if (g_PadState & 0x80)
            {
                strcat(str, TH_CUSTOM_PAD_RIGHT);
            }
            SetDlgItemTextA(hWnd, IDC_ACTIVE_JOY_DIR, str);
        }

        KillTimer(hWnd, 0x7FFF);
        SetTimer(hWnd, 0x7FFF, 10, NULL);
        return TRUE;
    }
    case WM_INITDIALOG:
        Initialize(hWnd);
        SetTimer(hWnd, 0x7FFF, 10, NULL);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_OK: {
            if (IsDlgButtonChecked(hWnd, IDC_DISABLE_VERTEX_BUFFERS) == BST_CHECKED)
            {
                g_Config.noVertexBuffers = 1;
            }
            else
            {
                g_Config.noVertexBuffers = 0;
            }

            if (IsDlgButtonChecked(hWnd, IDC_USE_16BIT_TEX) == BST_CHECKED)
            {
                g_Config.use16BitTextures = 1;
            }
            else
            {
                g_Config.use16BitTextures = 0;
            }

            if (IsDlgButtonChecked(hWnd, IDC_DISABLE_DINPUT) == BST_CHECKED)
            {
                g_Config.disableDinput = 1;
            }
            else
            {
                g_Config.disableDinput = 0;
            }

            if (IsDlgButtonChecked(hWnd, IDC_DISABLE_TEXTURE_BLEND) == BST_CHECKED)
            {
                g_Config.disableTextureBlend = 1;
            }
            else
            {
                g_Config.disableTextureBlend = 0;
            }

            if (IsDlgButtonChecked(hWnd, IDC_USE_REF_RASTERIZER) == BST_CHECKED)
            {
                g_Config.forceReferenceRender = 1;
            }
            else
            {
                g_Config.forceReferenceRender = 0;
            }

            if (IsDlgButtonChecked(hWnd, IDC_DISABLE_DEPTH_TEST) == BST_CHECKED)
            {
                g_Config.disableZBuffer = 1;
            }
            else
            {
                g_Config.disableZBuffer = 0;
            }

            if (IsDlgButtonChecked(hWnd, IDC_DISABLE_FOG) == BST_CHECKED)
            {
                g_Config.disableFog = 1;
            }
            else
            {
                g_Config.disableFog = 0;
            }

            if (IsDlgButtonChecked(hWnd, 1014) == BST_CHECKED)
            {
                g_Config.unused = 1;
            }
            else
            {
                g_Config.unused = 0;
            }

            g_Config.windowed = IsDlgButtonChecked(hWnd, IDC_FULLSCREEN) != BST_CHECKED;

            if (IsDlgButtonChecked(hWnd, IDC_FRAMESKIP_NONE) == BST_CHECKED)
            {
                g_Config.frameskipConfig = 0;
            }
            else
            {
                g_Config.frameskipConfig = IsDlgButtonChecked(hWnd, IDC_FRAMESKIP_HALF) != BST_CHECKED ? 2 : 1;
            }

            g_Config.colorMode16bit = IsDlgButtonChecked(hWnd, IDC_COLOR_MODE_32BIT) != BST_CHECKED;

            if (IsDlgButtonChecked(hWnd, IDC_REDRAW_EVERY_FRAME) == BST_CHECKED)
            {
                g_Config.redrawEveryFrame = 1;
            }
            else
            {
                g_Config.redrawEveryFrame = 0;
            }

            if (IsDlgButtonChecked(hWnd, IDC_QUALITY_WORST) == BST_CHECKED)
            {
                g_Config.effectQuality = 0;
            }
            else
            {
                g_Config.effectQuality = IsDlgButtonChecked(hWnd, IDC_QUALITY_MEDIUM) != BST_CHECKED ? 2 : 1;
            }

            if (IsDlgButtonChecked(hWnd, IDC_PRELOAD_BGM) == BST_CHECKED)
            {
                g_Config.preloadBgm = 1;
            }
            else
            {
                g_Config.preloadBgm = 0;
            }

            if (IsDlgButtonChecked(hWnd, IDC_DISABLE_VSYNC) == BST_CHECKED)
            {
                g_Config.disableVsync = 1;
            }
            else
            {
                g_Config.disableVsync = 0;
            }

            GetDlgItemTextA(hWnd, IDC_JOY_DEADZONE_X, str, 6);
            long padAxisX = atol(str);
            if (padAxisX < 0)
            {
                padAxisX = 1;
            }
            else if (padAxisX > 1000)
            {
                padAxisX = 1000;
            }
            g_Config.padAxisX = (short)padAxisX;

            GetDlgItemTextA(hWnd, IDC_JOY_DEADZONE_Y, str, 6);
            long padAxisY = atol(str);
            if (padAxisY < 0)
            {
                padAxisY = 1;
            }
            else if (padAxisY > 1000)
            {
                padAxisY = 1000;
            }
            g_Config.padAxisY = (i16)padAxisY;

            FILE *file = fopen("th07.cfg", "wb");
            if (file == NULL)
            {
                MessageBoxA(hWnd, TH_CUSTOM_ERR_FILE_WRITE, "error", MB_ICONHAND | MB_OK);
            }
            else
            {
                fwrite(&g_Config, sizeof(GameConfiguration), 1, file);
                fclose(file);
                MessageBoxA(hWnd, TH_CUSTOM_CONFIG_WRITTEN, TH_CUSTOM_CONFIRM, MB_ICONASTERISK | MB_OK);
            }
            EndDialog(hWnd, 1);
            return TRUE;
        }
        case IDC_CANCEL:
            EndDialog(hWnd, 1);
            return TRUE;
        }
    }

    return FALSE;
}
