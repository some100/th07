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

#define KEY_PRESSED(scancode, thButton)                                        \
  ((keyboardState[scancode] & 0x80) != 0 ? thButton : 0)
#define JOYSTICK_MIDPOINT(min, max) ((min + max) / 2)

// FUNCTION: TH07 0x00430290
bool Controller::GetJoystickCaps()

{
  MMRESULT MVar1;
  joyinfoex_tag joyinfo;

  joyinfo.dwSize = 0x34;
  joyinfo.dwFlags = 0xff;
  MVar1 = joyGetPosEx(0, &joyinfo);
  if (MVar1 == 0) {
    joyGetDevCapsA(0, &g_JoystickCaps, 0x194);
  } else {
    // STRING: TH07 0x00497d9c
    g_GameErrorContext.Log("使えるパッドが存在しないようです、残念\r\n");
  }
  return MVar1 != 0;
}

// FUNCTION: TH07 0x004302f0
u32 Controller::SetButtonFromDirectInputJoystate(u16 *outButtons,
                                                 i16 controllerButtonToTest,
                                                 u32 touhouButton,
                                                 u8 *inputButtons)

{
  u32 local_14;
  u16 local_10;

  if (controllerButtonToTest < 0) {
    local_14 = 0;
  } else {
    if ((inputButtons[controllerButtonToTest] & 0x80) == 0) {
      local_10 = 0;
    } else {
      local_10 = (u16)touhouButton;
    }
    *outButtons |= local_10;
    if ((inputButtons[controllerButtonToTest] & 0x80) == 0) {
      local_14 = 0;
    } else {
      local_14 = touhouButton & 0xffff;
    }
  }
  return local_14;
}

// FUNCTION: TH07 0x00430370
u32 Controller::SetButtonFromControllerInputs(u16 *outButtons,
                                              i16 controllerButtonToTest,
                                              u32 touhouButton,
                                              u32 inputButtons)

{
  u32 mask;
  u32 local_18;
  u16 local_14;

  if (controllerButtonToTest < 0) {
    local_18 = 0;
  } else {
    mask = 1 << ((u8)controllerButtonToTest & 0x1f);
    if ((inputButtons & mask) == 0) {
      local_14 = 0;
    } else {
      local_14 = (u16)touhouButton;
    }
    *outButtons = *outButtons | local_14;
    if ((inputButtons & mask) == 0) {
      local_18 = 0;
    } else {
      local_18 = touhouButton & 0xffff;
    }
  }
  return local_18;
}

// FUNCTION: TH07 0x004303f0
u16 Controller::GetControllerInput(u16 buttons)

