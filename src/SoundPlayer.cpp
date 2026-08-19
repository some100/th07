#include "SoundPlayer.hpp"

#include <cstdio>

#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "Supervisor.hpp"
#include "dxutil.hpp"
#include "utils.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

SoundBufferIdxVolume SOUND_BUFFER_IDX_VOL[38] = {
    {0, -2000, 0},   {0, -2500, 0},   {1, -1200, 5},   {1, -1500, 5},   {2, -1000, 100},
    {3, -400, 100},  {4, -400, 100},  {5, -1500, 50},  {6, -1700, 50},  {7, -1900, 50},
    {8, -1000, 100}, {9, -1000, 100}, {10, -1700, 10}, {11, -1200, 10}, {12, -900, 100},
    {5, -1500, 50},  {13, -900, 50},  {14, -900, 50},  {15, -900, 100}, {16, -200, 100},
    {17, -1400, 0},  {18, -1300, 0},  {5, -100, 20},   {6, -1800, 20},  {7, -1800, 20},
    {19, -800, 50},  {20, -1000, 50}, {21, -1300, 50}, {22, -300, 140}, {23, -900, 100},
    {24, -900, 20},  {25, -500, 90},  {26, -300, 100}, {27, -300, 100}, {24, -300, 20},
    {19, 0, 50},     {28, -300, 100}, {29, -300, 100}};

const char *g_SFXList[30] = {
    "data/wav/se_plst00.wav",   "data/wav/se_enep00.wav",   "data/wav/se_pldead00.wav",
    "data/wav/se_power0.wav",   "data/wav/se_power1.wav",   "data/wav/se_tan00.wav",
    "data/wav/se_tan01.wav",    "data/wav/se_tan02.wav",    "data/wav/se_ok00.wav",
    "data/wav/se_cancel00.wav", "data/wav/se_select00.wav", "data/wav/se_gun00.wav",
    "data/wav/se_cat00.wav",    "data/wav/se_lazer00.wav",  "data/wav/se_lazer01.wav",
    "data/wav/se_enep01.wav",   "data/wav/se_nep00.wav",    "data/wav/se_damage00.wav",
    "data/wav/se_item00.wav",   "data/wav/se_kira00.wav",   "data/wav/se_kira01.wav",
    "data/wav/se_kira02.wav",   "data/wav/se_extend.wav",   "data/wav/se_timeout.wav",
    "data/wav/se_graze.wav",    "data/wav/se_powerup.wav",  "data/wav/se_border.wav",
    "data/wav/se_bonus.wav",    "data/wav/se_bonus2.wav",   "data/wav/se_pause.wav",
};

SoundPlayer g_SoundPlayer;

