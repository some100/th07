//-----------------------------------------------------------------------------
// File: DSUtil.cpp
//
// Desc: DirectSound framework classes for reading and writing wav files and
//       playing them in DirectSound buffers. Feel free to use this class
//       as a starting point for adding extra functionality.
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#include "dsutil.hpp"
#include "dxutil.hpp"
#include "utils.hpp"
#include <dsound.h>
#include <mmsystem.h>
#include <windows.h>

u32 g_BgmSeekOffset;

//-----------------------------------------------------------------------------
// Name: CSoundManager::CSoundManager()
// Desc: Constructs the class
//-----------------------------------------------------------------------------
CSoundManager::CSoundManager() { pDS = NULL; }

//-----------------------------------------------------------------------------
// Name: CSoundManager::~CSoundManager()
// Desc: Destroys the class
//-----------------------------------------------------------------------------
CSoundManager::~CSoundManager() { SAFE_RELEASE(pDS); }

//-----------------------------------------------------------------------------
// Name: CSoundManager::Initialize()
// Desc: Initializes the IDirectSound object and also sets the primary buffer
//       format.  This function must be called before any others.
//-----------------------------------------------------------------------------
HRESULT CSoundManager::Initialize(HWND hWnd, DWORD dwCoopLevel,
                                  WORD dwPrimaryChannels, DWORD dwPrimaryFreq,
                                  u16 dwPrimaryBitRate) {
  SAFE_RELEASE(this->pDS);

  HRESULT hr = DirectSoundCreate8(NULL, &this->pDS, NULL);
  if (SUCCEEDED(hr)) {
    hr = this->pDS->SetCooperativeLevel(hWnd, dwCoopLevel);
    if (SUCCEEDED(hr)) {
      SetPrimaryBufferFormat(dwPrimaryChannels, dwPrimaryFreq,
                             dwPrimaryBitRate);
      return 0;
    }
  }
  return hr;
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::SetPrimaryBufferFormat()
// Desc: Set primary buffer to a specified format
//-----------------------------------------------------------------------------
HRESULT CSoundManager::SetPrimaryBufferFormat(WORD dwPrimaryChannels,
                                              DWORD dwPrimaryFreq,
                                              u16 dwPrimaryBitRate) {
  if (this->pDS == NULL)
    return CO_E_NOTINITIALIZED;

  DSBUFFERDESC dsbd;
  ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
  dsbd.dwSize = sizeof(DSBUFFERDESC);
  dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
  dsbd.dwBufferBytes = 0;
  dsbd.lpwfxFormat = NULL;

  LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;
  HRESULT hr = this->pDS->CreateSoundBuffer(&dsbd, &pDSBPrimary, NULL);
  if (SUCCEEDED(hr)) {
    WAVEFORMATEX wfx;
    ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
    wfx.cbSize = 0;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = dwPrimaryChannels;
    wfx.nSamplesPerSec = dwPrimaryFreq;
    wfx.wBitsPerSample = dwPrimaryBitRate;
    wfx.nBlockAlign = (wfx.wBitsPerSample / 8) * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    hr = pDSBPrimary->SetFormat(&wfx);
    if (pDSBPrimary != NULL) {
      pDSBPrimary->Release();
    }
    if (SUCCEEDED(hr))
      return S_OK;
  }
  return hr;
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::CreateStreaming()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CSoundManager::CreateStreaming(CStreamingSound **ppStreamingSound,
                                       LPCSTR strWaveFileName,
                                       DWORD dwCreationFlags,
                                       GUID guid3DAlgorithm,
                                       DWORD dwNotifyCount, DWORD dwNotifySize,
                                       HANDLE hNotifyEvent, ThBgmFormat *pzwf) {
  if (this->pDS == NULL)
    return CO_E_NOTINITIALIZED;

  LPDIRECTSOUNDBUFFER pDSBuffer = NULL;
  CWaveFile *pWaveFile = NULL;
  LPDSBPOSITIONNOTIFY aPosNotify = NULL;
  LPDIRECTSOUNDNOTIFY pDSNotify = NULL;

  pWaveFile = new CWaveFile();
  if (pWaveFile == NULL)
    return E_OUTOFMEMORY;

  if (pWaveFile->Open(strWaveFileName, pzwf, WAVEFILE_READ) == 0) {
    DWORD dwDSBufferSize = dwNotifySize * dwNotifyCount;
    DSBUFFERDESC dsbd;
    ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
    dsbd.dwSize = sizeof(DSBUFFERDESC);
    dsbd.dwFlags = dwCreationFlags | DSBCAPS_CTRLPOSITIONNOTIFY |
                   DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS |
                   DSBCAPS_CTRLVOLUME | DSBCAPS_LOCSOFTWARE;
    dsbd.dwBufferBytes = dwDSBufferSize;
    dsbd.guid3DAlgorithm = guid3DAlgorithm;
    dsbd.lpwfxFormat = &pWaveFile->m_pzwf->format;

    HRESULT hr = this->pDS->CreateSoundBuffer(&dsbd, &pDSBuffer, NULL);
    if (FAILED(hr)) {
      return E_FAIL;
    }

    hr = pDSBuffer->QueryInterface(IID_IDirectSoundNotify, (VOID **)&pDSNotify);
    if (FAILED(hr)) {
      return E_FAIL;
    }

    aPosNotify = new DSBPOSITIONNOTIFY[dwNotifyCount];
    if (aPosNotify == NULL) {
      return E_OUTOFMEMORY;
    }

    for (DWORD i = 0; i < dwNotifyCount; i++) {
      aPosNotify[i].dwOffset = (dwNotifySize * i) + dwNotifySize - 1;
      aPosNotify[i].hEventNotify = hNotifyEvent;
    }

    hr = pDSNotify->SetNotificationPositions(dwNotifyCount, aPosNotify);
    if (FAILED(hr)) {
      SAFE_RELEASE(pDSNotify);
      delete[] aPosNotify;
      return E_FAIL;
    }

    pDSNotify->Release();
    delete[] aPosNotify;

    CStreamingSound *pStreamingSound =
        new CStreamingSound(pDSBuffer, dwDSBufferSize, pWaveFile, dwNotifySize);
    *ppStreamingSound = pStreamingSound;

    pStreamingSound->m_dsbd = dsbd;
    pStreamingSound->m_pSoundManager = this;
    pStreamingSound->m_hNotifyEvent = hNotifyEvent;
    pStreamingSound->m_bIsLocked = FALSE;

    return S_OK;
  } else {
    delete pWaveFile;
    return E_FAIL;
  }
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::CreateStreamingFromMemory()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CSoundManager::CreateStreamingFromMemory(
    CStreamingSound **ppStreamingSound, u8 *pbData, ULONG ulDataSize,
    ThBgmFormat *pzwf, DWORD dwCreationFlags, GUID guid3DAlgorithm,
    DWORD dwNotifyCount, DWORD dwNotifySize, HANDLE hNotifyEvent) {
  DebugPrint("StreamingSound Create \r\n");
  if (this->pDS == NULL)
    return CO_E_NOTINITIALIZED;

  LPDIRECTSOUNDBUFFER pDSBuffer = NULL;
  CWaveFile *pWaveFile = NULL;
  LPDSBPOSITIONNOTIFY aPosNotify = NULL;
  LPDIRECTSOUNDNOTIFY pDSNotify = NULL;

  pWaveFile = new CWaveFile();
  pWaveFile->OpenFromMemory(pbData, ulDataSize, pzwf, 0);

  DWORD dwDSBufferSize = dwNotifySize * dwNotifyCount;
  DSBUFFERDESC dsbd;
  ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
  dsbd.dwSize = sizeof(DSBUFFERDESC);
  dsbd.dwFlags = dwCreationFlags | DSBCAPS_CTRLPOSITIONNOTIFY |
                 DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS |
                 DSBCAPS_CTRLVOLUME | DSBCAPS_LOCSOFTWARE;
  dsbd.dwBufferBytes = dwDSBufferSize;
  dsbd.guid3DAlgorithm = guid3DAlgorithm;
  dsbd.lpwfxFormat = &pWaveFile->m_pzwf->format;

  HRESULT hr = this->pDS->CreateSoundBuffer(&dsbd, &pDSBuffer, NULL);
  if (FAILED(hr)) {
    return E_FAIL;
  }

  hr = pDSBuffer->QueryInterface(IID_IDirectSoundNotify, (VOID **)&pDSNotify);
  if (FAILED(hr)) {
    return E_FAIL;
  }

  aPosNotify = new DSBPOSITIONNOTIFY[dwNotifyCount];
  if (aPosNotify == NULL) {
    return E_OUTOFMEMORY;
  }

  for (DWORD i = 0; i < dwNotifyCount; i++) {
    aPosNotify[i].dwOffset = (dwNotifySize * i) + dwNotifySize - 1;
    aPosNotify[i].hEventNotify = hNotifyEvent;
  }

  hr = pDSNotify->SetNotificationPositions(dwNotifyCount, aPosNotify);
  if (FAILED(hr)) {
    pDSNotify->Release();
    delete[] aPosNotify;
    return E_FAIL;
  }

  pDSNotify->Release();
  delete[] aPosNotify;

  CStreamingSound *pStreamingSound =
      new CStreamingSound(pDSBuffer, dwDSBufferSize, pWaveFile, dwNotifySize);
  *ppStreamingSound = pStreamingSound;

  pStreamingSound->m_dsbd = dsbd;
  pStreamingSound->m_pSoundManager = this;
  pStreamingSound->m_hNotifyEvent = hNotifyEvent;
  pStreamingSound->m_bIsLocked = FALSE;
  DebugPrint("Success \r\n");

  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CSound::CSound()
// Desc: Constructs the class
//-----------------------------------------------------------------------------
CSound::CSound(LPDIRECTSOUNDBUFFER *apDSBuffer, DWORD dwDSBufferSize,
               DWORD dwNumBuffers, CWaveFile *pWaveFile) {
  this->m_apDSBuffer = new LPDIRECTSOUNDBUFFER[dwNumBuffers];
  for (DWORD i = 0; i < dwNumBuffers; i++) {
    this->m_apDSBuffer[i] = apDSBuffer[i];
  }
  this->m_dwDSBufferSize = dwDSBufferSize;
  this->m_dwNumBuffers = dwNumBuffers;
  this->m_pWaveFile = pWaveFile;

  FillBufferWithSound(this->m_apDSBuffer[0], FALSE);

  for (DWORD i = 0; i < dwNumBuffers; i++) {
    this->m_apDSBuffer[i]->SetCurrentPosition(0);
  }
  this->m_bIsPlaying = 0;
}

//-----------------------------------------------------------------------------
// Name: CStreamingSound::InitSoundBuffers()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CStreamingSound::InitSoundBuffers() {
  this->m_bIsPlaying = 0;
  for (DWORD i = 0; i < this->m_dwNumBuffers; i++) {
    SAFE_RELEASE(this->m_apDSBuffer[i]);
  }
  SAFE_DELETE_ARRAY(this->m_apDSBuffer);

  LPDSBPOSITIONNOTIFY pPosNotify = NULL;
  LPDIRECTSOUNDNOTIFY pDSNotify = NULL;
  this->m_apDSBuffer = new LPDIRECTSOUNDBUFFER[this->m_dwNumBuffers];

  for (DWORD i = 0; i < this->m_dwNumBuffers; i++) {
    HRESULT hr = this->m_pSoundManager->pDS->CreateSoundBuffer(
        &this->m_dsbd, &this->m_apDSBuffer[i], NULL);
    if (FAILED(hr))
      return E_OUTOFMEMORY;

    hr = this->m_apDSBuffer[i]->QueryInterface(IID_IDirectSoundNotify,
                                               (void **)&pDSNotify);
    if (FAILED(hr))
      return E_OUTOFMEMORY;

    pPosNotify = new DSBPOSITIONNOTIFY[16];
    if (pPosNotify == NULL) {
      pDSNotify->Release();
      return E_OUTOFMEMORY;
    }

    for (DWORD j = 0; j < 16; j++) {
      pPosNotify[j].dwOffset =
          this->m_dwNotifySize * j + this->m_dwNotifySize - 1;
      pPosNotify[j].hEventNotify = this->m_hNotifyEvent;
    }

    hr = pDSNotify->SetNotificationPositions(16, pPosNotify);
    if (FAILED(hr)) {
      pDSNotify->Release();
      delete[] pPosNotify;
      return E_OUTOFMEMORY;
    }

    pDSNotify->Release();
    delete[] pPosNotify;
  }
  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CSound::~CSound()
// Desc: Destroys the class
//-----------------------------------------------------------------------------
CSound::~CSound() {
  for (DWORD i = 0; i < this->m_dwNumBuffers; i++) {
    SAFE_RELEASE(this->m_apDSBuffer[i]);
  }
  SAFE_DELETE_ARRAY(this->m_apDSBuffer);
  SAFE_DELETE(this->m_pWaveFile);
}

//-----------------------------------------------------------------------------
// Name: CSound::FillBufferWithSound()
// Desc: Fills a DirectSound buffer with a sound file
//-----------------------------------------------------------------------------
HRESULT CSound::FillBufferWithSound(LPDIRECTSOUNDBUFFER pDSB,
                                    BOOL bRepeatWavIfBufferLarger) {
  if (pDSB == NULL)
    return CO_E_NOTINITIALIZED;

  HRESULT hr = RestoreBuffer(pDSB, NULL);
  if (FAILED(hr))
    return hr;

  VOID *pDSLockedBuffer = NULL;
  DWORD dwDSLockedBufferSize = 0;
  VOID *pDSLockedBuffer2 = NULL;
  DWORD dwDSLockedBufferSize2 = 0;

  hr = pDSB->Lock(0, this->m_dwDSBufferSize, &pDSLockedBuffer,
                  &dwDSLockedBufferSize, NULL, NULL, 0);
  if (FAILED(hr))
    return hr;

  this->m_pWaveFile->ResetFile(false);

  DWORD dwWavDataRead = 0;
  hr = this->m_pWaveFile->Read((u8 *)pDSLockedBuffer, dwDSLockedBufferSize,
                               &dwWavDataRead);
  if (SUCCEEDED(hr)) {
    if (dwWavDataRead == 0) {
      u8 fill =
          (this->m_pWaveFile->m_pzwf->format.wBitsPerSample == 8) ? 128 : 0;
      FillMemory(pDSLockedBuffer, dwDSLockedBufferSize, fill);
    } else if (dwWavDataRead < dwDSLockedBufferSize) {
      if (bRepeatWavIfBufferLarger) {
        DWORD dwReadSoFar = dwWavDataRead;
        while (dwReadSoFar < dwDSLockedBufferSize) {
          hr = this->m_pWaveFile->ResetFile(false);
          if (FAILED(hr)) {
            return hr;
          }
          hr = this->m_pWaveFile->Read((u8 *)pDSLockedBuffer + dwReadSoFar,
                                       dwDSLockedBufferSize - dwReadSoFar,
                                       &dwWavDataRead);
          if (FAILED(hr)) {
            return hr;
          }
          dwReadSoFar += dwWavDataRead;
        }
      } else {
        u8 fill =
            (this->m_pWaveFile->m_pzwf->format.wBitsPerSample == 8) ? 128 : 0;
        FillMemory((u8 *)pDSLockedBuffer + dwWavDataRead,
                   dwDSLockedBufferSize - dwWavDataRead, fill);
      }
    }
  }

  pDSB->Unlock(pDSLockedBuffer, dwDSLockedBufferSize, pDSLockedBuffer2,
               dwDSLockedBufferSize2);
  return hr;
}

//-----------------------------------------------------------------------------
// Name: CSound::RestoreBuffer()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CSound::RestoreBuffer(LPDIRECTSOUNDBUFFER pDSB, BOOL *pbWasRestored) {
  if (pDSB == NULL)
    return CO_E_NOTINITIALIZED;

  if (pbWasRestored)
    *pbWasRestored = FALSE;

  DWORD dwStatus;
  HRESULT hr = pDSB->GetStatus(&dwStatus);
  if (SUCCEEDED(hr)) {
    if (dwStatus & DSBSTATUS_BUFFERLOST) {
      do {
        hr = pDSB->Restore();
        if (hr == DSERR_BUFFERLOST) {
          Sleep(10);
        }
      } while (hr == DSERR_BUFFERLOST);

      if (SUCCEEDED(hr)) {
        if (pbWasRestored)
          *pbWasRestored = TRUE;
        hr = S_OK;
      }
    } else {
      hr = S_FALSE;
    }
  }
  return hr;
}

//-----------------------------------------------------------------------------
// Name: CSound::GetFreeBuffer()
// Desc:
//-----------------------------------------------------------------------------
LPDIRECTSOUNDBUFFER CSound::GetFreeBuffer() {
  if (this->m_apDSBuffer == NULL)
    return NULL;

  DWORD i;
  for (i = 0; i < this->m_dwNumBuffers; i++) {
    if (this->m_apDSBuffer[i] != NULL) {
      DWORD dwStatus = 0;
      this->m_apDSBuffer[i]->GetStatus(&dwStatus);
      if ((dwStatus & DSBSTATUS_PLAYING) == 0)
        break;
    }
  }

  if (i < this->m_dwNumBuffers) {
    return this->m_apDSBuffer[i];
  } else {
    return this->m_apDSBuffer[rand() % this->m_dwNumBuffers];
  }
}

//-----------------------------------------------------------------------------
// Name: CSound::GetBuffer()
// Desc:
//-----------------------------------------------------------------------------
LPDIRECTSOUNDBUFFER CSound::GetBuffer(DWORD dwIndex) {
  if (this->m_apDSBuffer == NULL)
    return NULL;
  if (dwIndex >= this->m_dwNumBuffers)
    return NULL;
  return this->m_apDSBuffer[dwIndex];
}

//-----------------------------------------------------------------------------
// Name: CSound::Play()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CSound::Play(DWORD dwPriority, DWORD dwFlags) {
  if (this->m_apDSBuffer == NULL)
    return CO_E_NOTINITIALIZED;

  LPDIRECTSOUNDBUFFER pDSB = GetFreeBuffer();
  if (pDSB == NULL)
    return E_FAIL;

  BOOL bRestored;
  HRESULT hr = RestoreBuffer(pDSB, &bRestored);
  if (SUCCEEDED(hr)) {
    if (bRestored) {
      hr = FillBufferWithSound(pDSB, FALSE);
      if (FAILED(hr))
        return hr;
      Reset();
    }
    this->m_dwIsFadingOut = 0;
    this->m_dwCurFadeoutProgress = 0;
    this->m_dwTotalFadeout = 0;
    this->m_apDSBuffer[0]->SetVolume(0);
    this->m_bIsPlaying = 1;
    this->m_dwPriority = dwPriority;
    this->m_dwFlags = dwFlags;
    this->unused_2c = 0;
    hr = pDSB->Play(0, dwPriority, dwFlags);
  }
  return hr;
}

//-----------------------------------------------------------------------------
// Name: CSound::Stop()
// Desc:
//-----------------------------------------------------------------------------
u32 CSound::Stop() {
  if (this->m_apDSBuffer == NULL)
    return CO_E_NOTINITIALIZED;

  this->m_bIsPlaying = 0;
  HRESULT hr = 0;
  for (DWORD i = 0; i < this->m_dwNumBuffers; i++) {
    hr |= this->m_apDSBuffer[i]->Stop();
    hr |= this->m_apDSBuffer[i]->SetCurrentPosition(0);
  }
  this->m_dwIsFadingOut = 0;
  return hr;
}

//-----------------------------------------------------------------------------
// Name: CSound::Pause()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CSound::Pause() {
  if (this->m_apDSBuffer == NULL)
    return CO_E_NOTINITIALIZED;
  this->m_bIsPlaying = 0;
  return this->m_apDSBuffer[0]->Stop();
}

//-----------------------------------------------------------------------------
// Name: CSound::Unpause()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CSound::Unpause() {
  if (this->m_apDSBuffer == NULL)
    return CO_E_NOTINITIALIZED;
  this->m_bIsPlaying = 1;
  return this->m_apDSBuffer[0]->Play(0, this->m_dwPriority, this->m_dwFlags);
}

