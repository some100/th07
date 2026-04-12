#include "SoundPlayer.hpp"

#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "Supervisor.hpp"
#include "dxutil.hpp"

// GLOBAL: TH07 0x004ba0d8
SoundPlayer g_SoundPlayer;

// GLOBAL: TH07 0x0049ea88
SoundBufferIdxVolume SOUND_BUFFER_IDX_VOL[38] = {
    {0, -2000, 0},   {0, -2500, 0},   {1, -1200, 5},   {1, -1500, 5},
    {2, -1000, 100}, {3, -400, 100},  {4, -400, 100},  {5, -1500, 50},
    {6, -1700, 50},  {7, -1900, 50},  {8, -1000, 100}, {9, -1000, 100},
    {10, -1700, 10}, {11, -1200, 10}, {12, -900, 100}, {5, -1500, 50},
    {13, -900, 50},  {14, -900, 50},  {15, -900, 100}, {16, -200, 100},
    {17, -1400, 0},  {18, -1300, 0},  {5, -100, 20},   {6, -1800, 20},
    {7, -1800, 20},  {19, -800, 50},  {20, -1000, 50}, {21, -1300, 50},
    {22, -300, 140}, {23, -900, 100}, {24, -900, 20},  {25, -500, 90},
    {26, -300, 100}, {27, -300, 100}, {24, -300, 20},  {19, 0, 50},
    {28, -300, 100}, {29, -300, 100}};

// GLOBAL: TH07 0x0049ebb8
const char *g_SFXList[30] = {
    // STRING: TH07 0x00496344
    "data/wav/se_plst00.wav",
    // STRING: TH07 0x0049632c
    "data/wav/se_enep00.wav",
    // STRING: TH07 0x00496310
    "data/wav/se_pldead00.wav",
    // STRING: TH07 0x004962f8
    "data/wav/se_power0.wav",
    // STRING: TH07 0x004962e0
    "data/wav/se_power1.wav",
    // STRING: TH07 0x004962c8
    "data/wav/se_tan00.wav",
    // STRING: TH07 0x004962b0
    "data/wav/se_tan01.wav",
    // STRING: TH07 0x00496298
    "data/wav/se_tan02.wav",
    // STRING: TH07 0x00496280
    "data/wav/se_ok00.wav",
    // STRING: TH07 0x00496264
    "data/wav/se_cancel00.wav",
    // STRING: TH07 0x00496248
    "data/wav/se_select00.wav",
    // STRING: TH07 0x00496230
    "data/wav/se_gun00.wav",
    // STRING: TH07 0x00496218
    "data/wav/se_cat00.wav",
    // STRING: TH07 0x00496200
    "data/wav/se_lazer00.wav",
    // STRING: TH07 0x004961e8
    "data/wav/se_lazer01.wav",
    // STRING: TH07 0x004961d0
    "data/wav/se_enep01.wav",
    // STRING: TH07 0x004961b8
    "data/wav/se_nep00.wav",
    // STRING: TH07 0x0049619c
    "data/wav/se_damage00.wav",
    // STRING: TH07 0x00496184
    "data/wav/se_item00.wav",
    // STRING: TH07 0x0049616c
    "data/wav/se_kira00.wav",
    // STRING: TH07 0x00496154
    "data/wav/se_kira01.wav",
    // STRING: TH07 0x0049613c
    "data/wav/se_kira02.wav",
    // STRING: TH07 0x00496124
    "data/wav/se_extend.wav",
    // STRING: TH07 0x0049610c
    "data/wav/se_timeout.wav",
    // STRING: TH07 0x004960f4
    "data/wav/se_graze.wav",
    // STRING: TH07 0x004960dc
    "data/wav/se_powerup.wav",
    // STRING: TH07 0x004960c4
    "data/wav/se_border.wav",
    // STRING: TH07 0x004960ac
    "data/wav/se_bonus.wav",
    // STRING: TH07 0x00496094
    "data/wav/se_bonus2.wav",
    // STRING: TH07 0x0049607c
    "data/wav/se_pause.wav"};

// FUNCTION: TH07 0x0044b510
SoundPlayer::SoundPlayer()

{
  memset(this, 0, sizeof(SoundPlayer));
  for (i32 i = 0; i < 0x80; ++i)
    this->unusedSoundVolRelated[i] = -1;
}

// FUNCTION: TH07 0x0044b560
ZunResult SoundPlayer::InitializeDSound(HWND gameWindow)

