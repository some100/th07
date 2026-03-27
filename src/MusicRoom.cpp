#include "MusicRoom.hpp"

#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "SoundPlayer.hpp"
#include "Supervisor.hpp"

MusicRoom g_MusicRoom;
i32 g_MusicRoomExists = 0;

MusicRoom::MusicRoom() { memset(this, 0, sizeof(MusicRoom)); }

ZunResult MusicRoom::CheckInputEnable()

{
  i32 i;

  if (this->waitFramesCounter == 0) {
    for (i = 0; i < 0x1f; i += 1) {
      if (this->cursor == i) {
        this->titleSprites[i].pendingInterrupt = 1;
      } else {
        this->titleSprites[i].pendingInterrupt = 2;
      }
    }
    for (i = 0; i < 8; i += 1) {
      this->descriptionSprites[i].pendingInterrupt = 1;
    }
  }
  if (7 < this->waitFramesCounter) {
    this->enableInput = 1;
  }
  return ZUN_SUCCESS;
}

i32 MusicRoom::ProcessInput()

{
  char local_54[72];
  i32 local_c;

  if (((g_CurFrameRawInput & TH_BUTTON_UP) != 0) &&
      ((g_CurFrameRawInput & TH_BUTTON_UP) !=
       (g_LastFrameInput & TH_BUTTON_UP))) {
    this->cursor = this->cursor - 1;
    if (this->cursor < 0) {
      this->cursor = this->numDescriptors - 1;
      this->listingOffset = this->numDescriptors - 10;
      if (this->listingOffset < 0) {
        this->listingOffset = 0;
      }
    } else if (this->cursor < this->listingOffset) {
      this->listingOffset = this->cursor;
    }
    for (local_c = 0; local_c < 0x1f; local_c += 1) {
      if (this->cursor == local_c) {
        this->titleSprites[local_c].pendingInterrupt = 1;
      } else {
        this->titleSprites[local_c].pendingInterrupt = 2;
      }
    }
  }
  if (((g_CurFrameRawInput & TH_BUTTON_DOWN) != 0) &&
      ((g_CurFrameRawInput & TH_BUTTON_DOWN) !=
       (g_LastFrameInput & TH_BUTTON_DOWN))) {
    this->cursor = this->cursor + 1;
    if (this->cursor < this->numDescriptors) {
      if (this->listingOffset <= this->cursor - 10) {
        this->listingOffset = this->cursor - 9;
      }
    } else {
      this->cursor = 0;
      this->listingOffset = 0;
    }
    for (local_c = 0; local_c < 0x1f; local_c += 1) {
      if (this->cursor == local_c) {
        this->titleSprites[local_c].pendingInterrupt = 1;
      } else {
        this->titleSprites[local_c].pendingInterrupt = 2;
      }
    }
  }
  if (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) != 0) &&
      ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) !=
       (g_LastFrameInput & TH_BUTTON_SELECTMENU))) {
    this->selectedIdx = this->cursor;
    if ((g_Supervisor.cfg.opts >> 0xd & 1) != 0) {
      g_SoundPlayer.StartBGM("thbgm.dat");
    }
    Supervisor::PlayAudio(this->trackDescriptors[this->selectedIdx].path);
    for (local_c = 0; local_c < 8; local_c += 1) {
      memset(local_54, 0, 0x40);
      memcpy(local_54,
             this->trackDescriptors[this->selectedIdx].description[local_c],
             0x40);
      if (local_54[0] == '\0') {
        this->descriptionSprites[local_c].flags =
            this->descriptionSprites[local_c].flags & 0xfffffffd;
      } else {
        this->descriptionSprites[local_c].flags =
            this->descriptionSprites[local_c].flags | 2;
        AnmManager::DrawVmTextFmt(g_AnmManager,
                                  this->descriptionSprites + local_c, 0xffe0c0,
                                  0x300000, local_54);
      }
      this->descriptionSprites[local_c].pendingInterrupt = 1;
    }
  }
  if (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) == 0) ||
      ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) ==
       (g_LastFrameInput & TH_BUTTON_RETURNMENU))) {
    return 0;
  } else {
    g_Supervisor.curState = 1;
    return 1;
  }
}

u32 MusicRoom::OnUpdate(MusicRoom *arg)

