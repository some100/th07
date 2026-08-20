#pragma once

#include "ZunResult.hpp"
#include "ZunTimer.hpp"
#include "inttypes.hpp"
#include "utils.hpp"

extern const char *g_EclPaths[10];

// values from
// https://en.touhouwiki.net/wiki/User:Mddass/Touhou_File_Format_Specification/ECL
enum EclVarId
{
    ECL_VAR_LOCAL_INT1_1 = 10000,
    ECL_VAR_LOCAL_INT1_2 = 10001,
    ECL_VAR_LOCAL_INT1_3 = 10002,
    ECL_VAR_LOCAL_INT1_4 = 10003,
    ECL_VAR_LOCAL_FLOAT1_1 = 10004,
    ECL_VAR_LOCAL_FLOAT1_2 = 10005,
    ECL_VAR_LOCAL_FLOAT1_3 = 10006,
    ECL_VAR_LOCAL_FLOAT1_4 = 10007,
    ECL_VAR_LOCAL_FLOAT1_5 = 10008,
    ECL_VAR_LOCAL_FLOAT1_6 = 10009,
    ECL_VAR_LOCAL_FLOAT1_7 = 10010,
    ECL_VAR_LOCAL_FLOAT1_8 = 10011,
    ECL_VAR_LOCAL_INT2_1 = 10012,
    ECL_VAR_LOCAL_INT2_2 = 10013,
    ECL_VAR_LOCAL_INT2_3 = 10014,
    ECL_VAR_LOCAL_INT2_4 = 10015,
    ECL_VAR_DIFFICULTY = 10016,
    ECL_VAR_RANK = 10017,
    ECL_VAR_POS_X = 10018,
    ECL_VAR_POS_Y = 10019,
    ECL_VAR_POS_Z = 10020,
    ECL_VAR_PLAYER_POS_X = 10021,
    ECL_VAR_PLAYER_POS_Y = 10022,
    ECL_VAR_PLAYER_POS_Z = 10023,
    ECL_VAR_ANGLE_TO_PLAYER = 10024,
    ECL_VAR_CUR_TIME = 10025,
    ECL_VAR_DISTANCE_FROM_PLAYER = 10026,
    ECL_VAR_LIFE = 10027,
    ECL_VAR_PLAYER_SHOTTYPE = 10028,
    ECL_VAR_LOCAL_INT3_1 = 10029,
    ECL_VAR_LOCAL_INT3_2 = 10030,
    ECL_VAR_LOCAL_INT3_3 = 10031,
    ECL_VAR_LOCAL_INT3_4 = 10032,
    ECL_VAR_LOCAL_FLOAT3_1 = 10033,
    ECL_VAR_LOCAL_FLOAT3_2 = 10034,
    ECL_VAR_LOCAL_FLOAT3_3 = 10035,
    ECL_VAR_LOCAL_FLOAT3_4 = 10036,
    ECL_VAR_GLOBAL_INT_1 = 10037,
    ECL_VAR_GLOBAL_INT_2 = 10038,
    ECL_VAR_GLOBAL_INT_3 = 10039,
    ECL_VAR_GLOBAL_INT_4 = 10040,
    ECL_VAR_GLOBAL_FLOAT_1 = 10041,
    ECL_VAR_GLOBAL_FLOAT_2 = 10042,
    ECL_VAR_GLOBAL_FLOAT_3 = 10043,
    ECL_VAR_GLOBAL_FLOAT_4 = 10044,
    ECL_VAR_ANGLE = 10045,
    ECL_VAR_ANGULAR_VELOCITY = 10046,
    ECL_VAR_MOVE_SPEED = 10047,
    ECL_VAR_MOVE_ACCELERATION = 10048,
    ECL_VAR_MOVE_RADIUS = 10049,
    ECL_VAR_MOVE_INTERP_ORIGIN_X = 10050,
    ECL_VAR_MOVE_INTERP_ORIGIN_Y = 10051,
    ECL_VAR_MOVE_INTERP_ORIGIN_Z = 10052,
    ECL_VAR_MOVE_ANGLE = 10053,
    ECL_VAR_MOVE_ANGULAR_VELOCITY = 10054,
    ECL_VAR_RNG = 10055,
    ECL_VAR_RNG_CUSTOM_BOUND = 10056,
    ECL_VAR_MOVE_INTERP_TARGET_X = 10057,
    ECL_VAR_MOVE_INTERP_TARGET_Y = 10058,
    ECL_VAR_MOVE_INTERP_TARGET_Z = 10059,
    ECL_VAR_RNG_RADIAN = 10060,
    ECL_VAR_LAST_DAMAGE = 10061,
    ECL_VAR_BOSS_ID = 10062,
    ECL_VAR_DELTA_POS_X = 10063,
    ECL_VAR_DELTA_POS_Y = 10064,
    ECL_VAR_DELTA_POS_Z = 10065,
    ECL_VAR_BOSS_LIFE_THRESHOLD1 = 10066,
    ECL_VAR_BOSS_LIFE_THRESHOLD2 = 10067,
    ECL_VAR_BOSS_LIFE_THRESHOLD3 = 10068,
    ECL_VAR_BOSS_LIFE_THRESHOLD4 = 10069,
    ECL_VAR_ITEMDROP = 10070,
    ECL_VAR_SCORE = 10071,
    ECL_VAR_LOCAL_FLOAT2_1 = 10072,
    ECL_VAR_LOCAL_FLOAT2_2 = 10073
};