//-----------------------------------------------------------------------------
// Name: CSound::Reset()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CSound::Reset() {
  if (this->m_apDSBuffer == NULL)
    return CO_E_NOTINITIALIZED;

  HRESULT hr = S_OK;
  for (DWORD i = 0; i < this->m_dwNumBuffers; i++) {
    hr |= this->m_apDSBuffer[i]->SetCurrentPosition(0);
  }
  return hr;
}

//-----------------------------------------------------------------------------
// Name: CStreamingSound::CStreamingSound()
// Desc:
//-----------------------------------------------------------------------------
CStreamingSound::CStreamingSound(LPDIRECTSOUNDBUFFER pDSBuffer,
                                 DWORD dwDSBufferSize, CWaveFile *pWaveFile,
                                 DWORD dwNotifySize)
    : CSound(&pDSBuffer, dwDSBufferSize, 1, pWaveFile) {
  this->m_dwLastPlayPos = 0;
  this->m_dwPlayProgress = 0;
  this->m_dwNotifySize = dwNotifySize;
  this->m_dwNextWriteOffset = 0;
  this->m_bFillNextNotificationWithSilence = FALSE;
}

//-----------------------------------------------------------------------------
// Name: CStreamingSound::~CStreamingSound()
// Desc:
//-----------------------------------------------------------------------------
CStreamingSound::~CStreamingSound() {}

