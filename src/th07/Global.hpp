#pragma once

#include "ZunBool.hpp"
#include "ZunResult.hpp"
#include "i18n.hpp"
#include "inttypes.hpp"
#include "pbg4/Pbg4Archive.hpp"
#include "utils.hpp"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define IS_PRESSED_RAW(key) ((g_CurFrameRawInput & (key)) != 0)
#define IS_PRESSED_GAME(key) ((g_CurFrameGameInput & (key)) != 0)
#define WAS_PRESSED_RAW(key) (IS_PRESSED_RAW(key) && ((g_CurFrameRawInput & (key)) != (g_LastFrameRawInput & (key))))
#define WAS_PRESSED_GAME(key) (IS_PRESSED_GAME(key) && ((g_CurFrameGameInput & (key)) != (g_LastFrameGameInput & (key))))
#define IS_EIGHTH(key) (((g_CurFrameRawInput & (key)) != 0) && (g_IsEighthFrameOfHeldInput != 0))
#define WAS_PRESSED_SCROLLING(key) (WAS_PRESSED_RAW(key) || IS_EIGHTH(key))

#define ZUN_NEW(type, zunName) ((type *)g_ZunMemory.AddToRegistry(new type(), sizeof(type), zunName))
#define ZUN_NEW_ARRAY(type, number, zunName) \
    ((type *)g_ZunMemory.AddToRegistry(new type[number], sizeof(type) * number, zunName))
#define ZUN_DELETE(p)                  \
    g_ZunMemory.RemoveFromRegistry(p); \
    delete p;                          \
    p = NULL;

#define ZUN_ALLOC(size) g_ZunMemory.Alloc(size)
#define ZUN_ALLOC_WEIRD(size) g_ZunMemory.AllocEvilFakeMatch(size)
#define ZUN_FREE(p) g_ZunMemory.Free(p);

namespace utils
{

static void DebugPrint(const char *fmt, ...)
{
}

static void DebugPrint2(const char *fmt, ...)
{
}

} // namespace utils

enum ChainCallbackResult
{
    CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB,
    CHAIN_CALLBACK_RESULT_CONTINUE,
    CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN,
    CHAIN_CALLBACK_RESULT_BREAK,
    CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS,
    CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR,
    CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB
};

enum
{
    CHAIN_PRIO_CALC_SUPERVISOR = 0,
    CHAIN_PRIO_CALC_ASCIIMANAGER = 1,
    CHAIN_PRIO_CALC_GAMEMANAGER = 2,
    CHAIN_PRIO_CALC_MAINMENU = 3,
    CHAIN_PRIO_CALC_MUSICROOM = 3,
    CHAIN_PRIO_CALC_ENDING = 4,
    CHAIN_PRIO_CALC_REPLAYMANAGER_PLAYBACK_HIGH_PRIO = 5,
    CHAIN_PRIO_CALC_REPLAYMANAGER_PLAYBACK_LOW_PRIO = 6,
    CHAIN_PRIO_CALC_STAGE = 7,
    CHAIN_PRIO_CALC_PLAYER = 8,
    CHAIN_PRIO_CALC_ENEMYMANAGER = 10,
    CHAIN_PRIO_CALC_EFFECTMANAGER = 11,
    CHAIN_PRIO_CALC_BULLETMANAGER = 12,
    CHAIN_PRIO_CALC_GUI = 13,
    CHAIN_PRIO_CALC_RESULTSCREEN = 14,
    CHAIN_PRIO_CALC_SCREENEFFECT = 15,
    CHAIN_PRIO_CALC_REPLAYMANAGER_RECORD_HIGH_PRIO = 16,
    CHAIN_PRIO_CALC_REPLAYMANAGER_LOW_PRIO = 17,
};

