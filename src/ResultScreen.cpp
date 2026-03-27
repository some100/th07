#include "ResultScreen.hpp"

#include <direct.h>
#include <stdio.h>
#include <time.h>

#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameManager.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "ZunResult.hpp"
#include "pbg4/Lzss.hpp"
#include "dsutil.hpp"

const char *g_CharacterList[6] = {"博麗 霊夢 (霊)　", "博麗 霊夢 (夢)　",
                                  "霧雨 魔理沙 (魔)", "霧雨 魔理沙 (恋",
                                  "十六夜 咲夜 (幻)", "十六夜 咲夜 (時)"};
const char *g_AlphabetList =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ.,:;_@abcdefghijklmnopqrstuvwxyz+-/"
    "*=%0123456789#!?'\"$(){}[]<>&\\|~^ --";
const char *g_CharactersAndShotTypesStrings[6] = {
    "ReimuA ", "ReimuB ", "MarisaA", "MarisaB", "SakuyaA", "SakuyaB",
};
const char *g_DifficultyNameTable[6] = {
    "      Easy", "    Normal", "      Hard",
    "   Lunatic", "     Extra", "  Phantasm",
};

ResultScreen::ResultScreen() {
  memset(this, 0, sizeof(ResultScreen));
  this->cursor = 1;
}

i32 ResultScreen::LinkScore(ScoreListNode *prevNode, Hscr *hscr)

{
  ScoreListNode *pSVar1;
  ScoreListNode *local_14;
  i32 local_8;

  local_8 = 0;
  local_14 = prevNode;
  while ((local_14->next != NULL &&
          ((local_14->next->data == NULL ||
            (hscr->score < local_14->next->data->score))))) {
    local_14 = local_14->next;
    local_8 = local_8 + 1;
  }
  pSVar1 = local_14->next;
  local_14->next = (ScoreListNode *)malloc(sizeof(ScoreListNode));
  local_14->next->prev = local_14;
  local_14->next->data = hscr;
  local_14->next->next = pSVar1;
  return local_8;
}

void ResultScreen::FreeAllScores(ScoreListNode *scores)

{
  ScoreListNode *pSVar1;
  ScoreListNode *local_c;

  local_c = scores->next;
  while (local_c != NULL) {
    pSVar1 = local_c->next;
    free(local_c);
    local_c = pSVar1;
  }
}

ScoreDat *ResultScreen::OpenScore(const char *path)

{
  bool bVar2;
  ScoreDat *scoreData;
  Th7k *local_2c;
  i32 local_28;
  Th7k *local_24;
  i32 remainingData;
  ScoreDat *idx;
  u16 checksum;
  u8 xorValue;
  i32 local_10;
  ScoreDat *local_c;
  ScoreDat *pbVar2;

  Supervisor::DebugPrint2("info : score load\r\n");
  scoreData = (ScoreDat *)FileSystem::OpenFile(path, 1);
  local_c = scoreData;
  if (scoreData != NULL) {
    if (g_LastFileSize < sizeof(ScoreDat)) {
      Supervisor::DebugPrint2("warning : score.dat size is i16\r\n");
      free(scoreData);
    } else {
      checksum = 0;
      xorValue = 0;
      local_10 = 0;
      pbVar2 = scoreData;
      for (remainingData = g_LastFileSize - 2;
           idx = (ScoreDat *)(pbVar2->xorseed + 1), 0 < remainingData;
           remainingData = remainingData - 1) {
        xorValue += idx->xorseed[0];
        xorValue = (u8)((i32)(xorValue & 0xe0) >> 5) | xorValue * '\b';
        *(u8 *)&pbVar2->csum = (u8)pbVar2->csum ^ xorValue;
        if (1 < local_10) {
          checksum = checksum + (u8)pbVar2->csum;
        }
        local_10 = local_10 + 1;
        pbVar2 = idx;
      }
      if (scoreData->csum == checksum) {
        if (scoreData->dataOffset == sizeof(ScoreDat)) {
          if (scoreData->magic == 0xb) {
            local_c = (ScoreDat *)malloc(0xa001c);
            memcpy(local_c, scoreData, sizeof(ScoreDat));
            Lzss::Decompress(
                (u8 *)scoreData + sizeof(ScoreDat), scoreData->srcLen,
                (u8 *)local_c + sizeof(ScoreDat), scoreData->dstLen);
            free(scoreData);
            bVar2 = false;
            local_24 = (Th7k *)(local_c->xorseed + local_c->dataOffset);
            local_28 = local_c->fileLength - local_c->dataOffset;
            while (0 < local_28) {
              if (local_24->magic == 0x4b374854) {
                bVar2 = true;
                local_2c = local_24;
              }
              if ((local_24->magic == 0x4d535256) && (local_24->version == 1)) {
                if (g_Supervisor.CheckIntegrity(
                        ((Vrsm *)local_24)->versionStr,
                        ((Vrsm *)local_24)->exeSize,
                        ((Vrsm *)local_24)->exeChecksum) != ZUN_SUCCESS) {
                  Supervisor::DebugPrint2(
                      "warning : score.dat exesumcheck error\r\n");
                  goto LAB_00444c48;
                }
              }
              if (local_24->th7kLen == 0) {
                Supervisor::DebugPrint2(
                    "warning : score.dat chapter size is ZERO\r\n");
                goto LAB_00444c48;
              }
              u16 len = local_24->th7kLen;
              local_24 = (Th7k *)((u8 *)local_24 + local_24->th7kLen);
              local_28 -= len;
            }
            if ((bVar2) && (local_2c->version == 1))
              goto LAB_00444ed5;
            Supervisor::DebugPrint2("warning : score.dat version mismatch\r\n");
          } else {
            Supervisor::DebugPrint2("warning : score.dat version mismatch\r\n");
          }
        } else {
          Supervisor::DebugPrint2("warning : header size is mismatch\r\n");
        }
      } else {
        Supervisor::DebugPrint2("warning : score.dat chksum error\r\n");
      }
    }
  }
LAB_00444c48:
  Supervisor::DebugPrint2("info : score recreate\r\n");
  if (local_c != NULL) {
    free(local_c);
  }
  local_c = (ScoreDat *)malloc(sizeof(ScoreDat));
  local_c->dataOffset = sizeof(ScoreDat);
  local_c->fileLength = sizeof(ScoreDat);
LAB_00444ed5:
  local_c->scores = (ScoreListNode *)malloc(sizeof(ScoreListNode));
  local_c->scores->next = NULL;
  local_c->scores->data = NULL;
  local_c->scores->prev = NULL;
  return local_c;
}

u32 ResultScreen::GetHighScore(ScoreDat *scoreDat, ScoreListNode *node,
                               u32 character, u32 difficulty, u8 *noClue)

{
  u8 bVar1;
  Th7k *pTVar2;
  u32 local_24;
  u32 local_20;
  i32 local_c;
  Hscr *local_8;

  if (node == NULL) {
    FreeAllScores(scoreDat->scores);
    scoreDat->scores->next = NULL;
    scoreDat->scores->data = NULL;
    scoreDat->scores->prev = NULL;
  }
  local_8 = (Hscr *)(scoreDat->xorseed + scoreDat->dataOffset);
  for (local_c = scoreDat->fileLength - scoreDat->dataOffset; 0 < local_c;
       local_c = local_c - (u32)pTVar2->th7kLen) {
    if ((((local_8->magic == 0x52435348) && (local_8->version == 1)) &&
         (local_8->character == character)) &&
        (local_8->difficulty == difficulty)) {
      if (node == NULL) {
        LinkScore(scoreDat->scores, local_8);
      } else {
        LinkScore(node, local_8);
      }
    }
    pTVar2 = local_8;
    local_8 = (Hscr *)((u8 *)local_8 + local_8->th7kLen);
  }
  if (noClue != NULL) {
    if (scoreDat->scores->next == NULL) {
      bVar1 = 0;
    } else {
      bVar1 = scoreDat->scores->next->data->numRetries;
    }
    *noClue = bVar1;
  }
  if (scoreDat->scores->next == NULL) {
    local_24 = 100000;
  } else {
    if (scoreDat->scores->next->data->score < 0x186a1) {
      local_20 = 100000;
    } else {
      local_20 = scoreDat->scores->next->data->score;
    }
    local_24 = local_20;
  }
  return local_24;
}

ZunResult ResultScreen::ParseCatk(ScoreDat *scoreDat, Catk *catk)

{
  Th7k *pTVar1;
  i32 local_c;
  Catk *local_8;

  if (catk == NULL) {
    return ZUN_ERROR;
  } else {
    local_8 = (Catk *)(scoreDat->xorseed + scoreDat->dataOffset);
    for (local_c = scoreDat->fileLength - scoreDat->dataOffset; 0 < local_c;
         local_c = local_c - (u32)pTVar1->th7kLen) {
      if ((local_8->magic == 0x4b544143) && (local_8->version == 1)) {
        if (0x8c < (u16)local_8->idx)
          break;
        catk[local_8->idx] = *local_8;
      }
      pTVar1 = local_8;
      local_8 = (Catk *)((u8 *)local_8 + local_8->th7kLen);
    }
    return ZUN_SUCCESS;
  }
}

ZunResult ResultScreen::ParseLsnm(ScoreDat *scoreDat, Lsnm *param_2)

{
  i32 local_c;
  Lsnm *local_8;

  local_8 = (Lsnm *)(scoreDat->xorseed + scoreDat->dataOffset);
  local_c = scoreDat->fileLength - scoreDat->dataOffset;
  while (true) {
    if (local_c < 1) {
      return ZUN_ERROR;
    }
    if ((local_8->magic == 0x4d4e534c) && (local_8->version == 1))
      break;
    local_c = local_c - (u32)local_8->th7kLen;
    local_8 = (Lsnm *)((u8 *)local_8 + local_8->th7kLen);
  }
  param_2 = local_8;
  return ZUN_SUCCESS;
}

ZunResult ResultScreen::ParseClrd(ScoreDat *scoreDat, Clrd *clrd)

{
  u16 *puVar1;
  i32 j;
  i32 local_10;
  i32 i;
  Clrd *local_8;

  if (clrd == NULL) {
    return ZUN_ERROR;
  } else {
    for (i = 0; i < 6; i = i + 1) {
      memset(clrd + i, 0, sizeof(Clrd));
      clrd[i].magic = 0x44524c43;
      clrd[i].th7kLen2 = sizeof(Clrd);
      clrd[i].th7kLen = sizeof(Clrd);
      clrd[i].version = 1;
      clrd[i].characterShotType = (u8)i;
      for (j = 0; j < 5; j = j + 1) {
        clrd[i].difficultyClearedWithRetries[j] = 1;
        clrd[i].difficultyClearedWithoutRetries[j] = 1;
      }
    }
    local_8 = (Clrd *)(scoreDat->xorseed + scoreDat->dataOffset);
    for (local_10 = scoreDat->fileLength - scoreDat->dataOffset; 0 < local_10;
         local_10 = local_10 - (u32)*puVar1) {
      if ((local_8->magic == 0x44524c43) && (local_8->version == 1)) {
        if (5 < local_8->characterShotType)
          break;
        clrd[local_8->characterShotType] = *local_8;
      }
      puVar1 = &local_8->th7kLen;
      local_8 = (Clrd *)((u8 *)local_8 + local_8->th7kLen);
    }
    return ZUN_SUCCESS;
  }
}

ZunResult ResultScreen::ParsePscr(ScoreDat *scoreDat, Pscr *pscr)

{
  Th7k *pTVar1;
  i32 local_1c;
  i32 local_18;
  i32 local_14;
  i32 local_10;
  Pscr *local_c;
  Pscr *local_8;

  if (pscr == NULL) {
    return ZUN_ERROR;
  } else {
    local_8 = pscr;
    for (local_10 = 0; local_10 < 6; local_10 = local_10 + 1) {
      for (local_14 = 0; local_14 < 6; local_14 = local_14 + 1) {
        for (local_1c = 0; local_1c < 4; local_1c = local_1c + 1) {
          memset(local_8, 0, sizeof(Pscr));
          local_8->magic = 0x52435350;
          local_8->th7kLen2 = sizeof(Pscr);
          local_8->th7kLen = sizeof(Pscr);
          local_8->version = 1;
          local_8->character = (u8)local_10;
          local_8->difficulty = (u8)local_1c;
          local_8->stage = (u8)local_14;
          local_8->playCount = 0;
          local_8 = local_8 + 1;
        }
      }
    }
    local_c = (Pscr *)(scoreDat->xorseed + scoreDat->dataOffset);
    for (local_18 = scoreDat->fileLength - scoreDat->dataOffset; 0 < local_18;
         local_18 = local_18 - (u32)pTVar1->th7kLen) {
      if ((local_c->magic == 0x52435350) && (local_c->version == 1)) {
        if ((5 < local_c->character) ||
            ((4 < local_c->difficulty || (6 < local_c->stage))))
          break;
        pscr[(u32)local_c->stage * 4 + (u32)local_c->character * 0x18 +
             (u32)local_c->difficulty] = *local_c;
      }
      pTVar1 = local_c;
      local_c = (Pscr *)((u8 *)local_c + local_c->th7kLen);
    }
    return ZUN_SUCCESS;
  }
}

