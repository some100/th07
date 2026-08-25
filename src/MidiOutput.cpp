#include "MidiOutput.hpp"

#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "inttypes.hpp"
#include "utils.hpp"

// LARGE_INTEGER g_PerfCounter;

MidiDevice::MidiDevice()
{
    /*this->handle = NULL;
    this->deviceID = 0;*/
}

MidiDevice::~MidiDevice()
{
    Close();
}

u32 MidiDevice::OpenDevice(i32 deviceID)
{
    return 1;
    /*
        if (this->handle)
        {
            if (this->deviceID != deviceID)
            {
                Close();
            }
            else
            {
                return false;
            }
        }
        this->deviceID = deviceID;
        return midiOutOpen(&this->handle, deviceID, (DWORD_PTR)g_Supervisor.hwndGameWindow, 0,
                           CALLBACK_WINDOW) != MMSYSERR_NOERROR*/
    ;
}

ZunResult MidiDevice::Close()
{
    return ZUN_SUCCESS;

    /*
    if (!this->handle)
    {
        return ZUN_ERROR;
    }

    midiOutReset(this->handle);
    midiOutClose(this->handle);
    this->handle = NULL;
    return ZUN_SUCCESS;*/
}

/*i32 MidiDevice::SendLongMsg(LPMIDIHDR pmh)
{
    if (!this->handle)
    {
        return 0;
    }

    if (midiOutPrepareHeader(this->handle, pmh, sizeof(MIDIHDR)) != MMSYSERR_NOERROR)
    {
        return 1;
    }

    return midiOutLongMsg(this->handle, pmh, 0x40) != 0;
}*/

union MidiShortMsg {
    struct
    {
        u8 midiStatus;
        i8 firstByte;
        i8 secondByte;
        i8 unused;
    } msg;
    u32 dwMsg;
};

i32 MidiDevice::SendShortMsg(u8 midiStatus, u8 firstByte, u8 secondByte)
{
    return false;
    /*
    MidiShortMsg pkt;

    if (!this->handle)
    {
        return false;
    }

    pkt.msg.midiStatus = midiStatus;
    pkt.msg.firstByte = firstByte;
    pkt.msg.secondByte = secondByte;
    return midiOutShortMsg(this->handle, pkt.dwMsg) != MMSYSERR_NOERROR;*/
}

MidiTimer::MidiTimer()
{
    /*timeGetDevCaps(&this->timeCaps, 8);
        this->timerId = 0;*/
}

MidiTimer::~MidiTimer()
{
    /*StopTimer();
        timeEndPeriod(this->timeCaps.wPeriodMin);*/
}

/*u32 MidiTimer::StartTimer(u32 delay, LPTIMECALLBACK cb, DWORD_PTR data)
{
    StopTimer();
    timeBeginPeriod(this->timeCaps.wPeriodMin);
    if (cb)
    {
        this->timerId = timeSetEvent(delay, this->timeCaps.wPeriodMin, cb, data, TIME_PERIODIC);
    }
    else
    {
        this->timerId = timeSetEvent(delay, this->timeCaps.wPeriodMin, DefaultTimerCallback,
                                     (DWORD_PTR)this, TIME_PERIODIC);
    }
    return this->timerId;
}*/

i32 MidiTimer::StopTimer()
{
    return 1;

    /*if (this->timerId != 0)
    {
        timeKillEvent(this->timerId);
    }
    timeEndPeriod(this->timeCaps.wPeriodMin);
    this->timerId = 0;
    return 1;*/
}

/*void CALLBACK MidiTimer::DefaultTimerCallback(u32 delay, u32 wPeriodMin, DWORD_PTR dwUser,
                                              DWORD_PTR dw1, DWORD_PTR dw2)
{
    MidiTimer *timer = (MidiTimer *)dwUser;

    timer->OnTimerElapsed();
}*/

u16 MidiOutput::Ntohs(u16 x)
{
    u8 tmp[2];
    tmp[0] = ((u8 *)&x)[1];
    tmp[1] = ((u8 *)&x)[0];

    return *(u16 *)&tmp;
}