enum
{
    CHAIN_PRIO_DRAW_MAINMENU = 0,
    CHAIN_PRIO_DRAW_MUSICROOM = 0,
    CHAIN_PRIO_DRAW_ENDING = 1,
    CHAIN_PRIO_DRAW_GAMEMANAGER = 2,
    CHAIN_PRIO_DRAW_STAGE_HIGH_PRIO = 3,
    CHAIN_PRIO_DRAW_STAGE_LOW_PRIO = 4,
    CHAIN_PRIO_DRAW_ENEMYMANAGER_HIGH_PRIO = 5,
    CHAIN_PRIO_DRAW_PLAYER_HIGH_PRIO = 6,
    CHAIN_PRIO_DRAW_ENEMYMANAGER_LOW_PRIO = 7,
    CHAIN_PRIO_DRAW_PLAYER_LOW_PRIO = 8,
    CHAIN_PRIO_DRAW_EFFECTMANAGER = 9,
    CHAIN_PRIO_DRAW_BULLETMANAGER = 10,
    CHAIN_PRIO_DRAW_ASCIIMANAGER_POPUPS = 11,
    CHAIN_PRIO_DRAW_GUI = 12,
    CHAIN_PRIO_DRAW_RESULTSCREEN = 13,
    CHAIN_PRIO_DRAW_REPLAYMANAGER = 14,
    CHAIN_PRIO_DRAW_SUPERVISOR = 15,
    CHAIN_PRIO_DRAW_ASCIIMANAGER_MENUS = 16,
    CHAIN_PRIO_DRAW_SCREENEFFECT = 17
};

typedef u32 (*ChainCallback)(void *);
typedef ZunResult (*ChainLifecycleCallback)(void *);

struct ChainElem
{
    ChainElem();
    ~ChainElem();

    i16 priority;
    u16 isAllocated : 1;
    ChainCallback callback;
    ChainLifecycleCallback addedCallback;
    ChainLifecycleCallback deletedCallback;
    ChainElem *prev;
    ChainElem *next;
    ChainElem *unkPtr;
    void *arg;
};

struct Chain
{
    Chain();
    ~Chain();

    ZunResult AddToCalcChain(ChainElem *elem, i32 priority);
    ZunResult AddToDrawChain(ChainElem *elem, i32 priority);
    ChainElem *CreateElem(ChainCallback callback);
    void Cut(ChainElem *toRemove);
    void Release();
    void ReleaseSingleChain(ChainElem *root);
    i32 RunCalcChain();
    i32 RunDrawChain();

    ChainElem calcChain;
    ChainElem drawChain;
};
C_ASSERT(sizeof(Chain) == 0x40);
extern Chain g_Chain;

enum TouhouButton
{
    TH_BUTTON_SHOOT = 1 << 0,
    TH_BUTTON_BOMB = 1 << 1,
    TH_BUTTON_FOCUS = 1 << 2,
    TH_BUTTON_MENU = 1 << 3,
    TH_BUTTON_UP = 1 << 4,
    TH_BUTTON_DOWN = 1 << 5,
    TH_BUTTON_LEFT = 1 << 6,
    TH_BUTTON_RIGHT = 1 << 7,
    TH_BUTTON_SKIP = 1 << 8,
    TH_BUTTON_Q = 1 << 9,
    TH_BUTTON_S = 1 << 10,
    TH_BUTTON_HOME = 1 << 11,
    TH_BUTTON_ENTER = 1 << 12,
    TH_BUTTON_D = 1 << 13, // only used for cheat code
    TH_BUTTON_RESET = 1 << 14,

    TH_BUTTON_UP_LEFT = TH_BUTTON_UP | TH_BUTTON_LEFT,
    TH_BUTTON_UP_RIGHT = TH_BUTTON_UP | TH_BUTTON_RIGHT,
    TH_BUTTON_DOWN_LEFT = TH_BUTTON_DOWN | TH_BUTTON_LEFT,
    TH_BUTTON_DOWN_RIGHT = TH_BUTTON_DOWN | TH_BUTTON_RIGHT,
    TH_BUTTON_DIRECTION =
        TH_BUTTON_DOWN | TH_BUTTON_RIGHT | TH_BUTTON_UP | TH_BUTTON_LEFT,

