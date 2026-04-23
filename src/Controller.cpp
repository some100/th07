#include "Controller.hpp"

#include <dinput.h>
#include <stddef.h>
#include <stdio.h>

#include "GameErrorContext.hpp"
#include "Supervisor.hpp"
#include "dsutil.hpp"
#include "inttypes.hpp"

// GLOBAL: TH07 0x0049fc88
static JOYCAPSA g_JoystickCaps;

// GLOBAL: TH07 0x0049fe1c
static u16 g_FocusButtonConflictState;

#define KEY_PRESSED(scancode, thButton) \
    ((keyboardState[scancode] & 0x80) != 0 ? thButton : 0)
#define JOYSTICK_MIDPOINT(min, max) ((min + max) / 2)

// FUNCTION: TH07 0x00430290
u16 Controller::GetJoystickCaps()
{
    joyinfoex_tag joyinfo;

    joyinfo.dwSize = 0x34;
    joyinfo.dwFlags = 0xff;
    if (joyGetPosEx(0, &joyinfo) != 0)
    {
        // STRING: TH07 0x00497d9c
        g_GameErrorContext.Log("使えるパッドが存在しないようです、残念\r\n");
        return 1;
    }
    joyGetDevCapsA(0, &g_JoystickCaps, 0x194);
    return 0;
}

// FUNCTION: TH07 0x004302f0
u32 Controller::SetButtonFromDirectInputJoystate(u16 *outButtons,
                                                 i16 controllerButtonToTest,
                                                 u32 touhouButton,
                                                 u8 *inputButtons)
{
    if (controllerButtonToTest < 0)
    {
        return 0;
    }
    *outButtons |= (inputButtons[controllerButtonToTest] & 0x80) != 0
                       ? (u16)touhouButton
                       : 0;

    return (inputButtons[controllerButtonToTest] & 0x80) != 0 ? (u16)touhouButton
                                                              : 0;
}

// FUNCTION: TH07 0x00430370
u32 Controller::SetButtonFromControllerInputs(u16 *outButtons,
                                              i16 controllerButtonToTest,
                                              u32 touhouButton,
                                              u32 inputButtons)
{
    u32 mask;

    if (controllerButtonToTest < 0)
        return 0;

    mask = 1 << (i32)controllerButtonToTest;
    *outButtons |= (inputButtons & mask) != 0 ? (u16)touhouButton : 0;
    return (inputButtons & mask) != 0 ? (u16)touhouButton : 0;
}