static ma_result ThBgmDataSource_read(ma_data_source *pDataSource, void *pFramesOut,
                                      ma_uint64 frameCount, ma_uint64 *pFramesRead)
{
    ThBgmDataSource *pBgm = (ThBgmDataSource *)pDataSource;
    if (!pBgm || !pFramesRead)
    {
        return MA_INVALID_ARGS;
    }

    ma_uint32 frameSize = ma_get_bytes_per_frame(pBgm->format, pBgm->channels);
    ma_uint64 totalFramesRead = 0;
    u8 *pByteOut = (u8 *)pFramesOut;

    while (totalFramesRead < frameCount)
    {
        ma_uint64 framesRemainingToRead = frameCount - totalFramesRead;
        ma_uint64 bytesRemainingToRead = framesRemainingToRead * frameSize;

        if (pBgm->segmentBytesRemaining == 0)
        {
            if (pBgm->isMemory)
            {
                pBgm->currentOffset = pBgm->pFmt->introLength;
            }
            else
            {
                if (pBgm->file)
                {
                    SDL_SeekIO(pBgm->file,
                               g_SoundPlayer.bgmSeekOffset + pBgm->pFmt->startOffset +
                                   pBgm->pFmt->introLength,
                               SDL_IO_SEEK_SET);
                    pBgm->currentOffset = pBgm->pFmt->introLength;
                }
            }
            pBgm->segmentBytesRemaining = pBgm->pFmt->totalLength - pBgm->pFmt->introLength;
        }

        ma_uint64 bytesToReadThisStep = bytesRemainingToRead;
        if (bytesToReadThisStep > pBgm->segmentBytesRemaining)
        {
            bytesToReadThisStep = pBgm->segmentBytesRemaining;
        }

        ma_uint64 framesToReadThisStep = bytesToReadThisStep / frameSize;
        if (framesToReadThisStep == 0)
        {
            break;
        }

        bytesToReadThisStep = framesToReadThisStep * frameSize;
        size_t bytesActuallyRead = 0;

        if (pBgm->isMemory)
        {
            if (pBgm->pData)
            {
                if (pBgm->currentOffset + bytesToReadThisStep > pBgm->dataSize)
                {
                    bytesToReadThisStep = pBgm->dataSize - pBgm->currentOffset;
                    framesToReadThisStep = bytesToReadThisStep / frameSize;
                    bytesToReadThisStep = framesToReadThisStep * frameSize;
                }
                if (bytesToReadThisStep > 0)
                {
                    memcpy(pByteOut + (totalFramesRead * frameSize),
                           pBgm->pData + pBgm->currentOffset, (size_t)bytesToReadThisStep);
                    pBgm->currentOffset += (u32)bytesToReadThisStep;
                    bytesActuallyRead = (size_t)bytesToReadThisStep;
                }
            }
        }
        else
        {
            if (pBgm->file)
            {
                bytesActuallyRead = SDL_ReadIO(pBgm->file, pByteOut + (totalFramesRead * frameSize),
                                               (size_t)bytesToReadThisStep);
                pBgm->currentOffset += (u32)bytesActuallyRead;
            }
        }

        if (bytesActuallyRead == 0)
        {
            break;
        }

        ma_uint64 framesActuallyRead = bytesActuallyRead / frameSize;
        totalFramesRead += framesActuallyRead;
        pBgm->segmentBytesRemaining -= (u32)(framesActuallyRead * frameSize);

        if (framesActuallyRead < framesToReadThisStep)
        {
            break;
        }
    }

    *pFramesRead = totalFramesRead;

    if (totalFramesRead == 0 && frameCount > 0)
    {
        return MA_AT_END;
    }

    return MA_SUCCESS;
}

static ma_result ThBgmDataSource_seek(ma_data_source *pDataSource, ma_uint64 frameIndex)
{
    ThBgmDataSource *pBgm = (ThBgmDataSource *)pDataSource;
    if (!pBgm)
    {
        return MA_INVALID_ARGS;
    }

    ma_uint32 frameSize = ma_get_bytes_per_frame(pBgm->format, pBgm->channels);
    ma_uint64 targetByteOffset = frameIndex * frameSize;

    if (targetByteOffset < (ma_uint64)pBgm->pFmt->totalLength)
    {
        pBgm->currentOffset = (u32)targetByteOffset;
        if (!pBgm->isMemory && pBgm->file)
        {
            SDL_SeekIO(pBgm->file,
                       g_SoundPlayer.bgmSeekOffset + pBgm->pFmt->startOffset + pBgm->currentOffset,
                       SDL_IO_SEEK_SET);
        }
        pBgm->segmentBytesRemaining = pBgm->pFmt->totalLength - pBgm->currentOffset;
    }
    else
    {
        ma_uint64 loopStart = pBgm->pFmt->introLength;
        ma_uint64 loopEnd = pBgm->pFmt->totalLength;
        ma_uint64 loopLen = loopEnd - loopStart;

        if (loopLen == 0)
        {
            return MA_INVALID_ARGS;
        }

        ma_uint64 relativeOffset = targetByteOffset - loopEnd;
        ma_uint64 finalByteOffset = loopStart + (relativeOffset % loopLen);

        pBgm->currentOffset = (u32)finalByteOffset;
        if (!pBgm->isMemory && pBgm->file)
        {
            SDL_SeekIO(pBgm->file,
                       g_SoundPlayer.bgmSeekOffset + pBgm->pFmt->startOffset + pBgm->currentOffset,
                       SDL_IO_SEEK_SET);
        }
        pBgm->segmentBytesRemaining = (u32)(loopEnd - pBgm->currentOffset);
    }

    return MA_SUCCESS;
}

