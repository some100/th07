#pragma once

#include "AnmVm.hpp"
#include "Chain.hpp"
#include "EffectManager.hpp"
#include "GameManager.hpp"
#include "inttypes.hpp"

extern const char *g_ShooterTable[6];
extern const char *g_ShooterTableFocus[6];

typedef void (*BombCallback)(struct Player *);

enum PlayerState
{
    PLAYER_STATE_ALIVE,
    PLAYER_STATE_SPAWNING,
    PLAYER_STATE_DEAD,
    PLAYER_STATE_INVULNERABLE,
    PLAYER_STATE_BORDER,
};

enum PlayerDirection
{
    MOVEMENT_NONE,
    MOVEMENT_UP,
    MOVEMENT_DOWN,
    MOVEMENT_LEFT,
    MOVEMENT_RIGHT,
    MOVEMENT_UP_LEFT,
    MOVEMENT_UP_RIGHT,
    MOVEMENT_DOWN_LEFT,
    MOVEMENT_DOWN_RIGHT
};

enum OptionState
{
    OPTION_HIDDEN,
    OPTION_UNFOCUSED,
    OPTION_FOCUSING,
    OPTION_FOCUSED,
    OPTION_UNFOCUSING,
};

enum BorderState
{
    BORDER_NONE,
    BORDER_ACTIVE,
    BORDER_READY,
};

struct BombProjectile
{
    ZunVec3 pos;
    ZunVec3 size;
    i32 lifetime;
    union {
        i32 itemType;
        i32 damage;
    };
};

struct BombClearBox
{
    ZunVec3 pos;
    ZunVec3 size;
    i32 lifetime;
    union {
        i32 itemType;
        i32 damage;
    };
};

struct CachedBombClearBox
{
    bool isBox;
    f32 minX, maxX;
    f32 minY, maxY;
    f32 cx, cy;
    f32 radiusSq;
    i32 itemType;
};

struct PlayerBombSubInfo
{
    i32 state;
    i32 counter;
    f32 accel;
    f32 prevAccel;
    f32 speed;
    f32 angle;
    ZunVec3 bombRegionPositions;
    ZunVec3 prevBombRegionPositions;
    ZunVec3 bombRegionPositionsTrails[32];
    ZunVec3 bombRegionVelocities;
    ZunVec3 bombRegionAcceleration;
    AnmVm vms[8];
    Effect *effect;
    ZunTimer timer;
};

struct PlayerBombInfo
{
    static void SubtractCherryDrain(i32 cherryDrain)
    {
        if (g_GameManager.cherry - g_GameManager.globals->cherryStart >= cherryDrain)
        {
            g_GameManager.cherry -= cherryDrain;
        }
        else
        {
            g_GameManager.cherry = g_GameManager.globals->cherryStart;
        }
    }

    i32 isInUse;
    i32 isFocus;
    i32 bombDuration;
    i32 cherryDrain;
    ZunTimer bombTimer;
    BombCallback bombCalc;
    BombCallback draw;
    BombCallback bombFocusCalc;
    BombCallback drawFocus;
    PlayerBombSubInfo subInfo[128];
};

struct PlayerBullet
{
    f32 *GetPosX()
    {
        return &this->pos.x;
    }

    f32 *GetPosY()
    {
        return &this->pos.y;
    }

    f32 *GetVmPosX()
    {
        return &this->vm.pos.x;
    }

    f32 *GetVmPosY()
    {
        return &this->vm.pos.y;
    }

    AnmVm vm;
    ZunVec3 pos;
    ZunVec3 prevPos;
    ZunVec3 posHistory[16];
    ZunVec3 hitboxSize;
    Float2 velocity;
    Float2 offset;
    f32 speed;
    f32 angle;
    f32 prevAngle;
    ZunTimer timer;
    i16 damage;
    i16 bulletState;
    i16 bulletState2;
    i16 timerIdx;
    i16 optionId;
    i16 trailLength;
    i32 (*updateCallback)(struct Player *, struct PlayerBullet *);
    i32 (*drawCallback)(struct Player *, struct PlayerBullet *);
    i32 (*hitCallback)(struct Player *, struct PlayerBullet *, ZunVec3 *);
    struct ShtEntry *shtEntry;
};

struct PlayerBulletTimer
{
    ZunTimer timer;
    PlayerBullet *bullet;
};

struct Player
{
    static ZunResult RegisterChain(u32 param_1);
    static void CutChain();

    static ZunResult AddedCallback(Player *arg);
    static ZunResult DeletedCallback(Player *arg);
    static u32 OnUpdate(Player *arg);
    static u32 OnDrawHighPrio(Player *arg);
    static u32 OnDrawLowPrio(Player *arg);

    void UpdateBombProjectiles();
    void UpdateBorderAndBombState();
    i32 UpdateDeath();
    void UpdateState();
    void UpdateShots();
    i32 UpdateFireBulletTimer();
    void UpdateUI();

    void DrawBullets();
    void DrawBulletExplosions();

    void ActivateBorder();
    f32 AngleToPlayer(ZunVec3 *pos);
    void BreakBorder();
    void BreakBorderNaturally();

