#include <SDL3/SDL.h>
#include <cstdio>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

// pull in gameerrorcontext::flush before anmmanager::releasesurfaces
#include "AnmManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameManager.hpp"
#include "GameWindow.hpp"
#include "ResultScreen.hpp"
#include "SoundPlayer.hpp"
#include "Supervisor.hpp"
#include "Touch.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"

static i32 renderRes = RENDER_RESULT_KEEP_RUNNING;

void AnmManager::TakeScreenshotIfRequested()
{
    if (this->screenshotTextureId >= 0)
    {
        Flush();

        TakeScreenshot(this->screenshotTextureId, this->screenshotSrcLeft, this->screenshotSrcTop,
                       this->screenshotSrcWidth, this->screenshotSrcHeight, this->screenshotDstLeft,
                       this->screenshotDstTop, this->screenshotDstWidth, this->screenshotDstHeight);
        this->screenshotTextureId = -1;
    }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    if (g_Supervisor.LoadConfig("th07.cfg") != ZUN_SUCCESS)
    {
        return SDL_APP_FAILURE;
    }

    GameWindow::ChecksumExecutable();
    g_GameWindow.frequency = SDL_GetPerformanceFrequency();

start:
    if (GameWindow::CreateGameWindow())
    {
        return SDL_APP_FAILURE;
    }

    if (GameWindow::InitInterface())
    {
        return SDL_APP_FAILURE;
    }

    if (GameWindow::InitRendering())
    {
        return SDL_APP_FAILURE;
    }

    g_SoundPlayer.InitializeSound();
    Controller::ResetKeyboard();
    g_AnmManager = new AnmManager();
    if (!g_Supervisor.cfg.windowed)
    {
        SDL_HideCursor();
    }
    renderRes = g_Supervisor.RegisterChain();
    if (renderRes != ZUN_SUCCESS)
    {
        return SDL_APP_FAILURE;
    }
    renderRes = RENDER_RESULT_KEEP_RUNNING;
    g_GameWindow.curFrame = -30;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    renderRes = g_GameWindow.Render();
    if (renderRes != RENDER_RESULT_KEEP_RUNNING)
    {
        return SDL_APP_SUCCESS;
    }
    g_Supervisor.flags &= ~16;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        g_GameWindow.isAppActive = 1;
        if (!g_Supervisor.cfg.windowed)
        {
            SDL_HideCursor();
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
    case SDL_EVENT_WILL_ENTER_BACKGROUND:
    case SDL_EVENT_DID_ENTER_BACKGROUND:
        if (g_GameManager.notInMenu && !g_GameManager.isInPauseMenu)
        {
            g_GameManager.Pause();
        }
        while (g_SoundPlayer.ProcessQueues())
            ;
        Touch::CancelTouches();
        g_GameWindow.isAppActive = 0;
        SDL_ShowCursor();
        break;
    case SDL_EVENT_WILL_ENTER_FOREGROUND:
    case SDL_EVENT_DID_ENTER_FOREGROUND:
        g_GameWindow.isAppActive = 1;
        break;
    case SDL_EVENT_GAMEPAD_ADDED:
        if (!g_Supervisor.controller)
        {
            g_Supervisor.controller = SDL_OpenGamepad(event->gdevice.which);
        }
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        if (g_Supervisor.controller)
        {
            SDL_Joystick *joy = SDL_GetGamepadJoystick(g_Supervisor.controller);

            if (SDL_GetJoystickID(joy) == event->gdevice.which)
            {
                SDL_CloseGamepad(g_Supervisor.controller);
                g_Supervisor.controller = nullptr;
            }
        }
        break;
    case SDL_EVENT_FINGER_DOWN:
        Touch::FingerDown(event->tfinger);
        break;
    case SDL_EVENT_FINGER_CANCELED:
    case SDL_EVENT_FINGER_UP:
        Touch::FingerUp(event->tfinger);
        break;
    case SDL_EVENT_FINGER_MOTION:
        Touch::FingerMotion(event->tfinger);
        break;
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    if (g_GameManager.plst.base.magic != 0)
    {
        ResultScreen::RegisterChain(2);
    }
    g_Chain.Release();
    while (g_SoundPlayer.ProcessQueues())
        ;
    g_SoundPlayer.Release();

    SAFE_DELETE(g_AnmManager);
    SAFE_DELETE(g_Supervisor.gfxDevice);
    if (g_GameWindow.window)
    {
        SDL_DestroyWindow(g_GameWindow.window);
        g_GameWindow.window = NULL;
    }
    SDL_ShowCursor();
    if (renderRes == RENDER_RESULT_EXIT_ERROR)
    {
        g_GameErrorContext.m_BufferEnd = g_GameErrorContext.m_Buffer;
        *g_GameErrorContext.m_BufferEnd = '\0';
        g_GameErrorContext.Log("再起動を要するオプションが変更されたので再起動します\n");
        // we normally should restart the application but thats not really possible in this
        // model
        // however, this is really only ever used when the application is restarting after enabling
        // vsync. afaik, it should be pretty safe to just not have the application restart after
        // enabling vsync since theres nothing before the checkvsync call that needs vsyncDisabled to
        // be there beforehand
    }
    FileSystem::WriteDataToFile("th07.cfg", &g_Supervisor.cfg, sizeof(GameConfiguration));
    g_GameErrorContext.Flush();
}