static ma_result ThBgmDataSource_get_format(ma_data_source *pDataSource, ma_format *pFormat,
                                            ma_uint32 *pChannels, ma_uint32 *pSampleRate,
                                            ma_channel *pChannelMap, size_t channelMapCap)
{
    ThBgmDataSource *pBgm = (ThBgmDataSource *)pDataSource;
    if (!pBgm)
    {
        return MA_INVALID_ARGS;
    }

    if (pFormat)
    {
        *pFormat = pBgm->format;
    }
    if (pChannels)
    {
        *pChannels = pBgm->channels;
    }
    if (pSampleRate)
    {
        *pSampleRate = pBgm->sampleRate;
    }

    if (pChannelMap)
    {
        ma_channel_map_init_standard(ma_standard_channel_map_default, pChannelMap, channelMapCap,
                                     pBgm->channels);
    }

    return MA_SUCCESS;
}

static ma_result ThBgmDataSource_get_cursor(ma_data_source *pDataSource, ma_uint64 *pCursor)
{
    ThBgmDataSource *pBgm = (ThBgmDataSource *)pDataSource;
    if (!pBgm || !pCursor)
    {
        return MA_INVALID_ARGS;
    }

    ma_uint32 frameSize = ma_get_bytes_per_frame(pBgm->format, pBgm->channels);
    if (pBgm->isMemory)
    {
        *pCursor = pBgm->currentOffset / frameSize;
    }
    else
    {
        if (pBgm->file)
        {
            i64 pos = SDL_TellIO(pBgm->file);
            long baseOffset = g_SoundPlayer.bgmSeekOffset + pBgm->pFmt->startOffset;
            if (pos >= baseOffset)
            {
                *pCursor = (pos - baseOffset) / frameSize;
            }
            else
            {
                *pCursor = 0;
            }
        }
        else
        {
            *pCursor = 0;
        }
    }
    return MA_SUCCESS;
}

static ma_result ThBgmDataSource_get_length(ma_data_source *pDataSource, ma_uint64 *pLength)
{
    ThBgmDataSource *pBgm = (ThBgmDataSource *)pDataSource;
    if (!pBgm || !pLength)
    {
        return MA_INVALID_ARGS;
    }

    ma_uint32 frameSize = ma_get_bytes_per_frame(pBgm->format, pBgm->channels);
    *pLength = pBgm->pFmt->totalLength / frameSize;
    return MA_SUCCESS;
}

static ma_data_source_vtable g_ThBgmDataSourceVtable = {ThBgmDataSource_read,
                                                        ThBgmDataSource_seek,
                                                        ThBgmDataSource_get_format,
                                                        ThBgmDataSource_get_cursor,
                                                        ThBgmDataSource_get_length,
                                                        NULL,
                                                        0};

static void InitBgmData(ThBgmDataSource *pBgm, ThBgmFormat *pFmt)
{
    pBgm->pFmt = pFmt;
    pBgm->currentOffset = 0;
    pBgm->segmentBytesRemaining = pFmt->totalLength;
    pBgm->channels = 2;
    pBgm->sampleRate = 44100;
    pBgm->format = ma_format_s16;
}

static bool ThBgmDataSource_init_file(ThBgmDataSource *pBgm, const char *path, ThBgmFormat *pFmt)
{
    memset(pBgm, 0, sizeof(*pBgm));
    InitBgmData(pBgm, pFmt);
    pBgm->isMemory = false;

    pBgm->file = SDL_IOFromFile(path, "rb");
    if (!pBgm->file)
    {
        return false;
    }

    SDL_SeekIO(pBgm->file, g_SoundPlayer.bgmSeekOffset + pFmt->startOffset, SDL_IO_SEEK_SET);

    ma_data_source_config config = ma_data_source_config_init();
    config.vtable = &g_ThBgmDataSourceVtable;
    if (ma_data_source_init(&config, &pBgm->base) != MA_SUCCESS)
    {
        SDL_CloseIO(pBgm->file);
        return false;
    }
    return true;
}

