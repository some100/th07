//-----------------------------------------------------------------------------
// File: dsutil.hpp
//
// Desc:
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

#include <dsound.h>
#include <mmreg.h>
#include <mmsystem.h>
#include <windows.h>

#include "inttypes.hpp"

//-----------------------------------------------------------------------------
// Classes used by this header
//-----------------------------------------------------------------------------
class CSoundManager;
class CSound;
class CStreamingSound;
class CWaveFile;

//-----------------------------------------------------------------------------
// Typing macros
//-----------------------------------------------------------------------------
#define WAVEFILE_READ 1
#define WAVEFILE_WRITE 2

#define DSUtil_StopSound(s) \
    {                       \
        if (s)              \
            s->Stop();      \
    }
#define DSUtil_PlaySound(s) \
    {                       \
        if (s)              \
            s->Play(0, 0);  \
    }
#define DSUtil_PlaySoundLooping(s)       \
    {                                    \
        if (s)                           \
            s->Play(0, DSBPLAY_LOOPING); \
    }

struct ThBgmFormat
{
    char name[16];
    i32 startOffset;
    DWORD preloadAllocSize;
    i32 introLength;
    i32 totalLength;
    WAVEFORMATEX format;
    // pad 2
};
C_ASSERT(sizeof(ThBgmFormat) == 0x34);

//-----------------------------------------------------------------------------
// Name: class CWaveFile
// Desc: Encapsulates reading or writing sound data to or from a wave file
//-----------------------------------------------------------------------------
class CWaveFile
{
  public:
    HMMIO h_mmio;
    MMCKINFO m_ck;
    MMCKINFO m_ckRiff;
    DWORD m_dwSize;
    MMIOINFO m_mmioinfo;
    DWORD m_dwFlags;
    BOOL m_bIsReadingFromMemory;
    u8 *m_pbData;
    u8 *m_pbDataCur;
    ULONG m_ulDataSize;
    HANDLE m_hWaveFile;
    ThBgmFormat *m_pzwf;

    CWaveFile();
    ~CWaveFile();

    HRESULT Open(LPCSTR strFileName, ThBgmFormat *pzwf, DWORD dwFlags);
    HRESULT OpenFromMemory(u8 *pbData, ULONG ulDataSize, ThBgmFormat *pzwf,
                           DWORD dwFlags);
    HRESULT Close();

    HRESULT Read(u8 *pBuffer, DWORD dwSizeToRead, DWORD *pdwSizeRead);
    DWORD GetSize();
    HRESULT ResetFile(bool bLoop);
    HRESULT Reopen(ThBgmFormat *pzwf);

    ThBgmFormat *GetFormat()
    {
        return this->m_pzwf;
    }
};

//-----------------------------------------------------------------------------
// Name: class CSoundManager
// Desc:
//-----------------------------------------------------------------------------
class CSoundManager
{
  public:
    LPDIRECTSOUND8 pDS;

    CSoundManager();
    ~CSoundManager();

    HRESULT Initialize(HWND hWnd, DWORD dwCoopLevel, DWORD dwPrimaryChannels,
                       DWORD dwPrimaryFreq, DWORD dwPrimaryBitRate);
    inline LPDIRECTSOUND8 GetDirectSound()
    {
        return pDS;
    }
    HRESULT SetPrimaryBufferFormat(DWORD dwPrimaryChannels, DWORD dwPrimaryFreq,
                                   DWORD dwPrimaryBitRate);

    HRESULT CreateStreaming(CStreamingSound **ppStreamingSound,
                            LPCSTR strWaveFileName, DWORD dwCreationFlags,
                            GUID guid3DAlgorithm, DWORD dwNotifyCount,
                            DWORD dwNotifySize, HANDLE hNotifyEvent,
                            ThBgmFormat *pzwf);
    HRESULT CreateStreamingFromMemory(CStreamingSound **ppStreamingSound,
                                      u8 *pbData, ULONG ulDataSize,
                                      ThBgmFormat *pzwf, DWORD dwCreationFlags,
                                      GUID guid3DAlgorithm, DWORD dwNotifyCount,
                                      DWORD dwNotifySize, HANDLE hNotifyEvent);
};

//-----------------------------------------------------------------------------
// Name: class CSound
// Desc: Encapsulates functionality of a DirectSound buffer.
//-----------------------------------------------------------------------------
// VTABLE: TH07 0x00495290
class CSound
{
  public:
    LPDIRECTSOUNDBUFFER *m_apDSBuffer;
    DWORD m_dwDSBufferSize;
    CWaveFile *m_pWaveFile;
    DWORD m_dwNumBuffers;
    i32 m_iCurFadeoutProgress;
    i32 m_iTotalFadeout;
    DWORD m_dwIsFadingOut;
    DWORD m_dwPriority;
    DWORD m_dwFlags;
    DWORD unused_28;
    DWORD unused_2c;
    BOOL m_bIsPlaying;
    DSBUFFERDESC m_dsbd;
    CSoundManager *m_pSoundManager;

    CSound(LPDIRECTSOUNDBUFFER *apDSBuffer, DWORD dwDSBufferSize,
           DWORD dwNumBuffers, CWaveFile *pWaveFile);
    virtual ~CSound();

    // SYNTHETIC: TH07 0x0045d030
    // CSound::`scalar deleting destructor'

    HRESULT RestoreBuffer(LPDIRECTSOUNDBUFFER pDSB, BOOL *pbWasRestored);
    HRESULT FillBufferWithSound(LPDIRECTSOUNDBUFFER pDSB,
                                BOOL bRepeatWavIfBufferLarger);
    LPDIRECTSOUNDBUFFER GetFreeBuffer();
    LPDIRECTSOUNDBUFFER GetBuffer(DWORD dwIndex);

    HRESULT Play(DWORD dwPriority, DWORD dwFlags);
    u32 Stop();
    HRESULT Reset();
    HRESULT Pause();
    HRESULT Unpause();
};
C_ASSERT(sizeof(CSound) == 0x5c);

//-----------------------------------------------------------------------------
// Name: class CStreamingSound
// Desc: Encapsulates functionality to play a wave file with DirectSound.
//-----------------------------------------------------------------------------
// VTABLE: TH07 0x0049528c
class CStreamingSound : public CSound
{
  public:
    DWORD m_dwLastPlayPos;
    DWORD m_dwPlayProgress;
    DWORD m_dwNextWriteOffset;
    BOOL m_bFillNextNotificationWithSilence;
    DWORD m_dwNotifySize;
    HANDLE m_hNotifyEvent;
    BOOL m_bIsLocked;

    CStreamingSound(LPDIRECTSOUNDBUFFER pDSBuffer, DWORD dwDSBufferSize,
                    CWaveFile *pWaveFile, DWORD dwNotifySize);
    ~CStreamingSound();

    // SYNTHETIC: TH07 0x0045da80
    // CStreamingSound::`scalar deleting destructor'

    HRESULT HandleWaveStreamNotification(i32 bLoopedPlay);
    HRESULT Reset();
    HRESULT InitSoundBuffers();
    HRESULT UpdateFadeOut();

    void FadeOut(f32 duration)
    {
        this->m_dwIsFadingOut = 1;
        this->m_iCurFadeoutProgress = (duration * 60.0f);
        this->m_iTotalFadeout =
            this->m_iCurFadeoutProgress;
    }

    CWaveFile *GetWaveFile()
    {
        return this->m_pWaveFile;
    }
};
C_ASSERT(sizeof(CStreamingSound) == 0x78);

void DebugPrint(const char *fmt, ...);