{
  tWAVEFORMATEX wavFormat;
  LPVOID audioBuffer1Start;
  DWORD audioBuffer1Len;
  DWORD audioBuffer2Len;
  LPVOID audioBuffer2Start;
  DSBUFFERDESC bufdesc;

  memset(this, 0, sizeof(SoundPlayer));
  for (i32 i = 0; i < 0x80; i = i + 1) {
    this->unusedSoundVolRelated[i] = -1;
  }
  this->manager = new CSoundManager;
  if (FAILED(this->manager->Initialize(gameWindow, 2, 2, 0xac44, 0x10))) {
    // STRING: TH07 0x0049604c
    g_GameErrorContext.Log("DirectSound オブジェクトの初期化が失敗したよ\r\n");
    SAFE_DELETE(this->manager);
    return ZUN_ERROR;
  } else {
    this->directSoundHdl = this->manager->pDS;
    this->backgroundMusicThreadHandle = NULL;
    memset(&bufdesc, 0, sizeof(DSBUFFERDESC));
    bufdesc.dwSize = 0x24;
    bufdesc.dwFlags = 0x8008;
    bufdesc.dwBufferBytes = 0x8000;
    wavFormat.cbSize = 0;
    wavFormat.wFormatTag = 1;
    wavFormat.nChannels = 2;
    wavFormat.nSamplesPerSec = 0xac44;
    wavFormat.nAvgBytesPerSec = 0x2b110;
    wavFormat.nBlockAlign = 4;
    wavFormat.wBitsPerSample = 0x10;
    bufdesc.lpwfxFormat = &wavFormat;
    if (FAILED(this->directSoundHdl->CreateSoundBuffer(
            &bufdesc, &this->initSoundBuffer, NULL))) {
      return ZUN_ERROR;
    } else {
      if (FAILED(this->initSoundBuffer->Lock(
              0, 0x8000, &audioBuffer1Start, &audioBuffer1Len,
              &audioBuffer2Start, &audioBuffer2Len, 0))) {
        return ZUN_ERROR;
      } else {
        memset(audioBuffer1Start, 0, 0x8000);
        this->initSoundBuffer->Unlock(audioBuffer1Start, audioBuffer1Len,
                                      audioBuffer2Start, audioBuffer2Len);
        this->initSoundBuffer->Play(0, 0, 1);
        SetTimer(gameWindow, 0, 0xfa, NULL);
        this->gameWindow = gameWindow;
        // STRING: TH07 0x00496024
        g_GameErrorContext.Log("DirectSound は正常に初期化されました\r\n");
        return ZUN_SUCCESS;
      }
    }
  }
}

// FUNCTION: TH07 0x0044b830
ZunResult SoundPlayer::Release()

