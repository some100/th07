#include "Ending.hpp"

#include "AnmManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameManager.hpp"
#include "ScreenEffect.hpp"
#include "Supervisor.hpp"
#include "d3dx8.h"

// GLOBAL: TH07 0x0049f628
const char *g_BadEndingPaths[3] = {
    // STRING: TH07 0x004985b4
    "data/end00b.end",
    // STRING: TH07 0x004985a4
    "data/end10b.end",
    // STRING: TH07 0x00498594
    "data/end20b.end",
};

// GLOBAL: TH07 0x0049f634
const char *g_NormalEndingPaths[6] = {
    // STRING: TH07 0x00498584
    "data/end00.end",
    // STRING: TH07 0x00498574
    "data/end01.end",
    // STRING: TH07 0x00498564
    "data/end10.end",
    // STRING: TH07 0x00498554
    "data/end11.end",
    // STRING: TH07 0x00498544
    "data/end20.end",
    // STRING: TH07 0x00498534
    "data/end21.end",
};

// FUNCTION: TH07 0x0041d2f0
u32 Ending::OnUpdate(Ending *arg)
{
    i32 i;
    i32 framesSkipPressed;

    framesSkipPressed = 0;
    for (;;)
    {
        if (arg->ParseEndFile() != ZUN_SUCCESS)
        {
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
        for (i = 0; i < 15; i++)
        {
            g_AnmManager->ExecuteScript(&arg->sprites[i]);
        }

        if (arg->hasSeenEnding != 0 && IS_PRESSED_RAW(TH_BUTTON_SKIP) &&
            framesSkipPressed < 4)
        {
            framesSkipPressed++;
            continue;
        }

        break;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0041d380
u32 Ending::OnDraw(Ending *arg)
{
    g_AnmManager->DrawEndingRect(0, 0, 0, arg->backgroundPos.x,
                                 arg->backgroundPos.y, 0x280, 0x1e0);
    for (i32 i = 0; i < 0xf; i++)
    {
        g_AnmManager->Draw(&arg->sprites[i]);
    }
    arg->FadingEffect();
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0041d410
i32 Ending::ReadEndFileParameter()
{
    long cur = atol(this->endFileDataPtr);
    while (*this->endFileDataPtr != '\0')
    {
        this->endFileDataPtr++;
    }
    while (*this->endFileDataPtr == '\0')
    {
        this->endFileDataPtr++;
    }
    return cur;
}

#pragma var_order(rect, color)
// FUNCTION: TH07 0x0041d490
void Ending::FadingEffect()
{
    ZunRect rect;
    D3DCOLOR color;

    rect.left = 0.0f;
    rect.top = 0.0f;
    rect.right = 640.0f;
    rect.bottom = 480.0f;
    switch (this->fadeType)
    {
    case 1:
        if (this->timeFading >= this->fadeFrames)
        {
            this->fadeType = 0;
            this->endingFadeRectColor.color = 0;
            break;
        }

        color = 0xff - (this->timeFading * 0xff) / this->fadeFrames;
        this->endingFadeRectColor.color = color * 0x1000000;
        this->timeFading++;
        break;
    case 2:
        if (this->timeFading >= this->fadeFrames)
        {
            this->endingFadeRectColor.color = 0xff000000;
            break;
        }

        color = (this->timeFading * 0xff) / this->fadeFrames;
        this->endingFadeRectColor.color =
            color << 0x18;
        this->timeFading++;
        break;
    case 3:
        if (this->timeFading >= this->fadeFrames)
        {
            this->fadeType = 0;
            this->endingFadeRectColor.color = 0;
            break;
        }

        color = 0xff - (this->timeFading * 0xff) / this->fadeFrames;
        this->endingFadeRectColor.color = color * 0x1000000 | 0xffffff;
        this->timeFading++;
        break;
    case 4:
        if (this->timeFading >= this->fadeFrames)
        {
            this->endingFadeRectColor.color = 0xffffffff;
            break;
        }

        color = (this->timeFading * 0xff) / this->fadeFrames;
        this->endingFadeRectColor.color = color << 0x18 | 0xffffff;
        this->timeFading++;
        break;
    case 0:
        this->endingFadeRectColor.color = 0;
        break;
    }
    if ((this->endingFadeRectColor.color & 0xff000000) != 0)
    {
        ScreenEffect::DrawSquare(&rect, this->endingFadeRectColor.color);
    }
}

#pragma var_order(lineDisplayed, local_54, local_58, i, anmScriptIdx, vmIdx,   \
                  anmSpriteIdx, scrollBGDistance, scrollBGDuration, execOuter, \
                  execInner, j, musicFadeFrames)
// FUNCTION: TH07 0x0041d700
ZunResult Ending::ParseEndFile()
{
    f32 musicFadeFrames;
    i32 j;
    i32 execInner;
    i32 execOuter;
    i32 scrollBGDuration;
    i32 scrollBGDistance;
    i32 anmSpriteIdx;
    i32 vmIdx;
    i32 anmScriptIdx;
    i32 i;
    i32 local_58;
    char local_54[68];
    i32 lineDisplayed;

    lineDisplayed = 0;
    local_58 = 0;
    memset(local_54, 0, sizeof(local_54));
    if (this->timer3 > 0)
    {
        this->timer3--;
        if (this->minWaitResetFrames != 0)
        {
            this->minWaitResetFrames--;
        }
        else
        {
            if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU) ||
                (this->hasSeenEnding != 0 &&
                 IS_PRESSED_RAW(TH_BUTTON_SKIP)))
            {
                this->timer3 = 0;
            }
        }
        if (this->timer3 <= 0)
        {
            for (i = 0; i < 0xf; i++)
            {
                this->sprites[i].pendingInterrupt = 2;
            }
            this->timesFileParsed = 0;
        }
        else
        {
            goto stop;
        }
    }
    if (this->timer2 > 0)
    {
        this->timer2--;
        if (this->minWaitFrames != 0)
        {
            this->minWaitFrames--;
        }
        else
        {
            if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU) ||
                (this->hasSeenEnding != 0 &&
                 IS_PRESSED_RAW(TH_BUTTON_SKIP)))
            {
                this->timer2 = 0;
            }
        }
        goto stop;
    }

    while (true)
    {
        switch (*this->endFileDataPtr)
        {
        case '@':
            this->endFileDataPtr++;
            switch (*this->endFileDataPtr)
            {
            case 'b':
                if (g_AnmManager->LoadSurface(0, this->endFileDataPtr + 1) !=
                    ZUN_SUCCESS)
                {
                    return ZUN_ERROR;
                }
                break;
            case 'a':
                this->endFileDataPtr++;
                vmIdx = ReadEndFileParameter();
                anmScriptIdx = ReadEndFileParameter();
                anmSpriteIdx = ReadEndFileParameter();
                g_AnmManager->ExecuteAnmIdx(&this->sprites[vmIdx], anmScriptIdx + 0x600);
                g_AnmManager->SetActiveSprite(&this->sprites[vmIdx], anmSpriteIdx + 0x600);
                break;
            case 'V':
                this->endFileDataPtr++;
                scrollBGDistance = ReadEndFileParameter();
                scrollBGDuration = ReadEndFileParameter();
                this->backgroundScrollSpeed = (f32)scrollBGDistance / scrollBGDuration;
                break;
            case 'v':
                this->endFileDataPtr++;
                this->backgroundPos.y = (f32)ReadEndFileParameter();
                break;
            case 'F':
                if (LoadEnding(this->endFileDataPtr + 1) != ZUN_SUCCESS)
                {
                    return ZUN_ERROR;
                }
                local_58 = 0;
                lineDisplayed = 0;
                for (execOuter = 0; execOuter < 6; execOuter += 1)
                {
                    for (execInner = 0; execInner < 4; execInner += 1)
                    {
                        if (g_GameManager.clrd[execOuter]
                                    .difficultyClearedWithRetries[execInner] == 99 ||
                            g_GameManager.clrd[execOuter]
                                    .difficultyClearedWithoutRetries[execInner] == 99)
                        {
                            this->hasSeenEnding = 1;
                            break;
                        }
                    }
                }
            case 'R':
                for (j = 0; j < 16; j += 1)
                {
                    this->sprites[j].anmFileIdx = 0;
                }
                break;
            case 'm':
                g_Supervisor.LoadAudio(0, this->endFileDataPtr + 1);
                g_Supervisor.PlayLoadedAudio(0);
                break;
            case 'M':
                this->endFileDataPtr++;
                musicFadeFrames = ReadEndFileParameter();
                g_Supervisor.FadeOutMusic(musicFadeFrames);
                break;
            case 's':
                this->endFileDataPtr++;
                this->line2Delay = ReadEndFileParameter();
                this->topLineDelay = ReadEndFileParameter();
                break;
            case 'c':
                this->endFileDataPtr++;
                this->textColor.color = ReadEndFileParameter();
                break;
            case 'r':
                this->endFileDataPtr++;
                this->timer3 = ReadEndFileParameter();
                this->minWaitResetFrames = ReadEndFileParameter();
                while (*this->endFileDataPtr != '\n' &&
                       (*this->endFileDataPtr != '\r'))
                {
                    this->endFileDataPtr++;
                }
                while (*this->endFileDataPtr == '\n' ||
                       (*this->endFileDataPtr == '\r'))
                {
                    this->endFileDataPtr++;
                }
                goto stop;
            case 'w':
                this->endFileDataPtr++;
                this->timer2 = ReadEndFileParameter();
                this->minWaitFrames = ReadEndFileParameter();
                while (*this->endFileDataPtr != '\n' &&
                       (*this->endFileDataPtr != '\r'))
                {
                    this->endFileDataPtr++;
                }
                while (*this->endFileDataPtr == '\n' ||
                       (*this->endFileDataPtr == '\r'))
                {
                    this->endFileDataPtr++;
                }
                goto stop;
            case '0':
                this->endFileDataPtr++;
                this->fadeType = 1;
                this->timeFading = 0;
                this->fadeFrames = ReadEndFileParameter();
                break;
            case '1':
                this->endFileDataPtr++;
                this->fadeType = 2;
                this->timeFading = 0;
                this->fadeFrames = ReadEndFileParameter();
                break;
            case '2':
                this->endFileDataPtr++;
                this->fadeType = 3;
                this->timeFading = 0;
                this->fadeFrames = ReadEndFileParameter();
                break;
            case '3':
                this->endFileDataPtr++;
                this->fadeType = 4;
                this->timeFading = 0;
                this->fadeFrames = ReadEndFileParameter();
                break;
            case 'z':
                return ZUN_ERROR;
            }
            while (*this->endFileDataPtr != '\n' &&
                   (*this->endFileDataPtr != '\r'))
            {
                this->endFileDataPtr++;
            }
            while (*this->endFileDataPtr == '\n' ||
                   (*this->endFileDataPtr == '\r'))
            {
                this->endFileDataPtr++;
            }
            break;
        case '\0':
        case '\n':
        case '\r':
            if (local_58 != 0)
            {
                AnmManager::DrawVmTextFmt(g_AnmManager,
                                          &this->sprites[this->timesFileParsed],
                                          this->textColor.color, 0xffffffff, local_54);
                this->sprites[this->timesFileParsed].SetPendingInterrupt(1);
            }
            while ((*this->endFileDataPtr == '\n' || (*this->endFileDataPtr == '\0')) ||
                   (*this->endFileDataPtr == '\r'))
            {
                this->endFileDataPtr++;
            }
            if (IS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
            {
                this->timer2 = this->topLineDelay;
                this->minWaitFrames = this->topLineDelay;
            }
            else
            {
                this->timer2 = this->line2Delay;
                this->minWaitFrames = this->line2Delay;
            }
            this->timesFileParsed++;
            goto stop;
        default:
            local_54[local_58] = *this->endFileDataPtr;
            local_54[local_58 + 1] = this->endFileDataPtr[1];
            local_58 += 2;
            this->endFileDataPtr = this->endFileDataPtr + 2;
            break;
        }
    }

stop:
    this->timer1++;
    this->backgroundPos.y -= this->backgroundScrollSpeed;
    if (this->backgroundPos.y <= 0.0f)
    {
        this->backgroundPos.y = 0.0f;
        this->backgroundScrollSpeed = 0.0f;
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0041e4b0
ZunResult Ending::LoadEnding(const char *endFilePath)
{
    char *endFileDat;

    endFileDat = this->endFileData;
    this->endFileData = (char *)FileSystem::OpenFile(endFilePath, 0);
    if (this->endFileData == NULL)
    {
        // STRING: TH07 0x004985d8
        g_GameErrorContext.Log("error : エンディングファイルが読み込めない、ファイルが破壊されています\r\n");
        return ZUN_ERROR;
    }
    else
    {
        this->endFileDataPtr = this->endFileData;
        this->line2Delay = 8;
        this->timer2 = 0;
        this->timer1 = 0;
        if (endFileDat != NULL)
        {
            free(endFileDat);
        }
        return ZUN_SUCCESS;
    }
}

#pragma var_order(endingPath, unusedShotType, shotType, i)
// FUNCTION: TH07 0x0041e590
ZunResult Ending::AddedCallback(Ending *arg)
{
    i32 i;
    u32 shotType;
    u32 unusedShotType;
    const char *endingPath;

    unusedShotType = g_GameManager.shotTypeAndCharacter;
    g_GameManager.finished = 1;
    g_Supervisor.isInEnding = 1;
    g_AnmManager->LoadAnms(0x31, "data/staff01.anm", 0x600);
    g_AnmManager->SetTexture(NULL);
    g_AnmManager->SetSprite(NULL);
    g_AnmManager->SetBlendMode(0xff);
    g_AnmManager->SetVertexShader(0xff);
    shotType = g_GameManager.shotTypeAndCharacter;
    arg->hasSeenEnding = 0;
    if (g_GameManager.globals->numRetries == 0)
    {
        if (g_GameManager.clrd[shotType]
                .difficultyClearedWithRetries[g_GameManager.difficulty] == 99)
        {
            arg->hasSeenEnding = 1;
        }
        g_GameManager.clrd[shotType]
            .difficultyClearedWithRetries[g_GameManager.difficulty] = 99;
    }
    else if (g_GameManager.clrd[shotType]
                 .difficultyClearedWithoutRetries[g_GameManager.difficulty] ==
             99)
    {
        arg->hasSeenEnding = 1;
    }
    g_GameManager.clrd[shotType]
        .difficultyClearedWithoutRetries[g_GameManager.difficulty] = 99;
    for (i = 0; i < 0xf; i++)
    {
        g_AnmManager->ExecuteAnmIdx(&arg->sprites[i], i + 0x70f);
        arg->sprites[i].pos =
            D3DXVECTOR3(64.0f, (f32)i * 16.0f + 392.0f, 0.0f);
    }
    if (g_GameManager.globals->numRetries != 0)
    {
        endingPath = g_BadEndingPaths[g_GameManager.character];
    }
    else
    {
        endingPath = g_NormalEndingPaths[g_GameManager.shotTypeAndCharacter];
    }

    if (arg->LoadEnding(endingPath))
    {
        return ZUN_ERROR;
    }

    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0041e790
ZunResult Ending::DeletedCallback(Ending *arg)
{
    g_AnmManager->ReleaseAnm(0x31);
    g_Supervisor.curState = 6;
    g_AnmManager->ReleaseSurface(0);
    ZunMemory::Free(arg->endFileData);
    g_Chain.Cut(arg->drawChain);
    arg->drawChain = NULL;
    delete arg;
    arg = NULL;
    g_Supervisor.isInEnding = 0;

    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0041e820
ZunResult Ending::RegisterChain()
{
    Ending *ending = new Ending;
    ending->calcChain = g_Chain.CreateElem((ChainCallback)OnUpdate);
    ending->calcChain->arg = ending;
    ending->calcChain->addedCallback = (ChainLifecycleCallback)AddedCallback;
    ending->calcChain->deletedCallback = (ChainLifecycleCallback)DeletedCallback;
    if (g_Chain.AddToCalcChain(ending->calcChain, 4) != 0)
    {
        return ZUN_ERROR;
    }

    ending->drawChain = g_Chain.CreateElem((ChainCallback)OnDraw);
    ending->drawChain->arg = ending;
    g_Chain.AddToDrawChain(ending->drawChain, 1);
    return ZUN_SUCCESS;
}
