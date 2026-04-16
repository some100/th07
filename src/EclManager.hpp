#pragma once

#include "ZunResult.hpp"
#include "ZunTimer.hpp"
#include "inttypes.hpp"
#include "utils.hpp"

// values from
// https://en.touhouwiki.net/wiki/User:Mddass/Touhou_File_Format_Specification/ECL
typedef enum EclVarId
{
    VAR_LOCAL_INT1_1 = 10000,
    VAR_LOCAL_INT1_2 = 10001,
    VAR_LOCAL_INT1_3 = 10002,
    VAR_LOCAL_INT1_4 = 10003,
    VAR_LOCAL_FLOAT1_1 = 10004,
    VAR_LOCAL_FLOAT1_2 = 10005,
    VAR_LOCAL_FLOAT1_3 = 10006,
    VAR_LOCAL_FLOAT1_4 = 10007,
    VAR_LOCAL_FLOAT1_5 = 10008,
    VAR_LOCAL_FLOAT1_6 = 10009,
    VAR_LOCAL_FLOAT1_7 = 10010,
    VAR_LOCAL_FLOAT1_8 = 10011,
    VAR_LOCAL_INT2_1 = 10012,
    VAR_LOCAL_INT2_2 = 10013,
    VAR_LOCAL_INT2_3 = 10014,
    VAR_LOCAL_INT2_4 = 10015,
    VAR_DIFFICULTY = 10016,
    VAR_RANK = 10017,
    VAR_POS_X = 10018,
    VAR_POS_Y = 10019,
    VAR_POS_Z = 10020,
    VAR_PLAYER_POS_X = 10021,
    VAR_PLAYER_POS_Y = 10022,
    VAR_PLAYER_POS_Z = 10023,
    VAR_ANGLE_TO_PLAYER = 10024,
    VAR_CUR_TIME = 10025,
    VAR_DISTANCE_FROM_PLAYER = 10026,
    VAR_LIFE = 10027,
    VAR_PLAYER_SHOTTYPE = 10028,
    VAR_LOCAL_INT3_1 = 10029,
    VAR_LOCAL_INT3_2 = 10030,
    VAR_LOCAL_INT3_3 = 10031,
    VAR_LOCAL_INT3_4 = 10032,
    VAR_LOCAL_FLOAT3_1 = 10033,
    VAR_LOCAL_FLOAT3_2 = 10034,
    VAR_LOCAL_FLOAT3_3 = 10035,
    VAR_LOCAL_FLOAT3_4 = 10036,
    VAR_GLOBAL_INT_1 = 10037,
    VAR_GLOBAL_INT_2 = 10038,
    VAR_GLOBAL_INT_3 = 10039,
    VAR_GLOBAL_INT_4 = 10040,
    VAR_GLOBAL_FLOAT_1 = 10041,
    VAR_GLOBAL_FLOAT_2 = 10042,
    VAR_GLOBAL_FLOAT_3 = 10043,
    VAR_GLOBAL_FLOAT_4 = 10044,
    VAR_ANGLE = 10045,
    VAR_ANGULAR_VELOCITY = 10046,
    VAR_MOVE_SPEED = 10047,
    VAR_MOVE_ACCELERATION = 10048,
    VAR_MOVE_RADIUS = 10049,
    VAR_MOVE_INTERP_ORIGIN_X = 10050,
    VAR_MOVE_INTERP_ORIGIN_Y = 10051,
    VAR_MOVE_INTERP_ORIGIN_Z = 10052,
    VAR_MOVE_ANGLE = 10053,
    VAR_MOVE_ANGULAR_VELOCITY = 10054,
    VAR_RNG_0_TO_1 = 10055,
    VAR_RNG_CUSTOM_BOUND = 10056,
    VAR_MOVE_INTERP_TARGET_X = 10057,
    VAR_MOVE_INTERP_TARGET_Y = 10058,
    VAR_MOVE_INTERP_TARGET_Z = 10059,
    VAR_RNG_RADIAN = 10060,
    VAR_LAST_DAMAGE = 10061,
    VAR_BOSS_ID = 10062,
    VAR_FINAL_POS_X = 10063,
    VAR_FINAL_POS_Y = 10064,
    VAR_FINAL_POS_Z = 10065,
    VAR_BOSS_LIFE_THRESHOLD1 = 10066,
    VAR_BOSS_LIFE_THRESHOLD2 = 10067,
    VAR_BOSS_LIFE_THRESHOLD3 = 10068,
    VAR_BOSS_LIFE_THRESHOLD4 = 10069,
    VAR_ITEMDROP = 10070,
    VAR_SCORE = 10071,
    VAR_LOCAL_FLOAT2_1 = 10072,
    VAR_LOCAL_FLOAT2_2 = 10073
} EclVarId;

typedef void (*EclExInstr)(struct Enemy *, struct EclRawInstr *);
typedef void (*EclInterpFn)(struct Enemy *, struct EclInterp *, f32 t);

struct EclRawHeader
{
    i16 subCount;
    i16 timelineCount;
    struct EclTimelineInstr *timelinePtr[16];
    u32 subTable[];
};

struct EclRawInstr
{
    u32 time;
    i16 id;
    i16 size;
    u8 unused_8;
    u8 skipInstrOnDifficulty;
    u16 paramMask;
    AnyArg args[];
};

struct EclTimelineInstr
{
    i16 time;
    i16 arg0;
    u16 opcode;
    i16 size;
    AnyArg args[6];
};

struct EclTimeline
{
    ZunTimer timelineTime;
    EclTimelineInstr *timelineInstr;
};

struct EclGlobalVars
{
    i32 intVars[4];
    f32 floatVars[4];
};

struct EclContextArgs
{
    i32 intVars1[4];
    f32 floatVars1[8];
    i32 intVars2[4];
    f32 floatVars2[2];
    EclGlobalVars globalVars;
};

struct EclInterp
{
    EclInterpFn fn;
    ZunTimer timer;
    AnyArg args[8];
};

struct EclManager
{
    ZunResult Load(const char *path);
    void Unload();
    ZunResult CallEclSub(struct EnemyEclContext *param_1, i16 subId);
    static i32 *GetVar(Enemy *enemy, i32 *eclVarId, u16 paramMask, i32 param_4);
    static i32 GetVarValue(Enemy *enemy, i32 eclVarId);
    static f32 *GetFloatVar(Enemy *enemy, f32 *eclVarFloat, u16 paramMask,
                            i32 param_4);
    static f32 GetFloatVarValue(Enemy *enemy, f32 eclVarFloat);
    static void MoveDirTime(Enemy *enemy, EclRawInstr *instr);
    static void MovePosTime(Enemy *enemy, EclRawInstr *instr);
    static void MathLerp(Enemy *enemy, EclInterp *interp, f32 t);
    static void MathCubicInterp(Enemy *enemy, EclInterp *interp, f32 t);
    static void BeginSpellcard(Enemy *enemy, EclRawInstr *instr);
    static void EndSpellcard();
    ZunResult RunEcl(Enemy *enemy);

    EclRawHeader *eclFile;
    EclRawInstr **subTable;
};

extern EclManager g_EclManager;
extern const char *g_EclPaths[10];
extern EclGlobalVars g_GlobalEclVars;
