#include "Ending.hpp"

#include "AnmManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameManager.hpp"
#include "ScreenEffect.hpp"

// GLOBAL: TH07 0x0049f628
const char *g_BadEndingPaths[3] = {
    "data/end00b.end",
    "data/end10b.end",
    "data/end20b.end",
};

// GLOBAL: TH07 0x0049f634
const char *g_NormalEndingPaths[6] = {
    "data/end00.end",
    "data/end01.end",
    "data/end10.end",
    "data/end11.end",
    "data/end20.end",
    "data/end21.end",
};

// FUNCTION: TH07 0x0041d180
Ending::Ending()
{
    memset(this, 0, sizeof(Ending));
    this->line2Delay = 8;
    this->timer2.Initialize(0);
    this->timer1.Initialize(0);
    this->backgroundPos.x = 0.0f;
    this->backgroundPos.y = 0.0f;
    this->backgroundScrollSpeed = 0.0f;
}

// FUNCTION: TH07 0x0041d2f0
u32 Ending::OnUpdate(Ending *arg)
{
    i32 i;
    i32 local_8;

    local_8 = 0;
    while (true)
    {
        if (arg->ParseEndFile() != ZUN_SUCCESS)
        {
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
        for (i = 0; i < 15; i += 1)
        {
            g_AnmManager->ExecuteScript(&arg->sprites[i]);
        }
        if (((arg->hasSeenEnding == 0) ||
             IS_PRESSED_RAW(TH_BUTTON_SKIP)) ||
            (3 < local_8))
            break;
        local_8 += 1;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0041d380
u32 Ending::OnDraw(Ending *arg)
{
    i32 width;
    i32 height;

    height = 0x1e0;
    width = 0x280;
    g_AnmManager->DrawEndingRect(0, 0, 0, arg->backgroundPos.x,
                                 arg->backgroundPos.y, width, height);
    for (i32 i = 0; i < 0xf; i += 1)
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
        this->endFileDataPtr = this->endFileDataPtr + 1;
    }
    while (*this->endFileDataPtr == '\0')
    {
        this->endFileDataPtr = this->endFileDataPtr + 1;
    }
    return cur;
}

// FUNCTION: TH07 0x0041d490
void Ending::FadingEffect()
{
    ZunRect rect;

    rect.left = 0.0f;
    rect.top = 0.0f;
    rect.right = 640.0f;
    rect.bottom = 480.0f;
    switch (this->fadeType)
    {
    case 0:
        (this->endingFadeRectColor).color = 0;
        break;
    case 1:
        if (this->timeFading < this->fadeFrames)
        {
            (this->endingFadeRectColor).color =
                (0xff - (this->timeFading * 0xff) / this->fadeFrames) * 0x1000000;
            this->timeFading = this->timeFading + 1;
        }
        else
        {
            this->fadeType = 0;
            (this->endingFadeRectColor).color = 0;
        }
        break;
    case 2:
        if (this->timeFading < this->fadeFrames)
        {
            (this->endingFadeRectColor).color =
                (this->timeFading * 0xff) / this->fadeFrames << 0x18;
            this->timeFading = this->timeFading + 1;
        }
        else
        {
            (this->endingFadeRectColor).color = 0xff000000;
        }
        break;
    case 3:
        if (this->timeFading < this->fadeFrames)
        {
            (this->endingFadeRectColor).color =
                (0xff - (this->timeFading * 0xff) / this->fadeFrames) * 0x1000000 |
                0xffffff;
            this->timeFading = this->timeFading + 1;
        }
        else
        {
            this->fadeType = 0;
            (this->endingFadeRectColor).color = 0;
        }
        break;
    case 4:
        if (this->timeFading < this->fadeFrames)
        {
            (this->endingFadeRectColor).color =
                (this->timeFading * 0xff) / this->fadeFrames << 0x18 | 0xffffff;
            this->timeFading = this->timeFading + 1;
        }
        else
        {
            (this->endingFadeRectColor).color = 0xffffffff;
        }
    }
    if (((this->endingFadeRectColor).color & 0xff000000) != 0)
    {
        ScreenEffect::DrawSquare(&rect, (this->endingFadeRectColor).color);
    }
}

// FUNCTION: TH07 0x0041d700
ZunResult Ending::ParseEndFile()
{
    i32 tmp3;
    i32 tmp2;
    D3DCOLOR DVar2;
    i32 tmp;
    i32 j;
    i32 execInner;
    i32 execOuter;
    i32 i;
    i32 local_58;
    char local_54[68];

    local_58 = 0;
    memset(local_54, 0, sizeof(local_54));
    if (0 < this->timer3.current)
    {
        this->timer3.Decrement(1);
        if (this->minWaitResetFrames == 0)
        {
            if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU) ||
                ((this->hasSeenEnding != 0 &&
                  IS_PRESSED_RAW(TH_BUTTON_SKIP))))
            {
                this->timer3.Initialize(0);
            }
        }
        else
        {
            this->minWaitResetFrames = this->minWaitResetFrames - 1;
        }
        if (0 < this->timer3.current)
            goto LAB_0041e331;
        for (i = 0; i < 0xf; i += 1)
        {
            this->sprites[i].pendingInterrupt = 2;
        }
        this->possiblyTimesFileParsed = 0;
    }
    if (this->timer2.current < 1)
    {
        while (true)
        {
            switch (*this->endFileDataPtr)
            {
            case '\0':
            case '\n':
            case '\r':
                goto switchD_0041d980_caseD_0;
            default:
                local_54[local_58] = *this->endFileDataPtr;
                local_54[local_58 + 1] = this->endFileDataPtr[1];
                local_58 += 2;
                this->endFileDataPtr = this->endFileDataPtr + 2;
                break;
            case '@':
                this->endFileDataPtr = this->endFileDataPtr + 1;
                switch (*this->endFileDataPtr)
                {
                case '0':
                    this->endFileDataPtr = this->endFileDataPtr + 1;
                    this->fadeType = 1;
                    this->timeFading = 0;
                    tmp = ReadEndFileParameter();
                    this->fadeFrames = tmp;
                    break;
                case '1':
                    this->endFileDataPtr = this->endFileDataPtr + 1;
                    this->fadeType = 2;
                    this->timeFading = 0;
                    tmp = ReadEndFileParameter();
                    this->fadeFrames = tmp;
                    break;
                case '2':
                    this->endFileDataPtr = this->endFileDataPtr + 1;
                    this->fadeType = 3;
                    this->timeFading = 0;
                    tmp = ReadEndFileParameter();
                    this->fadeFrames = tmp;
                    break;
                case '3':
                    this->endFileDataPtr = this->endFileDataPtr + 1;
                    this->fadeType = 4;
                    this->timeFading = 0;
                    tmp = ReadEndFileParameter();
                    this->fadeFrames = tmp;
                    break;
                case 'F':
                    if (LoadEnding(this->endFileDataPtr + 1) != ZUN_SUCCESS)
                    {
                        return ZUN_ERROR;
                    }
                    local_58 = 0;
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
                case 'M':
                    this->endFileDataPtr = this->endFileDataPtr + 1;
                    /* musicFadeFrames */
                    tmp = ReadEndFileParameter();
                    g_Supervisor.FadeOutMusic((f32)tmp);
                    break;
                case 'V':
                    this->endFileDataPtr = this->endFileDataPtr + 1;
                    /* scrollBGDistance */
                    tmp = ReadEndFileParameter();
                    /* scrollBGDuration */
                    tmp2 = ReadEndFileParameter();
                    this->backgroundScrollSpeed = (f32)tmp / (f32)tmp2;
                    break;
                case 'a':
                    this->endFileDataPtr = this->endFileDataPtr + 1;
                    tmp = ReadEndFileParameter();
                    /* anmScriptIdx */
                    tmp2 = ReadEndFileParameter();
                    /* anmSpriteIdx */
                    tmp3 = ReadEndFileParameter();
                    g_AnmManager->ExecuteAnmIdx(this->sprites + tmp, tmp2 + 0x600);
                    g_AnmManager->SetActiveSprite(this->sprites + tmp, tmp3 + 0x600);
                    break;
                case 'b':
                    if (g_AnmManager->LoadSurface(0, this->endFileDataPtr + 1) !=
                        ZUN_SUCCESS)
                    {
                        return ZUN_ERROR;
                    }
                    break;
                case 'c':
                    this->endFileDataPtr = this->endFileDataPtr + 1;
                    DVar2 = ReadEndFileParameter();
                    (this->textColor).color = DVar2;
                    break;
                case 'm':
                    g_Supervisor.LoadAudio(0, this->endFileDataPtr + 1);
                    g_Supervisor.PlayLoadedAudio(0);
                    break;
                case 'r':
                    this->endFileDataPtr = this->endFileDataPtr + 1;
                    tmp = ReadEndFileParameter();
                    this->timer3.Initialize(tmp);
                    tmp = ReadEndFileParameter();
                    this->minWaitResetFrames = tmp;
                    while ((*this->endFileDataPtr != '\n' &&
                            (*this->endFileDataPtr != '\r')))
                    {
                        this->endFileDataPtr = this->endFileDataPtr + 1;
                    }
                    while ((*this->endFileDataPtr == '\n' ||
                            (*this->endFileDataPtr == '\r')))
                    {
                        this->endFileDataPtr = this->endFileDataPtr + 1;
                    }
                    goto LAB_0041e331;
                case 's':
                    this->endFileDataPtr = this->endFileDataPtr + 1;
                    tmp = ReadEndFileParameter();
                    this->line2Delay = tmp;
                    tmp = ReadEndFileParameter();
                    this->topLineDelay = tmp;
                    break;
                case 'v':
                    this->endFileDataPtr = this->endFileDataPtr + 1;
                    tmp = ReadEndFileParameter();
                    this->backgroundPos.y = (f32)tmp;
                    break;
                case 'w':
                    this->endFileDataPtr = this->endFileDataPtr + 1;
                    tmp = ReadEndFileParameter();
                    this->timer2.Initialize(tmp);
                    tmp = ReadEndFileParameter();
                    this->minWaitFrames = tmp;
                    while ((*this->endFileDataPtr != '\n' &&
                            (*this->endFileDataPtr != '\r')))
                    {
                        this->endFileDataPtr = this->endFileDataPtr + 1;
                    }
                    while ((*this->endFileDataPtr == '\n' ||
                            (*this->endFileDataPtr == '\r')))
                    {
                        this->endFileDataPtr = this->endFileDataPtr + 1;
                    }
                    goto LAB_0041e331;
                case 'z':
                    return ZUN_ERROR;
                }
                while ((*this->endFileDataPtr != '\n' &&
                        (*this->endFileDataPtr != '\r')))
                {
                    this->endFileDataPtr = this->endFileDataPtr + 1;
                }
                while ((*this->endFileDataPtr == '\n' ||
                        (*this->endFileDataPtr == '\r')))
                {
                    this->endFileDataPtr = this->endFileDataPtr + 1;
                }
            }
        }
    }
    this->timer2.Decrement(1);
    if (this->minWaitFrames == 0)
    {
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU) ||
            ((this->hasSeenEnding != 0 &&
              IS_PRESSED_RAW(TH_BUTTON_SKIP))))
        {
            this->timer2.Initialize(0);
        }
    }
    else
    {
        this->minWaitFrames = this->minWaitFrames - 1;
    }
    goto LAB_0041e331;