static bool ThBgmDataSource_init_memory(ThBgmDataSource *pBgm, const u8 *pData, u32 dataSize,
                                        ThBgmFormat *pFmt)
{
    memset(pBgm, 0, sizeof(*pBgm));
    InitBgmData(pBgm, pFmt);
    pBgm->isMemory = true;
    pBgm->pData = pData;
    pBgm->dataSize = dataSize;

    ma_data_source_config config = ma_data_source_config_init();
    config.vtable = &g_ThBgmDataSourceVtable;
    if (ma_data_source_init(&config, &pBgm->base) != MA_SUCCESS)
    {
        return false;
    }
    return true;
}

SoundPlayer::SoundPlayer()
{
    memset(this, 0, sizeof(SoundPlayer));
    for (i32 i = 0; i < ARRAY_SIZE_SIGNED(this->unusedSoundVolRelated); i++)
    {
        this->unusedSoundVolRelated[i] = -1;
    }
}

ZunResult SoundPlayer::InitializeSound()
{
    ma_engine_config engineConfig;

    memset(this, 0, sizeof(SoundPlayer));
    for (i32 i = 0; i < ARRAY_SIZE_SIGNED(this->unusedSoundVolRelated); i++)
    {
        this->unusedSoundVolRelated[i] = -1;
    }

    this->engine = new ma_engine;

    engineConfig = ma_engine_config_init();
    engineConfig.sampleRate = 44100;
    engineConfig.channels = 2;

    if (ma_engine_init(&engineConfig, this->engine) != MA_SUCCESS)
    {
        g_GameErrorContext.Log("DirectSound オブジェクトの初期化が失敗したよ\n");
        SAFE_DELETE(this->engine);
        return ZUN_ERROR;
    }

    if (ma_engine_start(this->engine) != MA_SUCCESS)
    {
        g_GameErrorContext.Log("DirectSound オブジェクトの初期化が失敗したよ\n");
        SAFE_DELETE(this->engine);
        return ZUN_ERROR;
    }

    g_GameErrorContext.Log("DirectSound は正常に初期化されました\n");
    return ZUN_SUCCESS;
}

ZunResult SoundPlayer::Release()
{
    i32 i;

    if (!this->engine)
    {
        return ZUN_SUCCESS;
    }

    for (i = 0; i < ARRAY_SIZE_SIGNED(this->soundBuffers); i++)
    {
        if (this->soundBuffers[i])
        {
            ma_sound_uninit(this->soundBuffers[i]);
            SAFE_DELETE(this->soundBuffers[i]);
        }
        if (this->duplicateSfxData[i])
        {
            ma_audio_buffer_uninit(this->duplicateSfxData[i]);
            SAFE_DELETE(this->duplicateSfxData[i]);
        }
        if (this->sfxPCMData[i])
        {
            ma_free(this->sfxPCMData[i], NULL);
            this->sfxPCMData[i] = NULL;
        }
    }

    StopBGM();

    ma_engine_uninit(this->engine);
    SAFE_DELETE(this->engine);
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->bgmPreloadData); i++)
    {
        SAFE_FREE(this->bgmPreloadData[i]);
    }
    if (this->bgmFmtData)
    {
        free(this->bgmFmtData);
    }
    return ZUN_SUCCESS;
}

i32 SoundPlayer::GetFmtIndexByName(const char *param_1)
{
    std::string filename;
    i32 local_c;
    const char *local_8;

    local_c = 0;
    local_8 = strrchr(param_1, '/');
    if (!local_8)
    {
        local_8 = strrchr(param_1, '\\');
    }
    if (!local_8)
    {
        filename = param_1;
    }
    else
    {
        filename = local_8 + 1;
    }
    while (this->bgmFmtData[local_c].name[0] != '\0')
    {
        if (strcmp(this->bgmFmtData[local_c].name, filename.c_str()) == 0)
        {
            break;
        }
        local_c++;
    }
    if (this->bgmFmtData[local_c].name[0] == '\0')
    {
        local_c = 0;
    }
    return local_c;
}

