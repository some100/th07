#include "GameManager.hpp"

#include <stdio.h>

#include "AnmManager.hpp"
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
#include "ZunMemory.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"
#include "i18n.hpp"

// GLOBAL: TH07 0x0049f5d0
i32 g_RankArray[6][3] = {
    {16, 12, 20},
    {16, 10, 32},
    {16, 10, 32},
    {16, 10, 32},
    {16, 15, 16},
    {16, 15, 16},
};

// ZUN name: Stg
// GLOBAL: TH07 0x00626270
GameManager g_GameManager;

// GLOBAL: TH07 0x0062f8b4
ChainElem g_GameManagerCalcChain;

// GLOBAL: TH07 0x0062f8d4
ChainElem g_GameManagerDrawChain;

// FUNCTION: TH07 0x0042d6d8
i32 GameManager::IsInBounds(f32 x, f32 y, f32 widthPx, f32 heightPx)
{
    if (widthPx / 2.0f + x < 0.0f)
    {
        return 0;
    }

    if (x - widthPx / 2.0f > 384.0f)
    {
        return 0;
    }

    if (heightPx / 2.0f + y < 0.0f)
    {
        return 0;
    }

    if (y - heightPx / 2.0f > 448.0f)
    {
        return 0;
    }

    return 1;
}

#pragma var_order(i, local_c)
// FUNCTION: TH07 0x0042d75a
i32 GameManager::ByteCsumAccumulator(u8 *param_1, i32 param_2)
{
    i32 local_c;
    i32 i;

    local_c = 0;
    for (i = 0; i < param_2; i++, param_1++)
    {
        local_c += (u32)*param_1;
        g_GameManager.globals->curCsum += g_GameManager.globals->csumData[2];
    }
    return local_c;
}

// FUNCTION: TH07 0x0042d7be
i32 GameManager::ComputeGameIntegrityCsum()
{
    i32 csum = ByteCsumAccumulator((u8 *)g_GameManager.globals->rng1,
                                   (i32) & this->globals->curCsum - (i32)this->globals->rng1);
    csum += ByteCsumAccumulator((u8 *)g_GameManager.globals->csumData,
                                sizeof(g_GameManager.globals->csumData));
    csum += ByteCsumAccumulator((u8 *)g_GameManager.defaultCfg,
                                sizeof(GameConfiguration));
    csum += ByteCsumAccumulator((u8 *)&g_Supervisor.cfg,
                                sizeof(GameConfiguration));
    return csum;
}

// FUNCTION: TH07 0x0042d83a
void GameManager::ExtendFromPoints()
{
    if ((i32)this->globals->livesRemaining < 8)
    {
        AddLivesRemaining(1);
        g_SoundPlayer.PlaySoundByIdx(SOUND_EXTEND, 0);
        IncreaseSubrank(200);
        g_Gui.lifeDisplayUpdateFrames = 2;
    }
    else
    {
        if ((i32)this->globals->bombsRemaining < 8)
        {
            AddBombsRemaining(1);
            g_SoundPlayer.PlaySoundByIdx(SOUND_EXTEND, 0);
            IncreaseSubrank(200);
            g_Gui.bombDisplayUpdateFrames = 2;
        }
    }
}