    TH_BUTTON_SELECTMENU = TH_BUTTON_ENTER | TH_BUTTON_SHOOT,
    TH_BUTTON_RETURNMENU = TH_BUTTON_MENU | TH_BUTTON_BOMB,
    TH_BUTTON_WRONG_CHEATCODE = TH_BUTTON_SHOOT | TH_BUTTON_BOMB |
                                TH_BUTTON_MENU | TH_BUTTON_Q | TH_BUTTON_S |
                                TH_BUTTON_ENTER,
    TH_BUTTON_ANY = 0xFFFF,
};

namespace Controller
{
u16 GetControllerInput(u16 buttons);
u8 *GetControllerState();
u16 GetInput();
u16 GetJoystickCaps();
void ResetKeyboard();
u32 SetButtonFromControllerInputs(u16 *outButtons, i16 controllerButtonToTest,
                                  u32 touhouButton, u32 inputButtons);
u32 SetButtonFromDirectInputJoystate(u16 *outButtons,
                                     i16 controllerButtonToTest,
                                     u32 touhouButton, u8 *inputButtons);
} // namespace Controller
extern u16 g_CurFrameRawInput;
extern u16 g_CurFrameGameInput;
extern u16 g_LastFrameRawInput;
extern u16 g_LastFrameGameInput;
extern u16 g_IsEighthFrameOfHeldInput;
extern u16 g_NumOfFramesInputsWereHeld;

namespace FileSystem
{
ZunBool CheckFileExists(const char *file);
u8 *OpenFile(const char *filepath, ZunBool isExternalResource);
i32 WriteDataToFile(const char *filename, const void *out, DWORD bytesToWrite);
} // namespace FileSystem
extern u32 g_LastFileSize;

struct GameErrorContext
{
    char m_Buffer[8192];
    char *m_BufferEnd;
    i8 m_ShowMessageBox;

    GameErrorContext()
    {
        m_BufferEnd = m_Buffer;
        m_Buffer[0] = '\0';
        m_ShowMessageBox = false;
        Log(TH_LOG_START);
    }

    const char *Fatal(const char *fmt, ...);
    const char *Log(const char *fmt, ...);

    // FUNCTION: TH07 0x00433e90
    void Flush()
    {
        if (this->m_BufferEnd != this->m_Buffer)
        {
            // STRING: TH07 0x00497c7c
            this->Log("---------------------------------------------------------- \r\n");
            if (this->m_ShowMessageBox)
            {
                // STRING: TH07 0x00497c78
                MessageBoxA(NULL, this->m_Buffer, "log", MB_ICONERROR);
            }
            // STRING: TH07 0x00497c6c
            FileSystem::WriteDataToFile("./log.txt", this->m_Buffer,
                                        strlen(this->m_Buffer));
        }
    }
};
extern GameErrorContext g_GameErrorContext;

struct Rng
{
    f32 GetRandomFloat();
    u16 GetRandomU16();
    u32 GetRandomU32();

    u16 GetRandomU16InRange(u16 range)
    {
        return range != 0 ? this->GetRandomU16() % range : 0;
    }

    u32 GetRandomU32InRange(u32 range)
    {
        return range != 0 ? this->GetRandomU32() % range : 0;
    }

    f32 GetRandomFloatInRange(f32 range)
    {
        return this->GetRandomFloat() * range;
    }

    void SetSeed(u16 newSeed)
    {
        this->seed = newSeed;
    }

    u32 GetGenCount()
    {
        return this->generationCount;
    }

    u16 seed;
    u16 seedBackup;
    u32 generationCount;
};
extern Rng g_Rng;

class ZunMemory
{
  public:
    ZunMemory()
    {
        this->bRegistryInUse = FALSE;
    }
    ~ZunMemory()
    {
    }

    void *Alloc(size_t size)
    {
        return malloc(size);
    }

    void *AllocEvilFakeMatch(size_t size)
    {
        size_t ok = size;
        return malloc(ok);
    }

    void Free(void *ptr)
    {
        free(ptr);
    }