ZunResult ResultScreen::ParsePlst(ScoreDat *scoreDat, Plst *param_2)

{
  u16 *puVar1;
  i32 local_c;
  Plst *local_8;

  local_8 = (Plst *)(scoreDat->xorseed + scoreDat->dataOffset);
  for (local_c = scoreDat->fileLength - scoreDat->dataOffset; 0 < local_c;
       local_c = local_c - (u32)*puVar1) {
    if ((local_8->magic == 0x54534c50) && (local_8->version == 1)) {
      param_2 = local_8;
    }
    puVar1 = &local_8->th7kLen;
    local_8 = (Plst *)((u8 *)local_8 + local_8->th7kLen);
  }
  return ZUN_SUCCESS;
}

void ResultScreen::ReleaseScoreDat(ScoreDat *scoreDat)

{
  FreeAllScores(scoreDat->scores);
  free(scoreDat->scores);
  free(scoreDat);
}

void ResultScreen::WriteScore()

{
  u8 uVar1;
  u8 bVar3;
  u8 *fileBuffer;
  u8 *compressedBuffer;
  DWORD bytesToWrite;
  u8 local_78;
  u8 local_74;
  u8 *local_64;
  u8 local_5d;
  i32 local_5c;
  Vrsm vrsm;
  i32 k;
  i32 j;
  Pscr *pscr;
  Catk *catk;
  Clrd *clrd;
  i32 local_1c;
  ScoreListNode *local_18;
  size_t local_14;
  i32 local_c;
  i32 i;

  fileBuffer = (u8 *)malloc(0xa0000);
  memcpy(fileBuffer, this->scoreDat, sizeof(ScoreDat));
  this->th7kHeader.magic = 0x4b374854;
  this->th7kHeader.th7kLen2 = sizeof(Th7k);
  this->th7kHeader.th7kLen = sizeof(Th7k);
  this->th7kHeader.version = 1;
  memcpy(fileBuffer + sizeof(ScoreDat), &this->th7kHeader, sizeof(Th7k));
  local_14 = 0x28;
  i = 0;
  while (true) {
    if (5 < i) {
      clrd = g_GameManager.clrd;
      for (i = 0; i < 6; i += 1) {
        clrd->magic = 0x44524c43;
        clrd->th7kLen2 = sizeof(Clrd);
        clrd->th7kLen = sizeof(Clrd);
        clrd->version = 1;
        memcpy(fileBuffer + local_14, clrd, sizeof(Clrd));
        local_14 += sizeof(Clrd);
        clrd = clrd + 1;
      }
      catk = g_GameManager.catk;
      for (i = 0; i < 0x8d; i += 1) {
        if (catk->magic == 0x4b544143) {
          catk->idx = i;
          catk->th7kLen2 = sizeof(Catk);
          catk->th7kLen = sizeof(Catk);
          catk->version = 1;
          memcpy(fileBuffer + local_14, catk, sizeof(Catk));
          local_14 += sizeof(Catk);
        }
        catk = catk + 1;
      }
      pscr = &g_GameManager.pscr[0][0][0];
      for (i = 0; i < 6; i += 1) {
        for (j = 0; j < 6; j += 1) {
          for (k = 0; k < 4; k += 1) {
            if (pscr->score != 0) {
              memcpy(fileBuffer + local_14, pscr, sizeof(Pscr));
              local_14 += sizeof(Pscr);
            }
            pscr = pscr + 1;
          }
        }
      }
      memcpy(fileBuffer + local_14, &this->lsnmHeader, sizeof(Lsnm));
      g_Supervisor.UpdateStartupTime();
      memcpy(fileBuffer + local_14 + sizeof(this->lsnmHeader),
             &g_GameManager.plst, sizeof(Plst));
      vrsm.magic = 0x4d535256;
      vrsm.version = 1;
      vrsm.th7kLen2 = sizeof(Vrsm);
      vrsm.th7kLen = sizeof(Vrsm);
      vrsm.isPlayerScore = 0;
      strcpy(vrsm.versionStr, "0100b");
      vrsm.exeSize = g_Supervisor.exeSize;
      vrsm.exeChecksum = g_Supervisor.exeChecksum;
      memcpy(fileBuffer + local_14 + sizeof(Lsnm) + sizeof(Plst), &vrsm,
             sizeof(Vrsm));

      ((ScoreDat *)fileBuffer)->dstLen = local_14 + sizeof(Lsnm) + sizeof(Plst);
      ((ScoreDat *)fileBuffer)->fileLength =
          local_14 + sizeof(Lsnm) + sizeof(Plst) + sizeof(ScoreDat);
      compressedBuffer = Lzss::Compress(fileBuffer + sizeof(ScoreDat),
                                        ((ScoreDat *)fileBuffer)->dstLen,
                                        &((ScoreDat *)fileBuffer)->srcLen);
      memcpy(fileBuffer + sizeof(ScoreDat), compressedBuffer,
             ((ScoreDat *)fileBuffer)->srcLen);
      GlobalFree(compressedBuffer);
      bytesToWrite = ((ScoreDat *)fileBuffer)->srcLen + sizeof(ScoreDat);
      ((ScoreDat *)fileBuffer)->dataOffset = sizeof(ScoreDat);
      ((ScoreDat *)fileBuffer)->csum = 0;
      local_74 = ((u32)g_Rng.GetRandomU16() % 0x100);
      ((ScoreDat *)fileBuffer)->xorseed[1] = local_74;
      local_78 = ((u32)g_Rng.GetRandomU16() % 0x100);
      ((ScoreDat *)fileBuffer)->unused_6 = local_78;
      ((ScoreDat *)fileBuffer)->magic = 0xb;
      for (local_5c = 4; local_5c < (i32)bytesToWrite; local_5c += 1) {
        ((ScoreDat *)fileBuffer)->csum +=
            ((ScoreDat *)fileBuffer)->xorseed[local_5c];
      }
      local_64 = fileBuffer + 1;
      local_5d = *local_64;
      for (local_5c = ((ScoreDat *)fileBuffer)->srcLen + 0x1a; 0 < local_5c;
           local_5c -= 1) {
        uVar1 = local_64[1];
        bVar3 = ((i32)(local_5d & 0xe0) >> 5) | local_5d << 3;
        local_64[1] = local_64[1] ^ bVar3;
        local_5d = bVar3 + uVar1;
        local_64 = local_64 + 1;
      }
      FileSystem::WriteDataToFile("score.dat", fileBuffer, bytesToWrite);
      free(fileBuffer);
      return;
    }
    for (local_1c = 0; local_1c < 6; local_1c += 1) {
      local_18 = this->scoreLists[i][local_1c].next;
      local_c = 0;
      do {
        if (local_18 == NULL)
          break;
        if (local_18->data->magic == 0x52435348) {
          local_18->data->character = local_1c;
          local_18->data->difficulty = i;
          local_18->data->th7kLen2 = sizeof(Hscr);
          local_18->data->th7kLen = sizeof(Hscr);
          local_18->data->version = 1;
          local_18->data->isPlayerScore = 0;
          memcpy(fileBuffer + local_14, local_18->data, sizeof(Hscr));
          local_14 += sizeof(Hscr);
        }
        local_18 = local_18->next;
        local_c += 1;
      } while (local_c < 10);
    }
    i += 1;
  }
}

i32 ResultScreen::MoveCursor(ResultScreen *screen, i32 param_2)

{
  if ((((g_CurFrameRawInput & TH_BUTTON_UP) == 0) ||
       ((g_CurFrameRawInput & TH_BUTTON_UP) ==
        (g_LastFrameInput & TH_BUTTON_UP))) &&
      (((g_CurFrameRawInput & TH_BUTTON_UP) == 0 ||
        (g_IsEighthFrameOfHeldInput == 0)))) {
    if ((((g_CurFrameRawInput & TH_BUTTON_DOWN) == 0) ||
         ((g_CurFrameRawInput & TH_BUTTON_DOWN) ==
          (g_LastFrameInput & TH_BUTTON_DOWN))) &&
        (((g_CurFrameRawInput & TH_BUTTON_DOWN) == 0 ||
          (g_IsEighthFrameOfHeldInput == 0)))) {
      return 0;
    } else {
      screen->cursor = screen->cursor + 1;
      if (param_2 <= screen->cursor) {
        screen->cursor = screen->cursor - param_2;
      }
      g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
      return 1;
    }
  } else {
    screen->cursor = screen->cursor - 1;
    if (screen->cursor < 0) {
      screen->cursor = screen->cursor + param_2;
    }
    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
    return -1;
  }
}

i32 ResultScreen::MoveCursor2(ResultScreen *screen, i32 param_2)

{
  if ((((g_CurFrameRawInput & TH_BUTTON_UP) == 0) ||
       ((g_CurFrameRawInput & TH_BUTTON_UP) ==
        (g_LastFrameInput & TH_BUTTON_UP))) &&
      (((g_CurFrameRawInput & TH_BUTTON_UP) == 0 ||
        (g_IsEighthFrameOfHeldInput == 0)))) {
    if ((((g_CurFrameRawInput & TH_BUTTON_DOWN) == 0) ||
         ((g_CurFrameRawInput & TH_BUTTON_DOWN) ==
          (g_LastFrameInput & TH_BUTTON_DOWN))) &&
        (((g_CurFrameRawInput & TH_BUTTON_DOWN) == 0 ||
          (g_IsEighthFrameOfHeldInput == 0)))) {
      return 0;
    } else {
      screen->spellcardListPage = screen->spellcardListPage + 1;
      if (param_2 <= screen->spellcardListPage) {
        screen->spellcardListPage = screen->spellcardListPage - param_2;
      }
      g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
      return 1;
    }
  } else {
    screen->spellcardListPage = screen->spellcardListPage - 1;
    if (screen->spellcardListPage < 0) {
      screen->spellcardListPage = screen->spellcardListPage + param_2;
    }
    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
    return -1;
  }
}

i32 ResultScreen::MoveCursorHorizontally(ResultScreen *screen, i32 param_2)

{
  if ((((g_CurFrameRawInput & TH_BUTTON_LEFT) == 0) ||
       ((g_CurFrameRawInput & TH_BUTTON_LEFT) ==
        (g_LastFrameInput & TH_BUTTON_LEFT))) &&
      (((g_CurFrameRawInput & TH_BUTTON_LEFT) == 0 ||
        (g_IsEighthFrameOfHeldInput == 0)))) {
    if ((((g_CurFrameRawInput & TH_BUTTON_RIGHT) == 0) ||
         ((g_CurFrameRawInput & TH_BUTTON_RIGHT) ==
          (g_LastFrameInput & TH_BUTTON_RIGHT))) &&
        (((g_CurFrameRawInput & TH_BUTTON_RIGHT) == 0 ||
          (g_IsEighthFrameOfHeldInput == 0)))) {
      return 0;
    } else {
      screen->cursor = screen->cursor + 1;
      if (param_2 <= screen->cursor) {
        screen->cursor = screen->cursor - param_2;
      }
      g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
      return 1;
    }
  } else {
    screen->cursor = screen->cursor - 1;
    if (screen->cursor < 0) {
      screen->cursor = screen->cursor + param_2;
    }
    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
    return -1;
  }
}

i32 ResultScreen::LinkScoreEx(Hscr *out, i32 difficulty, i32 character)

{
  return LinkScore(this->scoreLists[difficulty] + character, out);
}

void ResultScreen::FreeScore(i32 param_1, i32 param_2)

{
  FreeAllScores(this->scoreLists[param_1] + param_2);
}

u32 ResultScreen::OnUpdate(ResultScreen *arg)