enum EclOpcode
{
    // just returns ZUN_ERROR for RunEcl
    ECL_UNIMP = 1,
    ECL_SET_WAIT_TIMER = 45,
    // decrements int at arg 2. if its greater than 0, fallthrough to ECL_JUMP,
    // otherwise break
    ECL_DEC_JUMP = 3,
    // sets the current context time to int at arg 0, then jumps by an offset
    // of int at arg 1.
    ECL_JUMP = 2,
    ECL_SET_INT = 4,
    ECL_SET_FLOAT = 5,
    ECL_NORMALIZE_ANGLE = 40,
    ECL_RAND = 6,
    ECL_RAND_ADD = 7,
    ECL_RAND_FLOAT = 8,
    ECL_RAND_FLOAT_ADD = 9,
    ECL_RAND_SIGN = 10,
    ECL_RAND_SIGN_FLOAT = 11,
    ECL_INC = 17,
    ECL_DEC = 18,
    ECL_GET_BOSS_INT = 43,
    ECL_GET_BOSS_FLOAT = 44,
    ECL_ADD = 12,
    ECL_ADD_FLOAT = 19,
    ECL_SUB = 13,
    ECL_SUB_FLOAT = 20,
    ECL_MUL = 14,
    ECL_MUL_FLOAT = 21,
    ECL_DIV = 15,
    ECL_DIV_FLOAT = 22,
    ECL_MOD = 16,
    ECL_MOD_FLOAT = 23,
    ECL_SIN = 24,
    ECL_COS = 25,
    ECL_ATAN2 = 26,
    ECL_LERP = 159,
    ECL_INIT_INTERP = 27,
    ECL_JUMP_IF_EQ = 28,
    ECL_JUMP_IF_EQ_FLOAT = 29,
    ECL_JUMP_IF_NEQ = 30,
    ECL_JUMP_IF_NEQ_FLOAT = 31,
    ECL_JUMP_IF_LT = 32,
    ECL_JUMP_IF_LT_FLOAT = 33,
    ECL_JUMP_IF_LEQ = 34,
    ECL_JUMP_IF_LEQ_FLOAT = 35,
    ECL_JUMP_IF_GT = 36,
    ECL_JUMP_IF_GT_FLOAT = 37,
    ECL_JUMP_IF_GEQ = 38,
    ECL_JUMP_IF_GEQ_FLOAT = 39,
    ECL_SUB_CALL = 41,
    ECL_SUB_RET = 42,
    ECL_SET_ANM = 95,
    ECL_SET_SUB_ANM = 97,
    ECL_SET_POS = 46,
    ECL_SET_AXIS_SPEED = 47,
    ECL_SET_ANGULAR_VEL = 48,
    ECL_MOVE_AT_PLAYER = 53,
    ECL_SET_MOVE_SPEED = 49,
    ECL_SET_MOVE_ACCEL = 50,
    ECL_SET_MOVE_INTERP_TIMER_POLAR = 59,
    ECL_SET_MOVE_INTERP_TIMER_RADIAL = 60,
    ECL_SET_MOVE_INTERP_TIMER_INTERP = 61,
    ECL_SPAWN_BULLET_PATTERN_SPREAD_AIMED = 64,
    ECL_SPAWN_BULLET_PATTERN_SPREAD_ABS = 65,
    ECL_SPAWN_BULLET_PATTERN_RING_AIMED = 66,
    ECL_SPAWN_BULLET_PATTERN_RING_ABS = 67,
    ECL_SPAWN_BULLET_PATTERN_RING_SHIFTED_AIMED = 68,
    ECL_SPAWN_BULLET_PATTERN_RING_SHIFTED_ABS = 69,
    ECL_SPAWN_BULLET_PATTERN_ANGLE_RANDOM = 70,
    ECL_SPAWN_BULLET_PATTERN_RING_SPEED_RANDOM = 71,
    ECL_SPAWN_BULLET_PATTERN_RANDOM = 72,
    ECL_INIT_BULLET_CMD = 79,
    ECL_SET_DEATH_ANM = 98,
    ECL_SET_SHOOT_INTERVAL = 73,
    ECL_SET_SHOOT_INTERVAL_RAND = 74,
    ECL_DISABLE_BULLETS = 75,
    ECL_ENABLE_BULLETS = 76,
    ECL_SPAWN_PREV_BULLET_PATTERN = 77,
    ECL_SET_SHOOT_OFFSET = 78,
    ECL_SPAWN_LASER_PATTERN_FIXED = 82,
    ECL_SPAWN_LASER_PATTERN_MOVING = 83,
    ECL_SET_LASER_IDX = 84,
    ECL_ADD_LASER_ANGLE = 85,
    ECL_SET_LASER_ANGLE = 152,
    ECL_AIM_LASER_ANGLE_AT_PLAYER = 86,
    ECL_SET_LASER_POS_REL = 87,
    ECL_SET_LASER_HIDE_WARNING = 156,
    ECL_TEST_LASER_NOT_IN_USE = 88,
    ECL_STOP_LASER = 89,
    ECL_CLEAR_LASERS = 134,
    ECL_SET_LASER_START_LEN = 157,
    ECL_SET_LASER_OFFSETS = 158,
    // NO CLUE AT ALL
    ECL_IDFK = 147,
    ECL_SET_BOSS = 99,
    ECL_SPAWN_EFFECT = 100,
    ECL_MOVE_DIR_TIME = 54,
    ECL_MOVE_POS_TIME = 55,
    ECL_MOVE_ORBIT = 56,
    ECL_SET_ORBIT_RADIUS = 57,
    ECL_SET_ORBIT_ANGLE = 58,
    ECL_SET_MOVEMENT_BOUNDS = 62,
    ECL_DISABLE_MOVEMENT_BOUNDS = 63,
    ECL_RAND_FLOAT_RANGE = 51,
    ECL_GET_EXIT_ANGLE = 52,
    ECL_SET_MOVE_ANM = 96,
    ECL_SET_HITBOX_SIZE = 101,
    ECL_SET_GRAZE_SIZE = 153,
    ECL_SET_HAS_CONTACT_HITBOX = 102,
    ECL_SET_CAN_BE_DAMAGED = 103,
    ECL_SET_IS_HITTABLE = 104,
    ECL_PLAY_SOUND = 105,
    ECL_SET_DEATH_TYPE = 106,
    ECL_SET_DEATH_CALLBACK_SUB = 107,
    ECL_SET_INTERRUPT = 108,
    ECL_SET_RUN_INTERRUPT = 109,
    ECL_SET_LIFE = 110,
    ECL_SET_BOSS_HEALTH = 139,
    ECL_BEGIN_SPELLCARD = 90,
    ECL_END_SPELLCARD = 91,
    ECL_SET_TIMER = 111,
    ECL_SET_LIFE_CALLBACK_THRESHOLD = 112,
    ECL_SET_LIFE_CALLBACK_SUB = 113,
    ECL_SET_LIFE_CALLBACK = 148,
    ECL_SET_TIMER_CALLBACK_THRESHOLD = 114,
    ECL_SET_TIMER_CALLBACK_SUB = 115,
    ECL_SET_PERIODIC_CALLBACK = 144,
    ECL_SET_ENEMY_CAN_DIE = 116,
    // spawns oneshot particles, as opposed to ECL_SPAWN_EFFECT which is
    // tracked by the enemy's effects array.
    ECL_SPAWN_PARTICLES = 117,
    ECL_SPAWN_MOVING_PARTICLES = 118,
    // if the player's power is not at max, then it spawns power items,
    // otherwise it will spawn point items only
    ECL_SPAWN_ITEMS = 119,
    ECL_SPAWN_POINT_ITEMS = 154,
    ECL_SET_VM_AUTO_ROTATE = 120,
    // runs an ecl ex instr oneshot
    ECL_RUN_EX_INS = 121,
    // sets an enemy's ecl ex instr so it is called every ecl cycle
    ECL_SET_EX_INS = 122,
    ECL_ADD_TIME = 123,
    ECL_SPAWN_ITEM = 124,
    ECL_SET_SCRIPT_WAIT_TIME = 125,
    // the boss life markers are the things that appear on the top right side
    // when a boss has phases
    ECL_SET_NUM_BOSS_LIFE_MARKERS = 126,
    ECL_SPAWN_ENEMY_ABS = 92,
    ECL_SPAWN_ENEMY_REL = 93,
    // removes all enemies with a max score of 8000
    ECL_REMOVE_ALL_ENEMIES = 94,
    ECL_SET_PRIMARY_VM_INTERRUPT = 128,
    ECL_SET_VM_INTERRUPT = 129,
    // removes all bullets and spawns whatever the bullet manager's itemtype
    // happens to be
    ECL_REMOVE_ALL_BULLETS_SPAWN_ITEMS = 80,
    ECL_SET_BULLET_SOUND = 81,
    ECL_SET_NO_STACK_RET = 130,
    ECL_SET_BULLET_RANK_PARAMS = 131,
    ECL_SET_HAS_NO_COLLISION = 132,
    // sets the timer callback's subroutine to the death callback's subroutine
    ECL_BIND_TIMER_CALLBACK_TO_DEATH = 133,
    ECL_SET_IS_SURVIVAL_SPELLCARD = 135,
    ECL_SET_IS_PROJECTILE = 136,
    ECL_SET_DESPAWN_ON_OOB = 137,
    ECL_SET_TRAIL = 138,
    ECL_SET_GLOBAL_EFFECT_COLOR_MUL = 140,
    ECL_SET_INVINCIBILITY_TIMER = 142,
    ECL_REMOVE_BULLETS_RADIUS = 143,
    ECL_SET_BOSS_RUN_INTERRUPT = 145,
    ECL_REMOVE_ALL_BULLETS_NO_ITEMS = 146,
    ECL_SET_SPECIAL_EFFECT_POS = 149,
    ECL_SET_PRIMARY_VM_ROT_Z = 150,
    ECL_VEC_FROM_ANGLE_MAG = 151,
    ECL_RAND_EXIT_ANGLE = 155,
    ECL_ADD_CHERRY_PLUS = 160,
    // sets if an enemy's ecl script shouldn't be running while the player's
    // bomb is in use
    ECL_FREEZE_ECL_DURING_BOMB = 161,
};

