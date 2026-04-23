#include "GameManager.hpp"

#include <stdio.h>

#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "EclManager.hpp"
#include "EnemyManager.hpp"
#include "GameErrorContext.hpp"
#include "Gui.hpp"
#include "Player.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"

#pragma optimize("s", on)

// GLOBAL: TH07 0x00626270
GameManager g_GameManager;

// GLOBAL: TH07 0x0062f8b4
ChainElem g_GameManagerCalcChain;

// GLOBAL: TH07 0x0062f8d4
ChainElem g_GameManagerDrawChain;

// GLOBAL: TH07 0x0049f5d0
i32 g_RankArray[6][3] = {
    {16, 12, 20},
    {16, 10, 32},
    {16, 10, 32},
    {16, 10, 32},
    {16, 15, 16},
    {16, 15, 16},
};

// FUNCTION: TH07 0x0042d560
GameManager::GameManager()
{
    memset(this, 0, sizeof(GameManager));
    this->arcadeRegionTopLeftPos.x = 32.0f;
    this->arcadeRegionTopLeftPos.y = 16.0f;
    this->arcadeRegionSize.x = 384.0f;
    this->arcadeRegionSize.y = 448.0f;
    this->demoIdx = 2;
    this->isGameComplete = 1;
}

// FUNCTION: TH07 0x0042d5cd
void GameManager::AddLivesRemaining(i32 amount)
{
    if (CheckGameIntegrity() != 0)
    {
        NUKE_SUPERVISOR();
    }
    this->globals->livesRemaining += (f32)amount;
    RegenerateGameIntegrityCsum();
}

// FUNCTION: TH07 0x0042d612
void GameManager::AddBombsRemaining(i32 amount)
{
    if (CheckGameIntegrity() != 0)
    {
        NUKE_SUPERVISOR();
    }
    this->globals->bombsRemaining += (f32)amount;
    RegenerateGameIntegrityCsum();
}

// FUNCTION: TH07 0x0042d657
void GameManager::ResetRegionsPos()
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

// FUNCTION: TH07 0x0042d6d8
i32 GameManager::IsInBounds(f32 x, f32 y, f32 widthPx, f32 heightPx)
{
    if (widthPx / 2.0f + x < 0.0f)
        return 0;

    if (x - widthPx / 2.0f > 384.0f)
        return 0;

    if (heightPx / 2.0f + y < 0.0f)
        return 0;

    if (y - heightPx / 2.0f > 448.0f)
        return 0;

    return 1;
}

// FUNCTION: TH07 0x0042d75a
i32 GameManager::ByteCsumAccumulator(u8 *param_1, i32 param_2)
{
    u8 *local_10;
    i32 local_c;

    local_c = 0;
    local_10 = param_1;
    for (i32 i = 0; i < param_2; ++i)
    {
        local_c += (u32)*local_10;
        g_GameManager.globals->curCsum += g_GameManager.globals->csumData[2];
        local_10 += 1;
    }
    return local_c;
}

// FUNCTION: TH07 0x0042d7be
i32 GameManager::ComputeGameIntegrityCsum()
{
    return ByteCsumAccumulator((u8 *)g_GameManager.globals->rng1,
                               (i32)this->globals +
                                   (0xac - (i32)this->globals->rng1)) +
           ByteCsumAccumulator((u8 *)g_GameManager.globals->csumData,
                               sizeof(g_GameManager.globals->csumData)) +
           ByteCsumAccumulator((u8 *)g_GameManager.defaultCfg,
                               sizeof(GameConfiguration)) +
           ByteCsumAccumulator((u8 *)&g_Supervisor.cfg,
                               sizeof(GameConfiguration));
}

// FUNCTION: TH07 0x0042d83a
void GameManager::ExtendFromPoints()
{
    if ((i32)this->globals->livesRemaining < 8)
    {
        AddLivesRemaining(1);
        g_SoundPlayer.PlaySoundByIdx(SOUND_EXTEND, 0);
        IncreaseSubrank(200);
        g_Gui.flags = (g_Gui.flags & 0xfffffffc) | 2;
    }
    else
    {
        if ((i32)this->globals->bombsRemaining < 8)
        {
            AddBombsRemaining(1);
            g_SoundPlayer.PlaySoundByIdx(SOUND_EXTEND, 0);
            IncreaseSubrank(200);
            g_Gui.flags = (g_Gui.flags & 0xfffffff3) | 8;
        }
    }
}