{
  i32 iVar1;
  i32 j;
  i32 i;
  AnmVm *vm;
  i32 curVmIdx;

  switch (arg->resultScreenState) {
  case 0:
    goto switchD_00445ddb_caseD_0;
  case 1:
    goto switchD_00445ddb_caseD_1;
  case 2:
    if (0x3b < arg->frameTimer) {
      g_Supervisor.curState = 1;
      return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    }
    break;
  case 3:
  case 4:
  case 6:
  case 7:
  case 8:
    goto switchD_00445ddb_caseD_3;
  case 5:
    if (((g_CurFrameRawInput & TH_BUTTON_FOCUS) == 0) &&
        ((g_CurFrameRawInput & TH_BUTTON_SKIP) == 0)) {
      arg->cheatCodeStep = 0;
    } else if (arg->cheatCodeStep < 3) {
      if (((g_CurFrameRawInput & TH_BUTTON_UP) == 0) ||
          ((g_CurFrameRawInput & TH_BUTTON_UP) ==
           (g_LastFrameInput & TH_BUTTON_UP))) {
        if (((g_CurFrameRawInput & TH_BUTTON_WRONG_CHEATCODE) != 0) &&
            ((g_CurFrameRawInput & TH_BUTTON_WRONG_CHEATCODE) !=
             (g_LastFrameInput & TH_BUTTON_WRONG_CHEATCODE))) {
          arg->cheatCodeStep = 0;
        }
      } else {
        arg->cheatCodeStep = arg->cheatCodeStep + 1;
      }
    } else if (arg->cheatCodeStep < 5) {
      if (((g_CurFrameRawInput & TH_BUTTON_D) == 0) ||
          ((g_CurFrameRawInput & TH_BUTTON_D) ==
           (g_LastFrameInput & TH_BUTTON_D))) {
        if (((g_CurFrameRawInput & TH_BUTTON_WRONG_CHEATCODE) != 0) &&
            ((g_CurFrameRawInput & TH_BUTTON_WRONG_CHEATCODE) !=
             (g_LastFrameInput & TH_BUTTON_WRONG_CHEATCODE))) {
          arg->cheatCodeStep = 0;
        }
      } else {
        arg->cheatCodeStep = arg->cheatCodeStep + 1;
      }
    } else if (arg->cheatCodeStep < 7) {
      if (((g_CurFrameRawInput & TH_BUTTON_DOWN) == 0) ||
          ((g_CurFrameRawInput & TH_BUTTON_DOWN) ==
           (g_LastFrameInput & TH_BUTTON_DOWN))) {
        if (((g_CurFrameRawInput & TH_BUTTON_WRONG_CHEATCODE) != 0) &&
            ((g_CurFrameRawInput & TH_BUTTON_WRONG_CHEATCODE) !=
             (g_LastFrameInput & TH_BUTTON_WRONG_CHEATCODE))) {
          arg->cheatCodeStep = 0;
        }
      } else {
        arg->cheatCodeStep = arg->cheatCodeStep + 1;
      }
    } else if (arg->cheatCodeStep < 10) {
      if (((g_CurFrameRawInput & TH_BUTTON_Q) == 0) ||
          ((g_CurFrameRawInput & TH_BUTTON_Q) ==
           (g_LastFrameInput & TH_BUTTON_Q))) {
        if (((g_CurFrameRawInput & TH_BUTTON_WRONG_CHEATCODE) != 0) &&
            ((g_CurFrameRawInput & TH_BUTTON_WRONG_CHEATCODE) !=
             (g_LastFrameInput & TH_BUTTON_WRONG_CHEATCODE))) {
          arg->cheatCodeStep = 0;
        }
      } else {
        arg->cheatCodeStep = arg->cheatCodeStep + 1;
      }
    } else {
      for (i = 0; i < 6; i += 1) {
        for (j = 0; j < 6; j += 1) {
          g_GameManager.clrd[i].difficultyClearedWithRetries[j] = 99;
          g_GameManager.clrd[i].difficultyClearedWithoutRetries[j] = 99;
        }
      }
      arg->cheatCodeStep = 0;
      g_SoundPlayer.PlaySoundByIdx(SOUND_EXTEND);
    }
  switchD_00445ddb_caseD_3:
    if ((arg->charUsed != arg->cursor) && (arg->frameTimer == 0x14)) {
      arg->charUsed = arg->cursor;
      g_AnmManager->DrawStringFormat2(arg->spellcardListVms, 0xffffff, 0,
                                      g_CharacterList[arg->charUsed]);
      arg->spellcardListVms[0].color.bytes.a = 0xff;
    }
    if (arg->frameTimer < 0x1e)
      break;
    if (MoveCursorHorizontally(arg, 6) != 0) {
      arg->frameTimer = 0;
      vm = arg->vms;
      for (curVmIdx = 0; curVmIdx < 0x29; curVmIdx += 1) {
        vm->pendingInterrupt = (i16)arg->diffPlayed + 3;
        vm = vm + 1;
      }
    }
    if (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) == 0) ||
        ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) ==
         (g_LastFrameInput & TH_BUTTON_RETURNMENU)))
      break;
    g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
    arg->resultScreenState = 0;
    arg->frameTimer = 0;
    vm = arg->vms;
    for (curVmIdx = 0; curVmIdx < 0x29; curVmIdx += 1) {
      vm->pendingInterrupt = 1;
      vm = vm + 1;
    }
    arg->prevCursor = arg->cursor;
    arg->cursor = arg->diffPlayed;
    goto switchD_00445ddb_caseD_0;
  case 9:
    if (((arg->lastSpellcardSelected != arg->cursor) ||
         (arg->prevSpellcardListPage != arg->spellcardListPage)) &&
        (arg->frameTimer == 0x14)) {
      arg->lastSpellcardSelected = arg->cursor;
      arg->prevSpellcardListPage = arg->spellcardListPage;
      for (curVmIdx = arg->lastSpellcardSelected * 10;
           (curVmIdx < arg->lastSpellcardSelected * 10 + 10 &&
            (curVmIdx < 0x8d));
           curVmIdx += 1) {
        if (g_GameManager.catk[curVmIdx].numAttemptsPerShot[6] == 0) {
          AnmManager::DrawVmTextFmt(g_AnmManager,
                                    arg->spellcardListVms + curVmIdx % 10,
                                    0xffffff, 0, "？？？？？");
        } else {
          AnmManager::DrawVmTextFmt(
              g_AnmManager, arg->spellcardListVms + curVmIdx % 10, 0xffffff, 0,
              g_GameManager.catk[curVmIdx].name);
        }
        arg->spellcardListVms[curVmIdx % 10].color.bytes.a = 0xff;
      }
      AnmManager::DrawVmTextFmt(
          g_AnmManager, arg->spellcardListVms + 10, 0xffffff, 0,
          "%s %3d枚中%3d枚取得（キャラ切り替え↓↑）",
          g_CharacterList[arg->prevSpellcardListPage], 0x8d,
          arg->totalPlayCountPerCharacter[arg->spellcardListPage + 1]);
      arg->spellcardListVms[10].color.bytes.a = 0xff;
    }
    if (arg->frameTimer < 0x1e)
      break;
    if (MoveCursorHorizontally(arg, 0xf) == 0) {
      if (MoveCursor2(arg, 7) != 0) {
        arg->frameTimer = 0;
        arg->listScrollAnimState = 1;
      }
    } else {
      arg->frameTimer = 0;
      vm = arg->vms;
      for (curVmIdx = 0; curVmIdx < 0x29; curVmIdx += 1) {
        vm->pendingInterrupt = 10;
        vm = vm + 1;
      }
    }
    if (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) == 0) ||
        ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) ==
         (g_LastFrameInput & TH_BUTTON_RETURNMENU)))
      break;
    g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
    arg->resultScreenState = 0;
    arg->frameTimer = 0;
    vm = arg->vms;
    for (curVmIdx = 0; curVmIdx < 0x29; curVmIdx += 1) {
      vm->pendingInterrupt = 1;
      vm = vm + 1;
    }
    arg->savedCursor = arg->cursor;
    arg->cursor = arg->diffPlayed;
    goto switchD_00445ddb_caseD_0;
  case 10:
    arg->HandleResultKeyboard();
    break;
  case 0xb:
  case 0xc:
  case 0xd:
  case 0xe:
  case 0xf:
    arg->HandleReplaySaveKeyboard();
    break;
  case 0x10:
  case 0x11:
    arg->CheckConfirmButton();
    break;
  case 0x12:
    g_Supervisor.curState = 1;
    return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
  case 0x13:
    return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
  case 0x14:
  case 0x15:
  case 0x16:
    if (arg->DrawStats() == ZUN_SUCCESS)
      break;
  switchD_00445ddb_caseD_0:
    if (arg->frameTimer == 0) {
      vm = arg->vms;
      for (curVmIdx = 0; curVmIdx < 0x29; curVmIdx += 1) {
        vm->pendingInterrupt = 1;
        vm->flags = vm->flags | 0x20;
        vm->color = vm->color;
        vm = vm + 1;
      }
      vm = arg->vms;
      for (curVmIdx = 0; curVmIdx < 9; curVmIdx += 1) {
        if (curVmIdx == arg->cursor) {
          (vm->color).color = 0xffffffff;
          (vm->offset).x = -4.0f;
          (vm->offset).y = -4.0f;
          (vm->offset).z = 0.0f;
        } else {
          (vm->color).color = 0xb0ffffff;
          (vm->offset).x = 0.0f;
          (vm->offset).y = 0.0f;
          (vm->offset).z = 0.0f;
        }
        vm = vm + 1;
      }
      if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0) {
        arg->vms[5].flags = arg->vms[5].flags & 0xfffffffd;
        arg->vms[6].offset.y = arg->vms[6].offset.y - 32.0f;
        arg->vms[7].offset.y = arg->vms[7].offset.y - 32.0f;
        arg->vms[8].offset.y = arg->vms[8].offset.y - 32.0f;
      } else {
        arg->vms[5].flags = arg->vms[5].flags | 2;
      }
    }
    if (arg->frameTimer < 0x14)
      break;
    arg->resultScreenState = arg->resultScreenState + 1;
    arg->frameTimer = 0;
  switchD_00445ddb_caseD_1:
    iVar1 = MoveCursor(arg, 9);
    if ((arg->cursor == 5) &&
        (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0)) {
      arg->cursor += iVar1;
    }
    vm = arg->vms;
    for (curVmIdx = 0; curVmIdx < 9; curVmIdx += 1) {
      if (curVmIdx == arg->cursor) {
        (vm->color).color = 0xffffffff;
        (vm->offset).x = -4.0f;
        (vm->offset).y = -4.0f;
        (vm->offset).z = 0.0f;
      } else {
        (vm->color).color = 0xb0ffffff;
        (vm->offset).x = 0.0f;
        (vm->offset).y = 0.0f;
        (vm->offset).z = 0.0f;
      }
      vm = vm + 1;
    }
    if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0) {
      arg->vms[5].flags = arg->vms[5].flags & 0xfffffffd;
      arg->vms[6].offset.y = arg->vms[6].offset.y - 32.0f;
      arg->vms[7].offset.y = arg->vms[7].offset.y - 32.0f;
      arg->vms[8].offset.y = arg->vms[8].offset.y - 32.0f;
    }
    if (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) == 0) ||
        ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) ==
         (g_LastFrameInput & TH_BUTTON_RETURNMENU))) {
    LAB_00446123:
      if (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) == 0) ||
          ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) ==
           (g_LastFrameInput & TH_BUTTON_SELECTMENU)))
        break;
      vm = arg->vms;
      if (arg->cursor < 0)
        break;
      if (arg->cursor < 6) {
        for (curVmIdx = 0; curVmIdx < 0x29; curVmIdx += 1) {
          vm->pendingInterrupt = arg->cursor + 3;
          vm = vm + 1;
        }
        arg->diffPlayed = arg->cursor;
        arg->resultScreenState = arg->cursor + 3;
        arg->stateStep = arg->resultScreenState;
        arg->frameTimer = 0;
        arg->cursor = arg->prevCursor;
        arg->charUsed = -1;
        arg->lastSpellcardSelected = -1;
        break;
      }
      if (arg->cursor == 6) {
        for (curVmIdx = 0; curVmIdx < 0x29; curVmIdx += 1) {
          vm->pendingInterrupt = 10;
          vm = vm + 1;
        }
        arg->diffPlayed = arg->cursor;
        arg->resultScreenState = 9;
        arg->stateStep = arg->resultScreenState;
        arg->frameTimer = 0;
        arg->charUsed = -1;
        arg->cursor = arg->savedCursor;
        arg->lastSpellcardSelected = -1;
        break;
      }
      if (arg->cursor == 7) {
        for (curVmIdx = 0; curVmIdx < 0x29; curVmIdx += 1) {
          vm->pendingInterrupt = 9;
          vm = vm + 1;
        }
        arg->diffPlayed = arg->cursor;
        arg->resultScreenState = 0x14;
        arg->stateStep = arg->resultScreenState;
        arg->frameTimer = 0;
        arg->charUsed = -1;
        break;
      }
      if (arg->cursor != 8)
        break;
    } else if (arg->cursor != 8) {
      arg->cursor = 8;
      g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
      goto LAB_00446123;
    }
    vm = arg->vms;
    for (curVmIdx = 0; curVmIdx < 0x29; curVmIdx += 1) {
      vm->pendingInterrupt = 2;
      vm = vm + 1;
    }
    arg->resultScreenState = 2;
    g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
    arg->frameTimer = 0;
  }
  vm = arg->vms;
  for (curVmIdx = 0; curVmIdx < 0x29; curVmIdx += 1) {
    g_AnmManager->ExecuteScript(vm);
    vm = vm + 1;
  }
  arg->frameTimer = arg->frameTimer + 1;
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult ResultScreen::HandleResultKeyboard()