u32 MidiOutput::SkipVariableLength(u8 **curTrackDataCursor)
{
    u32 length;
    u8 tmp;

    length = 0;
    do
    {
        tmp = **curTrackDataCursor;
        *curTrackDataCursor = *curTrackDataCursor + 1;
        length = length * 0x80 + (tmp & 127);
    } while ((tmp & 0x80) != 0);
    return length;
}

MidiOutput::MidiOutput()
{
    /*i32 local_18;
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
    for (local_14 = 0; local_14 < ARRAY_SIZE_SIGNED(this->midiFileData); local_14 = local_14 + 1)
    {
        this->midiFileData[local_14] = NULL;
    }
    for (local_18 = 0; local_18 < ARRAY_SIZE_SIGNED(this->midiHeaders); local_18 = local_18 + 1)
    {
        this->midiHeaders[local_18] = NULL;
    }
    this->fileIdx = -1;
    this->midiHeadersCursor = 0;*/
}

MidiOutput::~MidiOutput()
{
    /*StopPlayback();
    ClearTracks();
    for (i32 i = 0; i < ARRAY_SIZE_SIGNED(this->midiFileData); i++)
    {
        ReleaseFileData(i);
    }*/
}

ZunResult MidiOutput::ReadFileData(i32 fileIdx, const char *path)
{
    return ZUN_ERROR;

    /*if (this->fileIdx == fileIdx)
    {
        StopPlayback();
    }
    ReleaseFileData(fileIdx);
    this->midiFileData[fileIdx] = FileSystem::OpenFile(path, 0);
    if (!this->midiFileData[fileIdx])
    {
        g_GameErrorContext.Log("error : MIDI File が読み込めない %s \rv\r\n", path);
        return ZUN_ERROR;
    }
    else
    {
        return ZUN_SUCCESS;
    }*/
}

void MidiOutput::ReleaseFileData(u32 idx)
{
    free(this->midiFileData[idx]);
    this->midiFileData[idx] = NULL;
}

void MidiOutput::ClearTracks()
{
    /*i32 i;

    for (i = 0; i < this->numTracks; i++)
    {
        free(this->tracks[i].trackData);
    }
    free(this->tracks);
    this->tracks = NULL;
    this->numTracks = 0;*/
}

ZunResult MidiOutput::ParseFile(i32 fileIdx)
{
    return ZUN_ERROR;

    /*
    u8 hdrRaw[8];
    i32 i;
    u8 *currentCursor;
    u8 *trackChunk;
    u8 *fileData;
    u32 hdrLength;
    u32 trackLength;
    u16 *header;

    ClearTracks();
    currentCursor = this->midiFileData[fileIdx];
    fileData = currentCursor;
    if (!currentCursor)
    {
        Supervisor::DebugPrint("error : まだMIDIが読み込まれていないのに再生しようとしている\r\n");
        return ZUN_ERROR;
    }

    memcpy(&hdrRaw, currentCursor, 8);
    currentCursor += sizeof(hdrRaw);
    hdrLength = Ntohl(*(u32 *)&hdrRaw[4]);
    header = (u16 *)currentCursor;
    currentCursor += hdrLength;

    this->format = Ntohs(header[0]);
    this->divisions = Ntohs(header[2]);
    this->numTracks = Ntohs(header[1]);
    this->tracks = (MidiTrack *)malloc(this->numTracks * sizeof(MidiTrack));
    memset(this->tracks, 0, this->numTracks * sizeof(MidiTrack));
    for (i = 0; i < this->numTracks; i++)
    {
        trackChunk = currentCursor;
        currentCursor += 8;
        trackLength = Ntohl(((u32 *)trackChunk)[1]);
        this->tracks[i].trackLength = trackLength;
        this->tracks[i].trackData = (u8 *)malloc(trackLength);
        this->tracks[i].trackPlaying = 1;
        memcpy(this->tracks[i].trackData, currentCursor, trackLength);
        currentCursor += trackLength;
    }
    this->tempo = 1000000;
    this->fileIdx = fileIdx;
    Supervisor::DebugPrint(" midi open %d\n", fileIdx);
    return ZUN_SUCCESS;*/
}

