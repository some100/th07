#include "MidiOutput.hpp"

#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "dsutil.hpp"
#include "inttypes.hpp"

// GLOBAL: TH07 0x0135e210
LARGE_INTEGER g_PerfCounter;

// FUNCTION: TH07 0x00436110
MidiDevice::MidiDevice()

{
  this->handle = NULL;
  this->deviceID = 0;
}

// FUNCTION: TH07 0x00436140
MidiDevice::~MidiDevice() { Close(); }

// FUNCTION: TH07 0x00436160
bool MidiDevice::OpenDevice(u32 deviceID)

{
  if (this->handle != NULL) {
    if (this->deviceID == deviceID) {
      return false;
    }
    Close();
  }
  this->deviceID = deviceID;
  return midiOutOpen(&this->handle, deviceID,
                     (DWORD_PTR)g_Supervisor.hwndGameWindow, 0,
                     CALLBACK_WINDOW) != MMSYSERR_NOERROR;
}

// FUNCTION: TH07 0x004361c0
ZunResult MidiDevice::Close()

{
  if (this->handle == NULL) {
    return ZUN_ERROR;
  } else {
    midiOutReset(this->handle);
    midiOutClose(this->handle);
    this->handle = NULL;
    return ZUN_SUCCESS;
  }
}

// FUNCTION: TH07 0x00436200
i32 MidiDevice::SendLongMsg(LPMIDIHDR pmh)

{
  if (this->handle == NULL) {
    return 0;
  } else {
    if (midiOutPrepareHeader(this->handle, pmh, sizeof(MIDIHDR)) !=
        MMSYSERR_NOERROR) {
      return 1;
    }

    return midiOutLongMsg(this->handle, pmh, 0x40) != 0;
  }
}

union MidiShortMsg {
  struct {
    u8 midiStatus;
    i8 firstByte;
    i8 secondByte;
    i8 unused;
  } msg;
  u32 dwMsg;
};

// FUNCTION: TH07 0x00436250
i32 MidiDevice::SendShortMsg(u8 midiStatus, u8 firstByte, u8 secondByte)

{
  MidiShortMsg pkt;

  if (this->handle == NULL) {
    return false;
  } else {
    pkt.msg.midiStatus = midiStatus;
    pkt.msg.firstByte = firstByte;
    pkt.msg.secondByte = secondByte;
    return midiOutShortMsg(this->handle, pkt.dwMsg) != MMSYSERR_NOERROR;
  }
}

// FUNCTION: TH07 0x004362a0
MidiTimer::MidiTimer()

{
  timeGetDevCaps(&this->timeCaps, 8);
  this->timerId = 0;
}

// FUNCTION: TH07 0x004362d0
MidiTimer::~MidiTimer()

{
  StopTimer();
  timeEndPeriod(this->timeCaps.wPeriodMin);
}

// FUNCTION: TH07 0x00436300
u32 MidiTimer::StartTimer(u32 delay, LPTIMECALLBACK cb, DWORD_PTR data)

{
  MMRESULT timerId;

  StopTimer();
  timeBeginPeriod(this->timeCaps.wPeriodMin);
  if (cb == NULL) {
    timerId =
        timeSetEvent(delay, this->timeCaps.wPeriodMin, DefaultTimerCallback,
                     (DWORD_PTR)this, TIME_PERIODIC);
    this->timerId = timerId;
  } else {
    timerId =
        timeSetEvent(delay, this->timeCaps.wPeriodMin, cb, data, TIME_PERIODIC);
    this->timerId = timerId;
  }
  return this->timerId;
}

// FUNCTION: TH07 0x00436380
i32 MidiTimer::StopTimer()

{
  if (this->timerId != 0) {
    timeKillEvent(this->timerId);
  }
  timeEndPeriod(this->timeCaps.wPeriodMin);
  this->timerId = 0;
  return 1;
}

// FUNCTION: TH07 0x004363c0
void CALLBACK MidiTimer::DefaultTimerCallback(u32 delay, u32 wPeriodMin,
                                              DWORD_PTR dwUser, DWORD_PTR dw1,
                                              DWORD_PTR dw2)

{
  MidiTimer *timer = (MidiTimer *)dwUser;

  timer->OnTimerElapsed();
}

// FUNCTION: TH07 0x004363e0
u16 MidiOutput::Ntohs(u16 x)

{
  u8 high = (u8)(x >> 8);
  u8 low = (u8)x;

  return ((u16)low << 8) | high;
}