{
  i32 local_24;
  i32 local_20;
  f32 local_10;
  AnmVm *local_c;
  i32 local_8;

  if ((Supervisor::CanSaveReplay() != 0) ||
      ((g_Supervisor.flags >> 3 & 1) != 0)) {
    this->resultScreenState = 0x10;
    this->frameTimer = 0;
    memcpy(g_GameManager.catk, g_GameManager.catkAgain, 0x4218);
    return ZUN_SUCCESS;
  }
  if (this->frameTimer == 0) {
    this->charUsed =
        (u32)g_GameManager.shotType + (u32)g_GameManager.character * 2;
    this->diffPlayed = g_GameManager.difficulty;
    local_c = this->vms;
    for (local_8 = 0; local_8 < 0x29; local_8 += 1) {
      local_c->pendingInterrupt = (i16)this->diffPlayed + 3;
      local_c = local_c + 1;
    }
    g_AnmManager->DrawStringFormat2(this->spellcardListVms, 0xffffff, 0,
                                    g_CharacterList[this->charUsed]);
    this->spellcardListVms[0].color.bytes.a = 0xff;
    (this->curScore).character = (u8)this->charUsed;
    (this->curScore).difficulty = (u8)this->diffPlayed;
    (this->curScore).score = g_GameManager.globals->score;
    (this->curScore).numRetries = g_GameManager.globals->numRetries;
    (this->curScore).version = 1;
    (this->curScore).magic = 0x52435348;
    if ((g_GameManager.flags >> 4 & 1) == 0) {
      (this->curScore).stage = (u8)g_GameManager.currentStage;
    } else {
      (this->curScore).stage = 99;
    }
    (this->curScore).isPlayerScore = 1;
    strcpy((this->curScore).name, (this->lsnmHeader).name);
    GetDate((this->curScore).date);
    local_10 =
        g_Supervisor.framerateMultiplier / g_Supervisor.fpsAccumulator - 0.5f;
    local_10 = local_10 + local_10;
    if (0.0f <= local_10) {
      if (1.0f <= local_10) {
        local_10 = 1.0f;
      }
    } else {
      local_10 = 0.0f;
    }
    (this->curScore).slowRatePercent = (u32)((1.0f - local_10) * 100.0f);
    if (9 < LinkScoreEx(&this->curScore, this->diffPlayed, this->charUsed))
      goto LAB_004470e9;
    this->cursor = 0;
    if (this->isClearingReplayName != 0) {
      this->selectedChar = 0x5f;
    }
    strcpy(this->replayName, "");
  }
  if (this->frameTimer < 0x1e) {
    return ZUN_SUCCESS;
  }
  if ((((g_CurFrameRawInput & TH_BUTTON_UP) != 0) &&
       ((g_CurFrameRawInput & TH_BUTTON_UP) !=
        (g_LastFrameInput & TH_BUTTON_UP))) ||
      (((g_CurFrameRawInput & TH_BUTTON_UP) != 0 &&
        (g_IsEighthFrameOfHeldInput != 0)))) {
    do {
      this->selectedChar = this->selectedChar - 0x10;
      if (this->selectedChar < 0) {
        this->selectedChar = this->selectedChar + 0x60;
      }
    } while (g_AlphabetList[this->selectedChar] == ' ');
    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
  }
  if ((((g_CurFrameRawInput & TH_BUTTON_DOWN) != 0) &&
       ((g_CurFrameRawInput & TH_BUTTON_DOWN) !=
        (g_LastFrameInput & TH_BUTTON_DOWN))) ||
      (((g_CurFrameRawInput & TH_BUTTON_DOWN) != 0 &&
        (g_IsEighthFrameOfHeldInput != 0)))) {
    do {
      this->selectedChar = this->selectedChar + 0x10;
      if (0x5f < this->selectedChar) {
        this->selectedChar = this->selectedChar - 0x60;
      }
    } while (g_AlphabetList[this->selectedChar] == ' ');
    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
  }
  if ((((g_CurFrameRawInput & TH_BUTTON_LEFT) != 0) &&
       ((g_CurFrameRawInput & TH_BUTTON_LEFT) !=
        (g_LastFrameInput & TH_BUTTON_LEFT))) ||
      (((g_CurFrameRawInput & TH_BUTTON_LEFT) != 0 &&
        (g_IsEighthFrameOfHeldInput != 0)))) {
    do {
      this->selectedChar = this->selectedChar - 1;
      if (this->selectedChar % 0x10 == 0xf) {
        this->selectedChar = this->selectedChar + 0x10;
      }
      if (this->selectedChar < 0) {
        this->selectedChar = 0xf;
      }
    } while (g_AlphabetList[this->selectedChar] == ' ');
    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
  }
  if ((((g_CurFrameRawInput & TH_BUTTON_RIGHT) != 0) &&
       ((g_CurFrameRawInput & TH_BUTTON_RIGHT) !=
        (g_LastFrameInput & TH_BUTTON_RIGHT))) ||
      (((g_CurFrameRawInput & TH_BUTTON_RIGHT) != 0 &&
        (g_IsEighthFrameOfHeldInput != 0)))) {
    do {
      this->selectedChar = this->selectedChar + 1;
      if (this->selectedChar % 0x10 == 0) {
        this->selectedChar = this->selectedChar - 0x10;
      }
    } while (g_AlphabetList[this->selectedChar] == ' ');
    g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
  }
  if ((((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) == 0) ||
       ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) ==
        (g_LastFrameInput & TH_BUTTON_SELECTMENU))) &&
      (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) == 0 ||
        (g_IsEighthFrameOfHeldInput == 0)))) {
  LAB_0044700b:
    if ((((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) != 0) &&
         ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) !=
          (g_LastFrameInput & TH_BUTTON_RETURNMENU))) ||
        (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) != 0 &&
          (g_IsEighthFrameOfHeldInput != 0)))) {
      if (this->cursor < 8) {
        local_24 = this->cursor;
      } else {
        local_24 = 7;
      }
      if (0 < this->cursor) {
        this->cursor = this->cursor - 1;
        (this->curScore).name[local_24] = ' ';
        (this->curScore).name[this->cursor] = ' ';
      }
      g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
    }
    if ((g_CurFrameRawInput & TH_BUTTON_MENU) == 0) {
      return ZUN_SUCCESS;
    }
    if ((g_CurFrameRawInput & TH_BUTTON_MENU) ==
        (g_LastFrameInput & TH_BUTTON_MENU)) {
      return ZUN_SUCCESS;
    }
  } else {
    if (this->cursor < 8) {
      local_20 = this->cursor;
    } else {
      local_20 = 7;
    }
    if (this->selectedChar < 0x5e) {
      (this->curScore).name[local_20] = g_AlphabetList[this->selectedChar];
    LAB_00446fd4:
      if ((this->cursor < 8) &&
          (this->cursor = this->cursor + 1, this->cursor == 8)) {
        this->selectedChar = 0x5f;
      }
      g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
      goto LAB_0044700b;
    }
    if (this->selectedChar == 0x5e) {
      (this->curScore).name[local_20] = ' ';
      goto LAB_00446fd4;
    }
  }
  g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
LAB_004470e9:
  this->resultScreenState = 0x10;
  this->frameTimer = 0;
  local_c = this->vms;
  for (local_8 = 0; local_8 < 0x29; local_8 += 1) {
    local_c->pendingInterrupt = 2;
    local_c = local_c + 1;
  }
  strcpy(this->replayName, (this->curScore).name);
  strcpy((this->lsnmHeader).name, this->replayName);
  return ZUN_SUCCESS;
}

void ResultScreen::GetDate(char *out)

{
  time_t local_c;
  tm *timeinfo;

  time(&local_c);
  timeinfo = localtime(&local_c);
  strftime(out, 6, "%m/%d", timeinfo);
}

ZunResult ResultScreen::HandleReplaySaveKeyboard()

