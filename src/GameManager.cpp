#include "GameManager.hpp"

#include <cstdio>

#include "AnmIdx.hpp"
#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "EclManager.hpp"
#include "EnemyManager.hpp"
#include "GameErrorContext.hpp"
#include "GameWindow.hpp"
#include "Gui.hpp"
#include "Player.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "Touch.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"
#include "graphics/ZunGraphics.hpp"

i32 g_RankArray[6][3] = {
    {16, 12, 20}, {16, 10, 32}, {16, 10, 32}, {16, 10, 32}, {16, 15, 16}, {16, 15, 16},
};

// ZUN name: Stg
// GLOBAL: TH07 0x00626270
GameManager g_GameManager;

ChainElem g_GameManagerCalcChain;

ChainElem g_GameManagerDrawChain;

GameManager::GameManager()
{
    memset(this, 0, sizeof(GameManager));
    this->arcadeRegionTopLeftPos.x = 32.0f;
    this->arcadeRegionTopLeftPos.y = 16.0f;
    this->arcadeRegionSize.x = 384.0f;
    this->arcadeRegionSize.y = 448.0f;
    this->demoIdx = 2;
    this->phantasmUnlocked = 1;
}

void GameManager::AddLivesRemaining(i32 amount)
{
    if (CheckGameIntegrity())
    {
        NUKE_SUPERVISOR();
    }
    this->globals->livesRemaining += (f32)amount;
    RegenerateGameIntegrityCsum();
}

void GameManager::AddBombsRemaining(i32 amount)
{
    if (CheckGameIntegrity())
    {
        NUKE_SUPERVISOR();
    }
    this->globals->bombsRemaining += (f32)amount;
    RegenerateGameIntegrityCsum();
}

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

i32 GameManager::ComputeGameIntegrityCsum()
{
    i32 csum = ByteCsumAccumulator((u8 *)g_GameManager.globals->rng1,
                                   (u8 *)&this->globals->curCsum - (u8 *)this->globals->rng1);
    csum += ByteCsumAccumulator((u8 *)g_GameManager.globals->csumData,
                                sizeof(g_GameManager.globals->csumData));
    csum += ByteCsumAccumulator((u8 *)g_GameManager.defaultCfg, sizeof(GameConfiguration));
    csum += ByteCsumAccumulator((u8 *)&g_Supervisor.cfg, sizeof(GameConfiguration));
    return csum;
}

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

void GameManager::Pause()
{
    this->isInPauseMenu = 1;
    g_GameManager.arcadeRegionTopLeftPos.x = 32.0f;
    g_GameManager.arcadeRegionTopLeftPos.y = 16.0f;
    g_GameManager.arcadeRegionSize.x = 384.0f;
    g_GameManager.arcadeRegionSize.y = 448.0f;
    this->isPaused = 1;
    g_Player.prevPositionCenter = g_Player.positionCenter;
    g_Player.prevOptionsPosition[0] = g_Player.optionsPosition[0];
    g_Player.prevOptionsPosition[1] = g_Player.optionsPosition[1];
    g_Stage.prevCam = g_Stage.cam;
    g_Stage.prevPos = g_Stage.pos;
    if (g_GameManager.currentStage != 6 || g_Gui.frameCounter >= 300)
    {
        g_SoundPlayer.PushCommand(AUDIO_PAUSE, 0, "Pause");
    }
    g_SoundPlayer.PlaySoundByIdx(SOUND_37, 0);
    g_Supervisor.UpdateTime();
}