//-----------------------------------------------------------------------------
// Name: CStreamingSound::UpdateFadeOut()
// Desc:
//-----------------------------------------------------------------------------
i32 CStreamingSound::UpdateFadeOut() {
  if (this->m_dwIsFadingOut != 0) {
    this->m_dwCurFadeoutProgress--;
    if (this->m_dwCurFadeoutProgress < 1) {
      this->m_dwIsFadingOut = 0;
      this->m_apDSBuffer[0]->Stop();
      return 1;
    }
    this->m_apDSBuffer[0]->SetVolume(
        (this->m_dwCurFadeoutProgress * 5000) / this->m_dwTotalFadeout - 5000);
  }
  return 0;
}

//-----------------------------------------------------------------------------
// Name: CStreamingSound::HandleWaveStreamNotification()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CStreamingSound::HandleWaveStreamNotification(i32 bLoopedPlay) {
  if (this->m_apDSBuffer == NULL || this->m_pWaveFile == NULL)
    return CO_E_NOTINITIALIZED;

  BOOL bRestored;
  HRESULT hr = RestoreBuffer(this->m_apDSBuffer[0], &bRestored);
  if (FAILED(hr)) {
    DebugPrint(
        "error : RestoreBuffer in HandleWaveStreamNotification\r\n");
    return hr;
  }

  if (bRestored) {
    hr = FillBufferWithSound(this->m_apDSBuffer[0], FALSE);
    if (FAILED(hr)) {
      DebugPrint(
          "error : FillBufferWithSound in HandleWaveStreamNotification\r\n");
      return hr;
    }
    return S_OK;
  }

  DWORD dwCurrentPlayPos, dwCurrentWritePos, dwPlayDelta;
  this->m_apDSBuffer[0]->GetCurrentPosition(&dwCurrentPlayPos,
                                            &dwCurrentWritePos);
  if (this->m_dwNextWriteOffset < dwCurrentWritePos - this->m_dwNotifySize ||
      dwCurrentWritePos <= this->m_dwNextWriteOffset) {
    VOID *pDSLockedBuffer = NULL;
    DWORD dwDSLockedBufferSize = 0;
    VOID *pDSLockedBuffer2 = NULL;
    DWORD dwDSLockedBufferSize2 = 0;

    hr = this->m_apDSBuffer[0]->Lock(
        this->m_dwNextWriteOffset, this->m_dwNotifySize, &pDSLockedBuffer,
        &dwDSLockedBufferSize, &pDSLockedBuffer2, &dwDSLockedBufferSize2, 0L);
    if (FAILED(hr)) {
      DebugPrint(
          "error : Buffer->Lock in HandleWaveStreamNotification\r\n");
      return hr;
    }

    DWORD dwBytesWrittenToBuffer = 0;

    if (!this->m_bFillNextNotificationWithSilence) {
      hr = this->m_pWaveFile->Read((u8 *)pDSLockedBuffer, dwDSLockedBufferSize,
                                   &dwBytesWrittenToBuffer);
      if (FAILED(hr)) {
        DebugPrint(
            "error : m_pWaveFile->Read in HandleWaveStreamNotification\r\n");
        return hr;
      }
    } else {
      u8 fill =
          (this->m_pWaveFile->m_pzwf->format.wBitsPerSample == 8) ? 128 : 0;
      FillMemory(pDSLockedBuffer, dwDSLockedBufferSize, fill);
      dwBytesWrittenToBuffer = dwDSLockedBufferSize;
    }

    if (dwBytesWrittenToBuffer < dwDSLockedBufferSize) {
      if (!bLoopedPlay) {
        u8 fill =
            (this->m_pWaveFile->m_pzwf->format.wBitsPerSample == 8) ? 128 : 0;
        FillMemory((u8 *)pDSLockedBuffer + dwBytesWrittenToBuffer,
                   dwDSLockedBufferSize - dwBytesWrittenToBuffer, fill);
        this->m_bFillNextNotificationWithSilence = TRUE;
      } else {
        DWORD dwReadSoFar = dwBytesWrittenToBuffer;
        while (dwReadSoFar < dwDSLockedBufferSize) {
          hr = this->m_pWaveFile->ResetFile(true);
          if (FAILED(hr)) {
            DebugPrint("error : m_pWaveFile->ResetFile in "
                              "HandleWaveStreamNotification\r\n");
            return hr;
          }

          hr = this->m_pWaveFile->Read((u8 *)pDSLockedBuffer + dwReadSoFar,
                                       dwDSLockedBufferSize - dwReadSoFar,
                                       &dwBytesWrittenToBuffer);
          if (FAILED(hr)) {
            DebugPrint("error : m_pWaveFile->Read(+) in "
                              "HandleWaveStreamNotification\r\n");
            return hr;
          }

          dwReadSoFar += dwBytesWrittenToBuffer;
        }
      }
    }

    this->m_apDSBuffer[0]->Unlock(pDSLockedBuffer, dwDSLockedBufferSize,
                                  pDSLockedBuffer2, dwDSLockedBufferSize2);

    hr = this->m_apDSBuffer[0]->GetCurrentPosition(&dwCurrentPlayPos, NULL);
    if (FAILED(hr)) {
      DebugPrint("error : m_apDSBuffer[0]->GetCurrentPosition in "
                        "HandleWaveStreamNotification\r\n");
    } else {
      if (dwCurrentPlayPos < this->m_dwLastPlayPos) {
        dwPlayDelta =
            (this->m_dwDSBufferSize - this->m_dwLastPlayPos) + dwCurrentPlayPos;
      } else {
        dwPlayDelta = dwCurrentPlayPos - this->m_dwLastPlayPos;
      }

      this->m_dwPlayProgress += dwPlayDelta;
      this->m_dwLastPlayPos = dwCurrentPlayPos;

      if (this->m_bFillNextNotificationWithSilence) {
        if (this->m_dwPlayProgress >= this->m_pWaveFile->GetSize()) {
          this->m_apDSBuffer[0]->Stop();
        }
      }

      this->m_dwNextWriteOffset += dwDSLockedBufferSize;
      this->m_dwNextWriteOffset %= this->m_dwDSBufferSize;
    }
  } else {
    DebugPrint("Stream Skip\n");
    hr = CO_E_NOTINITIALIZED;
  }
  return hr;
}

