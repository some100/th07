#pragma once

#include <dsound.h>

#include "ZunResult.hpp"
#include "dsutil.hpp"
#include "inttypes.hpp"

typedef enum AudioOpcode {
  AUDIO_PRELOAD = 1,
  AUDIO_START = 2,
  AUDIO_STOP = 3,
  AUDIO_SHUTDOWN = 4,
  AUDIO_FADEOUT = 5,
  AUDIO_PAUSE = 6,
  AUDIO_UNPAUSE = 7
} AudioOpcode;

typedef enum SoundIdx {
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

struct SoundBufferIdxVolume {
  i32 bufferIdx;
  i16 volume;
  i16 field2_0x6;
};

struct SoundPlayerCommand {
  i32 opcode;
  i32 arg1;
  i32 arg2;
  char string[256];
};
C_ASSERT(sizeof(SoundPlayerCommand) == 0x10c);

struct SoundPlayer {
  SoundPlayer();

  static DWORD __stdcall BackgroundMusicPlayerThread(LPVOID lpThreadParameter);
  i32 GetFmtIndexByName(const char *param_1);
  WAVEFORMATEX *GetWavFormatData(u8 *soundData, const char *formatString,
                                 i32 *formatSize, i32 fileSizeExcludingFormat);
  ZunResult InitializeDSound(HWND gameWindow);
  ZunResult InitSoundBuffers();
  i32 LoadBGM(i32 idx);
  i32 LoadFmt(const char *path);
  ZunResult LoadSound(i32 idx, const char *path);
  void PlaySoundByIdx(i32 idx);
  ZunResult PreloadBGM(i32 idx, const char *path);
  i32 ProcessQueues();
  void PushCommand(AudioOpcode opcode, i32 arg1, const char *arg2);
  ZunResult Release();
  ZunResult ReopenBGM(const char *name);
  ZunResult StartBGM(const char *path);
  void StopBGM();

  LPDIRECTSOUND8 directSoundHdl;
  i32 unused_4;
  LPDIRECTSOUNDBUFFER soundBuffers[128];
  LPDIRECTSOUNDBUFFER duplicateSoundBuffers[128];
  i32 unusedSoundVolRelated[128];
  LPDIRECTSOUNDBUFFER initSoundBuffer;
  HWND gameWindow;
  CSoundManager *manager;
  DWORD backgroundMusicThreadId;
  HANDLE backgroundMusicThreadHandle;
  u32 unused_61c;
  i32 soundQueue[5];
  ThBgmFormat *bgmPreloadFmtData[16];
  LPBYTE bgmPreloadData[16];
  LPBYTE bgmPreloadDataCursor[16];
  DWORD bgmPreloadAllocSizes[16];
  i32 curBgmIdx;
  ThBgmFormat *bgmFmtData;
  SoundPlayerCommand commandQueue[32];
  char bgmFileNames[16][256];
  char bgmArchivePath[256];
  CStreamingSound *backgroundMusic;
  HANDLE backgroundMusicUpdateEvent;
  i32 unused_39c4;
  i32 unused_39c8;
};
C_ASSERT(sizeof(SoundPlayer) == 0x39cc);

extern SoundPlayer g_SoundPlayer;