ZunResult MidiOutput::LoadFile(const char *path)
{
    return ZUN_ERROR;

    /*if (ReadFileData(31, path) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    ParseFile(31);
    ReleaseFileData(31);
    return ZUN_SUCCESS;*/
}

void MidiOutput::LoadTracks()
{
    /*MidiTrack *track;
    i32 i;

    track = this->tracks;
    this->fadeOutVolumeMultiplier = 1.0f;
    this->fadeOutState = 0;
    this->fadeOutFlag = 0;
    this->volume = 0;
    this->field_0x130 = 0;
    for (i = 0; i < this->numTracks; i++, track++)
    {
        track->curTrackDataCursor = track->trackData;
        track->savedTrackDataCursor = track->curTrackDataCursor;
        track->trackPlaying = 1;
        track->trackLengthOther = SkipVariableLength(&track->curTrackDataCursor);
    }*/
}

ZunResult MidiOutput::Play()
{
    return ZUN_ERROR;

    /*if (!this->tracks)
    {
        return ZUN_ERROR;
    }

    LoadTracks();
    this->midiOutDev.OpenDevice(-1);
    StartTimer(1, NULL, 0);
    Supervisor::DebugPrint(" midi play\n");
    return ZUN_SUCCESS;*/
}

ZunResult MidiOutput::StopPlayback()
{
    return ZUN_ERROR;

    /*if (!this->tracks)
    {
        return ZUN_ERROR;
    }

    for (i32 i = 0; i < ARRAY_SIZE_SIGNED(this->midiHeaders); i++)
    {
        if (this->midiHeaders[this->midiHeadersCursor])
        {
            UnprepareHeader(this->midiHeaders[this->midiHeadersCursor]);
        }
    }
    StopTimer();
    this->midiOutDev.Close();
    this->fileIdx = -1;
    return ZUN_SUCCESS;*/
}

ZunResult MidiOutput::UnprepareHeader(void *pmh) // LPMIDIHDR pmh
{
    return ZUN_ERROR;

    /*
    i32 i;

    if (!pmh)
    {
        Supervisor::DebugPrint("error :\r\n");
    }

    if (!this->midiOutDev.handle)
    {
        Supervisor::DebugPrint("error :\r\n");
    }

    for (i = 0; i < ARRAY_SIZE_SIGNED(this->midiHeaders); i++)
    {
        if (this->midiHeaders[i] == pmh)
        {
            this->midiHeaders[i] = NULL;
            goto success;
        }
    }
    return ZUN_ERROR;

success:
    MMRESULT res = midiOutUnprepareHeader(this->midiOutDev.handle, pmh, 0x40);
    if (res)
    {
        Supervisor::DebugPrint("error :\r\n");
    }

    free(pmh->lpData);
    free(pmh);
    return ZUN_SUCCESS;*/
}

ZunResult MidiOutput::SetFadeOut(i32 interval)
{
    this->fadeOutVolumeMultiplier = 0.0f;
    this->fadeOutInterval = interval;
    this->fadeOutElapsedMs = 0;
    this->fadeOutState = 0;
    this->fadeOutFlag = 1;
    return ZUN_SUCCESS;
}

void MidiOutput::OnTimerElapsed()
{
    /*u64 local_14;
    i32 i;
    i32 trackLoaded;

    trackLoaded = false;

    local_14 = this->field_0x130 + this->volume * this->divisions * 1000 / this->tempo;
    if (this->fadeOutFlag)
    {
        if (this->fadeOutElapsedMs < this->fadeOutInterval)
        {
            this->fadeOutVolumeMultiplier =
                1.0f - (f32)this->fadeOutElapsedMs / (f32)this->fadeOutInterval;
            if ((i32)(this->fadeOutVolumeMultiplier * 128.0f) != this->fadeOutLastSetVolume)
            {
                FadeOutSetVolume(0);
            }
            this->fadeOutLastSetVolume = (i32)(this->fadeOutVolumeMultiplier * 128.0f);
            this->fadeOutElapsedMs++;
        }
        else
        {
            this->fadeOutVolumeMultiplier = 0.0f;
            return;
        }
    }
    for (i = 0; i < this->numTracks; i++)
    {
        if (this->tracks[i].trackPlaying != 0)
        {
            trackLoaded = true;
            while (this->tracks[i].trackPlaying != 0)
            {
                if (this->tracks[i].trackLengthOther <= local_14)
                {
                    ProcessMsg(&this->tracks[i]);
                    local_14 =
                        this->field_0x130 + this->volume * this->divisions * 1000 / this->tempo;
                    continue;
                }
                break;
            }
        }
    }

    this->volume++;
    if (!trackLoaded)
    {
        LoadTracks();
    }*/
}

