#include "MusicRoom.hpp"

#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "SoundPlayer.hpp"
#include "Supervisor.hpp"

#pragma optimize("s", on)

// FUNCTION: TH07 0x0043a760
ZunResult MusicRoom::CheckInputEnable()
{
    i32 i;

    if (this->waitFramesCounter == 0)
    {
        for (i = 0; i < 0x1f; i++)
        {
            if (this->cursor == i)
            {
                this->titleSprites[i].pendingInterrupt = 1;
            }
            else
            {
                this->titleSprites[i].pendingInterrupt = 2;
            }
        }
        for (i = 0; i < 8; i++)
        {
            this->descriptionSprites[i].pendingInterrupt = 1;
        }
    }
    if (this->waitFramesCounter >= 8)
    {
        this->enableInput = 1;
    }
    return ZUN_SUCCESS;
}

#pragma var_order(unusedListingOffset, i, local_54)
// FUNCTION: TH07 0x0043a801
i32 MusicRoom::ProcessInput()
{
    char local_54[66];
    i32 i;
    i32 unusedListingOffset;

    unusedListingOffset = this->listingOffset;
    if (WAS_PRESSED_RAW(TH_BUTTON_UP))
    {
        this->cursor = this->cursor - 1;
        if (this->cursor < 0)
        {
            this->cursor = this->numDescriptors - 1;
            this->listingOffset = this->numDescriptors - 10;
            if (this->listingOffset < 0)
            {
                this->listingOffset = 0;
            }
        }
        else if (this->listingOffset > this->cursor)
        {
            this->listingOffset = this->cursor;
        }
        for (i = 0; i < 0x1f; i++)
        {
            if (this->cursor == i)
            {
                this->titleSprites[i].pendingInterrupt = 1;
            }
            else
            {
                this->titleSprites[i].pendingInterrupt = 2;
            }
        }
    }
    if (WAS_PRESSED_RAW(TH_BUTTON_DOWN))
    {
        this->cursor = this->cursor + 1;
        if (this->cursor >= this->numDescriptors)
        {
            this->cursor = 0;
            this->listingOffset = 0;
        }
        else
        {
            if (this->listingOffset <= this->cursor - 10)
            {
                this->listingOffset = this->cursor - 9;
            }
        }
        for (i = 0; i < 0x1f; i++)
        {
            if (this->cursor == i)
            {
                this->titleSprites[i].pendingInterrupt = 1;
            }
            else
            {
                this->titleSprites[i].pendingInterrupt = 2;
            }
        }
    }
    if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
    {
        this->selectedIdx = this->cursor;
        if ((g_Supervisor.cfg.opts >> 0xd & 1) != 0)
        {
            g_SoundPlayer.StartBGM("thbgm.dat");
        }
        g_Supervisor.PlayAudio(this->trackDescriptors[this->selectedIdx].path);
        for (i = 0; i < 8; i++)
        {
            memset(local_54, 0, sizeof(local_54));
            memcpy(local_54,
                   this->trackDescriptors[this->selectedIdx].description[i],
                   0x40);
            if (local_54[0] != '\0')
            {
                this->descriptionSprites[i].active = 1;
                AnmManager::DrawVmTextFmt(g_AnmManager,
                                          this->descriptionSprites + i, 0xffe0c0,
                                          0x300000, local_54);
            }
            else
            {
                this->descriptionSprites[i].active = 0;
            }
            this->descriptionSprites[i].pendingInterrupt = 1;
        }
    }
    if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
    {
        g_Supervisor.curState = 1;
        return 1;
    }

    return 0;
}