ZunResult SoundPlayer::LoadSound(i32 idx, const char *path)
{
    void *frames;
    ma_uint64 frameCount;
    u8 *soundFileDat;

    if (!this->engine)
    {
        return ZUN_SUCCESS;
    }

    if (this->sfxPCMData[idx])
    {
        ma_free(this->sfxPCMData[idx], NULL);
        this->sfxPCMData[idx] = NULL;
    }

    soundFileDat = FileSystem::OpenFile(path, 0);
    if (!soundFileDat)
    {
        return ZUN_ERROR;
    }

    if (strncmp((char *)soundFileDat, "RIFF", 4) != 0)
    {
        g_GameErrorContext.Log("Wav ファイルじゃない %s\n", path);
        free(soundFileDat);
        return ZUN_ERROR;
    }

    i32 fileSize = *(u32 *)(soundFileDat + 4) + 8;

    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 44100);
    if (ma_decode_memory(soundFileDat, fileSize, &decoderConfig, &frameCount, &frames) !=
        MA_SUCCESS)
    {
        g_GameErrorContext.Log("Wav ファイルじゃない? %s\n", path);
        free(soundFileDat);
        return ZUN_ERROR;
    }
    this->sfxPCMData[idx] = frames;
    this->sfxFrameCount[idx] = frameCount;

    free(soundFileDat);
    return ZUN_SUCCESS;
}

ZunResult SoundPlayer::LoadFmt(const char *param_1)
{
    this->bgmFmtData = (ThBgmFormat *)FileSystem::OpenFile(param_1, 0);
    return this->bgmFmtData != NULL ? ZUN_SUCCESS : ZUN_ERROR;
}

ZunResult SoundPlayer::StartBGM(const char *path)
{
    SDL_strlcpy(this->bgmArchivePath, FileSystem::GetBasePath(path).c_str(),
                sizeof(this->bgmArchivePath));
    if (!this->engine)
    {
        return ZUN_ERROR;
    }

    Supervisor::DebugPrint("Streming BGM Start\n");
    StopBGM();

    return ZUN_SUCCESS;
}

ZunResult SoundPlayer::ReopenBGM(const char *name)
{
    i32 fmtIdx = GetFmtIndexByName(name);
    StopBGM();

    this->bgmDataSource = new ThBgmDataSource;
    if (!ThBgmDataSource_init_file(this->bgmDataSource, this->bgmArchivePath,
                                   &this->bgmFmtData[fmtIdx]))
    {
        SAFE_DELETE(this->bgmDataSource);
        return ZUN_ERROR;
    }

    this->backgroundMusic = new ma_sound;
    if (ma_sound_init_from_data_source(this->engine, &this->bgmDataSource->base, 0, NULL,
                                       this->backgroundMusic) != MA_SUCCESS)
    {
        SAFE_DELETE(this->backgroundMusic);
        if (this->bgmDataSource->file)
        {
            SDL_CloseIO(this->bgmDataSource->file);
        }
        ma_data_source_uninit(&this->bgmDataSource->base);
        SAFE_DELETE(this->bgmDataSource);
        return ZUN_ERROR;
    }
    Supervisor::DebugPrint("Streming BGM Reopen %d\n", fmtIdx);
    return ZUN_SUCCESS;
}