//-----------------------------------------------------------------------------
// Name: CStreamingSound::Reset()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CStreamingSound::Reset() {
  if (this->m_apDSBuffer[0] == NULL || this->m_pWaveFile == NULL)
    return CO_E_NOTINITIALIZED;

  this->m_dwLastPlayPos = 0;
  this->m_dwPlayProgress = 0;
  this->m_dwNextWriteOffset = 0;
  this->m_bFillNextNotificationWithSilence = FALSE;

  BOOL bRestored;
  HRESULT hr = RestoreBuffer(this->m_apDSBuffer[0], &bRestored);
  if (SUCCEEDED(hr)) {
    if (!bRestored ||
        SUCCEEDED(hr = FillBufferWithSound(this->m_apDSBuffer[0], FALSE))) {
      this->m_pWaveFile->ResetFile(false);
      hr = this->m_apDSBuffer[0]->SetCurrentPosition(0);
    }
  }
  return hr;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::CWaveFile()
// Desc:
//-----------------------------------------------------------------------------
CWaveFile::CWaveFile() {
  this->m_pzwf = NULL;
  this->h_mmio = NULL;
  this->m_dwSize = 0;
  this->m_bIsReadingFromMemory = FALSE;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::~CWaveFile()
// Desc:
//-----------------------------------------------------------------------------
CWaveFile::~CWaveFile() { Close(); }

//-----------------------------------------------------------------------------
// Name: CWaveFile::Open()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CWaveFile::Open(LPCSTR strFileName, ThBgmFormat *pzwf, DWORD dwFlags) {
  this->m_dwFlags = dwFlags;
  this->m_bIsReadingFromMemory = FALSE;

  if (this->m_dwFlags == WAVEFILE_READ) {
    if (strFileName == NULL)
      return E_INVALIDARG;

    DebugPrint("Streaming File Open %s\r\n", strFileName);
    this->m_hWaveFile =
        CreateFileA(strFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (this->m_hWaveFile == INVALID_HANDLE_VALUE) {
      return E_FAIL;
    }

    this->m_pzwf = pzwf;
    ResetFile(false);
    this->m_dwSize = this->m_pzwf->totalLength;
  }

  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::Reopen()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CWaveFile::Reopen(ThBgmFormat *pzwf) {
  if (this->m_bIsReadingFromMemory == 0) {
    if (this->m_hWaveFile == INVALID_HANDLE_VALUE) {
      return E_FAIL;
    } else {
      this->m_pzwf = pzwf;
      ResetFile(false);
      this->m_dwSize = this->m_pzwf->totalLength;
      return S_OK;
    }
  } else {
    return E_FAIL;
  }
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::OpenFromMemory()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CWaveFile::OpenFromMemory(u8 *pbData, ULONG ulDataSize,
                                  ThBgmFormat *pzwf, DWORD dwFlags) {
  this->m_pzwf = pzwf;
  this->m_ulDataSize = ulDataSize;
  this->m_pbData = pbData;
  this->m_pbDataCur = this->m_pbData;
  this->m_bIsReadingFromMemory = TRUE;

  if (dwFlags == WAVEFILE_READ) {
    return S_OK;
  } else {
    return E_NOTIMPL;
  }
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::GetSize()
// Desc:
//-----------------------------------------------------------------------------
DWORD CWaveFile::GetSize() { return this->m_dwSize; }

//-----------------------------------------------------------------------------
// Name: CWaveFile::ResetFile()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CWaveFile::ResetFile(bool bLoop) {
  if (!this->m_bIsReadingFromMemory) {
    if (this->m_hWaveFile == NULL) {
      return CO_E_NOTINITIALIZED;
    }
    if (bLoop && this->m_pzwf->introLength > 0) {
      SetFilePointer(this->m_hWaveFile,
                     g_BgmSeekOffset + this->m_pzwf->introLength +
                         this->m_pzwf->startOffset,
                     NULL, FILE_BEGIN);
      this->m_ck.cksize = this->m_pzwf->totalLength - this->m_pzwf->introLength;
    } else {
      SetFilePointer(this->m_hWaveFile,
                     g_BgmSeekOffset + this->m_pzwf->startOffset, NULL,
                     FILE_BEGIN);
      this->m_ck.cksize = this->m_pzwf->totalLength;
    }
  } else {
    this->m_pbDataCur = this->m_pbData;
    if (this->m_pzwf->totalLength > 0) {
      this->m_ulDataSize = this->m_pzwf->totalLength;
    }
    if (bLoop && this->m_pzwf->introLength > 0) {
      this->m_pbDataCur += this->m_pzwf->introLength;
    }
  }
  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::Read()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CWaveFile::Read(u8 *pBuffer, DWORD dwSizeToRead, DWORD *pdwSizeRead) {
  if (!this->m_bIsReadingFromMemory) {
    if (this->m_hWaveFile == NULL)
      return CO_E_NOTINITIALIZED;
    if (pBuffer == NULL || pdwSizeRead == NULL)
      return E_INVALIDARG;

    DWORD sizeToRead = dwSizeToRead;
    if (this->m_ck.cksize < sizeToRead) {
      sizeToRead = this->m_ck.cksize;
    }
    this->m_ck.cksize -= sizeToRead;

    DWORD bytesRead = 0;
    ReadFile(this->m_hWaveFile, pBuffer, sizeToRead, &bytesRead, NULL);
    if (pdwSizeRead != NULL) {
      *pdwSizeRead = bytesRead;
    }
    return S_OK;
  } else {
    if (this->m_pbDataCur == NULL)
      return CO_E_NOTINITIALIZED;
    if (pdwSizeRead != NULL)
      *pdwSizeRead = 0;

    DWORD sizeToRead = dwSizeToRead;
    if ((DWORD)(this->m_pbData + this->m_ulDataSize) <
        (DWORD)(this->m_pbDataCur + sizeToRead)) {
      sizeToRead =
          this->m_ulDataSize - (DWORD)(this->m_pbDataCur - this->m_pbData);
    }

    CopyMemory(pBuffer, this->m_pbDataCur, sizeToRead);
    this->m_pbDataCur += sizeToRead;

    if (pdwSizeRead != NULL)
      *pdwSizeRead = sizeToRead;
    return S_OK;
  }
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::Close()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CWaveFile::Close() {
  if (this->m_dwFlags == WAVEFILE_READ) {
    CloseHandle(this->m_hWaveFile);
    this->m_hWaveFile = INVALID_HANDLE_VALUE;
  }
  return S_OK;
}

void DebugPrint(const char *fmt, ...) {} // why is this here