{
  i32 iVar1;
  i32 i;

  iVar1 = arg->enableInput;
  do {
    if (arg->enableInput != 0) {
      if ((arg->enableInput == 1) && (arg->ProcessInput() != 0)) {
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
      }
      break;
    }
  } while (arg->CheckInputEnable() != ZUN_SUCCESS);
  if (iVar1 == arg->enableInput) {
    arg->waitFramesCounter = arg->waitFramesCounter + 1;
  } else {
    arg->waitFramesCounter = 0;
  }
  g_AnmManager->ExecuteScript(&arg->vm);
  for (i = 0; i < 0x1f; i += 1) {
    g_AnmManager->ExecuteScript(arg->titleSprites + i);
  }
  for (i = 0; i < 8; i += 1) {
    g_AnmManager->ExecuteScript(arg->descriptionSprites + i);
  }
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

u32 MusicRoom::OnDraw(MusicRoom *arg)

{
  D3DXVECTOR3 *pDVar1;
  D3DXVECTOR3 local_18;
  char local_c[4];
  i32 i;

  local_c[0] = '\x7f';
  local_c[1] = 0;
  g_AnmManager->currentTexture = NULL;
  g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
  g_AnmManager->DrawNoRotation(&arg->vm);
  for (i = arg->listingOffset;
       (i < arg->listingOffset + 10 && (i < arg->numDescriptors)); i += 1) {
    g_AsciiManager.color = arg->titleSprites[i].color.color;
    arg->titleSprites[i].pos.x = 93.0f;
    arg->titleSprites[i].pos.y =
        ((f32)(((i + 1) - arg->listingOffset) * 0x12) + 104.0f) - 20.0f;
    arg->titleSprites[i].pos.z = 0.0f;
    g_AnmManager->DrawNoRotation(arg->titleSprites + i);
    pDVar1 = &arg->titleSprites[i].pos;
    local_18.y = pDVar1->y;
    local_18.z = pDVar1->z;
    local_18.x = pDVar1->x - 60.0f;
    if (arg->cursor == i) {
      g_AsciiManager.AddString(&local_18, local_c);
    }
    local_18.x = local_18.x + 15.0f;
    AsciiManager::AddFormatText(&g_AsciiManager, &local_18, "%2d.", i + 1);
  }
  for (i = 0; i < 8; i += 1) {
    g_AnmManager->DrawNoRotation(arg->descriptionSprites + i);
  }
  g_AsciiManager.color = 0xffffffff;
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult MusicRoom::AddedCallback(MusicRoom *arg)

{
  i16 local_90;
  i16 local_84;
  char lineCharBuffer[0x40];
  char *firstChar;
  i32 charIdx;
  char *currChar;
  i32 lineIdx;
  i32 offset;

  if (g_AnmManager->LoadSurface(0, "data/result/music.jpg") == ZUN_SUCCESS) {
    if (g_AnmManager->LoadAnms(0x2e, "data/music00.anm", 0x900) ==
        ZUN_SUCCESS) {
      (arg->vm).anmFileIdx = 0x900;
      g_AnmManager->SetAndExecuteScript(&arg->vm, g_AnmManager->scripts[0x900]);
      arg->waitFramesCounter = 0;
      firstChar = (char *)FileSystem::OpenFile("data/musiccmt.txt", 0);
      if ((u8 *)firstChar == NULL) {
        return ZUN_ERROR;
      } else {
        currChar = firstChar;
        arg->trackDescriptors = new TrackDescriptor[0x20];
        offset = -1;
        while ((u32)((i32)currChar - (i32)firstChar) < g_LastFileSize) {
          if (*currChar == '@') {
            currChar = currChar + 1;
            offset += 1;
            charIdx = 0;
            while ((*currChar != '\n' && (*currChar != '\r'))) {
              arg->trackDescriptors[offset].path[charIdx] = *currChar;
              currChar = currChar + 1;
              charIdx += 1;
              if (g_LastFileSize <= (u32)((i32)currChar - (i32)firstChar))
                goto LAB_0043b195;
            }
            while ((*currChar == '\n' || (*currChar == '\r'))) {
              currChar = currChar + 1;
              if (g_LastFileSize <= (u32)((i32)currChar - (i32)firstChar))
                goto LAB_0043b195;
            }
            charIdx = 0;
            while ((*currChar != '\n' && (*currChar != '\r'))) {
              arg->trackDescriptors[offset].title[charIdx] = *currChar;
              currChar = currChar + 1;
              charIdx += 1;
              if (g_LastFileSize <= (u32)((i32)currChar - (i32)firstChar))
                goto LAB_0043b195;
            }
            while ((*currChar == '\n' && (*currChar == '\r'))) {
              currChar = currChar + 1;
              if (g_LastFileSize <= (u32)((i32)currChar - (i32)firstChar))
                goto LAB_0043b195;
            }
            lineIdx = 0;
            while ((lineIdx < 8 && (*currChar != '@'))) {
              memset(arg->trackDescriptors[offset].description[lineIdx], 0,
                     0x40);
              charIdx = 0;
              while ((*currChar != '\n' && (*currChar != '\r'))) {
                arg->trackDescriptors[offset].description[lineIdx][charIdx] =
                    *currChar;
                currChar = currChar + 1;
                charIdx += 1;
                if (g_LastFileSize <= (u32)((i32)currChar - (i32)firstChar))
                  goto LAB_0043b195;
              }
              while ((*currChar == '\n' || (*currChar == '\r'))) {
                currChar = currChar + 1;
                if (g_LastFileSize <= (u32)((i32)currChar - (i32)firstChar))
                  goto LAB_0043b195;
              }
              lineIdx += 1;
            }
          } else {
            currChar = currChar + 1;
          }
        }
      LAB_0043b195:
        arg->numDescriptors = offset + 1;
        for (offset = 0; offset < arg->numDescriptors; offset += 1) {
          local_84 = (i16)offset + 0x901;
          arg->titleSprites[offset].anmFileIdx = local_84;
          g_AnmManager->SetAndExecuteScript(
              arg->titleSprites + offset,
              g_AnmManager->scripts[offset + 0x901]);
          AnmManager::DrawVmTextFmt(g_AnmManager, arg->titleSprites + offset,
                                    0xc0e0ff, 0x302080,
                                    arg->trackDescriptors[offset].title);
          arg->titleSprites[offset].pos.x = 93.0f;
          arg->titleSprites[offset].pos.y =
              ((f32)((offset + 1) * 0x12) + 104.0f) - 20.0f;
          arg->titleSprites[offset].pos.z = 0.0f;
          arg->titleSprites[offset].flags =
              arg->titleSprites[offset].flags | 0xc00;
        }
        for (offset = 0; offset < 8; offset += 1) {
          local_90 = (i16)offset + 0x707;
          arg->descriptionSprites[offset].anmFileIdx = local_90;
          g_AnmManager->SetAndExecuteScript(
              arg->descriptionSprites + offset,
              g_AnmManager->scripts[offset + 0x707]);
          memset(lineCharBuffer, 0, 0x40);
          memcpy(lineCharBuffer,
                 arg->trackDescriptors[arg->selectedIdx].description[offset],
                 0x40);
          if (*lineCharBuffer == '\0') {
            arg->descriptionSprites[offset].flags =
                arg->descriptionSprites[offset].flags & 0xfffffffd;
          } else {
            arg->descriptionSprites[offset].flags =
                arg->descriptionSprites[offset].flags | 2;
            AnmManager::DrawVmTextFmt(
                g_AnmManager, arg->descriptionSprites + offset, 0xffe0c0,
                0x300000, (char *)&lineCharBuffer);
          }
        }
        free(firstChar);
        return ZUN_SUCCESS;
      }
    } else {
      return ZUN_ERROR;
    }
  } else {
    return ZUN_ERROR;
  }
}

ZunResult MusicRoom::DeletedCallback(MusicRoom *arg)

{
  free(arg->trackDescriptors);
  arg->trackDescriptors = NULL;
  g_AnmManager->ReleaseSurface(0);
  g_AnmManager->ReleaseAnm(0x2e);
  g_AnmManager->ReleaseAnm(0x2f);
  g_Chain.Cut(arg->drawChain);
  arg->drawChain = NULL;
  return ZUN_SUCCESS;
}

ZunResult MusicRoom::RegisterChain() {
  if ((g_MusicRoomExists & 1) == 0) {
    g_MusicRoomExists |= 1;
    g_MusicRoom = MusicRoom();
  }
  memset(&g_MusicRoom, 0,
         sizeof(MusicRoom)); // memset it twice because why not?
  g_MusicRoom.calcChain = Chain::CreateElem((ChainCallback)OnUpdate);
  (g_MusicRoom.calcChain)->arg = &g_MusicRoom;
  (g_MusicRoom.calcChain)->addedCallback =
      (ChainLifecycleCallback)AddedCallback;
  (g_MusicRoom.calcChain)->deletedCallback =
      (ChainLifecycleCallback)DeletedCallback;
  if (g_Chain.AddToCalcChain(g_MusicRoom.calcChain, 3) == 0) {
    g_MusicRoom.drawChain = Chain::CreateElem((ChainCallback)OnDraw);
    (g_MusicRoom.drawChain)->arg = &g_MusicRoom;
    g_Chain.AddToDrawChain(g_MusicRoom.drawChain, 0);
    return ZUN_SUCCESS;
  } else {
    return ZUN_ERROR;
  }
}
