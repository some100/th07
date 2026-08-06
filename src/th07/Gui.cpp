#include "Gui.hpp"

#include <stdio.h>

#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "BulletManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "EnemyManager.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameManager.hpp"
#include "ItemManager.hpp"
#include "Player.hpp"
#include "SoundPlayer.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"
#include "utils.hpp"

// GLOBAL: TH07 0x0049f618
D3DCOLOR g_SpellcardTimeColors[4] = {
    0xa0d0ff,
    0xa080ff,
    0xe080c0,
    0xff4040,
};

// GLOBAL: TH07 0x0049fbf0
Gui g_Gui;

// GLOBAL: TH07 0x0062f914
ChainElem g_GuiCalcChain;

// GLOBAL: TH07 0x0062f8f4
ChainElem g_GuiDrawChain;

// FUNCTION: TH07 0x00427ae0
i32 Gui::IsStageFinished()
{
    return this->impl->stageClearTextVm.activeSpriteIdx >= 0 &&
           this->impl->stageClearTextVm.isStopped;
}

// FUNCTION: TH07 0x00427b21
void Gui::EndPlayerSpellcard()
{
    this->impl->bombSpellcardName.pendingInterrupt = 1;
    this->impl->bombSpellcardNameBg.SetInterrupt(2);
}

// FUNCTION: TH07 0x00427b54
void Gui::EndEnemySpellcard()
{
    this->impl->enemySpellcardName.pendingInterrupt = 1;
    this->impl->enemySpellcardNameBg.SetInterrupt(2);
    this->impl->spellcardBonusIndicator.SetInterrupt(2);
}

// FUNCTION: TH07 0x00427ba2
void Gui::ClearActiveSprites()
{
    this->impl->stageClearTextVm.activeSpriteIdx = -1;
    this->impl->stageClearBonusTextVm.activeSpriteIdx = -1;
    this->impl->stageTransitionSnapshotVm.activeSpriteIdx = -1;
    this->impl->activeTransitionQuads = 0;
}

// FUNCTION: TH07 0x00427be2
i32 Gui::IsDialogueSkippable()
{
    return this->impl->msg.dialogueSkippable;
}

// FUNCTION: TH07 0x00427bf8
void Gui::ShowBonusScore(i32 score)
{
    this->impl->bonusScore.pos = Float3(416.0f, 48.0f, 0.0f);
    this->impl->bonusScore.isShown = 1;
    this->impl->bonusScore.timer = 0;
    this->impl->bonusScore.fmtArg = score;
    g_Supervisor.renderSkipFrames = 2;
}

// FUNCTION: TH07 0x00427c81
void Gui::ShowFullPowerMode(i32 fmtArg, i32 isShown)
{
    this->impl->fullPowerMode.pos = Float3(416.0f, 168.0f, 0.0f);
    this->impl->fullPowerMode.isShown = isShown;
    this->impl->fullPowerMode.timer = 0;
    this->impl->fullPowerMode.fmtArg = fmtArg;
    g_Supervisor.renderSkipFrames = 2;
}

// FUNCTION: TH07 0x00427d09
void Gui::ShowSpellcardBonus(i32 fmtArg)
{
    this->impl->spellCardBonus.pos = Float3(224.0f, 16.0f, 0.0f);
    this->impl->spellCardBonus.isShown = 1;
    this->impl->spellCardBonus.timer = 0;
    this->impl->spellCardBonus.fmtArg = fmtArg;
    g_Supervisor.renderSkipFrames = 2;
}

// FUNCTION: TH07 0x00427d92
void Gui::CopyTemplateSpriteToSprite(i32 spriteIdx)
{
    RECT srcRect;
    RECT dstRect;

    srcRect.left = g_AnmManager->GetSprite(0x609)->startPixelInclusive.x;
    srcRect.top = g_AnmManager->GetSprite(0x609)->startPixelInclusive.y;
    srcRect.right = g_AnmManager->GetSprite(0x609)->endPixelInclusive.x;
    srcRect.bottom = g_AnmManager->GetSprite(0x609)->endPixelInclusive.y;
    dstRect.left = g_AnmManager->sprites[spriteIdx].startPixelInclusive.x;
    dstRect.top = g_AnmManager->sprites[spriteIdx].startPixelInclusive.y;
    dstRect.right = g_AnmManager->sprites[spriteIdx].endPixelInclusive.x;
    dstRect.bottom = g_AnmManager->sprites[spriteIdx].endPixelInclusive.y;
    g_AnmManager->CopyTexture(21, 22, &srcRect, &dstRect);
}

// FUNCTION: TH07 0x00427e7c
u32 Gui::OnUpdate(Gui *arg)
{
    if (g_GameManager.isTimeStopped)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    if (arg->impl->transitionToScoreScreen)
    {
        g_Supervisor.curState = 3;
        arg->impl->transitionToScoreScreen = 0;
    }
    arg->UpdateGui();
    arg->impl->RunMsg();
    arg->frameCounter = arg->frameCounter + 1;
    if (g_GameManager.currentStage == 6 && arg->frameCounter == 300)
    {
        g_Supervisor.PlayLoadedAudio(0);
    }
    if (IS_PRESSED_RAW(TH_BUTTON_SKIP) && g_Supervisor.renderSkipFrames < 8)
    {
        g_Supervisor.renderSkipFrames = 8;
    }

    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(stringPos, fmtArg)
// FUNCTION: TH07 0x00427f22
u32 Gui::OnDraw(Gui *arg)
{
    char fmtArg[32];
    Float3 stringPos;

    g_AnmManager->offset.y = 0.0f;
    g_AnmManager->offset.x = 0.0f;
    if (arg->impl->finishedStage)
    {
        stringPos.x = 144.0f;
        stringPos.y = 128.0f;
        stringPos.z = 0.0f;
        g_AsciiManager.color = 0xffffff40;
        if (g_GameManager.currentStage < 6)
        {
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos, "Stage Clear");
        }
        else
        {
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos, "All Clear!");
        }
        stringPos.y = stringPos.y + 32.0f;
        g_AsciiManager.color = 0xffffffff;
        AsciiManager::AddFormatText(&g_AsciiManager, &stringPos, "Clear  = %8d",
                                    g_GameManager.currentStage * 1000000);
        stringPos.y = stringPos.y + 16.0f;
        g_AsciiManager.color = 0xffe0e0ff;
        AsciiManager::AddFormatText(&g_AsciiManager, &stringPos, "Point  = %8d",
                                    arg->impl->clearPointItems * 50000);
        stringPos.y = stringPos.y + 16.0f;
        g_AsciiManager.color = 0xffd0d0ff;
        AsciiManager::AddFormatText(&g_AsciiManager, &stringPos, "Graze  = %8d",
                                    arg->impl->clearGraze * 500);
        stringPos.y = stringPos.y + 16.0f;
        g_AsciiManager.color = 0xffd0d0ff;
        AsciiManager::AddFormatText(&g_AsciiManager, &stringPos, "Cherry = %8d",
                                    arg->impl->clearCherryMax * 10);
        if (g_GameManager.currentStage >= 7 ||
            (g_GameManager.currentStage == 6 &&
             !g_GameManager.practice &&
             (!g_GameManager.replay ||
              g_ReplayManager->data->head.stageReplayData[4].data != NULL)))
        {
            stringPos.y = stringPos.y + 16.0f;
            g_AsciiManager.color = 0xffffff80;
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos, "Player =%9d",
                                        (i32)g_GameManager.globals->livesRemaining *
                                            20000000);
            stringPos.y = stringPos.y + 16.0f;
            g_AsciiManager.color = 0xffffff80;
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos, "Bomb   = %8d",
                                        (i32)g_GameManager.globals->bombsRemaining *
                                            4000000);
        }
        stringPos.y = stringPos.y + 32.0f;
        switch (g_GameManager.difficulty)
        {
        case DIFF_EASY:
            g_AsciiManager.color = 0xffff8080;
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                        "Easy Rank    *0.5");
            break;
        case DIFF_NORMAL:
            g_AsciiManager.color = 0xffff8080;
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                        "Normal Rank  *1.0");
            break;
        case DIFF_HARD:
            g_AsciiManager.color = 0xffff8080;
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                        "Hard Rank    *1.2");
            break;
        case DIFF_LUNATIC:
            g_AsciiManager.color = 0xffff8080;
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                        "Lunatic Rank *1.5");
            break;
        case DIFF_EXTRA:
            g_AsciiManager.color = 0xffff8080;
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                        "Extra Rank   *2.0");
            break;
        case DIFF_PHANTASM:
            g_AsciiManager.color = 0xffff8080;
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                        "Phantasm Rank*2.0");
            break;
        }
        stringPos.y = stringPos.y + 16.0f;
        if (g_GameManager.difficulty < 4 && !g_GameManager.practice)
        {
            switch (g_GameManager.defaultCfg->lifeCount)
            {
            case 3:
                g_AsciiManager.color = 0xffff8080;
                AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                            "Player Penalty*0.5");
                stringPos.y = stringPos.y + 16.0f;
                break;
            case 4:
                g_AsciiManager.color = 0xffff8080;
                AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                            "Player Penalty*0.2");
                stringPos.y = stringPos.y + 16.0f;
                break;
            }
        }
        g_AsciiManager.color = 0xffffffff;
        AsciiManager::AddFormatText(&g_AsciiManager, &stringPos, "Total = %8d0",
                                    arg->impl->stageClearBonus);
        g_AsciiManager.color = 0xffffffff;
    }
    arg->impl->DrawDialogue();
    arg->DrawStageElements();
    arg->DrawGameScene();
    g_AsciiManager.isGui = 1;
    if (arg->impl->bonusScore.isShown)
    {
        g_AsciiManager.color = 0xffffff80;
        AsciiManager::AddFormatText(&g_AsciiManager, &arg->impl->bonusScore.pos,
                                    "BONUS %8d", arg->impl->bonusScore.fmtArg);
        g_AsciiManager.color = 0xffffffff;
    }
    switch (arg->impl->fullPowerMode.isShown)
    {
    case 1:
        g_AsciiManager.color = 0xffc0b0ff;
        AsciiManager::AddFormatText(
            &g_AsciiManager, &arg->impl->fullPowerMode.pos, "Full Power Mode!");
        g_AsciiManager.color = 0xffffffff;
        break;
    case 2:
        g_AsciiManager.scale.x = 0.9f;
        g_AsciiManager.scale.y = 1.0f;
        g_AsciiManager.fontSpacing = 11;
        g_AsciiManager.color = 0xffe0b0ff;
        AsciiManager::AddFormatText(&g_AsciiManager,
                                    &arg->impl->fullPowerMode.pos,
                                    "Supernatural Border!!");
        g_AsciiManager.color = 0xffffffff;
        g_AsciiManager.scale.x = 1.0f;
        g_AsciiManager.scale.y = 1.0f;
        g_AsciiManager.fontSpacing = 14;
        break;
    case 3:
        g_AsciiManager.color = 0xffc0b0ff;
        AsciiManager::AddFormatText(
            &g_AsciiManager, &arg->impl->fullPowerMode.pos, "CherryPoint Max!");
        g_AsciiManager.color = 0xffffffff;
        break;
    case 4:
        g_AsciiManager.scale.x = 0.9f;
        g_AsciiManager.scale.y = 1.0f;
        g_AsciiManager.fontSpacing = 11;
        g_AsciiManager.color = 0xffe0b0ff;
        AsciiManager::AddFormatText(
            &g_AsciiManager, &arg->impl->fullPowerMode.pos, "Border Bonus %7d",
            arg->impl->fullPowerMode.fmtArg);
        g_AsciiManager.color = 0xffffffff;
        g_AsciiManager.scale.x = 1.0f;
        g_AsciiManager.scale.y = 1.0f;
        g_AsciiManager.fontSpacing = 14;
        break;
    }
    if (arg->impl->spellCardBonus.isShown)
    {
        g_AsciiManager.color = 0xffff0000;
        arg->impl->spellCardBonus.pos.x = (384.0f - strlen("Spell Card Bonus!") * 16.0f) / 2.0f + 32.0f;
        arg->impl->spellCardBonus.pos.y = 80.0f;
        AsciiManager::AddFormatText(
            &g_AsciiManager, &arg->impl->spellCardBonus.pos, "Spell Card Bonus!");
        arg->impl->spellCardBonus.pos.y =
            arg->impl->spellCardBonus.pos.y + 16.0f;
        sprintf(fmtArg, "+%d", arg->impl->spellCardBonus.fmtArg);
        arg->impl->spellCardBonus.pos.x =
            (384.0f - strlen(fmtArg) * 32.0f) / 2.0f + 32.0f;
        g_AsciiManager.scale.x = 2.0f;
        g_AsciiManager.scale.y = 2.0f;
        g_AsciiManager.color = 0xffff8080;
        g_AsciiManager.AddString(&arg->impl->spellCardBonus.pos, fmtArg);
        g_AsciiManager.scale.x = 1.0f;
        g_AsciiManager.scale.y = 1.0f;
        g_AsciiManager.color = 0xffffffff;
    }
    g_AsciiManager.isGui = 0;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0042868d
