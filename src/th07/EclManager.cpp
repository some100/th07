#include "EclManager.hpp"

#include <stdio.h>

#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "EnemyEclInstr.hpp"
#include "EnemyManager.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "Player.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "ZunMath.hpp"
#include "dsutil.hpp"

#define GET_INT_PTR(enemy, argIdx) \
    GetVar(enemy, &instr->args[argIdx].i, instr->paramMask, argIdx)

#define GET_FLOAT_PTR(enemy, argIdx) \
    GetFloatVar(enemy, &instr->args[argIdx].f, instr->paramMask, argIdx)

#define GET_INT_VALUE(enemy, argIdx) \
    (((instr->paramMask & (1 << argIdx)) != 0) ? GetVarValue(enemy, instr->args[argIdx].i) : instr->args[argIdx].i)

#define GET_FLOAT_VALUE(enemy, argIdx) \
    (((instr->paramMask & (1 << argIdx)) != 0) ? GetFloatVarValue(enemy, instr->args[argIdx].f) : instr->args[argIdx].f)

#define GET_INT_VALUE_D(enemy, args, argIdx, bitIdx) \
    (((instr->paramMask & (1 << bitIdx)) != 0) ? GetVarValue(enemy, args[argIdx].i) : args[argIdx].i)

#define GET_FLOAT_VALUE_D(enemy, args, argIdx, bitIdx) \
    (((instr->paramMask & (1 << bitIdx)) != 0) ? GetFloatVarValue(enemy, args[argIdx].f) : args[argIdx].f)

// GLOBAL: TH07 0x0049f560
const char *g_EclPaths[10] = {
    "dummy",
    // STRING: TH07 0x00497f38
    "data/ecldata1.ecl",
    // STRING: TH07 0x00497f24
    "data/ecldata2.ecl",
    // STRING: TH07 0x00497f10
    "data/ecldata3.ecl",
    // STRING: TH07 0x00497efc
    "data/ecldata4.ecl",
    // STRING: TH07 0x00497ee8
    "data/ecldata5.ecl",
    // STRING: TH07 0x00497ed4
    "data/ecldata6.ecl",
    // STRING: TH07 0x00497ec0
    "data/ecldata7.ecl",
    // STRING: TH07 0x00497eac
    "data/ecldata8.ecl",
    NULL,
};

// GLOBAL: TH07 0x01347938
EclManager g_EclManager;

// GLOBAL: TH07 0x01347aa0
EclGlobalVars g_GlobalEclVars;