typedef void (*EclExInstr)(struct Enemy *, struct EclRawInstr *);
typedef void (*EclInterpFn)(struct Enemy *, struct EclInterp *, f32 t);

struct EclRawHeader
{
    i16 subCount;
    i16 timelineCount;
    u32 timelineOffsets[16];
    u32 subTableOffsets[];
};
static_assert(sizeof(EclRawHeader) == 0x44);

struct EclRawInstr
{
    u32 time;
    i16 id;
    i16 size;
    u8 unused_8;
    u8 skipInstrOnDifficulty;
    u16 paramMask;
    AnyArg args[];

    AnyArg GetSecondArg()
    {
        return this->args[1];
    }
};
static_assert(sizeof(EclRawInstr) == 0xc);

struct EclTimelineInstrArgs
{
    AnyArg args[6];

    ZunVec3 *AsVec()
    {
        return (ZunVec3 *)&this->args;
    }
};
static_assert(sizeof(EclTimelineInstrArgs) == 0x18);

struct EclTimelineInstr
{
    i16 time;
    i16 arg0;
    i16 opcode;
    i16 size;
    EclTimelineInstrArgs args;
};
static_assert(sizeof(EclTimelineInstr) == 0x20);

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

extern EclGlobalVars g_GlobalEclVars;

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
    ZunResult CallEclSub(struct EnemyEclContext *ctx, i16 subId);
    static i32 *GetVar(Enemy *enemy, i32 *eclVarId, u16 paramMask, i32 param_4);
    static i32 GetVarValue(Enemy *enemy, i32 eclVarId);
    static f32 *GetFloatVar(Enemy *enemy, f32 *eclVarFloat, u16 paramMask, i32 param_4);
    static f32 GetFloatVarValue(Enemy *enemy, f32 eclVarFloat);
    static void MoveDirTime(Enemy *enemy, EclRawInstr *instr);
    static void MovePosTime(Enemy *enemy, EclRawInstr *instr);
    static void MathLerp(Enemy *enemy, EclInterp *interp, f32 t);
    static void MathCubicInterp(Enemy *enemy, EclInterp *interp, f32 t);
    static void BeginSpellcard(Enemy *enemy, EclRawInstr *instr);
    static void EndSpellcard();
    ZunResult RunEcl(Enemy *enemy);

    EclRawHeader *eclFile;
    EclTimelineInstr *timelinePtr[16];
    EclRawInstr **subTable;

    EclTimelineInstr *GetTimeline(i32 idx)
    {
        return this->timelinePtr[idx];
    }
};
extern EclManager g_EclManager;