void Gui::ShowBombNamePortrait(i32 sprite, const char *name)
{
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->bombSpellcardPortrait, 1185);
    g_AnmManager->SetActiveSprite(&this->impl->bombSpellcardPortrait, sprite);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->bombSpellcardDecorLeft, 1188);
    g_AnmManager->SetActiveSprite(&this->impl->bombSpellcardDecorLeft, 1196);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->bombSpellcardDecorRight, 1190);
    g_AnmManager->SetActiveSprite(&this->impl->bombSpellcardDecorRight, 1196);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->bombSpellcardName, 1796);
    AnmManager::DrawVmTextFmt(g_AnmManager, &this->impl->bombSpellcardName,
                              0xf0f0ff, 0, name);
    this->bombNameBarLength = (f32)(u32)(strlen(name) * 15) / 2.0f + 16.0f;
    this->impl->bombSpellcardNameBg.SetInterrupt(1);
    g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB, 0);
    g_Supervisor.renderSkipFrames = 2;
}

// FUNCTION: TH07 0x00428887
void Gui::ShowSpellcard(i32 spellcardSprite, const char *spellcardName)
{
    if (spellcardSprite >= 0)
    {
        g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->enemySpellcardPortrait,
                                                1187);
        g_AnmManager->SetActiveSprite(&this->impl->enemySpellcardPortrait,
                                      spellcardSprite + 1197);
        if (this->impl->enemySpellcardPortrait.sprite->widthPx > 256.0f)
        {
            this->impl->enemySpellcardPortrait.offset.x = -288.0f;
        }
        else
        {
            if (this->impl->enemySpellcardPortrait.sprite->widthPx > 128.0f)
            {
                this->impl->enemySpellcardPortrait.offset.x = -112.0f;
            }
            else
            {
                this->impl->enemySpellcardPortrait.offset.x = 0.0f;
            }
        }
    }
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->enemySpellcardRelated1, 1189);
    g_AnmManager->SetActiveSprite(&this->impl->enemySpellcardRelated1, 1196);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->enemySpellcardRelated2, 1191);
    g_AnmManager->SetActiveSprite(&this->impl->enemySpellcardRelated2, 1196);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->enemySpellcardName, 1797);
    g_AnmManager->DrawStringFormat(&this->impl->enemySpellcardName, 0xfff0f0, 0,
                                   spellcardName);
    this->spellcardBarLength =
        (f32)(u32)(strlen(spellcardName) * 15) / 2.0f + 16.0f;
    this->impl->enemySpellcardNameBg.SetInterrupt(1);
    this->impl->spellcardBonusIndicator.SetInterrupt(1);
    g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB, 0);
    g_Supervisor.renderSkipFrames = 2;
}

