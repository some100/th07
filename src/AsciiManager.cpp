#include "AsciiManager.hpp"

#include <stdio.h>

#include "AnmManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "Player.hpp"
#include "SoundPlayer.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "utils.hpp"

// GLOBAL: TH07 0x0134cdf4
ChainElem g_AsciiManagerOnDrawMenusChain;

// GLOBAL: TH07 0x0134ce18
AsciiManager g_AsciiManager;

// GLOBAL: TH07 0x0135dfac
ChainElem g_AsciiManagerCalcChain;

// GLOBAL: TH07 0x0135dfcc
ChainElem g_AsciiManagerOnDrawPopupsChain;

// FUNCTION: TH07 0x00401400
void AsciiManager::UpdateScripts()
{
    g_AnmManager->ExecuteScript(&this->cherryGauge);
    g_AnmManager->ExecuteScript(&this->cherryDigit);
    g_AnmManager->ExecuteScript(&this->bossMarkers[0]);
    g_AnmManager->ExecuteScript(&this->bossMarkers[1]);
    g_AnmManager->ExecuteScript(&this->bossMarkers[2]);
    g_AnmManager->ExecuteScript(&this->bossMarkers[3]);
    g_AnmManager->ExecuteScript(&this->cherryBorderActive);
}

AsciiManager::AsciiManager()
{
}

RetryMenu::RetryMenu()
{
}

PauseMenu::PauseMenu()
{
}

// FUNCTION: TH07 0x004017b0
void IncrementCapped(u32 *param, u32 param_2)
{
    if (*param < 999999)
    {
        (*param)++;
    }
}

#pragma var_order(i, curPopup)
// FUNCTION: TH07 0x004017e0
u32 AsciiManager::OnUpdate(AsciiManager *arg)
{
    i32 i;
    AsciiManagerPopup *curPopup;

    if ((g_GameManager.isInRetryMenu == 0) && (g_GameManager.isInPauseMenu == 0))
    {
        curPopup = arg->popups;
        for (i = 0; i < 0x2d3; i++, curPopup++)
        {
            if (curPopup->inUse == 0)
            {
                continue;
            }

            curPopup->position.y -= 0.5f * g_Supervisor.effectiveFramerateMultiplier;
            curPopup->timer++;
            if ((curPopup->timer > 60) != 0)
            {
                curPopup->inUse = 0;
            }
        }
    }
    else if (g_GameManager.isInRetryMenu != 0)
    {
        arg->retryMenu.OnUpdate();
    }
    if (g_GameManager.isInPauseMenu != 0)
    {
        arg->pauseMenu.OnUpdate();
    }
    arg->UpdateScripts();
    if (g_GameManager.demo != 0)
    {
        if (arg->vm.anmFileIdx == 0)
        {
            g_AnmManager->SetAnmIdxAndExecuteScript(&arg->vm, 7);
        }
        g_AnmManager->ExecuteScript(&arg->vm);
    }
    else
    {
        arg->vm.anmFileIdx = 0;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00401970
u32 AsciiManager::OnDrawMenus(AsciiManager *arg)
{
    arg->DrawStrings();
    arg->numStrings = 0;
    arg->retryMenu.OnDraw();
    arg->pauseMenu.OnDraw();
    if (arg->vm.anmFileIdx != 0)
    {
        g_AnmManager->DrawNoRotation(&arg->vm);
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x004019e0
u32 AsciiManager::OnDrawPopups(AsciiManager *arg)
{
    arg->DrawPopups();
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00401a00
void AsciiManager::InitializeVms()
{
    memset(&this->vm1, 0, sizeof(AnmVm));
    memset(&this->vm0, 0, sizeof(AnmVm));
    memset(&this->strings, 0, sizeof(this->strings));
    memset(&this->retryMenu, 0, sizeof(RetryMenu));
    memset(&this->pauseMenu, 0, sizeof(PauseMenu));
    memset(&this->popups, 0, sizeof(this->popups));
    this->numStrings = 0;
    this->isGui = 0;
    this->isSelected = 0;
    this->nextPopupIndex1 = 0;
    this->nextPopupIndex2 = 0;
    this->unused_74e4 = 0;
    this->color = 0xffffffff;
    this->scale.x = 1.0f;
    this->scale.y = 1.0f;
    this->vm1.anchor = 3;
    UselessStack::FourBytes();
    g_AnmManager->InitializeAndSetActiveSprite(&this->vm1, 0);
    UselessStack::FourBytes();
    g_AnmManager->InitializeAndSetActiveSprite(&this->vm0, 0x20);
    this->vm1.pos.z = 0.1f;
    this->isSelected = 0;
    this->fontSpacing = 0xe;
    this->SetFadeState(this->uiFadeState);
}

// FUNCTION: TH07 0x00401ba0
void AsciiManager::InitializeOtherVms()
{
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->cherryGauge, 4);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->cherryDigit, 3);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->cherryBorderActive, 5);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->bossMarkers[0], 6);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->bossMarkers[1], 6);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->bossMarkers[2], 6);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->bossMarkers[3], 6);
}

