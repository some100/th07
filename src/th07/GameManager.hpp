#pragma once

#include <d3d8.h>

#include "ResultScreen.hpp"
#include "Rng.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "inttypes.hpp"

struct ZunGlobals
{
    u32 guiScore;
    u32 score;
    u32 guiScoreDifference;
    u32 highScore;
    u8 highScoreNumContinues;
    // pad 3
    i32 grazeInStage;
    i32 grazeInTotal;
    i32 spellCardsCaptured;
    u8 numRetries;
    // pad 3
    i32 pointItemsCollectedThisStage;
    i32 pointItemsCollectedForExtend;
    i32 extendsFromPointItems;
    i32 nextNeededPointItemsForExtend;
    i32 rng1[7];
    f32 deaths; // ZUN quirk: Why the fuck are these stored as floats
    f32 rngFloat1[2];
    f32 livesRemaining;
    f32 rngFloat2[2];
    f32 bombsRemaining;
    f32 bombsUsed;
    f32 rngFloat3[3];
    f32 currentPower;
    f32 rngFloat4[2];
    i32 cherryStart;
    i32 rng2[8];
    u32 curCsum;
    i32 csumAsSum;
    i32 csumData[5];
};
C_ASSERT(sizeof(ZunGlobals) == 0xc8);

struct Rank
{
    i32 rank;
    i32 maxRank;
    i32 minRank;
};

struct GameManager
{
    GameManager()
    {
        memset(this, 0, sizeof(GameManager));
        this->arcadeRegionTopLeftPos.x = 32.0f;
        this->arcadeRegionTopLeftPos.y = 16.0f;
        this->arcadeRegionSize.x = 384.0f;
        this->arcadeRegionSize.y = 448.0f;
        this->demoIdx = 2;
        this->phantasmUnlocked = TRUE;
    }

#pragma var_order(local_10, local_c)
    // FUNCTION: TH07 0x004012b0
    void RegenerateGameIntegrityCsum()
    {
        this->globals->rng1[2] = g_Rng.GetRandomU32InRange(100000) + 6543;
        this->globals->rng2[3] = g_Rng.GetRandomU32InRange(100000) + 6543;
        this->globals->curCsum = this->globals->rng1[2];

        this->globals->csumAsSum = ComputeGameIntegrityCsum();
        this->csumFloat = (f32)(this->globals->csumAsSum + this->globals->rng2[3]);
    }

    // FUNCTION: TH07 0x00401390
    void SetBombsRemainingAndComputeCsum(i32 param_1)
    {
        this->globals->bombsRemaining = (f32)param_1;
        this->globals->curCsum = this->globals->rng1[2];
        this->globals->csumAsSum = ComputeGameIntegrityCsum();
        this->csumFloat =
            (f32)(this->globals->csumAsSum + this->globals->rng2[3]);
    }

    // FUNCTION: TH07 0x00404fe0
    ZunBool CheckGameIntegrity()
    {
#ifdef NON_MATCHING
        return FALSE;
#else
        return (this->globals->curCsum !=
                this->globals->rng1[2] + this->globals->csumData[2] *
                                             ((i32) & this->globals->curCsum - (i32)this->globals->rng1 +
                                                          sizeof(this->globals->csumData) + sizeof(GameConfiguration) * 2)) ||
               (this->globals->csumAsSum + this->globals->rng2[3] !=
                (i32)this->csumFloat);
#endif
    }

    // FUNCTION: TH07 0x0043b5c0
    void RerollRng()
    {
        this->globals->rng1[0] = g_Rng.GetRandomU32InRange(100000) + 6543;
        this->globals->rng1[1] = g_Rng.GetRandomU32InRange(100000) + 6543;
        this->globals->rng1[2] = g_Rng.GetRandomU32InRange(100000) + 6543;
        this->globals->rng1[3] = g_Rng.GetRandomU32InRange(100000) + 6543;
        this->globals->rng1[4] = g_Rng.GetRandomU32InRange(100000) + 6543;
        this->globals->rngFloat3[0] = g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
        this->globals->rngFloat3[1] = g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
        this->globals->rngFloat3[2] = g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
    }

    // FUNCTION: TH07 0x0043b750
    void CheckGameIntegrityOnDeath(i32 amount)
    {
        if (CheckGameIntegrity())
        {
            NUKE_SUPERVISOR();
        }
        this->globals->deaths += (f32)amount;
        RegenerateGameIntegrityCsum();
    }

    void AddCurrentPower(i32 amount)
    {
        if (CheckGameIntegrity())
        {
            NUKE_SUPERVISOR();
        }
        this->globals->currentPower += (f32)amount;
        RegenerateGameIntegrityCsum();
    }

    // FUNCTION: TH07 0x0043b7a0
    void AddBombsUsed(i32 amount)
    {
        if (CheckGameIntegrity())
        {
            NUKE_SUPERVISOR();
        }
        this->globals->bombsUsed += (f32)amount;
        RegenerateGameIntegrityCsum();
    }

