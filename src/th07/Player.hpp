#pragma once

#include "AnmVm.hpp"
#include "Chain.hpp"
#include "EffectManager.hpp"
#include "GameManager.hpp"
#include "inttypes.hpp"

extern const char *g_ShooterTable[6];
extern const char *g_ShooterTableFocus[6];

typedef void (*BombCallback)(struct Player *);

typedef enum PlayerState
{
    PLAYER_STATE_ALIVE = 0,
    PLAYER_STATE_SPAWNING = 1,
    PLAYER_STATE_DEAD = 2,
    PLAYER_STATE_INVULNERABLE = 3,
    PLAYER_STATE_BORDER = 4
} PlayerState;

typedef enum PlayerDirection
{
    MOVEMENT_NONE = 0,
    MOVEMENT_UP = 1,
    MOVEMENT_DOWN = 2,
    MOVEMENT_LEFT = 3,
    MOVEMENT_RIGHT = 4,
    MOVEMENT_UP_LEFT = 5,
    MOVEMENT_UP_RIGHT = 6,
    MOVEMENT_DOWN_LEFT = 7,
    MOVEMENT_DOWN_RIGHT = 8
} PlayerDirection;

typedef enum OptionState
{
    OPTION_HIDDEN = 0,
    OPTION_UNFOCUSED = 1,
    OPTION_FOCUSING = 2,
    OPTION_FOCUSED = 3,
    OPTION_UNFOCUSING = 4
} OptionState;

typedef enum BorderState
{
    BORDER_NONE = 0,
    BORDER_ACTIVE = 1,
    BORDER_READY = 2
} BorderState;

struct BombProjectile
{
    Float3 pos;
    Float3 size;
    i32 lifetime;
    union {
        i32 itemType;
        i32 damage;
    };
};
C_ASSERT(sizeof(BombProjectile) == 0x20);

struct BombClearBox
{
    PodFloat3 pos;
    PodFloat3 size;
    i32 lifetime;
    union {
        i32 itemType;
        i32 damage;
    };
};
C_ASSERT(sizeof(BombClearBox) == 0x20);

struct PlayerBombSubInfo
{
    i32 state;
    i32 counter;
    f32 accel;
    f32 speed;
    f32 angle;
    Float3 bombRegionPositions;
    Float3 bombRegionPositionsTrails[32];
    Float3 bombRegionVelocities;
    Float3 bombRegionAcceleration;
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
    Float3 pos;
    Float3 posHistory[16];
    Float3 hitboxSize;
    Float2 velocity;
    Float2 offset;
    f32 speed;
    f32 angle;
    ZunTimer timer;
    i16 damage;
    i16 bulletState;
    i16 bulletState2;
    i16 timerIdx;
    i16 optionId;
    i16 trailLength;
    i32 (*updateCallback)(struct Player *, struct PlayerBullet *);
    i32 (*drawCallback)(struct Player *, struct PlayerBullet *);
    i32 (*hitCallback)(struct Player *, struct PlayerBullet *, Float3 *);
    struct ShtEntry *shtEntry;
};
C_ASSERT(sizeof(PlayerBullet) == 0x364);

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
    f32 AngleToPlayer(Float3 *pos);
    void BreakBorder(u32 unused);
    void BreakBorderNaturally();

    i32 CalcItemBoxCollision(Float3 *center, Float3 *size);
    i32 CalcKillboxCollision(Float3 *center, Float3 *size);
    i32 CalcLaserHitbox(Float3 *center, Float3 *size,
                        Float3 *origin, f32 rotation, i32 canGraze);
    i32 CheckBombGraze(Float3 *center, Float3 *size);
    i32 CalcDamageToEnemy(Float3 *param_1, Float3 *param_2,
                          i32 *param_3);
    i32 CheckGraze(Float3 *center, Float3 *size);

    void Die();
    i32 HandlePlayerInputs();
    void Respawn();
    void ScoreGraze(Float3 *param_1);
    BombClearBox *SpawnBombEffect(Float3 *pos, f32 sizeY, f32 sizeZ,
                                    i32 lifetime, i32 itemType);
    BombClearBox *SpawnBombProjectile(Float3 *centerPosition, f32 posZ,
                                        f32 size, i32 itemType);
    static void SpawnBullets(Player *player, u32 timer);
    void StartFireBulletTimer();

    void SetToTopLeftPos(AnmVm *vm)
    {
        vm->pos[0] += g_GameManager.arcadeRegionTopLeftPos.x;
        vm->pos[1] += g_GameManager.arcadeRegionTopLeftPos.y;
        vm->pos[2] = 0.0f;
    }

    ZunTimer *GetBombTimer()
    {
        ZunTimer *timer = &this->bombInfo.bombTimer;
        return timer;
    }

    static void SetVecCorners(Float3 *topLeft, Float3 *bottomRight, Float3 *center, Float3 *size)
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

    AnmVm playerSprite;
    AnmVm optionsSprite[3];
    Float3 positionCenter;
    Float3 prevFramePos;
    Float3 hitboxTopLeft;
    Float3 hitboxBottomRight;
    Float3 grazeTopLeft;
    Float3 grazeBottomRight;
    Float3 grabItemTopLeft;
    Float3 grabItemBottomRight;
    Float3 hitboxSize;
    Float3 grazeSize;
    Float3 grabItemSize;
    Float3 optionsPosition[2];
    Float2 velocity;
    i32 unused_9d4;
    Effect *focusEffect;
    BombProjectile bombDamageBoxes[112];
    BombClearBox bombClearBoxes[96];
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
    Float3 positionOfLastEnemyHit;
    Float3 sakuyaTargetPosition;
    i32 targetingEnemy;
    PlayerBullet bullets[96];
    PlayerBulletTimer timers[3];
    ZunTimer fireBulletTimer;
    ZunTimer invulnerabilityTimer;
    ZunTimer borderTimer;
    i32 unused_16a18;
    i32 unused_16a1c;
    PlayerBombInfo bombInfo;
    Float3 bombStartPos;
    f32 optionAngle;
    ChainElem *calcChain;
    ChainElem *drawChain1;
    ChainElem *drawChain2;
    Effect *effect;
    Effect *borderEffect;
    struct ShtData *shooterData;
    struct ShtData *shooterDataFocus;
};
C_ASSERT(sizeof(Player) == 0xb7e78);
extern Player g_Player;

