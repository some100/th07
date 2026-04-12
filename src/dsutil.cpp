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
#include "SoundPlayer.hpp"
#include "dxutil.hpp"
#include <dsound.h>
#include <mmsystem.h>
#include <windows.h>

//-----------------------------------------------------------------------------
// Name: CSoundManager::CSoundManager()
// Desc: Constructs the class
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045c6f0
CSoundManager::CSoundManager() { pDS = NULL; }

//-----------------------------------------------------------------------------
// Name: CSoundManager::~CSoundManager()
// Desc: Destroys the class
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045c710
CSoundManager::~CSoundManager() { SAFE_RELEASE(pDS); }

//-----------------------------------------------------------------------------
// Name: CSoundManager::Initialize()
// Desc: Initializes the IDirectSound object and also sets the primary buffer
//       format.  This function must be called before any others.
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045c740
HRESULT CSoundManager::Initialize(HWND hWnd, DWORD dwCoopLevel,
                                  DWORD dwPrimaryChannels, DWORD dwPrimaryFreq,
                                  DWORD dwPrimaryBitRate)

{
  DWORD idk;
  HRESULT hr;

  idk = 0;
  SAFE_RELEASE(this->pDS);

  if (FAILED(hr = DirectSoundCreate8(NULL, &this->pDS, NULL)))
    return hr;

  if (FAILED(hr = this->pDS->SetCooperativeLevel(hWnd, dwCoopLevel)))
    return hr;

  SetPrimaryBufferFormat(dwPrimaryChannels, dwPrimaryFreq, dwPrimaryBitRate);
  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::SetPrimaryBufferFormat()
// Desc: Set primary buffer to a specified format
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045c7d0
HRESULT CSoundManager::SetPrimaryBufferFormat(DWORD dwPrimaryChannels,
                                              DWORD dwPrimaryFreq,
                                              DWORD dwPrimaryBitRate)

{
  HRESULT hr;

  LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;
  if (this->pDS == NULL)
    return CO_E_NOTINITIALIZED;

  DSBUFFERDESC dsbd;
  ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
  dsbd.dwSize = sizeof(DSBUFFERDESC);
  dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
  dsbd.dwBufferBytes = 0;
  dsbd.lpwfxFormat = NULL;

  if (FAILED(hr = this->pDS->CreateSoundBuffer(&dsbd, &pDSBPrimary, NULL)))
      return hr;

  WAVEFORMATEX wfx;
  ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
  wfx.wFormatTag = WAVE_FORMAT_PCM;
  wfx.nChannels = dwPrimaryChannels;
  wfx.nSamplesPerSec = dwPrimaryFreq;
  wfx.wBitsPerSample = dwPrimaryBitRate;
  wfx.nBlockAlign = (wfx.wBitsPerSample / 8) * wfx.nChannels;
  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

  hr = pDSBPrimary->SetFormat(&wfx);
  if (FAILED(hr))
      return hr;

  SAFE_RELEASE(pDSBPrimary);
  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::CreateStreaming()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045c8e0
HRESULT CSoundManager::CreateStreaming(CStreamingSound **ppStreamingSound,
                                       LPCSTR strWaveFileName,
                                       DWORD dwCreationFlags,
                                       GUID guid3DAlgorithm,
                                       DWORD dwNotifyCount, DWORD dwNotifySize,
                                       HANDLE hNotifyEvent, ThBgmFormat *pzwf)

{
  HRESULT hr;

  if (this->pDS == NULL)
    return CO_E_NOTINITIALIZED;

  LPDIRECTSOUNDBUFFER pDSBuffer = NULL;
  CWaveFile *pWaveFile = NULL;
  LPDSBPOSITIONNOTIFY aPosNotify = NULL;
  LPDIRECTSOUNDNOTIFY pDSNotify = NULL;

  pWaveFile = new CWaveFile();

  if (pWaveFile->Open(strWaveFileName, pzwf, WAVEFILE_READ) != 0) {
    delete pWaveFile;
    return E_FAIL;
  }
  DWORD dwDSBufferSize = dwNotifySize * dwNotifyCount;
  DSBUFFERDESC dsbd;
  ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
  dsbd.dwSize = sizeof(DSBUFFERDESC);
  dsbd.dwFlags = dwCreationFlags | DSBCAPS_CTRLPOSITIONNOTIFY |
                 DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2 |
                 DSBCAPS_CTRLVOLUME | DSBCAPS_LOCSOFTWARE;
  dsbd.dwBufferBytes = dwDSBufferSize;
  dsbd.guid3DAlgorithm = guid3DAlgorithm;
  dsbd.lpwfxFormat = &pWaveFile->m_pzwf->format;

  if (FAILED(hr = this->pDS->CreateSoundBuffer(&dsbd, &pDSBuffer, NULL))) {
    return E_FAIL;
  }

  if (FAILED(hr = pDSBuffer->QueryInterface(IID_IDirectSoundNotify,
                                            (VOID **)&pDSNotify))) {
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

  if (FAILED(hr = pDSNotify->SetNotificationPositions(dwNotifyCount,
                                                      aPosNotify))) {
    SAFE_RELEASE(pDSNotify);
    SAFE_DELETE_ARRAY(aPosNotify);
    return E_FAIL;
  }

  SAFE_RELEASE(pDSNotify);
  SAFE_DELETE_ARRAY(aPosNotify);

  *ppStreamingSound =
      new CStreamingSound(pDSBuffer, dwDSBufferSize, pWaveFile, dwNotifySize);

  (*ppStreamingSound)->m_dsbd = dsbd;
  (*ppStreamingSound)->m_pSoundManager = this;
  (*ppStreamingSound)->m_hNotifyEvent = hNotifyEvent;
  (*ppStreamingSound)->m_bIsLocked = FALSE;

  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::CreateStreamingFromMemory()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045cc30
HRESULT CSoundManager::CreateStreamingFromMemory(
    CStreamingSound **ppStreamingSound, u8 *pbData, ULONG ulDataSize,
    ThBgmFormat *pzwf, DWORD dwCreationFlags, GUID guid3DAlgorithm,
    DWORD dwNotifyCount, DWORD dwNotifySize, HANDLE hNotifyEvent)

{
  HRESULT hr;

  // STRING: TH07 0x0049548c
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
                DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2 |
                 DSBCAPS_CTRLVOLUME | DSBCAPS_LOCSOFTWARE;
  dsbd.dwBufferBytes = dwDSBufferSize;
  dsbd.guid3DAlgorithm = guid3DAlgorithm;
  dsbd.lpwfxFormat = &pWaveFile->m_pzwf->format;

  if (FAILED(hr = this->pDS->CreateSoundBuffer(&dsbd, &pDSBuffer, NULL)))
    return E_FAIL;

  if (FAILED(hr = pDSBuffer->QueryInterface(IID_IDirectSoundNotify, (VOID **)&pDSNotify)))
    return E_FAIL;

  aPosNotify = new DSBPOSITIONNOTIFY[dwNotifyCount];
  if (aPosNotify == NULL) {
    return E_OUTOFMEMORY;
  }

  for (DWORD i = 0; i < dwNotifyCount; i++) {
    aPosNotify[i].dwOffset = (dwNotifySize * i) + dwNotifySize - 1;
    aPosNotify[i].hEventNotify = hNotifyEvent;
  }

  if (FAILED(hr = pDSNotify->SetNotificationPositions(dwNotifyCount, aPosNotify))) {
    SAFE_RELEASE(pDSNotify);
    SAFE_DELETE_ARRAY(aPosNotify);
    return E_FAIL;
  }

  SAFE_RELEASE(pDSNotify);
  SAFE_DELETE_ARRAY(aPosNotify);

  *ppStreamingSound = new CStreamingSound(pDSBuffer, dwDSBufferSize, pWaveFile, dwNotifySize);;

  (*ppStreamingSound)->m_dsbd = dsbd;
  (*ppStreamingSound)->m_pSoundManager = this;
  (*ppStreamingSound)->m_hNotifyEvent = hNotifyEvent;
  (*ppStreamingSound)->m_bIsLocked = FALSE;
  // STRING: TH07 0x00495480
  DebugPrint("Success \r\n");

  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CSound::CSound()
// Desc: Constructs the class
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045cf50
CSound::CSound(LPDIRECTSOUNDBUFFER *apDSBuffer, DWORD dwDSBufferSize,
               DWORD dwNumBuffers, CWaveFile *pWaveFile)

{
  DWORD i;

  this->m_apDSBuffer = new LPDIRECTSOUNDBUFFER[dwNumBuffers];
  for (i = 0; i < dwNumBuffers; i++) {
    this->m_apDSBuffer[i] = apDSBuffer[i];
  }
  this->m_dwDSBufferSize = dwDSBufferSize;
  this->m_dwNumBuffers = dwNumBuffers;
  this->m_pWaveFile = pWaveFile;

  FillBufferWithSound(this->m_apDSBuffer[0], FALSE);

  for (i = 0; i < dwNumBuffers; i++) {
    this->m_apDSBuffer[i]->SetCurrentPosition(0);
  }
  this->m_bIsPlaying = 0;
}

//-----------------------------------------------------------------------------
// Name: CStreamingSound::InitSoundBuffers()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045d060
HRESULT CStreamingSound::InitSoundBuffers()

{
  DWORD i;

  this->m_bIsPlaying = 0;
  for (i = 0; i < this->m_dwNumBuffers; i++) {
    SAFE_RELEASE(this->m_apDSBuffer[i]);
  }
  SAFE_DELETE_ARRAY(this->m_apDSBuffer);

  LPDSBPOSITIONNOTIFY pPosNotify = NULL;
  LPDIRECTSOUNDNOTIFY pDSNotify = NULL;
  this->m_apDSBuffer = new LPDIRECTSOUNDBUFFER[this->m_dwNumBuffers];

  for (i = 0; i < this->m_dwNumBuffers; i++) {
    if (FAILED(this->m_pSoundManager->pDS->CreateSoundBuffer(
        &this->m_dsbd, &this->m_apDSBuffer[i], NULL)))
      return E_FAIL;

    if (FAILED(this->m_apDSBuffer[i]->QueryInterface(IID_IDirectSoundNotify,
                                               (void **)&pDSNotify)))
      return E_FAIL;

    pPosNotify = new DSBPOSITIONNOTIFY[16];
    if (pPosNotify == NULL) {
      return E_OUTOFMEMORY;
    }

    for (DWORD j = 0; j < 16; j++) {
      pPosNotify[j].dwOffset =
          this->m_dwNotifySize * j + this->m_dwNotifySize - 1;
      pPosNotify[j].hEventNotify = this->m_hNotifyEvent;
    }

    if (FAILED(pDSNotify->SetNotificationPositions(16, pPosNotify))) {
      SAFE_RELEASE(pDSNotify);
      SAFE_DELETE_ARRAY(pPosNotify);
      return E_FAIL;
    }

    SAFE_RELEASE(pDSNotify);
    SAFE_DELETE_ARRAY(pPosNotify);
  }
  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CSound::~CSound()
// Desc: Destroys the class
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045d2c0
CSound::~CSound()

{
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
// FUNCTION: TH07 0x0045d3c0
HRESULT CSound::FillBufferWithSound(LPDIRECTSOUNDBUFFER pDSB,
                                    BOOL bRepeatWavIfBufferLarger)

{
  HRESULT hr;
  VOID *pDSLockedBuffer = NULL;
  DWORD dwDSLockedBufferSize = 0;
  DWORD dwWavDataRead = 0;

  if (pDSB == NULL)
    return CO_E_NOTINITIALIZED;

  if (FAILED(hr = RestoreBuffer(pDSB, NULL)))
    return hr;

  if (FAILED(hr = pDSB->Lock(0, this->m_dwDSBufferSize, &pDSLockedBuffer,
                             &dwDSLockedBufferSize, NULL, NULL, 0)))
    return hr;

  this->m_pWaveFile->ResetFile(false);

  if (FAILED(hr = this->m_pWaveFile->Read(
                 (u8 *)pDSLockedBuffer, dwDSLockedBufferSize, &dwWavDataRead)))
    return hr;

  if (dwWavDataRead == 0) {
    FillMemory(pDSLockedBuffer, dwDSLockedBufferSize,
               (BYTE)(this->m_pWaveFile->m_pzwf->format.wBitsPerSample == 8 ? 128 : 0));
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
      FillMemory((u8 *)pDSLockedBuffer + dwWavDataRead,
                 dwDSLockedBufferSize - dwWavDataRead,
                 (BYTE)(this->m_pWaveFile->m_pzwf->format.wBitsPerSample == 8 ? 128
                                                                       : 0));
    }
  }

  pDSB->Unlock(pDSLockedBuffer, dwDSLockedBufferSize, NULL, 0);
  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CSound::RestoreBuffer()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045d5b0
HRESULT CSound::RestoreBuffer(LPDIRECTSOUNDBUFFER pDSB, BOOL *pbWasRestored)

{
  if (pDSB == NULL)
    return CO_E_NOTINITIALIZED;

  if (pbWasRestored)
    *pbWasRestored = FALSE;

  DWORD dwStatus;
  HRESULT hr;
  if (FAILED(hr = pDSB->GetStatus(&dwStatus)))
    return hr;

  if (dwStatus & DSBSTATUS_BUFFERLOST) {
    do {
      hr = pDSB->Restore();
      if (hr == DSERR_BUFFERLOST) {
        Sleep(10);
      }
    } while (hr = pDSB->Restore());

    if (pbWasRestored)
      *pbWasRestored = TRUE;
    return S_OK;
  } else {
    return S_FALSE;
  }
}

//-----------------------------------------------------------------------------
// Name: CSound::GetFreeBuffer()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045d660
LPDIRECTSOUNDBUFFER CSound::GetFreeBuffer()

{
  BOOL idk = 0;
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

  if (i != this->m_dwNumBuffers) {
    return this->m_apDSBuffer[i];
  } else {
    return this->m_apDSBuffer[rand() % this->m_dwNumBuffers];
  }
}

//-----------------------------------------------------------------------------
// Name: CSound::GetBuffer()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045d720
LPDIRECTSOUNDBUFFER CSound::GetBuffer(DWORD dwIndex)

{
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
// FUNCTION: TH07 0x0045d760
HRESULT CSound::Play(DWORD dwPriority, DWORD dwFlags)

{
  HRESULT hr;

  if (this->m_apDSBuffer == NULL)
    return CO_E_NOTINITIALIZED;

  LPDIRECTSOUNDBUFFER pDSB = GetFreeBuffer();
  if (pDSB == NULL)
    return E_FAIL;

  BOOL bRestored;
  if (FAILED(hr = RestoreBuffer(pDSB, &bRestored)))
    return hr;
  if (bRestored) {
    hr = FillBufferWithSound(pDSB, FALSE);
    if (FAILED(hr))
      return hr;
    Reset();
  }
  this->m_dwIsFadingOut = 0;
  this->m_iCurFadeoutProgress = 0;
  this->m_iTotalFadeout = 0;
  this->m_apDSBuffer[0]->SetVolume(0);
  this->m_bIsPlaying = 1;
  this->m_dwPriority = dwPriority;
  this->m_dwFlags = dwFlags;
  this->unused_2c = 0;
  return pDSB->Play(0, dwPriority, dwFlags);
}

//-----------------------------------------------------------------------------
// Name: CSound::Stop()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045d860
u32 CSound::Stop()

{
  if (this->m_apDSBuffer == NULL)
    return CO_E_NOTINITIALIZED;

  HRESULT hr = 0;
  this->m_bIsPlaying = 0;
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
// FUNCTION: TH07 0x0045d910
HRESULT CSound::Pause()

{
  if (this->m_apDSBuffer == NULL)
    return CO_E_NOTINITIALIZED;

  HRESULT hr = 0;
  this->m_bIsPlaying = 0;
  hr |= this->m_apDSBuffer[0]->Stop();
  return hr;
}

//-----------------------------------------------------------------------------
// Name: CSound::Unpause()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045d960
HRESULT CSound::Unpause()

{
  if (this->m_apDSBuffer == NULL)
    return CO_E_NOTINITIALIZED;

  LPDIRECTSOUNDBUFFER pDSB = this->m_apDSBuffer[0];
  this->m_bIsPlaying = 1;
  return pDSB->Play(0, this->m_dwPriority, this->m_dwFlags);
}

//-----------------------------------------------------------------------------
// Name: CSound::Reset()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045d9b0
HRESULT CSound::Reset()

{
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
// FUNCTION: TH07 0x0045da20
CStreamingSound::CStreamingSound(LPDIRECTSOUNDBUFFER pDSBuffer,
                                 DWORD dwDSBufferSize, CWaveFile *pWaveFile,
                                 DWORD dwNotifySize) : CSound(&pDSBuffer, dwDSBufferSize, 1, pWaveFile)

{
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
// FUNCTION: TH07 0x0045dab0
CStreamingSound::~CStreamingSound() {}

//-----------------------------------------------------------------------------
// Name: CStreamingSound::UpdateFadeOut()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045dad0
HRESULT CStreamingSound::UpdateFadeOut()

{
  if (this->m_dwIsFadingOut != 0) {
    if (--this->m_iCurFadeoutProgress <= 0) {
      this->m_dwIsFadingOut = 0;
      this->m_apDSBuffer[0]->Stop();
      return S_FALSE;
    }
    i32 vol = this->m_iCurFadeoutProgress * 5000 / this->m_iTotalFadeout - 5000;
    HRESULT hr = this->m_apDSBuffer[0]->SetVolume(vol);
  }
  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CStreamingSound::HandleWaveStreamNotification()
// Desc:
//-----------------------------------------------------------------------------
#pragma var_order(dwDSLockedBufferSize2, pDSLockedBuffer, dwBytesWrittenToBuffer, dwCurrentPlayPos, pDSLockedBuffer2, bRestored, dwPlayDelta, hr, dwDSLockedBufferSize, dwCurrentPlayPos2, dwCurrentWritePos, dwReadSoFar)
// FUNCTION: TH07 0x0045db60
HRESULT CStreamingSound::HandleWaveStreamNotification(i32 bLoopedPlay)

{
  HRESULT hr;
  DWORD dwPlayDelta;
  VOID *pDSLockedBuffer;
  VOID *pDSLockedBuffer2;
  DWORD dwCurrentWritePos;
  DWORD dwCurrentPlayPos;
  DWORD dwCurrentPlayPos2;
  DWORD dwDSLockedBufferSize;
  DWORD dwDSLockedBufferSize2;
  DWORD dwBytesWrittenToBuffer;
  DWORD dwReadSoFar;

  if (this->m_apDSBuffer == NULL || this->m_pWaveFile == NULL)
    return CO_E_NOTINITIALIZED;

  this->m_apDSBuffer[0]->GetCurrentPosition(&dwCurrentPlayPos,
                                            &dwCurrentWritePos);

  if ((this->m_dwNextWriteOffset >= dwCurrentWritePos - this->m_dwNotifySize &&
       this->m_dwNextWriteOffset < dwCurrentWritePos) ||
      (dwCurrentWritePos - this->m_dwNotifySize < 0 &&
       this->m_dwNextWriteOffset >= this->m_dwDSBufferSize - this->m_dwNotifySize)) {
    // STRING: TH07 0x00495470
    DebugPrint("Stream Skip\n");
    return CO_E_NOTINITIALIZED;
  }

  BOOL bRestored;
  if (FAILED(hr = RestoreBuffer(this->m_apDSBuffer[0], &bRestored))) {
    // STRING: TH07 0x00495438
    DebugPrint("error : RestoreBuffer in HandleWaveStreamNotification\r\n");
    return hr;
  }

  if (bRestored) {
    if (FAILED(hr = FillBufferWithSound(this->m_apDSBuffer[0], FALSE))) {
      // STRING: TH07 0x004953f8
      DebugPrint("error : FillBufferWithSound in HandleWaveStreamNotification\r\n");
      return hr;
    }
    return S_OK;
  }

  pDSLockedBuffer = NULL;
  pDSLockedBuffer2 = NULL;

  if (FAILED(hr = this->m_apDSBuffer[0]->Lock(
                 this->m_dwNextWriteOffset, this->m_dwNotifySize,
                 &pDSLockedBuffer, &dwDSLockedBufferSize, &pDSLockedBuffer2,
                 &dwDSLockedBufferSize2, 0L))) {
    // STRING: TH07 0x004953c0
    DebugPrint("error : Buffer->Lock in HandleWaveStreamNotification\r\n");
    return hr;
  }

  if (pDSLockedBuffer2 != NULL)
    return E_UNEXPECTED;

  if (!this->m_bFillNextNotificationWithSilence) {
    if (FAILED(hr = this->m_pWaveFile->Read((u8 *)pDSLockedBuffer, dwDSLockedBufferSize,
                                 &dwBytesWrittenToBuffer))) {
      // STRING: TH07 0x00495384
      DebugPrint("error : m_pWaveFile->Read in HandleWaveStreamNotification\r\n");
      return hr;
    }
  } else {
    FillMemory(pDSLockedBuffer, dwDSLockedBufferSize, (BYTE)(this->m_pWaveFile->m_pzwf->format.wBitsPerSample == 8 ? 128 : 0));
    dwBytesWrittenToBuffer = dwDSLockedBufferSize;
  }

  if (dwBytesWrittenToBuffer < dwDSLockedBufferSize) {
    if (!bLoopedPlay) {
      FillMemory((u8 *)pDSLockedBuffer + dwBytesWrittenToBuffer,
                 dwDSLockedBufferSize - dwBytesWrittenToBuffer, (BYTE)(this->m_pWaveFile->m_pzwf->format.wBitsPerSample == 8 ? 128 : 0));
      this->m_bFillNextNotificationWithSilence = TRUE;
    } else {
      dwReadSoFar = dwBytesWrittenToBuffer;
      while (dwReadSoFar < dwDSLockedBufferSize) {
        if (FAILED(hr = this->m_pWaveFile->ResetFile(true))) {
          // STRING: TH07 0x00495340
          DebugPrint("error : m_pWaveFile->ResetFile in HandleWaveStreamNotification\r\n");
          return hr;
        }

        if (FAILED(hr = this->m_pWaveFile->Read(
                       (u8 *)pDSLockedBuffer + dwReadSoFar,
                       dwDSLockedBufferSize - dwReadSoFar,
                       &dwBytesWrittenToBuffer))) {
          // STRING: TH07 0x00495300
          DebugPrint("error : m_pWaveFile->Read(+) in HandleWaveStreamNotification\r\n");
          return hr;
        }

        dwReadSoFar += dwBytesWrittenToBuffer;
      }
    }
  }

  this->m_apDSBuffer[0]->Unlock(pDSLockedBuffer, dwDSLockedBufferSize,
                                NULL, 0);

  if (FAILED(hr = this->m_apDSBuffer[0]->GetCurrentPosition(&dwCurrentPlayPos2,
                                                            NULL))) {
    // STRING: TH07 0x004952b0
    DebugPrint("error : m_apDSBuffer[0]->GetCurrentPosition in HandleWaveStreamNotification\r\n");
    return hr;
  }
  if (dwCurrentPlayPos2 < this->m_dwLastPlayPos) {
    dwPlayDelta =
        (this->m_dwDSBufferSize - this->m_dwLastPlayPos) + dwCurrentPlayPos2;
  } else {
    dwPlayDelta = dwCurrentPlayPos2 - this->m_dwLastPlayPos;
  }

  this->m_dwPlayProgress += dwPlayDelta;
  this->m_dwLastPlayPos = dwCurrentPlayPos2;

  if (this->m_bFillNextNotificationWithSilence) {
    if (this->m_dwPlayProgress >= this->m_pWaveFile->GetSize()) {
      this->m_apDSBuffer[0]->Stop();
    }
  }

  this->m_dwNextWriteOffset += dwDSLockedBufferSize;
  this->m_dwNextWriteOffset %= this->m_dwDSBufferSize;
  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CStreamingSound::Reset()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045df50
HRESULT CStreamingSound::Reset()

{
  HRESULT hr;

  if (this->m_apDSBuffer[0] == NULL || this->m_pWaveFile == NULL)
    return CO_E_NOTINITIALIZED;

  this->m_dwLastPlayPos = 0;
  this->m_dwPlayProgress = 0;
  this->m_dwNextWriteOffset = 0;
  this->m_bFillNextNotificationWithSilence = FALSE;

  BOOL bRestored;
  if (FAILED(hr = RestoreBuffer(this->m_apDSBuffer[0], &bRestored)))
      return hr;

  if (bRestored) {
      if (FAILED(hr = FillBufferWithSound(this->m_apDSBuffer[0], FALSE)))
          return hr;
  }

  this->m_pWaveFile->ResetFile(false);
  return this->m_apDSBuffer[0]->SetCurrentPosition(0);
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::CWaveFile()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045e020
CWaveFile::CWaveFile()

{
  this->m_pzwf = NULL;
  this->h_mmio = NULL;
  this->m_dwSize = 0;
  this->m_bIsReadingFromMemory = FALSE;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::~CWaveFile()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045e060
CWaveFile::~CWaveFile() { Close(); }

//-----------------------------------------------------------------------------
// Name: CWaveFile::Open()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045e080
HRESULT CWaveFile::Open(LPCSTR strFileName, ThBgmFormat *pzwf, DWORD dwFlags)

{
  this->m_dwFlags = dwFlags;
  this->m_bIsReadingFromMemory = FALSE;

  if (this->m_dwFlags == WAVEFILE_READ) {
    if (strFileName == NULL)
      return E_INVALIDARG;

    // STRING: TH07 0x00495294
    DebugPrint("Streaming File Open %s\r\n", strFileName);
    this->m_hWaveFile =
        CreateFileA(strFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL,
                    NULL);
    if (this->m_hWaveFile == INVALID_HANDLE_VALUE) {
      return E_FAIL;
    }

    this->m_pzwf = pzwf;
    ResetFile(false);
    this->m_dwSize = this->m_ck.cksize;
  }

  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::Reopen()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045e130
HRESULT CWaveFile::Reopen(ThBgmFormat *pzwf)

{
  if (this->m_bIsReadingFromMemory != 0)
    return E_FAIL;

  if (this->m_hWaveFile == INVALID_HANDLE_VALUE) {
    return E_FAIL;
  }

  this->m_pzwf = pzwf;
  ResetFile(false);
  this->m_dwSize = this->m_ck.cksize;
  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::OpenFromMemory()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045e190
HRESULT CWaveFile::OpenFromMemory(u8 *pbData, ULONG ulDataSize,
                                  ThBgmFormat *pzwf, DWORD dwFlags)

{
  this->m_pzwf = pzwf;
  this->m_ulDataSize = ulDataSize;
  this->m_pbData = pbData;
  this->m_pbDataCur = this->m_pbData;
  this->m_bIsReadingFromMemory = TRUE;

  if (dwFlags != WAVEFILE_READ)
    return E_NOTIMPL;

  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::GetSize()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045e1f0
DWORD CWaveFile::GetSize()

{
    return this->m_dwSize;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::ResetFile()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045e210
HRESULT CWaveFile::ResetFile(bool bLoop)

{
  DWORD unk;

  if (this->m_bIsReadingFromMemory) {
    this->m_pbDataCur = this->m_pbData;
    if (this->m_pzwf->totalLength > 0) {
      this->m_ulDataSize = this->m_pzwf->totalLength;
    }
    if (bLoop && this->m_pzwf->introLength > 0) {
      this->m_pbDataCur += this->m_pzwf->introLength;
    }
  } else {
    if (this->m_hWaveFile == NULL) {
      return CO_E_NOTINITIALIZED;
    }
    if (bLoop && this->m_pzwf->introLength > 0) {
      unk = SetFilePointer(this->m_hWaveFile,
                     g_SoundPlayer.bgmSeekOffset + this->m_pzwf->introLength +
                         this->m_pzwf->startOffset,
                     NULL, FILE_BEGIN);
      this->m_ck.cksize = this->m_pzwf->totalLength - this->m_pzwf->introLength;
    } else {
      unk = SetFilePointer(this->m_hWaveFile,
                     g_SoundPlayer.bgmSeekOffset + this->m_pzwf->startOffset,
                     NULL, FILE_BEGIN);
      this->m_ck.cksize = this->m_pzwf->totalLength;
    }
  }
  return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::Read()
// Desc:
//-----------------------------------------------------------------------------
#pragma var_order(bytesRead, sizeToRead)
// FUNCTION: TH07 0x0045e360
HRESULT CWaveFile::Read(u8 *pBuffer, DWORD dwSizeToRead, DWORD *pdwSizeRead)

{
  DWORD bytesRead;
  DWORD sizeToRead;

  if (this->m_bIsReadingFromMemory) {
    if (this->m_pbDataCur == NULL)
      return CO_E_NOTINITIALIZED;
    if (pdwSizeRead != NULL)
      *pdwSizeRead = 0;

    if ((DWORD)(this->m_pbDataCur + dwSizeToRead) >
        (DWORD)(this->m_pbData + this->m_ulDataSize)) {
      dwSizeToRead =
          this->m_ulDataSize - (DWORD)(this->m_pbDataCur - this->m_pbData);
    }

    memcpy(pBuffer, this->m_pbDataCur, dwSizeToRead);
    this->m_pbDataCur += dwSizeToRead;

    if (pdwSizeRead != NULL)
      *pdwSizeRead = dwSizeToRead;
    return S_OK;
  } else {
    if (this->m_hWaveFile == NULL)
      return CO_E_NOTINITIALIZED;
    if (pBuffer == NULL || pdwSizeRead == NULL)
      return E_INVALIDARG;

    sizeToRead = dwSizeToRead;
    if (sizeToRead > this->m_ck.cksize) {
      sizeToRead = this->m_ck.cksize;
    }
    this->m_ck.cksize -= sizeToRead;

    ReadFile(this->m_hWaveFile, pBuffer, sizeToRead, &bytesRead, NULL);
    if (pdwSizeRead != NULL) {
      *pdwSizeRead = bytesRead;
    }
    return S_OK;
  }
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::Close()
// Desc:
//-----------------------------------------------------------------------------
// FUNCTION: TH07 0x0045e4b0
HRESULT CWaveFile::Close()

{
  if (this->m_dwFlags == WAVEFILE_READ) {
    CloseHandle(this->m_hWaveFile);
    this->m_hWaveFile = INVALID_HANDLE_VALUE;
  }
  return S_OK;
}

// FUNCTION: TH07 0x0045e4f0
void DebugPrint(const char *fmt, ...) {} // why is this here
