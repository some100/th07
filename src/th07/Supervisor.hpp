#pragma once

#include <basetsd.h>
#include <d3d8.h>
#include <d3dx8math.h>
#include <dinput.h>

#include "MidiOutput.hpp"
#include "inttypes.hpp"

extern u16 g_CurFrameRawInput;
extern u16 g_CurFrameGameInput;
extern u16 g_LastFrameRawInput;
extern u16 g_LastFrameGameInput;
extern u16 g_IsEighthFrameOfHeldInput;
extern u16 g_NumOfFramesInputsWereHeld;

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
extern ControllerMapping g_ControllerMapping;

enum MusicMode
{
    MUSIC_OFF,
    MUSIC_WAV,
    MUSIC_MIDI,
};

enum Difficulty
{
    DIFF_EASY,
    DIFF_NORMAL,
    DIFF_HARD,
    DIFF_LUNATIC,
    DIFF_EXTRA,
    DIFF_PHANTASM,
    DIFF_COUNT,
};

enum EffectQuality
{
    QUALITY_WORST,
    QUALITY_MEDIUM,
    QUALITY_BEAUTIFUL,
};

enum SupervisorState
{
    SUPERVISOR_STATE_EXIT = -1,
    SUPERVISOR_STATE_INIT,
    SUPERVISOR_STATE_MAINMENU,
    SUPERVISOR_STATE_GAMEMANAGER,
    SUPERVISOR_STATE_NEXT_STAGE,
    SUPERVISOR_STATE_EXIT_ERROR,
    SUPERVISOR_STATE_RESULTSCREEN,
    SUPERVISOR_STATE_RESULTSCREEN_FROM_GAME,
    SUPERVISOR_STATE_REPLAY_END,
    SUPERVISOR_STATE_MUSICROOM,
    SUPERVISOR_STATE_ENDING,
    SUPERVISOR_STATE_RESTART_FROM_BEGINNING,
    SUPERVISOR_STATE_RESTART_STAGE,
    SUPERVISOR_STATE_NEXT_STAGE_USELESS,
};

struct GameConfiguration
{
    ControllerMapping controllerMapping;
    // pad 2
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
            u32 colorAddEmulation : 1;
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
            u32 disableVsync : 1;
        };
    };
};
C_ASSERT(sizeof(GameConfiguration) == 0x38);

#pragma pack(4)
struct Supervisor
{
    static void DebugPrint2(const char *fmt, ...);

    static ZunResult RegisterChain();

    static ZunResult AddedCallback(Supervisor *arg);
    static ZunResult DeletedCallback(Supervisor *arg);
    static u32 OnUpdate(Supervisor *arg);
    static u32 OnDraw(Supervisor *arg);

    ZunResult CheckIntegrity(const char *version, i32 exeSize, i32 exeChecksum);
    void CheckTiming();
    static i32 CheckVSync();
    static void DrawFpsCounter(i32 param_1);
    i32 FadeOutMusic(f32 musicFadeFrames);
    static void StopMidiTimer(MidiTimer *timer);
    HRESULT DisableFog();
    HRESULT EnableFog();
    i32 LoadAudio(i32 idx, const char *path);
    ZunResult LoadConfig(const char *configFilename);
    static ZunResult LoadGameData();
    ZunResult PlayAudio(const char *path);
    ZunResult PlayLoadedAudio(i32 idx);
    void SetRenderState(D3DRENDERSTATETYPE stateType, DWORD param_2);
    ZunResult SetupDInput();
    i32 SnapshotScreen(const char *filename);
    ZunResult StopAudio();
    void TickTimer(i32 *frames, f32 *subFrames);
    void UpdateStartupTime();
    void UpdateTime();

    i32 IsSlowMode();

    static i32 __stdcall ControllerCallback(LPCDIDEVICEOBJECTINSTANCE param_1,
                                            void *param_2);
    static i32 __stdcall EnumGameControllersCb(LPCDIDEVICEINSTANCEA param_1,
                                               void *param_2);

    void InitializeTimingVars()
    {
        this->timingErrorCount = 0;
        this->maxTimingError = 0;
        this->checkTiming = 0;
        this->timingSpikeAccumulator = 0;
        this->timingBadCount = 0;
    }

    i32 IsSoftwareTexturing()
    {
        return this->cfg.disableTextureBlend | this->cfg.colorAddEmulation;
    }

    i32 IsClearingBackbuffer()
    {
        return this->cfg.forceBackBufferClear | this->cfg.disableItemDrawAroundPlayfield;
    }

    i32 VsyncDisabled()
    {
        return this->vsyncDisabled;
    }

    HINSTANCE hInstance;
    IDirect3D8 *d3dIface;
    LPDIRECT3DDEVICE8 d3dDevice;
    LPDIRECTINPUT8A directInput;
    LPDIRECTINPUTDEVICE8A keyboard;
    LPDIRECTINPUTDEVICE8A controller;
    DIDEVCAPS controllerCaps;
    HWND hwndGameWindow;
    D3DXMATRIX viewMatrix;
    D3DXMATRIX projectionMatrix;
    D3DVIEWPORT8 viewport;
    D3DPRESENT_PARAMETERS presentParameters;
    DummyMidiTimer *midiTimer;
    GameConfiguration cfg;
    i32 calcCount;
    i32 wantedState;
    i32 curState;
    i32 prevState;
    i32 unused_160;
    i32 renderSkipFrames;
    i32 isInEnding;
    i32 vsyncDisabled;
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
    DWORD lastTotalPlayTimeUpdate;
    DWORD currentTime;
    D3DCAPS8 d3dCaps;
    LARGE_INTEGER perfFrequency;
    LARGE_INTEGER prevPerfCounter;
    LARGE_INTEGER curPerfCounter;
    SYSTEMTIME prevTime;
    SYSTEMTIME curTime;
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
C_ASSERT(sizeof(Supervisor) == 0x2d0);
extern Supervisor g_Supervisor;

#define NUKE_SUPERVISOR() memset(&g_Supervisor, -1, sizeof(g_Supervisor))
