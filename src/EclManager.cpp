#include "EclManager.hpp"

#include <cstdio>

#include "AnmIdx.hpp"
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

#define GET_INT_PTR(enemy, argIdx) GetVar(enemy, &instr->args[argIdx].i, instr->paramMask, argIdx)

#define GET_FLOAT_PTR(enemy, argIdx)                                                               \
    GetFloatVar(enemy, &instr->args[argIdx].f, instr->paramMask, argIdx)

#define GET_INT_VALUE(enemy, argIdx)                                                               \
    (((instr->paramMask & (1 << argIdx)) != 0) ? GetVarValue(enemy, instr->args[argIdx].i)         \
                                               : instr->args[argIdx].i)

#define GET_FLOAT_VALUE(enemy, argIdx)                                                             \
    (((instr->paramMask & (1 << argIdx)) != 0) ? GetFloatVarValue(enemy, instr->args[argIdx].f)    \
                                               : instr->args[argIdx].f)

#define GET_INT_VALUE_D(enemy, args, argIdx, bitIdx)                                               \
    (((instr->paramMask & (1 << bitIdx)) != 0) ? GetVarValue(enemy, args[argIdx].i)                \
                                               : args[argIdx].i)

#define GET_FLOAT_VALUE_D(enemy, args, argIdx, bitIdx)                                             \
    (((instr->paramMask & (1 << bitIdx)) != 0) ? GetFloatVarValue(enemy, args[argIdx].f)           \
                                               : args[argIdx].f)

const char *g_EclPaths[10] = {
    "dummy",
    "data/ecldata1.ecl",
    "data/ecldata2.ecl",
    "data/ecldata3.ecl",
    "data/ecldata4.ecl",
    "data/ecldata5.ecl",
    "data/ecldata6.ecl",
    "data/ecldata7.ecl",
    "data/ecldata8.ecl",
    NULL,
};

EclManager g_EclManager;

EclGlobalVars g_GlobalEclVars;

ZunResult EclManager::Load(const char *path)
{
    i32 i;

    this->eclFile = (EclRawHeader *)FileSystem::OpenFile(path, 0);
    if (!this->eclFile)
    {
        g_GameErrorContext.Log(
            "敵データの読み込みに失敗しました、データが壊れてるか失われています\n");
        return ZUN_ERROR;
    }

    for (i = 0; i < ARRAY_SIZE_SIGNED(this->timelinePtr); i++)
    {
        this->timelinePtr[i] = (EclTimelineInstr *)((uintptr_t)this->eclFile->timelineOffsets[i] +
                                                    (uintptr_t)this->eclFile);
    }
    this->subTable = new EclRawInstr *[this->eclFile->subCount];
    for (i = 0; i < this->eclFile->subCount; i++)
    {
        this->subTable[i] = (EclRawInstr *)((uintptr_t)this->eclFile->subTableOffsets[i] +
                                            (uintptr_t)this->eclFile);
    }
    return ZUN_SUCCESS;
}

void EclManager::Unload()
{
    if (this->subTable)
    {
        delete[] this->subTable;
        this->subTable = NULL;
    }
    if (this->eclFile)
    {
        free(this->eclFile);
    }
    this->eclFile = NULL;
}

ZunResult EclManager::CallEclSub(EnemyEclContext *ctx, i16 subId)
{
    ctx->curInstr = this->subTable[subId];
    ctx->time = 0;
    ctx->waitTimer = 0;
    ctx->subId = subId;
    return ZUN_SUCCESS;
}

i32 EclManager::GetVarValue(Enemy *enemy, i32 eclVar)
{
    switch (eclVar)
    {
    case ECL_VAR_LOCAL_INT1_1:
        return enemy->currentContext.eclContextArgs.intVars1[0];
    case ECL_VAR_LOCAL_INT1_2:
        return enemy->currentContext.eclContextArgs.intVars1[1];
    case ECL_VAR_LOCAL_INT1_3:
        return enemy->currentContext.eclContextArgs.intVars1[2];
    case ECL_VAR_LOCAL_INT1_4:
        return enemy->currentContext.eclContextArgs.intVars1[3];
    case ECL_VAR_LOCAL_INT3_1:
        return enemy->currentContext.eclContextArgs.globalVars.intVars[0];
    case ECL_VAR_LOCAL_INT3_2:
        return enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    case ECL_VAR_LOCAL_INT3_3:
        return enemy->currentContext.eclContextArgs.globalVars.intVars[2];
    case ECL_VAR_LOCAL_INT3_4:
        return enemy->currentContext.eclContextArgs.globalVars.intVars[3];
    case ECL_VAR_LOCAL_INT2_1:
        return enemy->currentContext.eclContextArgs.intVars2[0];
    case ECL_VAR_LOCAL_INT2_2:
        return enemy->currentContext.eclContextArgs.intVars2[1];
    case ECL_VAR_LOCAL_INT2_3:
        return enemy->currentContext.eclContextArgs.intVars2[2];
    case ECL_VAR_LOCAL_INT2_4:
        return enemy->currentContext.eclContextArgs.intVars2[3];
    case ECL_VAR_DIFFICULTY:
        return g_GameManager.difficulty;
    case ECL_VAR_RANK:
        return g_GameManager.rank.rank;
    case ECL_VAR_CUR_TIME:
        return enemy->timer.current;
    case ECL_VAR_LIFE:
        return enemy->life;
    case ECL_VAR_PLAYER_SHOTTYPE:
        return g_GameManager.shotTypeAndCharacter;
    case ECL_VAR_LOCAL_FLOAT2_1:
        return enemy->currentContext.eclContextArgs.floatVars2[0];
    case ECL_VAR_LOCAL_FLOAT2_2:
        return enemy->currentContext.eclContextArgs.floatVars2[1];
    case ECL_VAR_LOCAL_FLOAT1_1:
        return enemy->currentContext.eclContextArgs.floatVars1[0];
    case ECL_VAR_LOCAL_FLOAT1_2:
        return enemy->currentContext.eclContextArgs.floatVars1[1];
    case ECL_VAR_LOCAL_FLOAT1_3:
        return enemy->currentContext.eclContextArgs.floatVars1[2];
    case ECL_VAR_LOCAL_FLOAT1_4:
        return enemy->currentContext.eclContextArgs.floatVars1[3];
    case ECL_VAR_LOCAL_FLOAT1_5:
        return enemy->currentContext.eclContextArgs.floatVars1[4];
    case ECL_VAR_LOCAL_FLOAT1_6:
        return enemy->currentContext.eclContextArgs.floatVars1[5];
    case ECL_VAR_LOCAL_FLOAT1_7:
        return enemy->currentContext.eclContextArgs.floatVars1[6];
    case ECL_VAR_LOCAL_FLOAT1_8:
        return enemy->currentContext.eclContextArgs.floatVars1[7];
    case ECL_VAR_LOCAL_FLOAT3_1:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[0];
    case ECL_VAR_LOCAL_FLOAT3_2:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    case ECL_VAR_LOCAL_FLOAT3_3:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[2];
    case ECL_VAR_LOCAL_FLOAT3_4:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[3];
    case ECL_VAR_GLOBAL_INT_1:
        return g_GlobalEclVars.intVars[0];
    case ECL_VAR_GLOBAL_INT_2:
        return g_GlobalEclVars.intVars[1];
    case ECL_VAR_GLOBAL_INT_3:
        return g_GlobalEclVars.intVars[2];
    case ECL_VAR_GLOBAL_INT_4:
        return g_GlobalEclVars.intVars[3];
    case ECL_VAR_GLOBAL_FLOAT_1:
        return g_GlobalEclVars.floatVars[0];
    case ECL_VAR_GLOBAL_FLOAT_2:
        return g_GlobalEclVars.floatVars[1];
    case ECL_VAR_GLOBAL_FLOAT_3:
        return g_GlobalEclVars.floatVars[2];
    case ECL_VAR_GLOBAL_FLOAT_4:
        return g_GlobalEclVars.floatVars[3];
    case ECL_VAR_POS_X:
        return enemy->pos.x;
    case ECL_VAR_POS_Y:
        return enemy->pos.y;
    case ECL_VAR_POS_Z:
        return enemy->pos.z;
    case ECL_VAR_PLAYER_POS_X:
        return g_Player.positionCenter.x;
    case ECL_VAR_PLAYER_POS_Y:
        return g_Player.positionCenter.y;
    case ECL_VAR_PLAYER_POS_Z:
        return g_Player.positionCenter.z;
    case ECL_VAR_MOVE_INTERP_ORIGIN_X:
        return enemy->moveInterpStartPos.x;
    case ECL_VAR_MOVE_INTERP_ORIGIN_Y:
        return enemy->moveInterpStartPos.y;
    case ECL_VAR_MOVE_INTERP_ORIGIN_Z:
        return enemy->moveInterpStartPos.z;
    case ECL_VAR_DELTA_POS_X:
        return enemy->deltaPos.x;
    case ECL_VAR_DELTA_POS_Y:
        return enemy->deltaPos.y;
    case ECL_VAR_DELTA_POS_Z:
        return enemy->deltaPos.z;
    case ECL_VAR_BOSS_LIFE_THRESHOLD1:
        return enemy->lifeCallbackThreshold[0];
    case ECL_VAR_BOSS_LIFE_THRESHOLD2:
        return enemy->lifeCallbackThreshold[1];
    case ECL_VAR_BOSS_LIFE_THRESHOLD3:
        return enemy->lifeCallbackThreshold[2];
    case ECL_VAR_BOSS_LIFE_THRESHOLD4:
        return enemy->lifeCallbackThreshold[3];
    case ECL_VAR_ANGLE:
        return enemy->angle;
    case ECL_VAR_ANGULAR_VELOCITY:
        return enemy->angularVelocity;
    case ECL_VAR_MOVE_SPEED:
        return enemy->moveSpeed;
    case ECL_VAR_MOVE_ACCELERATION:
        return enemy->moveAcceleration;
    case ECL_VAR_MOVE_RADIUS:
        return enemy->moveRadius;
    case ECL_VAR_MOVE_ANGLE:
        return enemy->moveAngle;
    case ECL_VAR_MOVE_ANGULAR_VELOCITY:
        return enemy->moveAngularVelocity;
    case ECL_VAR_RNG:
        return g_Rng.GetRandomU32();
    case ECL_VAR_RNG_CUSTOM_BOUND:
        return g_Rng.GetRandomU32InRange(
                   enemy->currentContext.eclContextArgs.globalVars.intVars[0]) +
               enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    case ECL_VAR_LAST_DAMAGE:
        return enemy->lastDamage;
    case ECL_VAR_BOSS_ID:
        return enemy->bossId;
    case ECL_VAR_ITEMDROP:
        return enemy->itemDrop;
    case ECL_VAR_SCORE:
        return enemy->score;
    case ECL_VAR_ANGLE_TO_PLAYER:
        return g_Player.AngleToPlayer(&enemy->pos);
    case ECL_VAR_DISTANCE_FROM_PLAYER:
        return (g_Player.positionCenter - enemy->pos).Length();
    default:
        return eclVar;
    }
}