{
  ResultScreen *_Memory;
  i32 iVar2;
  i32 local_ac;
  char local_9c[68];
  i32 local_58;
  char local_54[64];
  ReplayHeaderAndData *local_14;
  i32 local_10;
  i32 local_c;
  AnmVm *local_8;

  if (this->resultScreenState == 0xb) {
    if (this->frameTimer == 0x3c) {
      if ((Supervisor::CanSaveReplay() == 0) &&
          ((g_Supervisor.flags >> 3 & 1) == 0)) {
        if (g_GameManager.globals->numRetries == 0) {
          local_c = 0xb;
        } else {
          local_c = 0xe;
        }
      } else {
        local_c = 0x13;
      }
      local_8 = this->vms;
      for (local_10 = 0; local_10 < 0x29; local_10 += 1) {
        local_8->pendingInterrupt = (i16)local_c;
        local_8 = local_8 + 1;
      }
      if (local_c != 0xb) {
        this->resultScreenState = 0xc;
      }
      this->cursor = 0;
    }
    local_8 = this->vms + 0x13;
    if (this->cursor == 0) {
      this->vms[0x13].color.color =
          (this->vms[0x13].color.color & 0xff000000) | 0xff6060;
      this->vms[0x14].color.color =
          (this->vms[0x14].color.color & 0xff000000) | 0x606060;
    } else {
      this->vms[0x13].color.color =
          (this->vms[0x13].color.color & 0xff000000) | 0x606060;
      this->vms[0x14].color.color =
          (this->vms[0x14].color.color & 0xff000000) | 0xff6060;
    }
    if (this->frameTimer < 0x50) {
      return ZUN_SUCCESS;
    }
    MoveCursorHorizontally(this, 2);
    if ((((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) == 0) ||
         ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) ==
          (g_LastFrameInput & TH_BUTTON_RETURNMENU))) &&
        (((g_CurFrameRawInput & TH_BUTTON_MENU) == 0 ||
          ((g_CurFrameRawInput & TH_BUTTON_MENU) ==
           (g_LastFrameInput & TH_BUTTON_MENU))))) {
      if ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) == 0) {
        return ZUN_SUCCESS;
      }
      if ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) ==
          (g_LastFrameInput & TH_BUTTON_SELECTMENU)) {
        return ZUN_SUCCESS;
      }
      if (this->cursor == 0)
        goto LAB_004473e3;
    }
    this->frameTimer = 0;
    g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
    this->resultScreenState = 2;
    local_8 = this->vms;
    for (local_10 = 0; local_10 < 0x29; local_10 += 1) {
      local_8->pendingInterrupt = 2;
      local_8 = local_8 + 1;
    }
  } else {
    if (this->resultScreenState == 0xc) {
      if (this->frameTimer < 0x14) {
        return ZUN_SUCCESS;
      }
      if (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) == 0) ||
          ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) ==
           (g_LastFrameInput & TH_BUTTON_SELECTMENU))) {
        if ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) == 0) {
          return ZUN_SUCCESS;
        }
        if ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) ==
            (g_LastFrameInput & TH_BUTTON_RETURNMENU)) {
          return ZUN_SUCCESS;
        }
      }
      this->frameTimer = 0;
      g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
      this->resultScreenState = 2;
      local_8 = this->vms;
      for (local_10 = 0; local_10 < 0x29; local_10 += 1) {
        local_8->pendingInterrupt = 2;
        local_8 = local_8 + 1;
      }
      return ZUN_SUCCESS;
    }
    if (this->resultScreenState != 0xd) {
      if (this->resultScreenState == 0xe) {
        if (this->frameTimer < 0x1e) {
          return ZUN_SUCCESS;
        }
        if ((((g_CurFrameRawInput & TH_BUTTON_UP) != 0) &&
             ((g_CurFrameRawInput & TH_BUTTON_UP) !=
              (g_LastFrameInput & TH_BUTTON_UP))) ||
            (((g_CurFrameRawInput & TH_BUTTON_UP) != 0 &&
              (g_IsEighthFrameOfHeldInput != 0)))) {
          do {
            this->selectedChar = this->selectedChar - 0x10;
            if (this->selectedChar < 0) {
              this->selectedChar = this->selectedChar + 0x60;
            }
          } while (g_AlphabetList[this->selectedChar] == ' ');
          g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
        }
        if ((((g_CurFrameRawInput & TH_BUTTON_DOWN) != 0) &&
             ((g_CurFrameRawInput & TH_BUTTON_DOWN) !=
              (g_LastFrameInput & TH_BUTTON_DOWN))) ||
            (((g_CurFrameRawInput & TH_BUTTON_DOWN) != 0 &&
              (g_IsEighthFrameOfHeldInput != 0)))) {
          do {
            this->selectedChar = this->selectedChar + 0x10;
            if (0x5f < this->selectedChar) {
              this->selectedChar = this->selectedChar - 0x60;
            }
          } while (g_AlphabetList[this->selectedChar] == ' ');
          g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
        }
        if ((((g_CurFrameRawInput & TH_BUTTON_LEFT) != 0) &&
             ((g_CurFrameRawInput & TH_BUTTON_LEFT) !=
              (g_LastFrameInput & TH_BUTTON_LEFT))) ||
            (((g_CurFrameRawInput & TH_BUTTON_LEFT) != 0 &&
              (g_IsEighthFrameOfHeldInput != 0)))) {
          do {
            this->selectedChar = this->selectedChar - 1;
            if (this->selectedChar % 0x10 == 0xf) {
              this->selectedChar = this->selectedChar + 0x10;
            }
            if (this->selectedChar < 0) {
              this->selectedChar = 0xf;
            }
          } while (g_AlphabetList[this->selectedChar] == ' ');
          g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
        }
        if ((((g_CurFrameRawInput & TH_BUTTON_RIGHT) != 0) &&
             ((g_CurFrameRawInput & TH_BUTTON_RIGHT) !=
              (g_LastFrameInput & TH_BUTTON_RIGHT))) ||
            (((g_CurFrameRawInput & TH_BUTTON_RIGHT) != 0 &&
              (g_IsEighthFrameOfHeldInput != 0)))) {
          do {
            this->selectedChar = this->selectedChar + 1;
            if (this->selectedChar % 0x10 == 0) {
              this->selectedChar = this->selectedChar - 0x10;
            }
          } while (g_AlphabetList[this->selectedChar] == ' ');
          g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU);
        }
        if ((((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) != 0) &&
             ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) !=
              (g_LastFrameInput & TH_BUTTON_SELECTMENU))) ||
            (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) != 0 &&
              (g_IsEighthFrameOfHeldInput != 0)))) {
          if (this->cursor < 8) {
            local_ac = this->cursor;
          } else {
            local_ac = 7;
          }
          local_58 = local_ac;
          if (this->selectedChar < 0x5e) {
            this->replayName[local_ac] = g_AlphabetList[this->selectedChar];
          } else if (this->selectedChar == 0x5e) {
            this->replayName[local_ac] = ' ';
          } else {
            sprintf(local_9c, "./replay/th7_%.2d.rpy",
                    this->chosenReplayIdx + 1);
            ReplayManager::SaveReplay(local_9c, this->replayName);
            this->frameTimer = 0;
            this->resultScreenState = 2;
            local_8 = this->vms;
            for (local_10 = 0; local_10 < 0x29; local_10 += 1) {
              local_8->pendingInterrupt = 2;
              local_8 = local_8 + 1;
            }
            strcpy((this->lsnmHeader).name, this->replayName);
          }
          if ((this->cursor < 8) &&
              (this->cursor = this->cursor + 1, this->cursor == 8)) {
            this->selectedChar = 0x5f;
          }
          g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
        }
        if ((((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) != 0) &&
             ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) !=
              (g_LastFrameInput & TH_BUTTON_RETURNMENU))) ||
            (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) != 0 &&
              (g_IsEighthFrameOfHeldInput != 0)))) {
          if (this->cursor < 8) {
            iVar2 = this->cursor;
          } else {
            iVar2 = 7;
          }
          if (0 < this->cursor) {
            this->cursor = this->cursor - 1;
            this->replayName[iVar2] = ' ';
            this->replayName[this->cursor] = ' ';
          }
          g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
        }
        if ((g_CurFrameRawInput & TH_BUTTON_MENU) == 0) {
          return ZUN_SUCCESS;
        }
        if ((g_CurFrameRawInput & TH_BUTTON_MENU) ==
            (g_LastFrameInput & TH_BUTTON_MENU)) {
          return ZUN_SUCCESS;
        }
      } else {
        if (this->resultScreenState != 0xf) {
          return ZUN_SUCCESS;
        }
        local_8 = this->vms + 0x13;
        if (this->cursor == 0) {
          this->vms[0x13].color.color =
              (this->vms[0x13].color.color & 0xff000000) | 0xff6060;
          this->vms[0x14].color.color =
              (this->vms[0x14].color.color & 0xff000000) | 0x606060;
        } else {
          this->vms[0x13].color.color =
              (this->vms[0x13].color.color & 0xff000000) | 0x606060;
          this->vms[0x14].color.color =
              (this->vms[0x14].color.color & 0xff000000) | 0xff6060;
        }
        if (this->frameTimer < 0x14) {
          return ZUN_SUCCESS;
        }
        MoveCursorHorizontally(this, 2);
        if ((((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) == 0) ||
             ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) ==
              (g_LastFrameInput & TH_BUTTON_RETURNMENU))) &&
            (((g_CurFrameRawInput & TH_BUTTON_MENU) == 0 ||
              ((g_CurFrameRawInput & TH_BUTTON_MENU) ==
               (g_LastFrameInput & TH_BUTTON_MENU))))) {
          if ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) == 0) {
            return ZUN_SUCCESS;
          }
          if ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) ==
              (g_LastFrameInput & TH_BUTTON_SELECTMENU)) {
            return ZUN_SUCCESS;
          }
          this->frameTimer = 0;
          if (this->cursor == 0) {
            local_8 = this->vms;
            for (local_10 = 0; local_10 < 0x29; local_10 += 1) {
              local_8->pendingInterrupt = 0x11;
              local_8 = local_8 + 1;
            }
            this->vms[(i32)((i32) & ((StageReplayData *)this->chosenReplayIdx)
                                            ->extendsFromPointItems +
                                        1)]
                .pendingInterrupt = 0x10;
            this->resultScreenState = 0xe;
            return ZUN_SUCCESS;
          }
        }
      }
    LAB_004473e3:
      g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
      this->resultScreenState = 0xd;
      local_8 = this->vms;
      for (local_10 = 0; local_10 < 0x29; local_10 += 1) {
        local_8->pendingInterrupt = 0xc;
        local_8 = local_8 + 1;
      }
      this->frameTimer = 0;
    }
    if (this->frameTimer == 0) {
      _mkdir("replay");
      for (local_10 = 0; local_10 < 0xf; local_10 += 1) {
        sprintf(local_54, "./replay/th7_%.2d.rpy", local_10 + 1);
        local_14 = (ReplayHeaderAndData *)FileSystem::OpenFile(local_54, 1);
        if (local_14 == NULL) {
          local_14 = NULL;
        } else {
          _Memory = (ResultScreen *)ReplayManager::ValidateReplayData(
              local_14, g_LastFileSize);
          local_14 = (ReplayHeaderAndData *)_Memory;
          if (_Memory != NULL) {
            this->replays[local_10] = *(ReplayHeaderAndData *)_Memory;
            free(_Memory);
          }
        }
      }
    }
    if (0x13 < this->frameTimer) {
      MoveCursor(this, 0xf);
      this->chosenReplayIdx = this->cursor;
      if (((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) != 0) &&
          ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) !=
           (g_LastFrameInput & TH_BUTTON_SELECTMENU))) {
        g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT);
        this->chosenReplayIdx = this->cursor;
        this->frameTimer = 0;
        GetDate(this->defaultReplay.data.date);
        this->defaultReplay.data.score = g_GameManager.globals->score;
        if ((this->replays[this->cursor].head.magic == 0x50523754) &&
            ((this->replays[this->cursor].head.version & 0xfff) == 0x100)) {
          local_8 = this->vms;
          for (local_10 = 0; local_10 < 0x29; local_10 += 1) {
            local_8->pendingInterrupt = 0xd;
            local_8 = local_8 + 1;
          }
          local_8 = this->vms + this->chosenReplayIdx + 0x19;
          local_8->pendingInterrupt = 0x10;
          this->resultScreenState = 0xf;
        } else {
          local_8 = this->vms;
          for (local_10 = 0; local_10 < 0x29; local_10 += 1) {
            local_8->pendingInterrupt = 0x11;
            local_8 = local_8 + 1;
          }
          local_8 = this->vms + this->chosenReplayIdx + 0x19;
          local_8->pendingInterrupt = 0x10;
          this->resultScreenState = 0xe;
        }
        this->cursor = 0;
        this->selectedChar = 0;
        if (this->isClearingReplayName != 0) {
          this->selectedChar = 0x5f;
        }
      }
      if (((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) != 0) &&
          ((g_CurFrameRawInput & TH_BUTTON_RETURNMENU) !=
           (g_LastFrameInput & TH_BUTTON_RETURNMENU))) {
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK);
        this->resultScreenState = 0xb;
        local_8 = this->vms;
        for (local_10 = 0; local_10 < 0x29; local_10 += 1) {
          local_8->pendingInterrupt = 2;
          local_8 = local_8 + 1;
        }
        this->frameTimer = 0;
      }
    }
  }
  return ZUN_SUCCESS;
}

ZunResult ResultScreen::CheckConfirmButton()

{
  if (this->resultScreenState == 0x10) {
    if (this->frameTimer < 0x1f) {
      this->vms[0x28].pendingInterrupt = 0x12;
    }
    if (((0x59 < this->frameTimer) &&
         ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) != 0)) &&
        ((g_CurFrameRawInput & TH_BUTTON_SELECTMENU) !=
         (g_LastFrameInput & TH_BUTTON_SELECTMENU))) {
      this->vms[0x28].pendingInterrupt = 2;
      this->frameTimer = 0;
      this->resultScreenState = 0x11;
    }
  } else if ((this->resultScreenState == 0x11) && (0x1d < this->frameTimer)) {
    this->frameTimer = 0x3b;
    this->resultScreenState = 0xb;
  }
  return ZUN_SUCCESS;
}

i32 ResultScreen::DrawStats()