    i32 CalcItemBoxCollision(ZunVec3 *center, ZunVec3 *size);
    i32 CalcKillboxCollision(ZunVec3 *center, ZunVec3 *size);
    i32 CalcLaserHitbox(ZunVec3 *center, ZunVec3 *size, ZunVec3 *origin, f32 rotation,
                        i32 canGraze);
    i32 CheckBombGraze(ZunVec3 *center, ZunVec3 *size);
    i32 CalcDamageToEnemy(ZunVec3 *param_1, ZunVec3 *param_2, i32 *param_3);
    i32 CheckGraze(ZunVec3 *center, ZunVec3 *size);

    void Die();
    i32 HandlePlayerInputs();
    void Respawn();
    void ScoreGraze(ZunVec3 *param_1);
    BombClearBox *SpawnBombEffect(ZunVec3 *pos, f32 sizeY, f32 sizeZ, i32 lifetime, i32 itemType);
    BombClearBox *SpawnBombProjectile(ZunVec3 *centerPosition, f32 posZ, f32 size, i32 itemType);
    static void SpawnBullets(Player *player, u32 timer);
    void StartFireBulletTimer();

    void RebuildBombBoxCache();

    void SetToTopLeftPos(AnmVm *vm)
    {
        vm->pos.x += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos.y += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos.z = 0.0f;
    }

    ZunTimer *GetBombTimer()
    {
        ZunTimer *timer = &this->bombInfo.bombTimer;
        return timer;
    }

    static void SetVecCorners(ZunVec3 *topLeft, ZunVec3 *bottomRight, ZunVec3 *center,
                              ZunVec3 *size)
    {
        topLeft->x = center->x - size->x * 0.5f;
        topLeft->y = center->y - size->y * 0.5f;
        bottomRight->x = center->x + size->x * 0.5f;
        bottomRight->y = center->y + size->y * 0.5f;
    }

    f32 *GetPosCenterX()
    {
        return &this->positionCenter.x;
    }

    f32 *GetPosCenterY()
    {
        return &this->positionCenter.y;
    }

    void SetFocusEffect(Effect *effect)
    {
        this->focusEffect = effect;
    }

    void UpdatePrev()
    {
        this->playerSprite.UpdatePrev();
        this->optionsSprite[0].UpdatePrev();
        this->optionsSprite[1].UpdatePrev();
        for (i32 i = 0; i < 96; i++)
        {
            if (this->bullets[i].bulletState != 0)
            {
                this->bullets[i].vm.UpdatePrev();
            }
        }
        if (this->bombInfo.isInUse)
        {
            for (i32 i = 0; i < 128; i++)
            {
                this->bombInfo.subInfo[i].prevBombRegionPositions =
                    this->bombInfo.subInfo[i].bombRegionPositions;
                this->bombInfo.subInfo[i].prevAccel = this->bombInfo.subInfo[i].accel;
                for (i32 j = 0; j < 8; j++)
                {
                    this->bombInfo.subInfo[i].vms[j].UpdatePrev();
                }
            }
        }
    }

    AnmVm playerSprite;
    AnmVm optionsSprite[3];
    ZunVec3 positionCenter;
    ZunVec3 prevPositionCenter;
    ZunVec3 prevFramePos;
    ZunVec3 hitboxTopLeft;
    ZunVec3 hitboxBottomRight;
    ZunVec3 grazeTopLeft;
    ZunVec3 grazeBottomRight;
    ZunVec3 grabItemTopLeft;
    ZunVec3 grabItemBottomRight;
    ZunVec3 hitboxSize;
    ZunVec3 grazeSize;
    ZunVec3 grabItemSize;
    ZunVec3 optionsPosition[2];
    ZunVec3 prevOptionsPosition[2];
    Float2 velocity;
    i32 unused_9d4;
    Effect *focusEffect;
    BombProjectile bombDamageBoxes[112];
    BombClearBox bombClearBoxes[96];
    CachedBombClearBox activeBombClearBoxesCache[96];
    i32 numActiveBombClearBoxes;
    bool dirtyBombBoxes;
    i32 isBombing;
    ShtEntry *shtEntries[4];
    f32 horizontalMovementSpeedMultiplierDuringBomb;
    f32 verticalMovementSpeedMultiplierDuringBomb;
    i32 respawnTimer;
    i32 borderInvulnerabilityTime;
    i32 bulletGracePeriod;
    i32 itemType;
    i8 playerState;
    u8 initParam;
    i8 optionState;
    i8 isFocus;
    u8 bombParticleTime;
    i8 hasBorder;
    // pad 2
    ZunTimer focusMovementTimer;
    PlayerDirection playerDirection;
    f32 previousHorizontalSpeed;
    f32 previousVerticalSpeed;
    ZunVec3 positionOfLastEnemyHit;
    ZunVec3 sakuyaTargetPosition;
    i32 targetingEnemy;
    PlayerBullet bullets[96];
    PlayerBulletTimer timers[3];
    ZunTimer fireBulletTimer;
    ZunTimer invulnerabilityTimer;
    ZunTimer borderTimer;
    i32 unused_16a18;
    i32 unused_16a1c;
    PlayerBombInfo bombInfo;
    ZunVec3 bombStartPos;
    f32 optionAngle;
    ChainElem *calcChain;
    ChainElem *drawChain1;
    ChainElem *drawChain2;
    Effect *effect;
    Effect *borderEffect;
    struct ShtData *shooterData;
    struct ShtData *shooterDataFocus;
};