i32 *EclManager::GetVar(Enemy *enemy, i32 *eclVar, u16 paramMask, i32 idx)
{
    if (idx >= 0 && ((u32)paramMask & 1 << idx) == 0)
    {
        return eclVar;
    }

    switch (*eclVar)
    {
    case ECL_VAR_LOCAL_INT1_1:
        return &enemy->currentContext.eclContextArgs.intVars1[0];
    case ECL_VAR_LOCAL_INT1_2:
        return &enemy->currentContext.eclContextArgs.intVars1[1];
    case ECL_VAR_LOCAL_INT1_3:
        return &enemy->currentContext.eclContextArgs.intVars1[2];
    case ECL_VAR_LOCAL_INT1_4:
        return &enemy->currentContext.eclContextArgs.intVars1[3];
    case ECL_VAR_LOCAL_INT3_1:
        return &enemy->currentContext.eclContextArgs.globalVars.intVars[0];
    case ECL_VAR_LOCAL_INT3_2:
        return &enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    case ECL_VAR_LOCAL_INT3_3:
        return &enemy->currentContext.eclContextArgs.globalVars.intVars[2];
    case ECL_VAR_LOCAL_INT3_4:
        return &enemy->currentContext.eclContextArgs.globalVars.intVars[3];
    case ECL_VAR_LOCAL_INT2_1:
        return &enemy->currentContext.eclContextArgs.intVars2[0];
    case ECL_VAR_LOCAL_INT2_2:
        return &enemy->currentContext.eclContextArgs.intVars2[1];
    case ECL_VAR_LOCAL_INT2_3:
        return &enemy->currentContext.eclContextArgs.intVars2[2];
    case ECL_VAR_LOCAL_INT2_4:
        return &enemy->currentContext.eclContextArgs.intVars2[3];
    case ECL_VAR_DIFFICULTY:
        return &g_GameManager.difficulty;
    case ECL_VAR_RANK:
        return &g_GameManager.rank.rank;
    case ECL_VAR_CUR_TIME:
        return &enemy->timer.current;
    case ECL_VAR_LIFE:
        return &enemy->life;
    case ECL_VAR_ITEMDROP:
        return &enemy->itemDrop;
    case ECL_VAR_SCORE:
        return &enemy->score;
    case ECL_VAR_GLOBAL_INT_1:
        return &g_GlobalEclVars.intVars[0];
    case ECL_VAR_GLOBAL_INT_2:
        return &g_GlobalEclVars.intVars[1];
    case ECL_VAR_GLOBAL_INT_3:
        return &g_GlobalEclVars.intVars[2];
    case ECL_VAR_GLOBAL_INT_4:
        return &g_GlobalEclVars.intVars[3];
    default:
        return eclVar;
    }
}

