#include "SoundPlayer.hpp"

#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "Supervisor.hpp"
#include "dsutil.hpp"
#include "dxutil.hpp"

// GLOBAL: TH07 0x004ba0d8
SoundPlayer g_SoundPlayer;

// GLOBAL: TH07 0x0049ea88
SoundBufferIdxVolume SOUND_BUFFER_IDX_VOL[38] = {
    {0, -2000, 0}, {0, -2500, 0}, {1, -1200, 5}, {1, -1500, 5}, {2, -1000, 100}, {3, -400, 100}, {4, -400, 100}, {5, -1500, 50}, {6, -1700, 50}, {7, -1900, 50}, {8, -1000, 100}, {9, -1000, 100}, {10, -1700, 10}, {11, -1200, 10}, {12, -900, 100}, {5, -1500, 50}, {13, -900, 50}, {14, -900, 50}, {15, -900, 100}, {16, -200, 100}, {17, -1400, 0}, {18, -1300, 0}, {5, -100, 20}, {6, -1800, 20}, {7, -1800, 20}, {19, -800, 50}, {20, -1000, 50}, {21, -1300, 50}, {22, -300, 140}, {23, -900, 100}, {24, -900, 20}, {25, -500, 90}, {26, -300, 100}, {27, -300, 100}, {24, -300, 20}, {19, 0, 50}, {28, -300, 100}, {29, -300, 100}};

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
    "data/wav/se_pause.wav",
};

// FUNCTION: TH07 0x0044b510
SoundPlayer::SoundPlayer()
{
    memset(this, 0, sizeof(SoundPlayer));
    for (i32 i = 0; i < 0x80; ++i)
    {
        this->unusedSoundVolRelated[i] = -1;
    }
}