#pragma var_order(pji, distance, DVar1, DVar2, hr, js, retryCount)
// FUNCTION: TH07 0x004303f0
u16 Controller::GetControllerInput(u16 buttons)
{
    i32 retryCount;
    DIJOYSTATE2 js;
    i32 hr;
    u32 DVar2;
    u32 DVar1;
    u32 distance;
    JOYINFOEX pji;

    if (g_Supervisor.controller == NULL)
    {
        memset(&pji, 0, sizeof(JOYINFOEX));
        pji.dwSize = 0x34;
        pji.dwFlags = 0xff;
        if (joyGetPosEx(0, &pji) != 0)
            return buttons;

        DVar1 = SetButtonFromControllerInputs(
            &buttons, g_Supervisor.cfg.controllerMapping.shootButton,
            TH_BUTTON_SHOOT, pji.dwButtons);
        if (g_Supervisor.cfg.shotSlow != 0)
        {
            if (DVar1 != 0)
            {
                if (g_FocusButtonConflictState < 20)
                {
                    g_FocusButtonConflictState++;
                }
                if (g_FocusButtonConflictState >= 10)
                {
                    buttons |= TH_BUTTON_FOCUS;
                }
            }
            else
            {
                if (g_FocusButtonConflictState > 0xa)
                {
                    g_FocusButtonConflictState = g_FocusButtonConflictState - 10;
                    buttons |= TH_BUTTON_FOCUS;
                }
                else
                {
                    g_FocusButtonConflictState = 0;
                }
            }
        }
        SetButtonFromControllerInputs(&buttons,
                                      g_Supervisor.cfg.controllerMapping.bombButton,
                                      TH_BUTTON_BOMB, pji.dwButtons);
        SetButtonFromControllerInputs(
            &buttons, g_Supervisor.cfg.controllerMapping.focusButton,
            TH_BUTTON_FOCUS, pji.dwButtons);
        SetButtonFromControllerInputs(&buttons,
                                      g_Supervisor.cfg.controllerMapping.menuButton,
                                      TH_BUTTON_MENU, pji.dwButtons);
        SetButtonFromControllerInputs(&buttons,
                                      g_Supervisor.cfg.controllerMapping.upButton,
                                      TH_BUTTON_UP, pji.dwButtons);
        SetButtonFromControllerInputs(&buttons,
                                      g_Supervisor.cfg.controllerMapping.downButton,
                                      TH_BUTTON_DOWN, pji.dwButtons);
        SetButtonFromControllerInputs(&buttons,
                                      g_Supervisor.cfg.controllerMapping.leftButton,
                                      TH_BUTTON_LEFT, pji.dwButtons);
        SetButtonFromControllerInputs(
            &buttons, g_Supervisor.cfg.controllerMapping.rightButton,
            TH_BUTTON_RIGHT, pji.dwButtons);
        SetButtonFromControllerInputs(&buttons,
                                      g_Supervisor.cfg.controllerMapping.skipButton,
                                      TH_BUTTON_SKIP, pji.dwButtons);
        distance = ((g_JoystickCaps.wXmax - g_JoystickCaps.wXmin) / 2) / 2;
        buttons |= JOYSTICK_MIDPOINT(g_JoystickCaps.wXmin, g_JoystickCaps.wXmax) +
                               distance <
                           pji.dwXpos
                       ? TH_BUTTON_RIGHT
                       : 0;
        buttons |= pji.dwXpos < JOYSTICK_MIDPOINT(g_JoystickCaps.wXmin,
                                                  g_JoystickCaps.wXmax) -
                                    distance
                       ? TH_BUTTON_LEFT
                       : 0;
        distance = ((g_JoystickCaps.wYmax - g_JoystickCaps.wYmin) / 2) / 2;
        buttons |= JOYSTICK_MIDPOINT(g_JoystickCaps.wYmin, g_JoystickCaps.wYmax) +
                               distance <
                           pji.dwYpos
                       ? TH_BUTTON_DOWN
                       : 0;
        buttons |= pji.dwYpos < JOYSTICK_MIDPOINT(g_JoystickCaps.wYmin,
                                                  g_JoystickCaps.wYmax) -
                                    distance
                       ? TH_BUTTON_UP
                       : 0;
        return buttons;
    }
    else
    {
        if (FAILED(hr = g_Supervisor.controller->Poll()))
        {
            retryCount = 0;
            // STRING: TH07 0x00497d80
            DebugPrint("error : DIERR_INPUTLOST\r\n");
            hr = g_Supervisor.controller->Acquire();
            while (hr == DIERR_INPUTLOST)
            {
                hr = g_Supervisor.controller->Acquire();
                // STRING: TH07 0x00497d60
                DebugPrint("error : DIERR_INPUTLOST %d\r\n", retryCount);
                retryCount++;
                if (retryCount >= 400)
                    return buttons;
            }
            return buttons;
        }
        else
        {
            memset(&js, 0, sizeof(DIJOYSTATE2));
            if (FAILED(hr = g_Supervisor.controller->GetDeviceState(0x110, &js)))
                return buttons;

            DVar2 = SetButtonFromDirectInputJoystate(
                &buttons, g_Supervisor.cfg.controllerMapping.shootButton, 1,
                js.rgbButtons);
            if (g_Supervisor.cfg.shotSlow != 0)
            {
                if (DVar2 != 0)
                {
                    if (g_FocusButtonConflictState < 20)
                    {
                        g_FocusButtonConflictState++;
                    }
                    if (g_FocusButtonConflictState >= 10)
                    {
                        buttons |= TH_BUTTON_FOCUS;
                    }
                }
                else
                {
                    if (g_FocusButtonConflictState > 0xa)
                    {
                        g_FocusButtonConflictState -= 10;
                        buttons |= TH_BUTTON_FOCUS;
                    }
                    else
                    {
                        g_FocusButtonConflictState = 0;
                    }
                }
            }
            SetButtonFromDirectInputJoystate(
                &buttons, g_Supervisor.cfg.controllerMapping.bombButton,
                TH_BUTTON_BOMB, js.rgbButtons);
            SetButtonFromDirectInputJoystate(
                &buttons, g_Supervisor.cfg.controllerMapping.focusButton,
                TH_BUTTON_FOCUS, js.rgbButtons);
            SetButtonFromDirectInputJoystate(
                &buttons, g_Supervisor.cfg.controllerMapping.menuButton,
                TH_BUTTON_MENU, js.rgbButtons);
            SetButtonFromDirectInputJoystate(
                &buttons, g_Supervisor.cfg.controllerMapping.upButton, TH_BUTTON_UP,
                js.rgbButtons);
            SetButtonFromDirectInputJoystate(
                &buttons, g_Supervisor.cfg.controllerMapping.downButton,
                TH_BUTTON_DOWN, js.rgbButtons);
            SetButtonFromDirectInputJoystate(
                &buttons, g_Supervisor.cfg.controllerMapping.leftButton,
                TH_BUTTON_LEFT, js.rgbButtons);
            SetButtonFromDirectInputJoystate(
                &buttons, g_Supervisor.cfg.controllerMapping.rightButton,
                TH_BUTTON_RIGHT, js.rgbButtons);
            SetButtonFromDirectInputJoystate(
                &buttons, g_Supervisor.cfg.controllerMapping.skipButton,
                TH_BUTTON_SKIP, js.rgbButtons);
            SetButtonFromDirectInputJoystate(&buttons, 7, TH_BUTTON_D, js.rgbButtons);
            buttons |= js.lX > g_Supervisor.cfg.padAxisX ? TH_BUTTON_RIGHT : 0;
            buttons |= js.lX < -g_Supervisor.cfg.padAxisX ? TH_BUTTON_LEFT : 0;
            buttons |= js.lY > g_Supervisor.cfg.padAxisY ? TH_BUTTON_DOWN : 0;
            buttons |= js.lY < -g_Supervisor.cfg.padAxisY ? TH_BUTTON_UP : 0;
        }
    }
    return buttons;
}