extern Player g_Player;

typedef i32 (*ShtFunc1)(Player *, PlayerBullet *, i32, struct ShtEntry *);
extern ShtFunc1 g_ShtFireFuncs[6];
typedef i32 (*ShtFunc2)(Player *, PlayerBullet *);
extern ShtFunc2 g_ShtUpdateFuncs[6];
typedef i32 (*ShtFunc3)(Player *, PlayerBullet *);
extern ShtFunc3 g_ShtDrawFuncs[2];
typedef i32 (*ShtFunc4)(Player *, PlayerBullet *, ZunVec3 *);
extern ShtFunc4 g_ShtHitFuncs[4];

struct ShtEntry
{
    i16 fireInterval;
    i16 fireOffset;
    Float2 offset;
    Float2 hitboxSize;
    f32 angle;
    f32 speed;
    i16 damage;
    i8 option;
    i8 bulletState2;
    i16 anmFileIdx;
    i16 soundIdx;
    i32 (*fireCallback)(Player *, PlayerBullet *, i32, struct ShtEntry *);
    i32 (*updateCallback)(Player *, PlayerBullet *);
    i32 (*drawCallback)(Player *, PlayerBullet *);
    i32 (*hitCallback)(Player *, PlayerBullet *, ZunVec3 *);
};

struct ShtLevel
{
    ShtEntry *entry;
    i32 requiredPower;
};

struct ShtData
{
    static ZunResult LoadShtData(ShtData **data, const char *shtPath);
    static i32 FireBulletDefault(Player *player, PlayerBullet *bullet, i32 fireTime,
                                 ShtEntry *shtEntry);
    static i32 FireOrbBulletUnfocused(Player *player, PlayerBullet *bullet, i32 fireTime,
                                      ShtEntry *shtEntry);
    static i32 FireOrbBulletFocused(Player *player, PlayerBullet *bullet, i32 fireTime,
                                    ShtEntry *shtEntry);
    static i32 FireHomingBullet(Player *player, PlayerBullet *bullet, i32 fireTime,
                                ShtEntry *shtEntry);
    static i32 FireRotatingOrbBullet(Player *player, PlayerBullet *bullet, i32 fireTime,
                                     ShtEntry *shtEntry);

    static i32 UpdateHomingBullet(Player *player, PlayerBullet *bullet);
    static i32 UpdateHomingBulletFocused(Player *player, PlayerBullet *bullet);
    static i32 UpdateUpwardAcceleratingBullet(Player *player, PlayerBullet *bullet);
    static i32 UpdateOrbLaser(Player *player, PlayerBullet *bullet);
    static i32 UpdatePlayerLaser(Player *player, PlayerBullet *bullet);

    static i32 DrawBulletWithTrail(Player *player, PlayerBullet *bullet);

    static i32 OnMissileHit(Player *player, PlayerBullet *bullet, ZunVec3 *pos);
    static i32 SpawnHitParticles(Player *player, PlayerBullet *bullet, ZunVec3 *pos);

    i16 unused;
    u16 numLevels;
    f32 initialBombs;
    i32 initialRespawnTimer;
    f32 hitboxRadius;
    f32 grabItemRadius;
    f32 itemCollectSpeed;
    f32 itemCollectRadius;
    f32 cherryPenaltyMultiplier;
    f32 pocY;
    f32 speed;
    f32 speedFocus;
    f32 speedDiagonal;
    f32 speedDiagonalFocus;
    ShtLevel *levels;
    ShtEntry *entries;
};

struct ShtRawEntry
{
    i16 fireInterval;
    i16 fireOffset;
    Float2 offset;
    Float2 hitboxSize;
    f32 angle;
    f32 speed;
    i16 damage;
    i8 option;
    i8 bulletState2;
    i16 anmFileIdx;
    i16 soundIdx;
    u32 fireCallback;
    u32 updateCallback;
    u32 drawCallback;
    u32 hitCallback;
};
static_assert(sizeof(ShtRawEntry) == 0x34);

struct ShtRawLevel
{
    u32 entryOffset;
    i32 requiredPower;
};
static_assert(sizeof(ShtRawLevel) == 0x8);

struct ShtRawData
{
    i16 numLevels;
    u16 entryCount;
    f32 initialBombs;
    i32 initialRespawnTimer;
    f32 hitboxRadius;
    f32 grabItemRadius;
    f32 itemCollectSpeed;
    f32 itemCollectRadius;
    f32 cherryPenaltyMultiplier;
    f32 pocY;
    f32 speed;
    f32 speedFocus;
    f32 speedDiagonal;
    f32 speedDiagonalFocus;
    ShtRawLevel levels[];
};
static_assert(sizeof(ShtRawData) == 0x34);