switchD_0041d980_caseD_0:
    if (local_58 != 0)
    {
        AnmManager::DrawVmTextFmt(g_AnmManager,
                                  this->sprites + this->possiblyTimesFileParsed,
                                  (this->textColor).color, 0xffffffff, local_54);
        this->sprites[this->possiblyTimesFileParsed].pendingInterrupt = 1;
    }
    while (((*this->endFileDataPtr == '\n' || (*this->endFileDataPtr == '\0')) ||
            (*this->endFileDataPtr == '\r')))
    {
        this->endFileDataPtr = this->endFileDataPtr + 1;
    }
    if (IS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
    {
        this->timer2.Initialize(this->topLineDelay);
        this->minWaitFrames = this->topLineDelay;
    }
    else
    {
        this->timer2.Initialize(this->line2Delay);
        this->minWaitFrames = this->line2Delay;
    }
    this->possiblyTimesFileParsed = this->possiblyTimesFileParsed + 1;
LAB_0041e331:
    this->timer1.previous = this->timer1.current;
    g_Supervisor.TickTimer(&this->timer1.current, &this->timer1.subFrame);
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
        this->timer2.Initialize(0);
        this->timer1.Initialize(0);
        if (endFileDat != NULL)
        {
            free(endFileDat);
        }
        return ZUN_SUCCESS;
    }
}