// GLOBAL: TH07 0x0135e218
static u8 g_ControllerData[32 * 4];

#pragma var_order(joyinfoex, joyButtonBit, joyButtonIndex, hr, dijoystate2, \
                  diRetryCount)
// FUNCTION: TH07 0x004309c0
u8 *Controller::GetControllerState()
{
    HRESULT hr;
    i32 diRetryCount;
    DIJOYSTATE2 dijoystate2;
    u32 joyButtonIndex;
    u32 joyButtonBit;
    JOYINFOEX joyinfoex;

    memset(g_ControllerData, 0, sizeof(g_ControllerData));
    if (g_Supervisor.controller == NULL)
    {
        memset(&joyinfoex, 0, sizeof(JOYINFOEX));
        joyinfoex.dwSize = 0x34;
        joyinfoex.dwFlags = 0xff;
        if (joyGetPosEx(0, &joyinfoex) != 0)
            return g_ControllerData;

        for (joyButtonBit = joyinfoex.dwButtons, joyButtonIndex = 0;
             joyButtonIndex < 32; joyButtonIndex += 1, joyButtonBit >>= 1)
        {
            if ((joyButtonBit & 1) != 0)
            {
                g_ControllerData[joyButtonIndex] = 0x80;
            }
        }
        return g_ControllerData;
    }
    else
    {
        if (FAILED(hr = g_Supervisor.controller->Poll()))
        {
            diRetryCount = 0;
            DebugPrint("error : DIERR_INPUTLOST\r\n");
            hr = g_Supervisor.controller->Acquire();
            while (hr == DIERR_INPUTLOST)
            {
                hr = g_Supervisor.controller->Acquire();
                diRetryCount++;
                if (diRetryCount >= 400)
                {
                    DebugPrint("error : DIERR_INPUTLOST %d\r\n", diRetryCount);
                    return g_ControllerData;
                }
            }
            return g_ControllerData;
        }
        else
        {
            g_Supervisor.controller->GetDeviceState(sizeof(DIJOYSTATE2),
                                                    &dijoystate2);
            // ZUN uses a stale hr here it seems
            if (FAILED(hr))
                return g_ControllerData;

            memcpy(g_ControllerData, dijoystate2.rgbButtons,
                   sizeof(g_ControllerData));
            return g_ControllerData;
        }
    }
}