#pragma var_order(i, j, k)
// FUNCTION: TH07 0x00428b19
ZunResult Gui::ActualAddedCallback()
{
    i32 k;
    i32 j;
    i32 i;

    this->frameCounter = 0;
    if (g_Supervisor.curState == 3 || g_Supervisor.curState == 11 || g_Supervisor.curState == 12 ? 0 : 1)
    {
        memset(this->impl, 0, sizeof(GuiImpl));

        if (g_AnmManager->LoadAnms(ANM_FILE_FRONT, "data/front.anm", ANM_OFFSET_FRONT) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        ClearActiveSprites();
        switch (g_GameManager.character)
        {
        case CHAR_REIMU:
            if (g_AnmManager->LoadAnms(ANM_FILE_FACE, "data/face_rm00.anm", ANM_OFFSET_FACE) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
            if (g_AnmManager->LoadAnms(ANM_FILE_LOADING, "data/loading.anm", ANM_OFFSET_LOADING) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
            break;
        case CHAR_MARISA:
            if (g_AnmManager->LoadAnms(ANM_FILE_FACE, "data/face_mr00.anm", ANM_OFFSET_FACE) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
            if (g_AnmManager->LoadAnms(ANM_FILE_LOADING, "data/loading2.anm", ANM_OFFSET_LOADING) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
            break;
        case CHAR_SAKUYA:
            if (g_AnmManager->LoadAnms(ANM_FILE_FACE, "data/face_sk00.anm", ANM_OFFSET_FACE) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
            if (g_AnmManager->LoadAnms(ANM_FILE_LOADING, "data/loading3.anm", ANM_OFFSET_LOADING) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
            break;
        }
    }
    else
    {
        ClearActiveSprites();
        g_AnmManager->SetAnmIdxAndExecuteScript(
            &this->impl->stageTransitionSnapshotVm,
            1829);
        this->impl->stageTransitionSnapshotVm.pendingInterrupt = 1;
        g_AnmManager->CreateScreenshotTexture(
            this->impl->stageTransitionSnapshotVm.sprite->startPixelInclusive
                .x,
            this->impl->stageTransitionSnapshotVm.sprite->startPixelInclusive
                .y,
            this->impl->stageTransitionSnapshotVm.sprite->widthPx,
            this->impl->stageTransitionSnapshotVm.sprite->heightPx);
        for (i = 0; i < 14; i++)
        {
            for (j = 0; j < 12; j++)
            {
                g_AnmManager->SetAnmIdxAndExecuteScript(
                    &this->impl->transitionQuads[i * 12 + j],
                    (i + j & 1) + 1830);
                this->impl->transitionQuads[i * 12 + j].intVars2[0] =
                    i + j * 2;
                this->impl->transitionQuads[i * 12 + j].pos.x =
                    (f32)j * 32.0f - 0.5f + 16.0f;
                this->impl->transitionQuads[i * 12 + j].pos.y =
                    (f32)i * 32.0f - 0.5f + 16.0f;
                this->impl->transitionQuads[i * 12 + j].pos.z = 0.0f;
                this->impl->transitionQuads[i * 12 + j].uvScrollPos.x =
                    (f32)j * 32.0f / 512.0f;
                this->impl->transitionQuads[i * 12 + j].uvScrollPos.y =
                    (f32)i * 32.0f / 512.0f;
            }
        }
        this->impl->activeTransitionQuads = 168;
    }
    switch (g_GameManager.currentStage)
    {
    case 1:
        CopyTemplateSpriteToSprite(1550);
        if (g_AnmManager->LoadAnms(ANM_FILE_FACE_STAGE, "data/face_01_00.anm", ANM_OFFSET_FACE_STAGE) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_TEXT, "data/std1txt.anm", ANM_OFFSET_STAGE_TEXT) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (LoadMsg("data/msg1.dat") != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 2:
        CopyTemplateSpriteToSprite(1552);
        if (g_AnmManager->LoadAnms(ANM_FILE_FACE_STAGE, "data/face_02_00.anm", ANM_OFFSET_FACE_STAGE) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_TEXT, "data/std2txt.anm", ANM_OFFSET_STAGE_TEXT) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (LoadMsg("data/msg2.dat") != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 3:
        CopyTemplateSpriteToSprite(1554);
        if (g_AnmManager->LoadAnms(ANM_FILE_FACE_STAGE, "data/face_03_00.anm", ANM_OFFSET_FACE_STAGE) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_TEXT, "data/std3txt.anm", ANM_OFFSET_STAGE_TEXT) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (LoadMsg("data/msg3.dat") != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 4:
        CopyTemplateSpriteToSprite(1556);
        if (g_AnmManager->LoadAnms(ANM_FILE_FACE_STAGE, "data/face_04_00.anm", ANM_OFFSET_FACE_STAGE) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_TEXT, "data/std4txt.anm", ANM_OFFSET_STAGE_TEXT) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (LoadMsg("data/msg4.dat") != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 5:
        CopyTemplateSpriteToSprite(1558);
        if (g_AnmManager->LoadAnms(ANM_FILE_FACE_STAGE, "data/face_05_00.anm", ANM_OFFSET_FACE_STAGE) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_TEXT, "data/std5txt.anm", ANM_OFFSET_STAGE_TEXT) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (LoadMsg("data/msg5.dat") != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 6:
        CopyTemplateSpriteToSprite(1560);
        if (g_AnmManager->LoadAnms(ANM_FILE_FACE_STAGE, "data/face_06_00.anm", ANM_OFFSET_FACE_STAGE) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_TEXT, "data/std6txt.anm", ANM_OFFSET_STAGE_TEXT) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (LoadMsg("data/msg6.dat") != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 7:
        CopyTemplateSpriteToSprite(1562);
        if (g_AnmManager->LoadAnms(ANM_FILE_FACE_STAGE, "data/face_07_00.anm", ANM_OFFSET_FACE_STAGE) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_TEXT, "data/std7txt.anm", ANM_OFFSET_STAGE_TEXT) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (LoadMsg("data/msg7.dat") != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 8:
        CopyTemplateSpriteToSprite(1564);
        if (g_AnmManager->LoadAnms(ANM_FILE_FACE_STAGE, "data/face_08_00.anm", ANM_OFFSET_FACE_STAGE) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_TEXT, "data/std8txt.anm", ANM_OFFSET_STAGE_TEXT) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (LoadMsg("data/msg8.dat") != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    default:
        return ZUN_ERROR;
    }
    if (g_Supervisor.curState == 3 || g_Supervisor.curState == 11 || g_Supervisor.curState == 12 ? 0 : 1)
    {
        for (k = 0; k < 33; k++)
        {
            g_AnmManager->SetAnmIdxAndExecuteScript(
                &this->impl->vms0[k], k + 1536);
        }
    }
    this->bossPresent = 0;
    this->impl->bossHealthBarState = 0;
    this->bossHealthBar = 0.0f;
    this->bossHealthBarEased = 0.0f;
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->bombSpellcardPortrait,
                                            1185);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->enemySpellcardPortrait,
                                            1187);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->bombSpellcardName,
                                            1796);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->enemySpellcardName,
                                            1797);
    g_AnmManager->ExecuteVmsAnms(this->impl->vms1, 2048, 5);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->bombSpellcardNameBg,
                                            1);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->enemySpellcardNameBg,
                                            0);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->spellcardBonusIndicator,
                                            2);
    g_AnmManager->SetAnmIdxAndExecuteScript(&this->impl->captureBonusVm,
                                            3);
    this->impl->bombSpellcardPortrait.currentInstruction = NULL;
    this->impl->bombSpellcardDecorLeft.currentInstruction = NULL;
    this->impl->bombSpellcardDecorRight.currentInstruction = NULL;
    this->impl->bombSpellcardName.currentInstruction = NULL;
    this->impl->enemySpellcardPortrait.currentInstruction = NULL;
    this->impl->enemySpellcardRelated1.currentInstruction = NULL;
    this->impl->enemySpellcardRelated2.currentInstruction = NULL;
    this->impl->enemySpellcardName.currentInstruction = NULL;
    this->impl->bombSpellcardPortrait.visible = 0;
    this->impl->bombSpellcardDecorLeft.visible = 0;
    this->impl->bombSpellcardDecorRight.visible = 0;
    this->impl->bombSpellcardName.visible = 0;
    this->impl->enemySpellcardPortrait.visible = 0;
    this->impl->enemySpellcardRelated1.visible = 0;
    this->impl->enemySpellcardRelated2.visible = 0;
    this->impl->enemySpellcardName.visible = 0;
    this->impl->bombSpellcardName.fontWidth = 15;
    this->impl->bombSpellcardName.fontHeight = 15;
    this->impl->enemySpellcardName.fontWidth = 15;
    this->impl->enemySpellcardName.fontHeight = 15;
    this->impl->msg.currentMsgIdx = -1;
    this->impl->finishedStage = 0;
    this->impl->bonusScore.isShown = 0;
    this->impl->fullPowerMode.isShown = 0;
    this->impl->spellCardBonus.isShown = 0;
    this->showLives = 2;
    this->showBombs = 2;
    this->showGraze = 2;
    this->showPoint = 2;
    this->showPower = 2;
    g_Supervisor.renderSkipFrames = 16;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00429935
ZunResult Gui::LoadMsg(const char *param_1)
{
    i32 i;

    FreeMsgFile();
    this->impl->msg.msgFile = (MsgRawHeader *)FileSystem::OpenFile(param_1, 0);
    if (!this->impl->msg.msgFile)
    {
        // STRING: TH07 0x00498108
        g_GameErrorContext.Log("error : メッセージファイル %s が読み込めませんでした\r\n", param_1);
        return ZUN_ERROR;
    }

    this->impl->msg.currentMsgIdx = -1;
    this->impl->msg.curInstr = NULL;
    for (i = 0; i < this->impl->msg.msgFile->numEntries; i++)
    {
        (&this->impl->msg.msgFile->entries)[i] =
            (MsgRawInstr *)((i32)(&this->impl->msg.msgFile->entries)[i] + (i32) & this->impl->msg.msgFile->numEntries);
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x004299f8
void Gui::FreeMsgFile()
{
    SAFE_FREE(this->impl->msg.msgFile);
}

// FUNCTION: TH07 0x00429a36
void Gui::MsgRead(i32 param_1)
{
    this->impl->MsgRead(param_1);
}

// FUNCTION: TH07 0x00429a4f
void GuiImpl::MsgRead(i32 msgIdx)
{
    MsgRawHeader *tmpMsgFile;

    if (this->msg.msgFile->numEntries <= msgIdx)
    {
        return;
    }

    tmpMsgFile = this->msg.msgFile;
    memset(&this->msg, 0, sizeof(GuiMsgVm));
    this->msg.currentMsgIdx = msgIdx;
    this->msg.msgFile = tmpMsgFile;
    this->msg.curInstr = (&this->msg.msgFile->entries)[msgIdx];
    this->msg.dialogueLines[0].anmFileIdx = -1;
    this->msg.dialogueLines[1].anmFileIdx = -1;
    this->msg.fontSize = 15;
    this->msg.textColorsA[0] = 0xe8f0ff;
    this->msg.textColorsA[1] = 0xffe8f0;
    this->msg.textColorsB[0] = 0;
    this->msg.textColorsB[1] = 0;
    this->msg.dialogueSkippable = 1;
    g_BulletManager.RemoveAllBullets(1);
    g_EnemyManager.RemoveAllEnemies(0, 0);
    g_ItemManager.RemoveAllItems();
    if (msgIdx % 10 == 0)
    {
        switch (g_GameManager.currentStage)
        {
        case 1:
            Gui::CopyTemplateSpriteToSprite(1551);
            break;
        case 2:
            Gui::CopyTemplateSpriteToSprite(1553);
            break;
        case 3:
            Gui::CopyTemplateSpriteToSprite(1555);
            break;
        case 4:
            Gui::CopyTemplateSpriteToSprite(1557);
            break;
        case 5:
            Gui::CopyTemplateSpriteToSprite(1559);
            break;
        case 6:
            Gui::CopyTemplateSpriteToSprite(1561);
            g_Stage.spellcardVmsIdx = 2;
            g_BulletManager.itemType = ITEM_STAR;
            break;
        case 7:
            Gui::CopyTemplateSpriteToSprite(1563);
            g_Stage.spellcardVmsIdx = 1;
            g_Stage.numSpellcardVms = 2;
            g_BulletManager.itemType = ITEM_STAR;
            break;
        case 8:
            Gui::CopyTemplateSpriteToSprite(1565);
            g_Stage.spellcardVmsIdx = 2;
            g_BulletManager.itemType = ITEM_STAR;
        }
    }
}

// FUNCTION: TH07 0x00429c42
ZunResult GuiImpl::RunMsg()
{
    MsgRawInstrArgs *args;

    if (this->msg.currentMsgIdx < 0)
    {
        return ZUN_ERROR;
    }
    if (this->msg.ignoreWaitCounter > 0)
    {
        this->msg.ignoreWaitCounter--;
    }
    if (this->msg.dialogueSkippable && IS_PRESSED_GAME(TH_BUTTON_SKIP))
    {
        this->msg.timer = (u32)this->msg.curInstr->time;
    }
    if (g_Player.hasBorder != BORDER_NONE)
    {
        g_Player.BreakBorderNaturally();
    }
    if (g_Player.playerState != PLAYER_STATE_DEAD)
    {
        g_ItemManager.RemoveAllItems();
    }
    while (this->msg.timer >= this->msg.curInstr->time)
    {
        switch (this->msg.curInstr->opcode)
        {
        case MSG_DELETE:
            this->msg.currentMsgIdx = -1;
            return ZUN_ERROR;
        case MSG_SHOW_PORTRAIT:
            args = &this->msg.curInstr->args;
            g_AnmManager->SetAnmIdxAndExecuteScript(
                &this->msg.portraits[args->portrait.portraitIdx],
                args->portrait.anmScriptIdx +
                    (args->portrait.portraitIdx !=
                             0
                         ? 1184 + 2
                         : 1184));
            if (this->msg.portraits[args->portrait.portraitIdx]
                    .sprite
                    ->widthPx > 128.0f)
            {
                this->msg.portraits[args->portrait.portraitIdx]
                    .offset.x = -112.0f;
            }
            else
            {
                this->msg.portraits[args->portrait.portraitIdx]
                    .offset.x = 0.0f;
            }
            break;
        case MSG_CHANGE_FACE:
            args = &this->msg.curInstr->args;
            g_AnmManager->SetActiveSprite(
                &this->msg.portraits[args->portrait.portraitIdx],
                args->portrait.anmScriptIdx + (args->portrait.portraitIdx == 0 ? 1184 : 1197));
            if (this->msg.portraits[args->portrait.portraitIdx]
                    .sprite
                    ->widthPx > 256.0f)
            {
                this->msg.portraits[args->portrait.portraitIdx]
                    .offset.x = -208.0f;
                this->msg.portraits[args->portrait.portraitIdx]
                    .offset.y = -50.0f;
            }
            else if (this->msg.portraits[args->portrait.portraitIdx]
                         .sprite
                         ->widthPx > 128.0f)
            {
                this->msg.portraits[args->portrait.portraitIdx]
                    .offset.x = -80.0f;
            }
            else
            {
                this->msg.portraits[args->portrait.portraitIdx]
                    .offset.x = 0.0f;
            }
            break;
        case MSG_DIALOGUE:
            args = &this->msg.curInstr->args;
            if (args->dialogue.textLine == 0 &&
                this->msg.dialogueLines[1].anmFileIdx >= 0)
            {
                AnmManager::DrawVmTextFmt(
                    g_AnmManager, this->msg.dialogueLines + 1,
                    this->msg.textColorsA[args->dialogue.textColor],
                    this->msg.textColorsB[args->dialogue.textColor],
                    " ");
            }
            g_AnmManager->SetAnmIdxAndExecuteScript(
                &this->msg.dialogueLines[args->dialogue.textLine],
                args->dialogue.textLine + 1792);
            this->msg.dialogueLines[args->dialogue.textLine]
                .fontHeight = (u8)this->msg.fontSize;
            this->msg.dialogueLines[args->dialogue.textLine]
                .fontWidth =
                this->msg.dialogueLines[args->dialogue.textLine]
                    .fontHeight;
            AnmManager::DrawVmTextFmt(
                g_AnmManager,
                &this->msg.dialogueLines[args->dialogue.textLine],
                this->msg.textColorsA[args->dialogue.textColor],
                this->msg.textColorsB[args->dialogue.textColor],
                args->dialogue.text);
            this->msg.framesElapsedDuringPause = 0;
            break;
        case MSG_PAUSE:
            if (this->msg.dialogueSkippable == 0 || !IS_PRESSED_GAME(TH_BUTTON_SKIP))
            {
                if (!WAS_PRESSED_GAME(TH_BUTTON_SHOOT) ||
                    this->msg.framesElapsedDuringPause < 12)
                {
                    if (this->msg.framesElapsedDuringPause >= this->msg.curInstr->args.pause.duration)
                    {
                        break;
                    }
                    this->msg.framesElapsedDuringPause =
                        this->msg.framesElapsedDuringPause + 1;
                    goto SKIP_TIME_INCREMENT;
                }
            }
            break;
        case MSG_SWITCH:
            args = &this->msg.curInstr->args;
            if (args->msgSwitch.unkIdx < 2)
            {
                this->msg.portraits[args->msgSwitch.unkIdx].pendingInterrupt =
                    args->msgSwitch.interrupt;
            }
            else
            {
                this->msg.dialogueLines[args->msgSwitch.unkIdx - 2].pendingInterrupt =
                    args->msgSwitch.interrupt;
            }
            break;
        case MSG_APPEAR_ENEMY:
            this->msg.ignoreWaitCounter++;
            break;
        case MSG_MUSIC:
            if (g_GameManager.currentStage != 6)
            {
                g_AnmManager->SetAnmIdxAndExecuteScript(&this->vms1[0],
                                                        2052);
            }
            else
            {
                g_AnmManager->SetAnmIdxAndExecuteScript(&this->vms1[0],
                                                        2053);
            }
            g_AnmManager->SetActiveSprite(
                this->vms1, this->msg.curInstr->args.music.musicIdx + 2051);
            if (g_Supervisor.PlayLoadedAudio(
                    this->msg.curInstr->args.music.musicIdx) != ZUN_SUCCESS)
            {
                g_Supervisor.PlayAudio(
                    g_Stage.stdData
                        ->bgmPaths[this->msg.curInstr->args.music.musicIdx]);
            }
            break;
        case MSG_TEXT_INTRODUCE:
            args = &this->msg.curInstr->args;
            g_AnmManager->SetAnmIdxAndExecuteScript(
                &this->msg.introLines[args->dialogue.textLine],
                args->dialogue.textLine + 1794);
            g_AnmManager->DrawStringFormat(
                this->msg.introLines + args->dialogue.textLine,
                this->msg.textColorsA[args->dialogue.textColor],
                this->msg.textColorsB[args->dialogue.textColor],
                args->dialogue.text);
            this->msg.framesElapsedDuringPause = 0;
            break;
        case MSG_STAGERESULTS:
            this->clearPower = g_GameManager.globals->currentPower;
            this->clearPointItems =
                g_GameManager.globals->pointItemsCollectedThisStage;
            this->clearCherryMax =
                g_GameManager.cherryMax - g_GameManager.globals->cherryStart;
            this->clearGraze = g_GameManager.globals->grazeInStage;
            this->finishedStage = 1;
            if (g_GameManager.currentStage < 6)
            {
                g_AnmManager->SetAnmIdxAndExecuteScript(&this->stageClearTextVm, 1566);
                g_AnmManager->SetAnmIdxAndExecuteScript(&this->stageTransitionSnapshotVm, 1829);
                g_AnmManager->CreateScreenshotTexture(
                    this->stageTransitionSnapshotVm.sprite->startPixelInclusive.x,
                    this->stageTransitionSnapshotVm.sprite->startPixelInclusive.y,
                    this->stageTransitionSnapshotVm.sprite->widthPx,
                    this->stageTransitionSnapshotVm.sprite->heightPx);
            }
            else
            {
                g_GameManager.globals->extendsFromPointItems = -1;
            }
            break;
        case MSG_FREEZE:
            goto SKIP_TIME_INCREMENT;
        case MSG_FADEOUT_MUSIC:
            g_Supervisor.FadeOutMusic(4.0f);
            break;
        case MSG_FADE_IN_EFFECT:
            BombEffects::RegisterChain(4, 0x192, 0xffffff, 0, 0);
            g_Supervisor.renderSkipFrames = 0x192;
            break;
        case MSG_NEXT_LEVEL:
            g_Supervisor.checkTiming = 0;
            g_GameManager.globals->guiScore = g_GameManager.globals->score;
            if (g_GameManager.practice)
            {
                g_GameManager.globals->guiScore = g_GameManager.globals->score;
                g_Supervisor.curState = 6;
                goto SKIP_TIME_INCREMENT;
            }

            if (g_GameManager.currentStage < 6)
            {
                if (g_GameManager.replay &&
                    !g_ReplayManager->StageReplayExists(g_GameManager.currentStage))
                {
                    g_Supervisor.curState = 7;
                    goto SKIP_TIME_INCREMENT;
                }

                UselessStack::FourBytes();
                g_AnmManager->InitializeAndSetActiveSprite(&this->stageClearBonusTextVm, 268);
                this->transitionToScoreScreen = 1;
                this->msg.currentMsgIdx = -2;
            }
            else if (!g_GameManager.replay)
            {
                if (g_GameManager.difficulty >= DIFF_EXTRA)
                {
                    if (g_GameManager.difficulty == DIFF_EXTRA)
                    {
                        g_GameManager.clrd[g_GameManager.shotTypeAndCharacter]
                            .difficultyClearedWithRetries[g_GameManager.difficulty] = 99;
                    }
                    ((Plst *)(g_GameManager.pscr + 6))
                        ->playDataByDifficulty[g_GameManager.difficulty]
                        .noContinueClearCount =
                        ((Plst *)(g_GameManager.pscr + 6))
                            ->playDataByDifficulty[g_GameManager.difficulty]
                            .noContinueClearCount +
                        1;
                    g_GameManager.finished = 1;
                    g_GameManager.globals->guiScore = g_GameManager.globals->score;
                    g_Supervisor.curState = 6;
                    goto SKIP_TIME_INCREMENT;
                }
                else
                {
                    g_GameManager.finished = 1;
                    g_GameManager.globals->guiScore = g_GameManager.globals->score;
                    g_Supervisor.curState = 9;
                    goto SKIP_TIME_INCREMENT;
                }
            }
            else
            {
                if (g_GameManager.currentStage == 8 &&
                    g_GameManager.globals->score !=
                        g_ReplayManager->data->data.score)
                {
                    ReplayManager::SaveReplay2(g_GameManager.replayFilename);
                }
                g_Supervisor.curState = 7;
            }
            goto SKIP_TIME_INCREMENT;
        case MSG_ALLOW_SKIP:
            this->msg.dialogueSkippable = *(u8 *)&this->msg.curInstr->args;
            break;
        }
        this->msg.curInstr = (MsgRawInstr *)((u8 *)&this->msg.curInstr->args +
                                             this->msg.curInstr->argsize);
    }
    this->msg.timer.NextTick();
SKIP_TIME_INCREMENT:
    g_AnmManager->ExecuteScript(&this->msg.portraits[0]);
    g_AnmManager->ExecuteScript(&this->msg.portraits[1]);
    g_AnmManager->ExecuteScript(&this->msg.dialogueLines[0]);
    g_AnmManager->ExecuteScript(&this->msg.dialogueLines[1]);
    g_AnmManager->ExecuteScript(&this->msg.introLines[0]);
    g_AnmManager->ExecuteScript(&this->msg.introLines[1]);
    if (this->msg.timer < 60 &&
        this->msg.dialogueSkippable &&
        IS_PRESSED_GAME(TH_BUTTON_SKIP))
    {
        this->msg.timer = 60;
    }

    return ZUN_SUCCESS;
}

#pragma var_order(height, dialogueBg, oldPos)
// FUNCTION: TH07 0x0042a876
ZunResult GuiImpl::DrawDialogue()
{
    Float3 oldPos;
    f32 height;

    if (this->msg.currentMsgIdx < 0)
    {
        return ZUN_ERROR;
    }
    if (g_GameManager.currentStage == 6 &&
        (this->msg.currentMsgIdx == 1 ||
         this->msg.currentMsgIdx == 11))
    {
        return ZUN_SUCCESS;
    }

    if (this->msg.timer < 60)
    {
        height =
            this->msg.timer.AsFloat() * 48.0f /
            60.0f;
    }
    else
    {
        height = 48.0f;
    }

    VertexDiffuseXyzrhw dialogueBg[4];
    dialogueBg[0].pos = Float3(g_GameManager.arcadeRegionTopLeftPos.x + 16.0f, 384.0f, 0.0f);
    dialogueBg[1].pos = Float3(g_GameManager.arcadeRegionTopLeftPos.x + 384.0f - 16.0f, 384.0f, 0.0f);
    dialogueBg[2].pos = Float3(g_GameManager.arcadeRegionTopLeftPos.x + 16.0f, 384.0f + height, 0.0f);
    dialogueBg[3].pos = Float3(g_GameManager.arcadeRegionTopLeftPos.x + 384.0f - 16.0f, 384.0f + height, 0.0f);
    dialogueBg[0].diffuse.color = dialogueBg[1].diffuse.color = 0xd0000000;
    dialogueBg[2].diffuse.color = dialogueBg[3].diffuse.color = 0x90000000;
    dialogueBg[0].w = dialogueBg[1].w = dialogueBg[2].w = dialogueBg[3].w = 1.0f;
    g_AnmManager->DrawNoRotation(&this->msg.portraits[0]);
    oldPos = this->msg.portraits[1].pos;
    this->msg.portraits[1].pos += this->msg.portraits[1].offset;
    g_AnmManager->DrawNoRotation(&this->msg.portraits[1]);
    this->msg.portraits[1].pos = oldPos;
    g_AnmManager->Flush();
    if (!g_Supervisor.cfg.disableTextureBlend)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 2);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 2);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 0);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 0);
    if (!g_Supervisor.cfg.disableZBuffer)
    {
        g_Supervisor.SetRenderState(D3DRS_ZWRITEENABLE, 0);
    }
    g_Supervisor.d3dDevice->SetVertexShader(D3DFVF_DIFFUSE | D3DFVF_XYZRHW);
    g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, dialogueBg,
                                            sizeof(VertexDiffuseXyzrhw));
    g_AnmManager->SetVertexShader(255);
    g_AnmManager->SetColorOp(255);
    g_AnmManager->SetBlendMode(255);
    g_AnmManager->SetZWriteDisable(255);
    if (!g_Supervisor.cfg.disableTextureBlend)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 4);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 4);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 2);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 2);
    g_AnmManager->DrawNoRotation(&this->msg.dialogueLines[0]);
    g_AnmManager->DrawNoRotation(&this->msg.dialogueLines[1]);
    g_AnmManager->DrawNoRotation(&this->msg.introLines[0]);
    g_AnmManager->DrawNoRotation(&this->msg.introLines[1]);
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0042ad29
i32 Gui::MsgWait()
{
    if (!this->impl)
    {
        return 0;
    }

    if (this->impl->msg.ignoreWaitCounter > 0)
    {
        return 0;
    }

    return this->impl->msg.currentMsgIdx >= 0;
}

// FUNCTION: TH07 0x0042ad66
i32 Gui::HasCurrentMsgIdx()
{
    if (!this->impl)
    {
        return 0;
    }

    return this->impl->msg.currentMsgIdx >= 0 ||
           this->impl->msg.currentMsgIdx == -2;
}

#pragma var_order(activeTransitionQuads, i, scoreBonus)
// FUNCTION: TH07 0x0042adab
void Gui::UpdateGui()
{
    i32 scoreBonus;
    i32 i;
    i32 activeTransitionQuads;

    if (this->impl->msg.currentMsgIdx < 0)
    {
        if (this->bossPresent)
        {
            if (this->impl->bossHealthBarState == 0)
            {
                this->impl->vms0[11].SetInterrupt(1);
                this->impl->bossHealthBarState = 1;
                this->bossHealthBarAlpha = 0;
            }
            else
            {
                if (this->impl->vms0[11].isStopped)
                {
                    this->impl->bossHealthBarState = 2;
                }
                if (this->bossHealthBarAlpha < 252)
                {
                    this->bossHealthBarAlpha += 4;
                }
                else
                {
                    this->bossHealthBarAlpha = 255;
                }
            }
        }
        else if (this->impl->bossHealthBarState != 0)
        {
            if (this->impl->bossHealthBarState <= 2)
            {
                this->impl->vms0[11].SetInterrupt(2);
                this->impl->bossHealthBarState = 3;
            }
            if (this->bossHealthBarAlpha > 0)
            {
                this->bossHealthBarAlpha -= 4;
            }
            else
            {
                this->bossHealthBarAlpha = 0;
            }
            if (this->impl->vms0[11].isStopped)
            {
                this->impl->bossHealthBarState = 0;
                this->bossHealthBarEased = 0.0f;
                this->bossHealthBarAlpha = 0;
            }
        }

        if (this->impl->bossHealthBarState >= 2)
        {
            if (this->bossHealthBar > this->bossHealthBarEased)
            {
                this->bossHealthBarEased += 0.01f;
                if (this->bossHealthBar < this->bossHealthBarEased)
                {
                    this->bossHealthBarEased = this->bossHealthBar;
                }
            }
            else if (this->bossHealthBar < this->bossHealthBarEased)
            {
                this->bossHealthBarEased -= 0.02f;
                if (this->bossHealthBar > this->bossHealthBarEased)
                {
                    this->bossHealthBarEased = this->bossHealthBar;
                }
            }
        }
    }
    g_AnmManager->ExecuteScripts(this->impl->vms0, 33);
    g_AnmManager->ExecuteScripts(this->impl->vms1, 5);
    g_AnmManager->ExecuteScript(&this->impl->bombSpellcardPortrait);
    g_AnmManager->ExecuteScript(&this->impl->bombSpellcardDecorLeft);
    g_AnmManager->ExecuteScript(&this->impl->bombSpellcardDecorRight);
    g_AnmManager->ExecuteScript(&this->impl->bombSpellcardName);
    g_AnmManager->ExecuteScript(&this->impl->enemySpellcardPortrait);
    g_AnmManager->ExecuteScript(&this->impl->enemySpellcardRelated1);
    g_AnmManager->ExecuteScript(&this->impl->enemySpellcardRelated2);
    g_AnmManager->ExecuteScript(&this->impl->enemySpellcardName);
    g_AnmManager->ExecuteScript(&this->impl->bombSpellcardNameBg);
    g_AnmManager->ExecuteScript(&this->impl->enemySpellcardNameBg);
    g_AnmManager->ExecuteScript(&this->impl->spellcardBonusIndicator);
    if (this->impl->stageClearTextVm.activeSpriteIdx >= 0)
    {
        if (g_AnmManager->ExecuteScript(&this->impl->stageClearTextVm))
        {
            this->impl->stageClearTextVm.activeSpriteIdx = -1;
        }
        if (g_AnmManager->ExecuteScript(&this->impl->stageTransitionSnapshotVm) !=
            0)
        {
            this->impl->stageTransitionSnapshotVm.activeSpriteIdx = -1;
        }
    }
    if (this->impl->activeTransitionQuads != 0)
    {
        activeTransitionQuads = 168;
        for (i = 0; i < 168; i++)
        {
            if (g_AnmManager->ExecuteScript(this->impl->transitionQuads + i))
            {
                activeTransitionQuads--;
            }
        }
        this->impl->activeTransitionQuads = activeTransitionQuads;
    }
    if (this->impl->bonusScore.isShown)
    {
        if (this->impl->bonusScore.timer < 30)
        {
            this->impl->bonusScore.pos.x =
                this->impl->bonusScore.timer.AsFloat() *
                    -312.0f /
                    30.0f +
                416.0f;
        }
        else
        {
            this->impl->bonusScore.pos.x = 104.0f;
        }
        if (this->impl->bonusScore.timer >= 250)
        {
            this->impl->bonusScore.isShown = 0;
        }
        ++this->impl->bonusScore.timer;
    }
    if (this->impl->fullPowerMode.isShown)
    {
        if (this->impl->fullPowerMode.timer < 30)
        {
            this->impl->fullPowerMode.pos.x =
                this->impl->fullPowerMode.timer.AsFloat() *
                    -312.0f /
                    30.0f +
                416.0f;
        }
        else
        {
            this->impl->fullPowerMode.pos.x = 104.0f;
        }
        if (this->impl->fullPowerMode.timer >= 180)
        {
            this->impl->fullPowerMode.isShown = 0;
        }
        ++this->impl->fullPowerMode.timer;
    }
    if (this->impl->spellCardBonus.isShown)
    {
        if (this->impl->spellCardBonus.timer >= 280)
        {
            this->impl->spellCardBonus.isShown = 0;
        }
        ++this->impl->spellCardBonus.timer;
    }
    if (this->impl->finishedStage == 1)
    {
        scoreBonus = 0;
        scoreBonus += g_GameManager.currentStage * 100000;
        scoreBonus += this->impl->clearGraze * 50;
        scoreBonus += this->impl->clearPointItems * 5000;
        scoreBonus += this->impl->clearCherryMax;
        if (g_GameManager.currentStage >= 7 ||
            (g_GameManager.currentStage == 6 &&
             !g_GameManager.practice &&
             (!g_GameManager.replay ||
              g_ReplayManager->data->head.stageReplayData[4].data)))
        {
            scoreBonus += (i32)g_GameManager.globals->livesRemaining * 2000000;
            scoreBonus += (i32)g_GameManager.globals->bombsRemaining * 400000;
        }
        switch (g_GameManager.difficulty)
        {
        case DIFF_EASY:
            scoreBonus /= 2;
            break;
        case DIFF_HARD:
            scoreBonus = scoreBonus * 12 / 10;
            break;
        case DIFF_LUNATIC:
            scoreBonus = scoreBonus * 15 / 10;
            break;
        case DIFF_EXTRA:
            scoreBonus <<= 1;
            break;
        case DIFF_PHANTASM:
            scoreBonus <<= 1;
            break;
        }

        switch (g_GameManager.defaultCfg->lifeCount)
        {
        case 3:
            scoreBonus = scoreBonus * 5 / 10;
            break;
        case 4:
            scoreBonus = (scoreBonus << 1) / 10;
            break;
        }
        this->impl->stageClearBonus = scoreBonus;

        // ZUN bloat:
        // To add the score, in full without divide
        // It would be simple if we multiplied
        // But ZUN said "we must copy paste ten times"
        // And so we got whatever the hell this is. Perchance
        g_GameManager.AddScore(scoreBonus);
        g_GameManager.AddScore(scoreBonus);
        g_GameManager.AddScore(scoreBonus);
        g_GameManager.AddScore(scoreBonus);
        g_GameManager.AddScore(scoreBonus);
        g_GameManager.AddScore(scoreBonus);
        g_GameManager.AddScore(scoreBonus);
        g_GameManager.AddScore(scoreBonus);
        g_GameManager.AddScore(scoreBonus);
        g_GameManager.AddScore(scoreBonus);
        this->impl->finishedStage++;
    }
}

#pragma var_order(y, x, i, vm, textDrawPos)
// FUNCTION: TH07 0x0042b603
void Gui::DrawGameScene()
{
    Float3 textDrawPos;
    AnmVm *vm;
    i32 i;
    f32 x;
    f32 y;

    g_AnmManager->Flush();
    g_Supervisor.viewport.X = 0;
    g_Supervisor.viewport.Y = 0;
    g_Supervisor.viewport.Width = 640;
    g_Supervisor.viewport.Height = 480;
    g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    vm = &this->impl->vms0[12];
    if (g_Supervisor.cfg.redrawEveryFrame ||
        vm->currentInstruction ||
        g_Supervisor.renderSkipFrames != 0)
    {
        for (y = 0.0f; y < 464.0f; y = y + 32.0f)
        {
            vm->pos = Float3(0.0f, y, 0.49f);
            g_AnmManager->DrawNoRotation(vm);
        }
        for (x = 416.0f; x < 624.0f; x = x + 32.0f)
        {
            for (y = 16.0f; y < 464.0f; y = y + 32.0f)
            {
                vm->pos = Float3(x, y, 0.49f);
                g_AnmManager->DrawNoRotation(vm);
            }
        }
        vm = &this->impl->vms0[13];
        for (x = 0.0f; x < 624.0f; x = x + 128.0f)
        {
            vm->pos = Float3(x, 0.0f, 0.49f);
            g_AnmManager->DrawNoRotation(vm);
            vm->pos = Float3(x, 464.0f, 0.49f);
            g_AnmManager->DrawNoRotation(vm);
        }
        g_AnmManager->DrawNoRotation(this->impl->vms0);
        g_AnmManager->Draw(this->impl->vms0 + 1);
        g_AnmManager->DrawNoRotation(this->impl->vms0 + 2);
        g_AnmManager->DrawNoRotation(this->impl->vms0 + 3);
        g_AnmManager->DrawNoRotation(this->impl->vms0 + 4);
        g_AnmManager->DrawNoRotation(this->impl->vms0 + 5);
        g_AnmManager->DrawNoRotation(this->impl->vms0 + 6);
        g_AnmManager->DrawNoRotation(this->impl->vms0 + 7);
        g_AnmManager->DrawNoRotation(this->impl->vms0 + 8);
        this->showLives = 2;
        this->showBombs = 2;
        this->showGraze = 2;
        this->showPoint = 2;
        this->showPower = 2;
    }
    if (!g_Supervisor.cfg.disableItemDrawAroundPlayfield)
    {
        vm = &this->impl->vms0[13];
        x = 496.0f;
        vm->pos = Float3(x, 48.0f, 0.49f);
        g_AnmManager->DrawNoRotation(vm);
        vm->pos = Float3(x, 64.0f, 0.49f);
        g_AnmManager->DrawNoRotation(vm);
        if (this->showLives)
        {
            vm->pos = Float3(x, 96.0f, 0.48f);
            g_AnmManager->DrawNoRotation(vm);
        }
        if (this->showBombs)
        {
            vm->pos = Float3(x, 112.0f, 0.48f);
            g_AnmManager->DrawNoRotation(vm);
        }
        if (this->showPower)
        {
            vm->pos = Float3(x, 144.0f, 0.48f);
            g_AnmManager->DrawNoRotation(vm);
        }
        if (this->showGraze)
        {
            vm->pos = Float3(x, 160.0f, 0.48f);
            g_AnmManager->DrawNoRotation(vm);
        }
        if (this->showPoint)
        {
            vm->pos = Float3(x, 176.0f, 0.48f);
            g_AnmManager->DrawNoRotation(vm);
        }
        vm->pos = Float3(512.0f, 464.0f, 0.48f);
        g_AnmManager->DrawNoRotation(vm);
    }
    if (this->showLives)
    {
        vm = &this->impl->vms0[9];
        for (i = 0, x = 496.0f;
             i < (i32)g_GameManager.globals->livesRemaining;
             i++, x += 16.0f)
        {
            vm->pos = Float3(x, 96.0f, 0.46f);
            g_AnmManager->DrawNoRotation(vm);
        }
    }
    if (this->showBombs)
    {
        vm = &this->impl->vms0[10];
        for (i = 0, x = 496.0f;
             i < (i32)g_GameManager.globals->bombsRemaining;
             i++, x += 16.0f)
        {
            vm->pos = Float3(x, 112.0f, 0.46f);
            g_AnmManager->DrawNoRotation(vm);
        }
    }
    vm = &this->impl->vms0[13];
    for (x = 32.0f; x < 368.0f; x = x + 128.0f)
    {
        vm->pos = Float3(x, 464.0f, 0.49f);
        g_AnmManager->DrawNoRotation(vm);
    }
    textDrawPos.x = 496.0f;
    textDrawPos.y = 64.0f;
    textDrawPos.z = 0.0f;
    if (g_GameManager.globals->guiScore < 100000000)
    {
        AsciiManager::AddFormatText(&g_AsciiManager, &textDrawPos, "%.8d",
                                    g_GameManager.globals->guiScore);
        textDrawPos.x = textDrawPos.x + 112.0f;
        AsciiManager::AddFormatText(&g_AsciiManager, &textDrawPos, "%1d",
                                    (u32)g_GameManager.globals->numRetries);
    }
    else
    {
        g_AsciiManager.scale.x = 0.9f;
        g_AsciiManager.scale.y = 1.0f;
        AsciiManager::AddFormatText(&g_AsciiManager, &textDrawPos, "%.9d",
                                    g_GameManager.globals->guiScore);
        textDrawPos.x = textDrawPos.x + 113.399994f;
        AsciiManager::AddFormatText(&g_AsciiManager, &textDrawPos, "%1d",
                                    (u32)g_GameManager.globals->numRetries);
        g_AsciiManager.scale.x = 1.0f;
        g_AsciiManager.scale.y = 1.0f;
    }
    textDrawPos = Float3(496.0f, 48.0f, 0.0f);
    if (g_GameManager.globals->highScore < 100000000)
    {
        AsciiManager::AddFormatText(&g_AsciiManager, &textDrawPos, "%.8d",
                                    g_GameManager.globals->highScore);
        textDrawPos.x = textDrawPos.x + 112.0f;
        AsciiManager::AddFormatText(
            &g_AsciiManager, &textDrawPos, "%1d",
            (u32)g_GameManager.globals->highScoreNumContinues);
    }
    else
    {
        g_AsciiManager.scale.x = 0.9f;
        g_AsciiManager.scale.y = 1.0f;
        AsciiManager::AddFormatText(&g_AsciiManager, &textDrawPos, "%.9d",
                                    g_GameManager.globals->highScore);
        textDrawPos.x = textDrawPos.x + 113.399994f;
        AsciiManager::AddFormatText(
            &g_AsciiManager, &textDrawPos, "%1d",
            (u32)g_GameManager.globals->highScoreNumContinues);
        g_AsciiManager.scale.x = 1.0f;
        g_AsciiManager.scale.y = 1.0f;
    }
    if (this->showGraze ||
        g_Supervisor.cfg.disableItemDrawAroundPlayfield)
    {
        textDrawPos = Float3(496.0f, 160.0f, 0.0f);
        AsciiManager::AddFormatText(&g_AsciiManager, &textDrawPos, "%d",
                                    g_GameManager.globals->grazeInTotal);
    }
    if (this->showPoint ||
        g_Supervisor.cfg.disableItemDrawAroundPlayfield)
    {
        textDrawPos = Float3(496.0f, 176.0f, 0.0f);
        AsciiManager::AddFormatText(
            &g_AsciiManager, &textDrawPos, "%d/%d",
            g_GameManager.globals->pointItemsCollectedForExtend,
            g_GameManager.globals->nextNeededPointItemsForExtend);
    }
    g_AnmManager->Flush();
    if (this->showPower ||
        g_Supervisor.cfg.disableItemDrawAroundPlayfield)
    {
        VertexDiffuseXyzrhw powerBarVerts[4];

        if (0 < (i32)g_GameManager.globals->currentPower)
        {
            powerBarVerts[0].pos = Float3(496.0f, 144.0f, 0.1f);
            powerBarVerts[1].pos =
                Float3((f32)((i32)g_GameManager.globals->currentPower + 0x1f0) + 0.0f, 144.0f, 0.1f);
            powerBarVerts[2].pos = Float3(496.0f, 160.0f, 0.1f);
            powerBarVerts[3].pos = Float3((f32)((i32)g_GameManager.globals->currentPower + 0x1f0) + 0.0f, 160.0f, 0.1f);
            powerBarVerts[0].diffuse.color = powerBarVerts[2].diffuse.color = 0xe0e0e0ff;
            powerBarVerts[1].diffuse.color = powerBarVerts[3].diffuse.color = 0x80e0e0ff;

            powerBarVerts[0].w = powerBarVerts[1].w = powerBarVerts[2].w = powerBarVerts[3].w = 1.0f;
            if (!g_Supervisor.cfg.disableTextureBlend)
            {
                g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 2);
                g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 2);
            }
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 0);
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 0);
            if (!g_Supervisor.cfg.disableZBuffer)
            {
                g_Supervisor.SetRenderState(D3DRS_ZWRITEENABLE, 0);
            }
            g_Supervisor.d3dDevice->SetVertexShader(D3DFVF_DIFFUSE | D3DFVF_XYZRHW);
            g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &powerBarVerts,
                                                    sizeof(VertexDiffuseXyzrhw));
            g_AnmManager->SetVertexShader(255);
            g_AnmManager->SetColorOp(255);
            g_AnmManager->SetBlendMode(255);
            g_AnmManager->SetZWriteDisable(255);
            if (!g_Supervisor.cfg.disableTextureBlend)
            {
                g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 4);
                g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 4);
            }
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 2);
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 2);
        }
        if ((i32)g_GameManager.globals->currentPower < 128)
        {
            AsciiManager::AddFormatText(&g_AsciiManager,
                                        &Float3(496.0f, 144.0f, 0.0f),
                                        "%d",
                                        (i32)g_GameManager.globals->currentPower);
        }
        else
        {
            AsciiManager::AddFormatText(&g_AsciiManager,
                                        &Float3(496.0f, 144.0f, 0.0f), "MAX");
        }
    }
    if (this->showLives)
    {
        this->showLives--;
    }
    if (this->showPower)
    {
        this->showPower--;
    }
    if (this->showBombs)
    {
        this->showBombs--;
    }
    if (this->showGraze)
    {
        this->showGraze--;
    }
}