typedef i32 (*ShtFunc1)(Player *, PlayerBullet *, i32, struct ShtEntry *);
extern ShtFunc1 g_ShtFireFuncs[6];
typedef i32 (*ShtFunc2)(Player *, PlayerBullet *);
extern ShtFunc2 g_ShtUpdateFuncs[6];
typedef i32 (*ShtFunc3)(Player *, PlayerBullet *);
extern ShtFunc3 g_ShtDrawFuncs[2];
typedef i32 (*ShtFunc4)(Player *, PlayerBullet *, Float3 *);
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
    i32 (*hitCallback)(Player *, PlayerBullet *, Float3 *);
};

struct ShtLevel
{
    ShtEntry *entry;
    i32 requiredPower;
};

struct ShtData
{
    static ZunResult LoadShtData(ShtData **data, const char *shtPath);
    static i32 FireBulletDefault(Player *player, PlayerBullet *bullet,
                                 i32 fireTime, ShtEntry *shtEntry);
    static i32 FireOrbBulletUnfocused(Player *player, PlayerBullet *bullet,
                                      i32 fireTime, ShtEntry *shtEntry);
    static i32 FireOrbBulletFocused(Player *player, PlayerBullet *bullet,
                                    i32 fireTime, ShtEntry *shtEntry);
    static i32 FireHomingBullet(Player *player, PlayerBullet *bullet,
                                i32 fireTime, ShtEntry *shtEntry);
    static i32 FireRotatingOrbBullet(Player *player, PlayerBullet *bullet,
                                     i32 fireTime, ShtEntry *shtEntry);

    static i32 UpdateHomingBullet(Player *player, PlayerBullet *bullet);
    static i32 UpdateHomingBulletFocused(Player *player, PlayerBullet *bullet);
    static i32 UpdateUpwardAcceleratingBullet(Player *player,
                                              PlayerBullet *bullet);
    static i32 UpdateOrbLaser(Player *player, PlayerBullet *bullet);
    static i32 UpdatePlayerLaser(Player *player, PlayerBullet *bullet);

    static i32 DrawBulletWithTrail(Player *player, PlayerBullet *bullet);

    static i32 OnMissileHit(Player *player, PlayerBullet *bullet,
                            Float3 *pos);
    static i32 SpawnHitParticles(Player *player, PlayerBullet *bullet,
                                 Float3 *pos);

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
    ShtLevel levels;
};
C_ASSERT(sizeof(ShtData) == 0x3c);