{
  MMRESULT joyResult;
  u32 DVar1;
  u32 joystickXDistance;
  u32 joystickYDistance;
  i32 retryCount;
  DIJOYSTATE2 js;
  i32 hr;
  joyinfoex_tag pji;

  if (g_Supervisor.controller == NULL) {
    memset(&pji, 0, sizeof(joyinfoex_tag));
    pji.dwSize = 0x34;
    pji.dwFlags = 0xff;
    joyResult = joyGetPosEx(0, &pji);
    if (joyResult == 0) {
      DVar1 = SetButtonFromControllerInputs(
          &buttons, g_Supervisor.cfg.controllerMapping.shootButton,
          TH_BUTTON_SHOOT, pji.dwButtons);
      if (g_Supervisor.cfg.shotSlow != 0) {
        if (DVar1 == 0) {
          if (g_FocusButtonConflictState < 0xb) {
            g_FocusButtonConflictState = 0;
          } else {
            g_FocusButtonConflictState = g_FocusButtonConflictState - 10;
            buttons |= TH_BUTTON_FOCUS;
          }
        } else {
          if (g_FocusButtonConflictState < 0x14) {
            g_FocusButtonConflictState = g_FocusButtonConflictState + 1;
          }
          if (9 < g_FocusButtonConflictState) {
            buttons |= TH_BUTTON_FOCUS;
          }
        }
      }
      SetButtonFromControllerInputs(
          &buttons, g_Supervisor.cfg.controllerMapping.bombButton,
          TH_BUTTON_BOMB, pji.dwButtons);
      SetButtonFromControllerInputs(
          &buttons, g_Supervisor.cfg.controllerMapping.focusButton,
          TH_BUTTON_FOCUS, pji.dwButtons);
      SetButtonFromControllerInputs(
          &buttons, g_Supervisor.cfg.controllerMapping.menuButton,
          TH_BUTTON_MENU, pji.dwButtons);
      SetButtonFromControllerInputs(&buttons,
                                    g_Supervisor.cfg.controllerMapping.upButton,
                                    TH_BUTTON_UP, pji.dwButtons);
      SetButtonFromControllerInputs(
          &buttons, g_Supervisor.cfg.controllerMapping.downButton,
          TH_BUTTON_DOWN, pji.dwButtons);
      SetButtonFromControllerInputs(
          &buttons, g_Supervisor.cfg.controllerMapping.leftButton,
          TH_BUTTON_LEFT, pji.dwButtons);
      SetButtonFromControllerInputs(
          &buttons, g_Supervisor.cfg.controllerMapping.rightButton,
          TH_BUTTON_RIGHT, pji.dwButtons);
      SetButtonFromControllerInputs(
          &buttons, g_Supervisor.cfg.controllerMapping.skipButton,
          TH_BUTTON_SKIP, pji.dwButtons);
      joystickXDistance = (g_JoystickCaps.wXmax - g_JoystickCaps.wXmin) / 4;
      joystickYDistance = (g_JoystickCaps.wYmax - g_JoystickCaps.wYmin) / 4;
      buttons |= JOYSTICK_MIDPOINT(g_JoystickCaps.wXmin, g_JoystickCaps.wXmax) +
                             joystickXDistance <
                         pji.dwXpos
                     ? TH_BUTTON_RIGHT
                     : 0;
      buttons |= pji.dwXpos < JOYSTICK_MIDPOINT(g_JoystickCaps.wXmin,
                                                g_JoystickCaps.wXmax) -
                                  joystickXDistance
                     ? TH_BUTTON_LEFT
                     : 0;
      buttons |= JOYSTICK_MIDPOINT(g_JoystickCaps.wYmin, g_JoystickCaps.wYmax) +
                             joystickYDistance <
                         pji.dwYpos
                     ? TH_BUTTON_DOWN
                     : 0;
      buttons |= pji.dwYpos < JOYSTICK_MIDPOINT(g_JoystickCaps.wYmin,
                                                g_JoystickCaps.wYmax) -
                                  joystickYDistance
                     ? TH_BUTTON_UP
                     : 0;
    }
  } else {
    if (FAILED(g_Supervisor.controller->Poll())) {
      retryCount = 0;
      // STRING: TH07 0x00497d80
      DebugPrint("error : DIERR_INPUTLOST\r\n");
      hr = (g_Supervisor.controller)->Acquire();
      do {
        if (hr != DIERR_INPUTLOST) {
          return buttons;
        }
        hr = (g_Supervisor.controller)->Acquire();
        // STRING: TH07 0x00497d60
        DebugPrint("error : DIERR_INPUTLOST %d\r\n", retryCount);
        retryCount = retryCount + 1;
      } while (retryCount < 400);
    } else {
      memset(&js, 0, sizeof(DIJOYSTATE2));
      if (SUCCEEDED(g_Supervisor.controller->GetDeviceState(0x110, &js))) {
        DVar1 = SetButtonFromDirectInputJoystate(
            &buttons, g_Supervisor.cfg.controllerMapping.shootButton, 1,
            js.rgbButtons);
        if (g_Supervisor.cfg.shotSlow != 0) {
          if (DVar1 == 0) {
            if (g_FocusButtonConflictState < 0xb) {
              g_FocusButtonConflictState = 0;
            } else {
              g_FocusButtonConflictState = g_FocusButtonConflictState - 10;
              buttons |= TH_BUTTON_FOCUS;
            }
          } else {
            if (g_FocusButtonConflictState < 0x14) {
              g_FocusButtonConflictState = g_FocusButtonConflictState + 1;
            }
            if (9 < g_FocusButtonConflictState) {
              buttons |= TH_BUTTON_FOCUS;
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
        SetButtonFromDirectInputJoystate(&buttons, 7, TH_BUTTON_D,
                                         js.rgbButtons);
        buttons |=
            ((js.lX <= g_Supervisor.cfg.padAxisX) - 1 & TH_BUTTON_RIGHT) |
            ((-(i32)g_Supervisor.cfg.padAxisX <= js.lX) - 1 & TH_BUTTON_LEFT) |
            ((js.lY <= g_Supervisor.cfg.padAxisY) - 1 & TH_BUTTON_DOWN) |
            ((-(i32)g_Supervisor.cfg.padAxisY <= js.lY) - 1 & TH_BUTTON_UP);
      }
    }
  }
  return buttons;
}

static u8 g_ControllerData[32 * 4];

// FUNCTION: TH07 0x004309c0
u8 *Controller::GetControllerState()

{
  HRESULT HVar2;
  i32 local_158;
  DIJOYSTATE2 local_154;
  i32 local_44;
  u32 local_40;
  u32 local_3c;
  joyinfoex_tag local_38;

  memset(g_ControllerData, 0, sizeof(g_ControllerData));
  if (g_Supervisor.controller == NULL) {
    memset(&local_38, 0, sizeof(joyinfoex_tag));
    local_38.dwSize = 0x34;
    local_38.dwFlags = 0xff;
    if (joyGetPosEx(0, &local_38) == 0) {
      local_3c = local_38.dwButtons;
      for (local_40 = 0; local_40 < 0x20; local_40 = local_40 + 1) {
        if ((local_3c & 1) != 0) {
          g_ControllerData[local_40] = 0x80;
        }
        local_3c = local_3c / 2;
      }
    }
  } else {
    HVar2 = (g_Supervisor.controller)->Poll();
    if (FAILED(HVar2)) {
      local_158 = 0;
      DebugPrint("error : DIERR_INPUTLOST\r\n");
      local_44 = (g_Supervisor.controller)->Acquire();
      do {
        if (local_44 != DIERR_INPUTLOST)
          goto LAB_00430b42;
        local_44 = (g_Supervisor.controller)->Acquire();
        local_158 = local_158 + 1;
      } while (local_158 < 400);
      DebugPrint("error : DIERR_INPUTLOST %d\r\n", local_158);
    } else {
      (g_Supervisor.controller)->GetDeviceState(0x110, &local_154);
      if (SUCCEEDED(HVar2)) {
        memcpy(g_ControllerData, local_154.rgbButtons,
               sizeof(g_ControllerData));
      }
    }
  }
LAB_00430b42:
  return g_ControllerData;
}

// FUNCTION: TH07 0x00430b50
u16 Controller::GetInput()

{
  u16 buttons;
  u8 keyboardState[256] = {0};

  if (g_Supervisor.keyboard == NULL) {
    GetKeyboardState(keyboardState);

    buttons =
        KEY_PRESSED(VK_UP, TH_BUTTON_UP) |
        KEY_PRESSED(VK_DOWN, TH_BUTTON_DOWN) |
        KEY_PRESSED(VK_LEFT, TH_BUTTON_LEFT) |
        KEY_PRESSED(VK_RIGHT, TH_BUTTON_RIGHT) |
        KEY_PRESSED(VK_NUMPAD8, TH_BUTTON_UP) |
        KEY_PRESSED(VK_NUMPAD2, TH_BUTTON_DOWN) |
        KEY_PRESSED(VK_NUMPAD4, TH_BUTTON_LEFT) |
        KEY_PRESSED(VK_NUMPAD6, TH_BUTTON_RIGHT) |
        KEY_PRESSED(VK_NUMPAD7, TH_BUTTON_UP_LEFT) |
        KEY_PRESSED(VK_NUMPAD9, TH_BUTTON_UP_RIGHT) |
        KEY_PRESSED(VK_NUMPAD1, TH_BUTTON_DOWN_LEFT) |
        KEY_PRESSED(VK_NUMPAD3, TH_BUTTON_DOWN_RIGHT) |
        KEY_PRESSED(VK_HOME, TH_BUTTON_HOME) | KEY_PRESSED('D', TH_BUTTON_D) |
        KEY_PRESSED('Z', TH_BUTTON_SHOOT) | KEY_PRESSED('X', TH_BUTTON_BOMB) |
        KEY_PRESSED(VK_SHIFT, TH_BUTTON_FOCUS) |
        KEY_PRESSED(VK_ESCAPE, TH_BUTTON_MENU) |
        KEY_PRESSED(VK_CONTROL, TH_BUTTON_SKIP) |
        KEY_PRESSED('Q', TH_BUTTON_Q) | KEY_PRESSED('S', TH_BUTTON_S) |
        KEY_PRESSED('R', TH_BUTTON_RESET) |
        KEY_PRESSED(VK_RETURN, TH_BUTTON_ENTER);
  } else {
    if ((g_Supervisor.keyboard)
            ->GetDeviceState(sizeof(keyboardState), keyboardState) ==
        DIERR_INPUTLOST) {
      (g_Supervisor.keyboard)->Acquire();
      return GetControllerInput(0);
    }
    buttons =
        KEY_PRESSED(DIK_UP, TH_BUTTON_UP) |
        KEY_PRESSED(DIK_DOWN, TH_BUTTON_DOWN) |
        KEY_PRESSED(DIK_LEFT, TH_BUTTON_LEFT) |
        KEY_PRESSED(DIK_RIGHT, TH_BUTTON_RIGHT) |
        KEY_PRESSED(DIK_NUMPAD8, TH_BUTTON_UP) |
        KEY_PRESSED(DIK_NUMPAD2, TH_BUTTON_DOWN) |
        KEY_PRESSED(DIK_NUMPAD4, TH_BUTTON_LEFT) |
        KEY_PRESSED(DIK_NUMPAD6, TH_BUTTON_RIGHT) |
        KEY_PRESSED(DIK_NUMPAD7, TH_BUTTON_UP_LEFT) |
        KEY_PRESSED(DIK_NUMPAD9, TH_BUTTON_UP_RIGHT) |
        KEY_PRESSED(DIK_NUMPAD1, TH_BUTTON_DOWN_LEFT) |
        KEY_PRESSED(DIK_NUMPAD3, TH_BUTTON_DOWN_RIGHT) |
        KEY_PRESSED(DIK_HOME, TH_BUTTON_HOME) |
        KEY_PRESSED(DIK_D, TH_BUTTON_D) | KEY_PRESSED(DIK_Z, TH_BUTTON_SHOOT) |
        KEY_PRESSED(DIK_X, TH_BUTTON_BOMB) |
        KEY_PRESSED(DIK_LSHIFT, TH_BUTTON_FOCUS) |
        KEY_PRESSED(DIK_RSHIFT, TH_BUTTON_FOCUS) |
        KEY_PRESSED(DIK_ESCAPE, TH_BUTTON_MENU) |
        KEY_PRESSED(DIK_LCONTROL, TH_BUTTON_SKIP) |
        KEY_PRESSED(DIK_RCONTROL, TH_BUTTON_SKIP) |
        KEY_PRESSED(DIK_Q, TH_BUTTON_Q) | KEY_PRESSED(DIK_S, TH_BUTTON_S) |
        KEY_PRESSED(DIK_RETURN, TH_BUTTON_ENTER) |
        KEY_PRESSED(DIK_R, TH_BUTTON_RESET);
  }
  return GetControllerInput(buttons);
}

// FUNCTION: TH07 0x004312c0
void Controller::ResetKeyboard()

{
  i32 i;
  u8 key_states[256];

  GetKeyboardState(key_states);
  for (i = 0; i < 0x100; i = i + 1) {
    key_states[i] = key_states[i] & 0x7f;
  }
  SetKeyboardState(key_states);
}
