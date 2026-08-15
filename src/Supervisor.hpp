#pragma once

#include "MidiOutput.hpp"
#include "ZunMath.hpp"
#include "graphics/ZunGraphics.hpp"
#include "inttypes.hpp"
#include <SDL3/SDL_gamepad.h>
#include <chrono>

extern u16 g_CurFrameRawInput;
extern u16 g_CurFrameGameInput;
extern u16 g_LastFrameRawInput;
extern u16 g_LastFrameGameInput;
extern u16 g_IsEighthFrameOfHeldInput;
extern u16 g_NumOfFramesInputsWereHeld;

typedef enum MusicMode
{
    MUSIC_OFF = 0,
    MUSIC_WAV = 1,
    MUSIC_MIDI = 2
} MusicMode;

typedef enum Difficulty
{
    DIFF_EASY = 0,
    DIFF_NORMAL = 1,
    DIFF_HARD = 2,
    DIFF_LUNATIC = 3,
    DIFF_EXTRA = 4,
    DIFF_PHANTASM = 5
} Difficulty;

typedef enum EffectQuality
{
    QUALITY_WORST = 0,
    QUALITY_MEDIUM = 1,
    QUALITY_BEAUTIFUL = 2
} EffectQuality;

struct ControllerMapping
{
    i16 shootButton;
    i16 bombButton;
    i16 focusButton;
    i16 menuButton;
    i16 upButton;
    i16 downButton;
    i16 leftButton;
    i16 rightButton;
    i16 skipButton;
};
static_assert(sizeof(ControllerMapping) == 0x12);
extern ControllerMapping g_ControllerMapping;

struct GameConfiguration
{
    ControllerMapping controllerMapping;
    u8 pad[2];
    i32 version;
    i16 padAxisX;
    i16 padAxisY;
    u8 lifeCount;
    u8 bombCount;
    u8 colorMode16bit;
    u8 musicMode;
    u8 playSounds;
    u8 defaultDifficulty;
    u8 windowed;
    u8 frameskipConfig;
    u8 effectQuality;
    u8 slowMode;
    u8 shotSlow;
    u8 unused_27[13];
    union {
        u32 opts;
        struct
        {
            u32 loaded : 1;
            u32 noVertexBuffers : 1;
            u32 use16BitTextures : 1;
            u32 forceBackBufferClear : 1;
            u32 disableItemDrawAroundPlayfield : 1;
            u32 disableGouraud : 1;
            u32 disableZBuffer : 1;
            u32 unused : 1;
            u32 disableTextureBlend : 1;
            u32 forceReferenceRender : 1;
            u32 disableFog : 1;
            u32 disableDinput : 1;
            u32 redrawEveryFrame : 1;
            u32 preloadBgm : 1;
            u32 enableVsync : 1;
        };
    };
};
static_assert(sizeof(GameConfiguration) == 0x38);

struct Supervisor
{
    static void DebugPrint(const char *fmt, ...);

    static ZunResult RegisterChain();

    static ZunResult AddedCallback(Supervisor *arg);
    static ZunResult DeletedCallback(Supervisor *arg);
    static u32 OnUpdate(Supervisor *arg);
    static u32 OnDraw(Supervisor *arg);

    void CheckTiming();
    static i32 CheckVSync();
    static void DrawFpsCounter(i32 param_1);
    i32 FadeOutMusic(f32 musicFadeFrames);
    static void StopMidiTimer(MidiTimer *timer);
    i32 DisableFog();
    i32 EnableFog();
    i32 LoadAudio(i32 idx, const char *path);
    ZunResult LoadConfig(const char *configFilename);
    static ZunResult LoadGameData();
    ZunResult PlayAudio(const char *path);
    ZunResult PlayLoadedAudio(i32 idx);
    ZunResult SetupInput();
    i32 SnapshotScreen(const char *filename);
    ZunResult StopAudio();
    void TickTimer(i32 *frames, f32 *subFrames);
    void UpdateStartupTime();
    void UpdateTime();

    i32 IsSlowMode();

    i32 IsClearingBackbuffer()
    {
        return this->cfg.forceBackBufferClear | this->cfg.disableItemDrawAroundPlayfield;
    }

    i32 VsyncEnabled()
    {
        return this->vsyncEnabled;
    }

    ZunGraphics *gfxDevice;
    SDL_Gamepad *controller;
    ZunMatrix viewMatrix;
    ZunMatrix projectionMatrix;
    ZunMatrix viewProjectionMatrix;
    ZunViewport viewport;
    DummyMidiTimer *midiTimer;
    GameConfiguration cfg;
    i32 calcCount;
    i32 wantedState;
    i32 curState;
    i32 prevState;
    i32 unused_160;
    i32 renderSkipFrames;
    i32 isInEnding;
    i32 vsyncEnabled;
    i32 lockableBackBuffer;
    u32 lastFrameTime;
    f32 effectiveFramerateMultiplier;
    MidiOutput *midiOutput;
    f32 framerateMultiplier;
    f32 fpsAccumulator;
    i16 curFps;
    i16 unused_18a;
    union {
        u32 flags;
        struct
        {
            u32 usingTnLHal : 1;
            u32 hasLockableBackbuffer : 1;
            u32 supports32BitTex : 1;
            u32 timingBad : 1;
            u32 deviceNotReset : 1;
            u32 forceIntegerTimer : 1;
        };
    };
    u64 lastTotalPlayTimeUpdate;
    u64 currentTime;
    u64 perfFrequency;
    u64 prevPerfCounter;
    u64 curPerfCounter;
    std::chrono::time_point<std::chrono::system_clock> prevTime;
    std::chrono::time_point<std::chrono::system_clock> curTime;
    i32 timingErrorCount;
    i32 isFpsBad;
    i32 maxTimingError;
    i32 timingSpikeAccumulator;
    i32 timingBadCount;
    i32 checkTiming;
    i32 fogEnabled;
    i32 exeChecksum;
    i32 exeSize;
    i32 versionTableSize;
    char *version;
};

extern Supervisor g_Supervisor;

#define NUKE_SUPERVISOR() memset(&g_Supervisor, -1, sizeof(g_Supervisor))