    // FUNCTION: TH07 0x0042d5cd
    void AddLivesRemaining(i32 amount)
    {
        if (CheckGameIntegrity())
        {
            NUKE_SUPERVISOR();
        }
        this->globals->livesRemaining += (f32)amount;
        RegenerateGameIntegrityCsum();
    }

    // FUNCTION: TH07 0x0042d612
    void AddBombsRemaining(i32 amount)
    {
        if (CheckGameIntegrity())
        {
            NUKE_SUPERVISOR();
        }
        this->globals->bombsRemaining += (f32)amount;
        RegenerateGameIntegrityCsum();
    }

    void SetIsReplay(ZunBool replay)
    {
        this->replay = replay;
    }

    void AddScore(i32 score)
    {
        this->globals->score += score / 10;
    }

    ZunBool IsCherryAtMax()
    {
        return this->cherry >= this->cherryMax;
    }

    void SetCurrentPower(i32 amount)
    {
        this->globals->currentPower = (f32)amount;
    }

    void SetLivesRemaining(i32 amount)
    {
        this->globals->livesRemaining = (f32)amount;
    }

    // FUNCTION: TH07 0x0042d657
    void ResetRegionsPos()
    {
        this->arcadeRegionTopLeftPos.x = 32.0f;
        this->arcadeRegionTopLeftPos.y = 16.0f;
        this->arcadeRegionSize.x = 384.0f;
        this->arcadeRegionSize.y = 448.0f;
        this->playerMovementAreaTopLeftPos.x = 8.0f;
        this->playerMovementAreaTopLeftPos.y = 16.0f;
        this->playerMovementAreaSize.x = 368.0f;
        this->playerMovementAreaSize.y = 416.0f;
    }

    static ZunResult RegisterChain();
    static void CutChain();

    static ZunResult AddedCallback(GameManager *arg);
    static ZunResult DeletedCallback(GameManager *arg);
    static u32 OnUpdate(GameManager *arg);
    static u32 OnDraw(GameManager *arg);

    static i32 ByteCsumAccumulator(u8 *param_1, i32 param_2);
    i32 ComputeGameIntegrityCsum();

    ZunBool HasReachedMaxClearsAnyDifficulty(i32 shotType);
    ZunBool HasReachedMaxClearsAnyShotType();
    ZunBool HasUnlockedPhantasm(i32 shotType);
    ZunBool HasUnlockedPhantasmAndMaxClears();

    void AddCherryPlus(i32 amount);
    void AddCherry(i32 amount);
    void ExtendFromPoints();

    void DecreaseSubrank(i32 amount);
    void IncreaseCherry(i32 amount);
    void IncreaseCherryMax(i32 amount);
    void IncreaseSubrank(i32 amount);
    void InitializeRank();
    static void InitializeRngAndCsum();
    i32 IsInBounds(f32 x, f32 y, f32 widthPx, f32 heightPx);

    static void DrawLoadingSprite();

    void *tmpBuffer;
    GameConfiguration *defaultCfg;
    ZunGlobals *globals;
    i8 isTimeStopped;
    i8 slowModeSlowActive;
    // pad 2
    i32 difficulty;
    u32 difficultyMask;
    struct Catk catk[SPELLCARD_COUNT];
    struct Catk catkAgain[SPELLCARD_COUNT];
    struct Clrd clrd[SHOT_COUNT];
    struct Pscr pscr[6][6][4];
    struct Plst plst;
    ZunBool isPaused;
    i8 powerItemCountForScore;
    u8 character;
    u8 shotType;
    u8 shotTypeAndCharacter;
    union {
        u32 flags;
        struct
        {
            u32 practice : 1;
            u32 demo : 1;
            u32 notInMenu : 1;
            u32 replay : 1;
            u32 finished : 1;
        };
    };
    u8 isInPauseMenu;
    u8 isInRetryMenu;
    u8 demoIdx;
    u8 replayStage;
    i32 demoFrames;
    char replayFilename[512];
    u16 stageRngSeed;
    // pad 2
    i32 framesThisStage;
    i32 currentStage;
    i32 unused_95f0;
    Float2 arcadeRegionTopLeftPos;
    Float2 arcadeRegionSize;
    Float2 playerMovementAreaTopLeftPos;
    Float2 playerMovementAreaSize;
    f32 csumFloat;
    i32 cherryMax;
    i32 cherry;
    i32 cherryPlus;
    ZunBool phantasmUnlocked;
    i32 playTimeAll; // ZUN name: PlayTimeAll
    u32 bulletLagTime;
    i32 maxRetries;
    Rank rank;
    i32 subrank;
};
C_ASSERT(sizeof(GameManager) == 0x9644);
extern GameManager g_GameManager;