// FUNCTION: TH07 0x00436400
i32 MidiOutput::SkipVariableLength(u8 **curTrackDataCursor)

{
  u8 tmp;

  i32 length = 0;
  do {
    tmp = **curTrackDataCursor;
    *curTrackDataCursor = *curTrackDataCursor + 1;
    length = length * 0x80 + (tmp & 0x7f);
  } while ((tmp & 0x80) != 0);
  return length;
}

// FUNCTION: TH07 0x00436450
MidiOutput::MidiOutput()

{
  i32 local_18;
  i32 local_14;

  this->tracks = NULL;
  this->divisions = 0;
  this->tempo = 0;
  this->numTracks = 0;
  this->pitchTranspose = 0;
  this->fadeOutVolumeMultiplier = 0.0f;
  this->fadeOutLastSetVolume = 0;
  this->unused_2d0 = 0;
  this->disableFadeOut = 0;
  this->unused_2d8 = 0;
  this->fadeOutState = 0;
  this->fadeOutFlag = 0;
  for (local_14 = 0; local_14 < 0x20; local_14 = local_14 + 1) {
    this->midiFileData[local_14] = NULL;
  }
  for (local_18 = 0; local_18 < 0x20; local_18 = local_18 + 1) {
    this->midiHeaders[local_18] = NULL;
  }
  this->fileIdx = -1;
  this->midiHeadersCursor = 0;
}

// FUNCTION: TH07 0x004365b0
MidiOutput::~MidiOutput()

{
  StopPlayback();
  ClearTracks();
  for (i32 i = 0; i < 0x20; i = i + 1) {
    ReleaseFileData(i);
  }
}

// FUNCTION: TH07 0x00436650
ZunResult MidiOutput::ReadFileData(i32 fileIdx, const char *path)

{
  if (this->fileIdx == fileIdx) {
    StopPlayback();
  }
  ReleaseFileData(fileIdx);
  this->midiFileData[fileIdx] = FileSystem::OpenFile(path, 0);
  if (this->midiFileData[fileIdx] == NULL) {
    // STRING: TH07 0x004972d0
    g_GameErrorContext.Log("error : MIDI File ‚ª“Ç‚Ýž‚ß‚È‚¢ %s \rv\r\n", path);
    return ZUN_ERROR;
  } else {
    return ZUN_SUCCESS;
  }
}

// FUNCTION: TH07 0x004366c0
void MidiOutput::ReleaseFileData(u32 idx)

{
  free(this->midiFileData[idx]);
  this->midiFileData[idx] = NULL;
}

// FUNCTION: TH07 0x00436700
void MidiOutput::ClearTracks()

{
  i32 i;

  for (i = 0; i < this->numTracks; i = i + 1) {
    free(this->tracks[i].trackData);
  }
  free(this->tracks);
  this->tracks = NULL;
  this->numTracks = 0;
}

// FUNCTION: TH07 0x00436790
ZunResult MidiOutput::ParseFile(i32 fileIdx)

{
  u32 trackLength;
  u8 *currentCursor;
  i32 i;
  MidiHeader *fileData;
  u32 hdrLength;

  ClearTracks();
  fileData = (MidiHeader *)this->midiFileData[fileIdx];
  if (fileData == NULL) {
    // STRING: TH07 0x00497290
    DebugPrint("error : ‚Ü‚¾MIDI‚ª“Ç‚Ýž‚Ü‚ê‚Ä‚¢‚È‚¢‚Ì‚ÉÄ¶‚µ‚æ‚¤‚Æ‚µ‚Ä‚¢‚é\r\n");
    return ZUN_ERROR;
  } else {
    hdrLength = Ntohl(fileData->length);
    currentCursor = ((u8 *)fileData + 8 + hdrLength);
    this->format = Ntohs(fileData->format);
    this->divisions = Ntohs(fileData->divisions);
    this->numTracks = Ntohs(fileData->numTracks);
    this->tracks = (MidiTrack *)malloc(this->numTracks << 5);
    memset(this->tracks, 0, this->numTracks * sizeof(MidiTrack));
    for (i = 0; i < this->numTracks; i += 1) {
      trackLength = Ntohl(*(u32 *)(currentCursor + 4));
      this->tracks[i].trackLength = trackLength;
      this->tracks[i].trackData = (u8 *)malloc(trackLength);
      this->tracks[i].trackPlaying = 1;
      memcpy(this->tracks[i].trackData, currentCursor + 8, trackLength);
      currentCursor = currentCursor + 8 + trackLength;
    }
    this->tempo = 1000000;
    this->fileIdx = fileIdx;
    // STRING: TH07 0x00497280
    DebugPrint(" midi open %d\n", fileIdx);
    return ZUN_SUCCESS;
  }
}