ZunResult SoundPlayer::PreloadBGM(i32 idx, const char *path)
{
    u8 *lpBuffer;
    SDL_IOStream *file;
    i32 fmtIdx;

    if (this->bgmPreloadData[idx])
    {
        if (strcmp(path, this->bgmFileNames[idx]) == 0)
        {
            return ZUN_SUCCESS;
        }
    }
    strcpy(g_SoundPlayer.bgmFileNames[idx], path);
    if (!g_Supervisor.cfg.preloadBgm)
    {
        return ZUN_SUCCESS;
    }

    if (!this->engine)
    {
        return ZUN_SUCCESS;
    }

    SAFE_FREE(this->bgmPreloadData[idx]);
    Supervisor::DebugPrint("Streming BGM PreLoad %d\n", idx);
    file = SDL_IOFromFile(this->bgmArchivePath, "rb");
    if (!file)
    {
        Supervisor::DebugPrint("error : bgmfile is not find %s\n", this->bgmArchivePath);
        return ZUN_ERROR;
    }

    fmtIdx = GetFmtIndexByName(path);
    SDL_SeekIO(file, this->bgmFmtData[fmtIdx].startOffset, SDL_IO_SEEK_SET);
    lpBuffer = (u8 *)malloc(this->bgmFmtData[fmtIdx].preloadAllocSize);
    if (!lpBuffer)
    {
        SDL_CloseIO(file);
        Supervisor::DebugPrint("error : bgmfile is not find %s\n", this->bgmArchivePath);
        return ZUN_ERROR;
    }

    SDL_ReadIO(file, lpBuffer, this->bgmFmtData[fmtIdx].preloadAllocSize);
    SDL_CloseIO(file);
    this->bgmPreloadFmtData[idx] = &this->bgmFmtData[fmtIdx];
    this->bgmPreloadData[idx] = lpBuffer;
    this->bgmPreloadDataCursor[idx] = lpBuffer;
    this->bgmPreloadAllocSizes[idx] = this->bgmPreloadFmtData[idx]->preloadAllocSize;
    return ZUN_SUCCESS;
}

ZunResult SoundPlayer::LoadBGM(i32 idx)
{
    if (!this->engine)
    {
        return ZUN_ERROR;
    }

    if (g_Supervisor.cfg.musicMode == MUSIC_OFF)
    {
        return ZUN_ERROR;
    }

    if (!g_Supervisor.cfg.preloadBgm)
    {
        return ReopenBGM(this->bgmFileNames[idx]);
    }

    if (!this->bgmPreloadData[idx])
    {
        return ZUN_ERROR;
    }
    Supervisor::DebugPrint("Streming BGM Load no %d\n", idx);
    StopBGM();

    this->bgmDataSource = new ThBgmDataSource;
    if (!ThBgmDataSource_init_memory(this->bgmDataSource, this->bgmPreloadDataCursor[idx],
                                     this->bgmPreloadAllocSizes[idx], this->bgmPreloadFmtData[idx]))
    {
        SAFE_DELETE(this->bgmDataSource);
        return ZUN_ERROR;
    }

    this->backgroundMusic = new ma_sound;
    if (ma_sound_init_from_data_source(this->engine, &this->bgmDataSource->base, 0, NULL,
                                       this->backgroundMusic) != MA_SUCCESS)
    {
        SAFE_DELETE(this->backgroundMusic);
        ma_data_source_uninit(&this->bgmDataSource->base);
        SAFE_DELETE(this->bgmDataSource);
        return ZUN_ERROR;
    }

    Supervisor::DebugPrint("load comp\n");
    this->curBgmIdx = idx;
    return ZUN_SUCCESS;
}

void SoundPlayer::StopBGM()
{
    if (this->backgroundMusic)
    {
        Supervisor::DebugPrint("Streming BGM stop\n");
        ma_sound_stop(this->backgroundMusic);
        ma_sound_uninit(this->backgroundMusic);
        SAFE_DELETE(this->backgroundMusic);
    }
    if (this->bgmDataSource)
    {
        if (this->bgmDataSource->file)
        {
            SDL_CloseIO(this->bgmDataSource->file);
        }
        ma_data_source_uninit(&this->bgmDataSource->base);
        SAFE_DELETE(this->bgmDataSource);
    }
}

