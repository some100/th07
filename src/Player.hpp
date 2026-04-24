#pragma once

#include "AnmVm.hpp"
#include "Chain.hpp"
#include "EffectManager.hpp"
#include "inttypes.hpp"

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

typedef enum OrbState
{
    ORB_HIDDEN = 0,
    ORB_UNFOCUSED = 1,
    ORB_FOCUSING = 2,
    ORB_FOCUSED = 3,
    ORB_UNFOCUSING = 4
} OrbState;

typedef enum BorderState
{
    BORDER_NONE = 0,
    BORDER_ACTIVE = 1,
    BORDER_READY = 2
} BorderState;

struct BombProjectile
{
    D3DXVECTOR3 pos;
    D3DXVECTOR3 size;
    i32 lifetime;
    i32 payload;
};
C_ASSERT(sizeof(BombProjectile) == 0x20);

struct PlayerBombSubInfo
{
    PlayerBombSubInfo();

    i32 state;
    i32 counter;
    f32 accel;
    f32 speed;
    f32 angle;
    D3DXVECTOR3 bombRegionPositions;
    D3DXVECTOR3 bombRegionPositionsTrails[32];
    D3DXVECTOR3 bombRegionVelocities;
    D3DXVECTOR3 bombRegionAcceleration;
    AnmVm vms[8];
    Effect *effect;
    ZunTimer timer;
};

struct PlayerBombInfo
{
    PlayerBombInfo();

    i32 isInUse;
    i32 isFocus;
    i32 bombDuration;
    i32 bombCherryDrain;
    struct ZunTimer bombTimer;
    BombCallback bombCalc;
    BombCallback draw;
    BombCallback bombFocusCalc;
    BombCallback drawFocus;
    PlayerBombSubInfo subInfo[128];
};

struct PlayerBullet
{
    PlayerBullet();

    AnmVm vm;
    D3DXVECTOR3 pos;
    D3DXVECTOR3 posHistory[16];
    D3DXVECTOR3 hitboxSize;
    D3DXVECTOR2 velocity;
    D3DXVECTOR2 offset;
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
    i32 (*hitCallback)(struct Player *, struct PlayerBullet *, D3DXVECTOR3 *);
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
    Player();

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
    void UpdateFireBulletTimer();
    void UpdateUI();

    void DrawBullets();
    void DrawBulletExplosions();

    void ActivateBorder();
    f32 AngleToPlayer(D3DXVECTOR3 *pos);
    void BreakBorder(u32 unused);
    void BreakBorderNaturally();

    i32 CalcItemBoxCollision(D3DXVECTOR3 *center, D3DXVECTOR3 *size);
    i32 CalcKillboxCollision(D3DXVECTOR3 *center, D3DXVECTOR3 *size);
    i32 CalcLaserHitbox(D3DXVECTOR3 *param_1, D3DXVECTOR3 *param_2,
                        D3DXVECTOR3 *param_3, f32 param_4, i32 canGraze);
    i32 CheckBombGraze(D3DXVECTOR3 *center, D3DXVECTOR3 *size);
    i32 CheckCollisionWithEnemy(D3DXVECTOR3 *param_1, D3DXVECTOR3 *param_2,
                                i32 *param_3);
    i32 CheckGraze(D3DXVECTOR3 *center, D3DXVECTOR3 *size);

    void Die();
    void HandlePlayerInputs();
    void Respawn();
    void ScoreGraze(D3DXVECTOR3 *param_1);
    BombProjectile *SpawnBombEffect(D3DXVECTOR3 *pos, f32 sizeY, f32 sizeZ,
                                    i32 lifetime, i32 payload);
    BombProjectile *SpawnBombProjectile(D3DXVECTOR3 *centerPosition, f32 posZ,
                                        f32 size, i32 payload);
    static void SpawnBullets(Player *player, u32 timer);
    void StartFireBulletTimer();

    AnmVm playerSprite;
    AnmVm orbsSprite[3];
    D3DXVECTOR3 positionCenter;
    D3DXVECTOR3 prevFramePos;
    D3DXVECTOR3 hitboxTopLeft;
    D3DXVECTOR3 hitboxBottomRight;
    D3DXVECTOR3 grazeTopLeft;
    D3DXVECTOR3 grazeBottomRight;
    D3DXVECTOR3 grabItemTopLeft;
    D3DXVECTOR3 grabItemBottomRight;
    D3DXVECTOR3 hitboxSize;
    D3DXVECTOR3 grazeSize;
    D3DXVECTOR3 grabItemSize;
    D3DXVECTOR3 orbsPosition[2];
    D3DXVECTOR2 velocity;
    i32 unused_9d4;
    Effect *focusEffect;
    BombProjectile bombProjectiles[112];
    BombProjectile bombHitboxes[96];
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
    i8 orbState;
    i8 isFocus;
    u8 bombParticleTime;
    i8 hasBorder;
    // pad 2
    ZunTimer focusMovementTimer;
    PlayerDirection playerDirection;
    f32 previousHorizontalSpeed;
    f32 previousVerticalSpeed;
    D3DXVECTOR3 positionOfLastEnemyHit;
    D3DXVECTOR3 sakuyaTargetPosition;
    i32 targetingEnemy;
    PlayerBullet bullets[96];
    PlayerBulletTimer timers[3];
    ZunTimer fireBulletTimer;
    ZunTimer invulnerabilityTimer;
    ZunTimer borderTimer;
    i32 unused_16a18;
    i32 unused_16a1c;
    PlayerBombInfo bombInfo;
    D3DXVECTOR3 bombStartPos;
    f32 orbAngle;
    ChainElem *calcChain;
    ChainElem *drawChain1;
    ChainElem *drawChain2;
    Effect *effect;
    Effect *borderEffect;
    struct ShtData *shooterData;
    struct ShtData *shooterData2;
};
C_ASSERT(sizeof(Player) == 0xb7e78);

typedef i32 (*ShtFunc1)(Player *, PlayerBullet *, i32, struct ShtEntry *);
typedef i32 (*ShtFunc2)(Player *, PlayerBullet *);
typedef i32 (*ShtFunc3)(Player *, PlayerBullet *);
typedef i32 (*ShtFunc4)(Player *, PlayerBullet *, D3DXVECTOR3 *);

struct ShtEntry
{
    i16 fireInterval;
    i16 fireOffset;
    D3DXVECTOR2 offset;
    D3DXVECTOR2 hitboxSize;
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
    i32 (*hitCallback)(Player *, PlayerBullet *, D3DXVECTOR3 *);
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
                            D3DXVECTOR3 *pos);
    static i32 SpawnHitParticles(Player *player, PlayerBullet *bullet,
                                 D3DXVECTOR3 *pos);

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

extern Player g_Player;
extern ShtFunc1 g_ShtFireFuncs[6];
extern ShtFunc2 g_ShtUpdateFuncs[6];
extern ShtFunc3 g_ShtDrawFuncs[2];
extern ShtFunc4 g_ShtHitFuncs[4];
extern const char *g_ShooterTable[6];
extern const char *g_ShooterTable2[6];