// FUNCTION: TH07 0x0043ab60
u32 MusicRoom::OnUpdate(MusicRoom *arg)
{
    i32 iVar1;
    i32 i;

    iVar1 = arg->enableInput;
recheck:
    switch (arg->enableInput)
    {
    case 0:
        if (!arg->CheckInputEnable())
        {
            break;
        }
        goto recheck;
    case 1:
        if (arg->ProcessInput() != 0)
        {
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
    default:
        break;
    }
    if (iVar1 != arg->enableInput)
    {
        arg->waitFramesCounter = 0;
    }
    else
    {
        arg->waitFramesCounter++;
    }
    g_AnmManager->ExecuteScript(&arg->vm);
    for (i = 0; i < 0x1f; i++)
    {
        g_AnmManager->ExecuteScript(&arg->titleSprites[i]);
    }
    for (i = 0; i < 8; i++)
    {
        g_AnmManager->ExecuteScript(&arg->descriptionSprites[i]);
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(i, local_c, local_18)
// FUNCTION: TH07 0x0043ac4c
u32 MusicRoom::OnDraw(MusicRoom *arg)
{
    D3DXVECTOR3 local_18;
    char local_c[4];
    i32 i;

    local_c[0] = 127;
    local_c[1] = 0;
    g_AnmManager->SetTexture(NULL);
    g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
    g_AnmManager->DrawNoRotation(&arg->vm);
    for (i = arg->listingOffset; i < arg->listingOffset + 10; i++)
    {
        if (i >= arg->numDescriptors)
        {
            break;
        }
        g_AsciiManager.SetColor(arg->titleSprites[i].color.color);
        arg->titleSprites[i].pos.x = 93.0f;
        arg->titleSprites[i].pos.y =
            ((f32)(((i + 1) - arg->listingOffset) * 0x12) + 104.0f) - 20.0f;
        arg->titleSprites[i].pos.z = 0.0f;
        g_AnmManager->DrawNoRotation(arg->titleSprites + i);
        local_18 = arg->titleSprites[i].pos;
        local_18.x -= 60.0f;
        if (arg->cursor == i)
        {
            g_AsciiManager.AddString(&local_18, local_c);
        }
        local_18.x += 15.0f;
        // STRING: TH07 0x00496c04
        AsciiManager::AddFormatText(&g_AsciiManager, &local_18, "%2d.", i + 1);
    }
    i++;
    for (i = 0; i < 8; i++)
    {
        g_AnmManager->DrawNoRotation(&arg->descriptionSprites[i]);
    }
    g_AsciiManager.color = 0xffffffff;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(offset, lineIdx, curChar, charIdx, firstChar, lineCharBuffer)
// FUNCTION: TH07 0x0043ae20
ZunResult MusicRoom::AddedCallback(MusicRoom *arg)
{
    char lineCharBuffer[66];
    char *firstChar;
    i32 charIdx;
    char *curChar;
    i32 lineIdx;
    i32 offset;

    // STRING: TH07 0x00496bec
    if (g_AnmManager->LoadSurface(0, "data/result/music.jpg") != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }
    // STRING: TH07 0x00496bd8
    if (g_AnmManager->LoadAnms(0x2e, "data/music00.anm", 0x900) !=
        ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    g_AnmManager->SetAnmIdxAndExecuteScript(&arg->vm, 0x900);
    arg->waitFramesCounter = 0;
    // STRING: TH07 0x00496bc4
    curChar = (char *)FileSystem::OpenFile("data/musiccmt.txt", 0);
    firstChar = curChar;
    if ((u8 *)curChar == NULL)
    {
        return ZUN_ERROR;
    }

    arg->trackDescriptors = new TrackDescriptor[0x20];
    offset = -1;
    while ((u32)((i32)curChar - (i32)firstChar) < g_LastFileSize)
    {
        if (*curChar == '@')
        {
            curChar++;
            offset++;
            charIdx = 0;
            while (*curChar != '\n' && (*curChar != '\r'))
            {
                arg->trackDescriptors[offset].path[charIdx] = *curChar;
                curChar++;
                charIdx++;
                if ((u32)((i32)curChar - (i32)firstChar) >= g_LastFileSize)
                {
                    goto LAB_0043b195;
                }
            }
            while (*curChar == '\n' || (*curChar == '\r'))
            {
                curChar++;
                if ((u32)((i32)curChar - (i32)firstChar) >= g_LastFileSize)
                {
                    goto LAB_0043b195;
                }
            }
            charIdx = 0;
            while (*curChar != '\n' && (*curChar != '\r'))
            {
                arg->trackDescriptors[offset].title[charIdx] = *curChar;
                curChar++;
                charIdx++;
                if ((u32)((i32)curChar - (i32)firstChar) >= g_LastFileSize)
                {
                    goto LAB_0043b195;
                }
            }
            while (*curChar == '\n' && (*curChar == '\r'))
            {
                curChar++;
                if ((u32)((i32)curChar - (i32)firstChar) >= g_LastFileSize)
                {
                    goto LAB_0043b195;
                }
            }
            for (lineIdx = 0; lineIdx < 8; lineIdx++)
            {
                if (*curChar == '@')
                {
                    break;
                }

                memset(arg->trackDescriptors[offset].description[lineIdx], 0,
                       sizeof(arg->trackDescriptors[offset].description[lineIdx]));
                charIdx = 0;
                while (*curChar != '\n' && (*curChar != '\r'))
                {
                    arg->trackDescriptors[offset].description[lineIdx][charIdx] =
                        *curChar;
                    curChar++;
                    charIdx++;
                    if ((u32)((i32)curChar - (i32)firstChar) >= g_LastFileSize)
                    {
                        goto LAB_0043b195;
                    }
                }
                while (*curChar == '\n' || (*curChar == '\r'))
                {
                    curChar++;
                    if ((u32)((i32)curChar - (i32)firstChar) >= g_LastFileSize)
                    {
                        goto LAB_0043b195;
                    }
                }
            }
        }
        else
        {
            curChar++;
        }
    }
LAB_0043b195:
    arg->numDescriptors = offset + 1;
    for (offset = 0; offset < arg->numDescriptors; offset++)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(&arg->titleSprites[offset], offset + 0x901);
        AnmManager::DrawVmTextFmt(g_AnmManager, arg->titleSprites + offset,
                                  0xc0e0ff, 0x302080,
                                  arg->trackDescriptors[offset].title);
        arg->titleSprites[offset].pos.x = 93.0f;
        arg->titleSprites[offset].pos.y =
            ((f32)((offset + 1) * 0x12) + 104.0f) - 20.0f;
        arg->titleSprites[offset].pos.z = 0.0f;
        arg->titleSprites[offset].anchor = 3;
    }
    for (offset = 0; offset < 8; offset++)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(
            &arg->descriptionSprites[offset],
            offset + 0x707);
        memset(lineCharBuffer, 0, sizeof(lineCharBuffer));
        memcpy(lineCharBuffer,
               arg->trackDescriptors[arg->selectedIdx].description[offset],
               0x40);
        if (*lineCharBuffer != '\0')
        {
            arg->descriptionSprites[offset].active = 1;
            AnmManager::DrawVmTextFmt(
                g_AnmManager, arg->descriptionSprites + offset, 0xffe0c0,
                0x300000, (char *)&lineCharBuffer);
        }
        else
        {
            arg->descriptionSprites[offset].active = 0;
        }
    }
    free(firstChar);
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0043b478
ZunResult MusicRoom::DeletedCallback(MusicRoom *arg)
{
    delete arg->trackDescriptors;
    arg->trackDescriptors = NULL;
    g_AnmManager->ReleaseSurface(0);
    g_AnmManager->ReleaseAnm(0x2e);
    g_AnmManager->ReleaseAnm(0x2f);
    g_Chain.Cut(arg->drawChain);
    arg->drawChain = NULL;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0043b4db
ZunResult MusicRoom::RegisterChain()
{
    static MusicRoom g_MusicRoom;
    MusicRoom *musicRoom = &g_MusicRoom;

    memset(musicRoom, 0,
           sizeof(MusicRoom)); // memset it twice because why not?
    musicRoom->calcChain = g_Chain.CreateElem((ChainCallback)OnUpdate);
    musicRoom->calcChain->arg = musicRoom;
    musicRoom->calcChain->addedCallback = (ChainLifecycleCallback)AddedCallback;
    musicRoom->calcChain->deletedCallback =
        (ChainLifecycleCallback)DeletedCallback;
    if (g_Chain.AddToCalcChain(musicRoom->calcChain, 3) != 0)
    {
        return ZUN_ERROR;
    }

    musicRoom->drawChain = g_Chain.CreateElem((ChainCallback)OnDraw);
    musicRoom->drawChain->arg = musicRoom;
    g_Chain.AddToDrawChain(musicRoom->drawChain, 0);
    return ZUN_SUCCESS;
}

#pragma optimize("s", off)
