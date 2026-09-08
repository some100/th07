#pragma once

#include "AnmManager.hpp"
#include "ScreenEffect.hpp"
#include "ZunResult.hpp"
#include "ZunTimer.hpp"
#include "utils.hpp"

enum StageEaseMode
{
    STAGE_EASE_OUT_QUAD = 1,
    STAGE_EASE_OUT_CUBIC = 2,
    STAGE_EASE_OUT_QUART = 3,
    STAGE_EASE_IN_QUAD = 4,
    STAGE_EASE_IN_CUBIC = 5,
    STAGE_EASE_IN_QUART = 6,
    STAGE_EASE_CUBIC_INTERP = 7, // this is not actually an easing mode
};

enum SpellcardState
{
    SPELLCARD_STATE_INACTIVE,
    SPELLCARD_STATE_STARTING,
    SPELLCARD_STATE_ACTIVE,
};

enum StdOpcode
{
    STD_CAM_POS_KEY,
    STD_FOG,
    STD_FOG_INTERP,
    STD_HALT,
    STD_JUMP,
    STD_CAM_POS,
    STD_CAM_POS_INTERP,
    STD_CAM_LOOKAT,
    STD_CAM_LOOKAT_INTERP,
    STD_CAM_UP,
    STD_CAM_UP_INTERP,
    STD_CAM_FOV,
    STD_CAM_FOV_INTERP,
    STD_COLOR,
    STD_CAM_POS_INTERP_START,
    STD_CAM_POS_INTERP_END,
    STD_CAM_POS_INTERP_TAN_START,
    STD_CAM_POS_INTERP_TAN_END,
    STD_CAM_POS_INTERP_BEZIER,
    STD_CAM_LOOKAT_INTERP_START,
    STD_CAM_LOOKAT_INTERP_END,
    STD_CAM_LOOKAT_INTERP_TAN_START,
    STD_CAM_LOOKAT_INTERP_TAN_END,
    STD_CAM_LOOKAT_INTERP_BEZIER,
    STD_CAM_UP_INTERP_START,
    STD_CAM_UP_INTERP_END,
    STD_CAM_UP_INTERP_TAN_START,
    STD_CAM_UP_INTERP_TAN_END,
    STD_CAM_UP_INTERP_BEZIER,
    STD_BG_SCRIPT1,
    STD_BG_SCRIPT2,
};

struct StageAnms
{
    const char *anmPath1;
    const char *anmPath2;
};
extern StageAnms g_EnemyAnmStageFiles[9];

struct StdRawHeader
{
    i16 objectsCount;
    i16 quadCount;
    u32 facesOffset;
    u32 scriptOffset;
    u32 unused_c;
    char stageName[128];
    char bgmNames[4][128];
    char bgmPaths[4][128];
};

struct StdRawQuadBasic
{
    i16 type;
    i16 byteSize;
    i16 anmScript;
    i16 vmIndex;
    Float3 pos;
    Float2 size;
};

struct StdRawObject
{
    u16 id;
    i8 zLevel;
    i8 flags;
    Float3 pos;
    Float3 size;
    StdRawQuadBasic firstQuad;
};

struct StdRawInstance
{
    i16 id;
    i16 field1_0x2;
    Float3 pos;
};

struct StdRawInstrArgs
{
    AnyArg args[3];

    Float3 *AsVec()
    {
        return (Float3 *)args;
    }
};

struct StdRawInstr
{
    i32 frame;
    i16 opcode;
    i16 size;
    StdRawInstrArgs args;
};

struct StageCameraSky
{
    f32 nearPlane;
    f32 farPlane;
};

struct StageCamera
{
    Float3 pos;
    Float3 lookAt;
    Float3 up;
    Float3 lookAtDir;
    Float3 right;
    f32 fov;
};

struct StageFog
{
    f32 nearPlane;
    f32 farPlane;
    ZunColor color;
};

struct Stage
{
    Stage();

    static ZunResult RegisterChain(i32 stage);
    static void CutChain();

    static ZunResult AddedCallback(Stage *arg);
    static ZunResult DeletedCallback(Stage *arg);
    static u32 OnUpdate(Stage *arg);
    static u32 OnDrawHighPrio(Stage *arg);
    static u32 OnDrawLowPrio(Stage *arg);

    static void DrawColoredQuad(ZunRect *rect, D3DCOLOR param_2, D3DCOLOR param_3,
                                D3DCOLOR param_4, D3DCOLOR param_5);
    ZunResult LoadStageData(const char *stdPath);
    i32 RenderObjects(i32 param_1);
    void SmoothBlendColor(ZunColor param_1);
    void UpdateCamera();
    ZunResult UpdateObjects();
    void SetupCameraStageBackground();
    static void UpdateScriptAndCamera(Stage *stage, i32 param_2,
                                      Float3 *param_3, Float3 *param_4,
                                      Float3 *param_5, Float3 *param_6,
                                      Float3 *param_7);

    AnmVm *quadVms;
    AnmVm vm1;
    AnmVm vm2;
    StdRawHeader *stdData;
    i32 quadCount;
    i32 objectsCount;
    StdRawObject **objects;
    StdRawInstance *objectInstances;
    StdRawInstr *beginningOfScript;
    ZunTimer scriptTime;
    i32 instructionIndex;
    i32 stageFrameCounter;
    u32 stage;
    Float3 pos;
    D3DCOLOR color;
    StageFog skyFog;
    StageFog fogEnd;
    StageFog fogStart;
    i32 skyFogInterpDuration;
    ZunTimer skyFogInterpTimer;
    u8 renderStateWasReset;
    // pad 3
    i32 spellCardState;
    i32 ticksSinceSpellcardStarted;
    ZunBool clearBackground;
    i32 numSpellcardVms;
    i32 spellcardVmsIdx;
    AnmVm spellcardVms[32];
    AnmVm unusedVm;
    i32 scriptWaitTime;
    StageCamera camEnd;
    StageCamera camStart;
    StageCamera camTangentEnd;
    StageCamera camTangentStart;
    StageCamera cam;
    i32 timersMax[4];
    ZunTimer timers[4];
    i32 easeModes[4];
    Float3 positionStart;
    i32 positionInterpEndTime;
    Float3 positionInterpInitial;
    i32 positionInterpStartTime;
    u8 cameraTeleported;
    // pad 3
    ZunColor color2;
    ZunBool isDarkening;
};
C_ASSERT(sizeof(Stage) == 0x52b4);
extern Stage g_Stage;