#pragma var_order(i, oldPos, remainingBonus, catk, digit, digitDivisor, leadingZeroSkipped,        \
                  color2, color1, healthBarRect, secondsRemaining, j, segmentEndHealth, timeColor, \
                  markerGap, timerPos)
// FUNCTION: TH07 0x0042c577
void Gui::DrawStageElements()
{
    Float3 timerPos;
    i32 markerGap;
    D3DCOLOR timeColor;
    f32 segmentEndHealth;
    i32 j;
    i32 secondsRemaining;
    ZunRect healthBarRect;
    D3DCOLOR color1;
    D3DCOLOR color2;
    i32 leadingZeroSkipped;
    i32 digitDivisor;
    i32 digit;
    Catk *catk;
    i32 remainingBonus;
    Float3 oldPos;
    i32 i;

    for (i = 0; i < 5; i++)
    {
        g_AnmManager->Draw(&this->impl->vms1[i]);
    }
    if (this->impl->bombSpellcardPortrait.visible)
    {
        g_AnmManager->DrawNoRotation(&this->impl->bombSpellcardPortrait);
        g_AnmManager->DrawNoRotation(&this->impl->bombSpellcardDecorLeft);
        g_AnmManager->Draw(&this->impl->bombSpellcardDecorRight);
    }
    if (this->impl->enemySpellcardPortrait.visible)
    {
        oldPos = this->impl->enemySpellcardPortrait.pos;
        this->impl->enemySpellcardPortrait.pos +=
            this->impl->enemySpellcardPortrait.offset;
        g_AnmManager->DrawNoRotation(&this->impl->enemySpellcardPortrait);
        this->impl->enemySpellcardPortrait.pos = oldPos;
        g_AnmManager->DrawNoRotation(&this->impl->enemySpellcardRelated1);
        g_AnmManager->Draw(&this->impl->enemySpellcardRelated2);
    }
    if (this->impl->bombSpellcardName.visible)
    {
        this->impl->bombSpellcardNameBg.pos = this->impl->bombSpellcardName.pos;
        g_AnmManager->DrawNoRotation(&this->impl->bombSpellcardNameBg);
        g_AnmManager->Draw(&this->impl->bombSpellcardName);
    }
    if (this->impl->enemySpellcardName.visible)
    {
        this->impl->enemySpellcardNameBg.pos = this->impl->enemySpellcardName.pos;
        g_AnmManager->DrawNoRotation(&this->impl->enemySpellcardNameBg);
        g_AnmManager->Draw(&this->impl->enemySpellcardName);
        g_AnmManager->DrawNoRotation(&this->impl->spellcardBonusIndicator);
        remainingBonus = g_EnemyManager.spellcardInfo.captureScore +
                         g_EnemyManager.spellcardInfo.grazeBonusScore;
        digitDivisor = 10000000;
        leadingZeroSkipped = 0;
        catk = &g_GameManager.catk[g_EnemyManager.spellcardInfo.spellcardIdx];
        if (!g_EnemyManager.spellcardInfo.isCapturing)
        {
            remainingBonus = 0;
        }
        this->impl->captureBonusVm.pos = this->impl->spellcardBonusIndicator.pos;
        this->impl->captureBonusVm.pos.x -= 40.0f;
        for (i = 0; i < 8; i++)
        {
            digit = remainingBonus / digitDivisor;
            if (digit != 0)
            {
                leadingZeroSkipped = 1;
            }
            if (leadingZeroSkipped != 0 || digitDivisor == 1)
            {
                this->impl->captureBonusVm.sprite =
                    g_AnmManager->GetSprite(digit + 132);
                g_AnmManager->DrawNoRotation(&this->impl->captureBonusVm);
            }
            this->impl->captureBonusVm.pos.x += 7.0f;
            remainingBonus %= digitDivisor;
            digitDivisor /= 10;
        }
        digit = catk->numSuccessesPerShot[g_GameManager.shotTypeAndCharacter];
        if (99 < digit)
        {
            digit = 99;
        }
        this->impl->captureBonusVm.pos.x += 36.0f;
        if (digit / 10 != 0)
        {
            this->impl->captureBonusVm.sprite =
                g_AnmManager->GetSprite(digit / 10 + 132);
            g_AnmManager->DrawNoRotation(&this->impl->captureBonusVm);
        }
        this->impl->captureBonusVm.pos.x += 7.0f;
        this->impl->captureBonusVm.sprite =
            g_AnmManager->GetSprite(digit % 10 + 132);
        g_AnmManager->DrawNoRotation(&this->impl->captureBonusVm);

        digit = catk->numAttemptsPerShot[g_GameManager.shotTypeAndCharacter];
        if (99 < digit)
        {
            digit = 99;
        }
        this->impl->captureBonusVm.pos.x += 14.0f;
        if (digit / 10 != 0)
        {
            this->impl->captureBonusVm.sprite =
                g_AnmManager->GetSprite(digit / 10 + 132);
            g_AnmManager->DrawNoRotation(&this->impl->captureBonusVm);
        }
        this->impl->captureBonusVm.pos.x += 7.0f;
        this->impl->captureBonusVm.sprite =
            g_AnmManager->GetSprite(digit % 10 + 132);
        g_AnmManager->DrawNoRotation(&this->impl->captureBonusVm);
    }
    if (this->impl->stageClearTextVm.activeSpriteIdx >= 0)
    {
        g_AnmManager->DrawNoRotation(&this->impl->stageClearTextVm);
        g_AnmManager->DrawNoRotation(&this->impl->stageTransitionSnapshotVm);
        if (this->impl->stageClearBonusTextVm.activeSpriteIdx >= 0)
        {
            this->impl->stageClearBonusTextVm.pos = Float3(304.0f, 448.0f, 0.0f);
            g_AnmManager->DrawNoRotation(&this->impl->stageClearBonusTextVm);
        }
    }
    if (this->impl->activeTransitionQuads != 0)
    {
        for (i = 0; i < 168; i++)
        {
            g_AnmManager->DrawProjected(&this->impl->transitionQuads[i]);
            g_AnmManager->SetSprite(NULL);
        }
    }
    if (this->impl->msg.currentMsgIdx < 0 &&
        this->bossPresent + this->impl->bossHealthBarState > 0)
    {
        healthBarRect.left = 64.0f;
        healthBarRect.top = 19.0f;
        healthBarRect.right = this->bossHealthBarEased * 320.0f + 64.0f;
        healthBarRect.bottom = 23.0f;
        color1 = this->bossHealthBarAlpha << 24 | 0xffffff;
        color2 = this->bossHealthBarAlpha << 24 | 0x202060;
        timerPos.x = 48.0f;
        timerPos.y = 16.0f;
        timerPos.z = 0.0f;
        ScreenEffect::DrawColoredQuad(&healthBarRect, color1, color1, color2, color2);
        for (j = 0; j < 8; j++)
        {
            if (this->bossHealth[j] == 0.0f)
            {
                continue;
            }
            if (this->bossHealthEased[j] >= this->bossHealthBarEased)
            {
                continue;
            }
            segmentEndHealth = this->bossHealth[j];
            if (this->bossHealthBarEased < segmentEndHealth)
            {
                segmentEndHealth = this->bossHealthBarEased;
            }
            healthBarRect.left = this->bossHealthEased[j] * 320.0f + 64.0f;
            healthBarRect.top = 19.0f;
            healthBarRect.right = segmentEndHealth * 320.0f + 64.0f;
            healthBarRect.bottom = 23.0f;
            color1 = this->bossHealthBarAlpha << 24 | (this->bossColor[j] & 0xffffff);
            color2 = this->bossHealthBarAlpha << 24 | ((i32)this->bossColor[j] >> 2 & 0x3f3f3fU);
            ScreenEffect::DrawColoredQuad(&healthBarRect, color1, color1, color2, color2);
        }
        g_AnmManager->DrawNoRotation(&this->impl->vms0[11]);
        healthBarRect.left = 33.0f;
        healthBarRect.top = 19.0f;
        healthBarRect.right = healthBarRect.left + 3.0f;
        healthBarRect.bottom = healthBarRect.top + 4.0f;
        secondsRemaining = this->bossLifeMarkers;
        markerGap = (this->bossLifeMarkers <= 5) + 1;
        for (j = 0; j < secondsRemaining; j++)
        {
            healthBarRect.left = (f32)j * 26.0f / (f32)secondsRemaining + 35.0f;
            healthBarRect.right = (f32)(j + 1) * 26.0f / (f32)secondsRemaining + 35.0f -
                                  (f32)markerGap;
            color1 = this->bossHealthBarAlpha << 24 | 0xffffff - j * 255 / 9;
            color2 = this->bossHealthBarAlpha << 24 | 0x202020;
            ScreenEffect::DrawColoredQuad(&healthBarRect, color1, color1, color2, color2);
        }
        timerPos = Float3(384.0f, 16.0f, 0.0f);
        if (this->spellcardSecondsRemaining >= 20)
        {
            timeColor = g_SpellcardTimeColors[0];
        }
        else if (this->spellcardSecondsRemaining >= 10)
        {
            timeColor = g_SpellcardTimeColors[1];
        }
        else if (this->spellcardSecondsRemaining >= 5)
        {
            timeColor = g_SpellcardTimeColors[2];
        }
        else
        {
            timeColor = g_SpellcardTimeColors[3];
        }
        g_AsciiManager.SetColor(this->bossHealthBarAlpha << 24 | timeColor);
        secondsRemaining = this->spellcardSecondsRemaining > 99 ? 99 : this->spellcardSecondsRemaining;
        if (secondsRemaining < 10 && this->lastSpellcardSecondsRemaining !=
                                         this->spellcardSecondsRemaining)
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_29, 0);
        }
        AsciiManager::AddFormatText(&g_AsciiManager, &timerPos, "%.2d", secondsRemaining);
        g_AsciiManager.color = 0xffffffff;
        this->lastSpellcardSecondsRemaining = this->spellcardSecondsRemaining;
    }
}

