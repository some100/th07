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

EclManager g_EclManager;
const char *g_EclPaths[10] = {"dummy",
                              "data/ecldata1.ecl",
                              "data/ecldata2.ecl",
                              "data/ecldata3.ecl",
                              "data/ecldata4.ecl",
                              "data/ecldata5.ecl",
                              "data/ecldata6.ecl",
                              "data/ecldata7.ecl",
                              "data/ecldata8.ecl",
                              NULL};
EclGlobalVars g_GlobalEclVars = {{0}};

ZunResult EclManager::Load(const char *path)

{
  this->eclFile = (EclRawHeader *)FileSystem::OpenFile(path, 0);
  if (this->eclFile == NULL) {
    g_GameErrorContext.Log("敵データの読み込みに失敗しました、データが壊れてる"
                           "か失われています\r\n");
    return ZUN_ERROR;
  } else {
    for (i32 i = 0; i < 0x10; i += 1) {
      this->eclFile->timelinePtr[i] =
          (EclTimelineInstr *)((u8 *)this->eclFile +
                               (i32)this->eclFile->timelinePtr[i]);
    }
    this->subTable = (EclRawInstr **)(this->eclFile + 1);
    for (i32 i = 0; i < this->eclFile->subCount; i += 1) {
      this->subTable[i] =
          (EclRawInstr *)((u8 *)this->eclFile + (i32)this->subTable[i]);
    }
    return ZUN_SUCCESS;
  }
}

void EclManager::Unload()

{
  if (this->eclFile != NULL) {
    free(this->eclFile);
  }
  this->eclFile = NULL;
}

ZunResult EclManager::CallEclSub(EnemyEclContext *param_1, i16 subId)

{
  param_1->curInstr = this->subTable[subId];
  (param_1->time).Initialize(0);
  (param_1->timer2).Initialize(0);
  param_1->subId = subId;
  return ZUN_SUCCESS;
}

i32 EclManager::GetVarValue(Enemy *enemy, i32 eclVar)

{
  u32 local_38;
  D3DXVECTOR3 local_10;

  switch (eclVar) {
  case VAR_LOCAL_INT1_1:
    eclVar = enemy->currentContext.eclContextArgs.intVars1[0];
    break;
  case VAR_LOCAL_INT1_2:
    eclVar = enemy->currentContext.eclContextArgs.intVars1[1];
    break;
  case VAR_LOCAL_INT1_3:
    eclVar = enemy->currentContext.eclContextArgs.intVars1[2];
    break;
  case VAR_LOCAL_INT1_4:
    eclVar = enemy->currentContext.eclContextArgs.intVars1[3];
    break;
  case VAR_LOCAL_FLOAT1_1:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[0];
    break;
  case VAR_LOCAL_FLOAT1_2:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[1];
    break;
  case VAR_LOCAL_FLOAT1_3:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[2];
    break;
  case VAR_LOCAL_FLOAT1_4:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[3];
    break;
  case VAR_LOCAL_FLOAT1_5:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[4];
    break;
  case VAR_LOCAL_FLOAT1_6:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[5];
    break;
  case VAR_LOCAL_FLOAT1_7:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[6];
    break;
  case VAR_LOCAL_FLOAT1_8:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[7];
    break;
  case VAR_LOCAL_INT2_1:
    eclVar = enemy->currentContext.eclContextArgs.intVars2[0];
    break;
  case VAR_LOCAL_INT2_2:
    eclVar = enemy->currentContext.eclContextArgs.intVars2[1];
    break;
  case VAR_LOCAL_INT2_3:
    eclVar = enemy->currentContext.eclContextArgs.intVars2[2];
    break;
  case VAR_LOCAL_INT2_4:
    eclVar = enemy->currentContext.eclContextArgs.intVars2[3];
    break;
  case VAR_DIFFICULTY:
    eclVar = g_GameManager.difficulty;
    break;
  case VAR_RANK:
    eclVar = g_GameManager.rank.rank;
    break;
  case VAR_POS_X:
    eclVar = enemy->position.x;
    break;
  case VAR_POS_Y:
    eclVar = enemy->position.y;
    break;
  case VAR_POS_Z:
    eclVar = enemy->position.z;
    break;
  case VAR_PLAYER_POS_X:
    eclVar = g_Player.positionCenter.x;
    break;
  case VAR_PLAYER_POS_Y:
    eclVar = g_Player.positionCenter.y;
    break;
  case VAR_PLAYER_POS_Z:
    eclVar = g_Player.positionCenter.z;
    break;
  case VAR_ANGLE_TO_PLAYER:
    eclVar = g_Player.AngleToPlayer(&enemy->position);
    break;
  case VAR_CUR_TIME:
    eclVar = enemy->timer.current;
    break;
  case VAR_DISTANCE_FROM_PLAYER:
    local_10.z = g_Player.positionCenter.z - enemy->position.z;
    local_10.y = g_Player.positionCenter.y - enemy->position.y;
    local_10.x = g_Player.positionCenter.x - enemy->position.x;
    eclVar = D3DXVec3Length(&local_10);
    break;
  case VAR_LIFE:
    eclVar = enemy->life;
    break;
  case VAR_PLAYER_SHOTTYPE:
    eclVar = g_GameManager.shotTypeAndCharacter;
    break;
  case VAR_LOCAL_INT3_1:
    eclVar = enemy->currentContext.eclContextArgs.globalVars.intVars[0];
    break;
  case VAR_LOCAL_INT3_2:
    eclVar = enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    break;
  case VAR_LOCAL_INT3_3:
    eclVar = enemy->currentContext.eclContextArgs.globalVars.intVars[2];
    break;
  case VAR_LOCAL_INT3_4:
    eclVar = enemy->currentContext.eclContextArgs.globalVars.intVars[3];
    break;
  case VAR_LOCAL_FLOAT3_1:
    eclVar = enemy->currentContext.eclContextArgs.globalVars.floatVars[0];
    break;
  case VAR_LOCAL_FLOAT3_2:
    eclVar = enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    break;
  case VAR_LOCAL_FLOAT3_3:
    eclVar = enemy->currentContext.eclContextArgs.globalVars.floatVars[2];
    break;
  case VAR_LOCAL_FLOAT3_4:
    eclVar = enemy->currentContext.eclContextArgs.globalVars.floatVars[3];
    break;
  case VAR_GLOBAL_INT_1:
    eclVar = g_GlobalEclVars.intVars[0];
    break;
  case VAR_GLOBAL_INT_2:
    eclVar = g_GlobalEclVars.intVars[1];
    break;
  case VAR_GLOBAL_INT_3:
    eclVar = g_GlobalEclVars.intVars[2];
    break;
  case VAR_GLOBAL_INT_4:
    eclVar = g_GlobalEclVars.intVars[3];
    break;
  case VAR_GLOBAL_FLOAT_1:
    eclVar = g_GlobalEclVars.floatVars[0];
    break;
  case VAR_GLOBAL_FLOAT_2:
    eclVar = g_GlobalEclVars.floatVars[1];
    break;
  case VAR_GLOBAL_FLOAT_3:
    eclVar = g_GlobalEclVars.floatVars[2];
    break;
  case VAR_GLOBAL_FLOAT_4:
    eclVar = g_GlobalEclVars.floatVars[3];
    break;
  case VAR_ANGLE:
    eclVar = enemy->angle;
    break;
  case VAR_ANGULAR_VELOCITY:
    eclVar = enemy->angularVelocity;
    break;
  case VAR_MOVE_SPEED:
    eclVar = enemy->moveSpeed;
    break;
  case VAR_MOVE_ACCELERATION:
    eclVar = enemy->moveAcceleration;
    break;
  case VAR_MOVE_RADIUS:
    eclVar = enemy->moveRadius;
    break;
  case VAR_MOVE_INTERP_ORIGIN_X:
    eclVar = enemy->moveInterpStartPos.x;
    break;
  case VAR_MOVE_INTERP_ORIGIN_Y:
    eclVar = enemy->moveInterpStartPos.y;
    break;
  case VAR_MOVE_INTERP_ORIGIN_Z:
    eclVar = enemy->moveInterpStartPos.z;
    break;
  case VAR_MOVE_ANGLE:
    eclVar = enemy->moveAngle;
    break;
  case VAR_MOVE_ANGULAR_VELOCITY:
    eclVar = enemy->moveAngularVelocity;
    break;
  case VAR_RNG_0_TO_1:
    eclVar = g_Rng.GetRandomU32();
    break;
  case VAR_RNG_CUSTOM_BOUND:
    if (enemy->currentContext.eclContextArgs.globalVars.intVars[0] == 0) {
      local_38 = 0;
    } else {
      local_38 = g_Rng.GetRandomU32() %
                 enemy->currentContext.eclContextArgs.globalVars.intVars[0];
    }
    eclVar =
        local_38 + enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    break;
  default:
    break;
  case VAR_LAST_DAMAGE:
    eclVar = enemy->lastDamage;
    break;
  case VAR_BOSS_ID:
    eclVar = enemy->bossId;
    break;
  case VAR_FINAL_POS_X:
    eclVar = enemy->finalPos.x;
    break;
  case VAR_FINAL_POS_Y:
    eclVar = enemy->finalPos.y;
    break;
  case VAR_FINAL_POS_Z:
    eclVar = enemy->finalPos.z;
    break;
  case VAR_BOSS_LIFE_THRESHOLD1:
    eclVar = enemy->lifeCallbackThreshold[0];
    break;
  case VAR_BOSS_LIFE_THRESHOLD2:
    eclVar = enemy->lifeCallbackThreshold[1];
    break;
  case VAR_BOSS_LIFE_THRESHOLD3:
    eclVar = enemy->lifeCallbackThreshold[2];
    break;
  case VAR_BOSS_LIFE_THRESHOLD4:
    eclVar = enemy->lifeCallbackThreshold[3];
    break;
  case VAR_ITEMDROP:
    eclVar = enemy->itemDrop;
    break;
  case VAR_SCORE:
    eclVar = enemy->score;
    break;
  case VAR_LOCAL_FLOAT2_1:
    eclVar = enemy->currentContext.eclContextArgs.floatVars2[0];
    break;
  case VAR_LOCAL_FLOAT2_2:
    eclVar = enemy->currentContext.eclContextArgs.floatVars2[1];
  }

  return eclVar;
}

i32 *EclManager::GetVar(Enemy *enemy, i32 *eclVar, u16 paramMask, i32 param_4)

{
  if ((-1 < param_4) && (((u32)paramMask & 1 << ((u8)param_4 & 0x1f)) == 0)) {
    return eclVar;
  }

  switch (*eclVar) {
  case VAR_LOCAL_INT1_1:
    eclVar = &enemy->currentContext.eclContextArgs.intVars1[0];
    break;
  case VAR_LOCAL_INT1_2:
    eclVar = &enemy->currentContext.eclContextArgs.intVars1[1];
    break;
  case VAR_LOCAL_INT1_3:
    eclVar = &enemy->currentContext.eclContextArgs.intVars1[2];
    break;
  case VAR_LOCAL_INT1_4:
    eclVar = &enemy->currentContext.eclContextArgs.intVars1[3];
    break;
  default:
    break;
  case VAR_LOCAL_INT2_1:
    eclVar = &enemy->currentContext.eclContextArgs.intVars2[0];
    break;
  case VAR_LOCAL_INT2_2:
    eclVar = &enemy->currentContext.eclContextArgs.intVars2[1];
    break;
  case VAR_LOCAL_INT2_3:
    eclVar = &enemy->currentContext.eclContextArgs.intVars2[2];
    break;
  case VAR_LOCAL_INT2_4:
    eclVar = &enemy->currentContext.eclContextArgs.intVars2[3];
    break;
  case VAR_DIFFICULTY:
    eclVar = &g_GameManager.difficulty;
    break;
  case VAR_RANK:
    eclVar = &g_GameManager.rank.rank;
    break;
  case VAR_CUR_TIME:
    eclVar = &enemy->timer.current;
    break;
  case VAR_LIFE:
    eclVar = &enemy->life;
    break;
  case VAR_LOCAL_INT3_1:
    eclVar = &enemy->currentContext.eclContextArgs.globalVars.intVars[0];
    break;
  case VAR_LOCAL_INT3_2:
    eclVar = &enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    break;
  case VAR_LOCAL_INT3_3:
    eclVar = &enemy->currentContext.eclContextArgs.globalVars.intVars[2];
    break;
  case VAR_LOCAL_INT3_4:
    eclVar = &enemy->currentContext.eclContextArgs.globalVars.intVars[3];
    break;
  case VAR_GLOBAL_INT_1:
    eclVar = &g_GlobalEclVars.intVars[0];
    break;
  case VAR_GLOBAL_INT_2:
    eclVar = &g_GlobalEclVars.intVars[1];
    break;
  case VAR_GLOBAL_INT_3:
    eclVar = &g_GlobalEclVars.intVars[2];
    break;
  case VAR_GLOBAL_INT_4:
    eclVar = &g_GlobalEclVars.intVars[3];
    break;
  case VAR_ITEMDROP:
    eclVar = &enemy->itemDrop;
    break;
  case VAR_SCORE:
    eclVar = &enemy->score;
  }

  return eclVar;
}

f32 EclManager::GetFloatVarValue(Enemy *enemy, f32 eclVar)