void MidiOutput::ProcessMsg(MidiTrack *track)
{
    /*MidiTrack *curTrack2;
    MidiTrack *curTrack1;
    i32 bpm;
    u8 metaType;
    LPMIDIHDR pmh;
    i32 curTrackLength;
    u8 arg1;
    u8 opcode;
    u8 opcodeHigh;
    u8 opcodeLow;
    i32 volumeClamped;
    u8 arg2;
    i32 i;
    i32 nextTrackLength;

    opcode = *track->curTrackDataCursor;
    if (opcode < OPCODE_NOTE_OFF)
    {
        opcode = track->opcode;
    }
    else
    {
        track->curTrackDataCursor++;
    }
    opcodeHigh = opcode & 0xf0;
    opcodeLow = opcode & 0x0f;
    switch (opcodeHigh)
    {
    case OPCODE_SYSTEM_EXCLUSIVE:
        if (opcode == OPCODE_SYSTEM_EXCLUSIVE)
        {
            if (this->midiHeaders[this->midiHeadersCursor])
            {
                UnprepareHeader(this->midiHeaders[this->midiHeadersCursor]);
            }
            pmh = this->midiHeaders[this->midiHeadersCursor] = (MIDIHDR *)malloc(sizeof(MIDIHDR));
            curTrackLength = SkipVariableLength(&track->curTrackDataCursor);
            memset(pmh, 0, sizeof(MIDIHDR));
            pmh->lpData = (LPSTR)malloc(curTrackLength + 1);
            pmh->lpData[0] = -0x10;
            pmh->dwFlags = 0;
            pmh->dwBufferLength = curTrackLength + 1;
            for (i = 0; i < curTrackLength; i++)
            {
                pmh->lpData[i + 1] = *track->curTrackDataCursor;
                track->curTrackDataCursor++;
            }
            if (this->midiOutDev.SendLongMsg(pmh))
            {
                free(pmh->lpData);
                free(pmh);
                this->midiHeaders[this->midiHeadersCursor] = NULL;
            }
            this->midiHeadersCursor++;
            this->midiHeadersCursor = this->midiHeadersCursor %
                                      ARRAY_SIZE_SIGNED(this->midiHeaders);
        }
        else if (opcode == OPCODE_SYSTEM_RESET)
        {
            metaType = *track->curTrackDataCursor;
            track->curTrackDataCursor++;
            curTrackLength = SkipVariableLength(&track->curTrackDataCursor);
            if (metaType == 0x2f)
            {
                track->trackPlaying = 0;
                return;
            }
            if (metaType == 0x51)
            {
                this->field_0x130 += this->volume * this->divisions * 1000 / this->tempo;
                this->volume = 0;
                this->tempo = 0;
                for (i = 0; i < curTrackLength; i++)
                {
                    this->tempo += this->tempo * 256 + *track->curTrackDataCursor;
                    track->curTrackDataCursor++;
                }
                bpm = 60000000 / this->tempo;
                break;
            }
            track->curTrackDataCursor = track->curTrackDataCursor + curTrackLength;
        }
        break;
    case OPCODE_NOTE_OFF:
    case OPCODE_NOTE_ON:
    case OPCODE_POLYPHONIC_AFTERTOUCH:
    case OPCODE_MODE_CHANGE:
    case OPCODE_PITCH_BEND_CHANGE:
        arg1 = *track->curTrackDataCursor;
        track->curTrackDataCursor++;
        arg2 = *track->curTrackDataCursor;
        track->curTrackDataCursor++;
        break;
    case OPCODE_PROGRAM_CHANGE:
    case OPCODE_CHANNEL_AFTERTOUCH:
        arg1 = *track->curTrackDataCursor;
        track->curTrackDataCursor++;
        arg2 = 0;
        break;
    }
    switch (opcodeHigh)
    {
    case OPCODE_NOTE_ON:
        if (arg2 != 0)
        {
            arg1 += this->pitchTranspose;
            this->channels[opcodeLow].keyPressedFlags[(i32)(u32)arg1 >> 3] |= (u8)(1 << (arg1 & 7));
            break;
        }
    case OPCODE_NOTE_OFF:
        arg1 += this->pitchTranspose;
        this->channels[opcodeLow].keyPressedFlags[(i32)(u32)arg1 >> 3] &= (u8) ~(1 << (arg1 & 7));
        break;
    case OPCODE_PROGRAM_CHANGE:
        this->channels[opcodeLow].instrument = arg1;
        break;
    case OPCODE_MODE_CHANGE:
        switch (arg1)
        {
        case 0:
            this->channels[opcodeLow].instrumentBank = arg2;
            break;
        case 7:
            this->channels[opcodeLow].channelVolume = arg2;
            volumeClamped = (i32)((f32)arg2 * this->fadeOutVolumeMultiplier);
            if (volumeClamped < 0)
            {
                volumeClamped = 0;
            }
            else if (volumeClamped > 127)
            {
                volumeClamped = 127;
            }
            this->channels[opcodeLow].modifiedVolume = (u8)volumeClamped;
            arg2 = (u8)volumeClamped;
            break;
        case 91:
            this->channels[opcodeLow].effectOneDepth = arg2;
            break;
        case 93:
            this->channels[opcodeLow].effectThreeDepth = arg2;
            break;
        case 10:
            this->channels[opcodeLow].pan = arg2;
            break;
        case 2:
            for (curTrack1 = this->tracks, i = 0; i < this->numTracks; i++, curTrack1++)
            {
                curTrack1->savedTrackDataCursor = curTrack1->curTrackDataCursor;
                curTrack1->savedTrackLengthOther = curTrack1->trackLengthOther;
            }
            this->savedTempo = this->tempo;
            this->savedVolume = this->volume;
            this->savedfield_0x130 = this->field_0x130;
            break;
        case 4:
            for (curTrack2 = this->tracks, i = 0; i < this->numTracks; i++, curTrack2++)
            {
                curTrack2->curTrackDataCursor = curTrack2->savedTrackDataCursor;
                curTrack2->trackLengthOther = curTrack2->savedTrackLengthOther;
            }
            this->tempo = this->savedTempo;
            this->volume = this->savedVolume;
            this->field_0x130 = this->savedfield_0x130;
            break;
        }
        break;
    }
    if (opcode < OPCODE_SYSTEM_EXCLUSIVE)
    {
        this->midiOutDev.SendShortMsg(opcode, arg1, arg2);
    }
    track->opcode = opcode;
    nextTrackLength = SkipVariableLength(&track->curTrackDataCursor);
    track->trackLengthOther += nextTrackLength;*/
}

void MidiOutput::FadeOutSetVolume(i32 vol)
{
    /*i32 volumeClamped;
    u32 midiStatus;
    u32 volumeByte;
    i32 i;
    i32 arg1;

    if (this->disableFadeOut)
    {
        return;
    }

    arg1 = 7;
    for (i = 0; i < ARRAY_SIZE_SIGNED(this->channels); i++)
    {
        midiStatus = (u8)(i + 0xb0);
        volumeClamped =
            (i32)(this->channels[i].channelVolume * this->fadeOutVolumeMultiplier) + vol;
        if (volumeClamped < 0)
        {
            volumeClamped = 0;
        }
        else if (volumeClamped > 127)
        {
            volumeClamped = 127;
        }
        volumeByte = (u8)volumeClamped;
        this->midiOutDev.SendShortMsg(midiStatus, arg1, volumeByte);
        }*/
}

void DummyMidiTimer::OnTimerElapsed()
{
    // QueryPerformanceCounter(&g_PerfCounter);
}

void MidiTimer::StartTimerDefault()
{
    // StartTimer(6, NULL, 0);
}

void Supervisor::StopMidiTimer(MidiTimer *timer)
{
    // timer->StopTimer();
}

void MidiTimer::OnTimerElapsed()
{
}