{
  f32 fVar1;
  AnmVm *vm;
  i32 local_20;
  i32 local_1c;
  i32 local_18;
  f32 local_10;
  AnmVm *local_8;

  if (this->resultScreenState == 0x14) {
    if (this->frameTimer == 1) {
      this->spellcardListVms[0].pos.x = 56.0f;
      this->spellcardListVms[0].pos.y = 128.0f;
      this->spellcardListVms[0].pos.z = 0.0f;
      g_Supervisor.UpdateStartupTime();
      AnmManager::DrawVmTextFmt(
          g_AnmManager, this->spellcardListVms, 0xffffff, 0,
          "総起動時間   %.2d:%.2d:%.2d", g_GameManager.plst.totalHours,
          g_GameManager.plst.totalMinutes, g_GameManager.plst.totalSeconds);
      g_Supervisor.UpdateStartupTime();
      this->lastTotalSeconds = g_GameManager.plst.totalSeconds;
      this->spellcardListVms[1].pos.x = 56.0f;
      this->spellcardListVms[1].pos.y = 145.0;
      this->spellcardListVms[1].pos.z = 0.0f;
      AnmManager::DrawVmTextFmt(
          g_AnmManager, this->spellcardListVms + 1, 0xffffff, 0,
          "総プレイ時間 %.2d:%.2d:%.2d", g_GameManager.plst.gameHours,
          g_GameManager.plst.gameMinutes, g_GameManager.plst.gameSeconds);
      local_8 = this->spellcardListVms + 2;
      local_10 = 162.0;
      this->spellcardListVms[2].pos.x = 56.0f;
      this->spellcardListVms[2].pos.y = 162.0;
      this->spellcardListVms[2].pos.z = 0.0f;
      if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0) {
        AnmManager::DrawVmTextFmt(
            g_AnmManager, local_8, 0xffffff, 0,
            "プレイ回数　　　 　Easy 　Norm 　Hard 　Luna  Extra  Total");
      } else {
        AnmManager::DrawVmTextFmt(g_AnmManager, local_8, 0xffffff, 0,
                                  "プレイ回数　　　 　Easy 　Norm 　Hard "
                                  "　Luna  Extra Phants  Total");
      }
      for (local_18 = 0; local_18 < 6; local_18 += 1) {
        vm = local_8 + 1;
        local_10 = local_10 + 17.0f;
        local_8[1].pos.x = 56.0f;
        local_8[1].pos.y = local_10;
        local_8[1].pos.z = 0.0f;
        if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0) {
          AnmManager::DrawVmTextFmt(
              g_AnmManager, vm, 0xffffff, 0, "%s %6d %6d %6d %6d %6d %6d",
              g_CharacterList[local_18],
              ((Plst *)(g_GameManager.pscr + 6))
                  ->playDataByDifficulty[0]
                  .playCountPerShotType[local_18],
              ((Plst *)(g_GameManager.pscr + 6))
                  ->playDataByDifficulty[1]
                  .playCountPerShotType[local_18],
              g_GameManager.plst.playDataByDifficulty[2]
                  .playCountPerShotType[local_18],
              g_GameManager.plst.playDataByDifficulty[3]
                  .playCountPerShotType[local_18],
              g_GameManager.plst.playDataTotals
                  .playCountPerShotType[local_18 - 0x16],
              g_GameManager.plst.playDataTotals.playCountPerShotType[local_18]);
        } else {
          AnmManager::DrawVmTextFmt(
              g_AnmManager, vm, 0xffffff, 0, "%s %6d %6d %6d %6d %6d %6d %6d",
              g_CharacterList[local_18],
              ((Plst *)(g_GameManager.pscr + 6))
                  ->playDataByDifficulty[0]
                  .playCountPerShotType[local_18],
              ((Plst *)(g_GameManager.pscr + 6))
                  ->playDataByDifficulty[1]
                  .playCountPerShotType[local_18],
              g_GameManager.plst.playDataByDifficulty[2]
                  .playCountPerShotType[local_18],
              g_GameManager.plst.playDataByDifficulty[3]
                  .playCountPerShotType[local_18],
              g_GameManager.plst.playDataTotals
                  .playCountPerShotType[local_18 - 0x16],
              g_GameManager.plst.playDataTotals
                  .playCountPerShotType[local_18 - 0xb],
              g_GameManager.plst.playDataTotals.playCountPerShotType[local_18]);
        }
        local_8 = vm;
      }
      local_8[1].pos.x = 56.0f;
      local_8[1].pos.y = local_10 + 17.0f;
      local_8[1].pos.z = 0.0f;
      if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0) {
        AnmManager::DrawVmTextFmt(
            g_AnmManager, local_8 + 1, 0xffffff, 0,
            "%s %6d %6d %6d %6d %6d %6d", "全主人公合計  　",
            g_GameManager.plst.playDataByDifficulty[0].playCount,
            g_GameManager.plst.playDataByDifficulty[1].playCount,
            g_GameManager.plst.playDataByDifficulty[2].playCount,
            g_GameManager.plst.playDataByDifficulty[3].playCount,
            g_GameManager.plst.playDataByDifficulty[4].playCount,
            g_GameManager.plst.playDataTotals.playCount);
      } else {
        AnmManager::DrawVmTextFmt(
            g_AnmManager, local_8 + 1, 0xffffff, 0,
            "%s %6d %6d %6d %6d %6d %6d %6d", "全主人公合計  　",
            g_GameManager.plst.playDataByDifficulty[0].playCount,
            g_GameManager.plst.playDataByDifficulty[1].playCount,
            g_GameManager.plst.playDataByDifficulty[2].playCount,
            g_GameManager.plst.playDataByDifficulty[3].playCount,
            g_GameManager.plst.playDataByDifficulty[4].playCount,
            g_GameManager.plst.playDataByDifficulty[5].playCount,
            g_GameManager.plst.playDataTotals.playCount);
      }
      fVar1 = local_10 + 17.0f + 34.0f;
      local_8[2].pos.x = 56.0f;
      local_8[2].pos.y = fVar1;
      local_8[2].pos.z = 0.0f;
      g_GameManager.plst.playDataTotals.noContinueClearCount =
          g_GameManager.plst.playDataByDifficulty[0].noContinueClearCount +
          g_GameManager.plst.playDataByDifficulty[1].noContinueClearCount +
          g_GameManager.plst.playDataByDifficulty[2].noContinueClearCount +
          g_GameManager.plst.playDataByDifficulty[3].noContinueClearCount +
          g_GameManager.plst.playDataByDifficulty[4].noContinueClearCount +
          g_GameManager.plst.playDataByDifficulty[5].noContinueClearCount;
      if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0) {
        AnmManager::DrawVmTextFmt(
            g_AnmManager, local_8 + 2, 0xffffff, 0,
            "クリア回数  　　 %6d %6d %6d %6d %6d %6d",
            g_GameManager.plst.playDataByDifficulty[0].noContinueClearCount,
            g_GameManager.plst.playDataByDifficulty[1].noContinueClearCount,
            g_GameManager.plst.playDataByDifficulty[2].noContinueClearCount,
            g_GameManager.plst.playDataByDifficulty[3].noContinueClearCount,
            g_GameManager.plst.playDataByDifficulty[4].noContinueClearCount,
            g_GameManager.plst.playDataTotals.noContinueClearCount);
      } else {
        AnmManager::DrawVmTextFmt(
            g_AnmManager, local_8 + 2, 0xffffff, 0,
            "クリア回数  　　 %6d %6d %6d %6d %6d %6d %6d",
            g_GameManager.plst.playDataByDifficulty[0].noContinueClearCount,
            g_GameManager.plst.playDataByDifficulty[1].noContinueClearCount,
            g_GameManager.plst.playDataByDifficulty[2].noContinueClearCount,
            g_GameManager.plst.playDataByDifficulty[3].noContinueClearCount,
            g_GameManager.plst.playDataByDifficulty[4].noContinueClearCount,
            g_GameManager.plst.playDataByDifficulty[5].noContinueClearCount,
            g_GameManager.plst.playDataTotals.noContinueClearCount);
      }
      fVar1 = fVar1 + 17.0f;
      local_8[3].pos.x = 56.0f;
      local_8[3].pos.y = fVar1;
      local_8[3].pos.z = 0.0f;
      if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0) {
        AnmManager::DrawVmTextFmt(
            g_AnmManager, local_8 + 3, 0xffffff, 0,
            "コンティニュー   %6d %6d %6d %6d %6d %6d",
            g_GameManager.plst.playDataByDifficulty[0].retryCount,
            g_GameManager.plst.playDataByDifficulty[1].retryCount,
            g_GameManager.plst.playDataByDifficulty[2].retryCount,
            g_GameManager.plst.playDataByDifficulty[3].retryCount,
            g_GameManager.plst.playDataByDifficulty[4].retryCount,
            g_GameManager.plst.playDataTotals.retryCount);
      } else {
        AnmManager::DrawVmTextFmt(
            g_AnmManager, local_8 + 3, 0xffffff, 0,
            "コンティニュー   %6d %6d %6d %6d %6d %6d %6d",
            g_GameManager.plst.playDataByDifficulty[0].retryCount,
            g_GameManager.plst.playDataByDifficulty[1].retryCount,
            g_GameManager.plst.playDataByDifficulty[2].retryCount,
            g_GameManager.plst.playDataByDifficulty[3].retryCount,
            g_GameManager.plst.playDataByDifficulty[4].retryCount,
            g_GameManager.plst.playDataByDifficulty[5].retryCount,
            g_GameManager.plst.playDataTotals.retryCount);
      }
      fVar1 = fVar1 + 17.0f;
      local_8[4].pos.x = 56.0f;
      local_8[4].pos.y = fVar1;
      local_8[4].pos.z = 0.0f;
      if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0) {
        AnmManager::DrawVmTextFmt(
            g_AnmManager, local_8 + 4, 0xffffff, 0,
            "プラクティス　   %6d %6d %6d %6d %6d %6d",
            g_GameManager.plst.playDataByDifficulty[0].extraClearCount,
            g_GameManager.plst.playDataByDifficulty[1].extraClearCount,
            g_GameManager.plst.playDataByDifficulty[2].extraClearCount,
            g_GameManager.plst.playDataByDifficulty[3].extraClearCount,
            g_GameManager.plst.playDataByDifficulty[4].extraClearCount,
            g_GameManager.plst.playDataTotals.extraClearCount);
      } else {
        AnmManager::DrawVmTextFmt(
            g_AnmManager, local_8 + 4, 0xffffff, 0,
            "プラクティス　   %6d %6d %6d %6d %6d %6d %6d",
            g_GameManager.plst.playDataByDifficulty[0].extraClearCount,
            g_GameManager.plst.playDataByDifficulty[1].extraClearCount,
            g_GameManager.plst.playDataByDifficulty[2].extraClearCount,
            g_GameManager.plst.playDataByDifficulty[3].extraClearCount,
            g_GameManager.plst.playDataByDifficulty[4].extraClearCount,
            g_GameManager.plst.playDataByDifficulty[5].extraClearCount,
            g_GameManager.plst.playDataTotals.extraClearCount);
      }
      local_8[5].pos.x = 56.0f;
      local_8[5].pos.y = fVar1 + 17.0f;
      local_8[5].pos.z = 0.0f;
      if (g_GameManager.HasUnlockedPhantomAndMaxClears() == 0) {
        AnmManager::DrawVmTextFmt(
            g_AnmManager, local_8 + 5, 0xffffff, 0,
            "リトライ回数  　 %6d %6d %6d %6d %6d %6d",
            g_GameManager.plst.playDataByDifficulty[0].clearCount,
            g_GameManager.plst.playDataByDifficulty[1].clearCount,
            g_GameManager.plst.playDataByDifficulty[2].clearCount,
            g_GameManager.plst.playDataByDifficulty[3].clearCount,
            g_GameManager.plst.playDataByDifficulty[4].clearCount,
            g_GameManager.plst.playDataTotals.clearCount);
      } else {
        AnmManager::DrawVmTextFmt(
            g_AnmManager, local_8 + 5, 0xffffff, 0,
            "リトライ回数  　 %6d %6d %6d %6d %6d %6d %6d",
            g_GameManager.plst.playDataByDifficulty[0].clearCount,
            g_GameManager.plst.playDataByDifficulty[1].clearCount,
            g_GameManager.plst.playDataByDifficulty[2].clearCount,
            g_GameManager.plst.playDataByDifficulty[3].clearCount,
            g_GameManager.plst.playDataByDifficulty[4].clearCount,
            g_GameManager.plst.playDataByDifficulty[5].clearCount,
            g_GameManager.plst.playDataTotals.clearCount);
      }
    }
    if (this->frameTimer < 0x28) {
      local_8 = this->spellcardListVms;
      for (local_1c = 0; local_1c < 0xe; local_1c += 1) {
        (local_8->color).bytes.a = (u8)((this->frameTimer * 0xff) / 0x28);
        local_8 = local_8 + 1;
      }
    } else {
      this->resultScreenState = 0x15;
    }
  } else if (this->resultScreenState == 0x15) {
    if ((this->frameTimer % 0x3c == 0) &&
        (g_Supervisor.UpdateStartupTime(),
         g_GameManager.plst.totalSeconds != this->lastTotalSeconds)) {
      AnmManager::DrawVmTextFmt(
          g_AnmManager, this->spellcardListVms, 0xffffff, 0,
          "総起動時間   %.2d:%.2d:%.2d", g_GameManager.plst.totalHours,
          g_GameManager.plst.totalMinutes, g_GameManager.plst.totalSeconds);
      this->lastTotalSeconds = (u8)g_GameManager.plst.totalSeconds;
    }
    if (((g_CurFrameRawInput & (TH_BUTTON_SHOOT | TH_BUTTON_BOMB |
                                TH_BUTTON_MENU | TH_BUTTON_ENTER)) != 0) &&
        ((g_CurFrameRawInput & (TH_BUTTON_SHOOT | TH_BUTTON_BOMB |
                                TH_BUTTON_MENU | TH_BUTTON_ENTER)) !=
         (g_LastFrameInput & (TH_BUTTON_SHOOT | TH_BUTTON_BOMB |
                              TH_BUTTON_MENU | TH_BUTTON_ENTER)))) {
      this->resultScreenState = 0x16;
      this->frameTimer = 0;
    }
  } else if (this->resultScreenState == 0x16) {
    if (0x13 < this->frameTimer) {
      this->resultScreenState = 0;
      this->frameTimer = 0;
      return 1;
    }
    local_8 = this->spellcardListVms;
    for (local_20 = 0; local_20 < 0xe; local_20 += 1) {
      (local_8->color).bytes.a = -(char)((this->frameTimer * 0xff) / 0x14) - 1;
      local_8 = local_8 + 1;
    }
  }
  return 0;
}