// FUNCTION: TH07 0x0040e420
ZunResult EclManager::Load(const char *path)
{
    i32 i;

    this->eclFile = (EclRawHeader *)FileSystem::OpenFile(path, 0);
    if (!this->eclFile)
    {
        // STRING: TH07 0x00498700
        g_GameErrorContext.Log("敵データの読み込みに失敗しました、データが壊れてるか失われています\r\n");
        return ZUN_ERROR;
    }

    for (i = 0; i < 16; i++)
    {
        this->eclFile->timelinePtr[i] =
            (EclTimelineInstr *)((i32)this->eclFile->timelinePtr[i] + (i32)this->eclFile);
    }
    this->subTable = this->eclFile->subTable;
    for (i = 0; i < this->eclFile->subCount; i++)
    {
        this->subTable[i] =
            (EclRawInstr *)((i32)this->subTable[i] + (i32)this->eclFile);
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0040e4f0
void EclManager::Unload()
{
    if (this->eclFile)
    {
        ZunMemory::Free(this->eclFile);
    }
    this->eclFile = NULL;
}

// FUNCTION: TH07 0x0040e530
ZunResult EclManager::CallEclSub(EnemyEclContext *ctx, i16 subId)
{
    ctx->curInstr = this->subTable[subId];
    ctx->time = 0;
    ctx->timer2 = 0;
    ctx->subId = subId;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0040e5b0
i32 EclManager::GetVarValue(Enemy *enemy, i32 eclVar)
{
    switch (eclVar)
    {
    case VAR_LOCAL_INT1_1:
        return enemy->currentContext.eclContextArgs.intVars1[0];
    case VAR_LOCAL_INT1_2:
        return enemy->currentContext.eclContextArgs.intVars1[1];
    case VAR_LOCAL_INT1_3:
        return enemy->currentContext.eclContextArgs.intVars1[2];
    case VAR_LOCAL_INT1_4:
        return enemy->currentContext.eclContextArgs.intVars1[3];
    case VAR_LOCAL_INT3_1:
        return enemy->currentContext.eclContextArgs.globalVars.intVars[0];
    case VAR_LOCAL_INT3_2:
        return enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    case VAR_LOCAL_INT3_3:
        return enemy->currentContext.eclContextArgs.globalVars.intVars[2];
    case VAR_LOCAL_INT3_4:
        return enemy->currentContext.eclContextArgs.globalVars.intVars[3];
    case VAR_LOCAL_INT2_1:
        return enemy->currentContext.eclContextArgs.intVars2[0];
    case VAR_LOCAL_INT2_2:
        return enemy->currentContext.eclContextArgs.intVars2[1];
    case VAR_LOCAL_INT2_3:
        return enemy->currentContext.eclContextArgs.intVars2[2];
    case VAR_LOCAL_INT2_4:
        return enemy->currentContext.eclContextArgs.intVars2[3];
    case VAR_DIFFICULTY:
        return g_GameManager.difficulty;
    case VAR_RANK:
        return g_GameManager.rank.rank;
    case VAR_CUR_TIME:
        return enemy->timer.current;
    case VAR_LIFE:
        return enemy->life;
    case VAR_PLAYER_SHOTTYPE:
        return g_GameManager.shotTypeAndCharacter;
    case VAR_LOCAL_FLOAT2_1:
        return enemy->currentContext.eclContextArgs.floatVars2[0];
    case VAR_LOCAL_FLOAT2_2:
        return enemy->currentContext.eclContextArgs.floatVars2[1];
    case VAR_LOCAL_FLOAT1_1:
        return enemy->currentContext.eclContextArgs.floatVars1[0];
    case VAR_LOCAL_FLOAT1_2:
        return enemy->currentContext.eclContextArgs.floatVars1[1];
    case VAR_LOCAL_FLOAT1_3:
        return enemy->currentContext.eclContextArgs.floatVars1[2];
    case VAR_LOCAL_FLOAT1_4:
        return enemy->currentContext.eclContextArgs.floatVars1[3];
    case VAR_LOCAL_FLOAT1_5:
        return enemy->currentContext.eclContextArgs.floatVars1[4];
    case VAR_LOCAL_FLOAT1_6:
        return enemy->currentContext.eclContextArgs.floatVars1[5];
    case VAR_LOCAL_FLOAT1_7:
        return enemy->currentContext.eclContextArgs.floatVars1[6];
    case VAR_LOCAL_FLOAT1_8:
        return enemy->currentContext.eclContextArgs.floatVars1[7];
    case VAR_LOCAL_FLOAT3_1:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[0];
    case VAR_LOCAL_FLOAT3_2:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    case VAR_LOCAL_FLOAT3_3:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[2];
    case VAR_LOCAL_FLOAT3_4:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[3];
    case VAR_GLOBAL_INT_1:
        return g_GlobalEclVars.intVars[0];
    case VAR_GLOBAL_INT_2:
        return g_GlobalEclVars.intVars[1];
    case VAR_GLOBAL_INT_3:
        return g_GlobalEclVars.intVars[2];
    case VAR_GLOBAL_INT_4:
        return g_GlobalEclVars.intVars[3];
    case VAR_GLOBAL_FLOAT_1:
        return g_GlobalEclVars.floatVars[0];
    case VAR_GLOBAL_FLOAT_2:
        return g_GlobalEclVars.floatVars[1];
    case VAR_GLOBAL_FLOAT_3:
        return g_GlobalEclVars.floatVars[2];
    case VAR_GLOBAL_FLOAT_4:
        return g_GlobalEclVars.floatVars[3];
    case VAR_POS_X:
        return enemy->pos.x;
    case VAR_POS_Y:
        return enemy->pos.y;
    case VAR_POS_Z:
        return enemy->pos.z;
    case VAR_PLAYER_POS_X:
        return g_Player.positionCenter.x;
    case VAR_PLAYER_POS_Y:
        return g_Player.positionCenter.y;
    case VAR_PLAYER_POS_Z:
        return g_Player.positionCenter.z;
    case VAR_MOVE_INTERP_ORIGIN_X:
        return enemy->moveInterpStartPos.x;
    case VAR_MOVE_INTERP_ORIGIN_Y:
        return enemy->moveInterpStartPos.y;
    case VAR_MOVE_INTERP_ORIGIN_Z:
        return enemy->moveInterpStartPos.z;
    case VAR_DELTA_POS_X:
        return enemy->deltaPos.x;
    case VAR_DELTA_POS_Y:
        return enemy->deltaPos.y;
    case VAR_DELTA_POS_Z:
        return enemy->deltaPos.z;
    case VAR_BOSS_LIFE_THRESHOLD1:
        return enemy->lifeCallbackThreshold[0];
    case VAR_BOSS_LIFE_THRESHOLD2:
        return enemy->lifeCallbackThreshold[1];
    case VAR_BOSS_LIFE_THRESHOLD3:
        return enemy->lifeCallbackThreshold[2];
    case VAR_BOSS_LIFE_THRESHOLD4:
        return enemy->lifeCallbackThreshold[3];
    case VAR_ANGLE:
        return enemy->angle;
    case VAR_ANGULAR_VELOCITY:
        return enemy->angularVelocity;
    case VAR_MOVE_SPEED:
        return enemy->moveSpeed;
    case VAR_MOVE_ACCELERATION:
        return enemy->moveAcceleration;
    case VAR_MOVE_RADIUS:
        return enemy->moveRadius;
    case VAR_MOVE_ANGLE:
        return enemy->moveAngle;
    case VAR_MOVE_ANGULAR_VELOCITY:
        return enemy->moveAngularVelocity;
    case VAR_RNG:
        return g_Rng.GetRandomU32();
    case VAR_RNG_CUSTOM_BOUND:
        return g_Rng.GetRandomU32InRange(
                   enemy->currentContext.eclContextArgs.globalVars.intVars[0]) +
               enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    case VAR_LAST_DAMAGE:
        return enemy->lastDamage;
    case VAR_BOSS_ID:
        return enemy->bossId;
    case VAR_ITEMDROP:
        return enemy->itemDrop;
    case VAR_SCORE:
        return enemy->score;
    case VAR_ANGLE_TO_PLAYER:
        return g_Player.AngleToPlayer(&enemy->pos);
    case VAR_DISTANCE_FROM_PLAYER:
        return D3DXVec3Length((g_Player.positionCenter - enemy->pos).asD3DX());
    default:
        return eclVar;
    }
}

// FUNCTION: TH07 0x0040ec00
i32 *EclManager::GetVar(Enemy *enemy, i32 *eclVar, u16 paramMask, i32 param_4)
{
    if (param_4 >= 0 && ((u32)paramMask & 1 << param_4) == 0)
    {
        return eclVar;
    }

    switch (*eclVar)
    {
    case VAR_LOCAL_INT1_1:
        return &enemy->currentContext.eclContextArgs.intVars1[0];
    case VAR_LOCAL_INT1_2:
        return &enemy->currentContext.eclContextArgs.intVars1[1];
    case VAR_LOCAL_INT1_3:
        return &enemy->currentContext.eclContextArgs.intVars1[2];
    case VAR_LOCAL_INT1_4:
        return &enemy->currentContext.eclContextArgs.intVars1[3];
    case VAR_LOCAL_INT3_1:
        return &enemy->currentContext.eclContextArgs.globalVars.intVars[0];
    case VAR_LOCAL_INT3_2:
        return &enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    case VAR_LOCAL_INT3_3:
        return &enemy->currentContext.eclContextArgs.globalVars.intVars[2];
    case VAR_LOCAL_INT3_4:
        return &enemy->currentContext.eclContextArgs.globalVars.intVars[3];
    case VAR_LOCAL_INT2_1:
        return &enemy->currentContext.eclContextArgs.intVars2[0];
    case VAR_LOCAL_INT2_2:
        return &enemy->currentContext.eclContextArgs.intVars2[1];
    case VAR_LOCAL_INT2_3:
        return &enemy->currentContext.eclContextArgs.intVars2[2];
    case VAR_LOCAL_INT2_4:
        return &enemy->currentContext.eclContextArgs.intVars2[3];
    case VAR_DIFFICULTY:
        return &g_GameManager.difficulty;
    case VAR_RANK:
        return &g_GameManager.rank.rank;
    case VAR_CUR_TIME:
        return &enemy->timer.current;
    case VAR_LIFE:
        return &enemy->life;
    case VAR_ITEMDROP:
        return &enemy->itemDrop;
    case VAR_SCORE:
        return &enemy->score;
    case VAR_GLOBAL_INT_1:
        return &g_GlobalEclVars.intVars[0];
    case VAR_GLOBAL_INT_2:
        return &g_GlobalEclVars.intVars[1];
    case VAR_GLOBAL_INT_3:
        return &g_GlobalEclVars.intVars[2];
    case VAR_GLOBAL_INT_4:
        return &g_GlobalEclVars.intVars[3];
    default:
        return eclVar;
    }
}

// FUNCTION: TH07 0x0040edf0
f32 EclManager::GetFloatVarValue(Enemy *enemy, f32 eclVar)
{
    switch ((i32)eclVar)
    {
    case VAR_LOCAL_INT1_1:
        return (f32)enemy->currentContext.eclContextArgs.intVars1[0];
    case VAR_LOCAL_INT1_2:
        return (f32)enemy->currentContext.eclContextArgs.intVars1[1];
    case VAR_LOCAL_INT1_3:
        return (f32)enemy->currentContext.eclContextArgs.intVars1[2];
    case VAR_LOCAL_INT1_4:
        return (f32)enemy->currentContext.eclContextArgs.intVars1[3];
    case VAR_LOCAL_INT3_1:
        return (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[0];
    case VAR_LOCAL_INT3_2:
        return (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    case VAR_LOCAL_INT3_3:
        return (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[2];
    case VAR_LOCAL_INT3_4:
        return (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[3];
    case VAR_LOCAL_INT2_1:
        return (f32)enemy->currentContext.eclContextArgs.intVars2[0];
    case VAR_LOCAL_INT2_2:
        return (f32)enemy->currentContext.eclContextArgs.intVars2[1];
    case VAR_LOCAL_INT2_3:
        return (f32)enemy->currentContext.eclContextArgs.intVars2[2];
    case VAR_LOCAL_INT2_4:
        return (f32)enemy->currentContext.eclContextArgs.intVars2[3];
    case VAR_DIFFICULTY:
        return (f32)g_GameManager.difficulty;
    case VAR_RANK:
        return (f32)g_GameManager.rank.rank;
    case VAR_CUR_TIME:
        return (f32)enemy->timer.current;
    case VAR_LIFE:
        return (f32)enemy->life;
    case VAR_PLAYER_SHOTTYPE:
        return (f32)g_GameManager.shotTypeAndCharacter;
    case VAR_ITEMDROP:
        return (f32)enemy->itemDrop;
    case VAR_SCORE:
        return (f32)enemy->score;
    case VAR_GLOBAL_INT_1:
        return (f32)g_GlobalEclVars.intVars[0];
    case VAR_GLOBAL_INT_2:
        return (f32)g_GlobalEclVars.intVars[1];
    case VAR_GLOBAL_INT_3:
        return (f32)g_GlobalEclVars.intVars[2];
    case VAR_GLOBAL_INT_4:
        return (f32)g_GlobalEclVars.intVars[3];
    case VAR_GLOBAL_FLOAT_1:
        return g_GlobalEclVars.floatVars[0];
    case VAR_GLOBAL_FLOAT_2:
        return g_GlobalEclVars.floatVars[1];
    case VAR_GLOBAL_FLOAT_3:
        return g_GlobalEclVars.floatVars[2];
    case VAR_GLOBAL_FLOAT_4:
        return g_GlobalEclVars.floatVars[3];
    case VAR_LOCAL_FLOAT1_1:
        return enemy->currentContext.eclContextArgs.floatVars1[0];
    case VAR_LOCAL_FLOAT1_2:
        return enemy->currentContext.eclContextArgs.floatVars1[1];
    case VAR_LOCAL_FLOAT1_3:
        return enemy->currentContext.eclContextArgs.floatVars1[2];
    case VAR_LOCAL_FLOAT1_4:
        return enemy->currentContext.eclContextArgs.floatVars1[3];
    case VAR_LOCAL_FLOAT1_5:
        return enemy->currentContext.eclContextArgs.floatVars1[4];
    case VAR_LOCAL_FLOAT1_6:
        return enemy->currentContext.eclContextArgs.floatVars1[5];
    case VAR_LOCAL_FLOAT1_7:
        return enemy->currentContext.eclContextArgs.floatVars1[6];
    case VAR_LOCAL_FLOAT1_8:
        return enemy->currentContext.eclContextArgs.floatVars1[7];
    case VAR_LOCAL_FLOAT3_1:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[0];
    case VAR_LOCAL_FLOAT3_2:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    case VAR_LOCAL_FLOAT3_3:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[2];
    case VAR_LOCAL_FLOAT3_4:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[3];
    case VAR_POS_X:
        return enemy->pos.x;
    case VAR_POS_Y:
        return enemy->pos.y;
    case VAR_POS_Z:
        return enemy->pos.z;
    case VAR_PLAYER_POS_X:
        return g_Player.positionCenter.x;
    case VAR_PLAYER_POS_Y:
        return g_Player.positionCenter.y;
    case VAR_PLAYER_POS_Z:
        return g_Player.positionCenter.z;
    case VAR_LOCAL_FLOAT2_1:
        return enemy->currentContext.eclContextArgs.floatVars2[0];
    case VAR_LOCAL_FLOAT2_2:
        return enemy->currentContext.eclContextArgs.floatVars2[1];
    case VAR_MOVE_INTERP_ORIGIN_X:
        return enemy->moveInterpStartPos.x;
    case VAR_MOVE_INTERP_ORIGIN_Y:
        return enemy->moveInterpStartPos.y;
    case VAR_MOVE_INTERP_ORIGIN_Z:
        return enemy->moveInterpStartPos.z;
    case VAR_MOVE_INTERP_TARGET_X:
        return enemy->moveInterp.x;
    case VAR_MOVE_INTERP_TARGET_Y:
        return enemy->moveInterp.y;
    case VAR_MOVE_INTERP_TARGET_Z:
        return enemy->moveInterp.z;
    case VAR_DELTA_POS_X:
        return enemy->deltaPos.x;
    case VAR_DELTA_POS_Y:
        return enemy->deltaPos.y;
    case VAR_DELTA_POS_Z:
        return enemy->deltaPos.z;
    case VAR_BOSS_LIFE_THRESHOLD1:
        return (f32)enemy->lifeCallbackThreshold[0];
    case VAR_BOSS_LIFE_THRESHOLD2:
        return (f32)enemy->lifeCallbackThreshold[1];
    case VAR_BOSS_LIFE_THRESHOLD3:
        return (f32)enemy->lifeCallbackThreshold[2];
    case VAR_BOSS_LIFE_THRESHOLD4:
        return (f32)enemy->lifeCallbackThreshold[3];
    case VAR_ANGLE_TO_PLAYER:
        return g_Player.AngleToPlayer(&enemy->pos);
    case VAR_ANGLE:
        return enemy->angle;
    case VAR_ANGULAR_VELOCITY:
        return enemy->angularVelocity;
    case VAR_MOVE_SPEED:
        return enemy->moveSpeed;
    case VAR_MOVE_ACCELERATION:
        return enemy->moveAcceleration;
    case VAR_MOVE_RADIUS:
        return enemy->moveRadius;
    case VAR_MOVE_ANGLE:
        return enemy->moveAngle;
    case VAR_MOVE_ANGULAR_VELOCITY:
        return enemy->moveAngularVelocity;
    case VAR_RNG:
        return g_Rng.GetRandomFloat();
    case VAR_RNG_CUSTOM_BOUND:
        return g_Rng.GetRandomFloatInRange(enemy->currentContext.eclContextArgs.globalVars.floatVars[0]) +
               enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    case VAR_RNG_RADIAN:
        return g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    case VAR_BOSS_ID:
        return (f32)enemy->bossId;
    case VAR_LAST_DAMAGE:
        return (f32)enemy->lastDamage;
    case VAR_DISTANCE_FROM_PLAYER:
        return D3DXVec3Length((g_Player.positionCenter - enemy->pos).asD3DX());
    default:
        return eclVar;
    }
}

// FUNCTION: TH07 0x0040f3c0
f32 *EclManager::GetFloatVar(Enemy *enemy, f32 *eclVar, u16 paramMask,
                             i32 param_4)
{
    if (param_4 >= 0 && ((u32)paramMask & 1 << param_4) == 0)
    {
        return eclVar;
    }

    switch ((i32)*eclVar)
    {
    case VAR_LOCAL_FLOAT1_1:
        return &enemy->currentContext.eclContextArgs.floatVars1[0];
    case VAR_LOCAL_FLOAT1_2:
        return &enemy->currentContext.eclContextArgs.floatVars1[1];
    case VAR_LOCAL_FLOAT1_3:
        return &enemy->currentContext.eclContextArgs.floatVars1[2];
    case VAR_LOCAL_FLOAT1_4:
        return &enemy->currentContext.eclContextArgs.floatVars1[3];
    case VAR_LOCAL_FLOAT1_5:
        return &enemy->currentContext.eclContextArgs.floatVars1[4];
    case VAR_LOCAL_FLOAT1_6:
        return &enemy->currentContext.eclContextArgs.floatVars1[5];
    case VAR_LOCAL_FLOAT1_7:
        return &enemy->currentContext.eclContextArgs.floatVars1[6];
    case VAR_LOCAL_FLOAT1_8:
        return &enemy->currentContext.eclContextArgs.floatVars1[7];
    case VAR_LOCAL_FLOAT3_1:
        return &enemy->currentContext.eclContextArgs.globalVars.floatVars[0];
    case VAR_LOCAL_FLOAT3_2:
        return &enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    case VAR_LOCAL_FLOAT3_3:
        return &enemy->currentContext.eclContextArgs.globalVars.floatVars[2];
    case VAR_LOCAL_FLOAT3_4:
        return &enemy->currentContext.eclContextArgs.globalVars.floatVars[3];
    case VAR_POS_X:
        return &enemy->pos.x;
    case VAR_POS_Y:
        return &enemy->pos.y;
    case VAR_POS_Z:
        return &enemy->pos.z;
    case VAR_PLAYER_POS_X:
        return &g_Player.positionCenter.x;
    case VAR_PLAYER_POS_Y:
        return &g_Player.positionCenter.y;
    case VAR_PLAYER_POS_Z:
        return &g_Player.positionCenter.z;
    case VAR_LOCAL_FLOAT2_1:
        return &enemy->currentContext.eclContextArgs.floatVars2[0];
    case VAR_LOCAL_FLOAT2_2:
        return &enemy->currentContext.eclContextArgs.floatVars2[1];
    case VAR_GLOBAL_FLOAT_1:
        return &g_GlobalEclVars.floatVars[0];
    case VAR_GLOBAL_FLOAT_2:
        return &g_GlobalEclVars.floatVars[1];
    case VAR_GLOBAL_FLOAT_3:
        return &g_GlobalEclVars.floatVars[2];
    case VAR_GLOBAL_FLOAT_4:
        return &g_GlobalEclVars.floatVars[3];
    case VAR_MOVE_INTERP_ORIGIN_X:
        return &enemy->moveInterpStartPos.x;
    case VAR_MOVE_INTERP_ORIGIN_Y:
        return &enemy->moveInterpStartPos.y;
    case VAR_MOVE_INTERP_ORIGIN_Z:
        return &enemy->moveInterpStartPos.z;
    case VAR_MOVE_INTERP_TARGET_X:
        return &enemy->moveInterp.x;
    case VAR_MOVE_INTERP_TARGET_Y:
        return &enemy->moveInterp.y;
    case VAR_MOVE_INTERP_TARGET_Z:
        return &enemy->moveInterp.z;
    case VAR_ANGLE:
        return &enemy->angle;
    case VAR_ANGULAR_VELOCITY:
        return &enemy->angularVelocity;
    case VAR_MOVE_SPEED:
        return &enemy->moveSpeed;
    case VAR_MOVE_ACCELERATION:
        return &enemy->moveAcceleration;
    case VAR_MOVE_RADIUS:
        return &enemy->moveRadius;
    case VAR_MOVE_ANGLE:
        return &enemy->moveAngle;
    case VAR_MOVE_ANGULAR_VELOCITY:
        return &enemy->moveAngularVelocity;
    default:
        return eclVar;
    }
}

// FUNCTION: TH07 0x0040f6b0
void EclManager::MoveDirTime(Enemy *enemy, EclRawInstr *instr)
{
    f32 fVar2;

    fVar2 = utils::AddNormalizeAngle(GET_FLOAT_VALUE(enemy, 2), 0.0f);
    enemy->moveInterp.x = cosf(fVar2) * GET_FLOAT_VALUE(enemy, 3) *
                          (f32)GET_INT_VALUE(enemy, 0);
    enemy->moveInterp.y = sinf(fVar2) * GET_FLOAT_VALUE(enemy, 3) *
                          (f32)GET_INT_VALUE(enemy, 0);
    enemy->moveInterp.z = 0.0f;
    enemy->moveInterpStartPos = enemy->pos;
    enemy->moveInterpTimer = enemy->moveInterpStartTime =
        GET_INT_VALUE(enemy, 0);
    enemy->interpEasing = (u8)GET_INT_VALUE(enemy, 1);
    enemy->moveMode = 2;
    if (enemy->mirror)
    {
        enemy->moveInterp.x = -enemy->moveInterp.x;
    }
}

// FUNCTION: TH07 0x0040f8f0
void EclManager::MovePosTime(Enemy *enemy, EclRawInstr *instr)
{
    Float3 newPos;
    newPos.x = GET_FLOAT_VALUE(enemy, 2);
    newPos.y = GET_FLOAT_VALUE(enemy, 3);
    newPos.z = GET_FLOAT_VALUE(enemy, 4);

    enemy->moveInterp = newPos - enemy->pos;
    enemy->moveInterpStartPos = enemy->pos;
    enemy->moveInterpTimer = enemy->moveInterpStartTime = GET_INT_VALUE(enemy, 0);
    enemy->interpEasing = (u8)GET_INT_VALUE(enemy, 1);
    enemy->moveMode = 2;
    enemy->axisSpeed = Float3(0.0f, 0.0f, 0.0f);
    if (enemy->mirror)
    {
        enemy->moveInterp.x = -enemy->moveInterp.x;
    }
}

// FUNCTION: TH07 0x0040fb30
#pragma var_order(b, a)
void EclManager::MathLerp(Enemy *enemy, EclInterp *interp, f32 t)
{
    f32 a = GetFloatVarValue(enemy, interp->args[3].f);
    f32 b = GetFloatVarValue(enemy, interp->args[4].f);

    *GetFloatVar(enemy, &interp->args[7].f, 0, -1) = (b - a) * t + a;
}

#pragma var_order(h11, m1, h01, m0, p1, h10, h00, p0)
// FUNCTION: TH07 0x0040fb90
void EclManager::MathCubicInterp(Enemy *enemy, EclInterp *interp, f32 t)
{
    float h11;
    float m1;
    float h01;
    float m0;
    float p1;
    float h10;
    float h00;
    float p0;

    p0 = GetFloatVarValue(enemy, interp->args[3].f);
    p1 = GetFloatVarValue(enemy, interp->args[4].f);
    m0 = GetFloatVarValue(enemy, interp->args[5].f);
    m1 = GetFloatVarValue(enemy, interp->args[6].f);

    h00 = (t - 1.0f) * (t - 1.0f) * (2.0f * t + 1.0f);
    h01 = t * t * (3.0f - 2.0f * t);
    h10 = (1.0f - t) * (1.0f - t) * t;
    h11 = (t - 1.0f) * t * t;

    *GetFloatVar(enemy, &interp->args[7].f, 0, -1) = h00 * p0 +
                                                     h01 * p1 +
                                                     h10 * m0 +
                                                     h11 * m1;
}

#pragma var_order(i, spellcardName, catk, j, nameCsum, newCsum)
// FUNCTION: TH07 0x0040fc90
void EclManager::BeginSpellcard(Enemy *enemy, EclRawInstr *instr)
{
    i32 newCsum;
    i32 nameCsum;
    i32 j;
    Catk *catk;
    char spellcardName[48];
    i32 i;

    memcpy(spellcardName, &instr->args[1], sizeof(spellcardName));
    for (i = 0; (u32)i < 48; i++)
    {
        spellcardName[i] = (u8)spellcardName[i] ^ 0xaa;
    }
    g_Gui.ShowSpellcard(instr->args[0].s[0], spellcardName);
    g_BulletManager.RemoveAllBullets(1);
    g_Stage.spellCardState = 1;
    g_Stage.ticksSinceSpellcardStarted = 0;
    for (i = 0; i < g_Stage.numSpellcardVms; i++)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(
            &g_Stage.spellcardVms[i], i + g_Stage.spellcardVmsIdx + 732);
    }
    g_EnemyManager.spellcardInfo.isActive = 1;
    g_EnemyManager.spellcardInfo.isCapturing = 1;
    g_EnemyManager.spellcardInfo.spellcardIdx =
        instr->args[0].us[1];
    g_EnemyManager.spellcardInfo.captureScore =
        g_SpellcardScore[g_EnemyManager.spellcardInfo.spellcardIdx];
    g_EnemyManager.spellcardInfo.grazeBonusScore = 0;
    g_EnemyManager.spellcardInfo.scoreDrainRate =
        g_EnemyManager.spellcardInfo.captureScore /
        (enemy->timerCallbackThreshold / 60 + 10);
    g_EnemyManager.timer = 0;
    enemy->bulletRankSpeedLow = -0.5f;
    enemy->bulletRankSpeedHigh = 0.5f;
    enemy->bulletRankAmount1Low = 0;
    enemy->bulletRankAmount1High = 0;
    enemy->bulletRankAmount2Low = 0;
    enemy->bulletRankAmount2High = 0;
    enemy->specialEffect =
        g_EffectManager.SpawnEffect(25, &enemy->pos, 1, 1, 0xffffffff);
    enemy->specialEffect->vm.interpStartTimes[4] = 0;
    enemy->specialEffect->vm.interpEndTimes[4] = enemy->timerCallbackThreshold;
    enemy->specialEffect->vm.interpModes[4] = 0;
    enemy->specialEffect->vm.scaleInterpInitial = enemy->specialEffect->vm.scale;
    enemy->specialEffect->vm.scaleInterpFinal.x = 0.125;
    enemy->specialEffect->vm.scaleInterpFinal.y = 0.125;
    enemy->specialEffect->pos1 = enemy->pos;
    enemy->customSpecialEffectPos = 0;
    if (!g_GameManager.replay)
    {
        catk = &g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx];
        nameCsum = 0;
        strcpy(catk->name, spellcardName);
        j = (i32)strlen(catk->name);
        while (0 < j)
        {
            j--;
            nameCsum += catk->name[j];
        }
        newCsum = nameCsum;
        for (j = 0; j < 7; j++)
        {
            nameCsum += catk->numSuccessesPerShot[j];
            nameCsum += catk->numAttemptsPerShot[j];
            nameCsum += catk->highScorePerShot[j];
        }
        if (catk->nameCsum != (u8)nameCsum)
        {
            for (j = 0; j < 7; j++)
            {
                catk->numSuccessesPerShot[j] = 0;
                catk->numAttemptsPerShot[j] = 0;
                catk->highScorePerShot[j] = 0;
            }
        }
        if (catk->numAttemptsPerShot[g_GameManager.shotTypeAndCharacter] < 9999)
        {
            catk->numAttemptsPerShot[g_GameManager.shotTypeAndCharacter]++;
        }
        if (catk->numAttemptsPerShot[6] < 9999)
        {
            catk->numAttemptsPerShot[6]++;
        }
        for (j = 0; j < 7; j++)
        {
            newCsum += catk->numSuccessesPerShot[j];
            newCsum += catk->numAttemptsPerShot[j];
            newCsum += catk->highScorePerShot[j];
        }
        catk->nameCsum = (u8)newCsum;
    }
}

#pragma var_order(score, catk, i, nameCsum, newCsum, j)
// FUNCTION: TH07 0x004101a0
void EclManager::EndSpellcard(Enemy *enemy, EclRawInstr *instr)
{
    i32 j;
    i32 newCsum;
    i32 nameCsum;
    u32 character;
    i32 i;
    Catk *catk;
    i32 score;

    if (g_EnemyManager.spellcardInfo.isActive)
    {
        g_Gui.EndEnemySpellcard();
        if (g_EnemyManager.spellcardInfo.isActive == 1)
        {
            score = g_BulletManager.DespawnBullets(8000, 1);
            score = g_EnemyManager.RemoveAllEnemies(8000, score);
            if (score != 0)
            {
                g_GameManager.AddScore(score);
                g_Gui.ShowBonusScore(score);
            }
            if (g_EnemyManager.spellcardInfo.isCapturing)
            {
                catk = &g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx];
                score = g_EnemyManager.spellcardInfo.captureScore +
                        g_EnemyManager.spellcardInfo.grazeBonusScore;
                g_Gui.ShowSpellcardBonus(score);
                g_GameManager.AddScore(score);
                if (!g_GameManager.replay)
                {
                    nameCsum = 0;
                    i = strlen(catk->name);
                    while (0 < i)
                    {
                        i--;
                        nameCsum += catk->name[i];
                    }
                    newCsum = nameCsum;
                    for (i = 0; i < 7; i++)
                    {
                        nameCsum += catk->numSuccessesPerShot[i];
                        nameCsum += catk->numAttemptsPerShot[i];
                        nameCsum += catk->highScorePerShot[i];
                    }
                    if (catk->nameCsum != (u8)nameCsum)
                    {
                        for (i = 0; i < 7; i++)
                        {
                            catk->numSuccessesPerShot[i] = 0;
                            catk->numAttemptsPerShot[i] = 0;
                            catk->highScorePerShot[i] = 0;
                        }
                    }
                    character = g_GameManager.shotTypeAndCharacter;
                    if (catk->highScorePerShot[character] < (u32)score)
                    {
                        catk->highScorePerShot[character] = score;
                    }
                    if (catk->highScorePerShot[6] < (u32)score)
                    {
                        catk->highScorePerShot[6] = score;
                    }
                    if (catk->numSuccessesPerShot[character] < 9999)
                    {
                        catk->numSuccessesPerShot[character]++;
                    }
                    if (catk->numSuccessesPerShot[6] < 9999)
                    {
                        catk->numSuccessesPerShot[6]++;
                    }
                    for (i = 0; i < 7; i++)
                    {
                        newCsum += catk->numSuccessesPerShot[i];
                        newCsum += catk->numAttemptsPerShot[i];
                        newCsum += catk->highScorePerShot[i];
                    }
                    catk->nameCsum = (u8)newCsum;
                }
                g_GameManager.globals->spellCardsCaptured++;
            }
        }
        g_EnemyManager.spellcardInfo.isActive = 0;
        for (j = 0; j < 8; j++)
        {
            if (g_EnemyManager.bosses[j] &&
                g_EnemyManager.bosses[j]->specialEffect != NULL)
            {
                g_EnemyManager.bosses[j]->specialEffect->inUseFlag = 0;
                g_EnemyManager.bosses[j]->specialEffect = NULL;
            }
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_ENEMY_SPELLCARD_END, 0);
    }
    g_Stage.spellCardState = 0;
}

#pragma var_order(local_8, instr, lerpDelta, interpIdx, interp, bulletInstrArgs,                              \
                  bulletProps, bulletCommand, laserProps, laserInstrArgs, laserIdx, effectInstrArgs,          \
                  exitAngle, healthIdx, bossIdx, particleVel, numDrops, itemDropIdx,                          \
                  itemDropPos, numPointItems, pointItemIdx, pointItemPos, unusedEnemyAbs, absSpawnInstrArgs,  \
                  absEnemySpawnPos, unusedEnemyRel, relSpawnInstrArgs, relEnemySpawnPos, local_d8, unused_e4, \
                  t1, anmDirection, interpIdx2, t2, local_f8, interp2)
// FUNCTION: TH07 0x00410520
ZunResult EclManager::RunEcl(Enemy *enemy)
{
    EclInterp *interp2;
    i32 local_f8;
    f32 t2;
    i32 interpIdx2;
    u32 anmDirection;
    f32 t1;
    Float3 unused_e4;
    Float3 local_d8;
    Float3 relEnemySpawnPos;
    AnyArg relSpawnInstrArgs[7];
    Enemy *unusedEnemyRel;
    Float3 absEnemySpawnPos;
    AnyArg absSpawnInstrArgs[7];
    Enemy *unusedEnemyAbs;
    Float3 pointItemPos;
    i32 pointItemIdx;
    i32 numPointItems;
    Float3 itemDropPos;
    i32 itemDropIdx;
    i32 numDrops;
    Float3 particleVel;
    i32 bossIdx;
    i32 healthIdx;
    f32 exitAngle;
    AnyArg *effectInstrArgs;
    i32 laserIdx;
    AnyArg *laserInstrArgs;
    EnemyLaserShooter *laserProps;
    BulletCommand *bulletCommand;
    EnemyBulletShooter *bulletProps;
    AnyArg *bulletInstrArgs;
    EclInterp *interp;
    i32 interpIdx;
    f32 lerpDelta;
    EclRawInstr *instr;
    i32 local_8;

restart:
    instr = enemy->currentContext.curInstr;
    if (enemy->runInterrupt >= 0)
    {
        goto handle_interrupt;
    }

    if (enemy->periodicCallbackSub >= 0)
    {
        enemy->periodicCounter++;
        if (enemy->periodicCounter.GetCurrent() >= enemy->periodicTimer.GetCurrent())
        {
            enemy->periodicCounter = 0;
            enemy->savedContextStack[enemy->stackDepth] = enemy->currentContext;
            enemy->currentContext.eclContextArgs = enemy->savedEclContextArgs;
            g_EclManager.CallEclSub(&enemy->currentContext,
                                    (i16)enemy->periodicCallbackSub);
            if (enemy->stackDepth < 15)
            {
                enemy->stackDepth++;
            }
            instr = enemy->currentContext.curInstr;
            enemy->currentContext.isPeriodicSub = 1;
        }
    }
    for (;;)
    {
        if (enemy->currentContext.timer2.GetCurrent() > 0)
        {
            enemy->currentContext.timer2--;
            enemy->currentContext.time--;
            goto exit;
        }
        if (enemy->currentContext.time == instr->time)
        {
            if ((instr->skipInstrOnDifficulty & g_GameManager.difficultyMask) == 0)
            {
                goto skip;
            }
            switch (instr->id)
            {
            case 1:
                return ZUN_ERROR;
            case 45:
                enemy->currentContext.timer2 = GET_INT_VALUE(enemy, 0);
                break;
            case 3:
                *GET_INT_PTR(enemy, 2) -= 1;
                if (GET_INT_VALUE(enemy, 2) <= 0)
                {
                    break;
                }
            case 2:
                enemy->currentContext.time.current = instr->args[0].i;
                instr = (EclRawInstr *)((u8 *)instr + instr->args[1].i);
                continue;
            case 4:
                *GET_INT_PTR(enemy, 0) = GET_INT_VALUE(enemy, 1);
                break;
            case 5:
                *GET_FLOAT_PTR(enemy, 0) =
                    GET_FLOAT_VALUE(enemy, 1);
                break;
            case 40:
                *GET_FLOAT_PTR(enemy, 0) =
                    utils::AddNormalizeAngle(GET_FLOAT_VALUE(enemy, 0), 0.0f);
                break;
            case 6:
                *GET_INT_PTR(enemy, 0) =
                    g_Rng.GetRandomU32InRange(GET_INT_VALUE(enemy, 1));
                break;
            case 7:
                *GET_INT_PTR(enemy, 0) =
                    g_Rng.GetRandomU32InRange(GET_INT_VALUE(enemy, 1)) +
                    GET_INT_VALUE(enemy, 2);
                break;
            case 8:
                *GET_FLOAT_PTR(enemy, 0) =
                    g_Rng.GetRandomFloatInRange(GET_FLOAT_VALUE(enemy, 1));
                break;
            case 9:
                *GET_FLOAT_PTR(enemy, 0) =
                    g_Rng.GetRandomFloatInRange(GET_FLOAT_VALUE(enemy, 1)) +
                    GET_FLOAT_VALUE(enemy, 2);
                break;
            case 10:
                *GET_INT_PTR(enemy, 0) =
                    ((g_Rng.GetRandomU16() & 1) != 0 ? 1 : -1) *
                    GET_INT_VALUE(enemy, 1);
                break;
            case 11:
                *GET_FLOAT_PTR(enemy, 0) =
                    ((g_Rng.GetRandomU16() & 1) != 0 ? 1.0f : -1.0f) *
                    GET_FLOAT_VALUE(enemy, 1);
                break;
            case 17:
                *GET_INT_PTR(enemy, 0) += 1;
                break;
            case 18:
                *GET_INT_PTR(enemy, 0) -= 1;
                break;
            case 43:
                *GET_INT_PTR(enemy, 0) =
                    GET_INT_VALUE(
                        g_EnemyManager.bosses[GET_INT_VALUE(enemy, 2)], 1);
                break;
            case 44:
                *GET_FLOAT_PTR(enemy, 0) =
                    GET_FLOAT_VALUE(
                        g_EnemyManager.bosses[GET_INT_VALUE(enemy, 2)], 1);
                break;
            case 12:
                *GET_INT_PTR(enemy, 0) =
                    GET_INT_VALUE(enemy, 1) + GET_INT_VALUE(enemy, 2);
                break;
            case 19:
                *GET_FLOAT_PTR(enemy, 0) =
                    GET_FLOAT_VALUE(enemy, 1) + GET_FLOAT_VALUE(enemy, 2);
                break;
            case 13:
                *GET_INT_PTR(enemy, 0) =
                    GET_INT_VALUE(enemy, 1) - GET_INT_VALUE(enemy, 2);
                break;
            case 20:
                *GET_FLOAT_PTR(enemy, 0) =
                    GET_FLOAT_VALUE(enemy, 1) - GET_FLOAT_VALUE(enemy, 2);
                break;
            case 14:
                *GET_INT_PTR(enemy, 0) =
                    GET_INT_VALUE(enemy, 1) * GET_INT_VALUE(enemy, 2);
                break;
            case 21:
                *GET_FLOAT_PTR(enemy, 0) =
                    GET_FLOAT_VALUE(enemy, 1) * GET_FLOAT_VALUE(enemy, 2);
                break;
            case 15:
                *GET_INT_PTR(enemy, 0) =
                    GET_INT_VALUE(enemy, 1) / GET_INT_VALUE(enemy, 2);
                break;
            case 22:
                *GET_FLOAT_PTR(enemy, 0) =
                    GET_FLOAT_VALUE(enemy, 1) / GET_FLOAT_VALUE(enemy, 2);
                break;
            case 16:
                *GET_INT_PTR(enemy, 0) =
                    GET_INT_VALUE(enemy, 1) % GET_INT_VALUE(enemy, 2);
                break;
            case 23:
                *GET_FLOAT_PTR(enemy, 0) =
                    fmodf(GET_FLOAT_VALUE(enemy, 1), GET_FLOAT_VALUE(enemy, 2));
                break;
            case 24:
                *GET_FLOAT_PTR(enemy, 0) =
                    sinf(GET_FLOAT_VALUE(enemy, 1));
                break;
            case 25:
                *GET_FLOAT_PTR(enemy, 0) =
                    cosf(GET_FLOAT_VALUE(enemy, 1));
                break;
            case 26:
                *GET_FLOAT_PTR(enemy, 0) =
                    atan2f(GET_FLOAT_VALUE(enemy, 4) - GET_FLOAT_VALUE(enemy, 2),
                           GET_FLOAT_VALUE(enemy, 3) - GET_FLOAT_VALUE(enemy, 1));
                break;
            case 159:
                lerpDelta = GET_FLOAT_VALUE(enemy, 1) -
                            GET_FLOAT_VALUE(enemy, 2);
                *GET_FLOAT_PTR(enemy, 0) =
                    lerpDelta * GET_FLOAT_VALUE(enemy, 3) + GET_FLOAT_VALUE(enemy, 2);
                break;
            case 27:
                interp = enemy->currentContext.interps;
                for (interpIdx = 0; interpIdx < 8; interpIdx++, interp++)
                {
                    if (interp->fn &&
                        interp->args[7].f != instr->args[0].f)
                    {
                        continue;
                    }
                    interp->timer = 0;
                    interp->args[7].i = instr->args[0].i;
                    interp->args[0].i = GET_INT_VALUE(enemy, 1);
                    interp->args[1].i = GET_INT_VALUE(enemy, 2);
                    interp->args[2].i = GET_INT_VALUE(enemy, 3);
                    interp->fn = g_EclInterpFuncs[interp->args[1].i];
                    interp->args[3].f = GET_FLOAT_VALUE(enemy, 4);
                    interp->args[4].f = GET_FLOAT_VALUE(enemy, 5);
                    interp->args[5].f = GET_FLOAT_VALUE(enemy, 6);
                    interp->args[6].f = GET_FLOAT_VALUE(enemy, 7);
                    break;
                }
                break;
            case 28:
                if (GET_INT_VALUE(enemy, 0) == GET_INT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 29:
                if (GET_FLOAT_VALUE(enemy, 0) == GET_FLOAT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 30:
                if (GET_INT_VALUE(enemy, 0) != GET_INT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 31:
                if (GET_FLOAT_VALUE(enemy, 0) != GET_FLOAT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 32:
                if (GET_INT_VALUE(enemy, 0) < GET_INT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 33:
                if (GET_FLOAT_VALUE(enemy, 0) < GET_FLOAT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 34:
                if (GET_INT_VALUE(enemy, 0) <= GET_INT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 35:
                if (GET_FLOAT_VALUE(enemy, 0) <= GET_FLOAT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 36:
                if (GET_INT_VALUE(enemy, 0) > GET_INT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 37:
                if (GET_FLOAT_VALUE(enemy, 0) > GET_FLOAT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 38:
                if (GET_INT_VALUE(enemy, 0) >= GET_INT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            case 39: {
                if (GET_FLOAT_VALUE(enemy, 0) >= GET_FLOAT_VALUE(enemy, 1))
                {
                    goto LAB_00411f00;
                }
                break;
            LAB_00411f00:
                enemy->currentContext.time.current = instr->args[2].i;
                instr = (EclRawInstr *)((u8 *)instr + instr->args[3].i);
                continue;
            case 41:
                local_8 = instr->args[0].i;
                enemy->currentContext.curInstr =
                    (EclRawInstr *)((u8 *)instr + instr->size);
                if (!enemy->noStackRet)
                {
                    enemy->savedContextStack[enemy->stackDepth] = enemy->currentContext;
                }
                g_EclManager.CallEclSub(&enemy->currentContext, (i16)local_8);
                enemy->currentContext.eclContextArgs.globalVars = g_GlobalEclVars;
                if (!enemy->noStackRet && enemy->stackDepth < 15)
                {
                    enemy->stackDepth++;
                }
                goto restart;
            case 42:
                if (enemy->noStackRet)
                {
                    // STRING: TH07 0x004986e4
                    DebugPrint("error : no Stack Ret\r\n");
                }
                enemy->stackDepth--;
                if (enemy->currentContext.isPeriodicSub)
                {
                    enemy->savedEclContextArgs = enemy->currentContext.eclContextArgs;
                    enemy->currentContext.isPeriodicSub = 0;
                }
                enemy->currentContext = enemy->savedContextStack[enemy->stackDepth];
                goto restart;
            case 95:
                g_AnmManager->SetAnmIdxAndExecuteScript(&enemy->primaryVm,
                                                        GET_INT_VALUE(enemy, 0) + 2304);
                break;
            case 97:
                if (GET_INT_VALUE(enemy, 0) >= 2)
                {
                    // STRING: TH07 0x004986c8
                    DebugPrint("error : sub anim overflow\r\n");
                }
                if (GET_INT_VALUE(enemy, 1) >= 0)
                {
                    g_AnmManager->SetAnmIdxAndExecuteScript(
                        &enemy->vms[GET_INT_VALUE(enemy, 0)],
                        GET_INT_VALUE(enemy, 1) + 2304);
                }
                else
                {
                    enemy->vms[GET_INT_VALUE(enemy, 0)].anmFileIdx = -1;
                }
                break;
            case 46:
                enemy->pos.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->pos.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->pos.z = GET_FLOAT_VALUE(enemy, 2);
                enemy->ClampPos();
                break;
            case 47:
                enemy->axisSpeed.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->axisSpeed.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->axisSpeed.z = GET_FLOAT_VALUE(enemy, 2);
                enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
                enemy->moveMode = 0;
                break;
            case 48:
                enemy->angularVelocity = GET_FLOAT_VALUE(enemy, 0);
                enemy->moveMode = 1;
                break;
            case 53:
                enemy->angle = g_Player.AngleToPlayer(&enemy->pos) +
                               GET_FLOAT_VALUE(enemy, 0);
                enemy->moveSpeed = GET_FLOAT_VALUE(enemy, 1);
                enemy->moveMode = 1;
                break;
            case 49:
                enemy->moveSpeed = GET_FLOAT_VALUE(enemy, 0);
                enemy->moveMode = 1;
                break;
            case 50:
                enemy->moveAcceleration = GET_FLOAT_VALUE(enemy, 0);
                enemy->moveMode = 1;
                break;
            case 59:
                enemy->moveMode = 1;
                enemy->moveInterpTimer = enemy->moveInterpStartTime =
                    GET_INT_VALUE(enemy, 0);
                break;
            case 60:
                enemy->moveMode = 3;
                enemy->moveInterpTimer = enemy->moveInterpStartTime =
                    GET_INT_VALUE(enemy, 0);
                break;
            case 61:
                enemy->moveMode = 2;
                enemy->moveInterpTimer = enemy->moveInterpStartTime =
                    GET_INT_VALUE(enemy, 0);
                break;
            case 64:
            case 65:
            case 66:
            case 67:
            case 68:
            case 69:
            case 70:
            case 71:
            case 72:
                if (enemy->life <= 0)
                {
                    break;
                }
                bulletInstrArgs = instr->args;
                bulletProps = &enemy->bulletProps;
                local_8 = bulletInstrArgs->s[0];
                bulletProps->sprite = (instr->paramMask & 1) != 0
                                          ? GetVarValue(enemy, local_8)
                                          : local_8;
                bulletProps->aimMode = instr->id - 64;
                bulletProps->count1 = GET_INT_VALUE_D(enemy, bulletInstrArgs, 1, 2);
                bulletProps->count2 = GET_INT_VALUE_D(enemy, bulletInstrArgs, 2, 3);
                bulletProps->pos = enemy->pos + enemy->shootOffset;
                bulletProps->angle1 = GET_FLOAT_VALUE_D(enemy, bulletInstrArgs, 5, 6);
                bulletProps->speed1 = GET_FLOAT_VALUE_D(enemy, bulletInstrArgs, 3, 4);
                bulletProps->angle2 = GET_FLOAT_VALUE_D(enemy, bulletInstrArgs, 6, 7);
                bulletProps->speed2 = GET_FLOAT_VALUE_D(enemy, bulletInstrArgs, 4, 5);
                if (!g_EnemyManager.spellcardInfo.isActive)
                {
                    bulletProps->count1 += enemy->BulletRankAmount1(g_GameManager.rank.rank);
                    if (bulletProps->count1 <= 0)
                    {
                        bulletProps->count1 = 1;
                    }

                    bulletProps->count2 += enemy->BulletRankAmount2(g_GameManager.rank.rank);
                    if (bulletProps->count2 <= 0)
                    {
                        bulletProps->count2 = 1;
                    }
                    if (bulletProps->speed1 != 0.0f)
                    {
                        bulletProps->speed1 += enemy->BulletRankSpeed(g_GameManager.rank.rank);
                        if (bulletProps->speed1 < 0.3f)
                        {
                            bulletProps->speed1 = 0.3f;
                        }
                    }
                    bulletProps->speed2 += enemy->BulletRankSpeed(g_GameManager.rank.rank) /
                                           2.0f;
                    if (bulletProps->speed2 < 0.3f)
                    {
                        bulletProps->speed2 = 0.3f;
                    }
                }
                bulletProps->unused_c2 = 0;
                bulletProps->flags = bulletInstrArgs[7].u;
                local_8 = bulletInstrArgs->s[1];
                bulletProps->spriteOffset = (instr->paramMask & 2) != 0
                                                ? GetVarValue(enemy, local_8)
                                                : local_8;
                if (!enemy->disableBullets)
                {
                    g_BulletManager.SpawnBulletPattern(bulletProps);
                }
                break;
            case 79:
                bulletCommand =
                    &enemy->bulletProps.commands[GET_INT_VALUE(enemy, 0)];
                bulletCommand->type = GET_INT_VALUE(enemy, 1);
                bulletCommand->flag = GET_INT_VALUE(enemy, 2);
                bulletCommand->duration = GET_INT_VALUE(enemy, 3);
                bulletCommand->loopCount = GET_INT_VALUE(enemy, 4);
                bulletCommand->speed = GET_FLOAT_VALUE(enemy, 5);
                bulletCommand->angle = GET_FLOAT_VALUE(enemy, 6);
                break;
            case 98:
                enemy->deathAnm1 = instr->args[0].c[0];
                enemy->deathAnm2 = instr->args[0].c[1];
                enemy->deathAnm3 = instr->args[0].c[2];
                break;
            case 73:
                enemy->shootInterval = GET_INT_VALUE(enemy, 0);
                if (enemy->shootInterval != 0)
                {
                    enemy->shootInterval += enemy->ShootInterval(g_GameManager.rank.rank);
                    enemy->shootIntervalTimer = 0;
                }
                break;
            case 74:
                enemy->shootInterval = GET_INT_VALUE(enemy, 0);
                if (enemy->shootInterval != 0)
                {
                    enemy->shootInterval += enemy->ShootInterval(g_GameManager.rank.rank);
                    enemy->shootIntervalTimer =
                        g_Rng.GetRandomU32InRange(enemy->shootInterval);
                }
                break;
            case 75:
                enemy->disableBullets = 1;
                break;
            case 76:
                enemy->disableBullets = 0;
                break;
            case 77:
                enemy->bulletProps.pos = enemy->pos + enemy->shootOffset;
                g_BulletManager.SpawnBulletPattern(&enemy->bulletProps);
                break;
            case 78:
                enemy->shootOffset.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->shootOffset.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->shootOffset.z = GET_FLOAT_VALUE(enemy, 2);
                break;
            case 82:
            case 83:
                laserInstrArgs = instr->args;
                laserProps = &enemy->laserProps;
                laserProps->pos = enemy->pos + enemy->shootOffset;
                laserProps->sprite = laserInstrArgs->s[0];
                laserProps->spriteOffset = (instr->paramMask & 2) != 0
                                               ? GetVarValue(enemy, laserInstrArgs->s[1])
                                               : (i32)laserInstrArgs->s[1];
                laserProps->angle1 = GET_FLOAT_VALUE_D(enemy, laserInstrArgs, 1, 2);
                laserProps->speed1 = GET_FLOAT_VALUE_D(enemy, laserInstrArgs, 2, 3);
                laserProps->startOffset = GET_FLOAT_VALUE_D(enemy, laserInstrArgs, 3, 4);
                laserProps->endOffset = GET_FLOAT_VALUE_D(enemy, laserInstrArgs, 4, 5);
                laserProps->startLength = GET_FLOAT_VALUE_D(enemy, laserInstrArgs, 5, 6);
                laserProps->width = laserInstrArgs[6].f;
                laserProps->startTime = laserInstrArgs[7].i;
                laserProps->duration = laserInstrArgs[8].i;
                laserProps->endTime = laserInstrArgs[9].i;
                laserProps->hitboxStartTime = laserInstrArgs[10].i;
                laserProps->hitboxEndTime = laserInstrArgs[11].i;
                laserProps->flags = laserInstrArgs[12].u;

                if (instr->id == 83)
                {
                    laserProps->type = 0;
                }
                else
                {
                    laserProps->type = 1;
                }
                enemy->lasers[enemy->laserIdx] =
                    g_BulletManager.SpawnLaserPattern(laserProps);
                break;
            case 84:
                enemy->laserIdx = GET_INT_VALUE(enemy, 0);
                break;
            case 85:
                local_8 = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[local_8])
                {
                    enemy->lasers[local_8]->angle = utils::AddNormalizeAngle(
                        enemy->lasers[local_8]->angle, GET_FLOAT_VALUE(enemy, 1));
                }
                break;
            case 152:
                local_8 = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[local_8])
                {
                    enemy->lasers[local_8]->angle = GET_FLOAT_VALUE(enemy, 1);
                }
                break;
            case 86:
                local_8 = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[local_8])
                {
                    enemy->lasers[local_8]->angle =
                        g_Player.AngleToPlayer(&enemy->lasers[local_8]->pos) +
                        GET_FLOAT_VALUE(enemy, 1);
                }
                break;
            case 87:
                local_8 = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[local_8])
                {
                    enemy->lasers[local_8]->pos.x =
                        GET_FLOAT_VALUE(enemy, 1) + enemy->pos.x;
                    enemy->lasers[local_8]->pos.y =
                        GET_FLOAT_VALUE(enemy, 2) + enemy->pos.y;
                    enemy->lasers[local_8]->pos.z =
                        GET_FLOAT_VALUE(enemy, 3) + enemy->pos.z;
                }
                break;
            case 156:
                local_8 = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[local_8])
                {
                    enemy->lasers[local_8]->hideWarning =
                        GET_INT_VALUE(enemy, 1);
                }
                break;
            case 88:
                local_8 = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[local_8] &&
                    enemy->lasers[local_8]->inUse)
                {
                    enemy->currentContext.compareRegister = 0;
                }
                else
                {
                    enemy->currentContext.compareRegister = 1;
                }
                break;
            case 89:
                local_8 = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[local_8] &&
                    enemy->lasers[local_8]->inUse &&
                    enemy->lasers[local_8]->state < 2)
                {
                    enemy->lasers[local_8]->state = 2;
                    enemy->lasers[local_8]->timer = 0;
                    enemy->lasers[local_8]->width =
                        enemy->lasers[local_8]->targetWidth;
                }
                break;
            case 134:
                for (laserIdx = 0; laserIdx < 32; laserIdx++)
                {
                    enemy->lasers[laserIdx] = NULL;
                }
                break;
            case 157:
                local_8 = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[local_8])
                {
                    enemy->lasers[local_8]->startLength =
                        GET_FLOAT_VALUE(enemy, 1);
                }
                break;
            case 158:
                local_8 = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[local_8])
                {
                    enemy->lasers[local_8]->startOffset =
                        GET_FLOAT_VALUE(enemy, 1);
                    enemy->lasers[local_8]->endOffset =
                        GET_FLOAT_VALUE(enemy, 2);
                }
                break;
            case 147:
                g_EnemyManager.unused_9545f0 = GET_INT_VALUE(enemy, 0);
                break;
            case 99:
                if (GET_INT_VALUE(enemy, 0) >= 0)
                {
                    g_EnemyManager.bosses[GET_INT_VALUE(enemy, 0)] = enemy;
                    g_Gui.bossPresent = 1;
                    g_Gui.bossHealthBar = 1.0f;
                    enemy->isBoss = 1;
                    enemy->bossId = GET_INT_VALUE(enemy, 0);
                    g_AsciiManager.SetBossMarkerInterrupt(enemy->bossId, 1);
                }
                else
                {
                    if (enemy->bossId < 4)
                    {
                        g_Gui.bossPresent = 0;
                    }
                    g_EnemyManager.bosses[enemy->bossId] = NULL;
                    enemy->isBoss = 0;
                    g_AsciiManager.SetBossMarkerInterrupt(enemy->bossId, 2);
                    enemy->ResetEffectArray();
                }
                break;
            case 100:
                effectInstrArgs = instr->args;
                enemy->effects[enemy->effectsNum] = g_EffectManager.SpawnParticles(
                    13, &enemy->pos, 1, g_BulletColor[effectInstrArgs->i]);
                enemy->effects[enemy->effectsNum]->direction = *(Float3 *)&effectInstrArgs[1];
                enemy->effectDistance = effectInstrArgs[4].f;
                enemy->effectsNum++;
                break;
            case 54:
                if (GET_INT_VALUE(enemy, 0) <= 0)
                {
                    enemy->angle =
                        utils::AddNormalizeAngle(
                            GET_FLOAT_VALUE(enemy, 2), 0.0f);
                    enemy->moveSpeed = GET_FLOAT_VALUE(enemy, 3);
                    enemy->moveMode = 1;
                    enemy->moveInterpTimer = enemy->moveInterpStartTime =
                        GET_INT_VALUE(enemy, 0);
                }
                else
                {
                    MoveDirTime(enemy, instr);
                }
                break;
            case 55:
                MovePosTime(enemy, instr);
                break;
            case 56:
                enemy->moveInterpTimer = enemy->moveInterpStartTime =
                    GET_INT_VALUE(enemy, 0);
                enemy->moveInterpStartPos.x = GET_FLOAT_VALUE(enemy, 1);
                enemy->moveInterpStartPos.y = GET_FLOAT_VALUE(enemy, 2);
                enemy->moveInterpStartPos.z = GET_FLOAT_VALUE(enemy, 3);
                enemy->moveAngle = GET_FLOAT_VALUE(enemy, 4);
                enemy->moveAngularVelocity = GET_FLOAT_VALUE(enemy, 5);
                enemy->moveRadius = GET_FLOAT_VALUE(enemy, 6);
                enemy->moveRadialVelocity = GET_FLOAT_VALUE(enemy, 7);
                enemy->moveMode = 3;
                break;
            case 57:
                enemy->moveRadius = GET_FLOAT_VALUE(enemy, 0);
                enemy->moveRadialVelocity = GET_FLOAT_VALUE(enemy, 1);
                break;
            case 58:
                enemy->moveAngle = GET_FLOAT_VALUE(enemy, 0);
                enemy->moveAngularVelocity = GET_FLOAT_VALUE(enemy, 1);
                break;
            case 62:
                enemy->lowerMoveLimit.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->lowerMoveLimit.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->upperMoveLimit.x = GET_FLOAT_VALUE(enemy, 2);
                enemy->upperMoveLimit.y = GET_FLOAT_VALUE(enemy, 3);
                enemy->hasMovementBounds = 1;
                break;
            case 63:
                enemy->hasMovementBounds = 0;
                break;
            case 51:
                *GET_FLOAT_PTR(enemy, 0) =
                    g_Rng.GetRandomFloatInRange(GET_FLOAT_VALUE(enemy, 2) -
                                                GET_FLOAT_VALUE(enemy, 1)) +
                    GET_FLOAT_VALUE(enemy, 1);
                break;
            case 52:
                if (g_Player.positionCenter.x < enemy->pos.x)
                {
                    exitAngle = utils::AddNormalizeAngle(
                        g_Rng.GetRandomFloatInRange(1.5707964f) + 2.3561945f, 0.0f);
                }
                else
                {
                    exitAngle = g_Rng.GetRandomFloatInRange(1.5707964f) - 0.7853982f;
                }
                if (enemy->pos.x < enemy->lowerMoveLimit.x + 96.0f)
                {
                    if (exitAngle > 1.5707964f)
                    {
                        exitAngle = 3.1415927f - exitAngle;
                    }
                    else if (exitAngle < -1.5707964f)
                    {
                        exitAngle = -3.1415927f - exitAngle;
                    }
                }
                if (enemy->upperMoveLimit.x - 96.0f < enemy->pos.x)
                {
                    if (exitAngle < 1.5707964f && exitAngle >= 0.0f)
                    {
                        exitAngle = 3.1415927f - enemy->angle;
                    }
                    else if (exitAngle > -1.5707964f && exitAngle <= 0.0f)
                    {
                        exitAngle = -3.1415927f - exitAngle;
                    }
                }
                if (enemy->lowerMoveLimit.y + 48.0f > enemy->pos.y &&
                    exitAngle < 0.0f)
                {
                    exitAngle = -exitAngle;
                }
                if (enemy->upperMoveLimit.y - 48.0f < enemy->pos.y &&
                    exitAngle > 0.0f)
                {
                    exitAngle = -exitAngle;
                }
                *GET_FLOAT_PTR(enemy, 0) = exitAngle;
                break;
            case 96:
                enemy->anmExDefaults = instr->args[0].s[0];
                enemy->anmExFarLeft = instr->args[0].s[1];
                enemy->anmExFarRight = instr->args[1].s[0];
                enemy->anmExLeft = instr->args[1].s[1];
                enemy->anmExRight = instr->args[2].s[0];
                enemy->anmExFlags = 255;
                break;
            case 101:
                enemy->hitboxSize.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->hitboxSize.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->hitboxSize.z = GET_FLOAT_VALUE(enemy, 2);
                break;
            case 153:
                enemy->grazeSize.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->grazeSize.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->grazeSize.z = GET_FLOAT_VALUE(enemy, 2);
                break;
            case 102:
                enemy->hasContactHitbox = instr->args[0].b[0];
                break;
            case 103:
                enemy->canBeDamaged = instr->args[0].b[0];
                break;
            case 104:
                enemy->isHittable = instr->args[0].b[0];
                break;
            case 105:
                g_SoundPlayer.PlaySoundByIdx(GET_INT_VALUE(enemy, 0), 0);
                break;
            case 106:
                enemy->deathType = instr->args[0].b[0];
                break;
            case 107:
                enemy->deathCallbackSub = (u32)instr->args[0].b[0];
                break;
            case 108:
                enemy->interrupts[GET_INT_VALUE(enemy, 1)] =
                    GET_INT_VALUE(enemy, 0);
                break;
            case 109:
                enemy->runInterrupt = GET_INT_VALUE(enemy, 0);
            handle_interrupt:
                enemy->currentContext.curInstr = (EclRawInstr *)((u8 *)instr + instr->size);
                if (!enemy->noStackRet)
                {
                    enemy->savedContextStack[enemy->stackDepth] = enemy->currentContext;
                }
                g_EclManager.CallEclSub(&enemy->currentContext,
                                        enemy->interrupts[enemy->runInterrupt]);
                if (enemy->stackDepth < 15)
                {
                    enemy->stackDepth = enemy->stackDepth + 1;
                }
                enemy->runInterrupt = -1;
                goto restart;
            case 110:
                enemy->life = enemy->maxLife = GET_INT_VALUE(enemy, 0);
                if (enemy->bossId == 0 && enemy->isBoss)
                {
                    for (healthIdx = 0; healthIdx < 8; healthIdx++)
                    {
                        g_Gui.bossHealthEased[healthIdx] = 0.0f;
                        g_Gui.bossHealth[healthIdx] = 0.0f;
                    }
                }
                break;
            case 139:
                bossIdx = GET_INT_VALUE(enemy, 0);
                g_Gui.SetBossHealth(bossIdx,
                                    GET_INT_VALUE(enemy, 1) /
                                        (f32)enemy->maxLife,
                                    GET_INT_VALUE(enemy, 2) /
                                        (f32)enemy->maxLife);
                g_Gui.bossColor[bossIdx] = GET_INT_VALUE(enemy, 3);
                break;
            case 90:
                BeginSpellcard(enemy, instr);
                break;
            case 91:
                EndSpellcard(enemy, instr);
                break;
            case 111:
                enemy->timer = GET_INT_VALUE(enemy, 0);
                break;
            case 112:
                enemy->lifeCallbackThreshold[0] = GET_INT_VALUE(enemy, 0);
                break;
            case 113:
                enemy->lifeCallbackSub[0] = GET_INT_VALUE(enemy, 0);
                break;
            case 148:
                enemy->lifeCallbackThreshold[GET_INT_VALUE(enemy, 0)] =
                    GET_INT_VALUE(enemy, 1);
                enemy->lifeCallbackSub[GET_INT_VALUE(enemy, 0)] =
                    GET_INT_VALUE(enemy, 2);
                break;
            case 114:
                enemy->timerCallbackThreshold = GET_INT_VALUE(enemy, 0);
                enemy->timer = 0;
                break;
            case 115:
                enemy->timerCallbackSub = GET_INT_VALUE(enemy, 0);
                break;
            case 144:
                enemy->periodicTimer = GET_INT_VALUE(enemy, 0);
                enemy->periodicCallbackSub = GET_INT_VALUE(enemy, 1);
                enemy->periodicCounter = 0;
                enemy->savedEclContextArgs = enemy->currentContext.eclContextArgs;
                break;
            case 116:
                enemy->canDie = instr->args[0].b[0];
                break;
            case 117:
                g_EffectManager.SpawnParticles(
                    GET_INT_VALUE(enemy, 0),
                    &enemy->pos,
                    GET_INT_VALUE(enemy, 1),
                    *(D3DCOLOR *)GET_INT_PTR(enemy, 2));
                break;
            case 118:
                particleVel.x = GET_FLOAT_VALUE(enemy, 3);
                particleVel.y = GET_FLOAT_VALUE(enemy, 4);
                particleVel.z = GET_FLOAT_VALUE(enemy, 5);
                g_EffectManager.SpawnMovingParticles(
                    GET_INT_VALUE(enemy, 0),
                    &enemy->pos,
                    &particleVel,
                    GET_INT_VALUE(enemy, 1),
                    *(D3DCOLOR *)GET_INT_PTR(enemy, 2));
                break;
            case 119:
                numDrops = GET_INT_VALUE(enemy, 0);
                for (itemDropIdx = 0; itemDropIdx < numDrops; itemDropIdx++)
                {
                    itemDropPos = enemy->pos;
                    itemDropPos[0] += g_Rng.GetRandomFloatInRange(128.0f) - 64.0f;
                    itemDropPos[1] += g_Rng.GetRandomFloatInRange(128.0f) - 64.0f;
                    if ((i32)g_GameManager.globals->currentPower < 128)
                    {
                        g_ItemManager.SpawnItem(&itemDropPos,
                                                itemDropIdx == 0 ? ITEM_POWER_BIG : ITEM_POWER_SMALL, 0);
                    }
                    else
                    {
                        g_ItemManager.SpawnItem(&itemDropPos, ITEM_POINT, 0);
                    }
                }
                break;
            case 154:
                numPointItems = GET_INT_VALUE(enemy, 0);
                for (pointItemIdx = 0; pointItemIdx < numPointItems; pointItemIdx++)
                {
                    pointItemPos = enemy->pos;
                    pointItemPos[0] += g_Rng.GetRandomFloatInRange(128.0f) - 64.0f;
                    pointItemPos[1] += g_Rng.GetRandomFloatInRange(128.0f) - 64.0f;
                    g_ItemManager.SpawnItem(&pointItemPos, ITEM_POINT, 0);
                }
                break;
            case 120:
                enemy->primaryVmAutoRotate = instr->args[0].b[0];
                break;
            case 121:
                g_EclExInstr[GET_INT_VALUE(enemy, 0)](enemy, instr);
                break;
            case 122:
                if (GET_INT_VALUE(enemy, 0) >= 0)
                {
                    enemy->currentContext.func =
                        g_EclExInstr[GET_INT_VALUE(enemy, 0)];
                    enemy->currentContext.eclExInstr = instr;
                }
                else
                {
                    enemy->currentContext.func = NULL;
                }
                break;
            case 123:
                enemy->currentContext.time += GET_INT_VALUE(enemy, 0);
                break;
            case 124:
                g_ItemManager.SpawnItem(&enemy->pos,
                                        GET_INT_VALUE(enemy, 0), 0);
                break;
            case 125:
                g_Stage.scriptWaitTime = GET_INT_VALUE(enemy, 0);
                break;
            case 126:
                g_Gui.bossLifeMarkers = GET_INT_VALUE(enemy, 0);
                g_GameManager.playTimeAll += 1800;
                break;
            case 92:
                if (enemy->life > 0)
                {
                    memcpy(absSpawnInstrArgs, instr->args, sizeof(absSpawnInstrArgs));
                    absEnemySpawnPos.x = GET_FLOAT_VALUE_D(enemy, absSpawnInstrArgs, 1, 1);
                    absEnemySpawnPos.y = GET_FLOAT_VALUE_D(enemy, absSpawnInstrArgs, 2, 2);
                    absEnemySpawnPos.z = GET_FLOAT_VALUE_D(enemy, absSpawnInstrArgs, 3, 3);
                    unusedEnemyAbs = g_EnemyManager.SpawnEnemyEx(absSpawnInstrArgs[0].i, &absEnemySpawnPos,
                                                                 GET_INT_VALUE(enemy, 4),
                                                                 GET_INT_VALUE(enemy, 5),
                                                                 GET_INT_VALUE(enemy, 6),
                                                                 &enemy->currentContext.eclContextArgs);
                }
                break;
            case 93:
                if (enemy->life > 0)
                {
                    memcpy(relSpawnInstrArgs, instr->args, sizeof(relSpawnInstrArgs));
                    relEnemySpawnPos.x = GET_FLOAT_VALUE_D(enemy, relSpawnInstrArgs, 1, 1);
                    relEnemySpawnPos.y = GET_FLOAT_VALUE_D(enemy, relSpawnInstrArgs, 2, 2);
                    relEnemySpawnPos.z = GET_FLOAT_VALUE_D(enemy, relSpawnInstrArgs, 3, 3);
                    relEnemySpawnPos += enemy->pos;
                    unusedEnemyRel = g_EnemyManager.SpawnEnemyEx(relSpawnInstrArgs[0].i, &relEnemySpawnPos,
                                                                 GET_INT_VALUE(enemy, 4),
                                                                 GET_INT_VALUE(enemy, 5),
                                                                 GET_INT_VALUE(enemy, 6),
                                                                 &enemy->currentContext.eclContextArgs);
                }
                break;
            case 94:
                g_EnemyManager.RemoveAllEnemies(8000, 0);
                break;
            case 128:
                enemy->primaryVm.pendingInterrupt = GET_INT_VALUE(enemy, 0);
                break;
            case 129:
                enemy->vms[instr->args[0].i].pendingInterrupt = instr->args[1].s[0];
                break;
            case 80:
                g_BulletManager.RemoveAllBullets(1);
                break;
            case 81:
                if (GET_INT_VALUE(enemy, 0) >= 0)
                {
                    enemy->bulletProps.soundIdx = GET_INT_VALUE(enemy, 0);
                    enemy->bulletProps.flags |= 0x200;
                }
                else
                {
                    enemy->bulletProps.flags &= 0xfffffdff;
                }
                enemy->bulletProps.soundOverride = GET_INT_VALUE(enemy, 1);
                break;
            case 130:
                enemy->noStackRet = instr->args[0].b[0];
                break;
            case 131:
                enemy->bulletRankSpeedLow = GET_FLOAT_VALUE(enemy, 0);
                enemy->bulletRankSpeedHigh = GET_FLOAT_VALUE(enemy, 1);
                enemy->bulletRankAmount1Low = GET_INT_VALUE(enemy, 2);
                enemy->bulletRankAmount1High = GET_INT_VALUE(enemy, 3);
                enemy->bulletRankAmount2Low = GET_INT_VALUE(enemy, 4);
                enemy->bulletRankAmount2High = GET_INT_VALUE(enemy, 5);
                break;
            case 132:
                enemy->hasNoCollision = instr->args[0].b[0];
                break;
            case 133:
                enemy->timerCallbackSub = enemy->deathCallbackSub;
                enemy->timer = 0;
                break;
            case 135:
                enemy->isSurvivalSpellcard = instr->args[0].b[0];
                break;
            case 136:
                enemy->isProjectile = instr->args[0].b[0];
                enemy->zLayer = 2;
                break;
            case 137:
                enemy->disableOOBDespawn = instr->args[0].b[0];
                break;
            case 138:
                enemy->trailFlags = instr->args[0].c[0];
                enemy->trailCount = GET_INT_VALUE(enemy, 1);
                enemy->trailInterval = GET_INT_VALUE(enemy, 2);
                enemy->trailNodeStep = GET_INT_VALUE(enemy, 3);
                if ((enemy->trailFlags & 8) != 0)
                {
                    g_AnmManager->UpdateTrail(
                        &enemy->primaryVm, enemy->trailVertices,
                        (i32)enemy->trailCount / (i32)enemy->trailNodeStep << 1);
                }
                break;
            case 140:
                g_EffectManager.globalColorMultiplierR =
                    GET_FLOAT_VALUE(enemy, 0);
                g_EffectManager.globalColorMultiplierG =
                    GET_FLOAT_VALUE(enemy, 1);
                g_EffectManager.globalColorMultiplierB =
                    GET_FLOAT_VALUE(enemy, 2);
                g_EffectManager.globalColorMultiplierA =
                    GET_FLOAT_VALUE(enemy, 3);
                break;
            case 142:
                enemy->invincibilityTimer = GET_INT_VALUE(enemy, 0);
                break;
            case 143:
                g_BulletManager.RemoveBulletsInRadius(&enemy->pos,
                                                      GET_FLOAT_VALUE(enemy, 0));
                break;
            case 145:
                if (g_EnemyManager.bosses[GET_INT_VALUE(enemy, 0)] != NULL)
                {
                    g_EnemyManager.bosses[GET_INT_VALUE(enemy, 0)]->runInterrupt = GET_INT_VALUE(enemy, 1);
                }
                break;
            case 146:
                g_BulletManager.RemoveAllBullets(0);
                break;
            case 149:
                enemy->customSpecialEffectPos = GET_INT_VALUE(enemy, 0);
                if (!enemy->customSpecialEffectPos)
                {
                    enemy->specialEffect->pos1.x = GET_FLOAT_VALUE(enemy, 1);
                    enemy->specialEffect->pos1.y = GET_FLOAT_VALUE(enemy, 2);
                    enemy->specialEffect->pos1.z = GET_FLOAT_VALUE(enemy, 3);
                }
                break;
            case 150:
                enemy->primaryVm.rotation.z = GET_FLOAT_VALUE(enemy, 0);
                break;
            case 151:
                *GET_FLOAT_PTR(enemy, 1) =
                    sinf(GET_FLOAT_VALUE(enemy, 2)) * GET_FLOAT_VALUE(enemy, 3);
                *GET_FLOAT_PTR(enemy, 0) =
                    cosf(GET_FLOAT_VALUE(enemy, 2)) * GET_FLOAT_VALUE(enemy, 3);
                break;
            case 155:
                if ((g_Player.positionCenter.x < enemy->pos.x &&
                     enemy->pos.x > 96.0f) ||
                    enemy->pos.x > 288.0f)
                {
                    *GET_FLOAT_PTR(enemy, 0) =
                        utils::AddNormalizeAngle(
                            g_Rng.GetRandomFloatInRange(1.5707964f) + 2.3561945f, 0.0f);
                }
                else
                {
                    *GET_FLOAT_PTR(enemy, 0) =
                        g_Rng.GetRandomFloatInRange(1.5707964f) - 0.7853982f;
                }
                break;
            case 160:
                g_GameManager.AddCherryPlus(GET_INT_VALUE(enemy, 0));
                break;
            case 161:
                enemy->freezeEclDuringBombs = GET_INT_VALUE(enemy, 0);
                break;
            }
            }

        skip:
            instr = (EclRawInstr *)((u8 *)instr + instr->size);
            continue;
        }
        else
        {
        exit:
            switch (enemy->moveMode)
            {
            case 3:
                enemy->moveAngle = utils::AddNormalizeAngle(
                    enemy->moveAngle, g_Supervisor.effectiveFramerateMultiplier *
                                          enemy->moveAngularVelocity);
                enemy->moveRadius = g_Supervisor.effectiveFramerateMultiplier *
                                        enemy->moveRadialVelocity +
                                    enemy->moveRadius;
                local_d8.FromAngleMagnitude(enemy->moveAngle, enemy->moveRadius);
                enemy->axisSpeed.x =
                    local_d8.x + enemy->moveInterpStartPos.x - enemy->pos.x;
                enemy->axisSpeed.y =
                    local_d8.y + enemy->moveInterpStartPos.y - enemy->pos.y;
                enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
                if (enemy->moveInterpStartTime > 0)
                {
                    enemy->moveInterpTimer--;
                    if (enemy->moveInterpTimer <= 0)
                    {
                        enemy->moveMode = 0;
                    }
                }
                break;
            case 1:
                enemy->angle = utils::AddNormalizeAngle(
                    enemy->angle,
                    g_Supervisor.effectiveFramerateMultiplier * enemy->angularVelocity);
                enemy->moveSpeed = g_Supervisor.effectiveFramerateMultiplier *
                                       enemy->moveAcceleration +
                                   enemy->moveSpeed;
                enemy->axisSpeed.FromAngleMagnitude(enemy->angle, enemy->moveSpeed);
                enemy->axisSpeed.z = 0.0f;
                if (enemy->moveInterpStartTime > 0)
                {
                    enemy->moveInterpTimer--;
                    if (enemy->moveInterpTimer <= 0)
                    {
                        enemy->moveMode = 0;
                    }
                }
                break;
            case 2:
                enemy->moveInterpTimer--;
                t1 = 1.0f - enemy->moveInterpTimer.AsFloat() /
                                (f32)enemy->moveInterpStartTime;
                if (t1 < 0.0f)
                {
                    t1 = 0.0f;
                }
                switch (enemy->interpEasing)
                {
                case 1: {
                    t1 = t1 * t1;
                    break;
                }
                case 2: {
                    t1 = t1 * t1 * t1;
                    break;
                }
                case 3: {
                    t1 = t1 * t1 * t1 * t1;
                    break;
                }
                case 4: {
                    t1 = 1.0f - t1;
                    t1 = t1 * t1;
                    t1 = 1.0f - t1;
                    break;
                }
                case 5: {
                    t1 = 1.0f - t1;
                    t1 = t1 * t1 * t1;
                    t1 = 1.0f - t1;
                    break;
                }
                case 6: {
                    t1 = 1.0f - t1;
                    t1 = t1 * t1 * t1 * t1;
                    t1 = 1.0f - t1;
                    break;
                }
                }
                enemy->axisSpeed =
                    t1 * enemy->moveInterp + enemy->moveInterpStartPos -
                    enemy->pos;
                if (enemy->mirror)
                {
                    enemy->axisSpeed.x = -enemy->axisSpeed.x;
                }
                enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
                if (enemy->moveInterpTimer <= 0)
                {
                    enemy->moveMode = 0;
                    enemy->pos = enemy->moveInterpStartPos + enemy->moveInterp;
                    enemy->axisSpeed = Float3(0.0f, 0.0f, 0.0f);
                }
                break;
            }
            if (enemy->life > 0)
            {
                if (enemy->shootInterval > 0)
                {
                    enemy->shootIntervalTimer++;
                    if (enemy->shootIntervalTimer >= enemy->shootInterval)
                    {
                        enemy->bulletProps.pos = enemy->pos + enemy->shootOffset;
                        g_BulletManager.SpawnBulletPattern(&enemy->bulletProps);
                        enemy->shootIntervalTimer = 0;
                    }
                }
                if (enemy->anmExLeft >= 0)
                {
                    anmDirection = 0;
                    if (!enemy->mirror)
                    {
                        if (enemy->axisSpeed.x < -0.01f)
                        {
                            anmDirection = 1;
                        }
                        else if (enemy->axisSpeed.x > 0.01f)
                        {
                            anmDirection = 2;
                        }
                    }
                    else
                    {
                        if (enemy->axisSpeed.x < -0.01f)
                        {
                            anmDirection = 2;
                        }
                        else if (enemy->axisSpeed.x > 0.01f)
                        {
                            anmDirection = 1;
                        }
                    }
                    if (enemy->anmExFlags != anmDirection)
                    {
                        switch (anmDirection)
                        {
                        case 0:
                            if (enemy->anmExFlags == 255)
                            {
                                g_AnmManager->SetAnmIdxAndExecuteScript(
                                    &enemy->primaryVm,
                                    enemy->anmExDefaults + 2304);
                            }
                            else if (enemy->anmExFlags == 1)
                            {
                                g_AnmManager->SetAnmIdxAndExecuteScript(
                                    &enemy->primaryVm,
                                    enemy->anmExFarLeft + 2304);
                            }
                            else
                            {
                                g_AnmManager->SetAnmIdxAndExecuteScript(
                                    &enemy->primaryVm,
                                    enemy->anmExFarRight + 2304);
                            }
                            break;
                        case 1:
                            g_AnmManager->SetAnmIdxAndExecuteScript(
                                &enemy->primaryVm,
                                enemy->anmExLeft + 2304);
                            break;
                        case 2:
                            g_AnmManager->SetAnmIdxAndExecuteScript(
                                &enemy->primaryVm,
                                enemy->anmExRight + 2304);
                            break;
                        }
                        enemy->anmExFlags = anmDirection;
                    }
                }
                if (enemy->currentContext.func)
                {
                    enemy->currentContext.func(enemy, enemy->currentContext.eclExInstr);
                }
                local_f8 = false;
                interp2 = enemy->currentContext.interps;
                Float3 local_104 = enemy->pos;
                for (interpIdx2 = 0; interpIdx2 < 8; interpIdx2++, interp2++)
                {
                    if (interp2->fn)
                    {
                        interp2->timer++;
                        if (interp2->timer >= interp2->args[0].i)
                        {
                            interp2->timer = interp2->args[0].i;
                        }
                        t2 =
                            interp2->timer.AsFloat() / (f32)interp2->args[0].i;
                        switch (interp2->args[2].i)
                        {
                        case 1: {
                            t2 = t2 * t2;
                            break;
                        }
                        case 2: {
                            t2 = t2 * t2 * t2;
                            break;
                        }
                        case 3: {
                            t2 = t2 * t2 * t2 * t2;
                            break;
                        }
                        case 4: {
                            t2 = 1.0f - t2;
                            t2 = t2 * t2;
                            t2 = 1.0f - t2;
                            break;
                        }
                        case 5: {
                            t2 = 1.0f - t2;
                            t2 = t2 * t2 * t2;
                            t2 = 1.0f - t2;
                            break;
                        }
                        case 6: {
                            t2 = 1.0f - t2;
                            t2 = t2 * t2 * t2 * t2;
                            t2 = 1.0f - t2;
                            break;
                        }
                        }
                        interp2->fn(enemy, interp2, t2);
                        if (interp2->timer >= interp2->args[0].i)
                        {
                            interp2->fn = NULL;
                        }
                        if (interp2->args[7].f == 10018.0f ||
                            interp2->args[7].f == 10019.0f ||
                            interp2->args[7].f == 10020.0f)
                        {
                            local_f8 = true;
                        }
                    }
                }
                if (local_f8)
                {
                    enemy->axisSpeed.x = enemy->pos.x - local_104.x;
                    enemy->axisSpeed.y = enemy->pos.y - local_104.y;
                    enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
                    enemy->pos = local_104;
                }
            }
            enemy->currentContext.curInstr = instr;
            enemy->currentContext.time++;
            if (enemy->isBoss && enemy->bossId == 0 &&
                (g_EnemyManager.spellcardInfo.isActive &&
                 g_EnemyManager.spellcardInfo.isCapturing))
            {
                if (!enemy->isSurvivalSpellcard)
                {
                    g_EnemyManager.spellcardInfo.captureScore =
                        (i32)((f32)(i32)
                                  g_SpellcardScore[g_EnemyManager.spellcardInfo.spellcardIdx] -
                              g_EnemyManager.timer.AsFloat() *
                                  (f32)g_EnemyManager.spellcardInfo.scoreDrainRate /
                                  60.0f);
                    g_EnemyManager.spellcardInfo.captureScore =
                        g_EnemyManager.spellcardInfo.captureScore -
                        g_EnemyManager.spellcardInfo.captureScore % 10;
                }
                g_EnemyManager.timer++;
            }
            if (enemy->isBoss && g_GameManager.currentStage >= 7)
            {
                if (g_Player.bombInfo.isInUse &&
                    g_EnemyManager.spellcardInfo.isActive &&
                    g_EnemyManager.spellcardInfo.spellcardIdx >= 118)
                {
                    enemy->invisibleOnBomb = 1;
                    enemy->spellcardDelayTimer = 1;
                }
                else if (enemy->spellcardDelayTimer > 0)
                {
                    enemy->spellcardDelayTimer--;
                }
                else
                {
                    enemy->invisibleOnBomb = 0;
                }
            }
            return ZUN_SUCCESS;
        }
    }
}