// FUNCTION: TH07 0x0042d03a
ZunResult Gui::AddedCallback(Gui *arg)
{
    return arg->ActualAddedCallback();
}

// FUNCTION: TH07 0x0042d04b
ZunResult Gui::DeletedCallback(Gui *arg)
{
    g_AnmManager->ReleaseAnm(24);
    g_AnmManager->ReleaseAnm(28);
    g_AnmManager->ReleaseAnm(29);
    g_AnmManager->ReleaseAnm(30);
    g_AnmManager->ReleaseAnm(31);
    arg->FreeMsgFile();
    if ((u32)(g_Supervisor.curState != 3 &&
              g_Supervisor.curState != 11 &&
              g_Supervisor.curState != 12))
    {
        g_AnmManager->ReleaseAnm(21);
        g_AnmManager->ReleaseAnm(23);
        g_AnmManager->ReleaseAnm(25);
        g_AnmManager->ReleaseAnm(26);
        g_AnmManager->ReleaseAnm(27);
        g_AnmManager->ReleaseAnm(22);
        delete arg->impl;
        arg->impl = NULL;
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0042d136
ZunResult Gui::RegisterChain()
{
    Gui *mgr = &g_Gui;

    if ((u32)(g_Supervisor.curState != 3 && g_Supervisor.curState != 11 &&
              g_Supervisor.curState != 12) != 0)
    {
        memset(mgr, 0, sizeof(Gui));
        mgr->impl = new GuiImpl;
    }

    g_GuiCalcChain.callback = (ChainCallback)OnUpdate;
    g_GuiCalcChain.addedCallback = NULL;
    g_GuiCalcChain.deletedCallback = NULL;
    g_GuiCalcChain.addedCallback = (ChainLifecycleCallback)AddedCallback;
    g_GuiCalcChain.deletedCallback = (ChainLifecycleCallback)DeletedCallback;
    g_GuiCalcChain.arg = mgr;
    if (g_Chain.AddToCalcChain(&g_GuiCalcChain, 13))
    {
        return ZUN_ERROR;
    }

    g_GuiDrawChain.callback = (ChainCallback)OnDraw;
    g_GuiDrawChain.addedCallback = NULL;
    g_GuiDrawChain.deletedCallback = NULL;
    g_GuiDrawChain.arg = mgr;
    g_Chain.AddToDrawChain(&g_GuiDrawChain, 12);
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0042d24d
GuiImpl::GuiImpl()
{
}

// FUNCTION: TH07 0x0042d465
GuiMsgVm::GuiMsgVm()
{
}

// FUNCTION: TH07 0x0042d53d
void Gui::CutChain()
{
    g_Chain.Cut(&g_GuiCalcChain);
    g_Chain.Cut(&g_GuiDrawChain);
}
