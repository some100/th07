#pragma once

#include <basetsd.h>
#include <d3d8.h>
#include <d3dx8math.h>
#include <dinput.h>

#include "MidiOutput.hpp"
#include "inttypes.hpp"

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
    u32 opts;
};
C_ASSERT(sizeof(GameConfiguration) == 0x38);

#pragma pack(push, 4)
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
    static void DestroyMidiTimer(MidiTimer *timer);
    i32 FadeOutMusic(f32 musicFadeFrames);
    static void StopMidiTimer(MidiTimer *timer);
    HRESULT DisableFog();
    HRESULT EnableFog();
    static i32 LoadAudio(i32 idx, const char *path);
    ZunResult LoadConfig(const char *configFilename);
    static ZunResult LoadGameData();
    static ZunResult PlayAudio(const char *path);
    static ZunResult PlayLoadedAudio(i32 idx);
    void SetRenderState(D3DRENDERSTATETYPE stateType, DWORD param_2);
    ZunResult SetupDInput();
    i32 SnapshotScreen(const char *param_1);
    static ZunResult StopAudio();
    void TickTimer(i32 *frames, f32 *subFrames);
    void UpdateStartupTime();
    void UpdateTime();

    static i32 CanSaveReplay();

    static i32 __stdcall ControllerCallback(LPCDIDEVICEOBJECTINSTANCE param_1,
                                            void *param_2);
    static i32 __stdcall EnumGameControllersCb(LPCDIDEVICEINSTANCEA param_1,
                                               void *param_2);

    inline void Nuke()
    {
        memset(this, -1, sizeof(Supervisor)); // nuke everything
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
    MidiTimer *midiTimer;
    GameConfiguration cfg;
    i32 calcCount;
    i32 wantedState;
    i32 curState;
    i32 wantedState2;
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
    u32 flags;
    DWORD startupTimeForMenuMusic;
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
#pragma pack(pop)

C_ASSERT(sizeof(Supervisor) == 0x2d0);

extern Supervisor g_Supervisor;
extern ControllerMapping g_ControllerMapping;
extern u16 g_CurFrameRawInput;
extern u16 g_CurFrameGameInput;
extern u16 g_LastFrameRawInput;
extern u16 g_LastFrameGameInput;
extern u16 g_IsEighthFrameOfHeldInput;
extern u16 g_NumOfFramesInputsWereHeld;