// FUNCTION: TH07 0x0041e590
ZunResult Ending::AddedCallback(Ending *arg)
{
    D3DXVECTOR3 *pDVar1;
    u32 shotType;
    i32 i;
    const char *local_8;

    g_GameManager.flags |= 0x10;
    g_Supervisor.isInEnding = 1;
    g_AnmManager->LoadAnms(0x31, "data/staff01.anm", 0x600);
    g_AnmManager->currentTexture = NULL;
    g_AnmManager->currentSprite = NULL;
    g_AnmManager->currentBlendMode = 0xff;
    g_AnmManager->currentVertexShader = 0xff;
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
    for (i = 0; i < 0xf; i += 1)
    {
        g_AnmManager->ExecuteAnmIdx(arg->sprites + i, i + 0x70f);
        pDVar1 = &arg->sprites[i].pos;
        pDVar1->x = 64.0f;
        pDVar1->y = (f32)i * 16.0f + 392.0f;
        pDVar1->z = 0.0f;
    }
    if (g_GameManager.globals->numRetries == 0)
    {
        local_8 = g_NormalEndingPaths[g_GameManager.shotTypeAndCharacter];
    }
    else
    {
        local_8 = g_BadEndingPaths[g_GameManager.character];
    }
    if (arg->LoadEnding(local_8) == ZUN_SUCCESS)
    {
        return ZUN_SUCCESS;
    }
    else
    {
        return ZUN_ERROR;
    }
}

// FUNCTION: TH07 0x0041e790
ZunResult Ending::DeletedCallback(Ending *arg)
{
    g_AnmManager->ReleaseAnm(0x31);
    g_Supervisor.curState = 6;
    g_AnmManager->ReleaseSurface(0);
    free(arg->endFileData);
    g_Chain.Cut(arg->drawChain);
    arg->drawChain = NULL;
    free(arg);
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
        return ZUN_ERROR;

    ending->drawChain = g_Chain.CreateElem((ChainCallback)OnDraw);
    ending->drawChain->arg = ending;
    g_Chain.AddToDrawChain(ending->drawChain, 1);
    return ZUN_SUCCESS;
}