// FUNCTION: TH07 0x004369c0
ZunResult MidiOutput::LoadFile(const char *path)

{
  if (ReadFileData(0x1f, path) == ZUN_SUCCESS) {
    ParseFile(0x1f);
    ReleaseFileData(0x1f);
    return ZUN_SUCCESS;
  } else {
    return ZUN_ERROR;
  }
}

// FUNCTION: TH07 0x00436a00
void MidiOutput::LoadTracks()

{
  MidiTrack *track;

  track = this->tracks;
  this->fadeOutVolumeMultiplier = 1.0f;
  this->fadeOutState = 0;
  this->fadeOutFlag = 0;
  this->volume = 0;
  this->field_0x130 = 0;
  for (i32 i = 0; i < this->numTracks; i = i + 1) {
    track->curTrackDataCursor = track->trackData;
    track->savedTrackDataCursor = track->curTrackDataCursor;
    track->trackPlaying = 1;
    track->trackLengthOther = SkipVariableLength(&track->curTrackDataCursor);
    track = track + 1;
  }
}

// FUNCTION: TH07 0x00436ad0
ZunResult MidiOutput::Play()

{
  if (this->tracks == NULL) {
    return ZUN_ERROR;
  } else {
    LoadTracks();
    this->midiOutDev.OpenDevice(0xffffffff);
    StartTimer(1, NULL, 0);
    // STRING: TH07 0x00497274
    DebugPrint(" midi play\n");
    return ZUN_SUCCESS;
  }
}

// FUNCTION: TH07 0x00436b30
ZunResult MidiOutput::StopPlayback()

{
  if (this->tracks == NULL) {
    return ZUN_ERROR;
  } else {
    for (i32 i = 0; i < 0x20; i = i + 1) {
      if (this->midiHeaders[this->midiHeadersCursor] != NULL) {
        UnprepareHeader(this->midiHeaders[this->midiHeadersCursor]);
      }
    }
    StopTimer();
    this->midiOutDev.Close();
    this->fileIdx = -1;
    return ZUN_SUCCESS;
  }
}

// FUNCTION: TH07 0x00436bc0
ZunResult MidiOutput::UnprepareHeader(LPMIDIHDR pmh)