f32 EclManager::GetFloatVarValue(Enemy *enemy, f32 eclVar)
{
    switch ((i32)eclVar)
    {
    case ECL_VAR_LOCAL_INT1_1:
        return (f32)enemy->currentContext.eclContextArgs.intVars1[0];
    case ECL_VAR_LOCAL_INT1_2:
        return (f32)enemy->currentContext.eclContextArgs.intVars1[1];
    case ECL_VAR_LOCAL_INT1_3:
        return (f32)enemy->currentContext.eclContextArgs.intVars1[2];
    case ECL_VAR_LOCAL_INT1_4:
        return (f32)enemy->currentContext.eclContextArgs.intVars1[3];
    case ECL_VAR_LOCAL_INT3_1:
        return (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[0];
    case ECL_VAR_LOCAL_INT3_2:
        return (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    case ECL_VAR_LOCAL_INT3_3:
        return (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[2];
    case ECL_VAR_LOCAL_INT3_4:
        return (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[3];
    case ECL_VAR_LOCAL_INT2_1:
        return (f32)enemy->currentContext.eclContextArgs.intVars2[0];
    case ECL_VAR_LOCAL_INT2_2:
        return (f32)enemy->currentContext.eclContextArgs.intVars2[1];
    case ECL_VAR_LOCAL_INT2_3:
        return (f32)enemy->currentContext.eclContextArgs.intVars2[2];
    case ECL_VAR_LOCAL_INT2_4:
        return (f32)enemy->currentContext.eclContextArgs.intVars2[3];
    case ECL_VAR_DIFFICULTY:
        return (f32)g_GameManager.difficulty;
    case ECL_VAR_RANK:
        return (f32)g_GameManager.rank.rank;
    case ECL_VAR_CUR_TIME:
        return (f32)enemy->timer.current;
    case ECL_VAR_LIFE:
        return (f32)enemy->life;
    case ECL_VAR_PLAYER_SHOTTYPE:
        return (f32)g_GameManager.shotTypeAndCharacter;
    case ECL_VAR_ITEMDROP:
        return (f32)enemy->itemDrop;
    case ECL_VAR_SCORE:
        return (f32)enemy->score;
    case ECL_VAR_GLOBAL_INT_1:
        return (f32)g_GlobalEclVars.intVars[0];
    case ECL_VAR_GLOBAL_INT_2:
        return (f32)g_GlobalEclVars.intVars[1];
    case ECL_VAR_GLOBAL_INT_3:
        return (f32)g_GlobalEclVars.intVars[2];
    case ECL_VAR_GLOBAL_INT_4:
        return (f32)g_GlobalEclVars.intVars[3];
    case ECL_VAR_GLOBAL_FLOAT_1:
        return g_GlobalEclVars.floatVars[0];
    case ECL_VAR_GLOBAL_FLOAT_2:
        return g_GlobalEclVars.floatVars[1];
    case ECL_VAR_GLOBAL_FLOAT_3:
        return g_GlobalEclVars.floatVars[2];
    case ECL_VAR_GLOBAL_FLOAT_4:
        return g_GlobalEclVars.floatVars[3];
    case ECL_VAR_LOCAL_FLOAT1_1:
        return enemy->currentContext.eclContextArgs.floatVars1[0];
    case ECL_VAR_LOCAL_FLOAT1_2:
        return enemy->currentContext.eclContextArgs.floatVars1[1];
    case ECL_VAR_LOCAL_FLOAT1_3:
        return enemy->currentContext.eclContextArgs.floatVars1[2];
    case ECL_VAR_LOCAL_FLOAT1_4:
        return enemy->currentContext.eclContextArgs.floatVars1[3];
    case ECL_VAR_LOCAL_FLOAT1_5:
        return enemy->currentContext.eclContextArgs.floatVars1[4];
    case ECL_VAR_LOCAL_FLOAT1_6:
        return enemy->currentContext.eclContextArgs.floatVars1[5];
    case ECL_VAR_LOCAL_FLOAT1_7:
        return enemy->currentContext.eclContextArgs.floatVars1[6];
    case ECL_VAR_LOCAL_FLOAT1_8:
        return enemy->currentContext.eclContextArgs.floatVars1[7];
    case ECL_VAR_LOCAL_FLOAT3_1:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[0];
    case ECL_VAR_LOCAL_FLOAT3_2:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    case ECL_VAR_LOCAL_FLOAT3_3:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[2];
    case ECL_VAR_LOCAL_FLOAT3_4:
        return enemy->currentContext.eclContextArgs.globalVars.floatVars[3];
    case ECL_VAR_POS_X:
        return enemy->pos.x;
    case ECL_VAR_POS_Y:
        return enemy->pos.y;
    case ECL_VAR_POS_Z:
        return enemy->pos.z;
    case ECL_VAR_PLAYER_POS_X:
        return g_Player.positionCenter.x;
    case ECL_VAR_PLAYER_POS_Y:
        return g_Player.positionCenter.y;
    case ECL_VAR_PLAYER_POS_Z:
        return g_Player.positionCenter.z;
    case ECL_VAR_LOCAL_FLOAT2_1:
        return enemy->currentContext.eclContextArgs.floatVars2[0];
    case ECL_VAR_LOCAL_FLOAT2_2:
        return enemy->currentContext.eclContextArgs.floatVars2[1];
    case ECL_VAR_MOVE_INTERP_ORIGIN_X:
        return enemy->moveInterpStartPos.x;
    case ECL_VAR_MOVE_INTERP_ORIGIN_Y:
        return enemy->moveInterpStartPos.y;
    case ECL_VAR_MOVE_INTERP_ORIGIN_Z:
        return enemy->moveInterpStartPos.z;
    case ECL_VAR_MOVE_INTERP_TARGET_X:
        return enemy->moveInterp.x;
    case ECL_VAR_MOVE_INTERP_TARGET_Y:
        return enemy->moveInterp.y;
    case ECL_VAR_MOVE_INTERP_TARGET_Z:
        return enemy->moveInterp.z;
    case ECL_VAR_DELTA_POS_X:
        return enemy->deltaPos.x;
    case ECL_VAR_DELTA_POS_Y:
        return enemy->deltaPos.y;
    case ECL_VAR_DELTA_POS_Z:
        return enemy->deltaPos.z;
    case ECL_VAR_BOSS_LIFE_THRESHOLD1:
        return (f32)enemy->lifeCallbackThreshold[0];
    case ECL_VAR_BOSS_LIFE_THRESHOLD2:
        return (f32)enemy->lifeCallbackThreshold[1];
    case ECL_VAR_BOSS_LIFE_THRESHOLD3:
        return (f32)enemy->lifeCallbackThreshold[2];
    case ECL_VAR_BOSS_LIFE_THRESHOLD4:
        return (f32)enemy->lifeCallbackThreshold[3];
    case ECL_VAR_ANGLE_TO_PLAYER:
        return g_Player.AngleToPlayer(&enemy->pos);
    case ECL_VAR_ANGLE:
        return enemy->angle;
    case ECL_VAR_ANGULAR_VELOCITY:
        return enemy->angularVelocity;
    case ECL_VAR_MOVE_SPEED:
        return enemy->moveSpeed;
    case ECL_VAR_MOVE_ACCELERATION:
        return enemy->moveAcceleration;
    case ECL_VAR_MOVE_RADIUS:
        return enemy->moveRadius;
    case ECL_VAR_MOVE_ANGLE:
        return enemy->moveAngle;
    case ECL_VAR_MOVE_ANGULAR_VELOCITY:
        return enemy->moveAngularVelocity;
    case ECL_VAR_RNG:
        return g_Rng.GetRandomFloat();
    case ECL_VAR_RNG_CUSTOM_BOUND:
        return g_Rng.GetRandomFloatInRange(
                   enemy->currentContext.eclContextArgs.globalVars.floatVars[0]) +
               enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    case ECL_VAR_RNG_RADIAN:
        return g_Rng.GetRandomFloatInRange(ZUN_2PI) - ZUN_PI;
    case ECL_VAR_BOSS_ID:
        return (f32)enemy->bossId;
    case ECL_VAR_LAST_DAMAGE:
        return (f32)enemy->lastDamage;
    case ECL_VAR_DISTANCE_FROM_PLAYER:
        return (g_Player.positionCenter - enemy->pos).Length();
    default:
        return eclVar;
    }
}

f32 *EclManager::GetFloatVar(Enemy *enemy, f32 *eclVar, u16 paramMask, i32 idx)
{
    if (idx >= 0 && ((u32)paramMask & 1 << idx) == 0)
    {
        return eclVar;
    }

    switch ((i32)*eclVar)
    {
    case ECL_VAR_LOCAL_FLOAT1_1:
        return &enemy->currentContext.eclContextArgs.floatVars1[0];
    case ECL_VAR_LOCAL_FLOAT1_2:
        return &enemy->currentContext.eclContextArgs.floatVars1[1];
    case ECL_VAR_LOCAL_FLOAT1_3:
        return &enemy->currentContext.eclContextArgs.floatVars1[2];
    case ECL_VAR_LOCAL_FLOAT1_4:
        return &enemy->currentContext.eclContextArgs.floatVars1[3];
    case ECL_VAR_LOCAL_FLOAT1_5:
        return &enemy->currentContext.eclContextArgs.floatVars1[4];
    case ECL_VAR_LOCAL_FLOAT1_6:
        return &enemy->currentContext.eclContextArgs.floatVars1[5];
    case ECL_VAR_LOCAL_FLOAT1_7:
        return &enemy->currentContext.eclContextArgs.floatVars1[6];
    case ECL_VAR_LOCAL_FLOAT1_8:
        return &enemy->currentContext.eclContextArgs.floatVars1[7];
    case ECL_VAR_LOCAL_FLOAT3_1:
        return &enemy->currentContext.eclContextArgs.globalVars.floatVars[0];
    case ECL_VAR_LOCAL_FLOAT3_2:
        return &enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    case ECL_VAR_LOCAL_FLOAT3_3:
        return &enemy->currentContext.eclContextArgs.globalVars.floatVars[2];
    case ECL_VAR_LOCAL_FLOAT3_4:
        return &enemy->currentContext.eclContextArgs.globalVars.floatVars[3];
    case ECL_VAR_POS_X:
        return &enemy->pos.x;
    case ECL_VAR_POS_Y:
        return &enemy->pos.y;
    case ECL_VAR_POS_Z:
        return &enemy->pos.z;
    case ECL_VAR_PLAYER_POS_X:
        return &g_Player.positionCenter.x;
    case ECL_VAR_PLAYER_POS_Y:
        return &g_Player.positionCenter.y;
    case ECL_VAR_PLAYER_POS_Z:
        return &g_Player.positionCenter.z;
    case ECL_VAR_LOCAL_FLOAT2_1:
        return &enemy->currentContext.eclContextArgs.floatVars2[0];
    case ECL_VAR_LOCAL_FLOAT2_2:
        return &enemy->currentContext.eclContextArgs.floatVars2[1];
    case ECL_VAR_GLOBAL_FLOAT_1:
        return &g_GlobalEclVars.floatVars[0];
    case ECL_VAR_GLOBAL_FLOAT_2:
        return &g_GlobalEclVars.floatVars[1];
    case ECL_VAR_GLOBAL_FLOAT_3:
        return &g_GlobalEclVars.floatVars[2];
    case ECL_VAR_GLOBAL_FLOAT_4:
        return &g_GlobalEclVars.floatVars[3];
    case ECL_VAR_MOVE_INTERP_ORIGIN_X:
        return &enemy->moveInterpStartPos.x;
    case ECL_VAR_MOVE_INTERP_ORIGIN_Y:
        return &enemy->moveInterpStartPos.y;
    case ECL_VAR_MOVE_INTERP_ORIGIN_Z:
        return &enemy->moveInterpStartPos.z;
    case ECL_VAR_MOVE_INTERP_TARGET_X:
        return &enemy->moveInterp.x;
    case ECL_VAR_MOVE_INTERP_TARGET_Y:
        return &enemy->moveInterp.y;
    case ECL_VAR_MOVE_INTERP_TARGET_Z:
        return &enemy->moveInterp.z;
    case ECL_VAR_ANGLE:
        return &enemy->angle;
    case ECL_VAR_ANGULAR_VELOCITY:
        return &enemy->angularVelocity;
    case ECL_VAR_MOVE_SPEED:
        return &enemy->moveSpeed;
    case ECL_VAR_MOVE_ACCELERATION:
        return &enemy->moveAcceleration;
    case ECL_VAR_MOVE_RADIUS:
        return &enemy->moveRadius;
    case ECL_VAR_MOVE_ANGLE:
        return &enemy->moveAngle;
    case ECL_VAR_MOVE_ANGULAR_VELOCITY:
        return &enemy->moveAngularVelocity;
    default:
        return eclVar;
    }
}

void EclManager::MoveDirTime(Enemy *enemy, EclRawInstr *instr)
{
    f32 fVar2;

    fVar2 = utils::AddNormalizeAngle(GET_FLOAT_VALUE(enemy, 2), 0.0f);
    enemy->moveInterp.x = cosf(fVar2) * GET_FLOAT_VALUE(enemy, 3) * (f32)GET_INT_VALUE(enemy, 0);
    enemy->moveInterp.y = sinf(fVar2) * GET_FLOAT_VALUE(enemy, 3) * (f32)GET_INT_VALUE(enemy, 0);
    enemy->moveInterp.z = 0.0f;
    enemy->moveInterpStartPos = enemy->pos;
    enemy->moveInterpTimer = enemy->moveInterpStartTime = GET_INT_VALUE(enemy, 0);
    enemy->interpEasing = (u8)GET_INT_VALUE(enemy, 1);
    enemy->moveMode = ENEMY_MOVE_INTERP;
    if (enemy->mirror)
    {
        enemy->moveInterp.x = -enemy->moveInterp.x;
    }
}

void EclManager::MovePosTime(Enemy *enemy, EclRawInstr *instr)
{
    ZunVec3 newPos;
    newPos.x = GET_FLOAT_VALUE(enemy, 2);
    newPos.y = GET_FLOAT_VALUE(enemy, 3);
    newPos.z = GET_FLOAT_VALUE(enemy, 4);

    enemy->moveInterp = newPos - enemy->pos;
    enemy->moveInterpStartPos = enemy->pos;
    enemy->moveInterpTimer = enemy->moveInterpStartTime = GET_INT_VALUE(enemy, 0);
    enemy->interpEasing = (u8)GET_INT_VALUE(enemy, 1);
    enemy->moveMode = ENEMY_MOVE_INTERP;
    enemy->axisSpeed = ZunVec3(0.0f, 0.0f, 0.0f);
    if (enemy->mirror)
    {
        enemy->moveInterp.x = -enemy->moveInterp.x;
    }
}

void EclManager::MathLerp(Enemy *enemy, EclInterp *interp, f32 t)
{
    f32 a = GetFloatVarValue(enemy, interp->args[3].f);
    f32 b = GetFloatVarValue(enemy, interp->args[4].f);

    *GetFloatVar(enemy, &interp->args[7].f, 0, -1) = (b - a) * t + a;
}

void EclManager::MathCubicInterp(Enemy *enemy, EclInterp *interp, f32 t)
{
    f32 h11;
    f32 m1;
    f32 h01;
    f32 m0;
    f32 p1;
    f32 h10;
    f32 h00;
    f32 p0;

    p0 = GetFloatVarValue(enemy, interp->args[3].f);
    p1 = GetFloatVarValue(enemy, interp->args[4].f);
    m0 = GetFloatVarValue(enemy, interp->args[5].f);
    m1 = GetFloatVarValue(enemy, interp->args[6].f);

    h00 = (t - 1.0f) * (t - 1.0f) * (2.0f * t + 1.0f);
    h01 = t * t * (3.0f - 2.0f * t);
    h10 = (1.0f - t) * (1.0f - t) * t;
    h11 = (t - 1.0f) * t * t;

    *GetFloatVar(enemy, &interp->args[7].f, 0, -1) = h00 * p0 + h01 * p1 + h10 * m0 + h11 * m1;
}

void EclManager::BeginSpellcard(Enemy *enemy, EclRawInstr *instr)
{
    i32 newCsum;
    i32 nameCsum;
    i32 j;
    Catk *catk;
    char spellcardName[48];
    i32 i;

    memcpy(spellcardName, &instr->args[1], sizeof(spellcardName));
    for (i = 0; i < ARRAY_SIZE(spellcardName); i++)
    {
        spellcardName[i] = (u8)spellcardName[i] ^ 0xaa;
    }
    g_Gui.ShowSpellcard(instr->args[0].s[0], spellcardName);
    g_BulletManager.RemoveAllBullets(1);
    g_Stage.spellCardState = SPELLCARD_STATE_STARTING;
    g_Stage.ticksSinceSpellcardStarted = 0;
    for (i = 0; i < g_Stage.numSpellcardVms; i++)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(&g_Stage.spellcardVms[i],
                                                i + g_Stage.spellcardVmsIdx +
                                                    ANM_SCRIPT_EFFECTS_SPELLCARD_BG_ARRAY);
    }
    g_EnemyManager.spellcardInfo.isActive = 1;
    g_EnemyManager.spellcardInfo.isCapturing = 1;
    g_EnemyManager.spellcardInfo.spellcardIdx = instr->args[0].us[1];
    g_EnemyManager.spellcardInfo.captureScore =
        g_SpellcardScore[g_EnemyManager.spellcardInfo.spellcardIdx];
    g_EnemyManager.spellcardInfo.grazeBonusScore = 0;
    g_EnemyManager.spellcardInfo.scoreDrainRate =
        g_EnemyManager.spellcardInfo.captureScore / (enemy->timerCallbackThreshold / 60 + 10);
    g_EnemyManager.timer = 0;
    enemy->bulletRankSpeedLow = -0.5f;
    enemy->bulletRankSpeedHigh = 0.5f;
    enemy->bulletRankAmount1Low = 0;
    enemy->bulletRankAmount1High = 0;
    enemy->bulletRankAmount2Low = 0;
    enemy->bulletRankAmount2High = 0;
    enemy->specialEffect = g_EffectManager.SpawnSpecialEffect(25, &enemy->pos, 1, 1, 0xffffffff);
    enemy->specialEffect->vm.interpStartTimes[4] = 0;
    enemy->specialEffect->vm.interpEndTimes[4] = enemy->timerCallbackThreshold;
    enemy->specialEffect->vm.easeModes[4] = 0;
    enemy->specialEffect->vm.scaleInterpInitial = enemy->specialEffect->vm.scale;
    enemy->specialEffect->vm.scaleInterpFinal.x = 1.0f / 8.0f;
    enemy->specialEffect->vm.scaleInterpFinal.y = 1.0f / 8.0f;
    enemy->specialEffect->pos1 = enemy->pos;
    enemy->customSpecialEffectPos = 0;
    if (!g_GameManager.replay)
    {
        catk = &g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx];
        nameCsum = 0;
        strcpy(catk->name, spellcardName);
        j = (i32)strlen(catk->name);
        while (j > 0)
        {
            j--;
            nameCsum += catk->name[j];
        }
        newCsum = nameCsum;
        for (j = 0; j < ARRAY_SIZE_SIGNED(catk->numSuccessesPerShot); j++)
        {
            nameCsum += catk->numSuccessesPerShot[j];
            nameCsum += catk->numAttemptsPerShot[j];
            nameCsum += catk->highScorePerShot[j];
        }
        if (catk->nameCsum != (u8)nameCsum)
        {
            for (j = 0; j < ARRAY_SIZE_SIGNED(catk->numSuccessesPerShot); j++)
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
        if (catk->numAttemptsPerShot[SHOT_COUNT] < 9999)
        {
            catk->numAttemptsPerShot[SHOT_COUNT]++;
        }
        for (j = 0; j < ARRAY_SIZE_SIGNED(catk->numSuccessesPerShot); j++)
        {
            newCsum += catk->numSuccessesPerShot[j];
            newCsum += catk->numAttemptsPerShot[j];
            newCsum += catk->highScorePerShot[j];
        }
        catk->nameCsum = (u8)newCsum;
    }
}

void EclManager::EndSpellcard()
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
                    while (i > 0)
                    {
                        i--;
                        nameCsum += catk->name[i];
                    }
                    newCsum = nameCsum;
                    for (i = 0; i < ARRAY_SIZE_SIGNED(catk->numSuccessesPerShot); i++)
                    {
                        nameCsum += catk->numSuccessesPerShot[i];
                        nameCsum += catk->numAttemptsPerShot[i];
                        nameCsum += catk->highScorePerShot[i];
                    }
                    if (catk->nameCsum != (u8)nameCsum)
                    {
                        for (i = 0; i < ARRAY_SIZE_SIGNED(catk->numSuccessesPerShot); i++)
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
                    for (i = 0; i < ARRAY_SIZE_SIGNED(catk->numSuccessesPerShot); i++)
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
        for (j = 0; j < ARRAY_SIZE_SIGNED(g_EnemyManager.bosses); j++)
        {
            if (g_EnemyManager.bosses[j] && g_EnemyManager.bosses[j]->specialEffect != NULL)
            {
                g_EnemyManager.bosses[j]->specialEffect->inUseFlag = 0;
                g_EnemyManager.bosses[j]->specialEffect = NULL;
            }
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_ENEMY_SPELLCARD_END, 0);
    }
    g_Stage.spellCardState = SPELLCARD_STATE_INACTIVE;
}

ZunResult EclManager::RunEcl(Enemy *enemy)
{
    EclInterp *interp2;
    i32 posModified;
    f32 t2;
    i32 interpIdx2;
    u32 anmDirection;
    f32 t1;
    ZunVec3 unused_e4;
    ZunVec3 moveVec;
    ZunVec3 relEnemySpawnPos;
    AnyArg relSpawnInstrArgs[7];
    ZunVec3 absEnemySpawnPos;
    AnyArg absSpawnInstrArgs[7];
    ZunVec3 pointItemPos;
    i32 pointItemIdx;
    i32 numPointItems;
    ZunVec3 itemDropPos;
    i32 itemDropIdx;
    i32 numDrops;
    ZunVec3 particleVel;
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
    i32 arg;

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
            g_EclManager.CallEclSub(&enemy->currentContext, (i16)enemy->periodicCallbackSub);
            if (enemy->stackDepth < ENEMY_STACK_SIZE)
            {
                enemy->stackDepth++;
            }
            instr = enemy->currentContext.curInstr;
            enemy->currentContext.isPeriodicSub = 1;
        }
    }
    for (;;)
    {
        if (enemy->currentContext.waitTimer.GetCurrent() > 0)
        {
            enemy->currentContext.waitTimer--;
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
            case ECL_UNIMP:
                return ZUN_ERROR;
            case ECL_SET_WAIT_TIMER:
                enemy->currentContext.waitTimer = GET_INT_VALUE(enemy, 0);
                break;
            case ECL_DEC_JUMP:
                *GET_INT_PTR(enemy, 2) -= 1;
                if (GET_INT_VALUE(enemy, 2) <= 0)
                {
                    break;
                }
            case ECL_JUMP:
                enemy->currentContext.time.current = instr->args[0].i;
                instr = (EclRawInstr *)((u8 *)instr + instr->args[1].i);
                continue;
            case ECL_SET_INT:
                *GET_INT_PTR(enemy, 0) = GET_INT_VALUE(enemy, 1);
                break;
            case ECL_SET_FLOAT:
                *GET_FLOAT_PTR(enemy, 0) = GET_FLOAT_VALUE(enemy, 1);
                break;
            case ECL_NORMALIZE_ANGLE:
                *GET_FLOAT_PTR(enemy, 0) =
                    utils::AddNormalizeAngle(GET_FLOAT_VALUE(enemy, 0), 0.0f);
                break;
            case ECL_RAND:
                *GET_INT_PTR(enemy, 0) = g_Rng.GetRandomU32InRange(GET_INT_VALUE(enemy, 1));
                break;
            case ECL_RAND_ADD:
                *GET_INT_PTR(enemy, 0) =
                    g_Rng.GetRandomU32InRange(GET_INT_VALUE(enemy, 1)) + GET_INT_VALUE(enemy, 2);
                break;
            case ECL_RAND_FLOAT:
                *GET_FLOAT_PTR(enemy, 0) = g_Rng.GetRandomFloatInRange(GET_FLOAT_VALUE(enemy, 1));
                break;
            case ECL_RAND_FLOAT_ADD:
                *GET_FLOAT_PTR(enemy, 0) = g_Rng.GetRandomFloatInRange(GET_FLOAT_VALUE(enemy, 1)) +
                                           GET_FLOAT_VALUE(enemy, 2);
                break;
            case ECL_RAND_SIGN:
                *GET_INT_PTR(enemy, 0) =
                    ((g_Rng.GetRandomU16() & 1) != 0 ? 1 : -1) * GET_INT_VALUE(enemy, 1);
                break;
            case ECL_RAND_SIGN_FLOAT:
                *GET_FLOAT_PTR(enemy, 0) =
                    ((g_Rng.GetRandomU16() & 1) != 0 ? 1.0f : -1.0f) * GET_FLOAT_VALUE(enemy, 1);
                break;
            case ECL_INC:
                *GET_INT_PTR(enemy, 0) += 1;
                break;
            case ECL_DEC:
                *GET_INT_PTR(enemy, 0) -= 1;
                break;
            case ECL_GET_BOSS_INT:
                *GET_INT_PTR(enemy, 0) =
                    GET_INT_VALUE(g_EnemyManager.bosses[GET_INT_VALUE(enemy, 2)], 1);
                break;
            case ECL_GET_BOSS_FLOAT:
                *GET_FLOAT_PTR(enemy, 0) =
                    GET_FLOAT_VALUE(g_EnemyManager.bosses[GET_INT_VALUE(enemy, 2)], 1);
                break;
            case ECL_ADD:
                *GET_INT_PTR(enemy, 0) = GET_INT_VALUE(enemy, 1) + GET_INT_VALUE(enemy, 2);
                break;
            case ECL_ADD_FLOAT:
                *GET_FLOAT_PTR(enemy, 0) = GET_FLOAT_VALUE(enemy, 1) + GET_FLOAT_VALUE(enemy, 2);
                break;
            case ECL_SUB:
                *GET_INT_PTR(enemy, 0) = GET_INT_VALUE(enemy, 1) - GET_INT_VALUE(enemy, 2);
                break;
            case ECL_SUB_FLOAT:
                *GET_FLOAT_PTR(enemy, 0) = GET_FLOAT_VALUE(enemy, 1) - GET_FLOAT_VALUE(enemy, 2);
                break;
            case ECL_MUL:
                *GET_INT_PTR(enemy, 0) = GET_INT_VALUE(enemy, 1) * GET_INT_VALUE(enemy, 2);
                break;
            case ECL_MUL_FLOAT:
                *GET_FLOAT_PTR(enemy, 0) = GET_FLOAT_VALUE(enemy, 1) * GET_FLOAT_VALUE(enemy, 2);
                break;
            case ECL_DIV:
                *GET_INT_PTR(enemy, 0) = GET_INT_VALUE(enemy, 1) / GET_INT_VALUE(enemy, 2);
                break;
            case ECL_DIV_FLOAT:
                *GET_FLOAT_PTR(enemy, 0) = GET_FLOAT_VALUE(enemy, 1) / GET_FLOAT_VALUE(enemy, 2);
                break;
            case ECL_MOD:
                *GET_INT_PTR(enemy, 0) = GET_INT_VALUE(enemy, 1) % GET_INT_VALUE(enemy, 2);
                break;
            case ECL_MOD_FLOAT:
                *GET_FLOAT_PTR(enemy, 0) =
                    fmodf(GET_FLOAT_VALUE(enemy, 1), GET_FLOAT_VALUE(enemy, 2));
                break;
            case ECL_SIN:
                *GET_FLOAT_PTR(enemy, 0) = sinf(GET_FLOAT_VALUE(enemy, 1));
                break;
            case ECL_COS:
                *GET_FLOAT_PTR(enemy, 0) = cosf(GET_FLOAT_VALUE(enemy, 1));
                break;
            case ECL_ATAN2:
                *GET_FLOAT_PTR(enemy, 0) =
                    atan2f(GET_FLOAT_VALUE(enemy, 4) - GET_FLOAT_VALUE(enemy, 2),
                           GET_FLOAT_VALUE(enemy, 3) - GET_FLOAT_VALUE(enemy, 1));
                break;
            case ECL_LERP:
                lerpDelta = GET_FLOAT_VALUE(enemy, 1) - GET_FLOAT_VALUE(enemy, 2);
                *GET_FLOAT_PTR(enemy, 0) =
                    lerpDelta * GET_FLOAT_VALUE(enemy, 3) + GET_FLOAT_VALUE(enemy, 2);
                break;
            case ECL_INIT_INTERP:
                interp = enemy->currentContext.interps;
                for (interpIdx = 0; interpIdx < ARRAY_SIZE_SIGNED(enemy->currentContext.interps);
                     interpIdx++, interp++)
                {
                    if (interp->fn && interp->args[7].f != instr->args[0].f)
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
            case ECL_JUMP_IF_EQ:
                if (GET_INT_VALUE(enemy, 0) == GET_INT_VALUE(enemy, 1))
                {
                    goto jump;
                }
                break;
            case ECL_JUMP_IF_EQ_FLOAT:
                if (GET_FLOAT_VALUE(enemy, 0) == GET_FLOAT_VALUE(enemy, 1))
                {
                    goto jump;
                }
                break;
            case ECL_JUMP_IF_NEQ:
                if (GET_INT_VALUE(enemy, 0) != GET_INT_VALUE(enemy, 1))
                {
                    goto jump;
                }
                break;
            case ECL_JUMP_IF_NEQ_FLOAT:
                if (GET_FLOAT_VALUE(enemy, 0) != GET_FLOAT_VALUE(enemy, 1))
                {
                    goto jump;
                }
                break;
            case ECL_JUMP_IF_LT:
                if (GET_INT_VALUE(enemy, 0) < GET_INT_VALUE(enemy, 1))
                {
                    goto jump;
                }
                break;
            case ECL_JUMP_IF_LT_FLOAT:
                if (GET_FLOAT_VALUE(enemy, 0) < GET_FLOAT_VALUE(enemy, 1))
                {
                    goto jump;
                }
                break;
            case ECL_JUMP_IF_LEQ:
                if (GET_INT_VALUE(enemy, 0) <= GET_INT_VALUE(enemy, 1))
                {
                    goto jump;
                }
                break;
            case ECL_JUMP_IF_LEQ_FLOAT:
                if (GET_FLOAT_VALUE(enemy, 0) <= GET_FLOAT_VALUE(enemy, 1))
                {
                    goto jump;
                }
                break;
            case ECL_JUMP_IF_GT:
                if (GET_INT_VALUE(enemy, 0) > GET_INT_VALUE(enemy, 1))
                {
                    goto jump;
                }
                break;
            case ECL_JUMP_IF_GT_FLOAT:
                if (GET_FLOAT_VALUE(enemy, 0) > GET_FLOAT_VALUE(enemy, 1))
                {
                    goto jump;
                }
                break;
            case ECL_JUMP_IF_GEQ:
                if (GET_INT_VALUE(enemy, 0) >= GET_INT_VALUE(enemy, 1))
                {
                    goto jump;
                }
                break;
            case ECL_JUMP_IF_GEQ_FLOAT:
                if (GET_FLOAT_VALUE(enemy, 0) >= GET_FLOAT_VALUE(enemy, 1))
                {
                    goto jump;
                }
                break;
            jump:
                enemy->currentContext.time.current = instr->args[2].i;
                instr = (EclRawInstr *)((u8 *)instr + instr->args[3].i);
                continue;
            case ECL_SUB_CALL:
                arg = instr->args[0].i;
                enemy->currentContext.curInstr = (EclRawInstr *)((u8 *)instr + instr->size);
                if (!enemy->noStackRet)
                {
                    enemy->savedContextStack[enemy->stackDepth] = enemy->currentContext;
                }
                g_EclManager.CallEclSub(&enemy->currentContext, (i16)arg);
                enemy->currentContext.eclContextArgs.globalVars = g_GlobalEclVars;
                if (!enemy->noStackRet && enemy->stackDepth < ENEMY_STACK_SIZE)
                {
                    enemy->stackDepth++;
                }
                goto restart;
            case ECL_SUB_RET:
                if (enemy->noStackRet)
                {
                    Supervisor::DebugPrint("error : no Stack Ret\n");
                }
                enemy->stackDepth--;
                if (enemy->currentContext.isPeriodicSub)
                {
                    enemy->savedEclContextArgs = enemy->currentContext.eclContextArgs;
                    enemy->currentContext.isPeriodicSub = 0;
                }
                enemy->currentContext = enemy->savedContextStack[enemy->stackDepth];
                goto restart;
            case ECL_SET_ANM:
                g_AnmManager->SetAnmIdxAndExecuteScript(
                    &enemy->primaryVm, GET_INT_VALUE(enemy, 0) + ANM_SCRIPT_ENEMY_ARRAY);
                break;
            case ECL_SET_SUB_ANM:
                if (GET_INT_VALUE(enemy, 0) >= ARRAY_SIZE_SIGNED(enemy->vms))
                {
                    Supervisor::DebugPrint("error : sub anim overflow\n");
                }
                if (GET_INT_VALUE(enemy, 1) >= 0)
                {
                    g_AnmManager->SetAnmIdxAndExecuteScript(&enemy->vms[GET_INT_VALUE(enemy, 0)],
                                                            GET_INT_VALUE(enemy, 1) +
                                                                ANM_SCRIPT_ENEMY_ARRAY);
                }
                else
                {
                    enemy->vms[GET_INT_VALUE(enemy, 0)].anmFileIdx = -1;
                }
                break;
            case ECL_SET_POS:
                enemy->pos.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->pos.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->pos.z = GET_FLOAT_VALUE(enemy, 2);
                enemy->ClampPos();
                break;
            case ECL_SET_AXIS_SPEED:
                enemy->axisSpeed.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->axisSpeed.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->axisSpeed.z = GET_FLOAT_VALUE(enemy, 2);
                enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
                enemy->moveMode = ENEMY_MOVE_AXIS;
                break;
            case ECL_SET_ANGULAR_VEL:
                enemy->angularVelocity = GET_FLOAT_VALUE(enemy, 0);
                enemy->moveMode = ENEMY_MOVE_POLAR;
                break;
            case ECL_MOVE_AT_PLAYER:
                enemy->angle = g_Player.AngleToPlayer(&enemy->pos) + GET_FLOAT_VALUE(enemy, 0);
                enemy->moveSpeed = GET_FLOAT_VALUE(enemy, 1);
                enemy->moveMode = ENEMY_MOVE_POLAR;
                break;
            case ECL_SET_MOVE_SPEED:
                enemy->moveSpeed = GET_FLOAT_VALUE(enemy, 0);
                enemy->moveMode = ENEMY_MOVE_POLAR;
                break;
            case ECL_SET_MOVE_ACCEL:
                enemy->moveAcceleration = GET_FLOAT_VALUE(enemy, 0);
                enemy->moveMode = ENEMY_MOVE_POLAR;
                break;
            case ECL_SET_MOVE_INTERP_TIMER_POLAR:
                enemy->moveMode = ENEMY_MOVE_POLAR;
                enemy->moveInterpTimer = enemy->moveInterpStartTime = GET_INT_VALUE(enemy, 0);
                break;
            case ECL_SET_MOVE_INTERP_TIMER_RADIAL:
                enemy->moveMode = ENEMY_MOVE_ORBIT;
                enemy->moveInterpTimer = enemy->moveInterpStartTime = GET_INT_VALUE(enemy, 0);
                break;
            case ECL_SET_MOVE_INTERP_TIMER_INTERP:
                enemy->moveMode = ENEMY_MOVE_INTERP;
                enemy->moveInterpTimer = enemy->moveInterpStartTime = GET_INT_VALUE(enemy, 0);
                break;
            case ECL_SPAWN_BULLET_PATTERN_SPREAD_AIMED:
            case ECL_SPAWN_BULLET_PATTERN_SPREAD_ABS:
            case ECL_SPAWN_BULLET_PATTERN_RING_AIMED:
            case ECL_SPAWN_BULLET_PATTERN_RING_ABS:
            case ECL_SPAWN_BULLET_PATTERN_RING_SHIFTED_AIMED:
            case ECL_SPAWN_BULLET_PATTERN_RING_SHIFTED_ABS:
            case ECL_SPAWN_BULLET_PATTERN_ANGLE_RANDOM:
            case ECL_SPAWN_BULLET_PATTERN_RING_SPEED_RANDOM:
            case ECL_SPAWN_BULLET_PATTERN_RANDOM:
                if (enemy->life <= 0)
                {
                    break;
                }
                bulletInstrArgs = instr->args;
                bulletProps = &enemy->bulletProps;
                arg = bulletInstrArgs->s[0];
                bulletProps->sprite = (instr->paramMask & 1) != 0 ? GetVarValue(enemy, arg) : arg;
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
                    bulletProps->speed2 += enemy->BulletRankSpeed(g_GameManager.rank.rank) / 2.0f;
                    if (bulletProps->speed2 < 0.3f)
                    {
                        bulletProps->speed2 = 0.3f;
                    }
                }
                bulletProps->unused_c2 = 0;
                bulletProps->flags = bulletInstrArgs[7].u;
                arg = bulletInstrArgs->s[1];
                bulletProps->spriteOffset =
                    (instr->paramMask & 2) != 0 ? GetVarValue(enemy, arg) : arg;
                if (!enemy->disableBullets)
                {
                    g_BulletManager.SpawnBulletPattern(bulletProps);
                }
                break;
            case ECL_INIT_BULLET_CMD:
                bulletCommand = &enemy->bulletProps.commands[GET_INT_VALUE(enemy, 0)];
                bulletCommand->type = GET_INT_VALUE(enemy, 1);
                bulletCommand->flag = GET_INT_VALUE(enemy, 2);
                bulletCommand->duration = GET_INT_VALUE(enemy, 3);
                bulletCommand->loopCount = GET_INT_VALUE(enemy, 4);
                bulletCommand->speed = GET_FLOAT_VALUE(enemy, 5);
                bulletCommand->angle = GET_FLOAT_VALUE(enemy, 6);
                break;
            case ECL_SET_DEATH_ANM:
                enemy->deathAnm1 = instr->args[0].c[0];
                enemy->deathAnm2 = instr->args[0].c[1];
                enemy->deathAnm3 = instr->args[0].c[2];
                break;
            case ECL_SET_SHOOT_INTERVAL:
                enemy->shootInterval = GET_INT_VALUE(enemy, 0);
                if (enemy->shootInterval != 0)
                {
                    enemy->shootInterval += enemy->ShootInterval(g_GameManager.rank.rank);
                    enemy->shootIntervalTimer = 0;
                }
                break;
            case ECL_SET_SHOOT_INTERVAL_RAND:
                enemy->shootInterval = GET_INT_VALUE(enemy, 0);
                if (enemy->shootInterval != 0)
                {
                    enemy->shootInterval += enemy->ShootInterval(g_GameManager.rank.rank);
                    enemy->shootIntervalTimer = g_Rng.GetRandomU32InRange(enemy->shootInterval);
                }
                break;
            case ECL_DISABLE_BULLETS:
                enemy->disableBullets = 1;
                break;
            case ECL_ENABLE_BULLETS:
                enemy->disableBullets = 0;
                break;
            case ECL_SPAWN_PREV_BULLET_PATTERN:
                enemy->bulletProps.pos = enemy->pos + enemy->shootOffset;
                g_BulletManager.SpawnBulletPattern(&enemy->bulletProps);
                break;
            case ECL_SET_SHOOT_OFFSET:
                enemy->shootOffset.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->shootOffset.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->shootOffset.z = GET_FLOAT_VALUE(enemy, 2);
                break;
            case ECL_SPAWN_LASER_PATTERN_FIXED:
            case ECL_SPAWN_LASER_PATTERN_MOVING:
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

                if (instr->id == ECL_SPAWN_LASER_PATTERN_MOVING)
                {
                    laserProps->type = 0;
                }
                else
                {
                    laserProps->type = 1;
                }
                enemy->lasers[enemy->laserIdx] = g_BulletManager.SpawnLaserPattern(laserProps);
                break;
            case ECL_SET_LASER_IDX:
                enemy->laserIdx = GET_INT_VALUE(enemy, 0);
                break;
            case ECL_ADD_LASER_ANGLE:
                arg = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[arg])
                {
                    enemy->lasers[arg]->angle = utils::AddNormalizeAngle(enemy->lasers[arg]->angle,
                                                                         GET_FLOAT_VALUE(enemy, 1));
                }
                break;
            case ECL_SET_LASER_ANGLE:
                arg = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[arg])
                {
                    enemy->lasers[arg]->angle = GET_FLOAT_VALUE(enemy, 1);
                }
                break;
            case ECL_AIM_LASER_ANGLE_AT_PLAYER:
                arg = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[arg])
                {
                    enemy->lasers[arg]->angle = g_Player.AngleToPlayer(&enemy->lasers[arg]->pos) +
                                                GET_FLOAT_VALUE(enemy, 1);
                }
                break;
            case ECL_SET_LASER_POS_REL:
                arg = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[arg])
                {
                    enemy->lasers[arg]->pos.x = GET_FLOAT_VALUE(enemy, 1) + enemy->pos.x;
                    enemy->lasers[arg]->pos.y = GET_FLOAT_VALUE(enemy, 2) + enemy->pos.y;
                    enemy->lasers[arg]->pos.z = GET_FLOAT_VALUE(enemy, 3) + enemy->pos.z;
                }
                break;
            case ECL_SET_LASER_HIDE_WARNING:
                arg = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[arg])
                {
                    enemy->lasers[arg]->hideWarning = GET_INT_VALUE(enemy, 1);
                }
                break;
            case ECL_TEST_LASER_NOT_IN_USE:
                arg = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[arg] && enemy->lasers[arg]->inUse)
                {
                    enemy->currentContext.laserNotInUse = 0;
                }
                else
                {
                    enemy->currentContext.laserNotInUse = 1;
                }
                break;
            case ECL_STOP_LASER:
                arg = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[arg] && enemy->lasers[arg]->inUse &&
                    enemy->lasers[arg]->state < 2)
                {
                    enemy->lasers[arg]->state = 2;
                    enemy->lasers[arg]->timer = 0;
                    enemy->lasers[arg]->width = enemy->lasers[arg]->targetWidth;
                }
                break;
            case ECL_CLEAR_LASERS:
                for (laserIdx = 0; laserIdx < ARRAY_SIZE_SIGNED(enemy->lasers); laserIdx++)
                {
                    enemy->lasers[laserIdx] = NULL;
                }
                break;
            case ECL_SET_LASER_START_LEN:
                arg = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[arg])
                {
                    enemy->lasers[arg]->startLength = GET_FLOAT_VALUE(enemy, 1);
                }
                break;
            case ECL_SET_LASER_OFFSETS:
                arg = GET_INT_VALUE(enemy, 0);
                if (enemy->lasers[arg])
                {
                    enemy->lasers[arg]->startOffset = GET_FLOAT_VALUE(enemy, 1);
                    enemy->lasers[arg]->endOffset = GET_FLOAT_VALUE(enemy, 2);
                }
                break;
            case ECL_IDFK:
                g_EnemyManager.unused_9545f0 = GET_INT_VALUE(enemy, 0);
                break;
            case ECL_SET_BOSS:
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
            case ECL_SPAWN_EFFECT:
                effectInstrArgs = instr->args;
                enemy->effects[enemy->effectsNum] = g_EffectManager.SpawnEffect(
                    13, &enemy->pos, 1, g_BulletColor[effectInstrArgs->i]);
                enemy->effects[enemy->effectsNum]->direction = *(ZunVec3 *)&effectInstrArgs[1];
                enemy->effectDistance = effectInstrArgs[4].f;
                enemy->effectsNum++;
                break;
            case ECL_MOVE_DIR_TIME:
                if (GET_INT_VALUE(enemy, 0) <= 0)
                {
                    enemy->angle = utils::AddNormalizeAngle(GET_FLOAT_VALUE(enemy, 2), 0.0f);
                    enemy->moveSpeed = GET_FLOAT_VALUE(enemy, 3);
                    enemy->moveMode = ENEMY_MOVE_POLAR;
                    enemy->moveInterpTimer = enemy->moveInterpStartTime = GET_INT_VALUE(enemy, 0);
                }
                else
                {
                    MoveDirTime(enemy, instr);
                }
                break;
            case ECL_MOVE_POS_TIME:
                MovePosTime(enemy, instr);
                break;
            case ECL_MOVE_ORBIT:
                enemy->moveInterpTimer = enemy->moveInterpStartTime = GET_INT_VALUE(enemy, 0);
                enemy->moveInterpStartPos.x = GET_FLOAT_VALUE(enemy, 1);
                enemy->moveInterpStartPos.y = GET_FLOAT_VALUE(enemy, 2);
                enemy->moveInterpStartPos.z = GET_FLOAT_VALUE(enemy, 3);
                enemy->moveAngle = GET_FLOAT_VALUE(enemy, 4);
                enemy->moveAngularVelocity = GET_FLOAT_VALUE(enemy, 5);
                enemy->moveRadius = GET_FLOAT_VALUE(enemy, 6);
                enemy->moveRadialVelocity = GET_FLOAT_VALUE(enemy, 7);
                enemy->moveMode = ENEMY_MOVE_ORBIT;
                break;
            case ECL_SET_ORBIT_RADIUS:
                enemy->moveRadius = GET_FLOAT_VALUE(enemy, 0);
                enemy->moveRadialVelocity = GET_FLOAT_VALUE(enemy, 1);
                break;
            case ECL_SET_ORBIT_ANGLE:
                enemy->moveAngle = GET_FLOAT_VALUE(enemy, 0);
                enemy->moveAngularVelocity = GET_FLOAT_VALUE(enemy, 1);
                break;
            case ECL_SET_MOVEMENT_BOUNDS:
                enemy->lowerMoveLimit.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->lowerMoveLimit.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->upperMoveLimit.x = GET_FLOAT_VALUE(enemy, 2);
                enemy->upperMoveLimit.y = GET_FLOAT_VALUE(enemy, 3);
                enemy->hasMovementBounds = 1;
                break;
            case ECL_DISABLE_MOVEMENT_BOUNDS:
                enemy->hasMovementBounds = 0;
                break;
            case ECL_RAND_FLOAT_RANGE:
                *GET_FLOAT_PTR(enemy, 0) = g_Rng.GetRandomFloatInRange(GET_FLOAT_VALUE(enemy, 2) -
                                                                       GET_FLOAT_VALUE(enemy, 1)) +
                                           GET_FLOAT_VALUE(enemy, 1);
                break;
            case ECL_GET_EXIT_ANGLE:
                if (g_Player.positionCenter.x < enemy->pos.x)
                {
                    exitAngle = utils::AddNormalizeAngle(
                        g_Rng.GetRandomFloatInRange(ZUN_PI / 2.0f) + ZUN_3PI / 4.0f, 0.0f);
                }
                else
                {
                    exitAngle = g_Rng.GetRandomFloatInRange(ZUN_PI / 2.0f) - ZUN_PI / 4.0f;
                }
                if (enemy->pos.x < enemy->lowerMoveLimit.x + 96.0f)
                {
                    if (exitAngle > ZUN_PI / 2.0f)
                    {
                        exitAngle = ZUN_PI - exitAngle;
                    }
                    else if (exitAngle < -ZUN_PI / 2.0f)
                    {
                        exitAngle = -ZUN_PI - exitAngle;
                    }
                }
                if (enemy->upperMoveLimit.x - 96.0f < enemy->pos.x)
                {
                    if (exitAngle < ZUN_PI / 2.0f && exitAngle >= 0.0f)
                    {
                        exitAngle = ZUN_PI - enemy->angle;
                    }
                    else if (exitAngle > -ZUN_PI / 2.0f && exitAngle <= 0.0f)
                    {
                        exitAngle = -ZUN_PI - exitAngle;
                    }
                }
                if (enemy->lowerMoveLimit.y + 48.0f > enemy->pos.y && exitAngle < 0.0f)
                {
                    exitAngle = -exitAngle;
                }
                if (enemy->upperMoveLimit.y - 48.0f < enemy->pos.y && exitAngle > 0.0f)
                {
                    exitAngle = -exitAngle;
                }
                *GET_FLOAT_PTR(enemy, 0) = exitAngle;
                break;
            case ECL_SET_MOVE_ANM:
                enemy->anmExDefaults = instr->args[0].s[0];
                enemy->anmExFarLeft = instr->args[0].s[1];
                enemy->anmExFarRight = instr->args[1].s[0];
                enemy->anmExLeft = instr->args[1].s[1];
                enemy->anmExRight = instr->args[2].s[0];
                enemy->anmExFlags = 255;
                break;
            case ECL_SET_HITBOX_SIZE:
                enemy->hitboxSize.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->hitboxSize.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->hitboxSize.z = GET_FLOAT_VALUE(enemy, 2);
                break;
            case ECL_SET_GRAZE_SIZE:
                enemy->grazeSize.x = GET_FLOAT_VALUE(enemy, 0);
                enemy->grazeSize.y = GET_FLOAT_VALUE(enemy, 1);
                enemy->grazeSize.z = GET_FLOAT_VALUE(enemy, 2);
                break;
            case ECL_SET_HAS_CONTACT_HITBOX:
                enemy->hasContactHitbox = instr->args[0].b[0];
                break;
            case ECL_SET_CAN_BE_DAMAGED:
                enemy->canBeDamaged = instr->args[0].b[0];
                break;
            case ECL_SET_IS_HITTABLE:
                enemy->isHittable = instr->args[0].b[0];
                break;
            case ECL_PLAY_SOUND:
                g_SoundPlayer.PlaySoundByIdx(GET_INT_VALUE(enemy, 0), 0);
                break;
            case ECL_SET_DEATH_TYPE:
                enemy->deathType = instr->args[0].b[0];
                break;
            case ECL_SET_DEATH_CALLBACK_SUB:
                enemy->deathCallbackSub = (u32)instr->args[0].b[0];
                break;
            case ECL_SET_INTERRUPT:
                enemy->interrupts[GET_INT_VALUE(enemy, 1)] = GET_INT_VALUE(enemy, 0);
                break;
            case ECL_SET_RUN_INTERRUPT:
                enemy->runInterrupt = GET_INT_VALUE(enemy, 0);
            handle_interrupt:
                enemy->currentContext.curInstr = (EclRawInstr *)((u8 *)instr + instr->size);
                if (!enemy->noStackRet)
                {
                    enemy->savedContextStack[enemy->stackDepth] = enemy->currentContext;
                }
                g_EclManager.CallEclSub(&enemy->currentContext,
                                        enemy->interrupts[enemy->runInterrupt]);
                if (enemy->stackDepth < ENEMY_STACK_SIZE)
                {
                    enemy->stackDepth = enemy->stackDepth + 1;
                }
                enemy->runInterrupt = -1;
                goto restart;
            case ECL_SET_LIFE:
                enemy->life = enemy->maxLife = GET_INT_VALUE(enemy, 0);
                if (enemy->bossId == 0 && enemy->isBoss)
                {
                    for (healthIdx = 0; healthIdx < ARRAY_SIZE_SIGNED(g_Gui.bossHealth);
                         healthIdx++)
                    {
                        g_Gui.bossHealthEased[healthIdx] = 0.0f;
                        g_Gui.bossHealth[healthIdx] = 0.0f;
                    }
                }
                break;
            case ECL_SET_BOSS_HEALTH:
                bossIdx = GET_INT_VALUE(enemy, 0);
                g_Gui.SetBossHealth(bossIdx, GET_INT_VALUE(enemy, 1) / (f32)enemy->maxLife,
                                    GET_INT_VALUE(enemy, 2) / (f32)enemy->maxLife);
                g_Gui.bossColor[bossIdx] = GET_INT_VALUE(enemy, 3);
                break;
            case ECL_BEGIN_SPELLCARD:
                BeginSpellcard(enemy, instr);
                break;
            case ECL_END_SPELLCARD:
                EndSpellcard();
                break;
            case ECL_SET_TIMER:
                enemy->timer = GET_INT_VALUE(enemy, 0);
                break;
            case ECL_SET_LIFE_CALLBACK_THRESHOLD:
                enemy->lifeCallbackThreshold[0] = GET_INT_VALUE(enemy, 0);
                break;
            case ECL_SET_LIFE_CALLBACK_SUB:
                enemy->lifeCallbackSub[0] = GET_INT_VALUE(enemy, 0);
                break;
            case ECL_SET_LIFE_CALLBACK:
                enemy->lifeCallbackThreshold[GET_INT_VALUE(enemy, 0)] = GET_INT_VALUE(enemy, 1);
                enemy->lifeCallbackSub[GET_INT_VALUE(enemy, 0)] = GET_INT_VALUE(enemy, 2);
                break;
            case ECL_SET_TIMER_CALLBACK_THRESHOLD:
                enemy->timerCallbackThreshold = GET_INT_VALUE(enemy, 0);
                enemy->timer = 0;
                break;
            case ECL_SET_TIMER_CALLBACK_SUB:
                enemy->timerCallbackSub = GET_INT_VALUE(enemy, 0);
                break;
            case ECL_SET_PERIODIC_CALLBACK:
                enemy->periodicTimer = GET_INT_VALUE(enemy, 0);
                enemy->periodicCallbackSub = GET_INT_VALUE(enemy, 1);
                enemy->periodicCounter = 0;
                enemy->savedEclContextArgs = enemy->currentContext.eclContextArgs;
                break;
            case ECL_SET_ENEMY_CAN_DIE:
                enemy->canDie = instr->args[0].b[0];
                break;
            case ECL_SPAWN_PARTICLES:
                g_EffectManager.SpawnEffect(GET_INT_VALUE(enemy, 0), &enemy->pos,
                                            GET_INT_VALUE(enemy, 1), *(u32 *)GET_INT_PTR(enemy, 2));
                break;
            case ECL_SPAWN_MOVING_PARTICLES:
                particleVel.x = GET_FLOAT_VALUE(enemy, 3);
                particleVel.y = GET_FLOAT_VALUE(enemy, 4);
                particleVel.z = GET_FLOAT_VALUE(enemy, 5);
                g_EffectManager.SpawnMovingParticles(GET_INT_VALUE(enemy, 0), &enemy->pos,
                                                     &particleVel, GET_INT_VALUE(enemy, 1),
                                                     *(u32 *)GET_INT_PTR(enemy, 2));
                break;
            case ECL_SPAWN_ITEMS:
                numDrops = GET_INT_VALUE(enemy, 0);
                for (itemDropIdx = 0; itemDropIdx < numDrops; itemDropIdx++)
                {
                    itemDropPos = enemy->pos;
                    itemDropPos.x += g_Rng.GetRandomFloatInRange(128.0f) - 64.0f;
                    itemDropPos.y += g_Rng.GetRandomFloatInRange(128.0f) - 64.0f;
                    if ((i32)g_GameManager.globals->currentPower < 128)
                    {
                        g_ItemManager.SpawnItem(
                            &itemDropPos, itemDropIdx == 0 ? ITEM_POWER_BIG : ITEM_POWER_SMALL, 0);
                    }
                    else
                    {
                        g_ItemManager.SpawnItem(&itemDropPos, ITEM_POINT, 0);
                    }
                }
                break;
            case ECL_SPAWN_POINT_ITEMS:
                numPointItems = GET_INT_VALUE(enemy, 0);
                for (pointItemIdx = 0; pointItemIdx < numPointItems; pointItemIdx++)
                {
                    pointItemPos = enemy->pos;
                    pointItemPos.x += g_Rng.GetRandomFloatInRange(128.0f) - 64.0f;
                    pointItemPos.y += g_Rng.GetRandomFloatInRange(128.0f) - 64.0f;
                    g_ItemManager.SpawnItem(&pointItemPos, ITEM_POINT, 0);
                }
                break;
            case ECL_SET_VM_AUTO_ROTATE:
                enemy->primaryVmAutoRotate = instr->args[0].b[0];
                break;
            case ECL_RUN_EX_INS:
                g_EclExInstr[GET_INT_VALUE(enemy, 0)](enemy, instr);
                break;
            case ECL_SET_EX_INS:
                if (GET_INT_VALUE(enemy, 0) >= 0)
                {
                    enemy->currentContext.func = g_EclExInstr[GET_INT_VALUE(enemy, 0)];
                    enemy->currentContext.eclExInstr = instr;
                }
                else
                {
                    enemy->currentContext.func = NULL;
                }
                break;
            case ECL_ADD_TIME:
                enemy->currentContext.time += GET_INT_VALUE(enemy, 0);
                break;
            case ECL_SPAWN_ITEM:
                g_ItemManager.SpawnItem(&enemy->pos, GET_INT_VALUE(enemy, 0), 0);
                break;
            case ECL_SET_SCRIPT_WAIT_TIME:
                g_Stage.scriptWaitTime = GET_INT_VALUE(enemy, 0);
                break;
            case ECL_SET_NUM_BOSS_LIFE_MARKERS:
                g_Gui.bossLifeMarkers = GET_INT_VALUE(enemy, 0);
                g_GameManager.playTimeAll += 1800;
                break;
            case ECL_SPAWN_ENEMY_ABS:
                if (enemy->life > 0)
                {
                    memcpy(absSpawnInstrArgs, instr->args, sizeof(absSpawnInstrArgs));
                    absEnemySpawnPos.x = GET_FLOAT_VALUE_D(enemy, absSpawnInstrArgs, 1, 1);
                    absEnemySpawnPos.y = GET_FLOAT_VALUE_D(enemy, absSpawnInstrArgs, 2, 2);
                    absEnemySpawnPos.z = GET_FLOAT_VALUE_D(enemy, absSpawnInstrArgs, 3, 3);
                    g_EnemyManager.SpawnEnemyEx(absSpawnInstrArgs[0].i, &absEnemySpawnPos,
                                                GET_INT_VALUE(enemy, 4), GET_INT_VALUE(enemy, 5),
                                                GET_INT_VALUE(enemy, 6),
                                                &enemy->currentContext.eclContextArgs);
                }
                break;
            case ECL_SPAWN_ENEMY_REL:
                if (enemy->life > 0)
                {
                    memcpy(relSpawnInstrArgs, instr->args, sizeof(relSpawnInstrArgs));
                    relEnemySpawnPos.x = GET_FLOAT_VALUE_D(enemy, relSpawnInstrArgs, 1, 1);
                    relEnemySpawnPos.y = GET_FLOAT_VALUE_D(enemy, relSpawnInstrArgs, 2, 2);
                    relEnemySpawnPos.z = GET_FLOAT_VALUE_D(enemy, relSpawnInstrArgs, 3, 3);
                    relEnemySpawnPos += enemy->pos;
                    g_EnemyManager.SpawnEnemyEx(relSpawnInstrArgs[0].i, &relEnemySpawnPos,
                                                GET_INT_VALUE(enemy, 4), GET_INT_VALUE(enemy, 5),
                                                GET_INT_VALUE(enemy, 6),
                                                &enemy->currentContext.eclContextArgs);
                }
                break;
            case ECL_REMOVE_ALL_ENEMIES:
                g_EnemyManager.RemoveAllEnemies(8000, 0);
                break;
            case ECL_SET_PRIMARY_VM_INTERRUPT:
                enemy->primaryVm.pendingInterrupt = GET_INT_VALUE(enemy, 0);
                break;
            case ECL_SET_VM_INTERRUPT:
                enemy->vms[instr->args[0].i].pendingInterrupt = instr->args[1].s[0];
                break;
            case ECL_REMOVE_ALL_BULLETS_SPAWN_ITEMS:
                g_BulletManager.RemoveAllBullets(1);
                break;
            case ECL_SET_BULLET_SOUND:
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
            case ECL_SET_NO_STACK_RET:
                enemy->noStackRet = instr->args[0].b[0];
                break;
            case ECL_SET_BULLET_RANK_PARAMS:
                enemy->bulletRankSpeedLow = GET_FLOAT_VALUE(enemy, 0);
                enemy->bulletRankSpeedHigh = GET_FLOAT_VALUE(enemy, 1);
                enemy->bulletRankAmount1Low = GET_INT_VALUE(enemy, 2);
                enemy->bulletRankAmount1High = GET_INT_VALUE(enemy, 3);
                enemy->bulletRankAmount2Low = GET_INT_VALUE(enemy, 4);
                enemy->bulletRankAmount2High = GET_INT_VALUE(enemy, 5);
                break;
            case ECL_SET_HAS_NO_COLLISION:
                enemy->hasNoCollision = instr->args[0].b[0];
                break;
            case ECL_BIND_TIMER_CALLBACK_TO_DEATH:
                enemy->timerCallbackSub = enemy->deathCallbackSub;
                enemy->timer = 0;
                break;
            case ECL_SET_IS_SURVIVAL_SPELLCARD:
                enemy->isSurvivalSpellcard = instr->args[0].b[0];
                break;
            case ECL_SET_IS_PROJECTILE:
                enemy->isProjectile = instr->args[0].b[0];
                enemy->zLayer = 2;
                break;
            case ECL_SET_DESPAWN_ON_OOB:
                enemy->disableOOBDespawn = instr->args[0].b[0];
                break;
            case ECL_SET_TRAIL:
                enemy->trailFlags = instr->args[0].c[0];
                enemy->trailCount = GET_INT_VALUE(enemy, 1);
                enemy->trailInterval = GET_INT_VALUE(enemy, 2);
                enemy->trailNodeStep = GET_INT_VALUE(enemy, 3);
                if ((enemy->trailFlags & 8) != 0)
                {
                    g_AnmManager->UpdateTrail(&enemy->primaryVm, enemy->trailVertices,
                                              (i32)enemy->trailCount / (i32)enemy->trailNodeStep
                                                  << 1);
                }
                break;
            case ECL_SET_GLOBAL_EFFECT_COLOR_MUL:
                g_EffectManager.globalColorMultiplierR = GET_FLOAT_VALUE(enemy, 0);
                g_EffectManager.globalColorMultiplierG = GET_FLOAT_VALUE(enemy, 1);
                g_EffectManager.globalColorMultiplierB = GET_FLOAT_VALUE(enemy, 2);
                g_EffectManager.globalColorMultiplierA = GET_FLOAT_VALUE(enemy, 3);
                break;
            case ECL_SET_INVINCIBILITY_TIMER:
                enemy->invincibilityTimer = GET_INT_VALUE(enemy, 0);
                break;
            case ECL_REMOVE_BULLETS_RADIUS:
                g_BulletManager.RemoveBulletsInRadius(&enemy->pos, GET_FLOAT_VALUE(enemy, 0));
                break;
            case ECL_SET_BOSS_RUN_INTERRUPT:
                if (g_EnemyManager.bosses[GET_INT_VALUE(enemy, 0)] != NULL)
                {
                    g_EnemyManager.bosses[GET_INT_VALUE(enemy, 0)]->runInterrupt =
                        GET_INT_VALUE(enemy, 1);
                }
                break;
            case ECL_REMOVE_ALL_BULLETS_NO_ITEMS:
                g_BulletManager.RemoveAllBullets(0);
                break;
            case ECL_SET_SPECIAL_EFFECT_POS:
                enemy->customSpecialEffectPos = GET_INT_VALUE(enemy, 0);
                if (!enemy->customSpecialEffectPos)
                {
                    enemy->specialEffect->pos1.x = GET_FLOAT_VALUE(enemy, 1);
                    enemy->specialEffect->pos1.y = GET_FLOAT_VALUE(enemy, 2);
                    enemy->specialEffect->pos1.z = GET_FLOAT_VALUE(enemy, 3);
                }
                break;
            case ECL_SET_PRIMARY_VM_ROT_Z:
                enemy->primaryVm.rotation.z = GET_FLOAT_VALUE(enemy, 0);
                break;
            case ECL_VEC_FROM_ANGLE_MAG:
                *GET_FLOAT_PTR(enemy, 1) =
                    sinf(GET_FLOAT_VALUE(enemy, 2)) * GET_FLOAT_VALUE(enemy, 3);
                *GET_FLOAT_PTR(enemy, 0) =
                    cosf(GET_FLOAT_VALUE(enemy, 2)) * GET_FLOAT_VALUE(enemy, 3);
                break;
            case ECL_RAND_EXIT_ANGLE:
                if ((g_Player.positionCenter.x < enemy->pos.x && enemy->pos.x > 96.0f) ||
                    enemy->pos.x > 288.0f)
                {
                    *GET_FLOAT_PTR(enemy, 0) = utils::AddNormalizeAngle(
                        g_Rng.GetRandomFloatInRange(ZUN_PI / 2.0f) + ZUN_3PI / 4.0f, 0.0f);
                }
                else
                {
                    *GET_FLOAT_PTR(enemy, 0) =
                        g_Rng.GetRandomFloatInRange(ZUN_PI / 2.0f) - ZUN_PI / 4.0f;
                }
                break;
            case ECL_ADD_CHERRY_PLUS:
                g_GameManager.AddCherryPlus(GET_INT_VALUE(enemy, 0));
                break;
            case ECL_FREEZE_ECL_DURING_BOMB:
                enemy->freezeEclDuringBombs = GET_INT_VALUE(enemy, 0);
                break;
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
            case ENEMY_MOVE_ORBIT:
                enemy->moveAngle = utils::AddNormalizeAngle(
                    enemy->moveAngle,
                    g_Supervisor.effectiveFramerateMultiplier * enemy->moveAngularVelocity);
                enemy->moveRadius =
                    g_Supervisor.effectiveFramerateMultiplier * enemy->moveRadialVelocity +
                    enemy->moveRadius;
                moveVec.FromAngleMagnitude(enemy->moveAngle, enemy->moveRadius);
                enemy->axisSpeed.x = moveVec.x + enemy->moveInterpStartPos.x - enemy->pos.x;
                enemy->axisSpeed.y = moveVec.y + enemy->moveInterpStartPos.y - enemy->pos.y;
                enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
                if (enemy->moveInterpStartTime > 0)
                {
                    enemy->moveInterpTimer--;
                    if (enemy->moveInterpTimer <= 0)
                    {
                        enemy->moveMode = ENEMY_MOVE_AXIS;
                    }
                }
                break;
            case ENEMY_MOVE_POLAR:
                enemy->angle = utils::AddNormalizeAngle(enemy->angle,
                                                        g_Supervisor.effectiveFramerateMultiplier *
                                                            enemy->angularVelocity);
                enemy->moveSpeed =
                    g_Supervisor.effectiveFramerateMultiplier * enemy->moveAcceleration +
                    enemy->moveSpeed;
                enemy->axisSpeed.FromAngleMagnitude(enemy->angle, enemy->moveSpeed);
                enemy->axisSpeed.z = 0.0f;
                if (enemy->moveInterpStartTime > 0)
                {
                    enemy->moveInterpTimer--;
                    if (enemy->moveInterpTimer <= 0)
                    {
                        enemy->moveMode = ENEMY_MOVE_AXIS;
                    }
                }
                break;
            case ENEMY_MOVE_INTERP:
                enemy->moveInterpTimer--;
                t1 = 1.0f - enemy->moveInterpTimer.AsFloat() / (f32)enemy->moveInterpStartTime;
                if (t1 < 0.0f)
                {
                    t1 = 0.0f;
                }
                switch (enemy->interpEasing)
                {
                case ANM_EASE_IN_QUAD: {
                    t1 = t1 * t1;
                    break;
                }
                case ANM_EASE_IN_CUBIC: {
                    t1 = t1 * t1 * t1;
                    break;
                }
                case ANM_EASE_IN_QUART: {
                    t1 = t1 * t1 * t1 * t1;
                    break;
                }
                case ANM_EASE_OUT_QUAD: {
                    t1 = 1.0f - t1;
                    t1 = t1 * t1;
                    t1 = 1.0f - t1;
                    break;
                }
                case ANM_EASE_OUT_CUBIC: {
                    t1 = 1.0f - t1;
                    t1 = t1 * t1 * t1;
                    t1 = 1.0f - t1;
                    break;
                }
                case ANM_EASE_OUT_QUART: {
                    t1 = 1.0f - t1;
                    t1 = t1 * t1 * t1 * t1;
                    t1 = 1.0f - t1;
                    break;
                }
                }
                enemy->axisSpeed = t1 * enemy->moveInterp + enemy->moveInterpStartPos - enemy->pos;
                if (enemy->mirror)
                {
                    enemy->axisSpeed.x = -enemy->axisSpeed.x;
                }
                enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
                if (enemy->moveInterpTimer <= 0)
                {
                    enemy->moveMode = ENEMY_MOVE_AXIS;
                    enemy->pos = enemy->moveInterpStartPos + enemy->moveInterp;
                    enemy->axisSpeed = ZunVec3(0.0f, 0.0f, 0.0f);
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
                                g_AnmManager->SetAnmIdxAndExecuteScript(&enemy->primaryVm,
                                                                        enemy->anmExDefaults +
                                                                            ANM_SCRIPT_ENEMY_ARRAY);
                            }
                            else if (enemy->anmExFlags == 1)
                            {
                                g_AnmManager->SetAnmIdxAndExecuteScript(&enemy->primaryVm,
                                                                        enemy->anmExFarLeft +
                                                                            ANM_SCRIPT_ENEMY_ARRAY);
                            }
                            else
                            {
                                g_AnmManager->SetAnmIdxAndExecuteScript(&enemy->primaryVm,
                                                                        enemy->anmExFarRight +
                                                                            ANM_SCRIPT_ENEMY_ARRAY);
                            }
                            break;
                        case 1:
                            g_AnmManager->SetAnmIdxAndExecuteScript(
                                &enemy->primaryVm, enemy->anmExLeft + ANM_SCRIPT_ENEMY_ARRAY);
                            break;
                        case 2:
                            g_AnmManager->SetAnmIdxAndExecuteScript(
                                &enemy->primaryVm, enemy->anmExRight + ANM_SCRIPT_ENEMY_ARRAY);
                            break;
                        }
                        enemy->anmExFlags = anmDirection;
                    }
                }
                if (enemy->currentContext.func)
                {
                    enemy->currentContext.func(enemy, enemy->currentContext.eclExInstr);
                }
                posModified = false;
                interp2 = enemy->currentContext.interps;
                ZunVec3 oldPos = enemy->pos;
                for (interpIdx2 = 0; interpIdx2 < ARRAY_SIZE_SIGNED(enemy->currentContext.interps);
                     interpIdx2++, interp2++)
                {
                    if (interp2->fn)
                    {
                        interp2->timer++;
                        if (interp2->timer >= interp2->args[0].i)
                        {
                            interp2->timer = interp2->args[0].i;
                        }
                        t2 = interp2->timer.AsFloat() / (f32)interp2->args[0].i;
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
                        if (interp2->args[7].f == 10018.0f || interp2->args[7].f == 10019.0f ||
                            interp2->args[7].f == 10020.0f)
                        {
                            posModified = true;
                        }
                    }
                }
                if (posModified)
                {
                    enemy->axisSpeed.x = enemy->pos.x - oldPos.x;
                    enemy->axisSpeed.y = enemy->pos.y - oldPos.y;
                    enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
                    enemy->pos = oldPos;
                }
            }
            enemy->currentContext.curInstr = instr;
            enemy->currentContext.time++;
            if (enemy->isBoss && enemy->bossId == 0 &&
                (g_EnemyManager.spellcardInfo.isActive && g_EnemyManager.spellcardInfo.isCapturing))
            {
                if (!enemy->isSurvivalSpellcard)
                {
                    g_EnemyManager.spellcardInfo.captureScore =
                        (i32)((f32)(i32)
                                  g_SpellcardScore[g_EnemyManager.spellcardInfo.spellcardIdx] -
                              g_EnemyManager.timer.AsFloat() *
                                  (f32)g_EnemyManager.spellcardInfo.scoreDrainRate / 60.0f);
                    g_EnemyManager.spellcardInfo.captureScore =
                        g_EnemyManager.spellcardInfo.captureScore -
                        g_EnemyManager.spellcardInfo.captureScore % 10;
                }
                g_EnemyManager.timer++;
            }
            if (enemy->isBoss && g_GameManager.currentStage >= EXTRASTAGE)
            {
                if (g_Player.bombInfo.isInUse && g_EnemyManager.spellcardInfo.isActive &&
                    g_EnemyManager.spellcardInfo.spellcardIdx >= SPELLCARD_EX_BOSS_1)
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