// FUNCTION: TH07 0x00401d70
ZunResult AsciiManager::AddedCallback(AsciiManager *arg)
{
    memset(arg, 0, sizeof(AsciiManager));
    if (g_AnmManager->LoadAnms(1, "data/ascii.anm", 0) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    if (g_AnmManager->LoadAnms(4, "data/capture.anm", 0x724) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    arg->InitializeVms();
    arg->InitializeOtherVms();
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00401de0
ZunResult AsciiManager::DeletedCallback(AsciiManager *arg)
{
    g_AnmManager->ReleaseAnm(1);
    g_AnmManager->ReleaseAnm(2);
    g_AnmManager->ReleaseAnm(4);
    g_AnmManager->ReleaseAnm(3);
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00401e30
ZunResult AsciiManager::RegisterChain()
{
    AsciiManager *mgr = &g_AsciiManager;
    g_AsciiManagerCalcChain.callback = (ChainCallback)OnUpdate;
    g_AsciiManagerCalcChain.addedCallback = NULL;
    g_AsciiManagerCalcChain.deletedCallback = NULL;
    g_AsciiManagerCalcChain.addedCallback = (ChainLifecycleCallback)AddedCallback;
    g_AsciiManagerCalcChain.deletedCallback =
        (ChainLifecycleCallback)DeletedCallback;
    g_AsciiManagerCalcChain.arg = mgr;

    if (g_Chain.AddToCalcChain(&g_AsciiManagerCalcChain, 1) != 0)
    {
        return ZUN_ERROR;
    }

    g_AsciiManagerOnDrawMenusChain.callback = (ChainCallback)OnDrawMenus;
    g_AsciiManagerOnDrawMenusChain.addedCallback = NULL;
    g_AsciiManagerOnDrawMenusChain.deletedCallback = NULL;
    g_AsciiManagerOnDrawMenusChain.arg = mgr;
    g_Chain.AddToDrawChain(&g_AsciiManagerOnDrawMenusChain, 0x10);
    g_AsciiManagerOnDrawPopupsChain.callback = (ChainCallback)OnDrawPopups;
    g_AsciiManagerOnDrawPopupsChain.addedCallback = NULL;
    g_AsciiManagerOnDrawPopupsChain.deletedCallback = NULL;
    g_AsciiManagerOnDrawPopupsChain.arg = mgr;
    g_Chain.AddToDrawChain(&g_AsciiManagerOnDrawPopupsChain, 0xb);
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00401f10
void AsciiManager::CutChain()
{
    g_Chain.Cut(&g_AsciiManagerCalcChain);
    g_Chain.Cut(&g_AsciiManagerOnDrawMenusChain);
    // forgot something? popups?
}

// FUNCTION: TH07 0x00401f40
void AsciiManager::AddString(D3DXVECTOR3 *param_1, const char *text)
{
    if (this->numStrings >= 0x100)
    {
        return;
    }

    AsciiManagerString *curString = &this->strings[this->numStrings];
    this->numStrings++;
    strcpy(curString->text, text);
    curString->position = *param_1;
    curString->color = this->color;
    curString->scale.x = this->scale.x;
    curString->scale.y = this->scale.y;
    curString->isGui = this->isGui;

    if ((g_Supervisor.cfg.opts & 1) | (g_Supervisor.cfg.opts >> 8 & 1))
    {
        curString->isSelected = this->isSelected;
    }
    else
    {
        curString->isSelected = 0;
    }
}

// FUNCTION: TH07 0x00402060
void AsciiManager::AddFormatText(AsciiManager *manager, D3DXVECTOR3 *position,
                                 const char *fmt, ...)
{
    char str[508];
    va_list args;

    va_start(args, fmt);
    vsprintf(str, fmt, args);
    manager->AddString(position, str);

    va_end(args);
}

#pragma var_order(charWidth, i, string, text, guiString, idk)
// FUNCTION: TH07 0x004020b0
void AsciiManager::DrawStrings()
{
    i32 idk[3];
    f32 charWidth;
    i32 guiString;
    char *text;
    AsciiManagerString *string;
    i32 i;

    guiString = 1;
    string = this->strings;
    this->vm0.visible = 1;
    this->vm0.anchor = 3;
    for (i = 0; i < this->numStrings; i++, string++)
    {
        this->vm0.pos = string->position;
        text = string->text;
        this->vm0.scale.x = string->scale.x;
        this->vm0.scale.y = string->scale.y;
        charWidth = (f32)this->fontSpacing * string->scale.x;
        if (guiString != string->isGui)
        {
            guiString = string->isGui;
            g_AnmManager->Flush();
            if (guiString != 0)
            {
                g_Supervisor.viewport.X = g_GameManager.arcadeRegionTopLeftPos.x;
                g_Supervisor.viewport.Y = g_GameManager.arcadeRegionTopLeftPos.y;
                g_Supervisor.viewport.Width = g_GameManager.arcadeRegionSize.x;
                g_Supervisor.viewport.Height = g_GameManager.arcadeRegionSize.y;
                g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
            }
            else
            {
                g_Supervisor.viewport.X = 0;
                g_Supervisor.viewport.Y = 0;
                g_Supervisor.viewport.Width = 0x280;
                g_Supervisor.viewport.Height = 0x1e0;
                g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
            }
        }
        while (*(u8 *)text != 0)
        {
            if (*(u8 *)text == '\n')
            {
                this->vm0.pos.y += 16.0f * string->scale.y;
                this->vm0.pos.x = string->position.x;
            }
            else if (*(u8 *)text == ' ')
            {
                this->vm0.pos.x += charWidth;
            }
            else
            {
                if (string->isSelected == 0)
                {
                    this->vm0.sprite = &g_AnmManager->sprites[(u8)*text - 1];
                    this->vm0.color.color = string->color;
                }
                else
                {
                    this->vm0.sprite = &g_AnmManager->sprites[(u8)*text + 0x7c];
                    this->vm0.color.color = 0xffffffff;
                }
                g_AnmManager->DrawNoRotation(&this->vm0);
                this->vm0.pos.x += charWidth;
            }
            text++;
        }
    }
    for (i = 0; i < 4; i++)
    {
        if ((this->bossMarkers[i].pos.x >= 56.0f) &&
            (this->bossMarkers[i].pos.x <= 392.0f))
        {
            charWidth = fabsf((this->bossMarkers[i].pos.x - 32.0f) -
                              g_Player.positionCenter.x);
            if (charWidth < 64.0f)
            {
                this->bossMarkers[i].color.bytes.a =
                    ((charWidth * 128.0f) / 64.0f + 48.0f);
            }
            else
            {
                this->bossMarkers[i].color.bytes.a = 0xb0;
            }
            if (this->bossDamageTint[i] != 0)
            {
                this->bossMarkers[i].color.bytes.a = 0x80;
                this->bossMarkers[i].color.bytes.r = 0x40;
                this->bossMarkers[i].color.bytes.g = 0x40;
                this->bossMarkers[i].color.bytes.b = 0xff;
            }
            else
            {
                this->bossMarkers[i].color.bytes.r = 0xff;
                this->bossMarkers[i].color.bytes.g = 0xff;
                this->bossMarkers[i].color.bytes.b = 0xff;
            }
            g_AnmManager->DrawNoRotation(&this->bossMarkers[i]);
        }
    }
}

// FUNCTION: TH07 0x004024f0
void AsciiManager::CreatePopup1(D3DXVECTOR3 *position, i32 value,
                                D3DCOLOR color)
{
    i32 characterCount;
    AsciiManagerPopup *popup;

    if (this->nextPopupIndex1 >= 0x2d0)
    {
        this->nextPopupIndex1 = 0;
    }
    popup = this->popups + this->nextPopupIndex1;
    popup->inUse = 1;
    characterCount = 0;
    if (value >= 0)
    {
        while (value != 0)
        {
            popup->digits[characterCount++] = (char)(value % 10);
            value /= 10;
        }
    }
    else
    {
        popup->digits[characterCount++] = '\n';
    }
    if (characterCount == 0)
    {
        popup->digits[characterCount++] = 0;
    }
    popup->characterCount = (u8)characterCount;
    popup->color = color;
    popup->timer.InitializeForPopup();
    popup->position = *position;
    popup->position.x += g_GameManager.arcadeRegionTopLeftPos.x;
    popup->position.y += g_GameManager.arcadeRegionTopLeftPos.y;
    this->nextPopupIndex1++;
}

// FUNCTION: TH07 0x00402630
void AsciiManager::CreatePopup2(D3DXVECTOR3 *position, i32 value,
                                D3DCOLOR color)
{
    i32 characterCount;
    AsciiManagerPopup *popup;

    if (this->nextPopupIndex2 >= 3)
    {
        this->nextPopupIndex2 = 0;
    }
    popup = &this->popups[this->nextPopupIndex2 + 0x2d0];
    popup->inUse = 1;
    characterCount = 0;
    if (value >= 0)
    {
        while (value != 0)
        {
            popup->digits[characterCount++] = (char)(value % 10);
            value /= 10;
        }
    }
    else
    {
        popup->digits[characterCount++] = '\n';
    }
    if (characterCount == 0)
    {
        popup->digits[characterCount++] = 0;
    }
    popup->characterCount = (u8)characterCount;
    popup->color = color;
    popup->timer.InitializeForPopup();
    popup->position = *position;
    popup->position.x += g_GameManager.arcadeRegionTopLeftPos.x;
    popup->position.y += g_GameManager.arcadeRegionTopLeftPos.y;
    this->nextPopupIndex2++;
}

// FUNCTION: TH07 0x00402780
i32 RetryMenu::OnUpdate()
{
    u32 i;

    if (WAS_PRESSED_RAW(TH_BUTTON_MENU) && this->curState != 4)
    {
        g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
        this->curState = 4;
        for (i = 0; i < 10; i++)
        {
            if (this->menuSprites[i].visible != 0)
            {
                this->menuSprites[i].pendingInterrupt = 2;
            }
        }
        this->numFrames = 0;
        this->menuBackground.pendingInterrupt = 1;
    }
    if (WAS_PRESSED_RAW(TH_BUTTON_Q) &&
        (this->curState != 9))
    {
        g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
        this->curState = 9;
        for (i = 0; i < 10; i++)
        {
            if (this->menuSprites[i].visible != 0)
            {
                this->menuSprites[i].pendingInterrupt = 2;
            }
        }
        this->numFrames = 0;
    }
    if ((g_GameManager.replay == 0) &&
        WAS_PRESSED_RAW(TH_BUTTON_RESET) &&
        this->curState != 9)
    {
        g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
        this->curState = 10;
        for (i = 0; i < 10; i++)
        {
            if (this->menuSprites[i].visible != 0)
            {
                this->menuSprites[i].pendingInterrupt = 2;
            }
        }
        this->numFrames = 0;
    }
    switch (this->curState)
    {
    case 0:
        for (i = 0; i < 10; i++)
        {
            g_AnmManager->SetAnmIdxAndExecuteScript(&this->menuSprites[i], i + 0xfe);
        }
        for (i = 0; (i32)i < 4; i++)
        {
            this->menuSprites[i].pendingInterrupt = 1;
        }
        g_AnmManager->SetActiveSprite(this->menuSprites + 7,
                                      g_GameManager.difficulty + 0x10d);
        if (g_GameManager.practice == 0)
        {
            this->menuSprites[8].SetInvisible();
        }
        if ((g_GameManager.defaultCfg)->slowMode == 0)
        {
            this->menuSprites[9].SetInvisible();
        }
        if (g_GameManager.replay != 0)
        {
            this->menuSprites[3].currentInstruction = NULL;
        }
        this->curState++;
        this->numFrames = 0;
        if ((g_Supervisor.flags >> 1 & 1) != 0)
        {
            g_AnmManager->SetAnmIdxAndExecuteScript(&this->menuBackground, 0x724);
            if (g_AnmManager->CreateScreenshotTexture(this->menuBackground.sprite->startPixelInclusive.x,
                                                      this->menuBackground.sprite->startPixelInclusive.y,
                                                      this->menuBackground.sprite->heightPx, this->menuBackground.sprite->widthPx) != 0)
            {
                this->curState = 0;
                return 0;
            }
            this->menuBackground.pos.x = 32.0f;
            this->menuBackground.pos.y = 16.0f;
            this->menuBackground.pos.z = 0.0f;
        }
    case 1:
        this->menuSprites[1].color.color = 0xffffffff;
        this->menuSprites[3].color.color = 0x80303030;
        this->menuSprites[2].color.color = 0x80303030;
        this->menuSprites[1].offset = D3DXVECTOR3(-4.0f, -4.0f, 0.0f);
        this->menuSprites[3].offset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        this->menuSprites[2].offset = this->menuSprites[3].offset;
        if (this->numFrames >= 4)
        {
            if (g_GameManager.replay == 0)
            {
                if (WAS_PRESSED_RAW(TH_BUTTON_UP))
                {
                    this->curState = 3;
                    g_SoundPlayer.PlaySoundByIdx(SOUND_0, 0);
                }
            }
            else if (WAS_PRESSED_RAW(TH_BUTTON_UP))
            {
                this->curState = 2;
                g_SoundPlayer.PlaySoundByIdx(SOUND_0, 0);
            }
            if (WAS_PRESSED_RAW(TH_BUTTON_DOWN))
            {
                this->curState = 2;
                g_SoundPlayer.PlaySoundByIdx(SOUND_0, 0);
            }
            if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
            {
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
                for (i = 0; (i32)i < 4; i++)
                {
                    this->menuSprites[i].pendingInterrupt = 2;
                }
                this->curState = 4;
                this->numFrames = 0;
                this->menuBackground.pendingInterrupt = 1;
            }
        }
        break;
    case 2:
        this->menuSprites[3].color.color = 0x80303030;
        this->menuSprites[1].color.color = 0x80303030;
        this->menuSprites[2].color.color = 0xffffffff;
        this->menuSprites[3].offset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        this->menuSprites[1].offset = this->menuSprites[3].offset;
        this->menuSprites[2].offset = D3DXVECTOR3(-4.0f, -4.0f, 0.0f);
        if (this->numFrames >= 4)
        {
            if (WAS_PRESSED_RAW(TH_BUTTON_UP))
            {
                this->curState = 1;
                g_SoundPlayer.PlaySoundByIdx(SOUND_0, 0);
            }
            if (g_GameManager.replay != 0)
            {
                if (WAS_PRESSED_RAW(TH_BUTTON_DOWN))
                {
                    this->curState = 1;
                    g_SoundPlayer.PlaySoundByIdx(SOUND_0, 0);
                }
            }
            else
            {
                if (WAS_PRESSED_RAW(TH_BUTTON_DOWN))
                {
                    this->curState = 3;
                    g_SoundPlayer.PlaySoundByIdx(SOUND_0, 0);
                }
            }
            if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
            {
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
                for (i = 0; (i32)i < 4; i++)
                {
                    this->menuSprites[i].pendingInterrupt = 2;
                }
                for (; (i32)i < 7; i++)
                {
                    this->menuSprites[i].pendingInterrupt = 1;
                }
                this->curState = 6;
                this->numFrames = 0;
            }
        }
        break;
    case 3:
        this->menuSprites[2].color.color = 0x80303030;
        this->menuSprites[1].color.color = 0x80303030;
        this->menuSprites[3].color.color = 0xffffffff;
        this->menuSprites[2].offset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        this->menuSprites[1].offset = this->menuSprites[2].offset;
        this->menuSprites[3].offset = D3DXVECTOR3(-4.0f, -4.0f, 0.0f);
        if (this->numFrames >= 4)
        {
            if (WAS_PRESSED_RAW(TH_BUTTON_UP))
            {
                this->curState = 2;
                g_SoundPlayer.PlaySoundByIdx(SOUND_0, 0);
            }
            if (WAS_PRESSED_RAW(TH_BUTTON_DOWN))
            {
                this->curState = 1;
                g_SoundPlayer.PlaySoundByIdx(SOUND_0, 0);
            }
            if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
            {
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
                for (i = 0; (i32)i < 4; i++)
                {
                    this->menuSprites[i].pendingInterrupt = 2;
                }
                for (; (i32)i < 7; i++)
                {
                    this->menuSprites[i].pendingInterrupt = 1;
                }
                this->curState = 8;
                this->numFrames = 0;
            }
        }
        break;
    case 4:
        if (this->numFrames >= 20)
        {
            this->curState = 0;
            g_GameManager.isInRetryMenu = 0;
            for (i = 0; i < 10; i++)
            {
                this->menuSprites[i].SetInvisible();
            }
            if ((g_GameManager.currentStage != 6) || (g_Gui.frameCounter >= 300))
            {
                // STRING: TH07 0x00498a38
                g_SoundPlayer.PushCommand(AUDIO_UNPAUSE, 0, (char *)"UnPause");
            }
            g_Supervisor.currentTime = timeGetTime();
        }
        break;
    case 5:
    case 7:
        this->menuSprites[5].color.color = 0xffff8080;
        this->menuSprites[6].color.color = 0x80808080;
        this->menuSprites[5].offset = D3DXVECTOR3(-4.0f, -4.0f, 0.0f);
        this->menuSprites[6].offset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        if (this->numFrames >= 4)
        {
            if (WAS_PRESSED_RAW(TH_BUTTON_UP) || WAS_PRESSED_RAW(TH_BUTTON_DOWN))
            {
                if (this->curState == 5)
                {
                    this->curState = 6;
                }
                else
                {
                    this->curState = 8;
                }
                g_SoundPlayer.PlaySoundByIdx(SOUND_0, 0);
            }
            if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
            {
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
                for (i = 4; (i32)i < 7; i++)
                {
                    this->menuSprites[i].pendingInterrupt = 2;
                }
                if (this->curState == 5)
                {
                    this->curState = 9;
                }
                else
                {
                    this->curState = 10;
                }
                this->numFrames = 0;
            }
        }
        break;
    case 6:
    case 8:
        this->menuSprites[5].color.color = 0x80808080;
        this->menuSprites[6].color.color = 0xffff8080;
        this->menuSprites[5].offset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        this->menuSprites[6].offset = D3DXVECTOR3(-4.0f, -4.0f, 0.0f);
        if (this->numFrames >= 4)
        {
            if (WAS_PRESSED_RAW(TH_BUTTON_UP) || WAS_PRESSED_RAW(TH_BUTTON_DOWN))
            {
                if (this->curState == 6)
                {
                    this->curState = 5;
                }
                else
                {
                    this->curState = 7;
                }
                g_SoundPlayer.PlaySoundByIdx(SOUND_0, 0);
            }
            if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
            {
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
                for (i = 0; (i32)i < 4; i++)
                {
                    this->menuSprites[i].pendingInterrupt = 1;
                }
                for (; (i32)i < 7; i++)
                {
                    this->menuSprites[i].pendingInterrupt = 2;
                }
                if (this->curState == 6)
                {
                    this->curState = 2;
                }
                else
                {
                    this->curState = 3;
                }
                this->numFrames = 0;
            }
        }
        break;
    case 9:
        if (this->numFrames >= 20)
        {
            this->curState = 0;
            g_GameManager.isInRetryMenu = 0;
            g_Supervisor.curState = 1;
            for (i = 0; i < 10; i++)
            {
                this->menuSprites[i].SetInvisible();
            }
            g_Supervisor.currentTime = timeGetTime();
        }
        break;
    case 10:
        if (this->numFrames >= 20)
        {
            this->curState = 0;
            g_GameManager.isInRetryMenu = 0;
            g_Supervisor.curState = 10;
            for (i = 0; i < 10; i++)
            {
                this->menuSprites[i].SetInvisible();
            }
            g_Supervisor.currentTime = timeGetTime();
        }
    }
    for (i = 0; i < 10; i++)
    {
        g_AnmManager->ExecuteScript(&this->menuSprites[i]);
    }
    if ((g_Supervisor.flags >> 1 & 1) != 0)
    {
        g_AnmManager->ExecuteScript(&this->menuBackground);
    }
    this->numFrames++;
    return 0;
}

// FUNCTION: TH07 0x00403a20
void RetryMenu::OnDraw()
{
    u32 i;

    if (g_GameManager.isInRetryMenu != 0)
    {
        g_AnmManager->Flush();
        g_Supervisor.viewport.X = (u32)(g_GameManager.arcadeRegionTopLeftPos.x);
        g_Supervisor.viewport.Y = (u32)(g_GameManager.arcadeRegionTopLeftPos.y);
        g_Supervisor.viewport.Width = (u32)(g_GameManager.arcadeRegionSize.x);
        g_Supervisor.viewport.Height = (u32)(g_GameManager.arcadeRegionSize.y);
        g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
        if (((g_Supervisor.flags >> 1 & 1) != 0) && (this->curState != 0))
        {
            AnmVm local_25c = this->menuBackground;
            local_25c.zWriteDisable = 1;
            g_AnmManager->DrawNoRotation(&local_25c);
        }
        for (i = 0; i < 10; i++)
        {
            if (this->menuSprites[i].visible != 0)
            {
                g_AnmManager->DrawNoRotation(this->menuSprites + i);
            }
        }
    }
}

// FUNCTION: TH07 0x00403b60
i32 PauseMenu::OnUpdate()
{
    i32 i;

    if (g_GameManager.practice != 0)
    {
        g_GameManager.isInPauseMenu = 0;
        g_GameManager.globals->guiScore = g_GameManager.globals->score;
        g_Supervisor.curState = 6;
        return 1;
    }
    if (g_GameManager.replay != 0)
    {
        g_GameManager.isInPauseMenu = 0;
        g_Supervisor.curState = 7;
        g_GameManager.globals->guiScore = g_GameManager.globals->score;
        return 1;
    }
    if (((i32)(u32)g_GameManager.globals->numRetries >= g_GameManager.maxRetries) ||
        (g_GameManager.difficulty >= 4))
    {
        g_GameManager.isInPauseMenu = 0;
        g_Supervisor.curState = 6;
        g_GameManager.globals->guiScore = g_GameManager.globals->score;
        return 1;
    }
    switch (this->curState)
    {
    case 0:
        if (this->numFrames == 0)
        {
            g_SoundPlayer.PushCommand(AUDIO_PAUSE, 0, "Pause");
            for (i = 0; i < 4; i++)
            {
                g_AnmManager->SetAnmIdxAndExecuteScript(&this->menuSprites[i], i + 0x108);
                this->menuSprites[i].pendingInterrupt = 1;
            }
            g_AnmManager->SetAnmIdxAndExecuteScript(&this->menuSprites[4], 0x10c);
            g_AnmManager->SetActiveSprite(&this->menuSprites[4],
                                          (g_GameManager.maxRetries + 0x106) -
                                              (u32)g_GameManager.globals->numRetries);
            this->menuSprites[4].pendingInterrupt = 1;
            if ((g_Supervisor.flags >> 1 & 1) != 0)
            {
                g_AnmManager->SetAnmIdxAndExecuteScript(&this->menuBackground, 0x724);
                if (g_AnmManager->CreateScreenshotTexture(this->menuBackground.sprite->startPixelInclusive.x,
                                                          this->menuBackground.sprite->startPixelInclusive.y,
                                                          this->menuBackground.sprite->heightPx, this->menuBackground.sprite->widthPx) != 0)
                {
                    this->curState = 0;
                    return 0;
                }
                this->menuBackground.pos.x = 32.0f;
                this->menuBackground.pos.y = 16.0f;
                this->menuBackground.pos.z = 0.0f;
            }
            g_Supervisor.UpdateTime();
        }
        if (8 < this->numFrames)
        {
            break;
        }
        this->curState = this->curState + 2;
        this->numFrames = 0;
    case 1:
        this->menuSprites[2].color.color = 0xffff8080;
        this->menuSprites[3].color.color = 0x80808080;
        this->menuSprites[2].offset = D3DXVECTOR3(-4.0f, -4.0f, 0.0f);
        this->menuSprites[3].offset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        if (this->numFrames >= 4)
        {
            if (WAS_PRESSED_RAW(TH_BUTTON_UP) || WAS_PRESSED_RAW(TH_BUTTON_DOWN))
            {
                this->curState = 2;
                g_SoundPlayer.PlaySoundByIdx(SOUND_0, 0);
            }
            if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
            {
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
                for (i = 0; i < 5; i++)
                {
                    this->menuSprites[i].pendingInterrupt = 2;
                }
                this->curState = 3;
                this->menuBackground.pendingInterrupt = 1;
                this->numFrames = 0;
            }
        }
        break;
    case 2:
        this->menuSprites[3].color.color = 0xffff8080;
        this->menuSprites[2].color.color = 0x80808080;
        this->menuSprites[3].offset = D3DXVECTOR3(-4.0f, -4.0f, 0.0f);
        this->menuSprites[2].offset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        if (this->numFrames >= 0x1e)
        {
            if (WAS_PRESSED_RAW(TH_BUTTON_UP) || WAS_PRESSED_RAW(TH_BUTTON_DOWN))
            {
                this->curState = 1;
                g_SoundPlayer.PlaySoundByIdx(SOUND_0, 0);
            }
            if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
            {
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
                for (i = 0; i < 5; i++)
                {
                    this->menuSprites[i].pendingInterrupt = 2;
                }
                this->curState = 4;
                this->numFrames = 0;
            }
        }
        break;
    case 4:
        if (this->numFrames >= 20)
        {
            this->curState = 0;
            this->numFrames = 0;
            g_GameManager.isInPauseMenu = 0;
            g_Supervisor.curState = 6;
            for (i = 0; i < 5; i++)
            {
                this->menuSprites[i].SetInvisible();
            }
            g_GameManager.globals->guiScore = g_GameManager.globals->score;
            g_Supervisor.currentTime = timeGetTime();
            return 0;
        }
        break;
    case 3:
        if (this->numFrames >= 0x1e)
        {
            this->curState = 0;
            this->numFrames = 0;
            g_GameManager.isInPauseMenu = 0;
            for (i = 0; i < 5; i++)
            {
                this->menuSprites[i].SetInvisible();
            }
            g_GameManager.globals->numRetries++;
            g_GameManager.globals->guiScore = (u32)g_GameManager.globals->numRetries;
            g_GameManager.globals->guiScoreDifference = 0;
            g_GameManager.globals->score = g_GameManager.globals->guiScore;
            g_GameManager.globals->livesRemaining =
                (f32)g_GameManager.defaultCfg->lifeCount;
            g_GameManager.RegenerateGameIntegrityCsum();
            g_GameManager.SetBombsRemainingAndComputeCsum(
                g_Player.shooterData->initialBombs);
            g_GameManager.globals->grazeInStage = 0;
            g_GameManager.globals->pointItemsCollectedThisStage = 0;
            g_GameManager.globals->pointItemsCollectedForExtend = 0;
            g_GameManager.globals->currentPower = 0.0f;
            g_GameManager.RegenerateGameIntegrityCsum();
            g_GameManager.globals->extendsFromPointItems = 0;
            g_GameManager.globals->nextNeededPointItemsForExtend = 0x32;
            g_GameManager.cherry = g_GameManager.globals->cherryStart;
            g_Gui.flags = (g_Gui.flags & 0xfffffffc) | 0x2;
            g_Gui.flags = (g_Gui.flags & 0xfffffff3) | 0x8;
            g_Gui.flags = (g_Gui.flags & 0xffffff3f) | 0x80;
            g_Gui.flags = (g_Gui.flags & 0xfffffcff) | 0x200;
            g_Gui.flags = (g_Gui.flags & 0xffffffcf) | 0x20;
            IncrementCapped(
                &g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty]
                     .playCount,
                999999);
            IncrementCapped(&g_GameManager.plst.playDataTotals.playCount, 999999);
            IncrementCapped(
                &g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty]
                     .playCountPerShotType[g_GameManager.shotTypeAndCharacter],
                999999);
            IncrementCapped(
                &g_GameManager.plst.playDataTotals
                     .playCountPerShotType[g_GameManager.shotTypeAndCharacter],
                999999);
            IncrementCapped(
                &g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty]
                     .retryCount,
                999999);
            IncrementCapped(&g_GameManager.plst.playDataTotals.retryCount, 999999);
            g_SoundPlayer.PushCommand(AUDIO_UNPAUSE, 0, "UnPause");
            g_Supervisor.currentTime = timeGetTime();
            return 0;
        }
        break;
    }
    for (i = 0; i < 5; i++)
    {
        g_AnmManager->ExecuteScript(&this->menuSprites[i]);
    }
    if ((g_Supervisor.flags >> 1 & 1) != 0)
    {
        g_AnmManager->ExecuteScript(&this->menuBackground);
    }
    this->numFrames++;
    UselessStack::FourBytes();
    return 0;
}

// FUNCTION: TH07 0x00404560
void PauseMenu::OnDraw()
{
    i32 i;

    if (g_GameManager.isInPauseMenu != 0)
    {
        g_AnmManager->Flush();
        g_Supervisor.viewport.X = g_GameManager.arcadeRegionTopLeftPos.x;
        g_Supervisor.viewport.Y = g_GameManager.arcadeRegionTopLeftPos.y;
        g_Supervisor.viewport.Width = g_GameManager.arcadeRegionSize.x;
        g_Supervisor.viewport.Height = g_GameManager.arcadeRegionSize.y;
        g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
        if (((g_Supervisor.flags >> 1 & 1) != 0) &&
            (this->curState != 0 || (2 < this->numFrames)))
        {
            g_AnmManager->DrawNoRotation(&this->menuBackground);
        }
        if ((this->curState == 1) || (this->curState == 2))
        {
            g_AnmManager->DrawNoRotation(&this->menuSprites[4]);
        }
        for (i = 0; i < 5; i++)
        {
            if (this->menuSprites[i].visible != 0)
            {
                g_AnmManager->DrawNoRotation(&this->menuSprites[i]);
            }
        }
    }
}

#pragma var_order(popup, alpha, dy, dx, j, i, digits, unused, cherry, \
                  xInc, hasNonZeroDigit, divisor)
// FUNCTION: TH07 0x00404690
void AsciiManager::DrawPopups()
{
    i32 divisor;
    BOOL hasNonZeroDigit;
    i32 xInc;
    i32 cherry;
    i32 unused[3];
    u8 *digits;
    i32 i;
    i32 j;
    f32 dx;
    f32 dy;
    i32 alpha;
    AsciiManagerPopup *popup;

    popup = this->popups;
    if ((g_Supervisor.cfg.opts >> 10 & 1) == 0)
    {
        g_Supervisor.DisableFog();
    }
    g_Supervisor.SetRenderState(D3DRS_ZFUNC, 8);

    for (i = 0; i < 0x2d3; i++, popup++)
    {
        if (!popup->inUse)
        {
            continue;
        }

        this->vm1.pos.x = popup->position.x - (f32)(popup->characterCount << 2);
        this->vm1.pos.y = popup->position.y;
        this->vm1.color.color = popup->color;

        dx = g_Player.positionCenter.x - popup->position.x;
        dy = g_Player.positionCenter.y - popup->position.y;
        alpha = (i32)(dx * dx + dy * dy);

        if (alpha > 0x1000)
        {
            alpha = 0xd0;
        }
        else
        {
            if (alpha > 0x400)
            {
                alpha = (alpha - 0x400) * 128 / 0xc00 + 0x50;
            }
            else
            {
                alpha = 0x50;
            }
        }

        digits = &popup->digits[popup->characterCount - 1];

        for (j = popup->characterCount; j > 0; j--)
        {
            if (popup->timer < 0x34 || *digits == 10)
            {
                this->vm1.sprite = &g_AnmManager->sprites[*digits];
                this->vm1.color.bytes.a = alpha;
            }
            else if (popup->timer < 0x38)
            {
                this->vm1.sprite = &g_AnmManager->sprites[*digits + 11];
                this->vm1.color.bytes.a = alpha;
            }
            else
            {
                this->vm1.sprite = &g_AnmManager->sprites[*digits + 21];
                this->vm1.color.bytes.a = alpha;
            }

            g_AnmManager->DrawNoRotation(&this->vm1);
            this->vm1.pos.x += 8.0f;
            digits--;
        }
    }

    if (this->cherryGauge.visible)
    {
        divisor = 100000;
        hasNonZeroDigit = FALSE;
        cherry = g_GameManager.cherry - g_GameManager.globals->cherryStart;

        g_AnmManager->DrawNoRotation(&this->cherryGauge);

        this->cherryDigit.pos.x = this->cherryGauge.pos.x + 40.0f + 6.0f;
        this->cherryDigit.pos.y = this->cherryGauge.pos.y + 11.0f;
        this->cherryDigit.pos.z = this->cherryGauge.pos.z;
        this->cherryDigit.color.bytes.a = this->cherryGauge.color.bytes.a;

        if (g_GameManager.IsCherryAtMax())
        {
            this->cherryDigit.color.bytes.r = 0xff;
            this->cherryDigit.color.bytes.g = 0xd0;
            this->cherryDigit.color.bytes.b = 0x80;
        }
        else
        {
            if (cherry >= 50000)
            {
                this->cherryDigit.color.bytes.r = 0xff;
                this->cherryDigit.color.bytes.g = 0xff;
                this->cherryDigit.color.bytes.b = 0x80;
            }
            else
            {
                this->cherryDigit.color.bytes.r = 0xff;
                this->cherryDigit.color.bytes.g = 0xff;
                this->cherryDigit.color.bytes.b = 0xff;
            }
        }

        for (i = 0; i < 6; i++, divisor /= 10)
        {
            j = cherry / divisor;
            cherry %= divisor;
            if (j != 0)
            {
                hasNonZeroDigit = TRUE;
            }
            if (hasNonZeroDigit || divisor == 1)
            {
                g_AnmManager->SetActiveSprite(&this->cherryDigit, j + 0x84);
                g_AnmManager->DrawNoRotation(&this->cherryDigit);
            }
            this->cherryDigit.pos.x += 7.0f;
        }

        hasNonZeroDigit = FALSE;
        cherry = g_GameManager.cherryMax - g_GameManager.globals->cherryStart;

        this->cherryDigit.color.bytes.r = 0xf0;
        this->cherryDigit.color.bytes.g = 0xd0;
        this->cherryDigit.color.bytes.b = 0xe0;
        this->cherryDigit.pos.x += 9.0f;

        if (cherry < 1000000)
        {
            divisor = 100000;
            i = 6;
        }
        else
        {
            divisor = 1000000;
            i = 7;
        }

        for (; i > 0; i--, divisor /= 10)
        {
            j = cherry / divisor;
            cherry %= divisor;
            if (j != 0)
            {
                hasNonZeroDigit = TRUE;
            }
            if (hasNonZeroDigit || divisor == 1)
            {
                g_AnmManager->SetActiveSprite(&this->cherryDigit, j + 0x84);
                g_AnmManager->DrawNoRotation(&this->cherryDigit);
            }
            this->cherryDigit.pos.x += 7.0f;
        }

        this->cherryDigit.scale.x = 1.0f;
        this->cherryDigit.scale.y = 1.0f;
        hasNonZeroDigit = FALSE;
        this->cherryDigit.pos.x = this->cherryGauge.pos.x + 40.0f + 6.0f + 7.0f;
        this->cherryDigit.pos.y = this->cherryGauge.pos.y + 2.0f;

        cherry = g_GameManager.cherryPlus - g_GameManager.globals->cherryStart;

        if (g_Player.hasBorder)
        {
            this->cherryDigit.color.bytes.r = 0xff;
            divisor = cherry % 4000;
            if (divisor >= 2000)
            {
                divisor = 4000 - divisor;
            }
            this->cherryDigit.color.bytes.g =
                (cherry * 0xc0) / 50000 + (divisor * 64) / 2000;
            this->cherryDigit.color.bytes.b =
                (cherry * 0xc0) / 50000 + (divisor * 64) / 2000;
            this->cherryDigit.scale.x = 1.41f;
            this->cherryDigit.scale.y = 1.41f;
            xInc = 10;
            this->cherryDigit.pos.x += 2.0f;
            this->cherryDigit.pos.y -= 2.0f;
        }
        else
        {
            this->cherryDigit.color.bytes.r = 0xc0;
            this->cherryDigit.color.bytes.g = 0x80;
            this->cherryDigit.color.bytes.b = 0xb0;
            xInc = 7;
        }

        for (divisor = 10000, i = 0; i < 5; i++, divisor /= 10)
        {
            j = cherry / divisor;
            cherry %= divisor;
            if (j != 0)
            {
                hasNonZeroDigit = TRUE;
            }
            if (hasNonZeroDigit || divisor == 1)
            {
                g_AnmManager->SetActiveSprite(&this->cherryDigit, j + 0x84);
                g_AnmManager->DrawNoRotation(&this->cherryDigit);
            }
            this->cherryDigit.pos.x += (f32)xInc;
        }

        this->cherryDigit.scale.x = 1.0f;
        this->cherryDigit.scale.y = 1.0f;

        if (g_Player.hasBorder == BORDER_ACTIVE)
        {
            this->cherryBorderActive.pos = this->cherryGauge.pos;
            this->cherryBorderActive.pos.x += 24.0f;
            this->cherryBorderActive.pos.y += 8.0f;
            this->cherryBorderActive.color = this->cherryGauge.color;
            g_AnmManager->DrawNoRotation(&this->cherryBorderActive);
        }
    }
}