#pragma var_order(csum, i, scoreIncrement)
// FUNCTION: TH07 0x0042d8d5
u32 GameManager::OnUpdate(GameManager *arg)
{
    u32 scoreIncrement;
    u32 i;
    i32 csum;

    if (arg->isInRetryMenu == 0 && arg->isInPauseMenu == 0 &&
        arg->demo == 0 &&
        (arg->slowModeSlowActive == 0 && WAS_PRESSED_RAW(TH_BUTTON_MENU)))
    {
        arg->isInPauseMenu = 1;
        g_GameManager.arcadeRegionTopLeftPos.x = 32.0f;
        g_GameManager.arcadeRegionTopLeftPos.y = 16.0f;
        g_GameManager.arcadeRegionSize.x = 384.0f;
        g_GameManager.arcadeRegionSize.y = 448.0f;
        arg->isPaused = TRUE;
        if (g_GameManager.currentStage != STAGE6 || g_Gui.frameCounter >= 300)
        {
            // STRING: TH07 0x00498a40
            g_SoundPlayer.PushCommand(AUDIO_PAUSE, 0, "Pause");
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_PAUSED, 0);
        g_Supervisor.UpdateTime();
    }
    g_Supervisor.viewport.X = arg->arcadeRegionTopLeftPos.x;
    g_Supervisor.viewport.Y = arg->arcadeRegionTopLeftPos.y;
    g_Supervisor.viewport.Width = arg->arcadeRegionSize.x;
    g_Supervisor.viewport.Height = arg->arcadeRegionSize.y;
    g_Supervisor.viewport.MinZ = 0.0f;
    g_Supervisor.viewport.MaxZ = 1.0f;
    g_AnmManager->SetCameraMode(255);
    if (g_GameManager.replay &&
        g_GameManager.replayStage == 1 &&
        !g_Gui.HasCurrentMsgIdx())
    {
        arg->bulletLagTime++;
        if ((g_Supervisor.curFps < 20 && arg->bulletLagTime % 3 != 0) ||
            (g_Supervisor.curFps >= 20 && g_Supervisor.curFps < 30 && arg->bulletLagTime % 2 != 0) ||
            (g_Supervisor.curFps >= 30 && g_Supervisor.curFps < 40 && arg->bulletLagTime % 3 == 0) ||
            (g_Supervisor.curFps >= 40 && g_Supervisor.curFps < 50 && arg->bulletLagTime % 6 == 0))
        {
            return CHAIN_CALLBACK_RESULT_BREAK;
        }
    }
    if (arg->demo)
    {
        if (WAS_PRESSED_RAW(TH_BUTTON_ANY))
        {
            g_Supervisor.curState = SUPERVISOR_STATE_MAINMENU;
        }
        arg->demoFrames = arg->demoFrames + 1;
        if ((arg->demoIdx == 0 && arg->demoFrames == 8100) ||
            (arg->demoIdx == 1 && arg->demoFrames == 7020) ||
            (arg->demoIdx == 2 && arg->demoFrames == 4620))
        {
            ScreenEffect::RegisterChain(
                SCREEN_EFFECT_FADE_IN_PLAY_AREA, 120, 0, 0, 0);
            g_Supervisor.FadeOutMusic(3.0f);
        }
        if ((arg->demoIdx == 0 && arg->demoFrames >= 8220) ||
            (arg->demoIdx == 1 && arg->demoFrames >= 7140) ||
            (arg->demoIdx == 2 && arg->demoFrames >= 4740))
        {
            g_Supervisor.curState = SUPERVISOR_STATE_MAINMENU;
            return CHAIN_CALLBACK_RESULT_BREAK;
        }
    }
    g_GameManager.globals->curCsum = g_GameManager.globals->rng1[2];
    csum = arg->ComputeGameIntegrityCsum();
    g_GameManager.csumFloat = (f32)csum + (f32)g_GameManager.globals->rng2[3];
    for (i = 0; i < ARRAY_SIZE_SIGNED(g_GameManager.globals->rng1); i++)
    {
        if (arg->globals->rng1[i] < 6543 || arg->globals->rng1[i] > 106543)
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(g_GameManager.globals->rngFloat2); i++)
    {
        if (arg->globals->rngFloat2[i] < 6543.0f ||
            arg->globals->rngFloat2[i] > 106543.0f)
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    arg->notInMenu = !arg->isInRetryMenu && !arg->isInPauseMenu;
    for (i = 0; i < ARRAY_SIZE_SIGNED(g_GameManager.globals->rngFloat1); i++)
    {
        if (arg->globals->rngFloat1[i] < 6543.0f ||
            arg->globals->rngFloat1[i] > 106543.0f)
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(g_GameManager.globals->rng2); i++)
    {
        if (arg->globals->rng2[i] < 6543 || arg->globals->rng2[i] > 106543)
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    g_Supervisor.d3dDevice->Clear(0, NULL, 2, g_Stage.skyFog.color.color, 1.0f, 0);
    if (arg->isInPauseMenu == 1 || arg->isInPauseMenu == 2 ||
        arg->isInRetryMenu)
    {
        return CHAIN_CALLBACK_RESULT_BREAK;
    }

    if (arg->globals->score >= 1000000000)
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
        if (scoreIncrement >= 578910)
        {
            scoreIncrement = 578910;
        }
        else if (scoreIncrement == 0)
        {
            scoreIncrement = 1;
        }

        if (arg->globals->guiScoreDifference < scoreIncrement)
        {
            arg->globals->guiScoreDifference = scoreIncrement;
        }
        if (arg->globals->guiScore + arg->globals->guiScoreDifference > arg->globals->score)
        {
            arg->globals->guiScoreDifference =
                arg->globals->score - arg->globals->guiScore;
        }
        arg->globals->guiScore =
            arg->globals->guiScore + arg->globals->guiScoreDifference;
        if (arg->globals->guiScore >= arg->globals->score)
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
    for (i = 0; i < ARRAY_SIZE_SIGNED(arg->globals->rngFloat3); i++)
    {
        if (arg->globals->rngFloat3[i] < 6543.0f ||
            arg->globals->rngFloat3[i] > 106543.0f)
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(arg->globals->rngFloat4); i++)
    {
        if (arg->globals->rngFloat4[i] < 6543.0f ||
            arg->globals->rngFloat4[i] > 106543.0f)
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(arg->globals->csumData); i++)
    {
        if (arg->globals->csumData[i] < 6543 ||
            arg->globals->csumData[i] > 106543)
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    if (g_GameManager.defaultCfg->slowMode)
    {
        g_GameManager.slowModeSlowActive = 0;
        arg->bulletLagTime = arg->bulletLagTime + 1;
        if ((g_BulletManager.bulletCount >= 320 && arg->bulletLagTime % 3 == 0) ||
            (g_BulletManager.bulletCount < 320 && g_BulletManager.bulletCount >= 224 && arg->bulletLagTime % 4 == 0) ||
            (g_BulletManager.bulletCount < 224 && g_BulletManager.bulletCount >= 128 && arg->bulletLagTime % 5 == 0))
        {
            g_GameManager.slowModeSlowActive = 1;
            return CHAIN_CALLBACK_RESULT_BREAK;
        }
        if (g_BulletManager.bulletCount < 128)
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
    if (arg->isInPauseMenu)
    {
        arg->isInPauseMenu = 2;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(rect, spriteVm, spritePos)
// FUNCTION: TH07 0x0042e1f8
void GameManager::DrawLoadingSprite()
{
    Float3 spritePos;
    AnmVm spriteVm;
    ZunRect rect;

    rect.left = 0.0f;
    rect.top = 0.0f;
    rect.right = (f32)GAME_WINDOW_WIDTH;
    rect.bottom = (f32)GAME_WINDOW_HEIGHT;
    g_AnmManager->InitializeAndSetActiveSprite(&spriteVm, ANM_SPRITE_ASCII_LOADING);
    spritePos.x = 528.0f;
    spritePos.y = 448.0f;
    spritePos.z = 0.0f;
    memcpy(&spriteVm.pos, spritePos, sizeof(Float3));
    g_Supervisor.d3dDevice->BeginScene();

    // ZUN bloat: This is doing the exact same thing twice
    ScreenEffect::DrawSquare(&rect, 0xa0000000);
    g_AnmManager->DrawNoRotation(&spriteVm);
    g_AnmManager->Flush();
    g_Supervisor.d3dDevice->EndScene();
    if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
    {
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
    }
    g_Supervisor.d3dDevice->BeginScene();
    ScreenEffect::DrawSquare(&rect, 0xa0000000);
    g_AnmManager->DrawNoRotation(&spriteVm);
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

    g_GameManager.globals->cherryStart = g_Rng.GetRandomU32InRange(100000) + 6543;
    for (i = 0; i < ARRAY_SIZE_SIGNED(g_GameManager.globals->rng1); i++)
    {
        g_GameManager.globals->rng1[i] = g_Rng.GetRandomU32InRange(100000) + 6543;
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(g_GameManager.globals->rng2); i++)
    {
        g_GameManager.globals->rng2[i] = g_Rng.GetRandomU32InRange(100000) + 6543;
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(g_GameManager.globals->rngFloat1); i++)
    {
        g_GameManager.globals->rngFloat1[i] =
            g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(g_GameManager.globals->rngFloat2); i++)
    {
        g_GameManager.globals->rngFloat2[i] =
            g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(g_GameManager.globals->rngFloat3); i++)
    {
        g_GameManager.globals->rngFloat3[i] =
            g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(g_GameManager.globals->rngFloat4); i++)
    {
        g_GameManager.globals->rngFloat4[i] =
            g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(g_GameManager.globals->csumData); i++)
    {
        g_GameManager.globals->csumData[i] = g_Rng.GetRandomU32InRange(100000) + 6543;
    }
    g_GameManager.globals->curCsum = g_GameManager.globals->rng1[2];
    i32 csum = g_GameManager.ComputeGameIntegrityCsum();
    g_GameManager.globals->csumAsSum = csum;
    g_GameManager.csumFloat = (f32)csum +
                              (f32)g_GameManager.globals->rng2[3];
}

#pragma var_order(catk, i, scoreDat, j)
// FUNCTION: TH07 0x0042e634
ZunResult ResultScreen::ParseScores()
{
    i32 j;
    ScoreDat *scoreDat;
    i32 i;
    Catk *catk;

    catk = g_GameManager.catk;
    RegisterChain(2);
    memset(g_GameManager.catk, 0, sizeof(g_GameManager.catk));
    for (i = 0; i < SPELLCARD_COUNT; i++, catk++)
    {
        catk->magic = 0x4b544143;
        catk->th7kLen2 = sizeof(Catk);
        catk->th7kLen = sizeof(Catk);
        catk->version = 1;
        catk->idx = (i16)i;
        for (j = 0; j < ARRAY_SIZE_SIGNED(catk->numAttemptsPerShot); j++)
        {
            catk->numAttemptsPerShot[j] = 0;
            catk->numSuccessesPerShot[j] = 0;
            catk->highScorePerShot[j] = 0;
        }
    }
    scoreDat = OpenScore("score.dat");
    if (!scoreDat)
    {
        // STRING: TH07 0x00498090
        g_GameErrorContext.Log(TH_ERR_SCORE_LOAD_FAIL);
        return ZUN_ERROR;
    }

    g_GameManager.globals->highScore = GetHighScore(scoreDat, NULL, (u32)g_GameManager.shotTypeAndCharacter,
                                                    g_GameManager.difficulty,
                                                    &g_GameManager.globals->highScoreNumContinues);
    ParseCatk(scoreDat, g_GameManager.catk);
    ParseClrd(scoreDat, g_GameManager.clrd);
    ParsePscr(scoreDat, &g_GameManager.pscr[0][0][0]);
    if (g_GameManager.practice)
    {
        g_GameManager.globals->highScore =
            g_GameManager
                .pscr[g_GameManager.shotTypeAndCharacter]
                     [g_GameManager.currentStage][g_GameManager.difficulty]
                .score;
        g_GameManager
            .pscr[g_GameManager.shotTypeAndCharacter][g_GameManager.currentStage]
                 [g_GameManager.difficulty]
            .playCount++;
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

#pragma var_order(size, shotTypeAndChar, oldSeed)
// FUNCTION: TH07 0x0042e83e
ZunResult GameManager::AddedCallback(GameManager *arg)
{
    u16 oldSeed;
    i32 shotTypeAndChar;
    u32 size;

    g_Supervisor.checkTiming = FALSE;
    arg->difficultyMask = 1 << arg->difficulty;
    arg->shotTypeAndCharacter = arg->character * 2 + arg->shotType;
    g_Supervisor.currentTime = timeGetTime();
    g_Supervisor.effectiveFramerateMultiplier = 1.0f;
    if (g_Supervisor.curState != SUPERVISOR_STATE_NEXT_STAGE)
    {
        DrawLoadingSprite();
        SAFE_DELETE(arg->defaultCfg);
        SAFE_DELETE(arg->globals);

        size = g_Rng.GetRandomU32InRange(65535) + 16;
        arg->tmpBuffer = malloc(size);
        arg->defaultCfg = new GameConfiguration;
        arg->globals = new ZunGlobals;
        InitializeRngAndCsum();
        *arg->defaultCfg = g_Supervisor.cfg;
        ZunMemory::Free(arg->tmpBuffer);
        arg->powerItemCountForScore = 0;
        arg->cherry = arg->globals->cherryStart;
        arg->cherryPlus = arg->globals->cherryStart;
        if (g_GameManager.difficulty >= 4)
        {
            arg->defaultCfg->lifeCount = 2;
        }
        if (g_GameManager.practice)
        {
            arg->defaultCfg->lifeCount = 8;
        }
        if (Player::RegisterChain(0) != ZUN_SUCCESS)
        {
            g_GameErrorContext.Log(TH_ERR_PLAYER_INIT_FAIL);
            return ZUN_ERROR;
        }
        if (!g_GameManager.replay)
        {
            g_GameManager.SetLivesRemaining(arg->defaultCfg->lifeCount);
            g_GameManager.RegenerateGameIntegrityCsum();
            g_GameManager.SetBombsRemainingAndComputeCsum(
                g_Player.shooterData->initialBombs);
        }
        arg->ResetRegionsPos();
        arg->globals->currentPower = 0.0f;
        arg->RegenerateGameIntegrityCsum();
        arg->playTimeAll = 0;
        arg->globals->guiScore = 0;
        arg->globals->score = 0;
        arg->globals->guiScoreDifference = 0;
        arg->globals->highScore = 100000;
        arg->globals->numRetries = 0;
        arg->globals->grazeInTotal = 0;
        arg->globals->pointItemsCollectedForExtend = 0;
        if (arg->difficulty < 4)
        {
            arg->globals->nextNeededPointItemsForExtend = 50;
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
        if (!g_GameManager.practice)
        {
            switch (arg->difficulty)
            {
            case DIFF_EASY:
                arg->cherryMax = arg->globals->cherryStart + 200000;
                break;
            case DIFF_NORMAL:
                arg->cherryMax = arg->globals->cherryStart + 200000;
                break;
            case DIFF_HARD:
                arg->cherryMax = arg->globals->cherryStart + 250000;
                break;
            case DIFF_LUNATIC:
                arg->cherryMax = arg->globals->cherryStart + 300000;
                break;
            case DIFF_EXTRA:
                arg->cherryMax = arg->globals->cherryStart + 400000;
                arg->cherry = arg->globals->cherryStart + 200000;
                break;
            case DIFF_PHANTASM:
                arg->cherryMax = arg->globals->cherryStart + 400000;
                arg->cherry = arg->globals->cherryStart + 300000;
                break;
            }
        }
        else
        {
            switch (arg->difficulty)
            {
            case DIFF_EASY:
                arg->cherryMax = arg->globals->cherryStart + 200000;
                break;
            case DIFF_NORMAL:
                arg->cherryMax = arg->globals->cherryStart + 200000;
                break;
            case DIFF_HARD:
                arg->cherryMax = arg->globals->cherryStart + 250000;
                break;
            case DIFF_LUNATIC:
                arg->cherryMax = arg->globals->cherryStart + 300000;
                break;
            }
            switch (arg->currentStage + 1)
            {
            case STAGE2:
                arg->cherry = arg->cherryMax;
                break;
            case STAGE3:
                arg->cherryMax += 50000;
                arg->cherry = arg->cherryMax;
                break;
            case STAGE4:
                arg->cherryMax += 100000;
                arg->cherry = arg->cherryMax;
                break;
            case STAGE5:
                arg->cherryMax += 150000;
                arg->cherry = arg->cherryMax;
                break;
            case STAGE6:
                arg->cherryMax += 200000;
                arg->cherry = arg->cherryMax;
                break;
            }
        }
        if (!g_GameManager.replay)
        {
            if (!arg->defaultCfg->slowMode)
            {
                IncrementCappedAgain(
                    &g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty]
                         .playCount,
                    999999);
                IncrementCappedAgain(&g_GameManager.plst.playDataByDifficulty[6].playCount,
                                     999999);
                IncrementCappedAgain(
                    &g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty]
                         .playCountPerShotType[arg->shotTypeAndCharacter],
                    999999);
                IncrementCappedAgain(
                    g_GameManager.plst.playDataByDifficulty[6].playCountPerShotType +
                        arg->shotTypeAndCharacter,
                    999999);
                if (g_Supervisor.curState == SUPERVISOR_STATE_RESTART_FROM_BEGINNING)
                {
                    IncrementCappedAgain(
                        &g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty]
                             .retryCount,
                        999999);
                    IncrementCappedAgain(&g_GameManager.plst.playDataByDifficulty[6].retryCount,
                                         999999);
                }
                if (g_GameManager.practice)
                {
                    IncrementCappedAgain(
                        &g_GameManager.plst
                             .playDataByDifficulty[g_GameManager.difficulty]
                             .practiceCount,
                        999999);
                    IncrementCappedAgain(
                        &g_GameManager.plst.playDataByDifficulty[6].practiceCount, 999999);
                }
            }
        }
        else
        {
            arg->defaultCfg->slowMode = 0;
        }
    }
    else
    {
        arg->globals->guiScore = arg->globals->score;
        arg->globals->guiScoreDifference = 0;
        if (Player::RegisterChain(0) != ZUN_SUCCESS)
        {
            g_GameErrorContext.Log(TH_ERR_PLAYER_INIT_FAIL);
            return ZUN_ERROR;
        }
    }
    arg->subrank = 0;
    arg->globals->pointItemsCollectedThisStage = 0;
    arg->globals->grazeInStage = 0;
    arg->isInPauseMenu = 0;
    arg->currentStage++;
    if (!g_GameManager.replay)
    {
        shotTypeAndChar = g_GameManager.shotTypeAndCharacter;
        if (arg->globals->numRetries == 0 &&
            (i32)(u32)arg->clrd[shotTypeAndChar]
                    .difficultyClearedWithRetries[g_GameManager.difficulty] <
                arg->currentStage - 1)
        {
            arg->clrd[shotTypeAndChar]
                .difficultyClearedWithRetries[g_GameManager.difficulty] =
                arg->currentStage - 1;
        }
        if ((i32)(u32)arg->clrd[shotTypeAndChar]
                .difficultyClearedWithoutRetries[g_GameManager.difficulty] <
            arg->currentStage - 1)
        {
            arg->clrd[shotTypeAndChar]
                .difficultyClearedWithoutRetries[g_GameManager.difficulty] =
                arg->currentStage - 1;
        }
    }
    if (arg->practice)
    {
        switch (arg->currentStage)
        {
        case STAGE1:
            break;
        default:
            arg->globals->currentPower = 128.0f;
            arg->RegenerateGameIntegrityCsum();
            break;
        }
    }
    if (g_GameManager.replay)
    {
        arg->InitializeRank();
        ReplayManager::RegisterChain(TRUE, g_GameManager.replayFilename);
        oldSeed = g_Rng.seed;
        arg->RegenerateGameIntegrityCsum();
        g_Rng.seed = oldSeed;
    }
    arg->stageRngSeed = g_Rng.seed;
    if (Stage::RegisterChain(arg->currentStage) != ZUN_SUCCESS)
    {
        g_GameErrorContext.Log(TH_ERR_STAGE_INIT_FAIL);
        return ZUN_ERROR;
    }

    if (BulletManager::RegisterChain("data/etama.anm") != ZUN_SUCCESS)
    {
        g_GameErrorContext.Log(TH_ERR_BULLET_INIT_FAIL);
        return ZUN_ERROR;
    }

    if (EnemyManager::RegisterChain(
            g_EnemyAnmStageFiles[arg->currentStage].anmPath1,
            g_EnemyAnmStageFiles[arg->currentStage].anmPath2) != ZUN_SUCCESS)
    {
        g_GameErrorContext.Log(TH_ERR_ENEMY_INIT_FAIL);
        return ZUN_ERROR;
    }

    if (g_EclManager.Load(g_EclPaths[arg->currentStage]) != ZUN_SUCCESS)
    {
        g_GameErrorContext.Log(TH_ERR_ECL_INIT_FAIL);
        return ZUN_ERROR;
    }

    if (EffectManager::RegisterChain() != ZUN_SUCCESS)
    {
        g_GameErrorContext.Log(TH_ERR_EFFECT_INIT_FAIL);
        return ZUN_ERROR;
    }

    if (Gui::RegisterChain() != ZUN_SUCCESS)
    {
        g_GameErrorContext.Log(TH_ERR_GUI_INIT_FAIL);
        return ZUN_ERROR;
    }

    if (!g_GameManager.replay)
    {
        // STRING: TH07 0x00497e1c
        ReplayManager::RegisterChain(FALSE, "replay/th7_00.rpy");
    }
    g_Supervisor.LoadAudio(0, g_Stage.stdData->bgmPaths[0]);
    g_Supervisor.LoadAudio(1, g_Stage.stdData->bgmPaths[1]);
    if (arg->currentStage != STAGE6)
    {
        g_Supervisor.PlayLoadedAudio(0);
    }
    else
    {
        g_Supervisor.StopAudio();
        g_Supervisor.LoadAudio(2, "bgm/th07_13b.mid");
    }
    while (g_SoundPlayer.ProcessQueues())
        ;
    arg->isInRetryMenu = 0;
    arg->notInMenu = 1;
    if (g_Supervisor.curState != SUPERVISOR_STATE_NEXT_STAGE)
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
                            g_Rng.GetGenCount());
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0042f2e4
ZunResult GameManager::DeletedCallback(GameManager *arg)
{
    g_Supervisor.StopAudio();
    if (g_Supervisor.cfg.musicMode == MUSIC_MIDI &&
        g_Supervisor.midiOutput)
    {
        g_Supervisor.midiOutput->PlayLoaded(30);
    }
    while (g_SoundPlayer.ProcessQueues())
        ;
    Stage::CutChain();
    BulletManager::CutChain();
    Player::CutChain();
    EnemyManager::CutChain();
    g_EclManager.Unload();
    EffectManager::CutChain();
    Gui::CutChain();
    ReplayManager::StopRecording();
    if (!g_GameManager.replay)
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
    if (g_Chain.AddToCalcChain(&g_GameManagerCalcChain, 2))
    {
        return ZUN_ERROR;
    }

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
        this->rank.rank++;
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
        this->rank.rank--;
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
    if (this->cherry >= this->cherryMax && oldCherry != this->cherry)
    {
        g_Gui.ShowStatusPopup(this->cherry - this->globals->cherryStart, 3);
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
    if (this->cherry >= this->cherryMax && oldCherry != this->cherry)
    {
        g_Gui.ShowStatusPopup(this->cherry - this->globals->cherryStart, 3);
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
ZunBool GameManager::HasReachedMaxClearsAnyDifficulty(i32 shotType)
{
    return this->clrd[shotType].difficultyClearedWithRetries[DIFF_EASY] == 99 ||
           this->clrd[shotType].difficultyClearedWithRetries[DIFF_NORMAL] == 99 ||
           this->clrd[shotType].difficultyClearedWithRetries[DIFF_HARD] == 99 ||
           this->clrd[shotType].difficultyClearedWithRetries[DIFF_LUNATIC] == 99;
}

// FUNCTION: TH07 0x0042f853
ZunBool GameManager::HasUnlockedPhantasm(i32 shotType)
{
    i32 numSuccesses = 0;
    for (i32 i = 0; i < SPELLCARD_COUNT; i++)
    {
        if (this->catk[i].numSuccessesPerShot[SHOT_COUNT] > 0)
        {
            numSuccesses++;
        }
    }
    if (numSuccesses >= 60 &&
        this->clrd[shotType].difficultyClearedWithRetries[DIFF_EXTRA] == 99)
    {
        this->clrd[shotType].difficultyClearedWithRetries[DIFF_PHANTASM] = 99;
    }
    return this->clrd[shotType].difficultyClearedWithRetries[DIFF_PHANTASM] == 99;
}

// FUNCTION: TH07 0x0042f8de
ZunBool GameManager::HasReachedMaxClearsAnyShotType()
{
    return HasReachedMaxClearsAnyDifficulty(SHOT_REIMU_A) ||
           HasReachedMaxClearsAnyDifficulty(SHOT_REIMU_B) ||
           HasReachedMaxClearsAnyDifficulty(SHOT_MARISA_A) ||
           HasReachedMaxClearsAnyDifficulty(SHOT_MARISA_B) ||
           HasReachedMaxClearsAnyDifficulty(SHOT_SAKUYA_A) ||
           HasReachedMaxClearsAnyDifficulty(SHOT_SAKUYA_B);
}

#pragma var_order(spellCardsCaptured, i, j)
// FUNCTION: TH07 0x0042f94c
ZunBool GameManager::HasUnlockedPhantasmAndMaxClears()
{
    i32 j;
    i32 i;
    i32 spellCardsCaptured;

    spellCardsCaptured = 0;
    for (i = 0; i < SPELLCARD_COUNT; i++)
    {
        if (this->catk[i].numSuccessesPerShot[SHOT_COUNT] > 0)
        {
            spellCardsCaptured++;
        }
    }
    if (spellCardsCaptured >= 60)
    {
        for (j = 0; j < SHOT_COUNT; j++)
        {
            if (this->clrd[j].difficultyClearedWithRetries[4] == 99)
            {
                this->clrd[j].difficultyClearedWithRetries[5] = 99;
            }
        }
    }

    if (this->clrd[SHOT_REIMU_A].difficultyClearedWithRetries[5] == 99)
    {
        spellCardsCaptured = 60;
    }

    return this->clrd[SHOT_REIMU_A].difficultyClearedWithRetries[5] == 99 ||
           this->clrd[SHOT_REIMU_B].difficultyClearedWithRetries[5] == 99 ||
           this->clrd[SHOT_MARISA_A].difficultyClearedWithRetries[5] == 99 ||
           this->clrd[SHOT_MARISA_B].difficultyClearedWithRetries[5] == 99 ||
           this->clrd[SHOT_SAKUYA_A].difficultyClearedWithRetries[5] == 99 ||
           this->clrd[SHOT_SAKUYA_B].difficultyClearedWithRetries[5] == 99;
}
