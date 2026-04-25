#pragma once

#include <windows.h>

#include "ZunResult.hpp"
#include "inttypes.hpp"

struct MidiHeader
{
    i8 magic[4];
    u32 length;
    u16 format;
    u16 numTracks;
    u16 divisions;
};

// VTABLE: TH07 0x00497260
struct MidiTimer
{
    MidiTimer();
    ~MidiTimer();

    u32 StartTimer(u32 delay, LPTIMECALLBACK cb, DWORD_PTR data);
    void StartTimerDefault();
    i32 StopTimer();
    void UpdatePerfCounter();

    virtual void OnTimerElapsed() {}
    static void CALLBACK DefaultTimerCallback(u32 delay, u32 wPeriodMin,
                                              DWORD_PTR dwUser, DWORD_PTR dw1,
                                              DWORD_PTR dw2);

    u32 timerId;
    TIMECAPS timeCaps;
    i32 fileIdx;
};

typedef enum MidiOpcode
{
    OPCODE_CHANNEL_1 = 1,
    OPCODE_CHANNEL_2 = 2,
    OPCODE_CHANNEL_3 = 3,
    OPCODE_CHANNEL_4 = 4,
    OPCODE_CHANNEL_5 = 5,
    OPCODE_CHANNEL_6 = 6,
    OPCODE_CHANNEL_7 = 7,
    OPCODE_CHANNEL_8 = 8,
    OPCODE_CHANNEL_9 = 9,
    OPCODE_CHANNEL_A = 10,
    OPCODE_CHANNEL_B = 11,
    OPCODE_CHANNEL_C = 12,
    OPCODE_CHANNEL_D = 13,
    OPCODE_CHANNEL_E = 14,
    OPCODE_CHANNEL_F = 15,
    OPCODE_NOTE_OFF = 128,
    OPCODE_NOTE_ON = 144,
    OPCODE_POLYPHONIC_AFTERTOUCH = 160,
    OPCODE_MODE_CHANGE = 176,
    OPCODE_PROGRAM_CHANGE = 192,
    OPCODE_CHANNEL_AFTERTOUCH = 208,
    OPCODE_PITCH_BEND_CHANGE = 224,
    OPCODE_SYSTEM_EXCLUSIVE = 240,
    OPCODE_MIDI_TIME_CODE_QTR_FRAME = 241,
    OPCODE_SONG_POSITION_POINTER = 242,
    OPCODE_SONG_SELECT = 243,
    OPCODE_RESERVED_F4 = 244,
    OPCODE_RESERVED_F5 = 245,
    OPCODE_TUNE_REQUEST = 246,
    OPCODE_END_OF_SYSEX = 247,
    OPCODE_TIMING_CLOCK = 248,
    OPCODE_RESERVED_F9 = 249,
    OPCODE_START = 250,
    OPCODE_CONTINUE = 251,
    OPCODE_STOP = 252,
    OPCODE_RESERVED_FD = 253,
    OPCODE_ACTIVE_SENSING = 254,
    OPCODE_SYSTEM_RESET = 255
} MidiOpcode;

struct MidiTrack
{
    u32 trackPlaying;
    u32 trackLengthOther;
    u32 trackLength;
    u8 opcode;
    // pad 3
    u8 *trackData;
    u8 *curTrackDataCursor;
    u8 *savedTrackDataCursor;
    u32 savedTrackLengthOther;
};
C_ASSERT(sizeof(MidiTrack) == 0x20);

struct MidiDevice
{
    MidiDevice();
    ~MidiDevice();

    ZunResult Close();
    u32 OpenDevice(i32 deviceID);
    i32 SendLongMsg(LPMIDIHDR pmh);
    i32 SendShortMsg(u8 midiStatus, u8 firstByte, u8 secondByte);

    HMIDIOUT handle;
    i32 deviceID;
};

struct MidiChannel
{
    u8 keyPressedFlags[16];
    u8 instrument;
    u8 instrumentBank;
    u8 pan;
    u8 effectOneDepth;
    u8 effectThreeDepth;
    u8 channelVolume;
    u8 modifiedVolume;
};

// VTABLE: TH07 0x00497264
struct MidiOutput : MidiTimer
{
    MidiOutput();
    ~MidiOutput();

    void ClearTracks();
    void FadeOutSetVolume(i32 vol);
    ZunResult LoadFile(const char *path);
    void LoadTracks();
    static u16 Ntohs(u16 x);
    void OnTimerElapsed();
    ZunResult ParseFile(i32 fileIdx);
    ZunResult Play();
    void ProcessMsg(MidiTrack *track);
    ZunResult ReadFileData(i32 fileIdx, const char *path);
    void ReleaseFileData(u32 idx);
    ZunResult SetFadeOut(i32 interval);
    static u32 SkipVariableLength(u8 **curTrackDataCursor);
    ZunResult StopPlayback();
    ZunResult UnprepareHeader(LPMIDIHDR pmh);

    static u32 Ntohl(u32 x)
    {
        u8 tmp[4];

        tmp[0] = ((u8 *)&x)[3];
        tmp[1] = ((u8 *)&x)[2];
        tmp[2] = ((u8 *)&x)[1];
        tmp[3] = ((u8 *)&x)[0];

        return *(const u32 *)tmp;
    }

    __forceinline void Play(const char *path)
    {
        this->StopPlayback();
        this->LoadFile(path);
        this->Play();
    }

    __forceinline void PlayLoaded(i32 idx)
    {
        this->StopPlayback();
        this->ParseFile(idx);
        this->Play();
    }

    MIDIHDR *midiHeaders[32];
    i32 midiHeadersCursor;
    u8 *midiFileData[32];
    i32 numTracks;
    u32 format;
    i32 divisions;
    i32 tempo;
    u64 volume;
    u64 field_0x130;
    MidiTrack *tracks;
    MidiDevice midiOutDev;
    u8 unused_144[16];
    MidiChannel channels[16];
    u8 pitchTranspose;
    // pad 3
    f32 fadeOutVolumeMultiplier;
    i32 fadeOutLastSetVolume;
    i32 unused_2d0;
    i32 disableFadeOut;
    i32 unused_2d8;
    i32 fadeOutState;
    i32 fadeOutFlag;
    i32 fadeOutInterval;
    i32 fadeOutElapsedMs;
    i32 savedTempo;
    u64 savedVolume;
    u64 savedfield_0x130;
};
C_ASSERT(sizeof(MidiOutput) == 0x300);

// VTABLE: TH07 0x00496c0c
struct DummyMidiTimer : MidiTimer
{
    virtual void OnTimerElapsed();
};

extern LARGE_INTEGER g_PerfCounter;
