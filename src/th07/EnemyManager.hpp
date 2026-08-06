#pragma once

#include "AnmManager.hpp"
#include "BulletManager.hpp"
#include "EclManager.hpp"
#include "EffectManager.hpp"
#include "inttypes.hpp"

extern u32 g_SpellcardScore[141];

struct EnemyHistory
{
    Float3 pos;
    Float3 axisSpeed;
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
    i32 captureScore;
    i32 grazeBonusScore;
    i32 scoreDrainRate;
    i32 spellcardIdx;
    u32 usedBomb;
};

struct Enemy
{
    Enemy();

    void CheckBulletPlayerCollision(Float3 *bulletCenter,
                                    Float3 *bulletSize);
    void ClampPos();
    void Despawn();
    i32 HandleLifeCallback();
    i32 HandleTimerCallback();
    void Move();
    void ResetEffectArray();
    void UpdateEffects();

    static i32 BulletRankAmountInner(i32 low, i32 high, i32 scaleFactor)
    {
        return scaleFactor * (high - low) / 32 + low;
    }

    i32 BulletRankAmount1(i32 scaleFactor)
    {
        return BulletRankAmountInner(this->bulletRankAmount1Low, this->bulletRankAmount1High, scaleFactor);
    }

    i32 BulletRankAmount2(i32 scaleFactor)
    {
        return BulletRankAmountInner(this->bulletRankAmount2Low, this->bulletRankAmount2High, scaleFactor);
    }

    static f32 BulletRankSpeedInner(f32 low, f32 high, f32 scaleFactor)
    {
        return scaleFactor * (high - low) / 32 + low;
    }

    f32 BulletRankSpeed(f32 scaleFactor)
    {
        return Enemy::BulletRankSpeedInner(this->bulletRankSpeedLow, this->bulletRankSpeedHigh, scaleFactor);
    }

    static i32 ShootIntervalInner(i32 low, i32 high, i32 scaleFactor)
    {
        return scaleFactor * (high - low) / 32 + low;
    }

    i32 ShootInterval(i32 scaleFactor)
    {
        return Enemy::ShootIntervalInner(this->shootInterval / 5, -this->shootInterval / 5, scaleFactor);
    }

    AnmVm primaryVm;
    AnmVm vms[2];
    EnemyEclContext currentContext;
    EnemyEclContext savedContextStack[16];
    i32 stackDepth;
    i32 unused_2a80;
    i32 deathCallbackSub;
    i32 interrupts[32];
    i32 runInterrupt;
    Float3 pos;
    Float3 axisSpeed;
    Float3 prevPos;
    Float3 deltaPos;
    Float3 hitboxSize;
    Float3 grazeSize;
    f32 angle;
    f32 angularVelocity;
    f32 moveAngle;
    f32 moveAngularVelocity;
    f32 moveSpeed;
    f32 moveAcceleration;
    f32 moveRadius;
    f32 moveRadialVelocity;
    Float3 shootOffset;
    Float3 moveInterp;
    Float3 moveInterpStartPos;
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
    ZunColor color;
    EnemyBulletShooter bulletProps;
    i32 shootInterval;
    ZunTimer shootIntervalTimer;
    EnemyLaserShooter laserProps;
    Laser *lasers[32];
    i32 laserIdx;
    i32 itemDrop;
    i8 deathAnm1;
    u8 deathAnm2;
    i8 deathAnm3;
    u8 bossId;
    u8 damageTintTimer;
    // pad 3
    ZunTimer unused_2e1c;
    union {
        i8 flags1;
        struct
        {
            u8 moveMode : 2;
            u8 interpEasing : 3;
            u8 disableBullets : 1;
            u8 mirror : 1;
            u8 active : 1;
        };
    };
    union {
        i8 flags2;
        struct
        {
            u8 canDie : 1;
            u8 hasContactHitbox : 1;
            u8 canBeDamaged : 1;
            u8 hasNoCollision : 1;
            u8 isHittable : 1;
            u8 isProjectile : 1;
            u8 isBoss : 1;
            u8 hasMovementBounds : 1;
        };
    };
    union {
        i8 flags3;
        struct
        {
            u8 deathType : 3;
            u8 isInBounds : 1;
            u8 primaryVmAutoRotate : 1;
            u8 noStackRet : 1;
            u8 isSurvivalSpellcard : 1;
            u8 disableOOBDespawn : 1;
        };
    };
    union {
        i8 flags4;
        struct
        {
            u8 disableMovement : 1;
            u8 customSpecialEffectPos : 1;
            u8 invisibleOnBomb : 1;
            u8 freezeEclDuringBombs : 1;
        };
    };
    u16 spellcardDelayTimer;
    u8 anmExFlags;
    u8 zLayer;
    i16 anmExDefaults;
    i16 anmExFarLeft;
    i16 anmExFarRight;
    i16 anmExLeft;
    i16 anmExRight;
    i16 unused_2e3a;
    Float2 lowerMoveLimit;
    Float2 upperMoveLimit;
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
    VertexTex1DiffuseXyzrhw trailVertices[194];
    union {
        u8 trailFlags;
        struct
        {
            u8 enable : 1;
            u8 shrink : 1;
            u8 fade : 1;
            u8 useTriangleStrip : 1;
            u8 hidePrimarySprite : 1;
        };
    };
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
    Enemy *SpawnEnemy(i32 eclSubId, Float3 *pos, i32 life, i32 itemDrop,
                      i32 score, u8 param_6);
    Enemy *SpawnEnemyEx(i32 eclSubId, Float3 *pos, i32 life, i32 itemDrop,
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