    void *AddToRegistry(void *ptr, size_t size, const char *name)
    {
        return ptr;
    }

    void RemoveFromRegistry(VOID *ptr)
    {
    }

  private:
    ZunBool bRegistryInUse;
};
extern ZunMemory g_ZunMemory;

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

struct ZunGlobals
{
    u32 guiScore;
    u32 score;
    u32 guiScoreDifference;
    u32 highScore;
    u8 highScoreNumContinues;
    // pad 3
    i32 grazeInStage;
    i32 grazeInTotal;
    i32 spellCardsCaptured;
    u8 numRetries;
    // pad 3
    i32 pointItemsCollectedThisStage;
    i32 pointItemsCollectedForExtend;
    i32 extendsFromPointItems;
    i32 nextNeededPointItemsForExtend;
    i32 rng1[7];
    f32 deaths; // ZUN quirk: Why the fuck are these stored as floats
    f32 rngFloat1[2];
    f32 livesRemaining;
    f32 rngFloat2[2];
    f32 bombsRemaining;
    f32 bombsUsed;
    f32 rngFloat3[3];
    f32 currentPower;
    f32 rngFloat4[2];
    i32 cherryStart;
    i32 rng2[8];
    u32 curCsum;
    i32 csumAsSum;
    i32 csumData[5];
};
C_ASSERT(sizeof(ZunGlobals) == 0xc8);

struct Float3
{
    Float3()
    {
    }

    Float3(f32 x, f32 y, f32 z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    // FUNCTION: TH07 0x004325c0
    void FromAngleMagnitude(f32 angle, f32 magnitude)
    {
        /* this->x = cosf(angle) * magnitude;
         * this->y = sinf(angle) * magnitude;
         */
        __asm {
            mov eax, this
            fld [angle]
            fsincos
            fmul [magnitude]
            fstp float ptr [eax]
            fmul [magnitude]
            fstp float ptr [eax + 4]
        }
    }

    operator f32 *()
    {
        return &x;
    }

    Float3 *operator+=(const Float3 &other)
    {
        this->x += other.x;
        this->y += other.y;
        this->z += other.z;
        return this;
    }

    Float3 *operator-=(const Float3 &other)
    {
        this->x -= other.x;
        this->y -= other.y;
        this->z -= other.z;
        return this;
    }

    Float3 *operator*=(f32 s)
    {
        this->x *= s;
        this->y *= s;
        this->z *= s;
        return this;
    }

    Float3 operator-() const
    {
        return Float3(-x, -y, -z);
    }

    Float3 operator+(const Float3 &other) const
    {
        return Float3(
            x + other.x,
            y + other.y,
            z + other.z);
    }

    Float3 operator-(const Float3 &other) const
    {
        return Float3(
            x - other.x,
            y - other.y,
            z - other.z);
    }

    Float3 operator*(f32 s) const
    {
        return Float3(
            x * s,
            y * s,
            z * s);
    }

    Float3 operator/(f32 s) const
    {
        f32 inv = 1.0f / s;
        return Float3(
            x * inv,
            y * inv,
            z * inv);
    }

    friend Float3 operator*(f32 s, const Float3 &v)
    {
        return Float3(
            s * v.x,
            s * v.y,
            s * v.z);
    }

    D3DXVECTOR3 *asD3DX()
    {
        return (D3DXVECTOR3 *)this;
    }

    f32 x;
    f32 y;
    f32 z;
};

struct Float2
{
    f32 x;
    f32 y;

    void FromAngleMagnitude(f32 angle, f32 magnitude)
    {
        /* this->x = cosf(angle) * magnitude;
         * this->y = sinf(angle) * magnitude;
         */
        __asm {
            mov eax, this
            fld [angle]
            fsincos
            fmul [magnitude]
            fstp float ptr [eax]
            fmul [magnitude]
            fstp float ptr [eax + 4]
        }
    }
};

f32 AddNormalizeAngle(f32 a, f32 b);
void Rotate(Float3 *out, Float3 *point, f32 angle);

extern Pbg4Archive g_Pbg4Archive;