ZunResult ResultScreen::DrawFinalStats()

{
  f32 local_34;
  f32 local_30;
  f32 local_20;
  f32 local_1c;
  D3DXVECTOR3 local_14;
  AnmVm *local_8;

  if ((0xf < this->resultScreenState) && (this->resultScreenState < 0x12)) {
    local_8 = this->vms + 0x28;
    g_AsciiManager.color = this->vms[0x28].color.color;
    if ((i32)g_GameManager.difficulty < 4) {
      local_30 = (f32)g_GameManager.activeFrameCounter / 180621.0f;
    } else {
      if (g_GameManager.difficulty == DIFF_EXTRA) {
        local_34 = (f32)g_GameManager.activeFrameCounter / 80000.0f;
      } else {
        local_34 = (f32)g_GameManager.activeFrameCounter / 85000.0f;
      }
      local_30 = local_34;
    }
    local_1c = local_30;
    local_14.z = this->vms[0x28].pos.z;
    local_14.x = this->vms[0x28].pos.x + 210.0f;
    local_14.y = this->vms[0x28].pos.y + 32.0f;
    AsciiManager::AddFormatText(&g_AsciiManager, &local_14, "%9d",
                                g_GameManager.globals->guiScore);
    local_14.x = local_14.x + 126.0f;
    AsciiManager::AddFormatText(&g_AsciiManager, &local_14, "%1d",
                                g_GameManager.globals->numRetries);
    local_14.x = local_14.x - 126.0f;
    local_14.y = local_14.y + 22.0f;
    g_AsciiManager.AddString(&local_14,
                             g_DifficultyNameTable[g_GameManager.difficulty]);
    local_14.x = local_14.x + 14.0f;
    local_14.y = local_14.y + 22.0f;
    if ((g_GameManager.flags >> 4 & 1) == 0) {
      if (1.0f <= local_30) {
        local_1c = 0.99f;
      }
      AsciiManager::AddFormatText(&g_AsciiManager, &local_14, "    %3.2f%%",
                                  (f64)(local_1c * 100.0f));
    } else {
      AsciiManager::AddFormatText(&g_AsciiManager, &local_14, "      100%%");
    }
    local_14.y = local_14.y + 22.0f;
    AsciiManager::AddFormatText(&g_AsciiManager, &local_14, "%9d",
                                g_GameManager.globals->numRetries);
    local_14.y = local_14.y + 22.0f;
    AsciiManager::AddFormatText(&g_AsciiManager, &local_14, "%9d",
                                (u32)g_GameManager.globals->deaths);
    local_14.y = local_14.y + 22.0f;
    AsciiManager::AddFormatText(&g_AsciiManager, &local_14, "%9d",
                                (u32)g_GameManager.globals->bombsUsed);
    local_14.y = local_14.y + 22.0f;
    AsciiManager::AddFormatText(&g_AsciiManager, &local_14, "%9d",
                                (u32)g_GameManager.globals->spellCardsCaptured);
    local_20 =
        g_Supervisor.framerateMultiplier / g_Supervisor.fpsAccumulator - 0.5f;
    local_20 = local_20 + local_20;
    if (0.0f <= local_20) {
      if (1.0f <= local_20) {
        local_20 = 1.0f;
      }
    } else {
      local_20 = 0.0f;
    }
    local_14.y = local_14.y + 22.0f;
    AsciiManager::AddFormatText(&g_AsciiManager, &local_14, "    %3.2f%%",
                                (f64)((1.0f - local_20) * 100.0f));
    g_AsciiManager.color = 0xffffffff;
  }
  return ZUN_SUCCESS;
}