// FUNCTION: TH07 0x00430b50
u16 Controller::GetInput()
{
    u8 keyboardState[256];

    u16 buttons = 0;

    if (g_Supervisor.keyboard == NULL)
    {
        GetKeyboardState(keyboardState);

        buttons |= KEY_PRESSED(VK_UP, TH_BUTTON_UP);
        buttons |= KEY_PRESSED(VK_DOWN, TH_BUTTON_DOWN);
        buttons |= KEY_PRESSED(VK_LEFT, TH_BUTTON_LEFT);
        buttons |= KEY_PRESSED(VK_RIGHT, TH_BUTTON_RIGHT);
        buttons |= KEY_PRESSED(VK_NUMPAD8, TH_BUTTON_UP);
        buttons |= KEY_PRESSED(VK_NUMPAD2, TH_BUTTON_DOWN);
        buttons |= KEY_PRESSED(VK_NUMPAD4, TH_BUTTON_LEFT);
        buttons |= KEY_PRESSED(VK_NUMPAD6, TH_BUTTON_RIGHT);
        buttons |= KEY_PRESSED(VK_NUMPAD7, TH_BUTTON_UP_LEFT);
        buttons |= KEY_PRESSED(VK_NUMPAD9, TH_BUTTON_UP_RIGHT);
        buttons |= KEY_PRESSED(VK_NUMPAD1, TH_BUTTON_DOWN_LEFT);
        buttons |= KEY_PRESSED(VK_NUMPAD3, TH_BUTTON_DOWN_RIGHT);
        buttons |= KEY_PRESSED(VK_HOME, TH_BUTTON_HOME);
        buttons |= KEY_PRESSED('D', TH_BUTTON_D);
        buttons |= KEY_PRESSED('Z', TH_BUTTON_SHOOT);
        buttons |= KEY_PRESSED('X', TH_BUTTON_BOMB);
        buttons |= KEY_PRESSED(VK_SHIFT, TH_BUTTON_FOCUS);
        buttons |= KEY_PRESSED(VK_ESCAPE, TH_BUTTON_MENU);
        buttons |= KEY_PRESSED(VK_CONTROL, TH_BUTTON_SKIP);
        buttons |= KEY_PRESSED('Q', TH_BUTTON_Q);
        buttons |= KEY_PRESSED('S', TH_BUTTON_S);
        buttons |= KEY_PRESSED('R', TH_BUTTON_RESET);
        buttons |= KEY_PRESSED(VK_RETURN, TH_BUTTON_ENTER);
    }
    else
    {
        HRESULT hr = g_Supervisor.keyboard->GetDeviceState(sizeof(keyboardState),
                                                           keyboardState);
        buttons = 0;
        if (hr == DIERR_INPUTLOST)
        {
            g_Supervisor.keyboard->Acquire();
            return GetControllerInput(buttons);
        }
        buttons |= KEY_PRESSED(DIK_UP, TH_BUTTON_UP);
        buttons |= KEY_PRESSED(DIK_DOWN, TH_BUTTON_DOWN);
        buttons |= KEY_PRESSED(DIK_LEFT, TH_BUTTON_LEFT);
        buttons |= KEY_PRESSED(DIK_RIGHT, TH_BUTTON_RIGHT);
        buttons |= KEY_PRESSED(DIK_NUMPAD8, TH_BUTTON_UP);
        buttons |= KEY_PRESSED(DIK_NUMPAD2, TH_BUTTON_DOWN);
        buttons |= KEY_PRESSED(DIK_NUMPAD4, TH_BUTTON_LEFT);
        buttons |= KEY_PRESSED(DIK_NUMPAD6, TH_BUTTON_RIGHT);
        buttons |= KEY_PRESSED(DIK_NUMPAD7, TH_BUTTON_UP_LEFT);
        buttons |= KEY_PRESSED(DIK_NUMPAD9, TH_BUTTON_UP_RIGHT);
        buttons |= KEY_PRESSED(DIK_NUMPAD1, TH_BUTTON_DOWN_LEFT);
        buttons |= KEY_PRESSED(DIK_NUMPAD3, TH_BUTTON_DOWN_RIGHT);
        buttons |= KEY_PRESSED(DIK_HOME, TH_BUTTON_HOME);
        buttons |= KEY_PRESSED(DIK_D, TH_BUTTON_D);
        buttons |= KEY_PRESSED(DIK_Z, TH_BUTTON_SHOOT);
        buttons |= KEY_PRESSED(DIK_X, TH_BUTTON_BOMB);
        buttons |= KEY_PRESSED(DIK_LSHIFT, TH_BUTTON_FOCUS);
        buttons |= KEY_PRESSED(DIK_RSHIFT, TH_BUTTON_FOCUS);
        buttons |= KEY_PRESSED(DIK_ESCAPE, TH_BUTTON_MENU);
        buttons |= KEY_PRESSED(DIK_LCONTROL, TH_BUTTON_SKIP);
        buttons |= KEY_PRESSED(DIK_RCONTROL, TH_BUTTON_SKIP);
        buttons |= KEY_PRESSED(DIK_Q, TH_BUTTON_Q);
        buttons |= KEY_PRESSED(DIK_S, TH_BUTTON_S);
        buttons |= KEY_PRESSED(DIK_RETURN, TH_BUTTON_ENTER);
        buttons |= KEY_PRESSED(DIK_R, TH_BUTTON_RESET);
    }
    return GetControllerInput(buttons);
}

// FUNCTION: TH07 0x004312c0
void Controller::ResetKeyboard()
{
    u8 key_states[256];

    GetKeyboardState(key_states);
    for (i32 i = 0; i < 0x100; i++)
    {
        key_states[i] = key_states[i] & 0x7f;
    }
    SetKeyboardState(key_states);
}
