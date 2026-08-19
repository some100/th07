#pragma once

#include "GameErrorContext.hpp"
#include "ZunResult.hpp"
#include "inttypes.hpp"
#include "miniaudio.h"

typedef enum AudioOpcode
{
    AUDIO_PRELOAD = 1,
    AUDIO_START = 2,
    AUDIO_STOP = 3,
    AUDIO_SHUTDOWN = 4,
    AUDIO_FADEOUT = 5,
    AUDIO_PAUSE = 6,
    AUDIO_UNPAUSE = 7
} AudioOpcode;

typedef enum SoundIdx
{
    SOUND_0 = 0,
    SOUND_PICHUN = 4,
    SOUND_BOMB_SAKUYA_A = 5,
    SOUND_BOMB_REIMARI = 6,
    SOUND_BOMB_MARISA_A_FOCUS = 7,
    SOUND_SELECT = 10,
    SOUND_BACK = 11,
    SOUND_MOVE_MENU = 12,
    SOUND_BOMB_REIMU_A = 13,
    SOUND_BOMB = 14,
    SOUND_ENEMY_SPELLCARD_END = 15,
    SOUND_BOMB_SAKUMARI = 19,
    SOUND_20 = 20,
    SOUND_21 = 21,
    SOUND_25 = 25,
    SOUND_EXTEND = 28,
    SOUND_29 = 29,
    SOUND_GRAZE = 30,
    SOUND_POWERUP = 31,
    SOUND_BORDER_ACTIVATE = 32,
    SOUND_BORDER_BREAK = 33,
    SOUND_BORDER_ACTIVATE2 = 36,
    SOUND_37 = 37
} SoundIdx;

struct ThWaveFormat
{
    u16 wFormatTag;
    u16 nChannels;
    u32 nSamplesPerSec;
    u32 nAvgBytesPerSec;
    u16 nBlockAlign;
    u16 wBitsPerSample;
    u16 cbSize;
};
static_assert(sizeof(ThWaveFormat) == 0x14);

struct ThBgmFormat
{
    char name[16];
    i32 startOffset;
    u32 preloadAllocSize;
    i32 introLength;
    i32 totalLength;
    ThWaveFormat format;
};
static_assert(sizeof(ThBgmFormat) == 0x34);

struct SoundBufferIdxVolume
{
    i32 bufferIdx;
    i16 volume;
    i16 field2_0x6;
};

struct SoundPlayerCommand
{
    i32 opcode;
    i32 arg1;
    i32 arg2;
    char string[256];
};

struct ThBgmDataSource
{
    ma_data_source_base base;
    ma_format format;
    ma_uint32 channels;
    ma_uint32 sampleRate;
    ThBgmFormat *pFmt;
    bool isMemory;

    SDL_IOStream *file;
    const u8 *pData;
    u32 dataSize;
    u32 currentOffset;
    u32 segmentBytesRemaining;
};

#define MAX_SOUND_COMMANDS 31

struct SoundPlayer
{
    SoundPlayer();

    i32 GetFmtIndexByName(const char *param_1);
    ZunResult InitializeSound();
    ZunResult InitSoundBuffers();
    ZunResult LoadBGM(i32 idx);
    ZunResult LoadFmt(const char *path);
    ZunResult LoadSound(i32 idx, const char *path);
    void PlaySoundByIdx(i32 idx, u32 param_2);
    ZunResult PreloadBGM(i32 idx, const char *path);
    i32 ProcessQueues();
    void PushCommand(AudioOpcode opcode, i32 arg1, const char *arg2);
    ZunResult Release();
    ZunResult ReopenBGM(const char *name);
    ZunResult StartBGM(const char *path);
    void StopBGM();

    void FadeOut(f32 duration)
    {
        g_GameErrorContext.Log("%f\n", duration);
        if (this->backgroundMusic)
        {
            ma_sound_set_fade_start_in_milliseconds(
                this->backgroundMusic, -1, 0, duration * 1000.0f,
                ma_engine_get_time_in_milliseconds(this->engine));
        }
    }

    ma_engine *engine;
    ma_audio_buffer *duplicateSfxData[128];
    ma_audio_buffer *sfxData[128];
    void *sfxPCMData[128];
    ma_uint64 sfxFrameCount[128];
    ma_sound *soundBuffers[128];
    i32 unusedSoundVolRelated[128];
    i32 soundQueue[5];
    ThBgmFormat *bgmPreloadFmtData[16];
    u8 *bgmPreloadData[16];
    u8 *bgmPreloadDataCursor[16];
    u32 bgmPreloadAllocSizes[16];
    i32 curBgmIdx;
    ThBgmFormat *bgmFmtData;
    SoundPlayerCommand commandQueue[MAX_SOUND_COMMANDS + 1];
    char bgmFileNames[16][256];
    char bgmArchivePath[512];
    ThBgmDataSource *bgmDataSource;
    ma_sound *backgroundMusic;
    i32 bgmSeekOffset;
};

extern SoundPlayer g_SoundPlayer;