{
  if (pmh == NULL) {
    // STRING: TH07 0x00497268
    DebugPrint("error :\r\n");
  }
  if (this->midiOutDev.handle == NULL) {
    DebugPrint("error :\r\n");
  }
  i32 i = 0;
  while (true) {
    if (0x1f < i) {
      return ZUN_ERROR;
    }
    if (this->midiHeaders[i] == pmh)
      break;
    i += 1;
  }
  this->midiHeaders[i] = NULL;
  if (midiOutUnprepareHeader(this->midiOutDev.handle, pmh, 0x40) != 0) {
    DebugPrint("error :\r\n");
  }
  free(pmh->lpData);
  free(pmh);
  return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00436c90
ZunResult MidiOutput::SetFadeOut(i32 interval)

{
  this->fadeOutVolumeMultiplier = 0.0f;
  this->fadeOutInterval = interval;
  this->fadeOutElapsedMs = 0;
  this->fadeOutState = 0;
  this->fadeOutFlag = 1;
  return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00436ce0
void MidiOutput::OnTimerElapsed()

{
  u64 local_14;
  i32 trackIndex;
  bool trackLoaded;

  trackLoaded = false;

  local_14 =
      this->field_0x130 + (this->volume * this->divisions * 1000) / this->tempo;
  if (this->fadeOutFlag != 0) {
    if (this->fadeOutInterval <= this->fadeOutElapsedMs) {
      this->fadeOutVolumeMultiplier = 0.0;
      return;
    }
    this->fadeOutVolumeMultiplier =
        1.0 - (f32)this->fadeOutElapsedMs / (f32)this->fadeOutInterval;
    if (this->fadeOutVolumeMultiplier * 128.0 != this->fadeOutLastSetVolume) {
      FadeOutSetVolume(0);
    }
    this->fadeOutLastSetVolume = this->fadeOutVolumeMultiplier * 128.0;
    this->fadeOutElapsedMs = this->fadeOutElapsedMs + 1;
  }
  trackIndex = 0;
  while (true) {
    if (this->numTracks <= trackIndex) {
      this->volume += 1;
      if (!trackLoaded) {
        LoadTracks();
      }
      return;
    }
    if (this->tracks[trackIndex].trackPlaying != 0) {
      trackLoaded = true;
      while (this->tracks[trackIndex].trackPlaying != 0) {
        if (this->tracks[trackIndex].trackLengthOther <= local_14) {
          ProcessMsg(&this->tracks[trackIndex]);
          local_14 = this->field_0x130 +
                     (this->volume * this->divisions * 1000) / this->tempo;
          continue;
        }
        break;
      }
    }
    trackIndex += 1;
  }
}

// FUNCTION: TH07 0x00436f50
void MidiOutput::ProcessMsg(MidiTrack *track)

{
  u8 uVar1;
  i32 iVar4;
  u8 MVar5;
  MidiTrack *local_30;
  MidiTrack *local_2c;
  u8 arg1;
  u8 opcode;
  i32 iStack_14;
  u8 arg2;
  i32 i;
  LPMIDIHDR pmh;

  opcode = *track->curTrackDataCursor;
  if (opcode < OPCODE_NOTE_OFF) {
    opcode = track->opcode;
  } else {
    track->curTrackDataCursor = track->curTrackDataCursor + 1;
  }
  MVar5 = opcode & OPCODE_CHANNEL_F;
  switch (opcode & OPCODE_SYSTEM_EXCLUSIVE) {
  case OPCODE_NOTE_OFF:
  case OPCODE_NOTE_ON:
  case OPCODE_POLYPHONIC_AFTERTOUCH:
  case OPCODE_MODE_CHANGE:
  case OPCODE_PITCH_BEND_CHANGE:
    arg1 = *track->curTrackDataCursor;
    track->curTrackDataCursor = track->curTrackDataCursor + 1;
    arg2 = *track->curTrackDataCursor;
    track->curTrackDataCursor = track->curTrackDataCursor + 1;
    break;
  case OPCODE_PROGRAM_CHANGE:
  case OPCODE_CHANNEL_AFTERTOUCH:
    arg1 = *track->curTrackDataCursor;
    track->curTrackDataCursor = track->curTrackDataCursor + 1;
    arg2 = 0;
    break;
  case OPCODE_SYSTEM_EXCLUSIVE:
    if (opcode == OPCODE_SYSTEM_EXCLUSIVE) {
      if (this->midiHeaders[this->midiHeadersCursor] != NULL) {
        UnprepareHeader(this->midiHeaders[this->midiHeadersCursor]);
      }
      this->midiHeaders[this->midiHeadersCursor] = (MIDIHDR *)malloc(0x40);
      pmh = this->midiHeaders[this->midiHeadersCursor];
      iVar4 = SkipVariableLength(&track->curTrackDataCursor);
      memset(pmh, 0, sizeof(MIDIHDR));
      pmh->lpData = (LPSTR)malloc(iVar4 + 1);
      *pmh->lpData = -0x10;
      pmh->dwFlags = 0;
      pmh->dwBufferLength = iVar4 + 1;
      for (i = 0; i < iVar4; i += 1) {
        pmh->lpData[i + 1] = *track->curTrackDataCursor;
        track->curTrackDataCursor = track->curTrackDataCursor + 1;
      }
      if (this->midiOutDev.SendLongMsg(pmh) != 0) {
        free(pmh->lpData);
        free(pmh);
        this->midiHeaders[this->midiHeadersCursor] = NULL;
      }
      this->midiHeadersCursor = this->midiHeadersCursor + 1;
      this->midiHeadersCursor = this->midiHeadersCursor % 32;
    } else if (opcode == OPCODE_SYSTEM_RESET) {
      uVar1 = *track->curTrackDataCursor;
      track->curTrackDataCursor = track->curTrackDataCursor + 1;
      iVar4 = SkipVariableLength(&track->curTrackDataCursor);
      if (uVar1 == 0x2f) {
        track->trackPlaying = 0;
        return;
      }
      if (uVar1 == 0x51) {
        this->field_0x130 +=
            (this->volume * this->divisions * 1000) / this->tempo;
        this->volume = 0;
        this->tempo = 0;
        for (i = 0; i < iVar4; i += 1) {
          this->tempo += this->tempo * 0x100 + (u32)*track->curTrackDataCursor;
          track->curTrackDataCursor = track->curTrackDataCursor + 1;
        }
      } else {
        track->curTrackDataCursor = track->curTrackDataCursor + iVar4;
      }
    }
  }
  switch (opcode & OPCODE_SYSTEM_EXCLUSIVE) {
  case OPCODE_NOTE_ON:
    if (arg2 != 0) {
      arg1 += this->pitchTranspose;
      this->channels[MVar5].keyPressedFlags[(i32)(u32)arg1 >> 3] =
          this->channels[MVar5].keyPressedFlags[(i32)(u32)arg1 >> 3] |
          (u8)(1 << (arg1 & 7));
      break;
    }
  case OPCODE_NOTE_OFF:
    arg1 += this->pitchTranspose;
    this->channels[MVar5].keyPressedFlags[(i32)(u32)arg1 >> 3] =
        this->channels[MVar5].keyPressedFlags[(i32)(u32)arg1 >> 3] &
        ~(u8)(1 << (arg1 & 7));
    break;
  case OPCODE_MODE_CHANGE:
    switch (arg1) {
    case 0:
      this->channels[MVar5].instrumentBank = arg2;
      break;
    case 2:
      local_2c = this->tracks;
      for (i = 0; i < this->numTracks; i += 1) {
        local_2c->savedTrackDataCursor = local_2c->curTrackDataCursor;
        local_2c->savedTrackLengthOther = local_2c->trackLengthOther;
        local_2c = local_2c + 1;
      }
      this->savedTempo = this->tempo;
      this->savedVolume = this->volume;
      this->savedfield_0x130 = this->field_0x130;
      break;
    case 4:
      local_30 = this->tracks;
      for (i = 0; i < this->numTracks; i += 1) {
        local_30->curTrackDataCursor = local_30->savedTrackDataCursor;
        local_30->trackLengthOther = local_30->savedTrackLengthOther;
        local_30 = local_30 + 1;
      }
      this->tempo = this->savedTempo;
      this->volume = this->savedVolume;
      this->field_0x130 = this->savedfield_0x130;
      break;
    case 7:
      this->channels[MVar5].channelVolume = arg2;
      iStack_14 = (f32)arg2 * this->fadeOutVolumeMultiplier;
      if (iStack_14 < 0) {
        iStack_14 = 0;
      } else if (0x7f < iStack_14) {
        iStack_14 = 0x7f;
      }
      this->channels[MVar5].modifiedVolume = (u8)iStack_14;
      arg2 = (u8)iStack_14;
      break;
    case 10:
      this->channels[MVar5].pan = arg2;
      break;
    case 0x5b:
      this->channels[MVar5].effectOneDepth = arg2;
      break;
    case 0x5d:
      this->channels[MVar5].effectThreeDepth = arg2;
    }
    break;
  case OPCODE_PROGRAM_CHANGE:
    this->channels[MVar5].instrument = arg1;
  }
  if (opcode < OPCODE_SYSTEM_EXCLUSIVE) {
    this->midiOutDev.SendShortMsg(opcode, arg1, arg2);
  }
  track->opcode = opcode;
  track->trackLengthOther =
      track->trackLengthOther + SkipVariableLength(&track->curTrackDataCursor);
}

// FUNCTION: TH07 0x004377f0
void MidiOutput::FadeOutSetVolume(i32 vol)

{
  i32 local_18;
  u8 local_10;

  if (this->disableFadeOut == 0) {
    for (i32 i = 0; i < 0x10; i = i + 1) {
      local_18 = (i32)((f32)this->channels[i].channelVolume *
                       this->fadeOutVolumeMultiplier) +
                 vol;
      if (local_18 < 0) {
        local_18 = 0;
      } else if (0x7f < local_18) {
        local_18 = 0x7f;
      }
      local_10 = (u8)local_18;
      this->midiOutDev.SendShortMsg((u8)i + 0xb0, 7, local_10);
    }
  }
}

void MidiTimer::OnTimerElapsed() { UpdatePerfCounter(); }

// FUNCTION: TH07 0x004378b0
void MidiTimer::UpdatePerfCounter() { QueryPerformanceCounter(&g_PerfCounter); }

// FUNCTION: TH07 0x004378d0
void MidiTimer::StartTimerDefault() { StartTimer(6, NULL, 0); }

// FUNCTION: TH07 0x004378f0
void Supervisor::StopMidiTimer(MidiTimer *timer) { timer->StopTimer(); }
