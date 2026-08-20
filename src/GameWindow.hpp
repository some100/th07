#pragma once

#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include "ZunResult.hpp"
#include "inttypes.hpp"

extern u64 g_LastPerfCounter;

enum RenderResult
{
    RENDER_RESULT_EXIT_SUCCESS_2 = -1,
    RENDER_RESULT_KEEP_RUNNING = 0,
    RENDER_RESULT_EXIT_SUCCESS = 1,
    RENDER_RESULT_EXIT_ERROR = 2
};

struct GameWindow
{
    static i32 ChecksumExecutable();
    static ZunResult CreateGameWindow();
    static ZunResult InitInterface();
    static ZunResult InitRendering();
    static void Present();
    RenderResult Render();
    static void ResetRenderState();

    void ResetAccumulator()
    {
        this->accumulator = 0.0;
        g_LastPerfCounter = SDL_GetPerformanceCounter();
    }

    SDL_Window *window;
    i32 isAppActive;
    i8 curFrame;
    // pad 3
    i64 frequency;
    f64 accumulator = 0.0;
    bool usesRelativePath;
    // pad 3
    u32 screen_save_active;
    u32 low_power_active;
    u32 power_off_active;
};

extern GameWindow g_GameWindow;

extern f32 g_RenderAlpha;
extern bool g_SuppressAnmAdvance;