{
  D3DXVECTOR3 local_10;

  switch ((i32)eclVar) {
  case VAR_LOCAL_INT1_1:
    eclVar = (f32)enemy->currentContext.eclContextArgs.intVars1[0];
    break;
  case VAR_LOCAL_INT1_2:
    eclVar = (f32)enemy->currentContext.eclContextArgs.intVars1[1];
    break;
  case VAR_LOCAL_INT1_3:
    eclVar = (f32)enemy->currentContext.eclContextArgs.intVars1[2];
    break;
  case VAR_LOCAL_INT1_4:
    eclVar = (f32)enemy->currentContext.eclContextArgs.intVars1[3];
    break;
  case VAR_LOCAL_FLOAT1_1:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[0];
    break;
  case VAR_LOCAL_FLOAT1_2:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[1];
    break;
  case VAR_LOCAL_FLOAT1_3:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[2];
    break;
  case VAR_LOCAL_FLOAT1_4:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[3];
    break;
  case VAR_LOCAL_FLOAT1_5:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[4];
    break;
  case VAR_LOCAL_FLOAT1_6:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[5];
    break;
  case VAR_LOCAL_FLOAT1_7:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[6];
    break;
  case VAR_LOCAL_FLOAT1_8:
    eclVar = enemy->currentContext.eclContextArgs.floatVars1[7];
    break;
  case VAR_LOCAL_INT2_1:
    eclVar = (f32)enemy->currentContext.eclContextArgs.intVars2[0];
    break;
  case VAR_LOCAL_INT2_2:
    eclVar = (f32)enemy->currentContext.eclContextArgs.intVars2[1];
    break;
  case VAR_LOCAL_INT2_3:
    eclVar = (f32)enemy->currentContext.eclContextArgs.intVars2[2];
    break;
  case VAR_LOCAL_INT2_4:
    eclVar = (f32)enemy->currentContext.eclContextArgs.intVars2[3];
    break;
  case VAR_DIFFICULTY:
    eclVar = (f32)g_GameManager.difficulty;
    break;
  case VAR_RANK:
    eclVar = (f32)g_GameManager.rank.rank;
    break;
  case VAR_POS_X:
    eclVar = enemy->position.x;
    break;
  case VAR_POS_Y:
    eclVar = enemy->position.y;
    break;
  case VAR_POS_Z:
    eclVar = enemy->position.z;
    break;
  case VAR_PLAYER_POS_X:
    eclVar = g_Player.positionCenter.x;
    break;
  case VAR_PLAYER_POS_Y:
    eclVar = g_Player.positionCenter.y;
    break;
  case VAR_PLAYER_POS_Z:
    eclVar = g_Player.positionCenter.z;
    break;
  case VAR_ANGLE_TO_PLAYER:
    eclVar = g_Player.AngleToPlayer(&enemy->position);
    break;
  case VAR_CUR_TIME:
    eclVar = (f32)enemy->timer.current;
    break;
  case VAR_DISTANCE_FROM_PLAYER:
    local_10.z = g_Player.positionCenter.z - enemy->position.z;
    local_10.y = g_Player.positionCenter.y - enemy->position.y;
    local_10.x = g_Player.positionCenter.x - enemy->position.x;
    eclVar = D3DXVec3Length(&local_10);
    break;
  case VAR_LIFE:
    eclVar = (f32)enemy->life;
    break;
  case VAR_PLAYER_SHOTTYPE:
    eclVar = (f32)g_GameManager.shotTypeAndCharacter;
    break;
  case VAR_LOCAL_INT3_1:
    eclVar = (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[0];
    break;
  case VAR_LOCAL_INT3_2:
    eclVar = (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[1];
    break;
  case VAR_LOCAL_INT3_3:
    eclVar = (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[2];
    break;
  case VAR_LOCAL_INT3_4:
    eclVar = (f32)enemy->currentContext.eclContextArgs.globalVars.intVars[3];
    break;
  case VAR_LOCAL_FLOAT3_1:
    eclVar = enemy->currentContext.eclContextArgs.globalVars.floatVars[0];
    break;
  case VAR_LOCAL_FLOAT3_2:
    eclVar = enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    break;
  case VAR_LOCAL_FLOAT3_3:
    eclVar = enemy->currentContext.eclContextArgs.globalVars.floatVars[2];
    break;
  case VAR_LOCAL_FLOAT3_4:
    eclVar = enemy->currentContext.eclContextArgs.globalVars.floatVars[3];
    break;
  case VAR_GLOBAL_INT_1:
    eclVar = (f32)g_GlobalEclVars.intVars[0];
    break;
  case VAR_GLOBAL_INT_2:
    eclVar = (f32)g_GlobalEclVars.intVars[1];
    break;
  case VAR_GLOBAL_INT_3:
    eclVar = (f32)g_GlobalEclVars.intVars[2];
    break;
  case VAR_GLOBAL_INT_4:
    eclVar = (f32)g_GlobalEclVars.intVars[3];
    break;
  case VAR_GLOBAL_FLOAT_1:
    eclVar = g_GlobalEclVars.floatVars[0];
    break;
  case VAR_GLOBAL_FLOAT_2:
    eclVar = g_GlobalEclVars.floatVars[1];
    break;
  case VAR_GLOBAL_FLOAT_3:
    eclVar = g_GlobalEclVars.floatVars[2];
    break;
  case VAR_GLOBAL_FLOAT_4:
    eclVar = g_GlobalEclVars.floatVars[3];
    break;
  case VAR_ANGLE:
    eclVar = enemy->angle;
    break;
  case VAR_ANGULAR_VELOCITY:
    eclVar = enemy->angularVelocity;
    break;
  case VAR_MOVE_SPEED:
    eclVar = enemy->moveSpeed;
    break;
  case VAR_MOVE_ACCELERATION:
    eclVar = enemy->moveAcceleration;
    break;
  case VAR_MOVE_RADIUS:
    eclVar = enemy->moveRadius;
    break;
  case VAR_MOVE_INTERP_ORIGIN_X:
    eclVar = enemy->moveInterpStartPos.x;
    break;
  case VAR_MOVE_INTERP_ORIGIN_Y:
    eclVar = enemy->moveInterpStartPos.y;
    break;
  case VAR_MOVE_INTERP_ORIGIN_Z:
    eclVar = enemy->moveInterpStartPos.z;
    break;
  case VAR_MOVE_ANGLE:
    eclVar = enemy->moveAngle;
    break;
  case VAR_MOVE_ANGULAR_VELOCITY:
    eclVar = enemy->moveAngularVelocity;
    break;
  case VAR_RNG_0_TO_1:
    eclVar = g_Rng.GetRandomFloat();
    break;
  case VAR_RNG_CUSTOM_BOUND:
    eclVar = g_Rng.GetRandomFloat() *
                 enemy->currentContext.eclContextArgs.globalVars.floatVars[0] +
             enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    break;
  case VAR_MOVE_INTERP_TARGET_X:
    eclVar = enemy->moveInterp.x;
    break;
  case VAR_MOVE_INTERP_TARGET_Y:
    eclVar = enemy->moveInterp.y;
    break;
  case VAR_MOVE_INTERP_TARGET_Z:
    eclVar = enemy->moveInterp.z;
    break;
  case VAR_RNG_RADIAN:
    eclVar = g_Rng.GetRandomFloat() * ZUN_2PI - ZUN_PI;
    break;
  case VAR_LAST_DAMAGE:
    eclVar = (f32)enemy->lastDamage;
    break;
  case VAR_BOSS_ID:
    eclVar = (f32)enemy->bossId;
    break;
  case VAR_FINAL_POS_X:
    eclVar = enemy->finalPos.x;
    break;
  case VAR_FINAL_POS_Y:
    eclVar = enemy->finalPos.y;
    break;
  case VAR_FINAL_POS_Z:
    eclVar = enemy->finalPos.z;
    break;
  case VAR_BOSS_LIFE_THRESHOLD1:
    eclVar = (f32)enemy->lifeCallbackThreshold[0];
    break;
  case VAR_BOSS_LIFE_THRESHOLD2:
    eclVar = (f32)enemy->lifeCallbackThreshold[1];
    break;
  case VAR_BOSS_LIFE_THRESHOLD3:
    eclVar = (f32)enemy->lifeCallbackThreshold[2];
    break;
  case VAR_BOSS_LIFE_THRESHOLD4:
    eclVar = (f32)enemy->lifeCallbackThreshold[3];
    break;
  case VAR_ITEMDROP:
    eclVar = (f32)enemy->itemDrop;
    break;
  case VAR_SCORE:
    eclVar = (f32)enemy->score;
    break;
  case VAR_LOCAL_FLOAT2_1:
    eclVar = enemy->currentContext.eclContextArgs.floatVars2[0];
    break;
  case VAR_LOCAL_FLOAT2_2:
    eclVar = enemy->currentContext.eclContextArgs.floatVars2[1];
  }
  return eclVar;
}

f32 *EclManager::GetFloatVar(Enemy *enemy, f32 *eclVar, u16 paramMask,
                             i32 param_4)

{
  if ((-1 < param_4) && (((u32)paramMask & 1 << ((u8)param_4 & 0x1f)) == 0)) {
    return eclVar;
  }

  switch ((i32)*eclVar) {
  case VAR_LOCAL_FLOAT1_1:
    eclVar = &enemy->currentContext.eclContextArgs.floatVars1[0];
    break;
  case VAR_LOCAL_FLOAT1_2:
    eclVar = &enemy->currentContext.eclContextArgs.floatVars1[1];
    break;
  case VAR_LOCAL_FLOAT1_3:
    eclVar = &enemy->currentContext.eclContextArgs.floatVars1[2];
    break;
  case VAR_LOCAL_FLOAT1_4:
    eclVar = &enemy->currentContext.eclContextArgs.floatVars1[3];
    break;
  case VAR_LOCAL_FLOAT1_5:
    eclVar = &enemy->currentContext.eclContextArgs.floatVars1[4];
    break;
  case VAR_LOCAL_FLOAT1_6:
    eclVar = &enemy->currentContext.eclContextArgs.floatVars1[5];
    break;
  case VAR_LOCAL_FLOAT1_7:
    eclVar = &enemy->currentContext.eclContextArgs.floatVars1[6];
    break;
  case VAR_LOCAL_FLOAT1_8:
    eclVar = &enemy->currentContext.eclContextArgs.floatVars1[7];
    break;
  default:
    break;
  case VAR_POS_X:
    eclVar = &enemy->position.x;
    break;
  case VAR_POS_Y:
    eclVar = &enemy->position.y;
    break;
  case VAR_POS_Z:
    eclVar = &enemy->position.z;
    break;
  case VAR_PLAYER_POS_X:
    eclVar = &g_Player.positionCenter.x;
    break;
  case VAR_PLAYER_POS_Y:
    eclVar = &g_Player.positionCenter.y;
    break;
  case VAR_PLAYER_POS_Z:
    eclVar = &g_Player.positionCenter.z;
    break;
  case VAR_LOCAL_FLOAT3_1:
    eclVar = &enemy->currentContext.eclContextArgs.globalVars.floatVars[0];
    break;
  case VAR_LOCAL_FLOAT3_2:
    eclVar = &enemy->currentContext.eclContextArgs.globalVars.floatVars[1];
    break;
  case VAR_LOCAL_FLOAT3_3:
    eclVar = &enemy->currentContext.eclContextArgs.globalVars.floatVars[2];
    break;
  case VAR_LOCAL_FLOAT3_4:
    eclVar = &enemy->currentContext.eclContextArgs.globalVars.floatVars[3];
    break;
  case VAR_GLOBAL_FLOAT_1:
    eclVar = &g_GlobalEclVars.floatVars[0];
    break;
  case VAR_GLOBAL_FLOAT_2:
    eclVar = &g_GlobalEclVars.floatVars[1];
    break;
  case VAR_GLOBAL_FLOAT_3:
    eclVar = &g_GlobalEclVars.floatVars[2];
    break;
  case VAR_GLOBAL_FLOAT_4:
    eclVar = &g_GlobalEclVars.floatVars[3];
    break;
  case VAR_ANGLE:
    eclVar = &enemy->angle;
    break;
  case VAR_ANGULAR_VELOCITY:
    eclVar = &enemy->angularVelocity;
    break;
  case VAR_MOVE_SPEED:
    eclVar = &enemy->moveSpeed;
    break;
  case VAR_MOVE_ACCELERATION:
    eclVar = &enemy->moveAcceleration;
    break;
  case VAR_MOVE_RADIUS:
    eclVar = &enemy->moveRadius;
    break;
  case VAR_MOVE_INTERP_ORIGIN_X:
    eclVar = &enemy->moveInterpStartPos.x;
    break;
  case VAR_MOVE_INTERP_ORIGIN_Y:
    eclVar = &enemy->moveInterpStartPos.y;
    break;
  case VAR_MOVE_INTERP_ORIGIN_Z:
    eclVar = &enemy->moveInterpStartPos.z;
    break;
  case VAR_MOVE_ANGLE:
    eclVar = &enemy->moveAngle;
    break;
  case VAR_MOVE_ANGULAR_VELOCITY:
    eclVar = &enemy->moveAngularVelocity;
    break;
  case VAR_MOVE_INTERP_TARGET_X:
    eclVar = &enemy->moveInterp.x;
    break;
  case VAR_MOVE_INTERP_TARGET_Y:
    eclVar = &enemy->moveInterp.y;
    break;
  case VAR_MOVE_INTERP_TARGET_Z:
    eclVar = &enemy->moveInterp.z;
    break;
  case VAR_LOCAL_FLOAT2_1:
    eclVar = &enemy->currentContext.eclContextArgs.floatVars2[0];
    break;
  case VAR_LOCAL_FLOAT2_2:
    eclVar = &enemy->currentContext.eclContextArgs.floatVars2[1];
  }

  return eclVar;
}

void EclManager::MoveDirTime(Enemy *enemy, EclRawInstr *instr)

{
  f32 fVar2;
  u8 local_38;
  i32 local_30;
  f32 local_2c;
  i32 local_24;
  f32 local_20;
  f32 local_1c;
  i32 local_c;

  if ((instr->paramMask & 4) == 0) {
    local_1c = instr->args[2].f;
  } else {
    local_1c = GetFloatVarValue(enemy, instr->args[2].f);
  }
  fVar2 = utils::AddNormalizeAngle(local_1c, 0.0f);
  if ((instr->paramMask & 8) == 0) {
    local_20 = instr->args[3].f;
  } else {
    local_20 = GetFloatVarValue(enemy, instr->args[3].f);
  }
  if ((instr->paramMask & 1) == 0) {
    local_24 = instr->args[0].i;
  } else {
    local_24 = GetVarValue(enemy, instr->args[0].i);
  }
  enemy->moveInterp.x = cosf(fVar2) * local_20 * (f32)local_24;
  if ((instr->paramMask & 8) == 0) {
    local_2c = instr->args[3].f;
  } else {
    local_2c = GetFloatVarValue(enemy, instr->args[3].f);
  }
  if ((instr->paramMask & 1) == 0) {
    local_30 = instr->args[0].i;
  } else {
    local_30 = GetVarValue(enemy, instr->args[0].i);
  }
  enemy->moveInterp.y = sinf(fVar2) * local_2c * (f32)local_30;
  enemy->moveInterp.z = 0.0f;
  enemy->moveInterpStartPos = enemy->position;
  if ((instr->paramMask & 1) == 0) {
    local_c = instr->args[0].i;
  } else {
    local_c = GetVarValue(enemy, instr->args[0].i);
  }
  enemy->moveInterpStartTime = local_c;
  (enemy->moveInterpTimer).Initialize(local_c);
  if ((instr->paramMask & 2) == 0) {
    local_38 = (u8)instr->args[1].i;
  } else {
    local_38 = (u8)GetVarValue(enemy, instr->args[1].i);
  }
  enemy->flags1 = (enemy->flags1 & 0xe3) | (local_38 & 7) << 2;
  enemy->flags1 = (enemy->flags1 & 0xfc) | 2;
  if ((enemy->flags1 >> 6 & 1) != 0) {
    enemy->moveInterp.x = -enemy->moveInterp.x;
  }
}

void EclManager::MovePosTime(Enemy *enemy, EclRawInstr *instr)

{
  u8 local_58;
  f32 local_54;
  f32 local_50;
  f32 local_4c;
  i32 local_3c;

  if ((instr->paramMask & 4) == 0) {
    local_4c = instr->args[2].f;
  } else {
    local_4c = GetFloatVarValue(enemy, instr->args[2].f);
  }
  if ((instr->paramMask & 8) == 0) {
    local_50 = instr->args[3].f;
  } else {
    local_50 = GetFloatVarValue(enemy, instr->args[3].f);
  }
  if ((instr->paramMask & 0x10) == 0) {
    local_54 = instr->args[4].f;
  } else {
    local_54 = GetFloatVarValue(enemy, instr->args[4].f);
  }
  enemy->moveInterp.x = local_4c - enemy->position.x;
  enemy->moveInterp.y = local_50 - enemy->position.y;
  enemy->moveInterp.z = local_54 - enemy->position.z;
  enemy->moveInterpStartPos = enemy->position;
  if ((instr->paramMask & 1) == 0) {
    local_3c = instr->args[0].i;
  } else {
    local_3c = GetVarValue(enemy, instr->args[0].i);
  }
  enemy->moveInterpStartTime = local_3c;
  (enemy->moveInterpTimer).Initialize(local_3c);
  if ((instr->paramMask & 2) == 0) {
    local_58 = (u8)instr->args[1].i;
  } else {
    local_58 = (u8)GetVarValue(enemy, instr->args[1].i);
  }
  enemy->flags1 = (enemy->flags1 & 0xe3) | (local_58 & 7) << 2;
  enemy->flags1 = (enemy->flags1 & 0xfc) | 2;
  enemy->axisSpeed.x = 0.0f;
  enemy->axisSpeed.y = 0.0f;
  enemy->axisSpeed.z = 0.0f;
  if ((enemy->flags1 >> 6 & 1) != 0) {
    enemy->moveInterp.x = -enemy->moveInterp.x;
  }
}

void EclManager::MathLerp(Enemy *enemy, EclInterp *interp, f32 t)

{
  *GetFloatVar(enemy, &interp->args[7].f, 0, -1) =
      (GetFloatVarValue(enemy, interp->args[4].f) -
       GetFloatVarValue(enemy, interp->args[3].f)) *
          t +
      GetFloatVarValue(enemy, interp->args[3].f);
}

void EclManager::MathCubicInterp(Enemy *enemy, EclInterp *interp, f32 t)

{
  *GetFloatVar(enemy, &interp->args[7].f, 0, -1) =
      (t - 1.0f) * t * t * GetFloatVarValue(enemy, interp->args[6].f) +
      (1.0f - t) * (1.0f - t) * t * GetFloatVarValue(enemy, interp->args[5].f) +
      (3.0f - t * 2.0f) * t * t * GetFloatVarValue(enemy, interp->args[4].f) +
      (t * 2.0f + 1.0f) * (t - 1.0f) * (t - 1.0f) *
          GetFloatVarValue(enemy, interp->args[3].f);
}

void EclManager::BeginSpellcard(Enemy *enemy, EclRawInstr *instr)

{
  u32 uVar3;
  i32 iVar4;
  i32 iVar6;
  i16 local_4c;
  i32 local_48;
  i32 local_44;
  i32 i;
  char spellcardName[48];
  u32 local_8;

  memcpy(spellcardName, &instr->args[1], sizeof(spellcardName));
  for (local_8 = 0; local_8 < 0x30; local_8 += 1) {
    spellcardName[local_8] = spellcardName[local_8] ^ 0xaa;
  }
  g_Gui.ShowSpellcard(instr->args[0].s[0], spellcardName);
  g_BulletManager.RemoveAllBullets(1);
  g_Stage.spellCardState = 1;
  g_Stage.ticksSinceSpellcardStarted = 0;
  for (local_8 = 0;
       iVar4 = g_Stage.spellcardVmsIdx, (i32)local_8 < g_Stage.numSpellcardVms;
       local_8 += 1) {
    local_4c = (i16)local_8 + 0x2dc + (i16)g_Stage.spellcardVmsIdx;
    g_Stage.spellcardVms[local_8].anmFileIdx = local_4c;
    g_AnmManager->SetAndExecuteScript(
        g_Stage.spellcardVms + local_8,
        g_AnmManager->scripts[iVar4 + local_8 + 0x2dc]);
  }
  g_EnemyManager.spellcardInfo.isActive = 1;
  g_EnemyManager.spellcardInfo.isCapturing = 1;
  g_EnemyManager.spellcardInfo.spellcardIdx =
      (u32) * (u16 *)((i32)&instr->args[0] + 2);
  g_EnemyManager.spellcardInfo.captureScore =
      g_SpellcardScore[g_EnemyManager.spellcardInfo.spellcardIdx];
  g_EnemyManager.spellcardInfo.grazeBonusScore = 0;
  g_EnemyManager.spellcardInfo.scoreDrainRate =
      (i32)g_EnemyManager.spellcardInfo.captureScore /
      (enemy->timerCallbackThreshold / 0x3c + 10);
  g_EnemyManager.timer.Initialize(0);
  enemy->bulletRankSpeedLow = -0.5f;
  enemy->bulletRankSpeedHigh = 0.5f;
  enemy->bulletRankAmount1Low = 0;
  enemy->bulletRankAmount1High = 0;
  enemy->bulletRankAmount2Low = 0;
  enemy->bulletRankAmount2High = 0;
  enemy->specialEffect =
      g_EffectManager.SpawnEffect(0x19, &enemy->position, 1, 1, 0xffffffff);
  enemy->specialEffect->vm.interpStartTimes[4].Initialize(0);
  enemy->specialEffect->vm.interpEndTimes[4].Initialize(
      enemy->timerCallbackThreshold);
  enemy->specialEffect->vm.interpModes[4] = 0;
  enemy->specialEffect->vm.scaleInterpInitial = enemy->specialEffect->vm.scale;
  enemy->specialEffect->vm.scaleInterpFinal.x = 0.125;
  enemy->specialEffect->vm.scaleInterpFinal.y = 0.125;
  enemy->specialEffect->pos1 = enemy->position;
  enemy->flags4 = enemy->flags4 & 0xfd;
  uVar3 = g_EnemyManager.spellcardInfo.spellcardIdx;
  if ((g_GameManager.flags >> 3 & 1) == 0) {
    iVar6 = g_EnemyManager.spellcardInfo.spellcardIdx * 0x78;
    local_44 = 0;
    strcpy(g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx].name,
           spellcardName);
    i = (i32)strlen(g_GameManager.catk[uVar3].name);
    while (0 < i) {
      i += -1;
      local_44 += g_GameManager.catk[uVar3].name[i];
    }
    local_48 = local_44;
    for (i = 0; i < 7; i += 1) {
      local_44 = local_44 +
                 (u32)g_GameManager.catk[iVar6].numSuccessesPerShot[i] +
                 (u32)g_GameManager.catk[iVar6].numAttemptsPerShot[i] +
                 g_GameManager.catk[iVar6].highScorePerShot[i];
    }
    if (g_GameManager.catk[uVar3].nameCsum != (char)local_44) {
      for (i = 0; i < 7; i += 1) {
        g_GameManager.catk[iVar6].numSuccessesPerShot[i] = 0;
        g_GameManager.catk[iVar6].numAttemptsPerShot[i] = 0;
        g_GameManager.catk[iVar6].highScorePerShot[i] = 0;
      }
    }
    if (g_GameManager.catk[iVar6]
            .numAttemptsPerShot[g_GameManager.shotTypeAndCharacter] < 9999) {
      g_GameManager.catk[iVar6]
          .numAttemptsPerShot[g_GameManager.shotTypeAndCharacter] += 1;
    }
    if (g_GameManager.catk[uVar3].numAttemptsPerShot[6] < 9999) {
      g_GameManager.catk[uVar3].numAttemptsPerShot[6] =
          g_GameManager.catk[uVar3].numAttemptsPerShot[6] + 1;
    }
    for (i = 0; i < 7; i += 1) {
      local_48 = local_48 +
                 (u32)g_GameManager.catk[iVar6].numSuccessesPerShot[i] +
                 (u32)g_GameManager.catk[iVar6].numAttemptsPerShot[i] +
                 g_GameManager.catk[iVar6].highScorePerShot[i];
    }
    g_GameManager.catk[uVar3].nameCsum = (char)local_48;
  }
}

void EclManager::EndSpellcard()

{
  i32 iVar3;
  u32 fmtArg;
  i32 i;
  i32 local_1c;
  i32 local_18;
  i32 local_10;

  if (g_EnemyManager.spellcardInfo.isActive != 0) {
    g_Gui.EndEnemySpellcard();
    if (g_EnemyManager.spellcardInfo.isActive == 1) {
      iVar3 = g_EnemyManager.RemoveAllEnemies(
          8000, g_BulletManager.DespawnBullets(8000, 1));
      if (iVar3 != 0) {
        g_GameManager.globals->score =
            iVar3 / 10 + g_GameManager.globals->score;
        g_Gui.ShowBonusScore(iVar3);
      }
      if (g_EnemyManager.spellcardInfo.isCapturing != 0) {
        fmtArg = g_EnemyManager.spellcardInfo.captureScore +
                 g_EnemyManager.spellcardInfo.grazeBonusScore;
        g_Gui.ShowSpellcardBonus(fmtArg);
        g_GameManager.globals->score =
            (i32)fmtArg / 10 + g_GameManager.globals->score;
        if ((g_GameManager.flags >> 3 & 1) == 0) {
          local_18 = 0;
          local_10 = strlen(
              g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                  .name);
          while (0 < local_10) {
            local_10 += -1;
            local_18 +=
                g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                    .name[local_10];
          }
          local_1c = local_18;
          for (local_10 = 0; local_10 < 7; local_10 += 1) {
            local_18 =
                local_18 +
                (u32)g_GameManager
                    .catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                    .numSuccessesPerShot[local_10] +
                (u32)g_GameManager
                    .catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                    .numAttemptsPerShot[local_10] +
                g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                    .highScorePerShot[local_10];
          }
          if (g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                  .nameCsum != (char)local_18) {
            for (local_10 = 0; local_10 < 7; local_10 += 1) {
              g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                  .numSuccessesPerShot[local_10] = 0;
              g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                  .numAttemptsPerShot[local_10] = 0;
              g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                  .highScorePerShot[local_10] = 0;
            }
          }
          if (g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                  .highScorePerShot[local_10] < fmtArg) {
            g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                .highScorePerShot[local_10] = fmtArg;
          }
          if (g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                  .highScorePerShot[6] < fmtArg) {
            g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                .highScorePerShot[6] = fmtArg;
          }
          if (g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                  .numSuccessesPerShot[local_10] < 9999) {
            g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                .numSuccessesPerShot[local_10] += 1;
          }
          if (g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                  .numSuccessesPerShot[6] < 9999) {
            g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                .numSuccessesPerShot[6] =
                g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                    .numSuccessesPerShot[6] +
                1;
          }
          for (local_10 = 0; local_10 < 7; local_10 += 1) {
            local_1c =
                local_1c +
                (u32)g_GameManager
                    .catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                    .numSuccessesPerShot[local_10] +
                (u32)g_GameManager
                    .catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                    .numAttemptsPerShot[local_10] +
                g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
                    .highScorePerShot[local_10];
          }
          g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx]
              .nameCsum = (char)local_1c;
        }
        g_GameManager.globals->spellCardsCaptured =
            g_GameManager.globals->spellCardsCaptured + 1;
      }
    }
    g_EnemyManager.spellcardInfo.isActive = 0;
    for (i = 0; i < 8; i += 1) {
      if ((g_EnemyManager.bosses[i] != NULL) &&
          (g_EnemyManager.bosses[i]->specialEffect != NULL)) {
        g_EnemyManager.bosses[i]->specialEffect->inUseFlag = 0;
        g_EnemyManager.bosses[i]->specialEffect = NULL;
      }
    }
    g_SoundPlayer.PlaySoundByIdx(SOUND_ENEMY_SPELLCARD_END);
  }
  g_Stage.spellCardState = 0;
}

ZunResult EclManager::RunEcl(Enemy *enemy)

{
  i32 iVar13;
  u32 uVar17;
  i32 iVar19;
  f32 fVar8;

  EclInterp *local_fc;
  f32 local_f4;
  i32 local_f0;
  f32 local_e8;
  D3DXVECTOR3 local_d8;
  D3DXVECTOR3 local_cc;
  AnyArg local_c0[7];
  D3DXVECTOR3 local_a0;
  AnyArg local_94[7];
  D3DXVECTOR3 local_74;
  i32 local_68;
  i32 local_64;
  D3DXVECTOR3 local_60;
  i32 local_54;
  i32 local_50;
  D3DXVECTOR3 local_4c;
  i32 local_40;
  i32 local_3c;
  AnyArg *local_34;
  i32 local_30;
  EnemyLaserShooter *local_28;
  BulletCommand *local_24;
  EnemyBulletShooter *local_20;
  AnyArg *local_1c;
  EclInterp *local_18;
  i32 local_14;
  f32 local_10;
  EclRawInstr *instr;
  i32 local_8;
  bool bVar7;
  f32 fVar28;
  f32 fVar3;
  u8 uVar6;

LAB_00410531:
  while (instr = enemy->currentContext.curInstr, -1 < enemy->runInterrupt) {
  LAB_00414b54:
    enemy->currentContext.curInstr = (EclRawInstr *)((u8 *)instr + instr->size);
    if ((enemy->flags3 >> 5 & 1) == 0) {
      enemy->savedContextStack[enemy->stackDepth] = enemy->currentContext;
    }
    g_EclManager.CallEclSub(&enemy->currentContext,
                            enemy->interrupts[enemy->runInterrupt]);
    if (enemy->stackDepth < 0xf) {
      enemy->stackDepth = enemy->stackDepth + 1;
    }
    enemy->runInterrupt = -1;
  }
  if (-1 < enemy->periodicCallbackSub) {
    enemy->periodicCounter.previous = enemy->periodicCounter.current;
    g_Supervisor.TickTimer(&enemy->periodicCounter.current,
                           &enemy->periodicCounter.subFrame);
    if ((enemy->periodicTimer).current <= enemy->periodicCounter.current) {
      enemy->periodicCounter.Initialize(0);
      enemy->savedContextStack[enemy->stackDepth] = enemy->currentContext;
      enemy->currentContext.eclContextArgs = enemy->savedEclContextArgs;
      g_EclManager.CallEclSub(&enemy->currentContext,
                              (i16)enemy->periodicCallbackSub);
      if (enemy->stackDepth < 0xf) {
        enemy->stackDepth = enemy->stackDepth + 1;
      }
      instr = enemy->currentContext.curInstr;
      enemy->currentContext.isPeriodicSub = 1;
    }
  }
LAB_0041069a:
  if (0 < enemy->currentContext.timer2.current) {
    enemy->currentContext.timer2.Decrement(1);
    enemy->currentContext.time.Decrement(1);
  LAB_0041678f:
    if ((enemy->flags1 & 3) == 1) {
      enemy->angle = utils::AddNormalizeAngle(
          enemy->angle,
          g_Supervisor.effectiveFramerateMultiplier * enemy->angularVelocity);
      enemy->moveSpeed =
          g_Supervisor.effectiveFramerateMultiplier * enemy->moveAcceleration +
          enemy->moveSpeed;
      AngleToVector(&enemy->axisSpeed, enemy->angle, enemy->moveSpeed);
      enemy->axisSpeed.z = 0.0f;
      if ((0 < enemy->moveInterpStartTime) &&
          (enemy->moveInterpTimer.Decrement(1),
           enemy->moveInterpTimer.current < 1)) {
        enemy->flags1 = enemy->flags1 & 0xfc;
      }
    } else if ((enemy->flags1 & 3) == 2) {
      enemy->moveInterpTimer.Decrement(1);
      local_e8 = 1.0f - ((f32)enemy->moveInterpTimer.current +
                         enemy->moveInterpTimer.subFrame) /
                            (f32)enemy->moveInterpStartTime;
      if (local_e8 < 0.0f) {
        local_e8 = 0.0f;
      }
      switch (enemy->flags1 >> 2 & 7) {
      case 1: {
        local_e8 = local_e8 * local_e8;
        break;
      }
      case 2: {
        local_e8 = local_e8 * local_e8 * local_e8;
        break;
      }
      case 3: {
        local_e8 = local_e8 * local_e8 * local_e8 * local_e8;
        break;
      }
      case 4: {
        local_e8 = 1.0f - (1.0f - local_e8) * (1.0f - local_e8);
        break;
      }
      case 5: {
        local_e8 = 1.0f - local_e8;
        local_e8 = 1.0f - local_e8 * local_e8 * local_e8;
        break;
      }
      case 6: {
        local_e8 = 1.0f - local_e8;
        local_e8 = 1.0f - local_e8 * local_e8 * local_e8 * local_e8;
      }
      }
      enemy->axisSpeed =
          (local_e8 * enemy->moveInterp + enemy->moveInterpStartPos) -
          enemy->position;
      if ((enemy->flags1 >> 6 & 1) != 0) {
        enemy->axisSpeed.x = -enemy->axisSpeed.x;
      }
      enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
      if (enemy->moveInterpTimer.current < 1) {
        enemy->flags1 = enemy->flags1 & 0xfc;
        enemy->position = enemy->moveInterpStartPos + enemy->moveInterp;
        enemy->axisSpeed = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
      }
    } else if ((enemy->flags1 & 3) == 3) {
      enemy->moveAngle = utils::AddNormalizeAngle(
          enemy->moveAngle, g_Supervisor.effectiveFramerateMultiplier *
                                enemy->moveAngularVelocity);
      enemy->moveRadius = g_Supervisor.effectiveFramerateMultiplier *
                              enemy->moveRadialVelocity +
                          enemy->moveRadius;
      AngleToVector(&local_d8, enemy->moveAngle, enemy->moveRadius);
      enemy->axisSpeed.x =
          (local_d8.x + enemy->moveInterpStartPos.x) - enemy->position.x;
      enemy->axisSpeed.y =
          (local_d8.y + enemy->moveInterpStartPos.y) - enemy->position.y;
      enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
      if ((0 < enemy->moveInterpStartTime) &&
          (enemy->moveInterpTimer.Decrement(1),
           enemy->moveInterpTimer.current < 1)) {
        enemy->flags1 &= 0xfc;
      }
    }
    if (0 < enemy->life) {
      if (0 < enemy->shootInterval) {
        enemy->shootIntervalTimer.previous = enemy->shootIntervalTimer.current;
        g_Supervisor.TickTimer(&enemy->shootIntervalTimer.current,
                               &enemy->shootIntervalTimer.subFrame);
        if (enemy->shootInterval <= enemy->shootIntervalTimer.current) {
          enemy->bulletProps.position = enemy->position + enemy->shootOffset;
          g_BulletManager.SpawnBulletPattern(&enemy->bulletProps);
          enemy->shootIntervalTimer.Initialize(0);
        }
      }
      if (-1 < enemy->anmExLeft) {
        uVar6 = 0;
        if ((enemy->flags1 >> 6 & 1) == 0) {
          if (-0.01 <= enemy->axisSpeed.x) {
            if (0.01 < enemy->axisSpeed.x) {
              uVar6 = 2;
            }
          } else {
            uVar6 = 1;
          }
        } else if (-0.01 <= enemy->axisSpeed.x) {
          if (0.01 < enemy->axisSpeed.x) {
            uVar6 = 1;
          }
        } else {
          uVar6 = 2;
        }
        if (enemy->anmExFlags != uVar6) {
          if (uVar6 == 0) {
            if (enemy->anmExFlags == 0xff) {
              i16 local_324 = enemy->anmExDefaults + 0x900;
              enemy->primaryVm.anmFileIdx = local_324;
              g_AnmManager->SetAndExecuteScript(
                  &enemy->primaryVm,
                  g_AnmManager->scripts[enemy->anmExDefaults + 0x900]);
            } else if (enemy->anmExFlags == 1) {
              i16 local_32c = enemy->anmExFarLeft + 0x900;
              enemy->primaryVm.anmFileIdx = local_32c;
              g_AnmManager->SetAndExecuteScript(
                  &enemy->primaryVm,
                  g_AnmManager->scripts[enemy->anmExFarLeft + 0x900]);
            } else {
              i16 local_334 = enemy->anmExFarRight + 0x900;
              enemy->primaryVm.anmFileIdx = local_334;
              g_AnmManager->SetAndExecuteScript(
                  &enemy->primaryVm,
                  g_AnmManager->scripts[enemy->anmExFarRight + 0x900]);
            }
          } else if (uVar6 == 1) {
            i16 local_33c = enemy->anmExLeft + 0x900;
            enemy->primaryVm.anmFileIdx = local_33c;
            g_AnmManager->SetAndExecuteScript(
                &enemy->primaryVm,
                g_AnmManager->scripts[enemy->anmExLeft + 0x900]);
          } else if (uVar6 == 2) {
            i16 local_344 = enemy->anmExRight + 0x900;
            enemy->primaryVm.anmFileIdx = local_344;
            g_AnmManager->SetAndExecuteScript(
                &enemy->primaryVm,
                g_AnmManager->scripts[enemy->anmExRight + 0x900]);
          }
          enemy->anmExFlags = uVar6;
        }
      }
      if (enemy->currentContext.func != NULL) {
        enemy->currentContext.func(enemy, enemy->currentContext.eclExInstr);
      }
      bVar7 = false;
      local_fc = enemy->currentContext.interps;
      fVar8 = enemy->position.x;
      fVar28 = enemy->position.y;
      fVar3 = enemy->position.z;
      for (local_f0 = 0; local_f0 < 8; local_f0 += 1) {
        if (local_fc->fn != NULL) {
          local_fc->timer.previous = local_fc->timer.current;
          g_Supervisor.TickTimer(&local_fc->timer.current,
                                 &local_fc->timer.subFrame);
          if (local_fc->args[0].i <= local_fc->timer.current) {
            local_fc->timer.Initialize(local_fc->args[0].i);
          }
          local_f4 = ((f32)local_fc->timer.current + local_fc->timer.subFrame) /
                     (f32)local_fc->args[0].i;
          switch (local_fc->args[2].i) {
          case 1: {
            local_f4 = local_f4 * local_f4;
            break;
          }
          case 2: {
            local_f4 = local_f4 * local_f4 * local_f4;
            break;
          }
          case 3: {
            local_f4 = local_f4 * local_f4 * local_f4 * local_f4;
            break;
          }
          case 4: {
            local_f4 = 1.0f - (1.0f - local_f4) * (1.0f - local_f4);
            break;
          }
          case 5: {
            local_f4 = 1.0f - local_f4;
            local_f4 = 1.0f - local_f4 * local_f4 * local_f4;
            break;
          }
          case 6: {
            local_f4 = 1.0f - local_f4;
            local_f4 = 1.0f - local_f4 * local_f4 * local_f4 * local_f4;
          }
          }
          local_fc->fn(enemy, local_fc, local_f4);
          if (local_fc->args[0].i <= local_fc->timer.current) {
            local_fc->fn = NULL;
          }
          if (((local_fc->args[7].f == 10018.0f) ||
               (local_fc->args[7].f == 10019.0f)) ||
              (local_fc->args[7].f == 10020.0f)) {
            bVar7 = true;
          }
        }
        local_fc = local_fc + 1;
      }
      if (bVar7) {
        enemy->axisSpeed.x = enemy->position.x - fVar8;
        enemy->axisSpeed.y = enemy->position.y - fVar28;
        enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
        enemy->position.x = fVar8;
        enemy->position.y = fVar28;
        enemy->position.z = fVar3;
      }
    }
    enemy->currentContext.curInstr = instr;
    enemy->currentContext.time.previous = enemy->currentContext.time.current;
    g_Supervisor.TickTimer(&enemy->currentContext.time.current,
                           &enemy->currentContext.time.subFrame);
    if ((((enemy->flags2 >> 6 & 1) != 0) && (enemy->bossId == 0)) &&
        ((g_EnemyManager.spellcardInfo.isActive != 0 &&
          (g_EnemyManager.spellcardInfo.isCapturing != 0)))) {
      if ((enemy->flags3 >> 6 & 1) == 0) {
        uVar17 =
            (f32)(i32)
                g_SpellcardScore[g_EnemyManager.spellcardInfo.spellcardIdx] -
            (((f32)g_EnemyManager.timer.current +
              g_EnemyManager.timer.subFrame) *
             (f32)g_EnemyManager.spellcardInfo.scoreDrainRate) /
                60.0;
        g_EnemyManager.spellcardInfo.captureScore = uVar17 - (i32)uVar17 % 10;
      }
      g_EnemyManager.timer.previous = g_EnemyManager.timer.current;
      g_Supervisor.TickTimer(&g_EnemyManager.timer.current,
                             &g_EnemyManager.timer.subFrame);
    }
    if (((enemy->flags2 >> 6 & 1) != 0) && (6 < g_GameManager.currentStage)) {
      if ((g_Player.bombInfo.isInUse == 0) ||
          ((g_EnemyManager.spellcardInfo.isActive == 0 ||
            ((i32)g_EnemyManager.spellcardInfo.spellcardIdx < 0x76)))) {
        if (enemy->spellcardDelayTimer == 0) {
          enemy->flags4 = enemy->flags4 & 0xfb;
        } else {
          enemy->spellcardDelayTimer = enemy->spellcardDelayTimer + -1;
        }
      } else {
        enemy->flags4 = enemy->flags4 | 4;
        enemy->spellcardDelayTimer = 1;
      }
    }
    return ZUN_SUCCESS;
  }
  if (enemy->currentContext.time.current != instr->time)
    goto LAB_0041678f;
  if ((instr->skipInstrOnDifficulty & g_GameManager.difficultyMask) == 0)
    goto switchD_0041073a_caseD_7e;
  switch (instr->id) {
  case 1:
    return ZUN_ERROR;
  case 2:
    goto switchD_0041073a_caseD_1;
  case 3: {
    *GetVar(enemy, &instr->args[2].i, instr->paramMask, 2) -= 1;
    i32 local_374 = (instr->paramMask & 4) == 0
                        ? instr->args[2].i
                        : GetVarValue(enemy, instr->args[2].i);
    if (local_374 < 1)
      break;
    goto switchD_0041073a_caseD_1;
  }
  case 4: {
    i32 local_378 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    *GetVar(enemy, &instr->args[0].i, instr->paramMask, 0) = local_378;
    break;
  }
  case 5: {
    f32 local_37c = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) = local_37c;
    break;
  }
  case 6: {
    i32 local_194 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    i32 local_388 = (local_194 == 0) ? 0 : g_Rng.GetRandomU32() % local_194;
    *GetVar(enemy, &instr->args[0].i, instr->paramMask, 0) = local_388;
    break;
  }
  case 7: {
    i32 local_198 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    i32 local_38c = (instr->paramMask & 4) == 0
                        ? instr->args[2].i
                        : GetVarValue(enemy, instr->args[2].i);
    i32 local_390 = (local_198 == 0) ? 0 : g_Rng.GetRandomU32() % local_198;
    *GetVar(enemy, &instr->args[0].i, instr->paramMask, 0) =
        local_390 + local_38c;
    break;
  }
  case 8: {
    f32 local_19c = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        g_Rng.GetRandomFloat() * local_19c;
    break;
  }
  case 9: {
    f32 local_1a0 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    f32 local_398 = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        g_Rng.GetRandomFloat() * local_1a0 + local_398;
    break;
  }
  case 10: {
    i32 local_3a0 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    *GetVar(enemy, &instr->args[0].i, instr->paramMask, 0) =
        (((g_Rng.GetRandomU16() % 2 != 0) ? 2 : 0) - 1) * local_3a0;
    break;
  }
  case 0xb: {
    f32 local_3a4 = (g_Rng.GetRandomU16() % 2 == 0) ? -1.0f : 1.0f;
    f32 local_3a8 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        local_3a4 * local_3a8;
    break;
  }
  case 0xc: {
    i32 local_3c8 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    i32 local_3cc = (instr->paramMask & 4) == 0
                        ? instr->args[2].i
                        : GetVarValue(enemy, instr->args[2].i);
    *GetVar(enemy, &instr->args[0].i, instr->paramMask, 0) =
        local_3c8 + local_3cc;
    break;
  }
  case 0xd: {
    i32 local_3dc = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    i32 local_3e0 = (instr->paramMask & 4) == 0
                        ? instr->args[2].i
                        : GetVarValue(enemy, instr->args[2].i);
    *GetVar(enemy, &instr->args[0].i, instr->paramMask, 0) =
        local_3dc - local_3e0;
    break;
  }
  case 0xe: {
    i32 local_3f0 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    i32 local_3f4 = (instr->paramMask & 4) == 0
                        ? instr->args[2].i
                        : GetVarValue(enemy, instr->args[2].i);
    *GetVar(enemy, &instr->args[0].i, instr->paramMask, 0) =
        local_3f0 * local_3f4;
    break;
  }
  case 0xf: {
    i32 local_404 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    i32 local_408 = (instr->paramMask & 4) == 0
                        ? instr->args[2].i
                        : GetVarValue(enemy, instr->args[2].i);
    *GetVar(enemy, &instr->args[0].i, instr->paramMask, 0) =
        local_404 / local_408;
    break;
  }
  case 0x10: {
    i32 local_418 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    i32 local_41c = (instr->paramMask & 4) == 0
                        ? instr->args[2].i
                        : GetVarValue(enemy, instr->args[2].i);
    *GetVar(enemy, &instr->args[0].i, instr->paramMask, 0) =
        local_418 % local_41c;
    break;
  }
  case 0x11: {
    *GetVar(enemy, &instr->args[0].i, instr->paramMask, 0) += 1;
    break;
  }
  case 0x12: {
    *GetVar(enemy, &instr->args[0].i, instr->paramMask, 0) -= 1;
    break;
  }
  case 0x13: {
    f32 local_3d0 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    f32 local_3d4 = (instr->paramMask & 2) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        local_3d0 + local_3d4;
    break;
  }
  case 0x14: {
    f32 local_3e4 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    f32 local_3e8 = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        local_3e4 - local_3e8;
    break;
  }
  case 0x15: {
    f32 local_3f8 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    f32 local_3fc = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        local_3f8 * local_3fc;
    break;
  }
  case 0x16: {
    f32 local_40c = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    f32 local_410 = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        local_40c / local_410;
    break;
  }
  case 0x17: {
    f32 local_1a4 = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    f32 local_1a8 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        fmodf(local_1a4, local_1a8);
    break;
  }
  case 0x18: {
    f32 local_1ac = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        sinf(local_1ac);
    break;
  }
  case 0x19: {
    f32 local_1b0 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        cosf(local_1b0);
    break;
  }
  case 0x1a: {
    f32 local_438 = (instr->paramMask & 8) == 0
                        ? instr->args[3].f
                        : GetFloatVarValue(enemy, instr->args[3].f);
    f32 local_43c = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    f32 local_440 = (instr->paramMask & 16) == 0
                        ? instr->args[4].f
                        : GetFloatVarValue(enemy, instr->args[4].f);
    f32 local_444 = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        atan2f(local_440 - local_444, local_438 - local_43c);
    break;
  }
  case 0x1b: {
    local_18 = enemy->currentContext.interps;
    for (local_14 = 0; local_14 < 8; local_14 += 1) {
      if ((local_18->fn == NULL) || (local_18->args[7].f == instr->args[0].f)) {
        (local_18->timer).Initialize(0);
        local_18->args[7] = instr->args[0];
        i32 local_464 = (instr->paramMask & 2) == 0
                            ? instr->args[1].i
                            : GetVarValue(enemy, instr->args[1].i);
        local_18->args[0].i = local_464;
        i32 local_468 = (instr->paramMask & 4) == 0
                            ? instr->args[2].i
                            : GetVarValue(enemy, instr->args[2].i);
        local_18->args[1].i = local_468;
        i32 local_46c = (instr->paramMask & 8) == 0
                            ? instr->args[3].i
                            : GetVarValue(enemy, instr->args[3].i);
        local_18->args[2].i = local_46c;
        local_18->fn = g_EclInterpFuncs[local_18->args[1].i];
        f32 local_470 = (instr->paramMask & 16) == 0
                            ? instr->args[4].f
                            : GetFloatVarValue(enemy, instr->args[4].f);
        local_18->args[3].f = local_470;
        f32 local_474 = (instr->paramMask & 32) == 0
                            ? instr->args[5].f
                            : GetFloatVarValue(enemy, instr->args[5].f);
        local_18->args[4].f = local_474;
        f32 local_478 = (instr->paramMask & 64) == 0
                            ? instr->args[6].f
                            : GetFloatVarValue(enemy, instr->args[6].f);
        local_18->args[5].f = local_478;
        f32 local_47c = (instr->paramMask & 128) == 0
                            ? instr->args[7].f
                            : GetFloatVarValue(enemy, instr->args[7].f);
        local_18->args[6].f = local_47c;
        break;
      }
      local_18 = local_18 + 1;
    }
    break;
  }
  case 0x1c: {
    i32 local_480 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    i32 local_484 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    if (local_480 != local_484)
      break;
  LAB_00411f00:
    enemy->currentContext.time.current = instr->args[2].i;
    instr = (EclRawInstr *)((u8 *)instr + instr->args[3].i);
    goto LAB_0041069a;
  }
  case 0x1d: {
    f32 local_488 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    f32 local_48c = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    if (local_488 == local_48c)
      goto LAB_00411f00;
    break;
  }
  case 0x1e: {
    i32 local_490 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    i32 local_494 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    if (local_490 != local_494)
      goto LAB_00411f00;
    break;
  }
  case 0x1f: {
    f32 local_498 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    f32 local_49c = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    if (local_498 != local_49c)
      goto LAB_00411f00;
    break;
  }
  case 0x20: {
    i32 local_4a0 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    i32 local_4a4 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    if (local_4a0 < local_4a4)
      goto LAB_00411f00;
    break;
  }
  case 0x21: {
    f32 local_4a8 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    f32 local_4ac = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    if (local_4a8 < local_4ac)
      goto LAB_00411f00;
    break;
  }
  case 0x22: {
    i32 local_4b0 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    i32 local_4b4 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    if (local_4b0 <= local_4b4)
      goto LAB_00411f00;
    break;
  }
  case 0x23: {
    f32 local_4b8 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    f32 local_4bc = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    if (local_4b8 <= local_4bc)
      goto LAB_00411f00;
    break;
  }
  case 0x24: {
    i32 local_4c0 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    i32 local_4c4 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    if (local_4c4 < local_4c0)
      goto LAB_00411f00;
    break;
  }
  case 0x25: {
    f32 local_4c8 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    f32 local_4cc = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    if (local_4cc < local_4c8)
      goto LAB_00411f00;
    break;
  }
  case 0x26: {
    i32 local_4d0 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    i32 local_4d4 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    if (local_4d4 <= local_4d0)
      goto LAB_00411f00;
    break;
  }
  case 0x27: {
    f32 local_4d8 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    f32 local_4dc = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    if (local_4dc <= local_4d8)
      goto LAB_00411f00;
    break;
  }
  case 0x28: {
    f32 local_380 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        utils::AddNormalizeAngle(local_380, 0.0f);
    break;
  }
  case 0x29:
    goto switchD_0041073a_caseD_28;
  case 0x2a:
    goto switchD_0041073a_caseD_29;
  case 0x2b: {
    i32 local_3bc;
    if ((instr->paramMask & 2) == 0) {
      local_3bc = instr->args[1].i;
    } else {
      i32 local_3b8 = (instr->paramMask & 4) == 0
                          ? instr->args[2].i
                          : GetVarValue(enemy, instr->args[2].i);
      local_3bc =
          GetVarValue(g_EnemyManager.bosses[local_3b8], instr->args[1].i);
    }
    *GetVar(enemy, &instr->args[0].i, instr->paramMask, 0) = local_3bc;
    break;
  }
  case 0x2c: {
    f32 local_3c4;
    if ((instr->paramMask & 2) == 0) {
      local_3c4 = instr->args[1].f;
    } else {
      i32 local_3c0 = (instr->paramMask & 4) == 0
                          ? instr->args[2].i
                          : GetVarValue(enemy, instr->args[2].i);
      local_3c4 =
          GetFloatVarValue(g_EnemyManager.bosses[local_3c0], instr->args[1].f);
    }
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) = local_3c4;
    break;
  }
  case 0x2d: {
    i32 local_18c = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->currentContext.timer2.Initialize(local_18c);
    break;
  }
  case 0x2e: {
    f32 local_4f8 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    enemy->position.x = local_4f8;
    f32 local_4fc = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    enemy->position.y = local_4fc;
    f32 local_500 = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    enemy->position.z = local_500;
    enemy->ClampPos();
    break;
  }
  case 0x2f: {
    f32 local_504 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    enemy->axisSpeed.x = local_504;
    f32 local_508 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    enemy->axisSpeed.y = local_508;
    f32 local_50c = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    enemy->axisSpeed.z = local_50c;
    enemy->angle = atan2f(enemy->axisSpeed.y, enemy->axisSpeed.x);
    enemy->flags1 &= 0xfc;
    break;
  }
  case 0x30: {
    f32 local_514 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    enemy->angularVelocity = local_514;
    enemy->flags1 = (enemy->flags1 & 0xfc) | 1;
    break;
  }
  case 0x31: {
    f32 local_520 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    enemy->moveSpeed = local_520;
    enemy->flags1 = (enemy->flags1 & 0xfc) | 1;
    break;
  }
  case 0x32: {
    f32 local_524 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    enemy->moveAcceleration = local_524;
    enemy->flags1 = (enemy->flags1 & 0xfc) | 1;
    break;
  }
  case 0x33: {
    f32 local_638 = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    f32 local_63c = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    f32 local_640 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        g_Rng.GetRandomFloat() * (local_638 - local_63c) + local_640;
    break;
  }
  case 0x34: {
    f32 local_38 =
        (enemy->position.x <= g_Player.positionCenter.x)
            ? g_Rng.GetRandomFloat() * 1.5707964f - 0.7853982f
            : utils::AddNormalizeAngle(
                  g_Rng.GetRandomFloat() * 1.5707964f + 2.3561945f, 0.0f);
    if (enemy->position.x < (enemy->lowerMoveLimit).x + 96.0) {
      if (local_38 <= 1.5707964f) {
        if (local_38 < -1.5707964f) {
          local_38 = -3.1415927f - local_38;
        }
      } else {
        local_38 = 3.1415927f - local_38;
      }
    }
    if ((enemy->upperMoveLimit).x - 96.0 < enemy->position.x) {
      if ((1.5707964f <= local_38) || (local_38 < 0.0f)) {
        if (-1.5707964f < local_38 && local_38 <= 0.0f) {
          local_38 = -3.1415927f - local_38;
        }
      } else {
        local_38 = 3.1415927f - enemy->angle;
      }
    }
    if ((enemy->position.y < (enemy->lowerMoveLimit).y + 48.0) &&
        (local_38 < 0.0f)) {
      local_38 = -local_38;
    }
    if (((enemy->upperMoveLimit).y - 48.0 < enemy->position.y) &&
        (0.0f < local_38)) {
      local_38 = -local_38;
    }
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) = local_38;
    break;
  }
  case 0x35: {
    f32 local_518 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    enemy->angle = g_Player.AngleToPlayer(&enemy->position) + local_518;
    f32 local_51c = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    enemy->moveSpeed = local_51c;
    enemy->flags1 = (enemy->flags1 & 0xfc) | 1;
    break;
  }
  case 0x36: {
    i32 local_5f0 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    if (local_5f0 < 1) {
      f32 local_5f4 = (instr->paramMask & 4) == 0
                          ? instr->args[2].f
                          : GetFloatVarValue(enemy, instr->args[2].f);
      enemy->angle = utils::AddNormalizeAngle(local_5f4, 0.0f);
      f32 local_5f8 = (instr->paramMask & 8) == 0
                          ? instr->args[3].f
                          : GetFloatVarValue(enemy, instr->args[3].f);
      enemy->moveSpeed = local_5f8;
      enemy->flags1 = (enemy->flags1 & 0xfc) | 1;
      i32 local_25c = (instr->paramMask & 1) == 0
                          ? instr->args[0].i
                          : GetVarValue(enemy, instr->args[0].i);
      enemy->moveInterpStartTime = local_25c;
      enemy->moveInterpTimer.Initialize(local_25c);
    } else {
      MoveDirTime(enemy, instr);
    }
    break;
  }
  case 0x37: {
    MovePosTime(enemy, instr);
    break;
  }
  case 0x38: {
    i32 local_264 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->moveInterpStartTime = local_264;
    enemy->moveInterpTimer.Initialize(local_264);
    f32 local_5fc = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    enemy->moveInterpStartPos.x = local_5fc;
    f32 local_600 = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    enemy->moveInterpStartPos.y = local_600;
    f32 local_604 = (instr->paramMask & 8) == 0
                        ? instr->args[3].f
                        : GetFloatVarValue(enemy, instr->args[3].f);
    enemy->moveInterpStartPos.z = local_604;
    f32 local_608 = (instr->paramMask & 16) == 0
                        ? instr->args[4].f
                        : GetFloatVarValue(enemy, instr->args[4].f);
    enemy->moveAngle = local_608;
    f32 local_60c = (instr->paramMask & 32) == 0
                        ? instr->args[5].f
                        : GetFloatVarValue(enemy, instr->args[5].f);
    enemy->moveAngularVelocity = local_60c;
    f32 local_610 = (instr->paramMask & 64) == 0
                        ? instr->args[6].f
                        : GetFloatVarValue(enemy, instr->args[6].f);
    enemy->moveRadius = local_610;
    f32 local_614 = (instr->paramMask & 128) == 0
                        ? instr->args[7].f
                        : GetFloatVarValue(enemy, instr->args[7].f);
    enemy->moveRadialVelocity = local_614;
    enemy->flags1 |= 3;
    break;
  }
  case 0x39: {
    f32 local_618 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    enemy->moveRadius = local_618;
    f32 local_61c = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    enemy->moveRadialVelocity = local_61c;
    break;
  }
  case 0x3a: {
    f32 local_620 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    enemy->moveAngle = local_620;
    f32 local_624 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    enemy->moveAngularVelocity = local_624;
    break;
  }
  case 0x3b: {
    enemy->flags1 = (enemy->flags1 & 0xfc) | 1;
    i32 local_1d4 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->moveInterpStartTime = local_1d4;
    enemy->moveInterpTimer.Initialize(local_1d4);
    break;
  }
  case 0x3c: {
    enemy->flags1 = enemy->flags1 | 3;
    i32 local_1dc = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->moveInterpStartTime = local_1dc;
    enemy->moveInterpTimer.Initialize(local_1dc);
    break;
  }
  case 0x3d: {
    enemy->flags1 = (enemy->flags1 & 0xfc) | 2;
    i32 local_1e4 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->moveInterpStartTime = local_1e4;
    enemy->moveInterpTimer.Initialize(local_1e4);
    break;
  }
  case 0x3e: {
    f32 local_628 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    (enemy->lowerMoveLimit).x = local_628;
    f32 local_62c = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    (enemy->lowerMoveLimit).y = local_62c;
    f32 local_630 = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    (enemy->upperMoveLimit).x = local_630;
    f32 local_634 = (instr->paramMask & 8) == 0
                        ? instr->args[3].f
                        : GetFloatVarValue(enemy, instr->args[3].f);
    (enemy->upperMoveLimit).y = local_634;
    enemy->flags2 = enemy->flags2 | 0x80;
    break;
  }
  case 0x3f: {
    enemy->flags2 = enemy->flags2 & 0x7f;
    break;
  }
  case 0x40:
  case 0x41:
  case 0x42:
  case 0x43:
  case 0x44:
  case 0x45:
  case 0x46:
  case 0x47:
  case 0x48: {
    if (0 < enemy->life) {
      local_1c = instr->args;
      local_20 = &enemy->bulletProps;
      i16 local_528 = local_1c->s[0];
      local_8 = (i32)local_528;
      if ((instr->paramMask & 1) != 0) {
        local_528 = (i16)GetVarValue(enemy, local_8);
      }
      local_20->sprite = local_528;
      local_20->aimMode = instr->id - 0x40;
      i16 local_52c = (instr->paramMask & 4) == 0
                          ? (i16)local_1c[1].i
                          : (i16)GetVarValue(enemy, local_1c[1].i);
      local_20->count1 = local_52c;
      i32 local_530 = (instr->paramMask & 8) == 0
                          ? (i16)local_1c[2].i
                          : (i16)GetVarValue(enemy, local_1c[2].i);
      local_20->count2 = local_530;
      (local_20->position) = enemy->position + enemy->shootOffset;
      f32 local_534 = (instr->paramMask & 64) == 0
                          ? local_1c[5].f
                          : GetFloatVarValue(enemy, local_1c[5].f);
      local_20->angle1 = local_534;
      f32 local_538 = (instr->paramMask & 16) == 0
                          ? local_1c[3].f
                          : GetFloatVarValue(enemy, local_1c[3].f);
      local_20->speed1 = local_538;
      f32 local_53c = (instr->paramMask & 128) == 0
                          ? local_1c[6].f
                          : GetFloatVarValue(enemy, local_1c[6].f);
      local_20->angle2 = local_53c;
      f32 local_540 = (instr->paramMask & 32) == 0
                          ? local_1c[4].f
                          : GetFloatVarValue(enemy, local_1c[4].f);
      local_20->speed2 = local_540;
      if (g_EnemyManager.spellcardInfo.isActive == 0) {
        iVar19 = ((i32)enemy->bulletRankAmount1High -
                  (i32)enemy->bulletRankAmount1Low) *
                 g_GameManager.rank.rank;
        local_20->count1 = (i16)(i32)(iVar19 / 32) + local_20->count1 +
                           enemy->bulletRankAmount1Low;
        if (local_20->count1 < 1) {
          local_20->count1 = 1;
        }
        iVar19 = ((i32)enemy->bulletRankAmount2High -
                  (i32)enemy->bulletRankAmount2Low) *
                 g_GameManager.rank.rank;
        local_20->count2 = (i16)(i32)(iVar19 / 32) + local_20->count2 +
                           enemy->bulletRankAmount2Low;
        if (local_20->count2 < 1) {
          local_20->count2 = 1;
        }
        if ((local_20->speed1 != 0.0f) &&
            (local_20->speed1 =
                 ((enemy->bulletRankSpeedHigh - enemy->bulletRankSpeedLow) *
                  (f32)g_GameManager.rank.rank) /
                     32.0f +
                 enemy->bulletRankSpeedLow + local_20->speed1,
             local_20->speed1 < 0.3f)) {
          local_20->speed1 = 0.3f;
        }
        local_20->speed2 =
            (((enemy->bulletRankSpeedHigh - enemy->bulletRankSpeedLow) *
              (f32)g_GameManager.rank.rank) /
                 32.0f +
             enemy->bulletRankSpeedLow) /
                2.0f +
            local_20->speed2;
        if (local_20->speed2 < 0.3f) {
          local_20->speed2 = 0.3f;
        }
      }
      local_20->unused_c2 = 0;
      local_20->flags = local_1c[7].u;
      i16 local_544 = local_1c->s[1];
      local_8 = (i32)local_544;
      if ((instr->paramMask & 2) != 0) {
        local_544 = (i16)GetVarValue(enemy, local_8);
      }
      local_20->spriteOffset = local_544;
      if ((enemy->flags1 >> 5 & 1) == 0) {
        g_BulletManager.SpawnBulletPattern(local_20);
      }
    }
    break;
  }
  case 0x49: {
    i32 local_564 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->shootInterval = local_564;
    if (enemy->shootInterval != 0) {
      iVar19 = enemy->shootInterval / 5;
      iVar13 = (-enemy->shootInterval / 5 - iVar19) * g_GameManager.rank.rank;
      enemy->shootInterval = (i32)(iVar13 / 32) + iVar19 + enemy->shootInterval;
      enemy->shootIntervalTimer.Initialize(0);
    }
    break;
  }
  case 0x4a: {
    i32 local_568 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->shootInterval = local_568;
    if (enemy->shootInterval != 0) {
      iVar19 = enemy->shootInterval / 5;
      iVar13 = (-enemy->shootInterval / 5 - iVar19) * g_GameManager.rank.rank;
      enemy->shootInterval = (i32)(iVar13 / 32) + iVar19 + enemy->shootInterval;
      i32 local_220 = (enemy->shootInterval == 0)
                          ? 0
                          : g_Rng.GetRandomU32() % enemy->shootInterval;
      enemy->shootIntervalTimer.Initialize(local_220);
    }
    break;
  }
  case 0x4b: {
    enemy->flags1 |= 0x20;
    break;
  }
  case 0x4c: {
    enemy->flags1 &= 0xdf;
    break;
  }
  case 0x4d: {
    enemy->bulletProps.position = enemy->position + enemy->shootOffset;
    g_BulletManager.SpawnBulletPattern(&enemy->bulletProps);
    break;
  }
  case 0x4e: {
    f32 local_56c = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    enemy->shootOffset.x = local_56c;
    f32 local_570 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    enemy->shootOffset.y = local_570;
    f32 local_574 = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    enemy->shootOffset.z = local_574;
    break;
  }
  case 0x4f: {
    i32 local_548 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    local_24 = &enemy->bulletProps.commands[local_548];
    i32 local_54c = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    local_24->type = local_54c;
    i32 local_550 = (instr->paramMask & 4) == 0
                        ? instr->args[2].i
                        : GetVarValue(enemy, instr->args[2].i);
    local_24->flag = local_550;
    i32 local_554 = (instr->paramMask & 8) == 0
                        ? instr->args[3].i
                        : GetVarValue(enemy, instr->args[3].i);
    local_24->duration = local_554;
    i32 local_558 = (instr->paramMask & 16) == 0
                        ? instr->args[4].i
                        : GetVarValue(enemy, instr->args[4].i);
    local_24->loopCount = local_558;
    f32 local_55c = (instr->paramMask & 32) == 0
                        ? instr->args[5].f
                        : GetFloatVarValue(enemy, instr->args[5].f);
    local_24->speed = local_55c;
    f32 local_560 = (instr->paramMask & 64) == 0
                        ? instr->args[6].f
                        : GetFloatVarValue(enemy, instr->args[6].f);
    local_24->angle = local_560;
    break;
  }
  case 0x50: {
    g_BulletManager.RemoveAllBullets(1);
    break;
  }
  case 0x51: {
    i32 local_724 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    if (local_724 < 0) {
      enemy->bulletProps.flags &= 0xfffffdff;
    } else {
      i32 local_728 = (instr->paramMask & 1) == 0
                          ? instr->args[0].i
                          : GetVarValue(enemy, instr->args[0].i);
      enemy->bulletProps.soundIdx = local_728;
      enemy->bulletProps.flags = enemy->bulletProps.flags | 0x200;
    }
    i32 local_72c = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    enemy->bulletProps.soundOverride = local_72c;
    break;
  }
  case 0x52:
  case 0x53: {
    local_28 = &enemy->laserProps;
    (enemy->laserProps).position = enemy->position + enemy->shootOffset;
    local_28->sprite = instr->args[0].s[0];
    i16 local_578 = (instr->paramMask & 2) == 0
                        ? instr->args[0].s[1]
                        : (i16)GetVarValue(enemy, (i32)instr->args[0].s[1]);
    local_28->spriteOffset = local_578;
    f32 local_57c = (instr->paramMask & 4) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    local_28->angle1 = local_57c;
    f32 local_580 = (instr->paramMask & 8) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    local_28->speed1 = local_580;
    f32 local_584 = (instr->paramMask & 16) == 0
                        ? instr->args[3].f
                        : GetFloatVarValue(enemy, instr->args[3].f);
    local_28->startOffset = local_584;
    f32 local_588 = (instr->paramMask & 32) == 0
                        ? instr->args[4].f
                        : GetFloatVarValue(enemy, instr->args[4].f);
    local_28->endOffset = local_588;
    f32 local_58c = (instr->paramMask & 64) == 0
                        ? instr->args[5].f
                        : GetFloatVarValue(enemy, instr->args[5].f);
    local_28->startLength = local_58c;
    local_28->width = instr->args[6].f;
    local_28->startTime = instr->args[7].i;
    local_28->duration = instr->args[8].i;
    local_28->endTime = instr->args[9].i;
    local_28->grazeDelay = instr->args[10].i;
    local_28->aimMode = instr->args[11].i;
    local_28->flags = instr->args[12].u;
    local_28->type = (instr->id == 0x53) ? 0 : 1;
    enemy->lasers[enemy->laserIdx] =
        g_BulletManager.SpawnLaserPattern(local_28);
    break;
  }
  case 0x54: {
    i32 local_590 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->laserIdx = local_590;
    break;
  }
  case 0x55: {
    i32 local_594 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    local_8 = local_594;
    if (enemy->lasers[local_594] != NULL) {
      f32 local_598 = (instr->paramMask & 2) == 0
                          ? instr->args[1].f
                          : GetFloatVarValue(enemy, instr->args[1].f);
      enemy->lasers[local_8]->angle =
          utils::AddNormalizeAngle(enemy->lasers[local_8]->angle, local_598);
    }
    break;
  }
  case 0x56: {
    i32 local_5a4 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    local_8 = local_5a4;
    if (enemy->lasers[local_5a4] != NULL) {
      f32 local_5a8 = (instr->paramMask & 2) == 0
                          ? instr->args[1].f
                          : GetFloatVarValue(enemy, instr->args[1].f);
      enemy->lasers[local_8]->angle =
          g_Player.AngleToPlayer(&enemy->lasers[local_8]->pos) + local_5a8;
    }
    break;
  }
  case 0x57: {
    i32 local_5ac = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    local_8 = local_5ac;
    if (enemy->lasers[local_5ac] != NULL) {
      f32 local_5b0 = (instr->paramMask & 2) == 0
                          ? instr->args[1].f
                          : GetFloatVarValue(enemy, instr->args[1].f);
      (enemy->lasers[local_8]->pos).x = local_5b0 + enemy->position.x;
      f32 local_5b4 = (instr->paramMask & 4) == 0
                          ? instr->args[2].f
                          : GetFloatVarValue(enemy, instr->args[2].f);
      (enemy->lasers[local_8]->pos).y = local_5b4 + enemy->position.y;
      f32 local_5b8 = (instr->paramMask & 8) == 0
                          ? instr->args[3].f
                          : GetFloatVarValue(enemy, instr->args[3].f);
      (enemy->lasers[local_8]->pos).z = local_5b8 + enemy->position.z;
    }
    break;
  }
  case 0x58: {
    i32 local_5c4 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    local_8 = local_5c4;
    if ((enemy->lasers[local_5c4] == NULL) ||
        (enemy->lasers[local_5c4]->inUse == 0)) {
      enemy->currentContext.compareRegister = 1;
    } else {
      enemy->currentContext.compareRegister = 0;
    }
    break;
  }
  case 0x59: {
    i32 local_5c8 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    local_8 = local_5c8;
    if (((enemy->lasers[local_5c8] != NULL) &&
         (enemy->lasers[local_5c8]->inUse != 0)) &&
        (enemy->lasers[local_5c8]->state < 2)) {
      enemy->lasers[local_5c8]->state = 2;
      enemy->lasers[local_5c8]->timer.Initialize(0);
      enemy->lasers[local_5c8]->width = enemy->lasers[local_5c8]->targetWidth;
    }
    break;
  }
  case 0x5a: {
    BeginSpellcard(enemy, instr);
    break;
  }
  case 0x5b: {
    EndSpellcard();
    break;
  }
  case 0x5c: {
    if (0 < enemy->life) {
      memcpy(local_94, instr->args, sizeof(local_94));
      f32 local_6f0 = (instr->paramMask & 2) == 0
                          ? local_94[1].f
                          : GetFloatVarValue(enemy, local_94[1].f);
      local_a0.x = local_6f0;
      f32 local_6f4 = (instr->paramMask & 4) == 0
                          ? local_94[2].f
                          : GetFloatVarValue(enemy, local_94[2].f);
      local_a0.y = local_6f4;
      f32 local_6f8 = (instr->paramMask & 8) == 0
                          ? local_94[3].f
                          : GetFloatVarValue(enemy, local_94[3].f);
      local_a0.z = local_6f8;
      i32 local_6fc = (instr->paramMask & 64) == 0
                          ? instr->args[6].i
                          : GetVarValue(enemy, instr->args[6].i);
      i32 local_700 = (instr->paramMask & 32) == 0
                          ? instr->args[5].i
                          : GetVarValue(enemy, instr->args[5].i);
      i32 local_704 = (instr->paramMask & 16) == 0
                          ? instr->args[4].i
                          : GetVarValue(enemy, instr->args[4].i);
      g_EnemyManager.SpawnEnemyEx(local_94[0].i, &local_a0, local_704,
                                  local_700, local_6fc,
                                  &enemy->currentContext.eclContextArgs);
    }
    break;
  }
  case 0x5d: {
    if (0 < enemy->life) {
      memcpy(local_c0, instr->args, sizeof(local_c0));
      f32 local_708 = (instr->paramMask & 2) == 0
                          ? local_c0[1].f
                          : GetFloatVarValue(enemy, local_c0[1].f);
      local_cc.x = local_708;
      f32 local_70c = (instr->paramMask & 4) == 0
                          ? local_c0[2].f
                          : GetFloatVarValue(enemy, local_c0[2].f);
      local_cc.y = local_70c;
      f32 local_710 = (instr->paramMask & 8) == 0
                          ? local_c0[3].f
                          : GetFloatVarValue(enemy, local_c0[3].f);
      local_cc.z = local_710;
      local_cc += enemy->position;
      i32 local_714 = (instr->paramMask & 64) == 0
                          ? instr->args[6].i
                          : GetVarValue(enemy, instr->args[6].i);
      i32 local_718 = (instr->paramMask & 32) == 0
                          ? instr->args[5].i
                          : GetVarValue(enemy, instr->args[5].i);
      i32 local_71c = (instr->paramMask & 16) == 0
                          ? instr->args[4].i
                          : GetVarValue(enemy, instr->args[4].i);
      g_EnemyManager.SpawnEnemyEx(local_c0[0].i, &local_cc, local_71c,
                                  local_718, local_714,
                                  &enemy->currentContext.eclContextArgs);
    }
    break;
  }
  case 0x5e: {
    g_EnemyManager.RemoveAllEnemies(8000, 0);
    break;
  }
  case 0x5f: {
    i32 local_4e0 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    i16 local_1b8 = (i16)local_4e0 + 0x900;
    enemy->primaryVm.anmFileIdx = local_1b8;
    g_AnmManager->SetAndExecuteScript(&enemy->primaryVm,
                                      g_AnmManager->scripts[local_4e0 + 0x900]);
    break;
  }
  case 0x60: {
    enemy->anmExDefaults = instr->args[0].s[0];
    enemy->anmExFarLeft = instr->args[0].s[1];
    enemy->anmExFarRight = instr->args[1].s[0];
    enemy->anmExLeft = instr->args[1].s[1];
    enemy->anmExRight = instr->args[2].s[0];
    enemy->anmExFlags = 0xff;
    break;
  }
  case 0x61: {
    i32 local_4e4 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    if (1 < local_4e4) {
      DebugPrint("error : sub anim overflow\r\n");
    }
    i32 local_4e8 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    if (local_4e8 < 0) {
      i32 local_4f4 = (instr->paramMask & 1) == 0
                          ? instr->args[0].i
                          : GetVarValue(enemy, instr->args[0].i);
      enemy->vms[local_4f4].anmFileIdx = -1;
    } else {
      i32 local_4ec = (instr->paramMask & 2) == 0
                          ? instr->args[1].i
                          : GetVarValue(enemy, instr->args[1].i);
      i32 local_4f0 = (instr->paramMask & 1) == 0
                          ? instr->args[0].i
                          : GetVarValue(enemy, instr->args[0].i);
      i16 local_1c0 = (i16)local_4ec + 0x900;
      enemy->vms[local_4f0].anmFileIdx = local_1c0;
      g_AnmManager->SetAndExecuteScript(
          enemy->vms + local_4f0, g_AnmManager->scripts[local_4ec + 0x900]);
    }
    break;
  }
  case 0x62: {
    enemy->deathAnm1 = instr->args[0].c[0];
    enemy->deathAnm2 = instr->args[0].c[1];
    enemy->deathAnm3 = instr->args[0].c[2];
    break;
  }
  case 99: {
    i32 local_5e4 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    if (local_5e4 < 0) {
      if (enemy->bossId < 4) {
        g_Gui.bossPresent = 0;
      }
      g_EnemyManager.bosses[enemy->bossId] = NULL;
      enemy->flags2 &= 0xbf;
      g_AsciiManager.otherVms[enemy->bossId + 3].pendingInterrupt = 2;
      enemy->ResetEffectArray();
    } else {
      i32 local_5e8 = (instr->paramMask & 1) == 0
                          ? instr->args[0].i
                          : GetVarValue(enemy, instr->args[0].i);
      g_EnemyManager.bosses[local_5e8] = enemy;
      g_Gui.bossPresent = 1;
      g_Gui.bossHealthBar = 1.0f;
      enemy->flags2 |= 0x40;
      i32 local_5ec = (instr->paramMask & 1) == 0
                          ? instr->args[0].i
                          : GetVarValue(enemy, instr->args[0].i);
      enemy->bossId = local_5ec;
      g_AsciiManager.otherVms[enemy->bossId + 3].pendingInterrupt = 1;
    }
    break;
  }
  case 100: {
    local_34 = instr->args;
    enemy->effects[enemy->effectsNum] = g_EffectManager.SpawnParticles(
        0xd, &enemy->position, 1, g_BulletColor[local_34->i]);
    (enemy->effects[enemy->effectsNum]->direction).x = local_34[1].f;
    (enemy->effects[enemy->effectsNum]->direction).y = local_34[2].f;
    (enemy->effects[enemy->effectsNum]->direction).z = local_34[3].f;
    enemy->effectDistance = local_34[4].f;
    enemy->effectsNum = enemy->effectsNum + 1;
    break;
  }
  case 0x65: {
    f32 local_64c = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    (enemy->hitboxSize).x = local_64c;
    f32 local_658 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    (enemy->hitboxSize).y = local_658;
    local_658 = (instr->paramMask & 4) == 0
                    ? instr->args[2].f
                    : GetFloatVarValue(enemy, instr->args[2].f);
    (enemy->hitboxSize).z = local_658;
    break;
  }
  case 0x66: {
    enemy->flags2 = (enemy->flags2 & 0xfd) | (instr->args[0].b[0] & 1) << 1;
    break;
  }
  case 0x67: {
    enemy->flags2 = (enemy->flags2 & 0xfb) | (instr->args[0].b[0] & 1) << 2;
    break;
  }
  case 0x68: {
    enemy->flags2 = (enemy->flags2 & 0xef) | (instr->args[0].b[0] & 1) << 4;
    break;
  }
  case 0x69: {
    i32 local_664 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    g_SoundPlayer.PlaySoundByIdx(local_664);
    break;
  }
  case 0x6a: {
    enemy->flags3 = (enemy->flags3 & 0xf8) | (instr->args[0].b[0] & 7);
    break;
  }
  case 0x6b: {
    enemy->deathCallbackSub = (u32)instr->args[0].b[0];
    break;
  }
  case 0x6c: {
    i32 local_668 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    i32 local_66c = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    enemy->interrupts[local_66c] = local_668;
    break;
  }
  case 0x6d: {
    i32 local_670 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->runInterrupt = local_670;
    goto LAB_00414b54;
  }
  case 0x6e: {
    i32 local_674 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->maxLife = local_674;
    enemy->life = local_674;
    if ((enemy->bossId == 0) && ((enemy->flags2 >> 6 & 1) != 0)) {
      for (local_3c = 0; local_3c < 8; local_3c += 1) {
        g_Gui.bossHealthEased[local_3c] = 0.0f;
        g_Gui.bossHealth[local_3c] = 0.0f;
      }
    }
    break;
  }
  case 0x6f: {
    i32 local_278 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->timer.Initialize(local_278);
    break;
  }
  case 0x70: {
    i32 local_684 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->lifeCallbackThreshold[0] = local_684;
    break;
  }
  case 0x71: {
    i32 local_688 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->lifeCallbackSub[0] = local_688;
    break;
  }
  case 0x72: {
    i32 local_69c = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->timerCallbackThreshold = local_69c;
    enemy->timer.Initialize(0);
    break;
  }
  case 0x73: {
    i32 local_6a0 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->timerCallbackSub = local_6a0;
    break;
  }
  case 0x74: {
    enemy->flags2 = (enemy->flags2 & 0xfe) | (instr->args[0].b[0] & 1);
    break;
  }
  case 0x75: {
    i32 local_6a8 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    i32 local_6ac = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    g_EffectManager.SpawnParticles(
        local_6ac, &enemy->position, local_6a8,
        *(D3DCOLOR *)GetVar(enemy, &instr->args[2].i, instr->paramMask, 2));
    break;
  }
  case 0x76: {
    f32 local_6b0 = (instr->paramMask & 8) == 0
                        ? instr->args[3].f
                        : GetFloatVarValue(enemy, instr->args[3].f);
    local_4c.x = local_6b0;
    f32 local_6b4 = (instr->paramMask & 16) == 0
                        ? instr->args[4].f
                        : GetFloatVarValue(enemy, instr->args[4].f);
    local_4c.y = local_6b4;
    f32 local_6b8 = (instr->paramMask & 32) == 0
                        ? instr->args[5].f
                        : GetFloatVarValue(enemy, instr->args[5].f);
    local_4c.z = local_6b8;
    i32 local_6bc = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    i32 local_6c0 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    g_EffectManager.SpawnMovingParticles(
        local_6c0, &enemy->position, &local_4c, local_6bc,
        *(D3DCOLOR *)GetVar(enemy, &instr->args[2].i, instr->paramMask, 2));
    break;
  }
  case 0x77: {
    i32 local_6c4 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    local_50 = local_6c4;
    for (local_54 = 0; local_54 < local_50; local_54 += 1) {
      local_60 = enemy->position;
      local_60.x = (g_Rng.GetRandomFloat() * 128.0f - 64.0f) + local_60.x;
      local_60.y = (g_Rng.GetRandomFloat() * 128.0f - 64.0f) + local_60.y;
      if ((i32)(g_GameManager.globals)->currentPower < 128) {
        g_ItemManager.SpawnItem(&local_60, (local_54 != 0) - 1 & 2, 0);
      } else {
        g_ItemManager.SpawnItem(&local_60, ITEM_POINT, 0);
      }
    }
    break;
  }
  case 0x78: {
    enemy->flags3 = (enemy->flags3 & 0xef) | (instr->args[0].b[0] & 1) << 4;
    break;
  }
  case 0x79: {
    i32 local_6dc = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    (*g_EclExInstr[local_6dc])(enemy, instr);
    break;
  }
  case 0x7a: {
    i32 local_6e0 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    if (local_6e0 < 0) {
      enemy->currentContext.func = NULL;
    } else {
      i32 local_6e4 = (instr->paramMask & 1) == 0
                          ? instr->args[0].i
                          : GetVarValue(enemy, instr->args[0].i);
      enemy->currentContext.func = g_EclExInstr[local_6e4];
      enemy->currentContext.eclExInstr = instr;
    }
    break;
  }
  case 0x7b: {
    i32 local_290 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->currentContext.time.Increment(local_290);
    break;
  }
  case 0x7c: {
    i32 local_6e8 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    g_ItemManager.SpawnItem(&enemy->position, local_6e8, 0);
    break;
  }
  case 0x7d: {
    i32 local_6ec = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    g_Stage.scriptWaitTime = local_6ec;
    break;
  }
  case 0x7e: {
    i32 local_294 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    g_Gui.bossLifeMarkers = local_294;
    g_GameManager.activeFrameCounter += 0x708;
    break;
  }
  case 0x80: {
    i32 local_720 = (instr->paramMask & 1) == 0
                        ? (i16)instr->args[0].i
                        : (i16)GetVarValue(enemy, instr->args[0].i);
    enemy->primaryVm.pendingInterrupt = local_720;
    break;
  }
  case 0x81: {
    enemy->vms[instr->args[0].i].pendingInterrupt = instr->args[1].s[0];
    break;
  }
  case 0x82: {
    enemy->flags3 = (enemy->flags3 & 0xdf) | (instr->args[0].b[0] & 1) << 5;
    break;
  }
  case 0x83: {
    f32 local_730 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    enemy->bulletRankSpeedLow = local_730;
    f32 local_734 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    enemy->bulletRankSpeedHigh = local_734;
    i16 local_738 = (instr->paramMask & 4) == 0
                        ? (i16)instr->args[2].i
                        : (i16)GetVarValue(enemy, instr->args[2].i);
    enemy->bulletRankAmount1Low = local_738;
    i16 local_73c = (instr->paramMask & 8) == 0
                        ? (i16)instr->args[3].i
                        : (i16)GetVarValue(enemy, instr->args[3].i);
    enemy->bulletRankAmount1High = local_73c;
    i16 local_740 = (instr->paramMask & 16) == 0
                        ? (i16)instr->args[4].i
                        : (i16)GetVarValue(enemy, instr->args[4].i);
    enemy->bulletRankAmount2Low = local_740;
    i16 local_744 = (instr->paramMask & 32) == 0
                        ? (i16)instr->args[5].i
                        : (i16)GetVarValue(enemy, instr->args[5].i);
    enemy->bulletRankAmount2High = local_744;
    break;
  }
  case 0x84: {
    enemy->flags2 = (enemy->flags2 & 0xf7) | (instr->args[0].b[0] & 1) << 3;
    break;
  }
  case 0x85: {
    enemy->timerCallbackSub = enemy->deathCallbackSub;
    enemy->timer.Initialize(0);
    break;
  }
  case 0x86: {
    for (local_30 = 0; local_30 < 0x20; local_30 += 1) {
      enemy->lasers[local_30] = NULL;
    }
    break;
  }
  case 0x87: {
    enemy->flags3 = (enemy->flags3 & 0xbf) | (instr->args[0].b[0] & 1) << 6;
    break;
  }
  case 0x88: {
    enemy->flags2 = (enemy->flags2 & 0xdf) | (instr->args[0].b[0] & 1) << 5;
    enemy->zLayer = 2;
    break;
  }
  case 0x89: {
    enemy->flags3 = (enemy->flags3 & 0x7f) | instr->args[0].b[0] << 7;
    break;
  }
  case 0x8a: {
    enemy->trailFlags = instr->args[0].c[0];
    i16 local_748 = (instr->paramMask & 2) == 0
                        ? (i16)instr->args[1].i
                        : (i16)GetVarValue(enemy, instr->args[1].i);
    enemy->trailCount = local_748;
    i16 local_74c = (instr->paramMask & 4) == 0
                        ? (i16)instr->args[2].i
                        : (i16)GetVarValue(enemy, instr->args[2].i);
    enemy->trailInterval = local_74c;
    i16 local_750 = (instr->paramMask & 8) == 0
                        ? (i16)instr->args[3].i
                        : (i16)GetVarValue(enemy, instr->args[3].i);
    enemy->trailNodeStep = local_750;
    if ((enemy->trailFlags & 8) != 0) {
      AnmManager::UpdateTrail(
          &enemy->primaryVm, enemy->trailVertices,
          (i32)enemy->trailCount / (i32)enemy->trailNodeStep << 1);
    }
    break;
  }
  case 0x8b: {
    i32 local_678 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    local_40 = local_678;
    i32 local_67c = (instr->paramMask & 4) == 0
                        ? instr->args[2].i
                        : GetVarValue(enemy, instr->args[2].i);
    i32 local_680 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    g_Gui.bossHealthEased[local_40] = (f32)local_680 / (f32)enemy->maxLife;
    g_Gui.bossHealth[local_40] = (f32)local_67c / (f32)enemy->maxLife;
    i32 local_274 = (instr->paramMask & 8) == 0
                        ? instr->args[3].i
                        : GetVarValue(enemy, instr->args[3].i);
    g_Gui.bossColor[local_40] = local_274;
    break;
  }
  case 0x8c: {
    f32 local_754 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    g_EffectManager.globalColorMultiplierR = local_754;
    f32 local_758 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    g_EffectManager.globalColorMultiplierG = local_758;
    f32 local_75c = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    g_EffectManager.globalColorMultiplierB = local_75c;
    f32 local_760 = (instr->paramMask & 8) == 0
                        ? instr->args[3].f
                        : GetFloatVarValue(enemy, instr->args[3].f);
    g_EffectManager.globalColorMultiplierA = local_760;
    break;
  }
  case 0x8e: {
    i32 local_2a0 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->invincibilityTimer.Initialize(local_2a0);
    break;
  }
  case 0x8f: {
    f32 local_764 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    BulletManager::RemoveBulletsInRadius(&enemy->position, local_764);
    break;
  }
  case 0x90: {
    i32 local_284 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    (enemy->periodicTimer).Initialize(local_284);
    i32 local_6a4 = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    enemy->periodicCallbackSub = local_6a4;
    enemy->periodicCounter.Initialize(0);
    enemy->savedEclContextArgs = enemy->currentContext.eclContextArgs;
    break;
  }
  case 0x91: {
    i32 local_768 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    if (g_EnemyManager.bosses[local_768] != NULL) {
      i32 local_76c = (instr->paramMask & 2) == 0
                          ? instr->args[1].i
                          : GetVarValue(enemy, instr->args[1].i);
      i32 local_770 = (instr->paramMask & 1) == 0
                          ? instr->args[0].i
                          : GetVarValue(enemy, instr->args[0].i);
      g_EnemyManager.bosses[local_770]->runInterrupt = local_76c;
    }
    break;
  }
  case 0x92: {
    g_BulletManager.RemoveAllBullets(0);
    break;
  }
  case 0x93: {
    i32 local_5e0 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    g_EnemyManager.unused_9545f0 = local_5e0;
    break;
  }
  case 0x94: {
    i32 local_68c = (instr->paramMask & 2) == 0
                        ? instr->args[1].i
                        : GetVarValue(enemy, instr->args[1].i);
    i32 local_690 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->lifeCallbackThreshold[local_690] = local_68c;
    i32 local_694 = (instr->paramMask & 4) == 0
                        ? instr->args[2].i
                        : GetVarValue(enemy, instr->args[2].i);
    i32 local_698 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    enemy->lifeCallbackSub[local_698] = local_694;
    break;
  }
  case 0x95: {
    u8 local_774 = (instr->paramMask & 1) == 0
                       ? (u8)instr->args[0].u
                       : (u8)GetVarValue(enemy, instr->args[0].i);
    enemy->flags4 = (enemy->flags4 & 0xfd) | (local_774 & 1) << 1;
    if ((enemy->flags4 >> 1 & 1) == 0) {
      f32 local_778 = (instr->paramMask & 2) == 0
                          ? instr->args[1].f
                          : GetFloatVarValue(enemy, instr->args[1].f);
      enemy->specialEffect->pos1.x = local_778;
      f32 local_77c = (instr->paramMask & 4) == 0
                          ? instr->args[2].f
                          : GetFloatVarValue(enemy, instr->args[2].f);
      enemy->specialEffect->pos1.y = local_77c;
      f32 local_780 = (instr->paramMask & 8) == 0
                          ? instr->args[3].f
                          : GetFloatVarValue(enemy, instr->args[3].f);
      enemy->specialEffect->pos1.z = local_780;
    }
    break;
  }
  case 0x96: {
    f32 local_784 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    enemy->primaryVm.rotation.z = local_784;
    break;
  }
  case 0x97: {
    f32 local_2a8 = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    f32 local_788 = (instr->paramMask & 8) == 0
                        ? instr->args[3].f
                        : GetFloatVarValue(enemy, instr->args[3].f);
    *GetFloatVar(enemy, &instr->args[1].f, instr->paramMask, 1) =
        sinf(local_2a8) * local_788;
    f32 local_2ac = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    f32 local_794 = (instr->paramMask & 8) == 0
                        ? instr->args[3].f
                        : GetFloatVarValue(enemy, instr->args[3].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        cosf(local_2ac) * local_794;
    break;
  }
  case 0x98: {
    i32 local_59c = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    local_8 = local_59c;
    if (enemy->lasers[local_59c] != NULL) {
      f32 local_5a0 = (instr->paramMask & 2) == 0
                          ? instr->args[1].f
                          : GetFloatVarValue(enemy, instr->args[1].f);
      enemy->lasers[local_8]->angle = local_5a0;
    }
    break;
  }
  case 0x99: {
    f32 local_658 = (instr->paramMask & 1) == 0
                        ? instr->args[0].f
                        : GetFloatVarValue(enemy, instr->args[0].f);
    enemy->grazeSize.x = local_658;
    f32 local_65c = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    enemy->grazeSize.y = local_65c;
    f32 local_660 = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    enemy->grazeSize.z = local_660;
    break;
  }
  case 0x9a: {
    i32 local_6d0 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    local_64 = local_6d0;
    for (local_68 = 0; local_68 < local_64; local_68 += 1) {
      local_74 = enemy->position;
      local_74.x = (g_Rng.GetRandomFloat() * 128.0f - 64.0f) + local_74.x;
      local_74.y = (g_Rng.GetRandomFloat() * 128.0f - 64.0f) + local_74.y;
      g_ItemManager.SpawnItem(&local_74, ITEM_POINT, 0);
    }
    break;
  }
  case 0x9b: {
    if (((g_Player.positionCenter.x < enemy->position.x) &&
         (96.0 < enemy->position.x)) ||
        (288.0 < enemy->position.x)) {
      *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
          utils::AddNormalizeAngle(
              g_Rng.GetRandomFloat() * 1.5707964f + 2.3561945f, 0.0f);
    } else {
      *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
          g_Rng.GetRandomFloat() * 1.5707964f - 0.7853982f;
    }
    break;
  }
  case 0x9c: {
    i32 local_5bc = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    local_8 = local_5bc;
    if (enemy->lasers[local_5bc] != NULL) {
      u8 local_5c0 = (instr->paramMask & 2) == 0
                         ? (u8)instr->args[1].i
                         : (u8)GetVarValue(enemy, instr->args[1].i);
      enemy->lasers[local_8]->hideWarning = local_5c0;
    }
    break;
  }
  case 0x9d: {
    i32 local_5cc = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    local_8 = local_5cc;
    if (enemy->lasers[local_5cc] != NULL) {
      f32 local_5d0 = (instr->paramMask & 2) == 0
                          ? instr->args[1].f
                          : GetFloatVarValue(enemy, instr->args[1].f);
      enemy->lasers[local_8]->startLength = local_5d0;
    }
    break;
  }
  case 0x9e: {
    i32 local_5d4 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    local_8 = local_5d4;
    if (enemy->lasers[local_5d4] != NULL) {
      f32 local_5d8 = (instr->paramMask & 2) == 0
                          ? instr->args[1].f
                          : GetFloatVarValue(enemy, instr->args[1].f);
      enemy->lasers[local_8]->startOffset = local_5d8;
      f32 local_5dc = (instr->paramMask & 4) == 0
                          ? instr->args[2].f
                          : GetFloatVarValue(enemy, instr->args[2].f);
      enemy->lasers[local_8]->endOffset = local_5dc;
    }
    break;
  }
  case 0x9f: {
    f32 local_450 = (instr->paramMask & 2) == 0
                        ? instr->args[1].f
                        : GetFloatVarValue(enemy, instr->args[1].f);
    f32 local_454 = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    local_10 = local_450 - local_454;
    f32 local_458 = (instr->paramMask & 8) == 0
                        ? instr->args[3].f
                        : GetFloatVarValue(enemy, instr->args[3].f);
    f32 local_45c = (instr->paramMask & 4) == 0
                        ? instr->args[2].f
                        : GetFloatVarValue(enemy, instr->args[2].f);
    *GetFloatVar(enemy, &instr->args[0].f, instr->paramMask, 0) =
        local_10 * local_458 + local_45c;
    break;
  }
  case 0xa0: {
    i32 local_7a8 = (instr->paramMask & 1) == 0
                        ? instr->args[0].i
                        : GetVarValue(enemy, instr->args[0].i);
    g_GameManager.AddCherryPlus(local_7a8);
    break;
  }
  case 0xa1: {
    u8 local_7ac = (instr->paramMask & 1) == 0
                       ? (u8)instr->args[0].u
                       : (u8)GetVarValue(enemy, instr->args[0].i);
    enemy->flags4 = (enemy->flags4 & 0xf7) | (local_7ac & 1) << 3;
  }
  }
