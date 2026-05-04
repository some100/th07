#pragma once

#include <d3d8.h>

#include "EclManager.hpp"

extern EclExInstr g_EclExInstr[24];
extern EclInterpFn g_EclInterpFuncs[8];

namespace EnemyEclInstr
{

void ExInsSetPosToBoss(Enemy *enemy, EclRawInstr *instr);
void ExInsAliceCurveBullets(Enemy *enemy, EclRawInstr *instr);
void ExInsTurnBulletsIntoOtherBullets(Enemy *enemy, EclRawInstr *instr);
void ExInsNoOp(Enemy *enemy, EclRawInstr *instr);
void ExInsDespawnLargeBulletAndSavePos(Enemy *enemy, EclRawInstr *instr);
void ExInsCopyMainBossMovement(Enemy *enemy, EclRawInstr *instr);
void ExInsSplitBulletsOrShootBackwards(Enemy *enemy, EclRawInstr *instr);
void ExInsReflectBulletsFromLasers(Enemy *enemy, EclRawInstr *instr);
void ExInsShootBulletsAlongLaser(Enemy *enemy, EclRawInstr *instr);
void ExInsEffect1eAccel(Enemy *enemy, EclRawInstr *instr);
void ExInsYoumuSetGameSpeed(Enemy *enemy, EclRawInstr *instr);
void ExInsYoumuRestoreGameSpeed(Enemy *enemy, EclRawInstr *instr);
void ExInsBurstLargeBullets(Enemy *enemy, EclRawInstr *instr);
void ExInsYoumuCurveBulletsBelow(Enemy *enemy, EclRawInstr *instr);
void ExInsYoumuRedirectBulletsToPlayer(Enemy *enemy, EclRawInstr *instr);
void ExInsFlashScreen(Enemy *enemy, EclRawInstr *instr);
void ExInsYuyukoTransformButterflyBullets(Enemy *enemy, EclRawInstr *instr);
void ExInsYuyukoButterflySpawnEnemy(Enemy *enemy, EclRawInstr *instr);
void ExInsYuyukoCountButterflyBullets(Enemy *enemy, EclRawInstr *instr);
void ExInsYuyukoFadeOutMusic(Enemy *enemy, EclRawInstr *instr);
void ExInsYuyukoPlayResurrectionButterflyBgm(Enemy *enemy, EclRawInstr *instr);
void ExInsBurstLargeBullets2(Enemy *enemy, EclRawInstr *instr);
void ExInsSpawnBulletsWithDirChange(Enemy *enemy, EclRawInstr *instr);
void ExInsSpawnBulletsWithDirChange2(Enemy *enemy, EclRawInstr *instr);
} // namespace EnemyEclInstr

i32 IsPointInRotatedRect(D3DXVECTOR3 *param_1, D3DXVECTOR3 *param_2,
                         D3DXVECTOR3 *param_3, D3DXVECTOR3 *param_4,
                         f32 param_5, f32 param_6);
void ModifyEffect1eAcceleration();