#pragma var_order(bufdesc, audioBuffer2Start, audioBuffer2Len, audioBuffer1Len, audioBuffer1Start, wavFormat)
// FUNCTION: TH07 0x0044b560
ZunResult SoundPlayer::InitializeDSound(HWND gameWindow)
{
    WAVEFORMATEX wavFormat;
    LPVOID audioBuffer1Start;
    DWORD audioBuffer1Len;
    DWORD audioBuffer2Len;
    LPVOID audioBuffer2Start;
    DSBUFFERDESC bufdesc;

    memset(this, 0, sizeof(SoundPlayer));
    for (i32 i = 0; i < 0x80; i++)
    {
        this->unusedSoundVolRelated[i] = -1;
    }
    this->manager = new CSoundManager;
    if (FAILED(this->manager->Initialize(gameWindow, 2, 2, 0xac44, 0x10)))
    {
        // STRING: TH07 0x0049604c
        g_GameErrorContext.Log("DirectSound オブジェクトの初期化が失敗したよ\r\n");
        SAFE_DELETE(this->manager);
        return ZUN_ERROR;
    }

    this->directSoundHdl = this->manager->GetDirectSound();
    this->backgroundMusicThreadHandle = NULL;
    memset(&bufdesc, 0, sizeof(DSBUFFERDESC));
    bufdesc.dwSize = 0x24;
    bufdesc.dwFlags = 0x8008;
    bufdesc.dwBufferBytes = 0x8000;
    memset(&wavFormat, 0, sizeof(WAVEFORMATEX));
    wavFormat.cbSize = 0;
    wavFormat.wFormatTag = 1;
    wavFormat.nChannels = 2;
    wavFormat.nSamplesPerSec = 0xac44;
    wavFormat.nAvgBytesPerSec = 0x2b110;
    wavFormat.nBlockAlign = 4;
    wavFormat.wBitsPerSample = 0x10;
    bufdesc.lpwfxFormat = &wavFormat;
    if (FAILED(this->directSoundHdl->CreateSoundBuffer(
            &bufdesc, &this->initSoundBuffer, NULL)))
    {
        return ZUN_ERROR;
    }

    if (FAILED(this->initSoundBuffer->Lock(
            0, 0x8000, &audioBuffer1Start, &audioBuffer1Len,
            &audioBuffer2Start, &audioBuffer2Len, 0)))
    {
        return ZUN_ERROR;
    }

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

// FUNCTION: TH07 0x0044b830
ZunResult SoundPlayer::Release()
{
    i32 i;

    if (this->manager == NULL)
    {
        return ZUN_SUCCESS;
    }

    for (i = 0; i < 0x80; i++)
    {
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
    for (i = 0; i < 0x10; i++)
    {
        SAFE_FREE(this->bgmPreloadData[i]);
    }
    if (this->bgmFmtData != NULL)
    {
        ZunMemory::Free(this->bgmFmtData);
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0044ba80
WAVEFORMATEX *SoundPlayer::GetWavFormatData(u8 *soundData,
                                            const char *formatString,
                                            i32 *formatSize,
                                            u32 fileSizeExcludingFormat)
{
    while (fileSizeExcludingFormat > 0)
    {
        *formatSize = *(i32 *)(soundData + 4);
        if (strncmp((char *)soundData, formatString, 4) == 0)
        {
            return (WAVEFORMATEX *)(soundData + 8);
        }
        fileSizeExcludingFormat = fileSizeExcludingFormat - (*formatSize + 8);
        soundData += +*formatSize + 8;
    }
    return NULL;
}

#pragma var_order(local_8, local_c, local_8c)
// FUNCTION: TH07 0x0044baf0
i32 SoundPlayer::GetFmtIndexByName(const char *param_1)
{
    char local_8c[128];
    i32 local_c;
    const char *local_8;

    local_c = 0;
    local_8 = strrchr(param_1, '/');
    if (local_8 == NULL)
    {
        local_8 = strrchr(param_1, '\\');
    }
    if (local_8 == NULL)
    {
        strcpy(local_8c, param_1);
    }
    else
    {
        strcpy(local_8c, local_8 + 1);
    }
    while (this->bgmFmtData[local_c].name[0] != '\0')
    {
        if (strcmp(this->bgmFmtData[local_c].name, local_8c) == 0)
        {
            break;
        }
        local_c = local_c + 1;
    }
    if (this->bgmFmtData[local_c].name[0] == '\0')
    {
        local_c = 0;
    }
    return local_c;
}

#pragma var_order(cursor, dsBuffer, wavDataPtr, formatSize, audioPtr2, audioSize2, \
                  audioSize1, audioPtr1, soundFileDat, wavData, fileSize)
// FUNCTION: TH07 0x0044bd00
ZunResult SoundPlayer::LoadSound(i32 idx, const char *path)
{
    u8 *soundFileDat;
    WAVEFORMATEX wavData;
    WAVEFORMATEX *audioPtr1;
    DWORD audioSize1;
    DWORD audioSize2;
    WAVEFORMATEX *audioPtr2;
    i32 formatSize;
    WAVEFORMATEX *wavDataPtr;
    DSBUFFERDESC dsBuffer;

    if (this->manager == NULL)
    {
        return ZUN_SUCCESS;
    }

    SAFE_RELEASE(this->soundBuffers[idx]);
    soundFileDat = FileSystem::OpenFile(path, 0);
    u8 *cursor = soundFileDat;
    if (cursor == NULL)
    {
        return ZUN_ERROR;
    }

    // STRING: TH07 0x0049601c
    if (strncmp((char *)cursor, "RIFF", 4) != 0)
    {
        // STRING: TH07 0x00496000
        g_GameErrorContext.Log("Wav ファイルじゃない %s\r\n", path);
        free(soundFileDat);
        return ZUN_ERROR;
    }

    cursor += 4;
    i32 fileSize = *(i32 *)cursor;
    cursor += 4;

    // STRING: TH07 0x00495ff8
    if (strncmp((char *)cursor, "WAVE", 4) != 0)
    {
        g_GameErrorContext.Log("Wav ファイルじゃない? %s\r\n", path);
        free(soundFileDat);
        return ZUN_ERROR;
    }

    cursor += 4;
    // STRING: TH07 0x00495fd4
    wavDataPtr = GetWavFormatData(cursor, "fmt ",
                                  &formatSize, fileSize - 12);
    if (wavDataPtr == NULL)
    {
        // STRING: TH07 0x00495fdc
        g_GameErrorContext.Log("Wav ファイルじゃない? %s\r\n", path);
        free(soundFileDat);
        return ZUN_ERROR;
    }

    wavData = *wavDataPtr;
    // STRING: TH07 0x00495fcc
    wavDataPtr = GetWavFormatData(cursor, "data",
                                  &formatSize, fileSize - 12);
    if (wavDataPtr == NULL)
    {
        g_GameErrorContext.Log("Wav ファイルじゃない? %s\r\n", path);
        free(soundFileDat);
        return ZUN_ERROR;
    }

    memset(&dsBuffer, 0, sizeof(DSBUFFERDESC));
    dsBuffer.dwSize = 0x24;
    dsBuffer.dwFlags = 0x8088;
    dsBuffer.dwBufferBytes = formatSize;
    dsBuffer.lpwfxFormat = &wavData;
    if (FAILED(this->directSoundHdl->CreateSoundBuffer(
            &dsBuffer, &this->soundBuffers[idx], NULL)))
    {
        free(soundFileDat);
        return ZUN_ERROR;
    }

    if (FAILED(this->soundBuffers[idx]->Lock(
            0, formatSize, (LPVOID *)&audioPtr1, &audioSize1,
            (LPVOID *)&audioPtr2, &audioSize2, 0)))
    {
        free(soundFileDat);
        return ZUN_ERROR;
    }

    memcpy(audioPtr1, wavDataPtr, audioSize1);
    if (audioSize2 != 0)
    {
        memcpy(audioPtr2, (u8 *)wavDataPtr + audioSize1, audioSize2);
    }
    this->soundBuffers[idx]->Unlock(audioPtr1, audioSize1, audioPtr2,
                                    audioSize2);
    free(soundFileDat);
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0044bff0
ZunResult SoundPlayer::LoadFmt(const char *param_1)
{
    this->bgmFmtData = (ThBgmFormat *)FileSystem::OpenFile(param_1, 0);
    return this->bgmFmtData != NULL ? ZUN_SUCCESS : ZUN_ERROR;
}

#pragma var_order(notifySize, pzwf, hr, samplesPerSec, blockAlign)
// FUNCTION: TH07 0x0044c020
ZunResult SoundPlayer::StartBGM(const char *path)
{
    DWORD blockAlign;
    DWORD samplesPerSec;
    HRESULT hr;
    ThBgmFormat *pzwf;
    DWORD notifySize;

    strcpy(this->bgmArchivePath, path);
    if (this->manager == NULL)
    {
        return ZUN_ERROR;
    }

    if (this->directSoundHdl == NULL)
    {
        return ZUN_ERROR;
    }

    // STRING: TH07 0x00495fb4
    DebugPrint("Streming BGM Start\r\n");
    StopBGM();
    pzwf = this->bgmFmtData;
    blockAlign = pzwf->format.nBlockAlign;
    samplesPerSec = pzwf->format.nSamplesPerSec;
    notifySize = samplesPerSec * 4 * blockAlign >> 4;
    notifySize -= notifySize % blockAlign;
    this->backgroundMusicUpdateEvent = CreateEventA(NULL, 0, 0, NULL);
    this->backgroundMusicThreadHandle = CreateThread(
        NULL, 0, BackgroundMusicPlayerThread, g_Supervisor.hwndGameWindow, 0,
        &this->backgroundMusicThreadId);
    if (FAILED(hr = this->manager->CreateStreaming(
                   &this->backgroundMusic, path, 0x10100, GUID_NULL, 0x10,
                   notifySize, this->backgroundMusicUpdateEvent, pzwf)))
    {
        // STRING: TH07 0x00495f70
        DebugPrint("error : ストリーミング用サウンドバッファを作成出来ませんでした\r\n");
        return ZUN_ERROR;
    }

    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0044c1b0
ZunResult SoundPlayer::ReopenBGM(const char *name)
{
    if (this->backgroundMusic == NULL)
    {
        return ZUN_ERROR;
    }

    i32 fmtIdx = GetFmtIndexByName(name);

    this->backgroundMusic->GetWaveFile()->Reopen(&this->bgmFmtData[fmtIdx]);
    // STRING: TH07 0x00495f54
    DebugPrint("Streming BGM Reopen %d\r\n", fmtIdx);
    return ZUN_SUCCESS;
}

#pragma var_order(fmtIdx, bytesRead, handle, lpBuffer)
// FUNCTION: TH07 0x0044c220
ZunResult SoundPlayer::PreloadBGM(i32 idx, const char *path)
{
    LPBYTE lpBuffer;
    HANDLE handle;
    DWORD bytesRead;
    i32 fmtIdx;

    if (this->bgmPreloadData[idx] != NULL)
    {
        if (strcmp(path, this->bgmFileNames[idx]) == 0)
        {
            return ZUN_SUCCESS;
        }
    }
    strcpy(g_SoundPlayer.bgmFileNames[idx], path);
    if ((g_Supervisor.cfg.opts >> 0xd & 1) == 0)
    {
        return ZUN_SUCCESS;
    }

    if (this->manager == NULL)
    {
        return ZUN_SUCCESS;
    }

    SAFE_FREE(this->bgmPreloadData[idx]);
    // STRING: TH07 0x00495f38
    DebugPrint("Streming BGM PreLoad %d\r\n", idx);
    handle = CreateFileA(this->bgmArchivePath, GENERIC_READ, 1, NULL, 3,
                         0x8000080, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        // STRING: TH07 0x00495f14
        DebugPrint("error : bgmfile is not find %s\r\n", this->bgmArchivePath);
        return ZUN_ERROR;
    }

    fmtIdx = GetFmtIndexByName(path);
    SetFilePointer(handle, this->bgmFmtData[fmtIdx].startOffset, NULL, 0);
    lpBuffer = (LPBYTE)ZunMemory::Alloc(this->bgmFmtData[fmtIdx].preloadAllocSize);
    if (lpBuffer == NULL)
    {
        CloseHandle(handle);
        DebugPrint("error : bgmfile is not find %s\r\n", this->bgmArchivePath);
        return ZUN_ERROR;
    }

    ReadFile(handle, lpBuffer, this->bgmFmtData[fmtIdx].preloadAllocSize,
             &bytesRead, NULL);
    CloseHandle(handle);
    this->bgmPreloadFmtData[idx] = this->bgmFmtData + fmtIdx;
    this->bgmPreloadData[idx] = lpBuffer;
    this->bgmPreloadDataCursor[idx] = lpBuffer;
    this->bgmPreloadAllocSizes[idx] =
        this->bgmPreloadFmtData[idx]->preloadAllocSize;
    return ZUN_SUCCESS;
}

#pragma var_order(notifySize, hr, samplesPerSec, blockAlign)
// FUNCTION: TH07 0x0044c4d0
ZunResult SoundPlayer::LoadBGM(i32 idx)
{
    DWORD blockAlign;
    DWORD samplesPerSec;
    HRESULT hr;
    DWORD notifySize;

    if (this->manager == NULL)
    {
        return ZUN_ERROR;
    }

    if (g_Supervisor.cfg.musicMode == MUSIC_OFF)
    {
        return ZUN_ERROR;
    }

    if (this->directSoundHdl == NULL)
    {
        return ZUN_ERROR;
    }

    if ((g_Supervisor.cfg.opts >> 0xd & 1) == 0)
    {
        return ReopenBGM(this->bgmFileNames[idx]);
    }

    if (this->bgmPreloadData[idx] == NULL)
    {
        return ZUN_ERROR;
    }

    // STRING: TH07 0x00495ef8
    DebugPrint("Streming BGM Load no %d\r\n", idx);
    blockAlign = (this->bgmPreloadFmtData[idx]->format).nBlockAlign;
    samplesPerSec = (this->bgmPreloadFmtData[idx]->format).nSamplesPerSec;
    notifySize = samplesPerSec * 4 * blockAlign >> 4;
    notifySize -= notifySize % blockAlign;
    this->backgroundMusicUpdateEvent = CreateEventA(NULL, 0, 0, NULL);
    this->backgroundMusicThreadHandle = CreateThread(
        NULL, 0, BackgroundMusicPlayerThread, g_Supervisor.hwndGameWindow, 0,
        &this->backgroundMusicThreadId);
    if (FAILED(hr = this->manager->CreateStreamingFromMemory(
                   &this->backgroundMusic, this->bgmPreloadDataCursor[idx],
                   this->bgmPreloadAllocSizes[idx], this->bgmPreloadFmtData[idx],
                   0x10100, GUID_NULL, 0x10, notifySize,
                   this->backgroundMusicUpdateEvent)))
    {
        DebugPrint(
            "error : ストリーミング用サウンドバッファを作成出来ませんでした\r\n");
        return ZUN_ERROR;
    }

    // STRING: TH07 0x00495eec
    DebugPrint("load comp\r\n");
    this->curBgmIdx = idx;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0044c6b0
void SoundPlayer::StopBGM()
{
    if (this->backgroundMusic != NULL)
    {
        // STRING: TH07 0x00495ed8
        DebugPrint("Streming BGM stop\r\n");
        this->backgroundMusic->Stop();
        if (this->backgroundMusicThreadHandle != NULL)
        {
            PostThreadMessageA(this->backgroundMusicThreadId, 0x12, 0, 0);
            // STRING: TH07 0x00495ebc
            DebugPrint("stop m_dwNotifyThreadID\r\n");
            while (WaitForSingleObject(this->backgroundMusicThreadHandle, 0x100) !=
                   0)
            {
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
    i32 i;

    if (this->manager == NULL)
    {
        return ZUN_ERROR;
    }
    if (this->directSoundHdl == NULL)
    {
        return ZUN_SUCCESS;
    }

    for (i = 0; i < 5; i++)
    {
        this->soundQueue[i] = -1;
    }
    for (i = 0; i < 30; i++)
    {
        if (LoadSound(i, g_SFXList[i]) != ZUN_SUCCESS)
        {
            g_GameErrorContext.Log(
                // STRING: TH07 0x00495e78
                "error : Sound ファイルが読み込めない データを確認 %s\r\n",
                g_SFXList[i]);
            return ZUN_ERROR;
        }
    }
    for (i = 0; (u32)i < 38; i++)
    {
        this->directSoundHdl->DuplicateSoundBuffer(
            this->soundBuffers[SOUND_BUFFER_IDX_VOL[i].bufferIdx],
            this->duplicateSoundBuffers + i);
        this->duplicateSoundBuffers[i]->SetCurrentPosition(0);
        this->duplicateSoundBuffers[i]->SetVolume(
            (i32)SOUND_BUFFER_IDX_VOL[i].volume);
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0044c930
void SoundPlayer::PlaySoundByIdx(i32 idx, u32 param_2)
{
    i32 iVar1;
    i32 i;

    iVar1 = SOUND_BUFFER_IDX_VOL[idx].field2_0x6;
    for (i = 0; i < 5; i++)
    {
        if (this->soundQueue[i] < 0)
        {
            break;
        }

        if (this->soundQueue[i] == idx)
        {
            return;
        }
    }
    if (i >= 5)
    {
        return;
    }

    this->soundQueue[i] = idx;
    this->unusedSoundVolRelated[idx] = iVar1;
}

#pragma var_order(loopAgain, i, commandCursor, curSound, buffer, name, fmtIdx, buffer2)
// FUNCTION: TH07 0x0044c9c0
i32 SoundPlayer::ProcessQueues()
{
    LPDIRECTSOUNDBUFFER buffer2;
    i32 fmtIdx;
    char (*name)[256];
    LPDIRECTSOUNDBUFFER buffer;
    i32 curSound;
    SoundPlayerCommand *commandCursor;
    i32 i;
    u32 loopAgain;

    if (this->manager == NULL)
    {
        return 0;
    }

    commandCursor = this->commandQueue;
loop:
    loopAgain = false;
    switch (commandCursor->opcode)
    {
    case AUDIO_PRELOAD:
        if ((g_Supervisor.cfg.opts >> 0xd & 1) != 0)
        {
            // STRING: TH07 0x00495e60
            DebugPrint("Sound : PreLoad Stage\r\n");
            if (commandCursor->arg2 == 0)
            {
                StopBGM();
                PreloadBGM(commandCursor->arg1, commandCursor->string);
                loopAgain = true;
                break;
            }
        }
        else
        {
            DebugPrint("Sound : PreLoad Stage\r\n");
            PreloadBGM(commandCursor->arg1, commandCursor->string);
            loopAgain = true;
            break;
        }
        commandCursor->arg2++;
        goto loop_breakout;
    case AUDIO_START:
        if ((g_Supervisor.cfg.opts >> 0xd & 1) != 0 && commandCursor->arg1 >= 0)
        {
            if (commandCursor->arg2 == 0)
            {
                // STRING: TH07 0x00495e48
                DebugPrint("Sound : Load Stage\r\n");
                if (LoadBGM(commandCursor->arg1) != ZUN_SUCCESS)
                {
                    break;
                }
            }
            else if (commandCursor->arg2 == 2)
            {
                // STRING: TH07 0x00495e30
                DebugPrint("Sound : Reset Stage\r\n");
                if (this->backgroundMusic != NULL)
                {
                    if (FAILED(this->backgroundMusic->Reset()))
                    {
                        break;
                    }
                }
            }
            else if (commandCursor->arg2 == 5)
            {
                DebugPrint("Sound : Fill Buffer Stage\r\n");
                buffer = this->backgroundMusic->GetBuffer(0);
                commandCursor->arg1 = this->backgroundMusic->GetWaveFile()
                                          ->GetFormat()
                                          ->totalLength != 0;
                if (FAILED(this->backgroundMusic->FillBufferWithSound(
                        buffer, commandCursor->arg1)))
                {
                    break;
                }
            }
            else if (commandCursor->arg2 == 7)
            {
                DebugPrint("Sound : Play Stage\r\n");
                this->backgroundMusic->Play(0, 1);
            }
            else if (commandCursor->arg2 >= 20)
            {
                break;
            }
        }
        else if (this->backgroundMusic == NULL)
        {
            break;
        }
        else if (commandCursor->arg2 == 0)
        {
            // STRING: TH07 0x00495de4
            DebugPrint("Sound : Stop Stage\r\n");
            this->backgroundMusic->Stop();
        }
        else if (commandCursor->arg2 == 1)
        {
            if (this->backgroundMusic->m_bIsLocked != 0)
            {
                goto loop_breakout;
            }
            // STRING: TH07 0x00495dc8
            DebugPrint("Sound : Recreate Stage\r\n");
            this->backgroundMusic->InitSoundBuffers();
        }
        else if (commandCursor->arg2 == 2)
        {
            // STRING: TH07 0x00495db0
            DebugPrint("Sound : ReOpen Stage\r\n");
            name = commandCursor->arg1 >= 0
                       ? &this->bgmFileNames[commandCursor->arg1]
                       : &commandCursor->string;
            fmtIdx = GetFmtIndexByName(*name);
            this->backgroundMusic->GetWaveFile()->Reopen(&this->bgmFmtData[fmtIdx]);
        }
        else if (commandCursor->arg2 == 3)
        {
            // STRING: TH07 0x00495e14
            DebugPrint("Sound : Fill Buffer Stage\r\n");
            buffer2 = this->backgroundMusic->GetBuffer(0);
            this->backgroundMusic->Reset();
            commandCursor->arg1 = this->backgroundMusic->GetWaveFile()
                                      ->GetFormat()
                                      ->totalLength != 0;
            if (FAILED(this->backgroundMusic->FillBufferWithSound(
                    buffer2, commandCursor->arg1)))
            {
                break;
            }
        }
        else if (commandCursor->arg2 == 4)
        {
            // STRING: TH07 0x00495dfc
            DebugPrint("Sound : Play Stage\r\n");
            this->backgroundMusic->Play(0, 1);
        }
        else if (commandCursor->arg2 >= 7)
        {
            break;
        }
        commandCursor->arg2++;
        goto loop_breakout;
    case AUDIO_SHUTDOWN:
        if (this->backgroundMusic == NULL)
        {
            break;
        }

        if (commandCursor->arg2 == 0)
        {
            DebugPrint("Sound : Stop Stage\r\n");
            this->backgroundMusic->Stop();
        }
        else if (commandCursor->arg2 == 1)
        {
            // STRING: TH07 0x00495d94
            DebugPrint("Sound : Thread Stop Stage\r\n");
            if (this->backgroundMusicThreadHandle == NULL)
            {
                break;
            }
            PostThreadMessageA(this->backgroundMusicThreadId, 0x12, 0, 0);
        }
        else if (commandCursor->arg2 == 2)
        {
            if (WaitForSingleObject(this->backgroundMusicThreadHandle, 0x100) != 0)
            {
                // STRING: TH07 0x00495d70
                DebugPrint("Sound : Thread Stop Wait Stage\r\n");
                PostThreadMessageA(this->backgroundMusicThreadId, 0x12, 0, 0);
                commandCursor->arg2 = commandCursor->arg2 - 1;
            }
            else
            {
                this->backgroundMusicThreadHandle = NULL;
            }
        }
        else if (commandCursor->arg2 == 3)
        {
            // STRING: TH07 0x00495d50
            DebugPrint("Sound : Handle Close Stage\r\n");
            CloseHandle(this->backgroundMusicThreadHandle);
            CloseHandle(this->backgroundMusicUpdateEvent);
            this->backgroundMusicThreadHandle = NULL;
            SAFE_DELETE(this->backgroundMusic);
        }
        else if (commandCursor->arg2 == 10)
        {
            break;
        }
        commandCursor->arg2++;
        goto loop_breakout;
    case AUDIO_STOP:
        if (this->backgroundMusic == NULL)
        {
            break;
        }

        if (commandCursor->arg2 == 0)
        {
            DebugPrint("Sound : Stop Stage\r\n");
            this->backgroundMusic->Stop();
        }
        else if (commandCursor->arg2 == 1)
        {
            break;
        }
        commandCursor->arg2++;
        goto loop_breakout;
    case AUDIO_FADEOUT: {
        // STRING: TH07 0x00495d34
        DebugPrint("Sound : Fade Out Stage %d\r\n", commandCursor->arg1);
        g_SoundPlayer.FadeOut(commandCursor->arg1);
        break;
    }
    case AUDIO_PAUSE:
        if (g_Supervisor.cfg.musicMode == MUSIC_WAV)
        {
            if (this->backgroundMusic->m_bIsLocked != 0)
            {
                // STRING: TH07 0x00495d2c
                DebugPrint("locked\n");
                goto loop_breakout;
            }
            if (this->backgroundMusic != NULL)
            {
                this->backgroundMusic->Pause();
            }
        }
        break;
    case AUDIO_UNPAUSE:
        if (g_Supervisor.cfg.musicMode == MUSIC_WAV)
        {
            if (this->backgroundMusic->m_bIsLocked != 0)
            {
                goto loop_breakout;
            }
            if (this->backgroundMusic != NULL)
            {
                this->backgroundMusic->Unpause();
            }
        }
        break;
    default:
        goto loop_breakout;
    }
    for (i = 0; i < 0x1f; i++, commandCursor++)
    {
        if (commandCursor->opcode == 0)
        {
            break;
        }
        commandCursor[0] = commandCursor[1];
    }

    if (loopAgain)
    {
        goto loop;
    }

loop_breakout:
    if (g_Supervisor.cfg.playSounds == 0)
    {
        return this->commandQueue[0].opcode;
    }
    else
    {
        for (i = 0; i < 5; i++)
        {
            if (this->soundQueue[i] < 0)
            {
                break;
            }

            curSound = this->soundQueue[i];
            this->soundQueue[i] = -1;
            if (this->duplicateSoundBuffers[curSound] == NULL)
            {
                continue;
            }

            this->duplicateSoundBuffers[curSound]->Stop();
            this->duplicateSoundBuffers[curSound]->SetCurrentPosition(0);
            this->duplicateSoundBuffers[curSound]->Play(0, 0, 0);
        }
        return this->commandQueue[0].opcode;
    }
}

#pragma var_order(msg, looped, lpThreadParameterCopy, waitObj, hr, stopped)
// FUNCTION: TH07 0x0044d200
DWORD __stdcall SoundPlayer::BackgroundMusicPlayerThread(LPVOID lpThreadParameter)
{
    u32 stopped;
    HRESULT hr;
    DWORD waitObj;
    LPVOID lpThreadParameterCopy;
    u32 looped;
    MSG msg;

    lpThreadParameterCopy = lpThreadParameter;
    stopped = false;
    looped = true;
    while (!stopped)
    {
        waitObj = MsgWaitForMultipleObjects(
            1, &g_SoundPlayer.backgroundMusicUpdateEvent, 0, 0xffffffff, 0xbf);
        if (g_SoundPlayer.backgroundMusic == NULL)
        {
            stopped = true;
        }

        switch (waitObj)
        {
        case 0:
            if (g_SoundPlayer.backgroundMusic != NULL &&
                g_SoundPlayer.backgroundMusic->m_bIsPlaying != 0)
            {
                g_SoundPlayer.backgroundMusic->m_bIsLocked = 1;
                hr = g_SoundPlayer.backgroundMusic->HandleWaveStreamNotification(looped);
                g_SoundPlayer.backgroundMusic->m_bIsLocked = 0;
            }
            break;
        case 1:
            while (PeekMessageA(&msg, NULL, 0, 0, 1) != 0)
            {
                if (msg.message == 0x12)
                {
                    stopped = true;
                }
            }
            break;
        }
    }
    // STRING: TH07 0x00495cf4
    DebugPrint("atention : ストリーミング用スレッドは終了しました。\r\n");
    return 0;
}

// FUNCTION: TH07 0x0044d2f0
void SoundPlayer::PushCommand(AudioOpcode opcode, i32 arg1, const char *arg2)
{
    for (i32 i = 0; i < 0x1f; i++)
    {
        if (this->commandQueue[i].opcode != 0)
        {
            continue;
        }

        this->commandQueue[i].opcode = opcode;
        this->commandQueue[i].arg1 = arg1;
        strcpy(this->commandQueue[i].string, arg2);
        this->commandQueue[i].arg2 = 0;

        break;
    }
    // STRING: TH07 0x00495ce0
    DebugPrint("Sound Que Add %d\r\n", opcode);
}