u32 GameManager::OnUpdate(GameManager *arg)
{
    u32 scoreIncrement;
    u32 i;
    i32 csum;

    if (arg->isInRetryMenu == 0 && arg->isInPauseMenu == 0 && arg->demo == 0 &&
        (arg->slowModeSlowActive == 0 && WAS_PRESSED_RAW(TH_BUTTON_MENU)))
    {
        arg->Pause();
    }
    g_Supervisor.viewport.x = arg->arcadeRegionTopLeftPos.x;
    g_Supervisor.viewport.y = arg->arcadeRegionTopLeftPos.y;
    g_Supervisor.viewport.width = arg->arcadeRegionSize.x;
    g_Supervisor.viewport.height = arg->arcadeRegionSize.y;
    g_Supervisor.viewport.minZ = 0.0f;
    g_Supervisor.viewport.maxZ = 1.0f;
    g_AnmManager->SetCameraMode(255);
    if (g_GameManager.replay && g_GameManager.replayStage == 1 && !g_Gui.HasCurrentMsgIdx())
    {
        arg->bulletLagTime++;
        if ((g_Supervisor.curFps < 20 && arg->bulletLagTime % 3 != 0) ||
            (g_Supervisor.curFps >= 20 && g_Supervisor.curFps < 30 &&
             arg->bulletLagTime % 2 != 0) ||
            (g_Supervisor.curFps >= 30 && g_Supervisor.curFps < 40 &&
             arg->bulletLagTime % 3 == 0) ||
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
            BombEffects::RegisterChain(2, 120, 0, 0, 0);
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
    for (i = 0; i < 7; i++)
    {
        if (arg->globals->rng1[i] < 6543 || arg->globals->rng1[i] > 106543)
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    for (i = 0; i < 2; i++)
    {
        if (arg->globals->rngFloat2[i] < 6543.0f || arg->globals->rngFloat2[i] > 106543.0f)
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    arg->notInMenu = !arg->isInRetryMenu && !arg->isInPauseMenu;
    for (i = 0; i < 2; i++)
    {
        if (arg->globals->rngFloat1[i] < 6543.0f || arg->globals->rngFloat1[i] > 106543.0f)
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    for (i = 0; i < 8; i++)
    {
        if (arg->globals->rng2[i] < 6543 || arg->globals->rng2[i] > 106543)
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    g_Supervisor.gfxDevice->SetClearColor(g_Stage.skyFog.color);
    g_Supervisor.gfxDevice->Clear(CLEAR_DEPTH_BUFFER);
    if (arg->isInPauseMenu == 1 || arg->isInPauseMenu == 2 || arg->isInRetryMenu)
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
            arg->globals->guiScoreDifference = arg->globals->score - arg->globals->guiScore;
        }
        arg->globals->guiScore = arg->globals->guiScore + arg->globals->guiScoreDifference;
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
    for (i = 0; i < 3; i++)
    {
        if (arg->globals->rngFloat3[i] < 6543.0f || arg->globals->rngFloat3[i] > 106543.0f)
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    for (i = 0; i < 2; i++)
    {
        if (arg->globals->rngFloat4[i] < 6543.0f || arg->globals->rngFloat4[i] > 106543.0f)
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    for (i = 0; i < 5; i++)
    {
        if (arg->globals->csumData[i] < 6543 || arg->globals->csumData[i] > 106543)
        {
            g_GameManager.csumFloat = -9999.0f;
        }
    }
    if (g_GameManager.defaultCfg->slowMode)
    {
        g_GameManager.slowModeSlowActive = 0;
        arg->bulletLagTime = arg->bulletLagTime + 1;
        if ((g_BulletManager.bulletCount >= 320 && arg->bulletLagTime % 3 == 0) ||
            (g_BulletManager.bulletCount < 320 && g_BulletManager.bulletCount >= 224 &&
             arg->bulletLagTime % 4 == 0) ||
            (g_BulletManager.bulletCount < 224 && g_BulletManager.bulletCount >= 128 &&
             arg->bulletLagTime % 5 == 0))
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

u32 GameManager::OnDraw(GameManager *arg)
{
    if (arg->isInPauseMenu)
    {
        arg->isInPauseMenu = 2;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

void GameManager::DrawLoadingSprite()
{
    ZunVec3 spritePos;
    AnmVm spriteVm;
    ZunRect rect;

    rect.left = 0.0f;
    rect.top = 0.0f;
    rect.right = 640.0f;
    rect.bottom = 480.0f;
    g_AnmManager->InitializeAndSetActiveSprite(&spriteVm, ANM_SPRITE_ASCII_LOADING);
    spritePos.x = 528.0f;
    spritePos.y = 448.0f;
    spritePos.z = 0.0f;
    spriteVm.pos = spritePos;
    g_Supervisor.gfxDevice->BeginFrame();
    ScreenEffect::DrawSquare(&rect, 0xa0000000);
    g_AnmManager->DrawNoRotation(&spriteVm);
    g_Supervisor.gfxDevice->EndFrame();
    g_Supervisor.gfxDevice->SwapBuffers();
}

void GameManager::InitializeRank()
{
    this->rank.rank = g_RankArray[g_GameManager.difficulty][0];
    this->rank.minRank = g_RankArray[g_GameManager.difficulty][1];
    this->rank.maxRank = g_RankArray[g_GameManager.difficulty][2];
}

void GameManager::InitializeRngAndCsum()
{
    u32 i;

    g_GameManager.globals->cherryStart = g_Rng.GetRandomU32InRange(100000) + 6543;
    for (i = 0; i < 7; i++)
    {
        g_GameManager.globals->rng1[i] = g_Rng.GetRandomU32InRange(100000) + 6543;
    }
    for (i = 0; i < 8; i++)
    {
        g_GameManager.globals->rng2[i] = g_Rng.GetRandomU32InRange(100000) + 6543;
    }
    for (i = 0; i < 2; i++)
    {
        g_GameManager.globals->rngFloat1[i] = g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
    }
    for (i = 0; i < 2; i++)
    {
        g_GameManager.globals->rngFloat2[i] = g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
    }
    for (i = 0; i < 3; i++)
    {
        g_GameManager.globals->rngFloat3[i] = g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
    }
    for (i = 0; i < 2; i++)
    {
        g_GameManager.globals->rngFloat4[i] = g_Rng.GetRandomFloatInRange(100000.0f) + 6543.0f;
    }
    for (i = 0; i < 5; i++)
    {
        g_GameManager.globals->csumData[i] = g_Rng.GetRandomU32InRange(100000) + 6543;
    }
    g_GameManager.globals->curCsum = g_GameManager.globals->rng1[2];
    i32 csum = g_GameManager.ComputeGameIntegrityCsum();
    g_GameManager.globals->csumAsSum = csum;
    g_GameManager.csumFloat = (f32)csum + (f32)g_GameManager.globals->rng2[3];
}

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
        catk->base.magic = 0x4b544143;
        catk->base.th7kLen2 = sizeof(Catk);
        catk->base.th7kLen = sizeof(Catk);
        catk->base.version = 1;
        catk->idx = (i16)i;
        for (j = 0; j < 7; j++)
        {
            catk->numAttemptsPerShot[j] = 0;
            catk->numSuccessesPerShot[j] = 0;
            catk->highScorePerShot[j] = 0;
        }
    }
    scoreDat = OpenScore(FileSystem::GetPrefPath("score.dat").c_str());
    if (!scoreDat)
    {
        g_GameErrorContext.Log("error : スコアファイルの読み取りに失敗しました\n");
        return ZUN_ERROR;
    }

    g_GameManager.globals->highScore =
        GetHighScore(scoreDat, NULL, (u32)g_GameManager.shotTypeAndCharacter,
                     g_GameManager.difficulty, &g_GameManager.globals->highScoreNumContinues);
    ParseCatk(scoreDat, g_GameManager.catk);
    ParseClrd(scoreDat, g_GameManager.clrd);
    ParsePscr(scoreDat, &g_GameManager.pscr[0][0][0]);
    if (g_GameManager.practice)
    {
        g_GameManager.globals->highScore =
            g_GameManager
                .pscr[g_GameManager.shotTypeAndCharacter][g_GameManager.currentStage]
                     [g_GameManager.difficulty]
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

void IncrementCappedAgain(u32 *param, u32 cap)
{
    // cap seemingly completely unused here
    if (*param < cap)
    {
        (*param)++; // otherwise duplicate of incrementcapped from asciimanager
    }
}

ZunResult GameManager::AddedCallback(GameManager *arg)
{
    u16 oldSeed;
    i32 shotTypeAndChar;
    u32 size;

    Touch::ResetRunUsage();

    g_GameWindow.ResetAccumulator();
    g_Supervisor.checkTiming = 0;
    arg->difficultyMask = 1 << arg->difficulty;
    arg->shotTypeAndCharacter = arg->character * 2 + arg->shotType;
    g_Supervisor.currentTime = SDL_GetTicks();
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
        free(arg->tmpBuffer);
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
            g_GameErrorContext.Log("error : プレイヤーの初期化に失敗しました\n");
            return ZUN_ERROR;
        }
        if (!g_GameManager.replay)
        {
            g_GameManager.SetLivesRemaining(arg->defaultCfg->lifeCount);
            g_GameManager.RegenerateGameIntegrityCsum();
            g_GameManager.SetBombsRemainingAndComputeCsum(g_Player.shooterData->initialBombs);
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
            case 2:
                arg->cherry = arg->cherryMax;
                break;
            case 3:
                arg->cherryMax += 50000;
                arg->cherry = arg->cherryMax;
                break;
            case 4:
                arg->cherryMax += 100000;
                arg->cherry = arg->cherryMax;
                break;
            case 5:
                arg->cherryMax += 150000;
                arg->cherry = arg->cherryMax;
                break;
            case 6:
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
                    &g_GameManager.plst.playDataByDifficulty[g_GameManager.difficulty].playCount,
                    999999);
                IncrementCappedAgain(&g_GameManager.plst.playDataByDifficulty[6].playCount, 999999);
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
                    IncrementCappedAgain(&((Plst *)(g_GameManager.pscr + 6))
                                              ->playDataByDifficulty[g_GameManager.difficulty]
                                              .clearCount,
                                         999999);
                    IncrementCappedAgain(&g_GameManager.plst.playDataByDifficulty[6].clearCount,
                                         999999);
                }
                if (g_GameManager.practice)
                {
                    IncrementCappedAgain(&((Plst *)(g_GameManager.pscr + 6))
                                              ->playDataByDifficulty[g_GameManager.difficulty]
                                              .extraClearCount,
                                         999999);
                    IncrementCappedAgain(
                        &g_GameManager.plst.playDataByDifficulty[6].extraClearCount, 999999);
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
            g_GameErrorContext.Log("error : プレイヤーの初期化に失敗しました\n");
            return ZUN_ERROR;
        }
    }
    arg->subrank = 0;
    arg->globals->pointItemsCollectedThisStage = 0;
    arg->globals->grazeInStage = 0;
    arg->isInPauseMenu = 0;
    arg->currentStage = arg->currentStage + 1;
    if (!g_GameManager.replay)
    {
        shotTypeAndChar = g_GameManager.shotTypeAndCharacter;
        if (arg->globals->numRetries == 0 &&
            (i32)(u32)arg->clrd[shotTypeAndChar]
                    .difficultyClearedWithRetries[g_GameManager.difficulty] < arg->currentStage - 1)
        {
            arg->clrd[shotTypeAndChar].difficultyClearedWithRetries[g_GameManager.difficulty] =
                arg->currentStage - 1;
        }
        if ((i32)(u32)arg->clrd[shotTypeAndChar]
                .difficultyClearedWithoutRetries[g_GameManager.difficulty] < arg->currentStage - 1)
        {
            arg->clrd[shotTypeAndChar].difficultyClearedWithoutRetries[g_GameManager.difficulty] =
                arg->currentStage - 1;
        }
    }
    if (arg->practice)
    {
        switch (arg->currentStage)
        {
        case 1:
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
        ReplayManager::RegisterChain(1, g_GameManager.replayFilename);
        oldSeed = g_Rng.seed;
        arg->RegenerateGameIntegrityCsum();
        g_Rng.seed = oldSeed;
    }
    arg->stageRngSeed = g_Rng.seed;
    if (Stage::RegisterChain(arg->currentStage) != ZUN_SUCCESS)
    {
        g_GameErrorContext.Log("error : 背景データの初期化に失敗しました\n");
        return ZUN_ERROR;
    }

    if (BulletManager::RegisterChain("data/etama.anm") != ZUN_SUCCESS)
    {
        g_GameErrorContext.Log("error : 敵弾の初期化に失敗しました\n");
        return ZUN_ERROR;
    }

    if (EnemyManager::RegisterChain(g_EnemyAnmStageFiles[arg->currentStage].anmPath1,
                                    g_EnemyAnmStageFiles[arg->currentStage].anmPath2) !=
        ZUN_SUCCESS)
    {
        g_GameErrorContext.Log("error : 敵の初期化に失敗しました\n");
        return ZUN_ERROR;
    }

    if (g_EclManager.Load(g_EclPaths[arg->currentStage]) != ZUN_SUCCESS)
    {
        g_GameErrorContext.Log("error : 敵頭脳の初期化に失敗しました\n");
        return ZUN_ERROR;
    }

    if (EffectManager::RegisterChain() != ZUN_SUCCESS)
    {
        g_GameErrorContext.Log("error : エフェクトの初期化に失敗しました\n");
        return ZUN_ERROR;
    }

    if (Gui::RegisterChain() != ZUN_SUCCESS)
    {
        g_GameErrorContext.Log("error : 2D表示の初期化に失敗しました\n");
        return ZUN_ERROR;
    }

    if (!g_GameManager.replay)
    {
        ReplayManager::RegisterChain(0, "replay/th7_00.rpy");
    }
    g_Supervisor.LoadAudio(0, g_Stage.stdData->bgmPaths[0]);
    g_Supervisor.LoadAudio(1, g_Stage.stdData->bgmPaths[1]);
    if (arg->currentStage != 6)
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
    Supervisor::DebugPrint("random seed %d %d\n", (u32)g_Rng.seed, g_Rng.GetGenCount());
    return ZUN_SUCCESS;
}

ZunResult GameManager::DeletedCallback(GameManager *arg)
{
    g_Supervisor.StopAudio();
    if (g_Supervisor.cfg.musicMode == MUSIC_MIDI && g_Supervisor.midiOutput)
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

ZunResult GameManager::RegisterChain()
{
    GameManager *mgr = &g_GameManager;
    g_GameManagerCalcChain.callback = (ChainCallback)OnUpdate;
    g_GameManagerCalcChain.addedCallback = NULL;
    g_GameManagerCalcChain.deletedCallback = NULL;
    g_GameManagerCalcChain.addedCallback = (ChainLifecycleCallback)AddedCallback;
    g_GameManagerCalcChain.deletedCallback = (ChainLifecycleCallback)DeletedCallback;
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

void GameManager::IncreaseCherry(i32 amount)
{
    this->cherry = this->cherry + amount;
    if (this->cherry > this->cherryMax)
    {
        this->cherry = this->cherryMax;
    }
}

void GameManager::IncreaseCherryMax(i32 amount)
{
    this->cherryMax = this->cherryMax + amount;
    if (this->cherryMax >= this->globals->cherryStart + 9999990)
    {
        this->cherryMax = this->globals->cherryStart + 9999990;
    }
}

i32 GameManager::HasReachedMaxClears(i32 shotType)
{
    return this->clrd[shotType].difficultyClearedWithRetries[0] != 99 &&
                   this->clrd[shotType].difficultyClearedWithRetries[1] != 99 &&
                   this->clrd[shotType].difficultyClearedWithRetries[2] != 99 &&
                   this->clrd[shotType].difficultyClearedWithRetries[3] != 99
               ? 0
               : 1;
}

i32 GameManager::HasUnlockedPhantom(i32 shotType)
{
    i32 numSuccesses = 0;
    for (i32 i = 0; i < SPELLCARD_COUNT; i++)
    {
        if (this->catk[i].numSuccessesPerShot[6] > 0)
        {
            numSuccesses++;
        }
    }
    if (numSuccesses >= 60 && this->clrd[shotType].difficultyClearedWithRetries[4] == 99)
    {
        this->clrd[shotType].difficultyClearedWithRetries[5] = 99;
    }
    return this->clrd[shotType].difficultyClearedWithRetries[5] == 99;
}

i32 GameManager::HasReachedMaxClearsAllShotTypes()
{
    return HasReachedMaxClears(0) == 0 && HasReachedMaxClears(1) == 0 &&
                   HasReachedMaxClears(2) == 0 && HasReachedMaxClears(3) == 0 &&
                   HasReachedMaxClears(4) == 0 && HasReachedMaxClears(5) == 0
               ? 0
               : 1;
}

i32 GameManager::HasUnlockedPhantomAndMaxClears()
{
    i32 j;
    i32 i;
    i32 spellCardsCaptured;

    spellCardsCaptured = 0;
    for (i = 0; i < SPELLCARD_COUNT; i++)
    {
        if (this->catk[i].numSuccessesPerShot[6] > 0)
        {
            spellCardsCaptured++;
        }
    }
    if (spellCardsCaptured >= 60)
    {
        for (j = 0; j < 6; j++)
        {
            if (this->clrd[j].difficultyClearedWithRetries[4] == 99)
            {
                this->clrd[j].difficultyClearedWithRetries[5] = 99;
            }
        }
    }

    if (this->clrd[0].difficultyClearedWithRetries[5] == 99)
    {
        spellCardsCaptured = 60;
    }

    return this->clrd[0].difficultyClearedWithRetries[5] == 99 ||
           this->clrd[1].difficultyClearedWithRetries[5] == 99 ||
           this->clrd[2].difficultyClearedWithRetries[5] == 99 ||
           this->clrd[3].difficultyClearedWithRetries[5] == 99 ||
           this->clrd[4].difficultyClearedWithRetries[5] == 99 ||
           this->clrd[5].difficultyClearedWithRetries[5] == 99;
}