u32 ResultScreen::OnDraw(ResultScreen *arg) {
  i32 cursor2;
  i32 cursor;
  char local_58[16];
  f32 local_48;
  f32 local_44;
  i32 local_40;
  f32 local_3c;
  D3DXVECTOR3 local_38;
  i32 local_2c;
  ScoreListNode *local_28;
  AnmVm *local_24;
  char buf[9];
  i32 local_14;
  D3DXVECTOR3 local_10;

  local_24 = arg->vms;
  g_AnmManager->Flush();
  g_Supervisor.viewport.X = 0;
  g_Supervisor.viewport.Y = 0;
  g_Supervisor.viewport.Width = 0x280;
  g_Supervisor.viewport.Height = 0x1e0;
  g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
  g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
  for (local_14 = 0; local_14 < 0x29; local_14 += 1) {
    local_38 = local_24->pos;
    local_24->pos += local_24->offset;
    g_AnmManager->DrawNoRotation(local_24);
    local_24->pos = local_38;
    local_24 = local_24 + 1;
  }
  local_24 = arg->vms + 0x10;
  if (arg->vms[0x10].pos.x < 640.0f) {
    if (arg->stateStep == 9) {
      local_38 = arg->vms[0x10].pos;
      arg->spellcardListVms[10].pos = local_38;
      g_AnmManager->DrawNoRotation(arg->spellcardListVms + 10);
      local_38.y = local_38.y + 16.0f;
      for (local_14 = 0;
           (local_14 < 10 &&
            (local_40 = arg->lastSpellcardSelected * 10 + local_14,
            local_40 < 0x8d));
           local_14 += 1) {
        local_3c = local_38.x;
        local_38.x += 320.0f;
        local_38.y += 16.0f;
        (arg->rightArrowVm).pos = local_38;
        (arg->rightArrowVm).scale.x = 2.375f;
        g_AnmManager->DrawNoRotation(&arg->rightArrowVm);
        local_38.y -= 16.0f;
        local_38.x = local_3c;
        arg->spellcardListVms[local_14].pos.x = local_3c;
        arg->spellcardListVms[local_14].pos.y = local_38.y;
        arg->spellcardListVms[local_14].pos.z = local_38.z;
        if (g_GameManager.catk[local_40]
                .numAttemptsPerShot[arg->prevSpellcardListPage] == 0) {
          g_AsciiManager.color = 0xc0c0c0ff;
        } else if (g_GameManager.catk[local_40]
                       .numSuccessesPerShot[arg->prevSpellcardListPage] == 0) {
          g_AsciiManager.color = 0xffc0a0a0;
        } else {
          g_AsciiManager.color = local_14 * -0x80800 - 0xf0f01;
        }
        AsciiManager::AddFormatText(&g_AsciiManager, &local_38, "No.%.2d",
                                    local_14 + 1);
        arg->spellcardListVms[local_14].pos.x += 96.0f;
        g_AnmManager->DrawNoRotation(arg->spellcardListVms + local_14);
        local_38.x += 496.0f;
        if (g_GameManager.catk[local_40]
                .numAttemptsPerShot[arg->prevSpellcardListPage] == 0) {
          AsciiManager::AddFormatText(&g_AsciiManager, &local_38, "---/---");
        } else {
          AsciiManager::AddFormatText(
              &g_AsciiManager, &local_38, "%3d/%3d",
              g_GameManager.catk[local_40]
                  .numAttemptsPerShot[arg->prevSpellcardListPage],
              g_GameManager.catk[local_40]
                  .numSuccessesPerShot[arg->prevSpellcardListPage]);
        }
        local_38.x = (local_38.x - 496.0f) + 424.0f;
        local_38.y -= 13.0f;
        g_AsciiManager.color = 0xffa08090;
        g_AsciiManager.scale.x = 0.8f;
        g_AsciiManager.scale.y = 0.8f;
        if (g_GameManager.catk[local_40]
                .numAttemptsPerShot[arg->prevSpellcardListPage] != 0) {
          AsciiManager::AddFormatText(
              &g_AsciiManager, &local_38, "MaxBonus %8d",
              g_GameManager.catk[local_40]
                  .highScorePerShot[arg->prevSpellcardListPage]);
        }
        local_38.x -= 424.0f;
        local_38.y += 13.0f;
        g_AsciiManager.scale.x = 1.0f;
        g_AsciiManager.scale.y = 1.0f;
        if (arg->listScrollAnimState == 0) {
          local_38.y = local_38.y + 33.0f;
        } else if (arg->frameTimer < 0x14) {
          local_38.y =
              (f32)(((0x14 - arg->frameTimer) * 0x21) / 0x14) + local_38.y;
        } else {
          local_38.y =
              (f32)(((arg->frameTimer - 0x14) * 0x21) / 0x14) + local_38.y;
        }
      }
      if (0x27 < arg->frameTimer) {
        arg->listScrollAnimState = 0;
      }
    } else {
      local_38 = arg->vms[0x10].pos;
      arg->spellcardListVms[0].pos = local_38;
      arg->spellcardListVms[0].pos.x += 64.0f;
      g_AnmManager->DrawNoRotation(arg->spellcardListVms);
      local_38.y += 18.0f;
      local_38.x += 24.0f;
      g_AsciiManager.color = 0xffe0e0ef;
      AsciiManager::AddFormatText(&g_AsciiManager, &local_38,
                                  "No  Name      Score(Stage)  Date   Slow");
      local_28 = arg->scoreLists[arg->diffPlayed][arg->charUsed].next;
      for (local_14 = 0; local_38.y = local_38.y + 18.0f, local_14 < 10;
           local_14 += 1) {
        if (arg->resultScreenState == 10) {
          if (local_28->data->isPlayerScore == 0) {
            g_AsciiManager.color = 0xc0ffc0c0;
          } else {
            g_AsciiManager.color = 0xfff0f0ff;
          }
        } else {
          g_AsciiManager.color = 0xffffc0c0;
        }
        AsciiManager::AddFormatText(&g_AsciiManager, &local_38, "%2d",
                                    local_14 + 1);
        local_38.x += 48.0f;
        if ((arg->resultScreenState == 10) &&
            (local_28->data->isPlayerScore != 0)) {
          strncpy(buf, "        ", 9);
          if (arg->cursor < 8) {
            cursor = arg->cursor;
          } else {
            cursor = 7;
          }
          buf[cursor] = '_';
          AsciiManager::AddFormatText(&g_AsciiManager, &local_38, "%8s", buf);
        }
        if (local_28->data->stage < 7) {
          AsciiManager::AddFormatText(
              &g_AsciiManager, &local_38, "%8s %9d%1d(%d)",
              local_28->data->name, local_28->data->score,
              (i32)local_28->data->numRetries, (u32)local_28->data->stage);
        } else if ((local_28->data->stage == 7) ||
                   (local_28->data->stage == 8)) {
          AsciiManager::AddFormatText(
              &g_AsciiManager, &local_38, "%8s %9d%1d(1)", local_28->data->name,
              local_28->data->score, (i32)local_28->data->numRetries);
        } else {
          AsciiManager::AddFormatText(
              &g_AsciiManager, &local_38, "%8s %9d%1d(C)", local_28->data->name,
              local_28->data->score, (i32)local_28->data->numRetries);
        }
        local_38.x += 320.0f;
        AsciiManager::AddFormatText(&g_AsciiManager, &local_38, " %5s   %3.2f",
                                    local_28->data->date);
        local_38.x -= 368.0f;
        local_28 = local_28->next;
      }
    }
  }
  if ((arg->resultScreenState == 10) || (arg->resultScreenState == 0xe)) {
    local_38.x = 160.0f;
    local_38.y = 356.0f;
    local_38.z = 0.0f;
    for (local_14 = 0; local_14 < 6; local_14 += 1) {
      for (local_2c = 0; local_2c < 0x10; local_2c += 1) {
        local_44 = 0.0f;
        if (arg->selectedChar == local_14 * 0x10 + local_2c) {
          g_AsciiManager.color = 0xffffffc0;
          if (arg->frameTimer % 0x40 < 0x20) {
            local_44 = ((f32)(arg->frameTimer % 0x20) * 0.8f) / 32.0f + 1.2f;
          } else {
            local_44 = 2.0f - ((f32)(arg->frameTimer % 0x20) * 0.8f) / 32.0f;
          }
          g_AsciiManager.scale.y = local_44;
          local_44 = -(local_44 - 1.0f) * 8.0f;
        } else {
          g_AsciiManager.color = 0xc0c0c0c0;
          g_AsciiManager.scale.y = 1.0f;
        }
        local_10.z = local_38.z;
        local_10.x = local_38.x + local_44;
        local_10.y = local_38.y + local_44;
        local_58[0] = g_AlphabetList[local_14 * 0x10 + local_2c];
        local_58[1] = 0;
        if (local_14 == 5) {
          if (local_2c == 0xe) {
            local_58[0] = -0x80;
          } else if (local_2c == 0xf) {
            local_58[0] = -0x7f;
          }
        }
        g_AsciiManager.scale.x = g_AsciiManager.scale.y;
        local_48 = local_44;
        g_AsciiManager.AddString(&local_10, local_58);
        local_38.x += 20.0f;
      }
      local_38.x -= (f32)(local_2c * 0x14);
      local_38.y += 18.0f;
    }
  }
  g_AsciiManager.scale.x = 1.0f;
  g_AsciiManager.scale.y = 1.0f;
  if ((10 < arg->resultScreenState) && (arg->resultScreenState < 0x10)) {
    local_24 = arg->vms + 0x12;
    for (local_14 = 0; local_14 < 6; local_14 += 1) {
      g_AnmManager->DrawNoRotation(local_24);
      local_24 = local_24 + 1;
    }
    local_38 = arg->vms[0x18].pos;
    local_24 = arg->vms + 0x19;
    AsciiManager::AddFormatText(&g_AsciiManager, &local_38,
                                "No.   Name     Date   Player Score");
    for (local_14 = 0; local_14 < 0xf; local_14 += 1) {
      local_38 = local_24->pos;
      local_24 = local_24 + 1;
      if (local_14 == arg->chosenReplayIdx) {
        g_AsciiManager.color = 0xffff8080;
      } else {
        g_AsciiManager.color = 0xff808080;
      }
      if (arg->resultScreenState == 0xe) {
        AsciiManager::AddFormatText(
            &g_AsciiManager, &local_38, "No.%.2d %8s %5s  %7s %9d0",
            local_14 + 1, arg->replayName, (arg->defaultReplay).data.date,
            g_CharactersAndShotTypesStrings[(u32)g_GameManager.shotType +
                                            (u32)g_GameManager.character * 2]);
        g_AsciiManager.color = 0xfff0f0ff;
        strncpy(buf, "        ", 9);
        if (arg->cursor < 8) {
          cursor2 = arg->cursor;
        } else {
          cursor2 = 7;
        }
        buf[cursor2] = '_';
        AsciiManager::AddFormatText(&g_AsciiManager, &local_38, "      %8s",
                                    buf);
      } else if ((arg->replays[local_14].head.magic == 0x50523754) &&
                 ((arg->replays[local_14].head.version & 0xfff) == 0x100)) {
        AsciiManager::AddFormatText(
            &g_AsciiManager, &local_38, "No.%.2d %8s %5s  %7s %9d0",
            local_14 + 1, arg->replays[local_14].data.name,
            arg->replays[local_14].data.date,
            g_CharactersAndShotTypesStrings[arg->replays[local_14]
                                                .data.shotType]);
      } else {
        AsciiManager::AddFormatText(
            &g_AsciiManager, &local_38,
            "No.%.2d -------- --/--  -------          0");
      }
    }
  }
  g_AsciiManager.color = 0xffffffff;
  arg->DrawFinalStats();
  if (((arg->resultScreenState == 0x14) || (arg->resultScreenState == 0x15)) ||
      (arg->resultScreenState == 0x16)) {
    local_24 = arg->spellcardListVms;
    for (local_14 = 0; local_14 < 0xe; local_14 += 1) {
      g_AnmManager->DrawNoRotation(local_24);
      local_24 = local_24 + 1;
    }
  }
  return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult ResultScreen::AddedCallback(ResultScreen *arg) {
  i16 local_44;
  i32 local_1c;
  Catk *local_18;
  AnmVm *local_c;

  g_GameManager.HasUnlockedPhantomAndMaxClears();
  for (i32 i = 0; i < 6; i = i + 1) {
    for (i32 j = 0; j < 6; j = j + 1) {
      for (i32 k = 0; k < 10; k = k + 1) {
        arg->defaultScores[i][j][k].score = k * -10000 + 100000;
        arg->defaultScores[i][j][k].slowRatePercent = 0;
        arg->defaultScores[i][j][k].magic = 0x53594d44;
        arg->defaultScores[i][j][k].difficulty = (u8)i;
        arg->defaultScores[i][j][k].version = 1;
        arg->defaultScores[i][j][k].th7kLen2 = sizeof(Hscr);
        arg->defaultScores[i][j][k].th7kLen = sizeof(Hscr);
        arg->defaultScores[i][j][k].stage = 1;
        arg->defaultScores[i][j][k].isPlayerScore = 0;
        arg->defaultScores[i][j][k].numRetries = 0;
        arg->LinkScoreEx(arg->defaultScores[i][j] + k, i, j);
        strcpy(arg->defaultScores[i][j][k].name, "--------");
        strcpy(arg->defaultScores[i][j][k].date, "--/--");
      }
    }
  }
  if (arg->resultScreenState != 0x13) {
    if (g_AnmManager->LoadSurface(0, "data/result/result.jpg") != ZUN_SUCCESS) {
      return ZUN_ERROR;
    }
    if (g_AnmManager->LoadAnms(0x2a, "data/result00.anm", 0x900) !=
        ZUN_SUCCESS) {
      return ZUN_ERROR;
    }
    local_c = arg->vms;
    for (i32 i = 0; i < 0x29; i = i + 1) {
      (local_c->pos).x = 0.0f;
      (local_c->pos).y = 0.0f;
      (local_c->pos).z = 0.0f;
      (local_c->offset).x = 0.0f;
      (local_c->offset).y = 0.0f;
      (local_c->offset).z = 0.0f;
      local_44 = (i16)i + 0x900;
      local_c->anmFileIdx = local_44;
      g_AnmManager->SetAndExecuteScript(local_c,
                                        g_AnmManager->scripts[i + 0x900]);
      local_c = local_c + 1;
    }
    arg->rightArrowVm.Initialize();
    g_AnmManager->SetActiveSprite(&arg->rightArrowVm, 0x910);
    local_c = arg->spellcardListVms;
    for (i32 i = 0; i < 0xf; i = i + 1) {
      local_c->Initialize();
      g_AnmManager->SetActiveSprite(local_c, i + 0x715);
      (local_c->pos).x = 0.0f;
      (local_c->pos).y = 0.0f;
      (local_c->pos).z = 0.0f;
      local_c->flags = local_c->flags | 0xc00;
      local_c->fontWidth = '\x0f';
      local_c->fontHeight = '\x0f';
      local_c = local_c + 1;
    }
  }
  arg->prevCursor = 0;
  arg->scoreDat = OpenScore("score.dat");
  for (i32 i = 0; i < 6; i = i + 1) {
    for (i32 j = 0; j < 6; j = j + 1) {
      GetHighScore(arg->scoreDat, arg->scoreLists[i] + j, j, i, NULL);
    }
  }
  (arg->lsnmHeader).magic = 0x4d4e534c;
  (arg->lsnmHeader).version = 1;
  (arg->lsnmHeader).th7kLen2 = sizeof(Lsnm);
  (arg->lsnmHeader).th7kLen = sizeof(Lsnm);
  strcpy((arg->lsnmHeader).name, "        ");
  arg->isClearingReplayName = ParseLsnm(arg->scoreDat, &arg->lsnmHeader);
  if ((arg->resultScreenState != 10) && (arg->resultScreenState != 0x12)) {
    ParseCatk(arg->scoreDat, g_GameManager.catk);
    ParseClrd(arg->scoreDat, g_GameManager.clrd);
    g_GameManager.HasUnlockedPhantomAndMaxClears();
    ParsePscr(arg->scoreDat, &g_GameManager.pscr[0][0][0]);
  }
  if (arg->resultScreenState == 0x12) {
    if (g_GameManager
            .pscr[g_GameManager.shotType + (u32)g_GameManager.character * 2]
                 [g_GameManager.currentStage - 1][g_GameManager.difficulty]
            .score < g_GameManager.globals->score) {
      g_GameManager
          .pscr[g_GameManager.shotType + (u32)g_GameManager.character * 2]
               [g_GameManager.currentStage - 1][g_GameManager.difficulty]
          .score = g_GameManager.globals->score;
    }
    arg->resultScreenState = 0xb;
    strcpy(arg->replayName, (arg->lsnmHeader).name);
  }
  for (i32 i = 0; i < 7; i = i + 1) {
    local_18 = g_GameManager.catk;
    arg->totalPlayCountPerCharacter[i + 1] = 0;
    for (local_1c = 0; local_1c < 0x8d; local_1c = local_1c + 1) {
      if (((local_18->magic == 0x4b544143) && (local_18->version == 1)) &&
          (local_18->numSuccessesPerShot[i] != 0)) {
        arg->totalPlayCountPerCharacter[i + 1] =
            arg->totalPlayCountPerCharacter[i + 1] + 1;
      }
      local_18 = local_18 + 1;
    }
  }
  arg->spellcardListPage = 6;
  arg->prevSpellcardListPage = 6;
  arg->listScrollAnimState = 0;
  (arg->leftArrowVm).activeSpriteIdx = -1;
  if (arg->resultScreenState == 0x13) {
    DeletedCallback(arg);
    return ZUN_ERROR;
  } else {
    return ZUN_SUCCESS;
  }
}

ZunResult ResultScreen::DeletedCallback(ResultScreen *arg) {
  if (arg->scoreDat != NULL) {
    arg->WriteScore();
    ReleaseScoreDat(arg->scoreDat);
  }
  arg->scoreDat = NULL;
  for (i32 i = 0; i < 6; i = i + 1) {
    for (i32 j = 0; j < 6; j = j + 1) {
      arg->FreeScore(i, j);
    }
  }
  g_AnmManager->ReleaseAnm(0x2a);
  g_AnmManager->ReleaseAnm(0x2b);
  g_AnmManager->ReleaseAnm(0x2c);
  g_AnmManager->ReleaseAnm(0x2d);
  g_AnmManager->ReleaseSurface(0);
  g_Chain.Cut(arg->drawChain);
  arg->drawChain = NULL;
  if (arg != NULL) {
    free(arg->scoreDat);
    free(arg);
  }
  return ZUN_SUCCESS;
}

ZunResult ResultScreen::RegisterChain(u32 param_1) {
  ResultScreen *resultScreen = new ResultScreen;
  Supervisor::DebugPrint2("Stg.PlayTimeAll = %d\r\n",
                     g_GameManager.activeFrameCounter);
  if (param_1 == 1) {
    if ((g_GameManager.flags & 1) == 0) {
      resultScreen->resultScreenState = 10;
    } else {
      resultScreen->resultScreenState = 0x12;
    }
  } else if (param_1 == 2) {
    resultScreen->resultScreenState = 0x13;
    AddedCallback(resultScreen);
    return ZUN_SUCCESS;
  }
  ChainElem *calcChain = Chain::CreateElem((ChainCallback)OnUpdate);
  resultScreen->calcChain = calcChain;
  resultScreen->calcChain->addedCallback =
      (ChainLifecycleCallback)AddedCallback;
  resultScreen->calcChain->deletedCallback =
      (ChainLifecycleCallback)DeletedCallback;
  resultScreen->calcChain->arg = resultScreen;
  if (g_Chain.AddToCalcChain(resultScreen->calcChain, 0xe) == 0) {
    ChainElem *drawChain = Chain::CreateElem((ChainCallback)OnDraw);
    resultScreen->drawChain = drawChain;
    resultScreen->drawChain->arg = resultScreen;
    g_Chain.AddToDrawChain(resultScreen->drawChain, 0xd);
    return ZUN_SUCCESS;
  } else {
    return ZUN_ERROR;
  }
}