{
  if (this->manager != NULL) {
    for (i32 i = 0; i < 0x80; i = i + 1) {
      SAFE_RELEASE(this->duplicateSoundBuffers[i]);
      SAFE_RELEASE(this->soundBuffers[i]);
    }
    KillTimer(this->gameWindow, 1);
    StopBGM();
    this->directSoundHdl = NULL;
    this->initSoundBuffer->Stop();
    SAFE_RELEASE(this->initSoundBuffer);
    SAFE_DELETE(this->backgroundMusic);
    SAFE_DELETE(this->manager);
    for (i32 i = 0; i < 0x10; i = i + 1) {
      SAFE_FREE(this->bgmPreloadData[i]);
    }
    if (this->bgmFmtData != NULL) {
      free(this->bgmFmtData);
    }
  }
  return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0044ba80
WAVEFORMATEX *
SoundPlayer::GetWavFormatData(u8 *soundData, const char *formatString,
                              i32 *formatSize, i32 fileSizeExcludingFormat)

{
  u8 *tmpSoundData;

  tmpSoundData = soundData;
  while (true) {
    if (fileSizeExcludingFormat == 0) {
      return NULL;
    }
    *formatSize = *(i32 *)(tmpSoundData + 4);
    if (strncmp((char *)tmpSoundData, formatString, 4) == 0)
      break;
    fileSizeExcludingFormat = fileSizeExcludingFormat - (*formatSize + 8);
    tmpSoundData = tmpSoundData + *formatSize + 8;
  }
  return (WAVEFORMATEX *)(tmpSoundData + 8);
}

// FUNCTION: TH07 0x0044baf0
i32 SoundPlayer::GetFmtIndexByName(const char *param_1)

{
  char local_8c[128];
  i32 local_c;
  const char *local_8;

  local_c = 0;
  local_8 = strrchr(param_1, '/');
  if (local_8 == NULL) {
    local_8 = strrchr(param_1, '\\');
  }
  if (local_8 == NULL) {
    strcpy(local_8c, param_1);
  } else {
    strcpy(local_8c, local_8 + 1);
  }
  while (this->bgmFmtData[local_c].name[0] != '\0') {
    if (strcmp(this->bgmFmtData[local_c].name, local_8c) == 0)
      break;
    local_c = local_c + 1;
  }
  if (this->bgmFmtData[local_c].name[0] == '\0') {
    local_c = 0;
  }
  return local_c;
}

// FUNCTION: TH07 0x0044bd00
ZunResult SoundPlayer::LoadSound(i32 idx, const char *path)

{
  u8 *soundFileDat;
  WAVEFORMATEX wavData;
  WAVEFORMATEX *local_44;
  DWORD local_40;
  DWORD local_3c;
  WAVEFORMATEX *local_38;
  DWORD local_34;
  WAVEFORMATEX *local_30;
  DSBUFFERDESC dsBuffer;

  if (this->manager == NULL) {
    return ZUN_SUCCESS;
  } else {
    SAFE_RELEASE(this->soundBuffers[idx]);
    soundFileDat = FileSystem::OpenFile(path, 0);
    if (soundFileDat == NULL) {
      return ZUN_ERROR;
    } else {
      // STRING: TH07 0x0049601c
      if (strncmp((char *)soundFileDat, "RIFF", 4) == 0) {
        i32 fileSize = *(i32 *)(soundFileDat + 4);
        // STRING: TH07 0x00495ff8
        if (strncmp((char *)soundFileDat + 8, "WAVE", 4) == 0) {
          // STRING: TH07 0x00495fd4
          local_30 = GetWavFormatData((u8 *)(soundFileDat + 0xc), "fmt ",
                                      (i32 *)&local_34, fileSize - 12);
          if (local_30 == NULL) {
            // STRING: TH07 0x00495fdc
            g_GameErrorContext.Log("Wav ファイルじゃない? %s\r\n", path);
            free(soundFileDat);
            return ZUN_ERROR;
          } else {
            wavData.wFormatTag = local_30->wFormatTag;
            wavData.nChannels = local_30->nChannels;
            wavData.nSamplesPerSec = local_30->nSamplesPerSec;
            wavData.nAvgBytesPerSec = local_30->nAvgBytesPerSec;
            wavData.nBlockAlign = local_30->nBlockAlign;
            wavData.wBitsPerSample = local_30->wBitsPerSample;
            wavData.cbSize = local_30->cbSize;
            // STRING: TH07 0x00495fcc
            local_30 = GetWavFormatData(soundFileDat + 12, "data",
                                        (i32 *)&local_34, fileSize - 12);
            if (local_30 == NULL) {
              g_GameErrorContext.Log("Wav ファイルじゃない? %s\r\n", path);
              free(soundFileDat);
              return ZUN_ERROR;
            } else {
              memset(&dsBuffer, 0, sizeof(DSBUFFERDESC));
              dsBuffer.dwSize = 0x24;
              dsBuffer.dwFlags = 0x8088;
              dsBuffer.dwBufferBytes = local_34;
              dsBuffer.lpwfxFormat = &wavData;
              if (FAILED(this->directSoundHdl->CreateSoundBuffer(
                      &dsBuffer, &this->soundBuffers[idx], NULL))) {
                free(soundFileDat);
                return ZUN_ERROR;
              } else {
                if (FAILED(this->soundBuffers[idx]->Lock(
                        0, local_34, (LPVOID *)&local_44, &local_40,
                        (LPVOID *)&local_38, &local_3c, 0))) {
                  free(soundFileDat);
                  return ZUN_ERROR;
                } else {
                  memcpy(local_44, local_30, local_40);
                  if (local_3c != 0) {
                    memcpy(local_38, (u8 *)local_30 + local_40, local_3c);
                  }
                  this->soundBuffers[idx]->Unlock(local_44, local_40, local_38,
                                                  local_3c);
                  free(soundFileDat);
                  return ZUN_SUCCESS;
                }
              }
            }
          }
        } else {
          g_GameErrorContext.Log("Wav ファイルじゃない? %s\r\n", path);
          free(soundFileDat);
          return ZUN_ERROR;
        }
      } else {
        // STRING: TH07 0x00496000
        g_GameErrorContext.Log("Wav ファイルじゃない %s\r\n", path);
        free(soundFileDat);
        return ZUN_ERROR;
      }
    }
  }
}

// FUNCTION: TH07 0x0044bff0
i32 SoundPlayer::LoadFmt(const char *param_1)

{
  this->bgmFmtData = (ThBgmFormat *)FileSystem::OpenFile(param_1, 0);
  return (this->bgmFmtData != NULL) - 1;
}

// FUNCTION: TH07 0x0044c020
ZunResult SoundPlayer::StartBGM(const char *path)

{
  ThBgmFormat *pzwf;
  u32 uVar5;
  u32 uVar6;

  strcpy(this->bgmArchivePath, path);
  if (this->manager == NULL) {
    return ZUN_ERROR;
  } else if (this->directSoundHdl == NULL) {
    return ZUN_ERROR;
  } else {
    // STRING: TH07 0x00495fb4
    DebugPrint("Streming BGM Start\r\n");
    StopBGM();
    pzwf = this->bgmFmtData;
    uVar5 = (u32)(pzwf->format).nBlockAlign;
    uVar6 = (pzwf->format).nSamplesPerSec * 4 * uVar5 >> 4;
    this->backgroundMusicUpdateEvent = CreateEventA(NULL, 0, 0, NULL);
    this->backgroundMusicThreadHandle = CreateThread(
        NULL, 0, BackgroundMusicPlayerThread, g_Supervisor.hwndGameWindow, 0,
        &this->backgroundMusicThreadId);
    if (FAILED(this->manager->CreateStreaming(
            &this->backgroundMusic, path, 0x10100, GUID_NULL, 0x10,
            uVar6 - uVar6 % uVar5, this->backgroundMusicUpdateEvent, pzwf))) {
      // STRING: TH07 0x00495f70
      DebugPrint("error : ストリーミング用サウンドバッファを作成出来ませんでした\r\n");
      return ZUN_ERROR;
    } else {
      return ZUN_SUCCESS;
    }
  }
}

// FUNCTION: TH07 0x0044c1b0
ZunResult SoundPlayer::ReopenBGM(const char *name)

{
  if (this->backgroundMusic == NULL) {
    return ZUN_ERROR;
  } else {
    i32 fmtIdx = GetFmtIndexByName(name);
    this->backgroundMusic->m_pWaveFile->Reopen(this->bgmFmtData + fmtIdx);
    // STRING: TH07 0x00495f54
    DebugPrint("Streming BGM Reopen %d\r\n", fmtIdx);
    return ZUN_SUCCESS;
  }
}

// FUNCTION: TH07 0x0044c220
ZunResult SoundPlayer::PreloadBGM(i32 idx, const char *path)

{
  HANDLE handle;
  LPBYTE lpBuffer;
  DWORD local_c;
  i32 fmtIdx;

  if (this->bgmPreloadData[idx] != NULL) {
    if (strcmp(path, this->bgmFileNames[idx]) == 0) {
      return ZUN_SUCCESS;
    }
  }
  strcpy(g_SoundPlayer.bgmFileNames[idx], path);
  if ((g_Supervisor.cfg.opts >> 0xd & 1) == 0) {
    return ZUN_SUCCESS;
  } else if (this->manager == NULL) {
    return ZUN_SUCCESS;
  } else {
    if (this->bgmPreloadData[idx] != NULL) {
      free(this->bgmPreloadData[idx]);
      this->bgmPreloadData[idx] = NULL;
    }
    // STRING: TH07 0x00495f38
    DebugPrint("Streming BGM PreLoad %d\r\n", idx);
    handle = CreateFileA(this->bgmArchivePath, GENERIC_READ, 1, NULL, 3,
                         0x8000080, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
      // STRING: TH07 0x00495f14
      DebugPrint("error : bgmfile is not find %s\r\n", this->bgmArchivePath);
      return ZUN_ERROR;
    } else {
      fmtIdx = GetFmtIndexByName(path);
      SetFilePointer(handle, this->bgmFmtData[fmtIdx].startOffset, NULL, 0);
      lpBuffer = (LPBYTE)malloc(this->bgmFmtData[fmtIdx].preloadAllocSize);
      if (lpBuffer == NULL) {
        CloseHandle(handle);
        DebugPrint("error : bgmfile is not find %s\r\n", this->bgmArchivePath);
        return ZUN_ERROR;
      } else {
        ReadFile(handle, lpBuffer, this->bgmFmtData[fmtIdx].preloadAllocSize,
                 &local_c, NULL);
        CloseHandle(handle);
        this->bgmPreloadFmtData[idx] = this->bgmFmtData + fmtIdx;
        this->bgmPreloadData[idx] = lpBuffer;
        this->bgmPreloadDataCursor[idx] = lpBuffer;
        this->bgmPreloadAllocSizes[idx] =
            this->bgmPreloadFmtData[idx]->preloadAllocSize;
        return ZUN_SUCCESS;
      }
    }
  }
}

// FUNCTION: TH07 0x0044c4d0
i32 SoundPlayer::LoadBGM(i32 idx)

{
  u32 bufferSize;
  u32 blockAlign;

  if (this->manager == NULL) {
    return ZUN_ERROR;
  } else if (g_Supervisor.cfg.musicMode == MUSIC_OFF) {
    return ZUN_ERROR;
  } else if (this->directSoundHdl == NULL) {
    return ZUN_ERROR;
  } else if ((g_Supervisor.cfg.opts >> 0xd & 1) == 0) {
    return ReopenBGM(this->bgmFileNames[idx]);
  } else if (this->bgmPreloadData[idx] == NULL) {
    return ZUN_ERROR;
  } else {
    // STRING: TH07 0x00495ef8
    DebugPrint("Streming BGM Load no %d\r\n", idx);
    blockAlign = (u32)(this->bgmPreloadFmtData[idx]->format).nBlockAlign;
    bufferSize = (this->bgmPreloadFmtData[idx]->format).nSamplesPerSec * 4 *
                     blockAlign >>
                 4;
    this->backgroundMusicUpdateEvent = CreateEventA(NULL, 0, 0, NULL);
    this->backgroundMusicThreadHandle = CreateThread(
        NULL, 0, BackgroundMusicPlayerThread, g_Supervisor.hwndGameWindow, 0,
        &this->backgroundMusicThreadId);
    if (FAILED(this->manager->CreateStreamingFromMemory(
            &this->backgroundMusic, this->bgmPreloadDataCursor[idx],
            this->bgmPreloadAllocSizes[idx], this->bgmPreloadFmtData[idx],
            0x10100, GUID_NULL, 0x10, bufferSize - bufferSize % blockAlign,
            this->backgroundMusicUpdateEvent))) {
      DebugPrint(
          "error : ストリーミング用サウンドバッファを作成出来ませんでした\r\n");
      return ZUN_ERROR;
    } else {
      // STRING: TH07 0x00495eec
      DebugPrint("load comp\r\n");
      this->curBgmIdx = idx;
      return ZUN_SUCCESS;
    }
  }
}

// FUNCTION: TH07 0x0044c6b0
void SoundPlayer::StopBGM()

{
  if (this->backgroundMusic != NULL) {
    // STRING: TH07 0x00495ed8
    DebugPrint("Streming BGM stop\r\n");
    this->backgroundMusic->Stop();
    if (this->backgroundMusicThreadHandle != NULL) {
      PostThreadMessageA(this->backgroundMusicThreadId, 0x12, 0, 0);
      // STRING: TH07 0x00495ebc
      DebugPrint("stop m_dwNotifyThreadID\r\n");
      while (WaitForSingleObject(this->backgroundMusicThreadHandle, 0x100) !=
             0) {
        PostThreadMessageA(this->backgroundMusicThreadId, 0x12, 0, 0);
      }
      // STRING: TH07 0x00495eb0
      DebugPrint("stop comp\r\n");
      CloseHandle(this->backgroundMusicThreadHandle);
      CloseHandle(this->backgroundMusicUpdateEvent);
      this->backgroundMusicThreadHandle = NULL;
    }
    SAFE_DELETE(this->backgroundMusic);
  }
}

// FUNCTION: TH07 0x0044c7d0
ZunResult SoundPlayer::InitSoundBuffers()

{
  if (this->manager == NULL) {
    return ZUN_ERROR;
  } else if (this->directSoundHdl == NULL) {
    return ZUN_SUCCESS;
  } else {
    for (i32 i = 0; i < 5; i = i + 1) {
      this->soundQueue[i] = -1;
    }
    for (i32 i = 0; i < 0x1e; i = i + 1) {
      if (LoadSound(i, g_SFXList[i]) != ZUN_SUCCESS) {
        g_GameErrorContext.Log(
        // STRING: TH07 0x00495e78
            "error : Sound ファイルが読み込めない データを確認 %s\r\n",
            g_SFXList[i]);
        return ZUN_ERROR;
      }
    }
    for (i32 i = 0; i < 0x26; i = i + 1) {
      this->directSoundHdl->DuplicateSoundBuffer(
          this->soundBuffers[SOUND_BUFFER_IDX_VOL[i].bufferIdx],
          this->duplicateSoundBuffers + i);
      this->duplicateSoundBuffers[i]->SetCurrentPosition(0);
      this->duplicateSoundBuffers[i]->SetVolume(
          (i32)SOUND_BUFFER_IDX_VOL[i].volume);
    }
    return ZUN_SUCCESS;
  }
}

// FUNCTION: TH07 0x0044c930
void SoundPlayer::PlaySoundByIdx(i32 idx)

{
  i16 sVar1;
  i32 i;

  sVar1 = SOUND_BUFFER_IDX_VOL[idx].field2_0x6;
  for (i = 0; (i < 5 && (-1 < this->soundQueue[i])); i = i + 1) {
    if (this->soundQueue[i] == idx) {
      return;
    }
  }
  if (i < 5) {
    this->soundQueue[i] = idx;
    this->unusedSoundVolRelated[idx] = sVar1;
  }
}

// FUNCTION: TH07 0x0044c9c0
i32 SoundPlayer::ProcessQueues()

{
  LPDIRECTSOUNDBUFFER buffer;
  i32 tmp;
  LPDIRECTSOUNDBUFFER buffer2;
  DWORD DVar2;
  char (*name)[256];
  SoundPlayerCommand *commandCursor;
  i32 i;
  bool loopAgain;

  if (this->manager == NULL) {
    return 0;
  }
  commandCursor = this->commandQueue;
loop:
  loopAgain = false;
  switch (commandCursor->opcode) {
  case AUDIO_PRELOAD:
    if ((g_Supervisor.cfg.opts >> 0xd & 1) == 0) {
      // STRING: TH07 0x00495e60
      DebugPrint("Sound : PreLoad Stage\r\n");
      PreloadBGM(commandCursor->arg1, commandCursor->string);
      loopAgain = true;
    } else {
      DebugPrint("Sound : PreLoad Stage\r\n");
      if (commandCursor->arg2 != 0) {
        commandCursor->arg2 = commandCursor->arg2 + 1;
        goto loop_breakout;
      }
      StopBGM();
      PreloadBGM(commandCursor->arg1, commandCursor->string);
      loopAgain = true;
    }
    break;
  case AUDIO_START:
    if (((g_Supervisor.cfg.opts >> 0xd & 1) == 0) ||
        (commandCursor->arg1 < 0)) {
      if (this->backgroundMusic != NULL) {
        if (commandCursor->arg2 == 0) {
          // STRING: TH07 0x00495de4
          DebugPrint("Sound : Stop Stage\r\n");
          this->backgroundMusic->Stop();
        } else {
          if (commandCursor->arg2 != 1) {
            if (commandCursor->arg2 == 2) {
              // STRING: TH07 0x00495db0
              DebugPrint("Sound : ReOpen Stage\r\n");
              if (commandCursor->arg1 < 0) {
                name = &commandCursor->string;
              } else {
                name = this->bgmFileNames + commandCursor->arg1;
              }
              tmp = GetFmtIndexByName(*name);
              this->backgroundMusic->m_pWaveFile->Reopen(this->bgmFmtData +
                                                         tmp);
            } else {
              if (commandCursor->arg2 == 3) {
                // STRING: TH07 0x00495e14
                DebugPrint("Sound : Fill Buffer Stage\r\n");
                buffer2 = this->backgroundMusic->GetBuffer(0);
                this->backgroundMusic->Reset();
                commandCursor->arg1 = (u32)((this->backgroundMusic->m_pWaveFile)
                                                ->m_pzwf->totalLength != 0);
                tmp = this->backgroundMusic->FillBufferWithSound(
                    buffer2, commandCursor->arg1);
                goto joined_r0x0044cd6a;
              }
              if (commandCursor->arg2 == 4) {
                // STRING: TH07 0x00495dfc
                DebugPrint("Sound : Play Stage\r\n");
                this->backgroundMusic->Play(0, 1);
              } else if (6 < commandCursor->arg2)
                break;
            }
            goto LAB_0044cdab;
          }
          if (this->backgroundMusic->m_bIsLocked != 0)
            goto loop_breakout;
          // STRING: TH07 0x00495dc8
          DebugPrint("Sound : Recreate Stage\r\n");
          this->backgroundMusic->InitSoundBuffers();
        }
        goto LAB_0044cdab;
      }
    } else {
      if (commandCursor->arg2 != 0) {
        if (commandCursor->arg2 == 2) {
          // STRING: TH07 0x00495e30
          DebugPrint("Sound : Reset Stage\r\n");
          if (this->backgroundMusic != NULL) {
            tmp = this->backgroundMusic->Reset();
          joined_r0x0044cd6a:
            if (tmp < 0)
              break;
          }
        } else {
          if (commandCursor->arg2 == 5) {
            DebugPrint("Sound : Fill Buffer Stage\r\n");
            buffer = this->backgroundMusic->GetBuffer(0);
            commandCursor->arg1 = (u32)((this->backgroundMusic->m_pWaveFile)
                                            ->m_pzwf->totalLength != 0);
            tmp = this->backgroundMusic->FillBufferWithSound(
                buffer, commandCursor->arg1);
            goto joined_r0x0044cd6a;
          }
          if (commandCursor->arg2 == 7) {
            DebugPrint("Sound : Play Stage\r\n");
            this->backgroundMusic->Play(0, 1);
          } else if (0x13 < commandCursor->arg2)
            break;
        }
      LAB_0044cdab:
        commandCursor->arg2 = commandCursor->arg2 + 1;
        goto loop_breakout;
      }
      // STRING: TH07 0x00495e48
      DebugPrint("Sound : Load Stage\r\n");
      tmp = LoadBGM(commandCursor->arg1);
      if (tmp == 0)
        goto LAB_0044cdab;
    }
    break;
  case AUDIO_STOP:
    if (this->backgroundMusic != NULL) {
      if (commandCursor->arg2 == 0) {
        DebugPrint("Sound : Stop Stage\r\n");
        this->backgroundMusic->Stop();
      } else if (commandCursor->arg2 == 1)
        break;
      commandCursor->arg2 = commandCursor->arg2 + 1;
      goto loop_breakout;
    }
    break;
  case AUDIO_SHUTDOWN:
    if (this->backgroundMusic != NULL) {
      if (commandCursor->arg2 == 0) {
        DebugPrint("Sound : Stop Stage\r\n");
        this->backgroundMusic->Stop();
      } else if (commandCursor->arg2 == 1) {
        // STRING: TH07 0x00495d94
        DebugPrint("Sound : Thread Stop Stage\r\n");
        if (this->backgroundMusicThreadHandle == NULL)
          break;
        PostThreadMessageA(this->backgroundMusicThreadId, 0x12, 0, 0);
      } else if (commandCursor->arg2 == 2) {
        DVar2 = WaitForSingleObject(this->backgroundMusicThreadHandle, 0x100);
        if (DVar2 == 0) {
          this->backgroundMusicThreadHandle = NULL;
        } else {
          // STRING: TH07 0x00495d70
          DebugPrint("Sound : Thread Stop Wait Stage\r\n");
          PostThreadMessageA(this->backgroundMusicThreadId, 0x12, 0, 0);
          commandCursor->arg2 = commandCursor->arg2 - 1;
        }
      } else if (commandCursor->arg2 == 3) {
        // STRING: TH07 0x00495d50
        DebugPrint("Sound : Handle Close Stage\r\n");
        CloseHandle(this->backgroundMusicThreadHandle);
        CloseHandle(this->backgroundMusicUpdateEvent);
        this->backgroundMusicThreadHandle = NULL;
        SAFE_DELETE(this->backgroundMusic);
      } else if (commandCursor->arg2 == 10)
        break;
      commandCursor->arg2 = commandCursor->arg2 + 1;
      goto loop_breakout;
    }
    break;
  case AUDIO_FADEOUT:
    // STRING: TH07 0x00495d34
    DebugPrint("Sound : Fade Out Stage %d\r\n", commandCursor->arg1);
    tmp = commandCursor->arg1;
    if (g_SoundPlayer.backgroundMusic != NULL) {
      g_SoundPlayer.backgroundMusic->m_dwIsFadingOut = 1;
      g_SoundPlayer.backgroundMusic->m_iCurFadeoutProgress =
          ((f32)tmp * 60.0f);
      g_SoundPlayer.backgroundMusic->m_iTotalFadeout =
          g_SoundPlayer.backgroundMusic->m_iCurFadeoutProgress;
    }
    break;
  case AUDIO_PAUSE:
    if (g_Supervisor.cfg.musicMode == MUSIC_WAV) {
      if (this->backgroundMusic->m_bIsLocked != 0) {
        // STRING: TH07 0x00495d2c
        DebugPrint("locked\n");
        goto loop_breakout;
      }
      if (this->backgroundMusic != NULL) {
        this->backgroundMusic->Pause();
      }
    }
    break;
  case AUDIO_UNPAUSE:
    if (g_Supervisor.cfg.musicMode == MUSIC_WAV) {
      if (this->backgroundMusic->m_bIsLocked != 0)
        goto loop_breakout;
      if (this->backgroundMusic != NULL) {
        this->backgroundMusic->Unpause();
      }
    }
    break;
  default:
    goto loop_breakout;
  }
  i = 0;
  while (i < 0x1f && commandCursor->opcode != 0) {
    commandCursor[0] = commandCursor[1];
    i = i + 1;
    commandCursor = commandCursor + 1;
  }
  if (!loopAgain) {
  loop_breakout:
    if (g_Supervisor.cfg.playSounds == 0) {
      return this->commandQueue[0].opcode;
    } else {
      for (i = 0; (i < 5 && (-1 < this->soundQueue[i])); i = i + 1) {
        tmp = this->soundQueue[i];
        this->soundQueue[i] = -1;
        if (this->duplicateSoundBuffers[tmp] != NULL) {
          this->duplicateSoundBuffers[tmp]->Stop();
          this->duplicateSoundBuffers[tmp]->SetCurrentPosition(0);
          this->duplicateSoundBuffers[tmp]->Play(0, 0, 0);
        }
      }
      return this->commandQueue[0].opcode;
    }
  }
  goto loop;
}

// FUNCTION: TH07 0x0044d200
DWORD __stdcall SoundPlayer::BackgroundMusicPlayerThread(
    LPVOID lpThreadParameter)

{
  bool bVar1;
  DWORD DVar2;
  tagMSG local_20;

  bVar1 = false;
  while (!bVar1) {
    DVar2 = MsgWaitForMultipleObjects(
        1, &g_SoundPlayer.backgroundMusicUpdateEvent, 0, 0xffffffff, 0xbf);
    if (g_SoundPlayer.backgroundMusic == NULL) {
      bVar1 = true;
    }
    if (DVar2 == 0) {
      if ((g_SoundPlayer.backgroundMusic != NULL) &&
          (g_SoundPlayer.backgroundMusic->m_bIsPlaying != 0)) {
        (g_SoundPlayer.backgroundMusic)->m_bIsLocked = 1;
        g_SoundPlayer.backgroundMusic->HandleWaveStreamNotification(1);
        (g_SoundPlayer.backgroundMusic)->m_bIsLocked = 0;
      }
    } else if (DVar2 == 1) {
      while (PeekMessageA(&local_20, NULL, 0, 0, 1) != 0) {
        if (local_20.message == 0x12) {
          bVar1 = true;
        }
      }
    }
  }
  // STRING: TH07 0x00495cf4
  DebugPrint("atention : ストリーミング用スレッドは終了しました。\r\n");
  return 0;
}

// FUNCTION: TH07 0x0044d2f0
void SoundPlayer::PushCommand(AudioOpcode opcode, i32 arg1, const char *arg2)

{
  i32 queueIdx = 0;
  while (true) {
    if (0x1e < queueIdx)
      goto stop;
    if (this->commandQueue[queueIdx].opcode == 0)
      break;
    queueIdx = queueIdx + 1;
  }
  this->commandQueue[queueIdx].opcode = opcode;
  this->commandQueue[queueIdx].arg1 = arg1;
  strcpy(this->commandQueue[queueIdx].string, arg2);
  this->commandQueue[queueIdx].arg2 = 0;
stop:
  // STRING: TH07 0x00495ce0
  DebugPrint("Sound Que Add %d\r\n", opcode);
}
