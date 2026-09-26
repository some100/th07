#include "Global.hpp"

#include "Supervisor.hpp"
#include "dsutil.hpp"

#include <stddef.h>
#include <stdio.h>

#define KEY_PRESSED(scancode, thButton) \
    ((keyboardState[scancode] & 0x80) != 0 ? thButton : 0)
#define JOYSTICK_MIDPOINT(min, max) ((min + max) / 2)

// GLOBAL: TH07 0x0049fe20
Rng g_Rng;

// GLOBAL: TH07 0x004b9e64
u32 g_LastFileSize;

// GLOBAL: TH07 0x00624210
GameErrorContext g_GameErrorContext;

// GLOBAL: TH07 0x00626218
Chain g_Chain;

// GLOBAL: TH07 0x00626258
Pbg4Archive g_Pbg4Archive;

ZunMemory g_ZunMemory;

// GLOBAL: TH07 0x0049fc88
static JOYCAPSA g_JoystickCaps;

// GLOBAL: TH07 0x0049fe1c
static u16 g_AutoFocusTimer;

// FUNCTION: TH07 0x0042fa60
Chain::~Chain()
{
}

// FUNCTION: TH07 0x0042fab0
ChainElem::ChainElem()
{
    this->prev = NULL;
    this->next = NULL;
    this->callback = NULL;
    this->unkPtr = this;
    this->addedCallback = NULL;
    this->deletedCallback = NULL;
    this->priority = 0;
    this->isAllocated = 0;
}

// FUNCTION: TH07 0x0042fb20
ChainElem::~ChainElem()
{
    if (this->deletedCallback)
    {
        this->deletedCallback(this->arg);
    }
    this->prev = NULL;
    this->next = NULL;
    this->callback = NULL;
    this->addedCallback = NULL;
    this->deletedCallback = NULL;
}

// FUNCTION: TH07 0x0042fb80
Chain::Chain()
{
}