switchD_0041073a_caseD_7e:
  instr = (EclRawInstr *)((u8 *)instr + instr->size);
  goto LAB_0041069a;
switchD_0041073a_caseD_29:
  if ((enemy->flags3 >> 5 & 1) != 0) {
    DebugPrint("error : no Stack Ret\r\n");
  }
  enemy->stackDepth = enemy->stackDepth + -1;
  if (enemy->currentContext.isPeriodicSub != 0) {
    enemy->savedEclContextArgs = enemy->currentContext.eclContextArgs;
    enemy->currentContext.isPeriodicSub = 0;
  }
  enemy->currentContext = enemy->savedContextStack[enemy->stackDepth];
  goto LAB_00410531;
switchD_0041073a_caseD_28:
  local_8 = instr->args[0].i;
  enemy->currentContext.curInstr = (EclRawInstr *)((u8 *)instr + instr->size);
  if ((enemy->flags3 >> 5 & 1) == 0) {
    enemy->savedContextStack[enemy->stackDepth] = enemy->currentContext;
  }
  g_EclManager.CallEclSub(&enemy->currentContext, (i16)local_8);
  enemy->currentContext.eclContextArgs.globalVars = g_GlobalEclVars;
  if (((enemy->flags3 >> 5 & 1) == 0) && (enemy->stackDepth < 0xf)) {
    enemy->stackDepth = enemy->stackDepth + 1;
  }
  goto LAB_00410531;
switchD_0041073a_caseD_1:
  enemy->currentContext.time.current = instr->args[0].i;
  instr = (EclRawInstr *)((u8 *)instr + instr->args[1].i);
  goto LAB_0041069a;
}
