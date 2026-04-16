#pragma once

#include <d3d8.h>

#include "inttypes.hpp"

typedef enum RenderResult
{
    RENDER_RESULT_EXIT_SUCCESS_2 = -1,
    RENDER_RESULT_KEEP_RUNNING = 0,
    RENDER_RESULT_EXIT_SUCCESS = 1,
    RENDER_RESULT_EXIT_ERROR = 2
} RenderResult;

#pragma pack(push, 4)
struct GameWindow
{
    static i32 CheckForRunningGameInstance();
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
    i32 lastActiveAppValue;
    i32 isAppActive;
    i8 curFrame;
    // pad 3
    LARGE_INTEGER lpFrequency;
    bool usesRelativePath;
    // pad 3
    u32 screen_save_active;
    u32 low_power_active;
    u32 power_off_active;
};
#pragma pack(pop)

C_ASSERT(sizeof(GameWindow) == 0x2c);

extern GameWindow g_GameWindow;