// FUNCTION: TH07 0x0042fbd0
ZunResult Chain::AddToCalcChain(ChainElem *elem, i32 priority)
{
    ZunResult result;
    ChainElem *curElem;

    curElem = &this->calcChain;
    elem->priority = priority;
    while (curElem->next)
    {
        if (curElem->priority > priority)
        {
            break;
        }
        curElem = curElem->next;
    }

    if (curElem->priority > priority)
    {
        elem->next = curElem;
        elem->prev = curElem->prev;
        if (elem->prev)
        {
            elem->prev->next = elem;
        }
        curElem->prev = elem;
    }
    else
    {
        elem->next = NULL;
        elem->prev = curElem;
        curElem->next = elem;
    }

    if (elem->addedCallback)
    {
        result = elem->addedCallback(elem->arg);
        elem->addedCallback = NULL;
        return result;
    }

    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0042fca0
ZunResult Chain::AddToDrawChain(ChainElem *elem, i32 priority)
{
    ChainElem *curElem;

    curElem = &this->drawChain;
    elem->priority = priority;
    while (curElem->next)
    {
        if (curElem->priority > priority)
        {
            break;
        }
        curElem = curElem->next;
    }

    if (curElem->priority > priority)
    {
        elem->next = curElem;
        elem->prev = curElem->prev;
        if (elem->prev)
        {
            elem->prev->next = elem;
        }
        curElem->prev = elem;
    }
    else
    {
        elem->next = NULL;
        elem->prev = curElem;
        curElem->next = elem;
    }

    if (elem->addedCallback)
    {
        return elem->addedCallback(elem->arg);
    }
    else
    {
        return ZUN_SUCCESS;
    }
}

// FUNCTION: TH07 0x0042fd60
i32 Chain::RunCalcChain()
{
    ChainElem *next;
    ChainElem *current;
    i32 updateCount;

restart_from_first_job:
    updateCount = 0;
    current = &this->calcChain;
    while (current)
    {
        if (current->callback)
        {
        execute_again:
            switch (current->callback(current->arg))
            {
            case CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB:
                next = current;
                current = current->next;
                Cut(next);
                updateCount++;
                continue;
            case CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN:
                goto execute_again;
            case CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS:
                return 0;
            case CHAIN_CALLBACK_RESULT_BREAK:
                return 1;
            case CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR:
                return -1;
            case CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB:
                goto restart_from_first_job;
            default:
                break;
            }
            updateCount++;
        }
        current = current->next;
    }
    return updateCount;
}

// FUNCTION: TH07 0x0042fe20
i32 Chain::RunDrawChain()
{
    ChainElem *next;
    ChainElem *current;
    i32 updateCount;

    updateCount = 0;
    current = &this->drawChain;
    while (current)
    {
        if (current->callback)
        {
        execute_again:
            switch (current->callback(current->arg))
            {
            case CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB:
                next = current;
                current = current->next;
                Cut(next);
                updateCount++;
                continue;
            case CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN:
                goto execute_again;
            case CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS:
                return 0;
            case CHAIN_CALLBACK_RESULT_BREAK:
                return 1;
            case CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR:
                return -1;
            default:
                break;
            }
            updateCount++;
        }
        current = current->next;
    }
    return updateCount;
}

#pragma var_order(curElem, tmp2, tmp, nextRootElem)
// FUNCTION: TH07 0x0042fee0
void Chain::ReleaseSingleChain(ChainElem *root)
{
    ChainElem nextRootElem;
    ChainElem *tmp2;
    ChainElem *tmp;
    ChainElem *curElem;

    tmp = ZUN_NEW(ChainElem, "funcChainInf");
    nextRootElem.next = tmp;
    curElem = root;
    while (curElem)
    {
        tmp->unkPtr = curElem;
        tmp->next = ZUN_NEW(ChainElem, "funcChainInf");
        tmp = tmp->next;
        curElem = curElem->next;
    }
    curElem = &nextRootElem;
    while (curElem)
    {
        Cut(curElem->unkPtr);
        curElem = curElem->next;
    }
    tmp = nextRootElem.next;
    while (tmp)
    {
        tmp2 = tmp->next;
        ZUN_DELETE(tmp);
        tmp = tmp2;
    }
}

// FUNCTION: TH07 0x00430060
void Chain::Release()
{
    ReleaseSingleChain(&this->calcChain);
    ReleaseSingleChain(&this->drawChain);
}

// FUNCTION: TH07 0x00430090
ChainElem *Chain::CreateElem(ChainCallback callback)
{
    ChainElem *elem = ZUN_NEW(ChainElem, "funcChainInf");
    elem->callback = callback;
    elem->addedCallback = NULL;
    elem->deletedCallback = NULL;
    elem->isAllocated = 1;
    return elem;
}

// FUNCTION: TH07 0x00430140
void Chain::Cut(ChainElem *toRemove)
{
    BOOL isDrawChain;
    ChainElem *curElem;

    isDrawChain = FALSE;

    if (!toRemove)
    {
        return;
    }

    curElem = &this->calcChain;
    while (curElem)
    {
        if (curElem == toRemove)
        {
            goto destroy_elem;
        }
        curElem = curElem->next;
    }
    isDrawChain = TRUE;
    curElem = &this->drawChain;
    while (curElem)
    {
        if (curElem == toRemove)
        {
            goto destroy_elem;
        }
        curElem = curElem->next;
    }

    return;

destroy_elem:
    if (toRemove->prev)
    {
        toRemove->callback = NULL;
        toRemove->prev->next = toRemove->next;
        if (toRemove->next)
        {
            toRemove->next->prev = toRemove->prev;
        }
        toRemove->prev = NULL;
        toRemove->next = NULL;

        if (toRemove->isAllocated)
        {
            ZUN_DELETE(toRemove);
        }
        else
        {
            if (toRemove->deletedCallback)
            {
                ChainLifecycleCallback deletedCallback = toRemove->deletedCallback;
                toRemove->deletedCallback = NULL;
                deletedCallback(toRemove->arg);
            }
        }
    }
}

// FUNCTION: TH07 0x00430290
u16 Controller::GetJoystickCaps()
{
    JOYINFOEX joyinfo;

    joyinfo.dwSize = sizeof(JOYINFOEX);
    joyinfo.dwFlags = 255;
    if (joyGetPosEx(0, &joyinfo))
    {
        g_GameErrorContext.Log(TH_CONTROL_NO_USABLE_PADS);
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
    {
        return 0;
    }

    mask = 1 << (i32)controllerButtonToTest;
    *outButtons |= (inputButtons & mask) != 0 ? (u16)touhouButton : 0;
    return (inputButtons & mask) != 0 ? (u16)touhouButton : 0;
}

#pragma var_order(pji, distance, shootPressed, shootPressed2, hr, js, retryCount)
// FUNCTION: TH07 0x004303f0
u16 Controller::GetControllerInput(u16 buttons)
{
    i32 retryCount;
    DIJOYSTATE2 js;
    i32 hr;
    u32 shootPressed2;
    u32 shootPressed;
    u32 distance;
    JOYINFOEX pji;

    if (!g_Supervisor.controller)
    {
        memset(&pji, 0, sizeof(JOYINFOEX));
        pji.dwSize = sizeof(JOYINFOEX);
        pji.dwFlags = 255;
        if (joyGetPosEx(0, &pji))
        {
            return buttons;
        }

        shootPressed = SetButtonFromControllerInputs(
            &buttons, g_Supervisor.cfg.controllerMapping.shootButton,
            TH_BUTTON_SHOOT, pji.dwButtons);
        if (g_Supervisor.cfg.shotSlow)
        {
            if (shootPressed != 0)
            {
                if (g_AutoFocusTimer < 20)
                {
                    g_AutoFocusTimer++;
                }
                if (g_AutoFocusTimer >= 10)
                {
                    buttons |= TH_BUTTON_FOCUS;
                }
            }
            else
            {
                if (g_AutoFocusTimer > 10)
                {
                    g_AutoFocusTimer -= 10;
                    buttons |= TH_BUTTON_FOCUS;
                }
                else
                {
                    g_AutoFocusTimer = 0;
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
        distance = (g_JoystickCaps.wXmax - g_JoystickCaps.wXmin) / 2 / 2;
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
        distance = (g_JoystickCaps.wYmax - g_JoystickCaps.wYmin) / 2 / 2;
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
            utils::DebugPrint("error : DIERR_INPUTLOST\r\n");
            hr = g_Supervisor.controller->Acquire();
            while (hr == DIERR_INPUTLOST)
            {
                hr = g_Supervisor.controller->Acquire();
                // STRING: TH07 0x00497d60
                utils::DebugPrint("error : DIERR_INPUTLOST %d\r\n", retryCount);
                retryCount++;
                if (retryCount >= 400)
                {
                    return buttons;
                }
            }
            return buttons;
        }
        else
        {
            memset(&js, 0, sizeof(DIJOYSTATE2));
            if (FAILED(hr = g_Supervisor.controller->GetDeviceState(0x110, &js)))
            {
                return buttons;
            }

            shootPressed2 = SetButtonFromDirectInputJoystate(
                &buttons, g_Supervisor.cfg.controllerMapping.shootButton, 1,
                js.rgbButtons);
            if (g_Supervisor.cfg.shotSlow)
            {
                if (shootPressed2 != 0)
                {
                    if (g_AutoFocusTimer < 20)
                    {
                        g_AutoFocusTimer++;
                    }
                    if (g_AutoFocusTimer >= 10)
                    {
                        buttons |= TH_BUTTON_FOCUS;
                    }
                }
                else
                {
                    if (g_AutoFocusTimer > 10)
                    {
                        g_AutoFocusTimer -= 10;
                        buttons |= TH_BUTTON_FOCUS;
                    }
                    else
                    {
                        g_AutoFocusTimer = 0;
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
    if (!g_Supervisor.controller)
    {
        memset(&joyinfoex, 0, sizeof(JOYINFOEX));
        joyinfoex.dwSize = sizeof(JOYINFOEX);
        joyinfoex.dwFlags = 255;
        if (joyGetPosEx(0, &joyinfoex))
        {
            return g_ControllerData;
        }

        for (joyButtonBit = joyinfoex.dwButtons, joyButtonIndex = 0;
             joyButtonIndex < 32; joyButtonIndex++, joyButtonBit >>= 1)
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
            utils::DebugPrint("error : DIERR_INPUTLOST\r\n");
            hr = g_Supervisor.controller->Acquire();
            while (hr == DIERR_INPUTLOST)
            {
                hr = g_Supervisor.controller->Acquire();
                diRetryCount++;
                if (diRetryCount >= 400)
                {
                    utils::DebugPrint("error : DIERR_INPUTLOST %d\r\n", diRetryCount);
                    return g_ControllerData;
                }
            }
            return g_ControllerData;
        }
        else
        {
            g_Supervisor.controller->GetDeviceState(sizeof(DIJOYSTATE2),
                                                    &dijoystate2);
            // ZUN landmine: hr holds the result of Poll, not GetDeviceState
            if (FAILED(hr))
            {
                return g_ControllerData;
            }

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

    if (!g_Supervisor.keyboard)
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
    for (i32 i = 0; i < ARRAY_SIZE_SIGNED(key_states); i++)
    {
        key_states[i] = key_states[i] & 0x7f;
    }
    SetKeyboardState(key_states);
}

#pragma var_order(entryIdx, filename, fsize, buf, hFile)
// FUNCTION: TH07 0x00431330
u8 *FileSystem::OpenFile(const char *filepath, ZunBool isExternalResource)
{
    HANDLE hFile;
    u8 *buf;
    DWORD fsize;
    const char *filename;
    i32 entryIdx;

    entryIdx = -1;
    if (!isExternalResource)
    {
        filename = strrchr(filepath, '\\');
        if (!filename)
        {
            filename = filepath;
        }
        else
        {
            filename++;
        }

        filename = strrchr(filename, '/');
        if (!filename)
        {
            filename = filepath;
        }
        else
        {
            filename++;
        }
        fsize = g_Pbg4Archive.GetEntrySize(filename);
        g_LastFileSize = fsize;
        if (fsize == 0)
        {
            // STRING: TH07 0x00497d38
            g_GameErrorContext.Fatal("error : %s is not found in arcfile.\r\n",
                                     filename);
            return NULL;
        }
        if (fsize != 0)
        {
            // STRING: TH07 0x00497d24
            utils::DebugPrint("%s Decode ... \r\n", filename);
            buf = (u8 *)ZUN_ALLOC(fsize);
            if (!buf)
            {
                return NULL;
            }

            g_Pbg4Archive.ReadDecompressEntry(filename, buf);
            return buf;
        }
    }
    // STRING: TH07 0x00497d14
    utils::DebugPrint("%s Load ... \r\n", filepath);
    hFile = CreateFileA(filepath, GENERIC_READ, 1, NULL, 3, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        // STRING: TH07 0x00497cf8
        utils::DebugPrint("error : %s is not found.\r\n", filepath);
        return NULL;
    }

    fsize = GetFileSize(hFile, NULL);
    buf = (u8 *)ZUN_ALLOC(fsize);
    if (!buf)
    {
        CloseHandle(hFile);
        return NULL;
    }

    // ZUN landmine: ReadFile is not checked for failure.
    ReadFile(hFile, buf, fsize, &fsize, NULL);
    g_LastFileSize = fsize;
    CloseHandle(hFile);
    return buf;
}

// FUNCTION: TH07 0x004314f0
ZunBool FileSystem::CheckFileExists(const char *file)
{
    HANDLE hObject;

    hObject = CreateFileA(file, GENERIC_READ, 1, NULL, 3, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, NULL);
    if (hObject != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hObject);
        return true;
    }
    return false;
}

#pragma var_order(bytesWritten, hFile)
// FUNCTION: TH07 0x00431540
i32 FileSystem::WriteDataToFile(const char *filename, const void *out,
                                DWORD bytesToWrite)
{
    HANDLE hFile;
    DWORD bytesWritten;

    hFile = CreateFileA(filename, GENERIC_WRITE, 1, NULL, 2, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        // STRING: TH07 0x00497cdc
        utils::DebugPrint("error : %s write error\r\n", filename);
        return -1;
    }
    WriteFile(hFile, out, bytesToWrite, &bytesWritten, NULL);
    if (bytesToWrite != bytesWritten)
    {
        CloseHandle(hFile);
        utils::DebugPrint("error : %s write error\r\n", filename);
        return -2;
    }
    CloseHandle(hFile);
    // STRING: TH07 0x00497ccc
    utils::DebugPrint("%s write ...\r\n", filename);
    return 0;
}

// FUNCTION: TH07 0x004315f0
const char *GameErrorContext::Log(const char *fmt, ...)
{
    char tmp[8192];
    size_t tmpSize;
    va_list args;

    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    tmpSize = strlen(tmp);
    if (this->m_BufferEnd + tmpSize < this->m_Buffer + 0x1fff)
    {
        strcpy(this->m_BufferEnd, tmp);

        this->m_BufferEnd += tmpSize;
        *this->m_BufferEnd = '\0';
    }
    va_end(args);
    return fmt;
}

// FUNCTION: TH07 0x00431730
const char *GameErrorContext::Fatal(const char *fmt, ...)
{
    char tmp[512];
    size_t tmpSize;
    va_list args;

    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    tmpSize = strlen(tmp);
    if (this->m_BufferEnd + tmpSize < this->m_Buffer + 0x1fff)
    {
        strcpy(this->m_BufferEnd, tmp);
        this->m_BufferEnd += tmpSize;
        *this->m_BufferEnd = '\0';
    }
    va_end(args);
    this->m_ShowMessageBox = true;
    return fmt;
}

// FUNCTION: TH07 0x00431870
u16 Rng::GetRandomU16()
{
    u16 tmp;

    tmp = (this->seed ^ 0x9630) - 0x6553;
    this->seed = ((tmp & 0xc000) >> 14) + tmp * 4 & 0xFFFF;
    this->generationCount++;
    return this->seed;
}

// FUNCTION: TH07 0x004318d0
u32 Rng::GetRandomU32()
{
    return GetRandomU16() << 16 | GetRandomU16();
}

// FUNCTION: TH07 0x00431900
f32 Rng::GetRandomFloat()
{
    return (f32)GetRandomU32() / 4294967296.0f;
}

// FUNCTION: TH07 0x00431930
f32 AddNormalizeAngle(f32 a, f32 b)
{
    i32 i;

    i = 0;
    a += b;
    while (a > ZUN_PI)
    {
        a -= ZUN_2PI;
        if (i++ > 16)
        {
            break;
        }
    }
    while (a < -ZUN_PI)
    {
        a += ZUN_2PI;
        if (i++ > 16)
        {
            break;
        }
    }
    return a;
}

// FUNCTION: TH07 0x004319b0
void Rotate(Float3 *out, Float3 *point, f32 angle)
{
    f32 sinAngle;
    f32 cosAngle;

    sinAngle = sinf(angle);
    cosAngle = cosf(angle);
    out->x = cosAngle * point->x + sinAngle * point->y;
    out->y = cosAngle * point->y - sinAngle * point->x;
}