#pragma var_order(i, scoreIncrement, arg, isNotInMenu)
// FUNCTION: TH07 0x0042d8d5
u32 GameManager::OnUpdate(GameManager *arg)
{
    i32 isNotInMenu;
    u32 scoreIncrement;
    u32 i;

    if ((((arg->isInPauseMenu == 0 && arg->isInRetryMenu == 0) &&
          arg->demo == 0) &&
         ((arg->slowModeSlowActive == 0 && WAS_PRESSED_RAW(TH_BUTTON_MENU)))))
    {
        arg->isInRetryMenu = 1;
        g_GameManager.arcadeRegionTopLeftPos.x = 32.0f;
        g_GameManager.arcadeRegionTopLeftPos.y = 16.0f;
        g_GameManager.arcadeRegionSize.x = 384.0f;
        g_GameManager.arcadeRegionSize.y = 448.0f;
        arg->isPaused = 1;
        if (g_GameManager.currentStage != 6 || g_Gui.frameCounter >= 300)
        {
            // STRING: TH07 0x00498a40
            g_SoundPlayer.PushCommand(AUDIO_PAUSE, 0, "Pause");
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_37, 0);
        g_Supervisor.UpdateTime();
    }
    g_Supervisor.viewport.X = (arg->arcadeRegionTopLeftPos).x;
    g_Supervisor.viewport.Y = (arg->arcadeRegionTopLeftPos).y;
    g_Supervisor.viewport.Width = (arg->arcadeRegionSize).x;
    g_Supervisor.viewport.Height = (arg->arcadeRegionSize).y;
    g_Supervisor.viewport.MinZ = 0.0f;
    g_Supervisor.viewport.MaxZ = 1.0f;
    g_AnmManager->currentCameraMode = 0xff;
    if ((((g_GameManager.replay != 0) &&
          (g_GameManager.replayStage == 1)) &&
         (g_Gui.HasCurrentMsgIdx() == 0)) &&
        (((((arg->bulletLagTime = arg->bulletLagTime + 1,
             g_Supervisor.curFps < 20 && (arg->bulletLagTime % 3 != 0)) ||
            ((g_Supervisor.curFps > 20 &&
              ((g_Supervisor.curFps < 0x1e &&
                (arg->bulletLagTime % 2 != 0)))))) ||
           ((g_Supervisor.curFps > 0x1e &&
             ((g_Supervisor.curFps < 0x28 && (arg->bulletLagTime % 3 == 0)))))) ||
          ((g_Supervisor.curFps > 0x28 &&
            ((g_Supervisor.curFps < 0x32 && (arg->bulletLagTime % 6 == 0))))))))
    {
        return CHAIN_CALLBACK_RESULT_BREAK;
    }
    if (arg->demo != 0)
    {
        if (WAS_PRESSED_RAW(TH_BUTTON_ANY))
        {
            g_Supervisor.curState = 1;
        }
        arg->demoFrames = arg->demoFrames + 1;
        if ((((arg->demoIdx == 0) && (arg->demoFrames == 0x1fa4)) ||
             ((arg->demoIdx == 1 && (arg->demoFrames == 0x1b6c)))) ||
            ((arg->demoIdx == 2 && (arg->demoFrames == 0x120c))))
        {
            BombEffects::RegisterChain(2, 0x78, 0, 0, 0);
            g_Supervisor.FadeOutMusic(3.0f);
        }
        if ((((arg->demoIdx == 0) && (arg->demoFrames > 0x201b)) ||
             ((arg->demoIdx == 1 && (arg->demoFrames > 0x1be4)))) ||
            ((arg->demoIdx == 2 && (arg->demoFrames > 0x1283))))
        {
            g_Supervisor.curState = 1;
            return CHAIN_CALLBACK_RESULT_BREAK;
        }
    }
    g_GameManager.globals->curCsum = g_GameManager.globals->rng1[2];
    g_GameManager.csumFloat = (f32)arg->ComputeGameIntegrityCsum() +
                              (f32)g_GameManager.globals->rng2[3];
    for (i = 0; i < 7; i++)
    {
        if ((arg->globals->rng1[i] < 0x198f) || (0x1a02f < arg->globals->rng1[i]))
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    for (i = 0; i < 2; i++)
    {
        if ((arg->globals->rngFloat2[i] < 6543.0f) ||
            (arg->globals->rngFloat2[i] > 106543.0f))
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    if ((arg->isInPauseMenu == 0) && (arg->isInRetryMenu == 0))
    {
        isNotInMenu = 1;
    }
    else
    {
        isNotInMenu = 0;
    }
    arg->notInMenu = isNotInMenu;
    for (i = 0; i < 2; i++)
    {
        if ((arg->globals->rngFloat1[i] < 6543.0f) ||
            (106543.0f < arg->globals->rngFloat1[i]))
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    for (i = 0; i < 8; i++)
    {
        if ((arg->globals->rng2[i] < 0x198f) || (0x1a02f < arg->globals->rng2[i]))
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    g_Supervisor.d3dDevice->Clear(0, NULL, 2, g_Stage.fogColor.color, 1.0f, 0);
    if (((arg->isInRetryMenu == 1) || (arg->isInRetryMenu == 2)) ||
        (arg->isInPauseMenu != 0))
        return CHAIN_CALLBACK_RESULT_BREAK;

    if (arg->globals->score >= 999999999)
    {
        arg->globals->score = 999999999;
    }
    if (arg->globals->guiScore != arg->globals->score)
    {
        if (arg->globals->score < arg->globals->guiScore)
        {
            arg->globals->score = arg->globals->guiScore;
        }
        scoreIncrement = (arg->globals->score - arg->globals->guiScore) >> 5;
        if (scoreIncrement < 0x8d55e)
        {
            if (scoreIncrement == 0)
            {
                scoreIncrement = 1;
            }
        }
        else
        {
            scoreIncrement = 0x8d55e;
        }
        if (arg->globals->guiScoreDifference < scoreIncrement)
        {
            arg->globals->guiScoreDifference = scoreIncrement;
        }
        if (arg->globals->score <
            arg->globals->guiScore + arg->globals->guiScoreDifference)
        {
            arg->globals->guiScoreDifference =
                arg->globals->score - arg->globals->guiScore;
        }
        arg->globals->guiScore =
            arg->globals->guiScore + arg->globals->guiScoreDifference;
        if (arg->globals->score <= arg->globals->guiScore)
        {
            arg->globals->guiScoreDifference = 0;
            arg->globals->guiScore = arg->globals->score;
        }
        if (arg->globals->highScore < arg->globals->guiScore)
        {
            arg->globals->highScore = arg->globals->guiScore;
            arg->globals->highScoreNumContinues = arg->globals->numRetries;
        }
    }
    for (i = 0; i < 3; i++)
    {
        if ((arg->globals->rngFloat3[i] < 6543.0f) ||
            (106543.0f < arg->globals->rngFloat3[i]))
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    for (i = 0; i < 2; i++)
    {
        if ((arg->globals->rngFloat4[i] < 6543.0f) ||
            (106543.0f < arg->globals->rngFloat4[i]))
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    for (i = 0; i < 5; i++)
    {
        if ((arg->globals->csumData[i] < 0x198f) ||
            (0x1a02f < arg->globals->csumData[i]))
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    if ((g_GameManager.defaultCfg)->slowMode != 0)
    {
        g_GameManager.slowModeSlowActive = 0;
        arg->bulletLagTime = arg->bulletLagTime + 1;
        if ((((g_BulletManager.bulletCount > 0x140) &&
              (arg->bulletLagTime % 3 == 0)) ||
             ((g_BulletManager.bulletCount < 0x140 &&
               ((g_BulletManager.bulletCount > 0xe0 &&
                 (arg->bulletLagTime % 4 == 0)))))) ||
            ((g_BulletManager.bulletCount < 0xe0 &&
              ((g_BulletManager.bulletCount > 0x80 &&
                (arg->bulletLagTime % 5 == 0))))))
        {
            g_GameManager.slowModeSlowActive = 1;
            return CHAIN_CALLBACK_RESULT_BREAK;
        }
        if (g_BulletManager.bulletCount < 0x80)
        {
            arg->bulletLagTime = 0;
        }
    }
    arg->framesThisStage = arg->framesThisStage + 1;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0042e1d4
u32 GameManager::OnDraw(GameManager *arg)
{
    if (arg->isInRetryMenu != 0)
    {
        arg->isInRetryMenu = 2;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0042e1f8
void GameManager::DrawLoadingSprite()
{
    AnmVm local_264;
    ZunRect local_14;

    local_14.left = 0.0f;
    local_14.top = 0.0f;
    local_14.right = 640.0f;
    local_14.bottom = 480.0f;
    local_264.Initialize();
    g_AnmManager->SetActiveSprite(&local_264, 0x10c);
    local_264.pos.x = 528.0f;
    local_264.pos.y = 448.0f;
    local_264.pos.z = 0.0f;
    g_Supervisor.d3dDevice->BeginScene();
    ScreenEffect::DrawSquare(&local_14, 0xa0000000);
    g_AnmManager->DrawNoRotation(&local_264);
    g_AnmManager->Flush();
    g_Supervisor.d3dDevice->EndScene();
    if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
    {
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
    }
    g_Supervisor.d3dDevice->BeginScene();
    ScreenEffect::DrawSquare(&local_14, 0xa0000000);
    g_AnmManager->DrawNoRotation(&local_264);
    g_AnmManager->Flush();
    g_Supervisor.d3dDevice->EndScene();
    if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
    {
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
    }
}

// FUNCTION: TH07 0x0042e38c
void GameManager::InitializeRank()
{
    this->rank.rank = g_RankArray[g_GameManager.difficulty][0];
    this->rank.minRank = g_RankArray[g_GameManager.difficulty][1];
    this->rank.maxRank = g_RankArray[g_GameManager.difficulty][2];
}

#pragma var_order(csum, i)
// FUNCTION: TH07 0x0042e3da
void GameManager::InitializeRngAndCsum()
{
    u32 i;

    g_GameManager.globals->cherryStart = g_Rng.GetRandomU32InRange(100000) + 0x198f;
    for (i = 0; i < 7; i++)
    {
        g_GameManager.globals->rng1[i] = g_Rng.GetRandomU32InRange(100000) + 0x198f;
    }
    for (i = 0; i < 8; i++)
    {
        g_GameManager.globals->rng2[i] = g_Rng.GetRandomU32InRange(100000) + 0x198f;
    }
    for (i = 0; i < 2; i++)
    {
        g_GameManager.globals->rngFloat1[i] =
            g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
    }
    for (i = 0; i < 2; i++)
    {
        g_GameManager.globals->rngFloat2[i] =
            g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
    }
    for (i = 0; i < 3; i++)
    {
        g_GameManager.globals->rngFloat3[i] =
            g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
    }
    for (i = 0; i < 2; i++)
    {
        g_GameManager.globals->rngFloat4[i] =
            g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
    }
    for (i = 0; i < 5; i++)
    {
        g_GameManager.globals->csumData[i] = g_Rng.GetRandomU32InRange(100000) + 0x198f;
    }
    g_GameManager.globals->curCsum = g_GameManager.globals->rng1[2];
    i32 csum = g_GameManager.ComputeGameIntegrityCsum();
    g_GameManager.globals->csumAsSum = csum;
    g_GameManager.csumFloat = (f32)csum +
                              (f32)g_GameManager.globals->rng2[3];
}

#pragma var_order(local_8, local_c, scoreDat, local_14)
// FUNCTION: TH07 0x0042e634
ZunResult ResultScreen::ParseScores()
{
    ScoreDat *scoreDat;
    i32 local_14;
    i32 local_c;
    Catk *local_8;

    local_8 = g_GameManager.catk;
    RegisterChain(2);
    memset(g_GameManager.catk, 0, sizeof(g_GameManager.catk));
    for (local_c = 0; local_c < 141; local_c++, local_8++)
    {
        local_8->magic = 0x4b544143;
        local_8->th7kLen2 = sizeof(Catk);
        local_8->th7kLen = sizeof(Catk);
        local_8->version = 1;
        local_8->idx = (i16)local_c;
        for (local_14 = 0; local_14 < 7; local_14++)
        {
            local_8->numAttemptsPerShot[local_14] = 0;
            local_8->numSuccessesPerShot[local_14] = 0;
            local_8->highScorePerShot[local_14] = 0;
        }
    }
    scoreDat = OpenScore("score.dat");
    if (scoreDat == NULL)
    {
        // STRING: TH07 0x00498090
        g_GameErrorContext.Log("error : ƒXƒRƒAƒtƒ@ƒCƒ‹‚Ì“Ç‚ÝŽæ‚è‚ÉŽ¸”s‚µ‚Ü‚µ‚½\r\n");
        return ZUN_ERROR;
    }

    g_GameManager.globals->highScore = GetHighScore(scoreDat, NULL, (u32)g_GameManager.shotTypeAndCharacter,
                                                    g_GameManager.difficulty,
                                                    &g_GameManager.globals->highScoreNumContinues);
    ParseCatk(scoreDat, g_GameManager.catk);
    ParseClrd(scoreDat, g_GameManager.clrd);
    ParsePscr(scoreDat, &g_GameManager.pscr[0][0][0]);
    if (g_GameManager.practice != 0)
    {
        g_GameManager.globals->highScore =
            g_GameManager
                .pscr[g_GameManager.shotTypeAndCharacter]
                     [g_GameManager.currentStage][g_GameManager.difficulty]
                .score;
        g_GameManager
            .pscr[g_GameManager.shotTypeAndCharacter][g_GameManager.currentStage]
                 [g_GameManager.difficulty]
            .playCount += 1;
        g_GameManager.globals->highScoreNumContinues = 0;
    }
    ReleaseScoreDat(scoreDat);
    memcpy(g_GameManager.catkAgain, g_GameManager.catk, sizeof(g_GameManager.catkAgain));
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0042e81b
void IncrementCappedAgain(u32 *param, u32 cap)
{
    // cap seemingly completely unused here
    if (*param < 999999)
    {
        (*param)++; // otherwise duplicate of incrementcapped from asciimanager
    }
}

// FUNCTION: TH07 0x0042e83e
ZunResult GameManager::AddedCallback(GameManager *arg)
{
    u16 uVar2;

    g_Supervisor.checkTiming = 0;
    arg->difficultyMask = 1 << ((u8)arg->difficulty & 0x1f);
    arg->shotTypeAndCharacter = (arg->shotType + arg->character * 2);
    g_Supervisor.currentTime = timeGetTime();
    g_Supervisor.effectiveFramerateMultiplier = 1.0f;
    if (g_Supervisor.curState == 3)
    {
        arg->globals->guiScore = arg->globals->score;
        arg->globals->guiScoreDifference = 0;
        if (Player::RegisterChain(0) != ZUN_SUCCESS)
        {
            // STRING: TH07 0x00498064
            g_GameErrorContext.Log("error : ƒvƒŒƒCƒ„[‚Ì‰Šú‰»‚ÉŽ¸”s‚µ‚Ü‚µ‚½\r\n");
            return ZUN_ERROR;
        }
    }
    else
    {
        DrawLoadingSprite();
        SAFE_FREE(arg->defaultCfg);
        SAFE_FREE(arg->globals);

        arg->tmpBuffer = malloc(g_Rng.GetRandomU32InRange(0xffff) + 0x10);
        arg->defaultCfg = new GameConfiguration;
        arg->globals = new ZunGlobals;
        InitializeRngAndCsum();
        *arg->defaultCfg = g_Supervisor.cfg;
        free(arg->tmpBuffer);
        arg->powerItemCountForScore = 0;
        arg->cherry = arg->globals->cherryStart;
        arg->cherryPlus = arg->globals->cherryStart;
        if (3 < g_GameManager.difficulty)
        {
            arg->defaultCfg->lifeCount = 2;
        }
        if (g_GameManager.practice != 0)
        {
            arg->defaultCfg->lifeCount = 8;
        }
        if (Player::RegisterChain(0) != ZUN_SUCCESS)
        {
            g_GameErrorContext.Log("error : ƒvƒŒƒCƒ„[‚Ì‰Šú‰»‚ÉŽ¸”s‚µ‚Ü‚µ‚½\r\n");
            return ZUN_ERROR;
        }
        if (g_GameManager.replay == 0)
        {
            g_GameManager.globals->livesRemaining = (f32)arg->defaultCfg->lifeCount;
            g_GameManager.RegenerateGameIntegrityCsum();
            g_GameManager.SetBombsRemainingAndComputeCsum(
                g_Player.shooterData->initialBombs);
        }
        arg->ResetRegionsPos();
        arg->globals->currentPower = 0.0f;
        arg->RegenerateGameIntegrityCsum();
        arg->activeFrameCounter = 0;
        arg->globals->guiScore = 0;
        arg->globals->score = 0;
        arg->globals->guiScoreDifference = 0;
        arg->globals->highScore = 100000;
        arg->globals->numRetries = 0;
        arg->globals->grazeInTotal = 0;
        arg->globals->pointItemsCollectedForExtend = 0;
        if (arg->difficulty < 4)
        {
            arg->globals->nextNeededPointItemsForExtend = 0x32;
        }
        else
        {
            arg->globals->nextNeededPointItemsForExtend = 200;
            arg->defaultCfg->slowMode = 0;
        }
        arg->globals->extendsFromPointItems = 0;
        if (ResultScreen::ParseScores() != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        arg->InitializeRank();
        arg->globals->deaths = 0.0f;
        arg->RegenerateGameIntegrityCsum();
        arg->globals->bombsUsed = 0.0f;
        arg->RegenerateGameIntegrityCsum();
        arg->globals->spellCardsCaptured = 0;
        if (g_GameManager.practice == 0)
        {
            if (arg->difficulty == DIFF_EASY)
            {
                arg->cherryMax = arg->globals->cherryStart + 200000;
            }
            else if (arg->difficulty == DIFF_NORMAL)
            {
                arg->cherryMax = arg->globals->cherryStart + 200000;
            }
            else if (arg->difficulty == DIFF_HARD)
            {
                arg->cherryMax = arg->globals->cherryStart + 250000;
            }
            else if (arg->difficulty == DIFF_LUNATIC)
            {
                arg->cherryMax = arg->globals->cherryStart + 300000;
            }
            else if (arg->difficulty == DIFF_EXTRA)
            {
                arg->cherryMax = arg->globals->cherryStart + 400000;
                arg->cherry = arg->globals->cherryStart + 200000;
            }
            else if (arg->difficulty == DIFF_PHANTASM)
            {
                arg->cherryMax = arg->globals->cherryStart + 400000;
                arg->cherry = arg->globals->cherryStart + 300000;
            }
        }
        else
        {
            if (arg->difficulty == DIFF_EASY)
            {
                arg->cherryMax = arg->globals->cherryStart + 200000;
            }
            else if (arg->difficulty == DIFF_NORMAL)
            {
                arg->cherryMax = arg->globals->cherryStart + 200000;
            }
            else if (arg->difficulty == DIFF_HARD)
            {
                arg->cherryMax = arg->globals->cherryStart + 250000;
            }
            else if (arg->difficulty == DIFF_LUNATIC)
            {
                arg->cherryMax = arg->globals->cherryStart + 300000;
            }
            if (arg->currentStage == 1)
            {
                arg->cherry = arg->cherryMax;
            }
            else if (arg->currentStage == 2)
            {
                arg->cherryMax += 50000;
                arg->cherry = arg->cherryMax;
            }
            else if (arg->currentStage == 3)
            {
                arg->cherryMax += 100000;
                arg->cherry = arg->cherryMax;
            }
            else if (arg->currentStage == 4)
            {
                arg->cherryMax += 150000;
                arg->cherry = arg->cherryMax;
            }
            else if (arg->currentStage == 5)
            {
                arg->cherryMax += 200000;
                arg->cherry = arg->cherryMax;
            }
        }
        if (g_GameManager.replay == 0)
        {
            if (arg->defaultCfg->slowMode == 0)
            {
                IncrementCappedAgain(
                    &g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty]
                         .playCount,
                    999999);
                IncrementCappedAgain(&g_GameManager.plst.playDataTotals.playCount,
                                     999999);
                IncrementCappedAgain(
                    &g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty]
                         .playCountPerShotType[arg->shotTypeAndCharacter],
                    999999);
                IncrementCappedAgain(
                    g_GameManager.plst.playDataTotals.playCountPerShotType +
                        arg->shotTypeAndCharacter,
                    999999);
                if (g_Supervisor.curState == 10)
                {
                    IncrementCappedAgain(
                        &((Plst *)(g_GameManager.pscr + 6))
                             ->playDataByDifficulty[g_GameManager.difficulty]
                             .clearCount,
                        999999);
                    IncrementCappedAgain(&g_GameManager.plst.playDataTotals.clearCount,
                                         999999);
                }
                if (g_GameManager.practice != 0)
                {
                    IncrementCappedAgain(
                        &((Plst *)(g_GameManager.pscr + 6))
                             ->playDataByDifficulty[g_GameManager.difficulty]
                             .extraClearCount,
                        999999);
                    IncrementCappedAgain(
                        &g_GameManager.plst.playDataTotals.extraClearCount, 999999);
                }
            }
        }
        else
        {
            arg->defaultCfg->slowMode = 0;
        }
    }
    arg->subrank = 0;
    arg->globals->pointItemsCollectedThisStage = 0;
    arg->globals->grazeInStage = 0;
    arg->isInRetryMenu = 0;
    arg->currentStage = arg->currentStage + 1;
    if (g_GameManager.replay == 0)
    {
        if ((arg->globals->numRetries == 0) &&
            ((i32)(u32)arg->clrd[g_GameManager.shotTypeAndCharacter]
                 .difficultyClearedWithRetries[g_GameManager.difficulty] <
             arg->currentStage - 1))
        {
            arg->clrd[g_GameManager.shotTypeAndCharacter]
                .difficultyClearedWithRetries[g_GameManager.difficulty] =
                (char)arg->currentStage - 1;
        }
        if ((i32)(u32)arg->clrd[g_GameManager.shotTypeAndCharacter]
                .difficultyClearedWithoutRetries[g_GameManager.difficulty] <
            arg->currentStage - 1)
        {
            arg->clrd[g_GameManager.shotTypeAndCharacter]
                .difficultyClearedWithoutRetries[g_GameManager.difficulty] =
                (char)arg->currentStage - 1;
        }
    }
    if ((arg->demo != 0) && (arg->currentStage != 1))
    {
        arg->globals->currentPower = 128.0f;
        arg->RegenerateGameIntegrityCsum();
    }
    if (g_GameManager.replay != 0)
    {
        arg->InitializeRank();
        ReplayManager::RegisterChain(1, g_GameManager.replayFilename);
        uVar2 = g_Rng.seed;
        arg->RegenerateGameIntegrityCsum();
        g_Rng.seed = uVar2;
    }
    arg->stageRngSeed = g_Rng.seed;
    if (Stage::RegisterChain(arg->currentStage) == ZUN_SUCCESS)
    {
        if (BulletManager::RegisterChain("data/etama.anm") == ZUN_SUCCESS)
        {
            if (EnemyManager::RegisterChain(
                    g_AnmStageFiles[arg->currentStage].stageName1,
                    g_AnmStageFiles[arg->currentStage].stageName2) == ZUN_SUCCESS)
            {
                if (g_EclManager.Load(g_EclPaths[arg->currentStage]) == ZUN_SUCCESS)
                {
                    if (EffectManager::RegisterChain() == ZUN_SUCCESS)
                    {
                        if (Gui::RegisterChain() == ZUN_SUCCESS)
                        {
                            if (g_GameManager.replay == 0)
                            {
                                // STRING: TH07 0x00497e1c
                                ReplayManager::RegisterChain(0, "replay/th7_00.rpy");
                            }
                            g_Supervisor.LoadAudio(0, (g_Stage.stdData)->bgmPaths[0]);
                            g_Supervisor.LoadAudio(1, (g_Stage.stdData)->bgmPaths[1]);
                            if (arg->currentStage == 6)
                            {
                                g_Supervisor.StopAudio();
                                g_Supervisor.LoadAudio(2, "bgm/th07_13b.mid");
                            }
                            else
                            {
                                g_Supervisor.PlayLoadedAudio(0);
                            }
                            while (g_SoundPlayer.ProcessQueues() != 0)
                                ;
                            arg->isInPauseMenu = 0;
                            arg->notInMenu = 1;
                            if (g_Supervisor.curState != 3)
                            {
                                g_Supervisor.framerateMultiplier = 0.0f;
                                g_Supervisor.fpsAccumulator = 0.0f;
                            }
                            arg->isTimeStopped = 0;
                            arg->globals->score = 0;
                            arg->finished = 0;
                            g_AsciiManager.InitializeVms();
                            g_GameManager.slowModeSlowActive = 0;
                            Supervisor::DrawFpsCounter(0);
                            // STRING: TH07 0x00497e08
                            Supervisor::DebugPrint2("random seed %d %d\r\n", (u32)g_Rng.seed,
                                                    g_Rng.generationCount);
                            return ZUN_SUCCESS;
                        }
                        else
                        {
                            // STRING: TH07 0x00497e30
                            g_GameErrorContext.Log("error : 2D•\Ž¦‚Ì‰Šú‰»‚ÉŽ¸”s‚µ‚Ü‚µ‚½\r\n");
                            return ZUN_ERROR;
                        }
                    }
                    else
                    {
                        // STRING: TH07 0x00497e58
                        g_GameErrorContext.Log("error : ƒGƒtƒFƒNƒg‚Ì‰Šú‰»‚ÉŽ¸”s‚µ‚Ü‚µ‚½\r\n");
                        return ZUN_ERROR;
                    }
                }
                else
                {
                    // STRING: TH07 0x00497e84
                    g_GameErrorContext.Log("error : “G“ª”]‚Ì‰Šú‰»‚ÉŽ¸”s‚µ‚Ü‚µ‚½\r\n");
                    return ZUN_ERROR;
                }
            }
            else
            {
                // STRING: TH07 0x00497f4c
                g_GameErrorContext.Log("error : “G‚Ì‰Šú‰»‚ÉŽ¸”s‚µ‚Ü‚µ‚½\r\n");
                return ZUN_ERROR;
            }
        }
        else
        {
            // STRING: TH07 0x00498010
            g_GameErrorContext.Log("error : “G’e‚Ì‰Šú‰»‚ÉŽ¸”s‚µ‚Ü‚µ‚½\r\n");
            return ZUN_ERROR;
        }
    }
    else
    {
        // STRING: TH07 0x00498038
        g_GameErrorContext.Log("error : ”wŒiƒf[ƒ^‚Ì‰Šú‰»‚ÉŽ¸”s‚µ‚Ü‚µ‚½\r\n");
        return ZUN_ERROR;
    }
}

// FUNCTION: TH07 0x0042f2e4
ZunResult GameManager::DeletedCallback(GameManager *arg)
{
    g_Supervisor.StopAudio();
    if ((g_Supervisor.cfg.musicMode == MUSIC_MIDI) &&
        (g_Supervisor.midiOutput != NULL))
    {
        g_Supervisor.midiOutput->Stop();
    }
    while (g_SoundPlayer.ProcessQueues() != 0)
        ;
    Stage::CutChain();
    BulletManager::CutChain();
    Player::CutChain();
    EnemyManager::CutChain();
    g_EclManager.Unload();
    EffectManager::CutChain();
    Gui::CutChain();
    ReplayManager::StopRecording();
    if (g_GameManager.replay == 0)
    {
        g_Supervisor.UpdateTime();
    }
    g_Supervisor.currentTime = 0;
    g_Supervisor.UpdateStartupTime();
    arg->notInMenu = 0;
    g_AsciiManager.InitializeVms();
    g_GameManager.slowModeSlowActive = 0;
    g_GameManager.framesThisStage = 0;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0042f3c5
ZunResult GameManager::RegisterChain()
{
    GameManager *mgr = &g_GameManager;
    g_GameManagerCalcChain.callback = (ChainCallback)OnUpdate;
    g_GameManagerCalcChain.addedCallback = NULL;
    g_GameManagerCalcChain.deletedCallback = NULL;
    g_GameManagerCalcChain.addedCallback = (ChainLifecycleCallback)AddedCallback;
    g_GameManagerCalcChain.deletedCallback =
        (ChainLifecycleCallback)DeletedCallback;
    g_GameManagerCalcChain.arg = mgr;
    mgr->framesThisStage = 0;
    if (g_Chain.AddToCalcChain(&g_GameManagerCalcChain, 2) != 0)
        return ZUN_ERROR;

    g_GameManagerDrawChain.callback = (ChainCallback)OnDraw;
    g_GameManagerDrawChain.addedCallback = NULL;
    g_GameManagerDrawChain.deletedCallback = NULL;
    g_GameManagerDrawChain.arg = mgr;
    g_Chain.AddToDrawChain(&g_GameManagerDrawChain, 2);
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0042f45d
void GameManager::CutChain()
{
    g_Chain.Cut(&g_GameManagerCalcChain);
    g_Chain.Cut(&g_GameManagerDrawChain);
    if (1000000000 <= g_GameManager.globals->score)
    {
        g_GameManager.globals->score = 999999999;
    }
    g_GameManager.globals->guiScore = g_GameManager.globals->score;
}

// FUNCTION: TH07 0x0042f4aa
void GameManager::IncreaseSubrank(i32 amount)
{
    this->subrank += amount;
    while (100 <= this->subrank)
    {
        ++this->rank.rank;
        this->subrank -= 100;
    }
    if (this->rank.rank > this->rank.maxRank)
    {
        this->rank.rank = this->rank.maxRank;
    }
}

// FUNCTION: TH07 0x0042f526
void GameManager::DecreaseSubrank(i32 amount)
{
    this->subrank -= amount;
    while (this->subrank < 0)
    {
        this->rank.rank -= 1;
        this->subrank += 100;
    }
    if (this->rank.rank < this->rank.minRank)
    {
        this->rank.rank = this->rank.minRank;
    }
}

// FUNCTION: TH07 0x0042f5a2
void GameManager::AddCherryPlus(i32 amount)
{
    i32 oldCherry = this->cherry;
    this->cherry = this->cherry + amount;
    if (this->cherry > this->cherryMax)
    {
        this->cherry = this->cherryMax;
    }
    if (0 < amount && g_Player.hasBorder == BORDER_NONE)
    {
        this->cherryPlus = this->cherryPlus + amount;
        if (this->cherryPlus >= this->globals->cherryStart + 50000)
        {
            this->cherryPlus = this->globals->cherryStart + 50000;
            g_Player.ActivateBorder();
        }
    }
    if ((this->cherry >= this->cherryMax) && (oldCherry != this->cherry))
    {
        g_Gui.ShowFullPowerMode(this->cherry - this->globals->cherryStart, 3);
    }
}

// FUNCTION: TH07 0x0042f69f
void GameManager::AddCherry(i32 amount)
{
    i32 oldCherry = this->cherry;
    this->cherry = this->cherry + amount;
    if (this->cherry > cherryMax)
    {
        this->cherry = this->cherryMax;
    }
    if ((this->cherry >= this->cherryMax) && (oldCherry != this->cherry))
    {
        g_Gui.ShowFullPowerMode(this->cherry - this->globals->cherryStart, 3);
    }
}

// FUNCTION: TH07 0x0042f736
void GameManager::IncreaseCherry(i32 amount)
{
    i32 idk = this->cherry;
    this->cherry = this->cherry + amount;
    if (this->cherry > this->cherryMax)
    {
        this->cherry = this->cherryMax;
    }
}

// FUNCTION: TH07 0x0042f789
void GameManager::IncreaseCherryMax(i32 amount)
{
    this->cherryMax = this->cherryMax + amount;
    if (this->cherryMax >= this->globals->cherryStart + 9999990)
    {
        this->cherryMax = this->globals->cherryStart + 9999990;
    }
}

// FUNCTION: TH07 0x0042f7df
i32 GameManager::HasReachedMaxClears(i32 shotType)
{
    if ((((this->clrd[shotType].difficultyClearedWithRetries[0] == 99) ||
          (this->clrd[shotType].difficultyClearedWithRetries[1] == 99)) ||
         (this->clrd[shotType].difficultyClearedWithRetries[2] == 99)) ||
        (this->clrd[shotType].difficultyClearedWithRetries[3] == 99))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// FUNCTION: TH07 0x0042f853
i32 GameManager::HasUnlockedPhantom(i32 shotType)
{
    i32 local_8 = 0;
    for (i32 i = 0; i < 141; i++)
    {
        if (this->catk[i].numSuccessesPerShot[6] > 0)
        {
            local_8 += 1;
        }
    }
    if ((local_8 >= 60) &&
        (this->clrd[shotType].difficultyClearedWithRetries[4] == 99))
    {
        this->clrd[shotType].difficultyClearedWithRetries[5] = 99;
    }
    return this->clrd[shotType].difficultyClearedWithRetries[5] == 99;
}

// FUNCTION: TH07 0x0042f8de
i32 GameManager::HasReachedMaxClearsAllShotTypes()
{
    if ((((HasReachedMaxClears(0) == 0) && (HasReachedMaxClears(1) == 0)) &&
         (HasReachedMaxClears(2) == 0)) &&
        (((HasReachedMaxClears(3) == 0 && (HasReachedMaxClears(4) == 0)) &&
          (HasReachedMaxClears(5) == 0))))
    {
        return 0;
    }
    return 1;
}

// FUNCTION: TH07 0x0042f94c
i32 GameManager::HasUnlockedPhantomAndMaxClears()
{
    i32 local_10;
    i32 local_8;

    local_8 = 0;
    for (i32 i = 0; i < 141; i++)
    {
        if (this->catk[i].numSuccessesPerShot[6] != 0)
        {
            local_8 = local_8 + 1;
        }
    }
    if (local_8 > 60)
    {
        for (local_10 = 0; local_10 < 6; local_10 = local_10 + 1)
        {
            if (this->clrd[local_10].difficultyClearedWithRetries[4] == 99)
            {
                this->clrd[local_10].difficultyClearedWithRetries[5] = 99;
            }
        }
    }
    if ((((this->clrd[0].difficultyClearedWithRetries[5] == 99) ||
          (this->clrd[1].difficultyClearedWithRetries[5] == 99)) ||
         (this->clrd[2].difficultyClearedWithRetries[5] == 99)) ||
        (((this->clrd[3].difficultyClearedWithRetries[5] == 99 ||
           (this->clrd[4].difficultyClearedWithRetries[5] == 99)) ||
          (this->clrd[5].difficultyClearedWithRetries[5] == 99))))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

#pragma optimize("s", off)
