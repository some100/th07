#pragma once

#include "AnmManager.hpp"
#include "BulletManager.hpp"
#include "EclManager.hpp"
#include "EffectManager.hpp"
#include "inttypes.hpp"

extern u32 g_SpellcardScore[141];

struct EnemyHistory
{
    D3DXVECTOR3 position;
    D3DXVECTOR3 axisSpeed;
    f32 angle;
};

struct EnemyEclContext
{
    EnemyEclContext();

    EclRawInstr *curInstr;
    ZunTimer time;
    EclExInstr func;
    EclRawInstr *eclExInstr;
    EclContextArgs eclContextArgs;
    ZunTimer timer2;
    EclInterp interps[8];
    i32 compareRegister;
    i32 isPeriodicSub;
    i16 subId;
    // pad 2
};
C_ASSERT(sizeof(EnemyEclContext) == 0x218);

struct SpellcardInfo
{
    u32 isCapturing;
    u32 isActive;
    u32 captureScore;
    i32 grazeBonusScore;
    i32 scoreDrainRate;
    u32 spellcardIdx;
    u32 usedBomb;
};

struct Enemy
{
    Enemy();

    void CheckBulletPlayerCollision(D3DXVECTOR3 *bulletCenter,
                                    D3DXVECTOR3 *bulletSize);
    void ClampPos();
    void Despawn();
    i32 HandleLifeCallback();
    i32 HandleTimerCallback();
    void Move();
    void ResetEffectArray();
    void UpdateEffects();

    AnmVm primaryVm;
    AnmVm vms[2];
    EnemyEclContext currentContext;
    EnemyEclContext savedContextStack[16];
    i32 stackDepth;
    i32 unused_2a80;
    i32 deathCallbackSub;
    i32 interrupts[32];
    i32 runInterrupt;
    D3DXVECTOR3 position;
    D3DXVECTOR3 axisSpeed;
    D3DXVECTOR3 prevPos;
    D3DXVECTOR3 finalPos;
    D3DXVECTOR3 hitboxSize;
    D3DXVECTOR3 grazeSize;
    f32 angle;
    f32 angularVelocity;
    f32 moveAngle;
    f32 moveAngularVelocity;
    f32 moveSpeed;
    f32 moveAcceleration;
    f32 moveRadius;
    f32 moveRadialVelocity;
    D3DXVECTOR3 shootOffset;
    D3DXVECTOR3 moveInterp;
    D3DXVECTOR3 moveInterpStartPos;
    ZunTimer moveInterpTimer;
    i32 moveInterpStartTime;
    f32 bulletRankSpeedLow;
    f32 bulletRankSpeedHigh;
    i16 bulletRankAmount1Low;
    i16 bulletRankAmount1High;
    i16 bulletRankAmount2Low;
    i16 bulletRankAmount2High;
    i32 life;
    i32 maxLife;
    i32 score;
    ZunTimer timer;
    union ZunColor color;
    EnemyBulletShooter bulletProps;
    i32 shootInterval;
    ZunTimer shootIntervalTimer;
    EnemyLaserShooter laserProps;
    i32 unused_2d88;
    Laser *lasers[32];
    i32 laserIdx;
    i32 itemDrop;
    i8 deathAnm1;
    i8 deathAnm2;
    i8 deathAnm3;
    u8 bossId;
    u8 damageTintTimer;
    // pad 3
    ZunTimer unused_2e1c;
    i8 flags1;
    i8 flags2;
    i8 flags3;
    i8 flags4;
    i16 spellcardDelayTimer;
    u8 anmExFlags;
    u8 zLayer;
    i16 anmExDefaults;
    i16 anmExFarLeft;
    i16 anmExFarRight;
    i16 anmExLeft;
    i16 anmExRight;
    i16 unused_2e3a;
    D3DXVECTOR2 lowerMoveLimit;
    D3DXVECTOR2 upperMoveLimit;
    i32 lastDamage;
    Effect *effects[24];
    Effect *specialEffect;
    i32 effectsNum;
    f32 effectDistance;
    i32 lifeCallbackThreshold[4];
    i32 lifeCallbackSub[4];
    i32 timerCallbackThreshold;
    i32 timerCallbackSub;
    i32 periodicCallbackSub;
    EclContextArgs savedEclContextArgs;
    ZunTimer periodicTimer;
    ZunTimer periodicCounter;
    f32 unused_2f68;
    ZunTimer unused_2f6c;
    EnemyHistory enemyHistory[96];
    VertexTex1DiffuseXyzrwh trailVertices[194];
    u8 trailFlags;
    // pad 1
    i16 trailCount;
    i16 trailInterval;
    i16 trailNodeStep;
    ZunTimer invincibilityTimer;
    Enemy *next;
};
C_ASSERT(sizeof(Enemy) == 0x4f48);

struct EnemyManager
{
    EnemyManager();

    static ZunResult RegisterChain(const char *stgEnm1, const char *stgEnm2);
    static void CutChain();

    static ZunResult AddedCallback(EnemyManager *arg);
    static ZunResult DeletedCallback(EnemyManager *arg);
    static u32 OnUpdate(EnemyManager *arg);
    static u32 OnDraw1(EnemyManager *arg);
    static u32 OnDraw2(EnemyManager *arg);

    static u32 ActualOnDraw(EnemyManager *arg, i32 param_2, i32 param_3);
    void Initialize();

    i32 HasActiveBoss();
    i32 RemoveAllEnemies(i32 scoreMax, i32 scoreMin);
    static void RunEclTimeline(EclTimeline *timeline);
    Enemy *SpawnEnemy(i16 eclSubId, D3DXVECTOR3 *pos, i32 life, char itemDrop,
                      i32 score, u8 param_6);
    Enemy *SpawnEnemyEx(i32 eclSubId, D3DXVECTOR3 *pos, i32 life, i32 itemDrop,
                        i32 score, EclContextArgs *args);

    const char *stgEnmAnmFilename;
    const char *stgEnm2AnmFilename;
    Enemy enemyTemplate;
    Enemy enemies[481];
    Enemy *bosses[8];
    u16 randomItemSpawnIdx;
    u16 randomItemTableIdx;
    i32 enemyCountReal;
    i32 unused_9545c0;
    SpellcardInfo spellcardInfo;
    ZunTimer timer;
    i32 unused_9545ec;
    i32 unused_9545f0;
    EclTimeline timelines[16];
    ZunTimer timelineTime;
    Enemy *enemyHead[4];
};
C_ASSERT(sizeof(EnemyManager) == 0x954710);
extern EnemyManager g_EnemyManager;
