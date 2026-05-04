#pragma once

#include "ItemManager.hpp"
#include "ZunResult.hpp"

extern D3DCOLOR *g_BulletColor;

typedef enum BulletState
{
    BULLET_INACTIVE = 0,
    BULLET_NORMAL = 1,
    BULLET_SPAWNING_FAST = 2,
    BULLET_SPAWNING_NORMAL = 3,
    BULLET_SPAWNING_SLOW = 4,
    BULLET_DESPAWN = 5,
    BULLET_END_ARRAY = 6
} BulletState;

struct BulletTypeInfo
{
    i32 anmFileIdx;
    i32 spawnFastIdx;
    i32 spawnNormalIdx;
    i32 spawnSlowIdx;
    i32 spawnDonutIdx;
};

struct BulletTypeSprites
{
    AnmVm spriteBullet;
    AnmVm spriteSpawnEffectFast;
    AnmVm spriteSpawnEffectNormal;
    AnmVm spriteSpawnEffectSlow;
    AnmVm spriteSpawnEffectDonut;
    D3DXVECTOR3 grazeSize;
    u8 unused_b88;
    u8 bulletHeight;
    u8 collisionType;
    // pad 1
};
C_ASSERT(sizeof(BulletTypeSprites) == 0xb8c);

struct BulletCommand
{
    f32 speed;
    f32 angle;
    i32 duration;
    i32 loopCount;
    u32 type;
    i32 flag;
};

struct BulletCommandState
{
    ZunTimer timer;
    f32 speed;
    f32 angle;
    D3DXVECTOR3 vec3;
    i32 duration;
    i32 maxTimes;
    i32 minTimes;
};

struct EnemyBulletShooter
{
    BulletCommand *AddCommand(i32 command, i32 flag, u32 type);
    void AddAngleAccelCommand(i32 command, i32 flag, i32 duration, f32 angle,
                              f32 speed);
    void AddDirChangeCommand(i32 command, i32 flag, i32 duration, i32 loopCount,
                             f32 speed, f32 angle);
    void AddSpawnDelayCommand(i32 command, i32 flag, i32 duration);
    void AddTargetVelocityCommand(i32 command, i32 flag, i32 duration, f32 speed,
                                  f32 angle);

    i16 sprite;
    i16 spriteOffset;
    D3DXVECTOR3 position;
    f32 angle1;
    f32 angle2;
    f32 speed1;
    f32 speed2;
    BulletCommand commands[6];
    i32 unused_b0[3];
    i16 count1;
    i16 count2;
    u16 aimMode;
    i16 unused_c2;
    u32 flags;
    i32 soundIdx;
    i32 soundOverride;
    BulletTypeSprites *sprites;
};
C_ASSERT(sizeof(EnemyBulletShooter) == 0xd4);

struct EnemyLaserShooter
{
    i16 sprite;
    i16 spriteOffset;
    D3DXVECTOR3 position;
    f32 angle1;
    f32 angle2;
    f32 speed1;
    f32 speed2;
    BulletCommand commands[5];
    f32 startOffset;
    f32 endOffset;
    f32 startLength;
    f32 width;
    i32 startTime;
    i32 duration;
    i32 endTime;
    i32 grazeDelay;
    i32 aimMode;
    i32 unused_bc;
    u16 type;
    i16 unused_c2;
    u32 flags;
    i32 unused_c8;
    i32 soundOverride;
};

struct Laser
{
    struct AnmVm vm0;
    struct AnmVm vm1;
    D3DXVECTOR3 pos;
    f32 angle;
    f32 startOffset;
    f32 endOffset;
    f32 startLength;
    f32 width;
    f32 targetWidth;
    f32 speed;
    i32 startTime;
    i32 grazeDelay;
    i32 duration;
    i32 endTime;
    i32 grazeInterval;
    i32 inUse;
    ZunTimer timer;
    u16 flags;
    i16 color;
    u8 state;
    u8 hideWarning;
    // pad 2
};
C_ASSERT(sizeof(Laser) == 0x4ec);

struct Bullet
{
    BulletCommand *AddCommand(i32 command, i32 flag, u32 type);
    void AddAngleAccelCommand(i32 command, i32 flag, i32 duration, f32 angle,
                              f32 speed);
    void AddTargetVelocityCommand(i32 command, i32 flag, i32 duration, f32 speed,
                                  f32 angle);
    void RunCommands();

    void UpdateBulletBurstSpeed();
    void UpdateBulletTargetVelocity();
    void UpdateBulletTargetAngle();
    void UpdateBulletDirChangeAndResume();
    void UpdateBulletDirChangeAbsoluteAndResume();
    void UpdateBulletDirChangeAimAtPlayer();
    void UpdateBulletBounce();

    void Draw();

    // FUNCTION: TH07 0x00417b20
    void Initialize()
    {
        this->state = BULLET_INACTIVE;
        this->timer1 = 0;
        this->timer2 = 0;
    }

    BulletTypeSprites sprites;
    D3DXVECTOR3 pos;
    D3DXVECTOR3 velocity;
    D3DXVECTOR3 unused_ba4;
    f32 speed;
    f32 acceleration;
    f32 angularVelocity;
    f32 angle;
    f32 unused_bc0;
    f32 unused_bc4;
    ZunTimer timer1;
    ZunTimer timer2;
    i32 unused_be0[4];
    i32 spawnDelay;
    u16 exFlags;
    u16 moreFlags;
    i16 spriteOffset;
    i16 unused_bfa;
    u16 state;
    u16 outOfBoundsTime;
    u8 spawned;
    u8 grazed;
    // pad 2
    Bullet *next;
    i32 state2;
    i32 soundIdx;
    i32 curCmdIdx;
    BulletCommand commands[5];
    BulletCommandState commandStates[5];
};
C_ASSERT(sizeof(Bullet) == 0xd68);

struct BulletManager
{
    BulletManager()
    {
        Initialize();
    }

    static ZunResult RegisterChain(const char *etamaAnmPath);
    static void CutChain();

    static ZunResult AddedCallback(BulletManager *arg);
    static ZunResult DeletedCallback(BulletManager *arg);
    static u32 OnUpdate(BulletManager *arg);
    static u32 OnDraw(BulletManager *arg);

    void Initialize();

    i32 DespawnBullets(i32 param_1, i32 turnIntoItem);
    void RemoveAllBullets(i32 param_1);
    void RemoveBulletsInRadius(D3DXVECTOR3 *centerPos, f32 radius);
    static void SetActiveSpriteByResolution(AnmVm *sprite,
                                            AnmVm *bulletTypeTemplate,
                                            Bullet *bullet, i32 spriteOffset);
    i32 SpawnBulletPattern(struct EnemyBulletShooter *bulletProps);
    Laser *SpawnLaserPattern(struct EnemyLaserShooter *laserProps);
    i32 SpawnSingleBullet(EnemyBulletShooter *bulletProps, i32 x, i32 y,
                          f32 angle);
    void StopBulletMovement();

    // i have no idea why this exists either
    static void AnInlineFunctionThatAllocates4BytesAndNothingElse()
    {
        i32 idk;
    }

    BulletTypeSprites bulletTypeTemplates[16];
    Bullet bullets[1025];
    Laser lasers[64];
    i32 bulletCount;
    i32 screenClearTime;
    ZunTimer time;
    i32 updateCount;
    const char *etamaAnmPath;
    Bullet *bulletsPtrs[6];
    Bullet *bulletsStart;
    ItemType itemType;
};
C_ASSERT(sizeof(BulletManager) == 0x37a164);
extern BulletManager g_BulletManager;