ZunResult SoundPlayer::InitSoundBuffers()
{
    i32 i;

    if (!this->engine)
    {
        return ZUN_ERROR;
    }

    for (i = 0; i < ARRAY_SIZE_SIGNED(this->soundQueue); i++)
    {
        this->soundQueue[i] = -1;
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(g_SFXList); i++)
    {
        if (LoadSound(i, g_SFXList[i]) != ZUN_SUCCESS)
        {
            g_GameErrorContext.Log("error : Sound ファイルが読み込めない データを確認 %s\n",
                                   g_SFXList[i]);
            return ZUN_ERROR;
        }
    }
    for (i = 0; i < ARRAY_SIZE(SOUND_BUFFER_IDX_VOL); i++)
    {
        i32 bufIdx = SOUND_BUFFER_IDX_VOL[i].bufferIdx;

        this->duplicateSfxData[i] = new ma_audio_buffer;
        ma_audio_buffer_config bufCfg = ma_audio_buffer_config_init(
            ma_format_f32, 2, this->sfxFrameCount[bufIdx], this->sfxPCMData[bufIdx], NULL);
        ma_audio_buffer_init(&bufCfg, this->duplicateSfxData[i]);

        this->soundBuffers[i] = new ma_sound;
        if (ma_sound_init_from_data_source(this->engine,
                                           (ma_data_source *)this->duplicateSfxData[i], 0, NULL,
                                           this->soundBuffers[i]) != MA_SUCCESS)
        {
            return ZUN_ERROR;
        }

        ma_sound_seek_to_pcm_frame(this->soundBuffers[i], 0);
        ma_sound_set_volume(this->soundBuffers[i],
                            ma_volume_db_to_linear(SOUND_BUFFER_IDX_VOL[i].volume / 100.0f));
    }
    return ZUN_SUCCESS;
}

void SoundPlayer::PlaySoundByIdx(i32 idx, u32 param_2)
{
    (void)param_2;

    i32 iVar1;
    i32 i;

    iVar1 = SOUND_BUFFER_IDX_VOL[idx].field2_0x6;
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->soundQueue); i++)
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

i32 SoundPlayer::ProcessQueues()
{
    char (*name)[256];
    i32 curSound;
    SoundPlayerCommand *commandCursor;
    i32 i;
    u32 loopAgain;

    if (!this->engine)
    {
        return 0;
    }

    commandCursor = this->commandQueue;
loop:
    loopAgain = false;
    switch (commandCursor->opcode)
    {
    case AUDIO_PRELOAD:
        if (g_Supervisor.cfg.preloadBgm)
        {
            Supervisor::DebugPrint("Sound : PreLoad Stage\n");
            if (!commandCursor->arg2)
            {
                StopBGM();
                PreloadBGM(commandCursor->arg1, commandCursor->string);
                loopAgain = true;
                break;
            }
        }
        else
        {
            Supervisor::DebugPrint("Sound : PreLoad Stage\n");
            PreloadBGM(commandCursor->arg1, commandCursor->string);
            loopAgain = true;
            break;
        }
        commandCursor->arg2++;
        goto loop_breakout;
    case AUDIO_START:
        if (g_Supervisor.cfg.preloadBgm && commandCursor->arg1 >= 0)
        {
            if (!commandCursor->arg2)
            {
                Supervisor::DebugPrint("Sound : Load Stage\n");
                if (LoadBGM(commandCursor->arg1) != ZUN_SUCCESS)
                {
                    break;
                }
            }
            else if (commandCursor->arg2 == 2)
            {
                Supervisor::DebugPrint("Sound : Reset Stage\n");
                if (this->backgroundMusic)
                {
                    if (ma_sound_seek_to_pcm_frame(this->backgroundMusic, 0) != MA_SUCCESS)
                    {
                        break;
                    }
                }
            }
            else if (commandCursor->arg2 == 5)
            {
                Supervisor::DebugPrint("Sound : Fill Buffer Stage\n");
            }
            else if (commandCursor->arg2 == 7)
            {
                Supervisor::DebugPrint("Sound : Play Stage\n");
                if (this->backgroundMusic)
                {
                    ma_sound_start(this->backgroundMusic);
                }
            }
            else if (commandCursor->arg2 >= 20)
            {
                break;
            }
        }
        else if (!commandCursor->arg2)
        {
            Supervisor::DebugPrint("Sound : Stop Stage\n");
            ma_sound_stop(this->backgroundMusic);
        }
        else if (commandCursor->arg2 == 1)
        {
            Supervisor::DebugPrint("Sound : Recreate Stage\n");
        }
        else if (commandCursor->arg2 == 2)
        {
            Supervisor::DebugPrint("Sound : ReOpen Stage\n");
            name = commandCursor->arg1 >= 0 ? &this->bgmFileNames[commandCursor->arg1]
                                            : &commandCursor->string;
            ReopenBGM(*name);
        }
        else if (commandCursor->arg2 == 3)
        {
            Supervisor::DebugPrint("Sound : Fill Buffer Stage\n");
        }
        else if (commandCursor->arg2 == 4)
        {
            Supervisor::DebugPrint("Sound : Play Stage\n");
            ma_sound_start(this->backgroundMusic);
        }
        else if (commandCursor->arg2 >= 7)
        {
            break;
        }
        commandCursor->arg2++;
        goto loop_breakout;
    case AUDIO_SHUTDOWN:
        if (!this->backgroundMusic)
        {
            break;
        }

        if (!commandCursor->arg2)
        {
            Supervisor::DebugPrint("Sound : Stop Stage\n");
            ma_sound_stop(this->backgroundMusic);
        }
        else if (commandCursor->arg2 == 1)
        {
            Supervisor::DebugPrint("Sound : Thread Stop Stage\n");
        }
        else if (commandCursor->arg2 == 2)
        {
        }
        else if (commandCursor->arg2 == 3)
        {
            Supervisor::DebugPrint("Sound : Handle Close Stage\n");
            StopBGM();
        }
        else if (commandCursor->arg2 == 10)
        {
            break;
        }
        commandCursor->arg2++;
        goto loop_breakout;
    case AUDIO_STOP:
        if (!this->backgroundMusic)
        {
            break;
        }

        if (!commandCursor->arg2)
        {
            Supervisor::DebugPrint("Sound : Stop Stage\n");
            ma_sound_stop(this->backgroundMusic);
        }
        else if (commandCursor->arg2 == 1)
        {
            break;
        }
        commandCursor->arg2++;
        goto loop_breakout;
    case AUDIO_FADEOUT: {
        Supervisor::DebugPrint("Sound : Fade Out Stage %d\n", commandCursor->arg1);
        g_SoundPlayer.FadeOut(commandCursor->arg1);
        break;
    }
    case AUDIO_PAUSE:
        if (g_Supervisor.cfg.musicMode == MUSIC_WAV)
        {
            if (this->backgroundMusic)
            {
                ma_sound_stop(this->backgroundMusic);
            }
        }
        break;
    case AUDIO_UNPAUSE:
        if (g_Supervisor.cfg.musicMode == MUSIC_WAV)
        {
            if (this->backgroundMusic)
            {
                ma_sound_start(this->backgroundMusic);
            }
        }
        break;
    default:
        goto loop_breakout;
    }
    for (i = 0; i < MAX_SOUND_COMMANDS; i++, commandCursor++)
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
    if (!g_Supervisor.cfg.playSounds)
    {
        return this->commandQueue[0].opcode;
    }
    else
    {
        for (i = 0; i < ARRAY_SIZE_SIGNED(this->soundQueue); i++)
        {
            if (this->soundQueue[i] < 0)
            {
                break;
            }

            curSound = this->soundQueue[i];
            this->soundQueue[i] = -1;
            if (!this->soundBuffers[curSound])
            {
                continue;
            }

            ma_sound_stop(this->soundBuffers[curSound]);
            ma_sound_seek_to_pcm_frame(this->soundBuffers[curSound], 0);
            ma_sound_start(this->soundBuffers[curSound]);
        }
        return this->commandQueue[0].opcode;
    }
}

void SoundPlayer::PushCommand(AudioOpcode opcode, i32 arg1, const char *arg2)
{
    for (i32 i = 0; i < MAX_SOUND_COMMANDS; i++)
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
    Supervisor::DebugPrint("Sound Que Add %d\n", opcode);
}
