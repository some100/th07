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

#pragma optimize("s", on)

// GLOBAL: TH07 0x0049fbf0
Gui g_Gui;

// GLOBAL: TH07 0x0062f914
ChainElem g_GuiCalcChain;

// GLOBAL: TH07 0x0062f8f4
ChainElem g_GuiDrawChain;

// GLOBAL: TH07 0x0049f618
D3DCOLOR g_SpellcardTimeColors[4] = {
    0xa0d0ff,
    0xa080ff,
    0xe080c0,
    0xff4040,
};

// FUNCTION: TH07 0x00427ae0
i32 Gui::IsStageFinished()
{
    i32 local_c;

    if ((this->impl->stageClearTextVm.activeSpriteIdx < 0) ||
        ((this->impl->stageClearTextVm.flags >> 0xd & 1) == 0))
    {
        local_c = 0;
    }
    else
    {
        local_c = 1;
    }
    return local_c;
}

// FUNCTION: TH07 0x00427b21
void Gui::EndPlayerSpellcard()
{
    this->impl->bombSpellcardName.pendingInterrupt = 1;
    this->impl->bombSpellcardNameBg.pendingInterrupt = 2;
}

// FUNCTION: TH07 0x00427b54
void Gui::EndEnemySpellcard()
{
    this->impl->enemySpellcardName.pendingInterrupt = 1;
    this->impl->enemySpellcardNameBg.pendingInterrupt = 2;
    this->impl->spellcardBonusIndicator.pendingInterrupt = 2;
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
    this->impl->bonusScore.pos.x = 416.0f;
    this->impl->bonusScore.pos.y = 48.0f;
    this->impl->bonusScore.pos.z = 0.0f;
    this->impl->bonusScore.isShown = 1;
    this->impl->bonusScore.timer.Initialize(0);
    this->impl->bonusScore.fmtArg = score;
    g_Supervisor.renderSkipFrames = 2;
}

// FUNCTION: TH07 0x00427c81
void Gui::ShowFullPowerMode(i32 fmtArg, i32 isShown)
{
    this->impl->fullPowerMode.pos.x = 416.0f;
    this->impl->fullPowerMode.pos.y = 168.0f;
    this->impl->fullPowerMode.pos.z = 0.0f;
    this->impl->fullPowerMode.isShown = isShown;
    this->impl->fullPowerMode.timer.Initialize(0);
    this->impl->fullPowerMode.fmtArg = fmtArg;
    g_Supervisor.renderSkipFrames = 2;
}

// FUNCTION: TH07 0x00427d09
void Gui::ShowSpellcardBonus(i32 fmtArg)
{
    this->impl->spellCardBonus.pos.x = 224.0f;
    this->impl->spellCardBonus.pos.y = 16.0f;
    this->impl->spellCardBonus.pos.z = 0.0f;
    this->impl->spellCardBonus.isShown = 1;
    this->impl->spellCardBonus.timer.Initialize(0);
    this->impl->spellCardBonus.fmtArg = fmtArg;
    g_Supervisor.renderSkipFrames = 2;
}

// FUNCTION: TH07 0x00427d92
void Gui::CopyTemplateSpriteToSpriteProbably(i32 spriteIdx)
{
    RECT local_24;
    RECT local_14;

    local_24.left = g_AnmManager->sprites[0x609].startPixelInclusive.x;
    local_24.top = g_AnmManager->sprites[0x609].startPixelInclusive.y;
    local_24.right = g_AnmManager->sprites[0x609].endPixelInclusive.x;
    local_24.bottom = g_AnmManager->sprites[0x609].endPixelInclusive.y;
    local_14.left = g_AnmManager->sprites[spriteIdx].startPixelInclusive.x;
    local_14.top = g_AnmManager->sprites[spriteIdx].startPixelInclusive.y;
    local_14.right = g_AnmManager->sprites[spriteIdx].endPixelInclusive.x;
    local_14.bottom = g_AnmManager->sprites[spriteIdx].endPixelInclusive.y;
    g_AnmManager->CopyTexture(0x15, 0x16, &local_24, &local_14);
}

// FUNCTION: TH07 0x00427e7c
u32 Gui::OnUpdate(Gui *arg)
{
    if (g_GameManager.isTimeStopped == 0)
    {
        if (arg->impl->transitionToScoreScreen != 0)
        {
            g_Supervisor.curState = 3;
            arg->impl->transitionToScoreScreen = 0;
        }
        arg->UpdateGui();
        arg->impl->RunMsg();
        arg->frameCounter = arg->frameCounter + 1;
        if ((g_GameManager.currentStage == 6) && (arg->frameCounter == 300))
        {
            Supervisor::PlayLoadedAudio(0);
        }
        if (((g_CurFrameGameInput & TH_BUTTON_SKIP) != 0) &&
            (g_Supervisor.renderSkipFrames < 8))
        {
            g_Supervisor.renderSkipFrames = 8;
        }
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00427f22
u32 Gui::OnDraw(Gui *arg)
{
    char local_30[32];
    D3DXVECTOR3 stringPos;

    (g_AnmManager->offset).y = 0.0f;
    (g_AnmManager->offset).x = 0.0f;
    if (arg->impl->finishedStage != 0)
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
        if ((6 < g_GameManager.currentStage) ||
            (((g_GameManager.currentStage == 6 &&
               ((g_GameManager.flags & 1) == 0)) &&
              (((g_GameManager.flags >> 3 & 1) == 0 ||
                (g_ReplayManager->data->head.stageReplayData[4].data != NULL))))))
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
        if (g_GameManager.difficulty == DIFF_EASY)
        {
            g_AsciiManager.color = 0xffff8080;
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                        "Easy Rank    *0.5");
        }
        else if (g_GameManager.difficulty == DIFF_NORMAL)
        {
            g_AsciiManager.color = 0xffff8080;
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                        "Normal Rank  *1.0");
        }
        else if (g_GameManager.difficulty == DIFF_HARD)
        {
            g_AsciiManager.color = 0xffff8080;
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                        "Hard Rank    *1.2");
        }
        else if (g_GameManager.difficulty == DIFF_LUNATIC)
        {
            g_AsciiManager.color = 0xffff8080;
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                        "Lunatic Rank *1.5");
        }
        else if (g_GameManager.difficulty == DIFF_EXTRA)
        {
            g_AsciiManager.color = 0xffff8080;
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                        "Extra Rank   *2.0");
        }
        else if (g_GameManager.difficulty == DIFF_PHANTASM)
        {
            g_AsciiManager.color = 0xffff8080;
            AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                        "Phantasm Rank*2.0");
        }
        stringPos.y = stringPos.y + 16.0f;
        if ((g_GameManager.difficulty < 4) && ((g_GameManager.flags & 1) == 0))
        {
            if ((g_GameManager.defaultCfg)->lifeCount == 3)
            {
                g_AsciiManager.color = 0xffff8080;
                AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                            "Player Penalty*0.5");
                stringPos.y = stringPos.y + 16.0f;
            }
            else if ((g_GameManager.defaultCfg)->lifeCount == 4)
            {
                g_AsciiManager.color = 0xffff8080;
                AsciiManager::AddFormatText(&g_AsciiManager, &stringPos,
                                            "Player Penalty*0.2");
                stringPos.y = stringPos.y + 16.0f;
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
    if ((arg->impl->bonusScore).isShown != 0)
    {
        g_AsciiManager.color = 0xffffff80;
        AsciiManager::AddFormatText(&g_AsciiManager, &(arg->impl->bonusScore).pos,
                                    "BONUS %8d", (arg->impl->bonusScore).fmtArg);
        g_AsciiManager.color = 0xffffffff;
    }
    if ((arg->impl->fullPowerMode).isShown == 1)
    {
        g_AsciiManager.color = 0xffc0b0ff;
        AsciiManager::AddFormatText(
            &g_AsciiManager, &(arg->impl->fullPowerMode).pos, "Full Power Mode!");
        g_AsciiManager.color = 0xffffffff;
    }
    else if ((arg->impl->fullPowerMode).isShown == 2)
    {
        g_AsciiManager.scale.x = 0.9f;
        g_AsciiManager.scale.y = 1.0f;
        g_AsciiManager.fontSpacing = 0xb;
        g_AsciiManager.color = 0xffe0b0ff;
        AsciiManager::AddFormatText(&g_AsciiManager,
                                    &(arg->impl->fullPowerMode).pos,
                                    "Supernatural Border!!");
        g_AsciiManager.color = 0xffffffff;
        g_AsciiManager.scale.x = 1.0f;
        g_AsciiManager.scale.y = 1.0f;
        g_AsciiManager.fontSpacing = 0xe;
    }
    else if ((arg->impl->fullPowerMode).isShown == 3)
    {
        g_AsciiManager.color = 0xffc0b0ff;
        AsciiManager::AddFormatText(
            &g_AsciiManager, &(arg->impl->fullPowerMode).pos, "CherryPoint Max!");
        g_AsciiManager.color = 0xffffffff;
    }
    else if ((arg->impl->fullPowerMode).isShown == 4)
    {
        g_AsciiManager.scale.x = 0.9f;
        g_AsciiManager.scale.y = 1.0f;
        g_AsciiManager.fontSpacing = 0xb;
        g_AsciiManager.color = 0xffe0b0ff;
        AsciiManager::AddFormatText(
            &g_AsciiManager, &(arg->impl->fullPowerMode).pos, "Border Bonus %7d",
            (arg->impl->fullPowerMode).fmtArg);
        g_AsciiManager.color = 0xffffffff;
        g_AsciiManager.scale.x = 1.0f;
        g_AsciiManager.scale.y = 1.0f;
        g_AsciiManager.fontSpacing = 0xe;
    }
    if ((arg->impl->spellCardBonus).isShown != 0)
    {
        g_AsciiManager.color = 0xffff0000;
        (arg->impl->spellCardBonus).pos.x = 88.0f;
        (arg->impl->spellCardBonus).pos.y = 80.0f;
        AsciiManager::AddFormatText(
            &g_AsciiManager, &(arg->impl->spellCardBonus).pos, "Spell Card Bonus!");
        (arg->impl->spellCardBonus).pos.y =
            (arg->impl->spellCardBonus).pos.y + 16.0f;
        sprintf(local_30, "+%d", (arg->impl->spellCardBonus).fmtArg);
        (arg->impl->spellCardBonus).pos.x =
            (384.0f - (f32)(u32)(strlen(local_30)) * 32.0f) / 2.0f + 32.0f;
        g_AsciiManager.scale.x = 2.0f;
        g_AsciiManager.scale.y = 2.0f;
        g_AsciiManager.color = 0xffff8080;
        g_AsciiManager.AddString(&(arg->impl->spellCardBonus).pos, local_30);
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
    this->impl->bombSpellcardPortrait.anmFileIdx = 0x4a1;
    g_AnmManager->SetAndExecuteScript(&this->impl->bombSpellcardPortrait,
                                      g_AnmManager->scripts[0x4a1]);
    g_AnmManager->SetActiveSprite(&this->impl->bombSpellcardPortrait, sprite);
    this->impl->bombSpellcardDecorLeft.anmFileIdx = 0x4a4;
    g_AnmManager->SetAndExecuteScript(&this->impl->bombSpellcardDecorLeft,
                                      g_AnmManager->scripts[0x4a4]);
    g_AnmManager->SetActiveSprite(&this->impl->bombSpellcardDecorLeft, 0x4ac);
    this->impl->bombSpellcardDecorRight.anmFileIdx = 0x4a6;
    g_AnmManager->SetAndExecuteScript(&this->impl->bombSpellcardDecorRight,
                                      g_AnmManager->scripts[0x4a6]);
    g_AnmManager->SetActiveSprite(&this->impl->bombSpellcardDecorRight, 0x4ac);
    this->impl->bombSpellcardName.anmFileIdx = 0x704;
    g_AnmManager->SetAndExecuteScript(&this->impl->bombSpellcardName,
                                      g_AnmManager->scripts[0x704]);
    AnmManager::DrawVmTextFmt(g_AnmManager, &this->impl->bombSpellcardName,
                              0xf0f0ff, 0, name);
    this->bombNameBarLength = (f32)(u32)((strlen(name)) * 0xf) / 2.0f + 16.0f;
    this->impl->bombSpellcardNameBg.pendingInterrupt = 1;
    g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB, 0);
    g_Supervisor.renderSkipFrames = 2;
}

// FUNCTION: TH07 0x00428887
void Gui::ShowSpellcard(i32 spellcardSprite, const char *spellcardName)
{
    if (-1 < spellcardSprite)
    {
        this->impl->enemySpellcardPortrait.anmFileIdx = 0x4a3;
        g_AnmManager->SetAndExecuteScript(&this->impl->enemySpellcardPortrait,
                                          g_AnmManager->scripts[0x4a3]);
        g_AnmManager->SetActiveSprite(&this->impl->enemySpellcardPortrait,
                                      spellcardSprite + 0x4ad);
        if (this->impl->enemySpellcardPortrait.sprite->widthPx <= 256.0f)
        {
            if (this->impl->enemySpellcardPortrait.sprite->widthPx <= 128.0f)
            {
                this->impl->enemySpellcardPortrait.offset.x = 0.0f;
            }
            else
            {
                this->impl->enemySpellcardPortrait.offset.x = -112.0f;
            }
        }
        else
        {
            this->impl->enemySpellcardPortrait.offset.x = -288.0f;
        }
    }
    (this->impl->enemySpellcardRelated1).anmFileIdx = 0x4a5;
    g_AnmManager->SetAndExecuteScript(&this->impl->enemySpellcardRelated1,
                                      g_AnmManager->scripts[0x4a5]);
    g_AnmManager->SetActiveSprite(&this->impl->enemySpellcardRelated1, 0x4ac);
    (this->impl->enemySpellcardRelated2).anmFileIdx = 0x4a7;
    g_AnmManager->SetAndExecuteScript(&this->impl->enemySpellcardRelated2,
                                      g_AnmManager->scripts[0x4a7]);
    g_AnmManager->SetActiveSprite(&this->impl->enemySpellcardRelated2, 0x4ac);
    this->impl->enemySpellcardName.anmFileIdx = 0x705;
    g_AnmManager->SetAndExecuteScript(&this->impl->enemySpellcardName,
                                      g_AnmManager->scripts[0x705]);
    g_AnmManager->DrawStringFormat(&this->impl->enemySpellcardName, 0xfff0f0, 0,
                                   spellcardName);
    this->spellcardBarLength =
        (f32)(u32)((strlen(spellcardName)) * 0xf) / 2.0f + 16.0f;
    this->impl->enemySpellcardNameBg.pendingInterrupt = 1;
    this->impl->spellcardBonusIndicator.pendingInterrupt = 1;
    g_SoundPlayer.PlaySoundByIdx(SOUND_BOMB, 0);
    g_Supervisor.renderSkipFrames = 2;
}

// FUNCTION: TH07 0x00428b19
ZunResult Gui::ActualAddedCallback()
{
    bool bVar1;
    u32 uVar8;
    AnmVm *pAVar9;
    i16 local_3c;
    i16 local_30;
    i32 local_10;
    i32 local_c;
    i32 local_8;

    this->frameCounter = 0;
    if (((g_Supervisor.curState == 3) || (g_Supervisor.curState == 0xb)) ||
        (g_Supervisor.curState == 0xc))
    {
        bVar1 = false;
    }
    else
    {
        bVar1 = true;
    }
    if (bVar1)
    {
        memset(this->impl, 0, sizeof(GuiImpl));

        if (g_AnmManager->LoadAnms(0x15, "data/front.anm", 0x600) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        ClearActiveSprites();
        if (g_GameManager.character == CHAR_REIMU)
        {
            if (g_AnmManager->LoadAnms(0x19, "data/face_rm00.anm", 0x4a0) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
            if (g_AnmManager->LoadAnms(0x17, "data/loading.anm", 0x61e) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
        }
        else if (g_GameManager.character == CHAR_MARISA)
        {
            if (g_AnmManager->LoadAnms(0x19, "data/face_mr00.anm", 0x4a0) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
            if (g_AnmManager->LoadAnms(0x17, "data/loading2.anm", 0x61e) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
        }
        else if (g_GameManager.character == CHAR_SAKUYA)
        {
            if (g_AnmManager->LoadAnms(0x19, "data/face_sk00.anm", 0x4a0) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
            if (g_AnmManager->LoadAnms(0x17, "data/loading3.anm", 0x61e) !=
                ZUN_SUCCESS)
            {
                return ZUN_ERROR;
            }
        }
    }
    else
    {
        ClearActiveSprites();
        this->impl->stageTransitionSnapshotVm.anmFileIdx = 0x725;
        g_AnmManager->SetAndExecuteScript(&this->impl->stageTransitionSnapshotVm,
                                          g_AnmManager->scripts[0x725]);
        this->impl->stageTransitionSnapshotVm.pendingInterrupt = 1;
        if (g_AnmManager->screenshotTextureId < 0)
        {
            g_AnmManager->screenshotTextureId = 4;
            g_AnmManager->screenshotSrcLeft = 0x20;
            g_AnmManager->screenshotSrcTop = 0x10;
            g_AnmManager->screenshotSrcWidth = 0x180;
            g_AnmManager->screenshotSrcHeight = 0x1c0;
            g_AnmManager->screenshotDstLeft =
                ((this->impl->stageTransitionSnapshotVm.sprite)->startPixelInclusive)
                    .x;
            g_AnmManager->screenshotDstTop =
                ((this->impl->stageTransitionSnapshotVm.sprite)->startPixelInclusive)
                    .y;
            g_AnmManager->screenshotDstWidth =
                (this->impl->stageTransitionSnapshotVm.sprite)->widthPx;
            g_AnmManager->screenshotDstHeight =
                (this->impl->stageTransitionSnapshotVm.sprite)->heightPx;
        }
        for (local_8 = 0; local_8 < 0xe; local_8 += 1)
        {
            for (local_c = 0; local_c < 0xc; local_c += 1)
            {
                uVar8 = local_8 + local_c & 1;
                local_30 = (i16)uVar8 + 0x726;
                pAVar9 = this->impl->transitionQuads + local_8 * 0xc + local_c;
                pAVar9->anmFileIdx = local_30;
                g_AnmManager->SetAndExecuteScript(pAVar9,
                                                  g_AnmManager->scripts[uVar8 + 0x726]);
                this->impl->transitionQuads[local_8 * 0xc + local_c].intVars2[0] =
                    local_8 + local_c * 2;
                this->impl->transitionQuads[local_8 * 0xc + local_c].pos.x =
                    ((f32)local_c * 32.0f - 0.5f) + 16.0f;
                this->impl->transitionQuads[local_8 * 0xc + local_c].pos.y =
                    ((f32)local_8 * 32.0f - 0.5f) + 16.0f;
                this->impl->transitionQuads[local_8 * 0xc + local_c].pos.z = 0.0f;
                this->impl->transitionQuads[local_8 * 0xc + local_c].uvScrollPos.x =
                    ((f32)local_c * 32.0f) / 512.0f;
                this->impl->transitionQuads[local_8 * 0xc + local_c].uvScrollPos.y =
                    ((f32)local_8 * 32.0f) / 512.0f;
            }
        }
        this->impl->activeTransitionQuads = 0xa8;
    }
    switch (g_GameManager.currentStage)
    {
    case 1:
        CopyTemplateSpriteToSpriteProbably(0x60e);
        if (g_AnmManager->LoadAnms(0x1c, "data/face_01_00.anm", 0x4ad) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(0x18, "data/std1txt.anm", 0x800) !=
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
        CopyTemplateSpriteToSpriteProbably(0x610);
        if (g_AnmManager->LoadAnms(0x1c, "data/face_02_00.anm", 0x4ad) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(0x18, "data/std2txt.anm", 0x800) !=
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
        CopyTemplateSpriteToSpriteProbably(0x612);
        if (g_AnmManager->LoadAnms(0x1c, "data/face_03_00.anm", 0x4ad) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(0x18, "data/std3txt.anm", 0x800) !=
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
        CopyTemplateSpriteToSpriteProbably(0x614);
        if (g_AnmManager->LoadAnms(0x1c, "data/face_04_00.anm", 0x4ad) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(0x18, "data/std4txt.anm", 0x800) !=
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
        CopyTemplateSpriteToSpriteProbably(0x616);
        if (g_AnmManager->LoadAnms(0x1c, "data/face_05_00.anm", 0x4ad) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(0x18, "data/std5txt.anm", 0x800) !=
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
        CopyTemplateSpriteToSpriteProbably(0x618);
        if (g_AnmManager->LoadAnms(0x1c, "data/face_06_00.anm", 0x4ad) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(0x18, "data/std6txt.anm", 0x800) !=
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
        CopyTemplateSpriteToSpriteProbably(0x61a);
        if (g_AnmManager->LoadAnms(0x1c, "data/face_07_00.anm", 0x4ad) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(0x18, "data/std7txt.anm", 0x800) !=
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
        CopyTemplateSpriteToSpriteProbably(0x61c);
        if (g_AnmManager->LoadAnms(0x1c, "data/face_08_00.anm", 0x4ad) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(0x18, "data/std8txt.anm", 0x800) !=
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
    if (((g_Supervisor.curState == 3) || (g_Supervisor.curState == 0xb)) ||
        (g_Supervisor.curState == 0xc))
    {
        bVar1 = false;
    }
    else
    {
        bVar1 = true;
    }
    if (bVar1)
    {
        for (local_10 = 0; local_10 < 0x21; local_10 += 1)
        {
            local_3c = (i16)local_10 + 0x600;
            pAVar9 = this->impl->vms0 + local_10;
            pAVar9->anmFileIdx = local_3c;
            g_AnmManager->SetAndExecuteScript(
                pAVar9, g_AnmManager->scripts[local_10 + 0x600]);
        }
    }
    this->bossPresent = 0;
    this->impl->bossHealthBarState = 0;
    this->bossHealthBar = 0.0f;
    this->bossHealthBarEased = 0.0f;
    this->impl->bombSpellcardPortrait.anmFileIdx = 0x4a1;
    g_AnmManager->SetAndExecuteScript(&this->impl->bombSpellcardPortrait,
                                      g_AnmManager->scripts[0x4a1]);
    this->impl->enemySpellcardPortrait.anmFileIdx = 0x4a3;
    g_AnmManager->SetAndExecuteScript(&this->impl->enemySpellcardPortrait,
                                      g_AnmManager->scripts[0x4a3]);
    this->impl->bombSpellcardName.anmFileIdx = 0x704;
    g_AnmManager->SetAndExecuteScript(&this->impl->bombSpellcardName,
                                      g_AnmManager->scripts[0x704]);
    this->impl->enemySpellcardName.anmFileIdx = 0x705;
    g_AnmManager->SetAndExecuteScript(&this->impl->enemySpellcardName,
                                      g_AnmManager->scripts[0x705]);
    AnmManager::ExecuteVmsAnms(this->impl->vms1, 0x800, 5);
    this->impl->bombSpellcardNameBg.anmFileIdx = 1;
    g_AnmManager->SetAndExecuteScript(&this->impl->bombSpellcardNameBg,
                                      g_AnmManager->scripts[1]);
    this->impl->enemySpellcardNameBg.anmFileIdx = 0;
    g_AnmManager->SetAndExecuteScript(&this->impl->enemySpellcardNameBg,
                                      g_AnmManager->scripts[0]);
    this->impl->spellcardBonusIndicator.anmFileIdx = 2;
    g_AnmManager->SetAndExecuteScript(&this->impl->spellcardBonusIndicator,
                                      g_AnmManager->scripts[2]);
    (this->impl->captureBonusVm).anmFileIdx = 3;
    g_AnmManager->SetAndExecuteScript(&this->impl->captureBonusVm,
                                      g_AnmManager->scripts[3]);
    this->impl->bombSpellcardPortrait.currentInstruction = NULL;
    this->impl->bombSpellcardDecorLeft.currentInstruction = NULL;
    this->impl->bombSpellcardDecorRight.currentInstruction = NULL;
    this->impl->bombSpellcardName.currentInstruction = NULL;
    this->impl->enemySpellcardPortrait.currentInstruction = NULL;
    (this->impl->enemySpellcardRelated1).currentInstruction = NULL;
    (this->impl->enemySpellcardRelated2).currentInstruction = NULL;
    this->impl->enemySpellcardName.currentInstruction = NULL;
    this->impl->bombSpellcardPortrait.flags =
        this->impl->bombSpellcardPortrait.flags & 0xfffffffe;
    this->impl->bombSpellcardDecorLeft.flags =
        this->impl->bombSpellcardDecorLeft.flags & 0xfffffffe;
    this->impl->bombSpellcardDecorRight.flags =
        this->impl->bombSpellcardDecorRight.flags & 0xfffffffe;
    this->impl->bombSpellcardName.flags =
        this->impl->bombSpellcardName.flags & 0xfffffffe;
    this->impl->enemySpellcardPortrait.flags =
        this->impl->enemySpellcardPortrait.flags & 0xfffffffe;
    (this->impl->enemySpellcardRelated1).flags =
        (this->impl->enemySpellcardRelated1).flags & 0xfffffffe;
    (this->impl->enemySpellcardRelated2).flags =
        (this->impl->enemySpellcardRelated2).flags & 0xfffffffe;
    this->impl->enemySpellcardName.flags =
        this->impl->enemySpellcardName.flags & 0xfffffffe;
    this->impl->bombSpellcardName.fontWidth = 0xf;
    this->impl->bombSpellcardName.fontHeight = 0xf;
    this->impl->enemySpellcardName.fontWidth = 0xf;
    this->impl->enemySpellcardName.fontHeight = 0xf;
    this->impl->msg.currentMsgIdx = -1;
    this->impl->finishedStage = 0;
    this->impl->bonusScore.isShown = 0;
    this->impl->fullPowerMode.isShown = 0;
    this->impl->spellCardBonus.isShown = 0;
    this->flags = (this->flags & 0xfffffffc) | 2;
    this->flags = (this->flags & 0xfffffff3) | 8;
    this->flags = (this->flags & 0xffffff3f) | 0x80;
    this->flags = (this->flags & 0xfffffcff) | 0x200;
    this->flags = (this->flags & 0xffffffcf) | 0x20;
    g_Supervisor.renderSkipFrames = 0x10;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00429935
ZunResult Gui::LoadMsg(const char *param_1)
{
    i32 iVar4;

    FreeMsgFile();
    this->impl->msg.msgFile = (MsgRawHeader *)FileSystem::OpenFile(param_1, 0);
    if (this->impl->msg.msgFile == NULL)
    {
        // STRING: TH07 0x00498108
        g_GameErrorContext.Log("error : メッセージファイル %s が読み込めませんでした\r\n", param_1);
        return ZUN_ERROR;
    }
    else
    {
        this->impl->msg.currentMsgIdx = -1;
        this->impl->msg.curInstr = NULL;
        for (iVar4 = 0; iVar4 < this->impl->msg.msgFile->numEntries; iVar4 += 1)
        {
            (&this->impl->msg.msgFile->entries)[iVar4] =
                (MsgRawInstr *)((u8 *)&this->impl->msg.msgFile->numEntries +
                                (i32)(&this->impl->msg.msgFile->entries)[iVar4]);
        }
        return ZUN_SUCCESS;
    }
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

    if (msgIdx < this->msg.msgFile->numEntries)
    {
        tmpMsgFile = this->msg.msgFile;
        memset(&this->msg, 0, sizeof(GuiMsgVm));
        this->msg.currentMsgIdx = msgIdx;
        this->msg.msgFile = tmpMsgFile;
        this->msg.curInstr = (&this->msg.msgFile->entries)[msgIdx];
        this->msg.dialogueLines[0].anmFileIdx = -1;
        this->msg.dialogueLines[1].anmFileIdx = -1;
        this->msg.fontSize = 0xf;
        this->msg.textColorsA[0].color = 0xe8f0ff;
        this->msg.textColorsA[1].color = 0xffe8f0;
        this->msg.textColorsB[0].color = 0;
        this->msg.textColorsB[1].color = 0;
        this->msg.dialogueSkippable = 1;
        g_BulletManager.RemoveAllBullets(1);
        g_EnemyManager.RemoveAllEnemies(0, 0);
        g_ItemManager.RemoveAllItems();
        if (msgIdx % 10 == 0)
        {
            switch (g_GameManager.currentStage)
            {
            case 1:
                Gui::CopyTemplateSpriteToSpriteProbably(0x60f);
                break;
            case 2:
                Gui::CopyTemplateSpriteToSpriteProbably(0x611);
                break;
            case 3:
                Gui::CopyTemplateSpriteToSpriteProbably(0x613);
                break;
            case 4:
                Gui::CopyTemplateSpriteToSpriteProbably(0x615);
                break;
            case 5:
                Gui::CopyTemplateSpriteToSpriteProbably(0x617);
                break;
            case 6:
                Gui::CopyTemplateSpriteToSpriteProbably(0x619);
                g_Stage.spellcardVmsIdx = 2;
                g_BulletManager.itemType = ITEM_STAR;
                break;
            case 7:
                Gui::CopyTemplateSpriteToSpriteProbably(0x61b);
                g_Stage.spellcardVmsIdx = 1;
                g_Stage.numSpellcardVms = 2;
                g_BulletManager.itemType = ITEM_STAR;
                break;
            case 8:
                Gui::CopyTemplateSpriteToSpriteProbably(0x61d);
                g_Stage.spellcardVmsIdx = 2;
                g_BulletManager.itemType = ITEM_STAR;
            }
        }
    }
}

// FUNCTION: TH07 0x00429c42
ZunResult GuiImpl::RunMsg()
{
    AnmVm *pAVar1;
    i16 sVar2;
    u32 uVar7;
    i16 local_3c;
    i16 local_20;
    i16 local_14;

    if (this->msg.currentMsgIdx < 0)
    {
        return ZUN_ERROR;
    }
    else
    {
        if (this->msg.ignoreWaitCounter != 0)
        {
            this->msg.ignoreWaitCounter = this->msg.ignoreWaitCounter - 1;
        }
        if ((this->msg.dialogueSkippable != 0) &&
            ((g_CurFrameGameInput & TH_BUTTON_SKIP) != 0))
        {
            this->msg.timer.Initialize((u32)this->msg.curInstr->time);
        }
        if (g_Player.hasBorder != BORDER_NONE)
        {
            g_Player.BreakBorderNaturally();
        }
        if (g_Player.playerState != PLAYER_STATE_DEAD)
        {
            g_ItemManager.RemoveAllItems();
        }
        while ((i32)(u32)this->msg.curInstr->time <= this->msg.timer.current)
        {
            switch (this->msg.curInstr->opcode)
            {
            case MSG_DELETE:
                this->msg.currentMsgIdx = -1;
                return ZUN_ERROR;
            case MSG_SHOW_PORTRAIT:
                uVar7 = this->msg.curInstr->args.dialogue.textColor != 0 ? 2 : 0;
                sVar2 = this->msg.curInstr->args.dialogue.textLine;
                local_14 = sVar2 + (i16)uVar7 + 0x4a0;
                pAVar1 =
                    this->msg.portraits + this->msg.curInstr->args.dialogue.textColor;
                pAVar1->anmFileIdx = local_14;
                g_AnmManager->SetAndExecuteScript(
                    pAVar1, g_AnmManager->scripts[uVar7 + sVar2 + 0x4a0]);
                if ((this->msg.portraits[this->msg.curInstr->args.dialogue.textColor]
                         .sprite)
                        ->widthPx <= 128.0f)
                {
                    this->msg.portraits[this->msg.curInstr->args.dialogue.textColor]
                        .offset.x = 0.0f;
                }
                else
                {
                    this->msg.portraits[this->msg.curInstr->args.dialogue.textColor]
                        .offset.x = -112.0f;
                }
                break;
            case MSG_CHANGE_FACE:
                g_AnmManager->SetActiveSprite(
                    this->msg.portraits + this->msg.curInstr->args.dialogue.textColor,
                    (i32)this->msg.curInstr->args.dialogue.textLine +
                        (this->msg.curInstr->args.dialogue.textColor != 0 ? 13 : 0) +
                        0x4a0);
                if ((this->msg.portraits[this->msg.curInstr->args.dialogue.textColor]
                         .sprite)
                        ->widthPx <= 256.0f)
                {
                    if ((this->msg.portraits[this->msg.curInstr->args.dialogue.textColor]
                             .sprite)
                            ->widthPx <= 128.0f)
                    {
                        this->msg.portraits[this->msg.curInstr->args.dialogue.textColor]
                            .offset.x = 0.0f;
                    }
                    else
                    {
                        this->msg.portraits[this->msg.curInstr->args.dialogue.textColor]
                            .offset.x = -80.0f;
                    }
                }
                else
                {
                    this->msg.portraits[this->msg.curInstr->args.dialogue.textColor]
                        .offset.x = -208.0f;
                    this->msg.portraits[this->msg.curInstr->args.dialogue.textColor]
                        .offset.y = -50.0f;
                }
                break;
            case MSG_DIALOGUE:
                if ((this->msg.curInstr->args.dialogue.textLine == 0) &&
                    (-1 < this->msg.dialogueLines[1].anmFileIdx))
                {
                    AnmManager::DrawVmTextFmt(
                        g_AnmManager, this->msg.dialogueLines + 1,
                        this->msg.textColorsA[this->msg.curInstr->args.dialogue.textColor]
                            .color,
                        this->msg.textColorsB[this->msg.curInstr->args.dialogue.textColor]
                            .color,
                        " ");
                }
                sVar2 = this->msg.curInstr->args.dialogue.textLine;
                local_20 = sVar2 + 0x700;
                pAVar1 = this->msg.dialogueLines +
                         this->msg.curInstr->args.dialogue.textLine;
                pAVar1->anmFileIdx = local_20;
                g_AnmManager->SetAndExecuteScript(pAVar1,
                                                  g_AnmManager->scripts[sVar2 + 0x700]);
                this->msg.dialogueLines[this->msg.curInstr->args.dialogue.textLine]
                    .fontHeight = (u8)this->msg.fontSize;
                this->msg.dialogueLines[this->msg.curInstr->args.dialogue.textLine]
                    .fontWidth =
                    this->msg.dialogueLines[this->msg.curInstr->args.dialogue.textLine]
                        .fontHeight;
                AnmManager::DrawVmTextFmt(
                    g_AnmManager,
                    this->msg.dialogueLines +
                        this->msg.curInstr->args.dialogue.textLine,
                    this->msg.textColorsA[this->msg.curInstr->args.dialogue.textColor]
                        .color,
                    this->msg.textColorsB[this->msg.curInstr->args.dialogue.textColor]
                        .color,
                    this->msg.curInstr->args.dialogue.text);
                this->msg.framesElapsedDuringPause = 0;
                break;
            case MSG_PAUSE:
                if ((((this->msg.dialogueSkippable == 0) ||
                      ((g_CurFrameGameInput & TH_BUTTON_SKIP) == 0)) &&
                     (((g_CurFrameGameInput & TH_BUTTON_SHOOT) == 0 ||
                       (((g_CurFrameGameInput & TH_BUTTON_SHOOT) ==
                             (g_LastFrameGameInput & TH_BUTTON_SHOOT) ||
                         (this->msg.framesElapsedDuringPause < 0xc)))))) &&
                    (this->msg.framesElapsedDuringPause <
                     this->msg.curInstr->args.pause.duration))
                {
                    this->msg.framesElapsedDuringPause =
                        this->msg.framesElapsedDuringPause + 1;
                    goto LAB_0042a766;
                }
                break;
            case MSG_SWITCH:
                if ((this->msg.curInstr->args.msgSwitch).portraitIdx < 2)
                {
                    // these are exactly the same?
                    this->msg.portraits[(this->msg.curInstr->args.msgSwitch).portraitIdx]
                        .pendingInterrupt =
                        (u16)this->msg.curInstr->args.msgSwitch.interrupt;
                }
                else
                {
                    this->msg.portraits[(this->msg.curInstr->args.msgSwitch).portraitIdx]
                        .pendingInterrupt =
                        (u16)this->msg.curInstr->args.msgSwitch.interrupt;
                }
                break;
            case MSG_APPEAR_ENEMY:
                this->msg.ignoreWaitCounter = this->msg.ignoreWaitCounter + 1;
                break;
            case MSG_MUSIC:
                if (g_GameManager.currentStage == 6)
                {
                    this->vms1[0].anmFileIdx = 0x805;
                    g_AnmManager->SetAndExecuteScript(this->vms1,
                                                      g_AnmManager->scripts[0x805]);
                }
                else
                {
                    this->vms1[0].anmFileIdx = 0x804;
                    g_AnmManager->SetAndExecuteScript(this->vms1,
                                                      g_AnmManager->scripts[0x804]);
                }
                g_AnmManager->SetActiveSprite(
                    this->vms1, this->msg.curInstr->args.portrait.portrait + 0x803);
                if (Supervisor::PlayLoadedAudio(
                        this->msg.curInstr->args.portrait.portrait) != ZUN_SUCCESS)
                {
                    Supervisor::PlayAudio(
                        g_Stage.stdData
                            ->bgmPaths[this->msg.curInstr->args.portrait.portrait]);
                }
                break;
            case MSG_TEXT_INTRODUCE:
                sVar2 = this->msg.curInstr->args.dialogue.textLine;
                local_3c = sVar2 + 0x702;
                pAVar1 =
                    this->msg.introLines + this->msg.curInstr->args.dialogue.textLine;
                pAVar1->anmFileIdx = local_3c;
                g_AnmManager->SetAndExecuteScript(pAVar1,
                                                  g_AnmManager->scripts[sVar2 + 0x702]);
                g_AnmManager->DrawStringFormat(
                    this->msg.introLines + this->msg.curInstr->args.dialogue.textLine,
                    this->msg.textColorsA[this->msg.curInstr->args.dialogue.textColor]
                        .color,
                    this->msg.textColorsB[this->msg.curInstr->args.dialogue.textColor]
                        .color,
                    this->msg.curInstr->args.dialogue.text);
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
                    (this->stageClearTextVm).anmFileIdx = 0x61e;
                    g_AnmManager->SetAndExecuteScript(&this->stageClearTextVm,
                                                      g_AnmManager->scripts[0x61e]);
                    this->stageTransitionSnapshotVm.anmFileIdx = 0x725;
                    g_AnmManager->SetAndExecuteScript(&this->stageTransitionSnapshotVm,
                                                      g_AnmManager->scripts[0x725]);
                    if (g_AnmManager->screenshotTextureId < 0)
                    {
                        g_AnmManager->screenshotTextureId = 4;
                        g_AnmManager->screenshotSrcLeft = 0x20;
                        g_AnmManager->screenshotSrcTop = 0x10;
                        g_AnmManager->screenshotSrcWidth = 0x180;
                        g_AnmManager->screenshotSrcHeight = 0x1c0;
                        g_AnmManager->screenshotDstLeft =
                            this->stageTransitionSnapshotVm.sprite->startPixelInclusive.x;
                        g_AnmManager->screenshotDstTop =
                            this->stageTransitionSnapshotVm.sprite->startPixelInclusive.y;
                        g_AnmManager->screenshotDstWidth =
                            this->stageTransitionSnapshotVm.sprite->widthPx;
                        g_AnmManager->screenshotDstHeight =
                            this->stageTransitionSnapshotVm.sprite->heightPx;
                    }
                }
                else
                {
                    g_GameManager.globals->extendsFromPointItems = -1;
                }
                break;
            case MSG_FREEZE:
                goto LAB_0042a766;
            case MSG_NEXT_LEVEL:
                g_Supervisor.checkTiming = 0;
                g_GameManager.globals->guiScore = g_GameManager.globals->score;
                if ((g_GameManager.flags & 1) == 0)
                {
                    if (g_GameManager.currentStage < 6)
                    {
                        if (((g_GameManager.flags >> 3 & 1) == 0) ||
                            (g_ReplayManager->data->head
                                 .stageReplayData[g_GameManager.currentStage]
                                 .data != NULL))
                        {
                            this->stageClearBonusTextVm.Initialize();
                            g_AnmManager->SetActiveSprite(&this->stageClearBonusTextVm,
                                                          0x10c);
                            this->transitionToScoreScreen = 1;
                            this->msg.currentMsgIdx = -2;
                        }
                        else
                        {
                            g_Supervisor.curState = 7;
                        }
                    }
                    else if ((g_GameManager.flags >> 3 & 1) == 0)
                    {
                        if (g_GameManager.difficulty < 4)
                        {
                            g_GameManager.flags |= 0x10;
                            g_GameManager.globals->guiScore = g_GameManager.globals->score;
                            g_Supervisor.curState = 9;
                        }
                        else
                        {
                            if (g_GameManager.difficulty == DIFF_EXTRA)
                            {
                                g_GameManager.clrd[g_GameManager.shotTypeAndCharacter]
                                    .difficultyClearedWithRetries[4] = 99;
                            }
                            ((Plst *)(g_GameManager.pscr + 6))
                                ->playDataByDifficulty[g_GameManager.difficulty]
                                .noContinueClearCount =
                                ((Plst *)(g_GameManager.pscr + 6))
                                    ->playDataByDifficulty[g_GameManager.difficulty]
                                    .noContinueClearCount +
                                1;
                            g_GameManager.flags |= 0x10;
                            g_GameManager.globals->guiScore = g_GameManager.globals->score;
                            g_Supervisor.curState = 6;
                        }
                    }
                    else
                    {
                        if ((g_GameManager.currentStage == 8) &&
                            (g_GameManager.globals->score !=
                             g_ReplayManager->data->data.score))
                        {
                            ReplayManager::SaveReplay2(g_GameManager.replayFilename);
                        }
                        g_Supervisor.curState = 7;
                    }
                }
                else
                {
                    g_GameManager.globals->guiScore = g_GameManager.globals->score;
                    g_Supervisor.curState = 6;
                }
                goto LAB_0042a766;
            case MSG_FADEOUT_MUSIC:
                g_Supervisor.FadeOutMusic(4.0f);
                break;
            case MSG_ALLOW_SKIP:
                this->msg.dialogueSkippable = *(u8 *)&this->msg.curInstr->args;
                break;
            case MSG_FADE_IN_EFFECT:
                BombEffects::RegisterChain(4, 0x192, 0xffffff, 0, 0);
                g_Supervisor.renderSkipFrames = 0x192;
            }
            this->msg.curInstr = (MsgRawInstr *)((u8 *)&this->msg.curInstr->args +
                                                 this->msg.curInstr->argsize);
        }
        this->msg.timer.NextTick();
    LAB_0042a766:
        g_AnmManager->ExecuteScript(this->msg.portraits);
        g_AnmManager->ExecuteScript(this->msg.portraits + 1);
        g_AnmManager->ExecuteScript(this->msg.dialogueLines);
        g_AnmManager->ExecuteScript(this->msg.dialogueLines + 1);
        g_AnmManager->ExecuteScript(this->msg.introLines);
        g_AnmManager->ExecuteScript(this->msg.introLines + 1);
        if (((this->msg.timer.current < 0x3c) &&
             (this->msg.dialogueSkippable != 0)) &&
            ((g_CurFrameGameInput & TH_BUTTON_SKIP) != 0))
        {
            this->msg.timer.Initialize(0x3c);
        }
        return ZUN_SUCCESS;
    }
}

// FUNCTION: TH07 0x0042a876
ZunResult GuiImpl::DrawDialogue()
{
    f32 fVar1;
    f32 fVar2;
    f32 fVar3;
    VertexDiffuseXyzrhw local_5c[4];
    f32 local_8;

    if (this->msg.currentMsgIdx < 0)
    {
        return ZUN_ERROR;
    }
    else if ((g_GameManager.currentStage == 6) &&
             ((this->msg.currentMsgIdx == 1 ||
               (this->msg.currentMsgIdx == 0xb))))
    {
        return ZUN_SUCCESS;
    }
    else
    {
        if (this->msg.timer.current < 0x3c)
        {
            local_8 =
                (((f32)this->msg.timer.current + this->msg.timer.subFrame) * 48.0f) /
                60.0f;
        }
        else
        {
            local_8 = 48.0f;
        }
        local_5c[0].pos.x = g_GameManager.arcadeRegionTopLeftPos.x + 16.0f;
        local_5c[0].pos.y = 384.0f;
        local_5c[0].pos.z = 0.0f;
        local_5c[1].pos.x =
            (g_GameManager.arcadeRegionTopLeftPos.x + 384.0f) - 16.0f;
        local_5c[1].pos.y = 384.0f;
        local_5c[1].pos.z = 0.0f;
        local_5c[2].pos.x = g_GameManager.arcadeRegionTopLeftPos.x + 16.0f;
        local_5c[2].pos.y = local_8 + 384.0f;
        local_5c[2].pos.z = 0.0f;
        local_5c[3].pos.x =
            (g_GameManager.arcadeRegionTopLeftPos.x + 384.0f) - 16.0f;
        local_5c[3].pos.y = local_8 + 384.0f;
        local_5c[3].pos.z = 0.0f;
        local_5c[1].diffuse.color = 0xd0000000;
        local_5c[0].diffuse.color = 0xd0000000;
        local_5c[3].diffuse.color = 0x90000000;
        local_5c[2].diffuse.color = 0x90000000;
        local_5c[3].w = 1.0f;
        local_5c[2].w = 1.0f;
        local_5c[1].w = 1.0f;
        local_5c[0].w = 1.0f;
        g_AnmManager->DrawNoRotation(&this->msg.portraits[0]);
        fVar1 = this->msg.portraits[1].pos.x;
        fVar2 = this->msg.portraits[1].pos.y;
        fVar3 = this->msg.portraits[1].pos.z;
        this->msg.portraits[1].pos.x += this->msg.portraits[1].offset.x;
        this->msg.portraits[1].pos.y += this->msg.portraits[1].offset.y;
        this->msg.portraits[1].pos.z += this->msg.portraits[1].offset.z;
        g_AnmManager->DrawNoRotation(&this->msg.portraits[1]);
        this->msg.portraits[1].pos.x = fVar1;
        this->msg.portraits[1].pos.y = fVar2;
        this->msg.portraits[1].pos.z = fVar3;
        g_AnmManager->Flush();
        if ((g_Supervisor.cfg.opts >> 8 & 1) == 0)
        {
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 2);
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 2);
        }
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 0);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 0);
        if ((g_Supervisor.cfg.opts >> 6 & 1) == 0)
        {
            g_Supervisor.SetRenderState(D3DRS_ZWRITEENABLE, 0);
        }
        g_Supervisor.d3dDevice->SetVertexShader(0x44);
        g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, local_5c,
                                                sizeof(VertexDiffuseXyzrhw));
        g_AnmManager->currentVertexShader = 0xff;
        g_AnmManager->currentColorOp = 0xff;
        g_AnmManager->currentBlendMode = 0xff;
        g_AnmManager->currentZWriteDisable = 0xff;
        if ((g_Supervisor.cfg.opts >> 8 & 1) == 0)
        {
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 4);
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 4);
        }
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 2);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 2);
        g_AnmManager->DrawNoRotation(this->msg.dialogueLines);
        g_AnmManager->DrawNoRotation(this->msg.dialogueLines + 1);
        g_AnmManager->DrawNoRotation(this->msg.introLines);
        g_AnmManager->DrawNoRotation(this->msg.introLines + 1);
        return ZUN_SUCCESS;
    }
}

// FUNCTION: TH07 0x0042ad29
i32 Gui::MsgWait()
{
    if (this->impl == NULL)
    {
        return 0;
    }
    else if (this->impl->msg.ignoreWaitCounter == 0)
    {
        return -1 < this->impl->msg.currentMsgIdx;
    }
    else
    {
        return 0;
    }
}

// FUNCTION: TH07 0x0042ad66
i32 Gui::HasCurrentMsgIdx()
{
    if (this->impl == NULL)
    {
        return 0;
    }
    else if ((this->impl->msg.currentMsgIdx < 0) &&
             (this->impl->msg.currentMsgIdx != -2))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

// FUNCTION: TH07 0x0042adab
void Gui::UpdateGui()
{
    i32 local_10;
    i32 local_c;
    i32 local_8;

    if (this->impl->msg.currentMsgIdx < 0)
    {
        if (this->bossPresent == 0)
        {
            if (this->impl->bossHealthBarState != 0)
            {
                if (this->impl->bossHealthBarState < 3)
                {
                    this->impl->vms0[0xb].pendingInterrupt = 2;
                    this->impl->bossHealthBarState = 3;
                }
                if (this->bossHealthBarAlpha == 0)
                {
                    this->bossHealthBarAlpha = 0;
                }
                else
                {
                    this->bossHealthBarAlpha = this->bossHealthBarAlpha - 4;
                }
                if ((this->impl->vms0[0xb].flags >> 0xd & 1) != 0)
                {
                    this->impl->bossHealthBarState = 0;
                    this->bossHealthBarEased = 0.0f;
                    this->bossHealthBarAlpha = 0;
                }
            }
        }
        else if (this->impl->bossHealthBarState == 0)
        {
            this->impl->vms0[0xb].pendingInterrupt = 1;
            this->impl->bossHealthBarState = 1;
            this->bossHealthBarAlpha = 0;
        }
        else
        {
            if ((this->impl->vms0[0xb].flags >> 0xd & 1) != 0)
            {
                this->impl->bossHealthBarState = 2;
            }
            if (this->bossHealthBarAlpha < 0xfc)
            {
                this->bossHealthBarAlpha = this->bossHealthBarAlpha + 4;
            }
            else
            {
                this->bossHealthBarAlpha = 0xff;
            }
        }
        if (1 < this->impl->bossHealthBarState)
        {
            if (this->bossHealthBar <= this->bossHealthBarEased)
            {
                if ((this->bossHealthBar < this->bossHealthBarEased) &&
                    (this->bossHealthBarEased = this->bossHealthBarEased - 0.02f,
                     this->bossHealthBarEased < this->bossHealthBar))
                {
                    this->bossHealthBarEased = this->bossHealthBar;
                }
            }
            else
            {
                this->bossHealthBarEased = this->bossHealthBarEased + 0.01f;
                if (this->bossHealthBar < this->bossHealthBarEased)
                {
                    this->bossHealthBarEased = this->bossHealthBar;
                }
            }
        }
    }
    g_AnmManager->ExecuteScripts(this->impl->vms0, 0x21);
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
    if (-1 < this->impl->stageClearTextVm.activeSpriteIdx)
    {
        if (g_AnmManager->ExecuteScript(&this->impl->stageClearTextVm) != 0)
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
        local_8 = 0xa8;
        for (local_c = 0; local_c < 0xa8; local_c += 1)
        {
            if (g_AnmManager->ExecuteScript(this->impl->transitionQuads + local_c) !=
                0)
            {
                local_8 += -1;
            }
        }
        this->impl->activeTransitionQuads = local_8;
    }
    if (this->impl->bonusScore.isShown != 0)
    {
        if (this->impl->bonusScore.timer.current < 0x1e)
        {
            this->impl->bonusScore.pos.x =
                (((f32)this->impl->bonusScore.timer.current +
                  this->impl->bonusScore.timer.subFrame) *
                 -312.0f) /
                    30.0f +
                416.0f;
        }
        else
        {
            this->impl->bonusScore.pos.x = 104.0f;
        }
        if (0xf9 < this->impl->bonusScore.timer.current)
        {
            this->impl->bonusScore.isShown = 0;
        }
        this->impl->bonusScore.timer.NextTick();
    }
    if (this->impl->fullPowerMode.isShown != 0)
    {
        if (this->impl->fullPowerMode.timer.current < 0x1e)
        {
            this->impl->fullPowerMode.pos.x =
                (((f32)this->impl->fullPowerMode.timer.current +
                  this->impl->fullPowerMode.timer.subFrame) *
                 -312.0f) /
                    30.0f +
                416.0f;
        }
        else
        {
            this->impl->fullPowerMode.pos.x = 104.0f;
        }
        if (0xb3 < this->impl->fullPowerMode.timer.current)
        {
            this->impl->fullPowerMode.isShown = 0;
        }
        this->impl->fullPowerMode.timer.NextTick();
    }
    if (this->impl->spellCardBonus.isShown != 0)
    {
        if (0x117 < this->impl->spellCardBonus.timer.current)
        {
            this->impl->spellCardBonus.isShown = 0;
        }
        this->impl->spellCardBonus.timer.NextTick();
    }
    if (this->impl->finishedStage == 1)
    {
        local_10 = g_GameManager.currentStage * 100000 +
                   this->impl->clearGraze * 0x32 +
                   this->impl->clearPointItems * 5000 + this->impl->clearCherryMax;
        if ((6 < g_GameManager.currentStage) ||
            (((g_GameManager.currentStage == 6 &&
               ((g_GameManager.flags & 1) == 0)) &&
              (((g_GameManager.flags >> 3 & 1) == 0 ||
                (g_ReplayManager->data->head.stageReplayData[4].data != NULL))))))
        {
            local_10 = local_10 + g_GameManager.globals->livesRemaining * 2000000 +
                       g_GameManager.globals->bombsRemaining * 400000;
        }
        if (g_GameManager.difficulty == DIFF_EASY)
        {
            local_10 /= 2;
        }
        else if (g_GameManager.difficulty == DIFF_HARD)
        {
            local_10 = (local_10 * 0xc) / 10;
        }
        else if (g_GameManager.difficulty == DIFF_LUNATIC)
        {
            local_10 = (local_10 * 0xf) / 10;
        }
        else if (g_GameManager.difficulty == DIFF_EXTRA)
        {
            local_10 <<= 1;
        }
        else if (g_GameManager.difficulty == DIFF_PHANTASM)
        {
            local_10 <<= 1;
        }
        if ((g_GameManager.defaultCfg)->lifeCount == 3)
        {
            local_10 = (local_10 * 5) / 10;
        }
        else if ((g_GameManager.defaultCfg)->lifeCount == 4)
        {
            local_10 = (local_10 << 1) / 10;
        }
        this->impl->stageClearBonus = local_10;
        g_GameManager.globals->score = local_10 / 10 + g_GameManager.globals->score;
        g_GameManager.globals->score = local_10 / 10 + g_GameManager.globals->score;
        g_GameManager.globals->score = local_10 / 10 + g_GameManager.globals->score;
        g_GameManager.globals->score = local_10 / 10 + g_GameManager.globals->score;
        g_GameManager.globals->score = local_10 / 10 + g_GameManager.globals->score;
        g_GameManager.globals->score = local_10 / 10 + g_GameManager.globals->score;
        g_GameManager.globals->score = local_10 / 10 + g_GameManager.globals->score;
        g_GameManager.globals->score = local_10 / 10 + g_GameManager.globals->score;
        g_GameManager.globals->score = local_10 / 10 + g_GameManager.globals->score;
        g_GameManager.globals->score = local_10 / 10 + g_GameManager.globals->score;
        this->impl->finishedStage = this->impl->finishedStage + 1;
    }
}

// FUNCTION: TH07 0x0042b603
void Gui::DrawGameScene()
{
    D3DXVECTOR3 local_194;
    D3DXVECTOR3 local_188;
    D3DXVECTOR3 local_17c;
    D3DXVECTOR3 local_170;
    D3DXVECTOR3 local_164;
    D3DXVECTOR3 local_158;
    D3DXVECTOR3 local_14c;
    D3DXVECTOR3 local_140;
    D3DXVECTOR3 local_134;
    D3DXVECTOR3 local_128;
    D3DXVECTOR3 local_11c;
    D3DXVECTOR3 local_110;
    D3DXVECTOR3 local_104;
    D3DXVECTOR3 local_f8;
    D3DXVECTOR3 local_ec;
    D3DXVECTOR3 local_e0;
    D3DXVECTOR3 local_d4;
    D3DXVECTOR3 local_c8;
    VertexDiffuseXyzrhw local_74[4];
    D3DXVECTOR3 local_20;
    AnmVm *local_14;
    i32 local_10;
    f32 local_c;
    f32 local_8;

    g_AnmManager->Flush();
    g_Supervisor.viewport.X = 0;
    g_Supervisor.viewport.Y = 0;
    g_Supervisor.viewport.Width = 0x280;
    g_Supervisor.viewport.Height = 0x1e0;
    g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    local_14 = this->impl->vms0 + 0xc;
    if ((((g_Supervisor.cfg.opts >> 0xc & 1) != 0) ||
         (this->impl->vms0[0xc].currentInstruction != NULL)) ||
        (g_Supervisor.renderSkipFrames != 0))
    {
        for (local_8 = 0.0f; local_8 < 464.0f; local_8 = local_8 + 32.0f)
        {
            (local_14->pos).x = 0.0f;
            (local_14->pos).y = local_8;
            (local_14->pos).z = 0.49f;
            g_AnmManager->DrawNoRotation(local_14);
        }
        for (local_c = 416.0f; local_c < 624.0f; local_c = local_c + 32.0f)
        {
            for (local_8 = 16.0f; local_8 < 464.0f; local_8 = local_8 + 32.0f)
            {
                (local_14->pos).x = local_c;
                (local_14->pos).y = local_8;
                (local_14->pos).z = 0.49f;
                g_AnmManager->DrawNoRotation(local_14);
            }
        }
        local_14 = this->impl->vms0 + 0xd;
        for (local_c = 0.0f; local_c < 624.0f; local_c = local_c + 128.0f)
        {
            (local_14->pos).x = local_c;
            (local_14->pos).y = 0.0f;
            (local_14->pos).z = 0.49f;
            g_AnmManager->DrawNoRotation(local_14);
            (local_14->pos).x = local_c;
            (local_14->pos).y = 464.0f;
            (local_14->pos).z = 0.49f;
            g_AnmManager->DrawNoRotation(local_14);
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
        this->flags = (this->flags & 0xfffffffc) | 2;
        this->flags = (this->flags & 0xfffffff3) | 8;
        this->flags = (this->flags & 0xffffff3f) | 0x80;
        this->flags = (this->flags & 0xfffffcff) | 0x200;
        this->flags = (this->flags & 0xffffffcf) | 0x20;
    }
    if ((g_Supervisor.cfg.opts >> 4 & 1) == 0)
    {
        local_14 = this->impl->vms0 + 0xd;
        local_c = 496.0f;
        this->impl->vms0[0xd].pos.x = 496.0f;
        this->impl->vms0[0xd].pos.y = 48.0f;
        this->impl->vms0[0xd].pos.z = 0.49f;
        g_AnmManager->DrawNoRotation(local_14);
        (local_14->pos).x = local_c;
        (local_14->pos).y = 64.0f;
        (local_14->pos).z = 0.49f;
        g_AnmManager->DrawNoRotation(local_14);
        if ((this->flags & 3) != 0)
        {
            (local_14->pos).x = local_c;
            (local_14->pos).y = 96.0f;
            (local_14->pos).z = 0.48f;
            g_AnmManager->DrawNoRotation(local_14);
        }
        if ((this->flags >> 2 & 3) != 0)
        {
            (local_14->pos).x = local_c;
            (local_14->pos).y = 112.0f;
            (local_14->pos).z = 0.48f;
            g_AnmManager->DrawNoRotation(local_14);
        }
        if ((this->flags >> 4 & 3) != 0)
        {
            (local_14->pos).x = local_c;
            (local_14->pos).y = 144.0f;
            (local_14->pos).z = 0.48f;
            g_AnmManager->DrawNoRotation(local_14);
        }
        if ((this->flags >> 6 & 3) != 0)
        {
            (local_14->pos).x = local_c;
            (local_14->pos).y = 160.0f;
            (local_14->pos).z = 0.48f;
            g_AnmManager->DrawNoRotation(local_14);
        }
        if ((this->flags >> 8 & 3) != 0)
        {
            (local_14->pos).x = local_c;
            (local_14->pos).y = 176.0f;
            (local_14->pos).z = 0.48f;
            g_AnmManager->DrawNoRotation(local_14);
        }
        (local_14->pos).x = 512.0f;
        (local_14->pos).y = 464.0f;
        (local_14->pos).z = 0.48f;
        g_AnmManager->DrawNoRotation(local_14);
    }
    if ((this->flags & 3) != 0)
    {
        local_14 = this->impl->vms0 + 9;
        local_10 = 0;
        local_c = 496.0f;
        while (local_10 < (i32)g_GameManager.globals->livesRemaining)
        {
            (local_14->pos).x = local_c;
            (local_14->pos).y = 96.0f;
            (local_14->pos).z = 0.46f;
            g_AnmManager->DrawNoRotation(local_14);
            local_10 += 1;
            local_c = local_c + 16.0f;
        }
    }
    if ((this->flags >> 2 & 3) != 0)
    {
        local_14 = this->impl->vms0 + 10;
        local_10 = 0;
        local_c = 496.0f;
        while (local_10 < (i32)g_GameManager.globals->bombsRemaining)
        {
            (local_14->pos).x = local_c;
            (local_14->pos).y = 112.0f;
            (local_14->pos).z = 0.46f;
            g_AnmManager->DrawNoRotation(local_14);
            local_10 += 1;
            local_c = local_c + 16.0f;
        }
    }
    local_14 = this->impl->vms0 + 0xd;
    for (local_c = 32.0f; local_c < 368.0f; local_c = local_c + 128.0f)
    {
        (local_14->pos).x = local_c;
        (local_14->pos).y = 464.0f;
        (local_14->pos).z = 0.49f;
        g_AnmManager->DrawNoRotation(local_14);
    }
    local_20.x = 496.0f;
    local_20.y = 64.0f;
    local_20.z = 0.0f;
    if (g_GameManager.globals->guiScore < 100000000)
    {
        AsciiManager::AddFormatText(&g_AsciiManager, &local_20, "%.8d",
                                    g_GameManager.globals->guiScore);
        local_20.x = local_20.x + 112.0f;
        AsciiManager::AddFormatText(&g_AsciiManager, &local_20, "%1d",
                                    (u32)g_GameManager.globals->numRetries);
    }
    else
    {
        g_AsciiManager.scale.x = 0.9f;
        g_AsciiManager.scale.y = 1.0f;
        AsciiManager::AddFormatText(&g_AsciiManager, &local_20, "%.9d",
                                    g_GameManager.globals->guiScore);
        local_20.x = local_20.x + 113.399994f;
        AsciiManager::AddFormatText(&g_AsciiManager, &local_20, "%1d",
                                    (u32)g_GameManager.globals->numRetries);
        g_AsciiManager.scale.x = 1.0f;
        g_AsciiManager.scale.y = 1.0f;
    }
    local_20.x = 496.0f;
    local_20.y = 48.0f;
    local_20.z = 0.0f;
    if (g_GameManager.globals->highScore < 100000000)
    {
        AsciiManager::AddFormatText(&g_AsciiManager, &local_20, "%.8d",
                                    g_GameManager.globals->highScore);
        local_20.x = local_20.x + 112.0f;
        AsciiManager::AddFormatText(
            &g_AsciiManager, &local_20, "%1d",
            (u32)g_GameManager.globals->highScoreNumContinues);
    }
    else
    {
        g_AsciiManager.scale.x = 0.9f;
        g_AsciiManager.scale.y = 1.0f;
        AsciiManager::AddFormatText(&g_AsciiManager, &local_20, "%.9d",
                                    g_GameManager.globals->highScore);
        local_20.x = local_20.x + 113.399994f;
        AsciiManager::AddFormatText(
            &g_AsciiManager, &local_20, "%1d",
            (u32)g_GameManager.globals->highScoreNumContinues);
        g_AsciiManager.scale.x = 1.0f;
        g_AsciiManager.scale.y = 1.0f;
    }
    if (((this->flags >> 6 & 3) != 0) ||
        ((g_Supervisor.cfg.opts >> 4 & 1) != 0))
    {
        local_20.x = 496.0f;
        local_20.y = 160.0f;
        local_20.z = 0.0f;
        AsciiManager::AddFormatText(&g_AsciiManager, &local_20, "%d",
                                    g_GameManager.globals->grazeInTotal);
    }
    if (((this->flags >> 8 & 3) != 0) ||
        ((g_Supervisor.cfg.opts >> 4 & 1) != 0))
    {
        local_20.x = 496.0f;
        local_20.y = 176.0f;
        local_20.z = 0.0f;
        AsciiManager::AddFormatText(
            &g_AsciiManager, &local_20, "%d/%d",
            g_GameManager.globals->pointItemsCollectedForExtend,
            g_GameManager.globals->nextNeededPointItemsForExtend);
    }
    g_AnmManager->Flush();
    if (((this->flags >> 4 & 3) != 0) ||
        ((g_Supervisor.cfg.opts >> 4 & 1) != 0))
    {
        if (0 < (i32)g_GameManager.globals->currentPower)
        {
            local_74[0].pos.x = 496.0f;
            local_74[0].pos.y = 144.0f;
            local_74[0].pos.z = 0.1f;
            local_74[1].pos.x =
                (f32)(i32)((g_GameManager.globals)->currentPower + 0x1f0) + 0.0f;
            local_74[1].pos.y = 144.0f;
            local_74[1].pos.z = 0.1f;
            local_74[2].pos.x = 496.0f;
            local_74[2].pos.y = 160.0f;
            local_74[2].pos.z = 0.1f;
            local_17c.x =
                (f32)(i32)((g_GameManager.globals)->currentPower + 0x1f0) + 0.0f;
            local_17c.y = 160.0f;
            local_17c.z = 0.1f;
            local_74[3].pos.y = 160.0f;
            local_74[3].pos.z = 0.1f;
            local_74[2].diffuse.color = 0xe0e0e0ff;
            local_74[0].diffuse.color = 0xe0e0e0ff;
            local_74[3].diffuse.color = 0x80e0e0ff;
            local_74[1].diffuse.color = 0x80e0e0ff;
            local_74[3].w = 1.0;
            local_74[2].w = 1.0;
            local_74[1].w = 1.0;
            local_74[0].w = 1.0;
            local_74[3].pos.x = local_17c.x;
            if ((g_Supervisor.cfg.opts >> 8 & 1) == 0)
            {
                g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 2);
                g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 2);
            }
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 0);
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 0);
            if ((g_Supervisor.cfg.opts >> 6 & 1) == 0)
            {
                g_Supervisor.SetRenderState(D3DRS_ZWRITEENABLE, 0);
            }
            g_Supervisor.d3dDevice->SetVertexShader(0x44);
            g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &local_74,
                                                    sizeof(VertexDiffuseXyzrhw));
            g_AnmManager->currentVertexShader = 0xff;
            g_AnmManager->currentColorOp = 0xff;
            g_AnmManager->currentBlendMode = 0xff;
            g_AnmManager->currentZWriteDisable = 0xff;
            if ((g_Supervisor.cfg.opts >> 8 & 1) == 0)
            {
                g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 4);
                g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 4);
            }
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 2);
            g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 2);
        }
        if ((i32)g_GameManager.globals->currentPower < 128)
        {
            local_188.x = 496.0f;
            local_188.y = 144.0f;
            local_188.z = 0.0f;
            AsciiManager::AddFormatText(&g_AsciiManager, &local_188, "%d",
                                        (i32)g_GameManager.globals->currentPower);
        }
        else
        {
            local_194.x = 496.0f;
            local_194.y = 144.0f;
            local_194.z = 0.0f;
            AsciiManager::AddFormatText(&g_AsciiManager, &local_194, "MAX");
        }
    }
    if ((this->flags & 3) != 0)
    {
        this->flags = (this->flags & 0xfffffffc) | ((this->flags & 3) - 1 & 3);
    }
    if ((this->flags >> 4 & 3) != 0)
    {
        this->flags = (this->flags & 0xffffffcf) | ((this->flags >> 4 & 3) - 1 & 3)
                                                       << 4;
    }
    if ((this->flags >> 2 & 3) != 0)
    {
        this->flags = (this->flags & 0xfffffff3) | ((this->flags >> 2 & 3) - 1 & 3)
                                                       << 2;
    }
    if ((this->flags >> 6 & 3) != 0)
    {
        this->flags = (this->flags & 0xffffff3f) | ((this->flags >> 6 & 3) - 1 & 3)
                                                       << 6;
    }
}

// FUNCTION: TH07 0x0042c577
void Gui::DrawStageElements()
{
    f32 fVar1;
    f32 fVar2;
    f32 fVar3;
    bool bVar6;
    i32 iVar8;
    i32 local_94;
    D3DXVECTOR3 local_60;
    i32 local_54;
    D3DCOLOR local_50;
    f32 local_4c;
    i32 local_48;
    i32 local_44;
    ZunRect local_40;
    i32 local_24;
    u32 local_20;
    i32 local_18;
    i32 i;

    for (i = 0; i < 5; i += 1)
    {
        g_AnmManager->Draw(this->impl->vms1 + i);
    }
    if ((this->impl->bombSpellcardPortrait.flags & 1) != 0)
    {
        g_AnmManager->DrawNoRotation(&this->impl->bombSpellcardPortrait);
        g_AnmManager->DrawNoRotation(&this->impl->bombSpellcardDecorLeft);
        g_AnmManager->Draw(&this->impl->bombSpellcardDecorRight);
    }
    if ((this->impl->enemySpellcardPortrait.flags & 1) != 0)
    {
        fVar1 = this->impl->enemySpellcardPortrait.pos.x;
        fVar2 = this->impl->enemySpellcardPortrait.pos.y;
        fVar3 = this->impl->enemySpellcardPortrait.pos.z;
        this->impl->enemySpellcardPortrait.pos +=
            this->impl->enemySpellcardPortrait.offset;
        g_AnmManager->DrawNoRotation(&this->impl->enemySpellcardPortrait);
        this->impl->enemySpellcardPortrait.pos.x = fVar1;
        this->impl->enemySpellcardPortrait.pos.y = fVar2;
        this->impl->enemySpellcardPortrait.pos.z = fVar3;
        g_AnmManager->DrawNoRotation(&this->impl->enemySpellcardRelated1);
        g_AnmManager->Draw(&this->impl->enemySpellcardRelated2);
    }
    if ((this->impl->bombSpellcardName.flags & 1) != 0)
    {
        this->impl->bombSpellcardNameBg.pos = this->impl->bombSpellcardName.pos;
        g_AnmManager->DrawNoRotation(&this->impl->bombSpellcardNameBg);
        g_AnmManager->Draw(&this->impl->bombSpellcardName);
    }
    if ((this->impl->enemySpellcardName.flags & 1) != 0)
    {
        this->impl->enemySpellcardNameBg.pos = this->impl->enemySpellcardName.pos;
        g_AnmManager->DrawNoRotation(&this->impl->enemySpellcardNameBg);
        g_AnmManager->Draw(&this->impl->enemySpellcardName);
        g_AnmManager->DrawNoRotation(&this->impl->spellcardBonusIndicator);
        local_18 = g_EnemyManager.spellcardInfo.captureScore +
                   g_EnemyManager.spellcardInfo.grazeBonusScore;
        local_24 = 10000000;
        bVar6 = false;
        iVar8 = g_EnemyManager.spellcardInfo.spellcardIdx * 0x78;
        if (g_EnemyManager.spellcardInfo.isCapturing == 0)
        {
            local_18 = 0;
        }
        (this->impl->captureBonusVm).pos = this->impl->spellcardBonusIndicator.pos;
        (this->impl->captureBonusVm).pos.x -= 40.0f;
        for (i = 0; i < 8; i += 1)
        {
            if (local_18 / local_24 != 0)
            {
                bVar6 = true;
            }
            if ((bVar6) || (local_24 == 1))
            {
                (this->impl->captureBonusVm).sprite =
                    g_AnmManager->sprites + local_18 / local_24 + 0x84;
                g_AnmManager->DrawNoRotation(&this->impl->captureBonusVm);
            }
            (this->impl->captureBonusVm).pos.x =
                (this->impl->captureBonusVm).pos.x + 7.0f;
            local_18 %= local_24;
            local_24 /= 10;
        }
        local_20 = g_GameManager.catk[iVar8]
                       .numSuccessesPerShot[g_GameManager.shotTypeAndCharacter];
        if (99 < local_20)
        {
            local_20 = 99;
        }
        (this->impl->captureBonusVm).pos.x =
            (this->impl->captureBonusVm).pos.x + 36.0f;
        if (local_20 / 10 != 0)
        {
            (this->impl->captureBonusVm).sprite =
                g_AnmManager->sprites + local_20 / 10 + 0x84;
            g_AnmManager->DrawNoRotation(&this->impl->captureBonusVm);
        }
        (this->impl->captureBonusVm).pos.x += 7.0f;
        (this->impl->captureBonusVm).sprite =
            g_AnmManager->sprites + local_20 % 10 + 0x84;
        g_AnmManager->DrawNoRotation(&this->impl->captureBonusVm);
        local_20 = g_GameManager.catk[iVar8]
                       .numAttemptsPerShot[g_GameManager.shotTypeAndCharacter];
        if (99 < local_20)
        {
            local_20 = 99;
        }
        (this->impl->captureBonusVm).pos.x += 14.0f;
        if (local_20 / 10 != 0)
        {
            (this->impl->captureBonusVm).sprite =
                g_AnmManager->sprites + local_20 / 10 + 0x84;
            g_AnmManager->DrawNoRotation(&this->impl->captureBonusVm);
        }
        (this->impl->captureBonusVm).pos.x += 7.0f;
        (this->impl->captureBonusVm).sprite =
            g_AnmManager->sprites + local_20 % 10 + 0x84;
        g_AnmManager->DrawNoRotation(&this->impl->captureBonusVm);
    }
    if (-1 < this->impl->stageClearTextVm.activeSpriteIdx)
    {
        g_AnmManager->DrawNoRotation(&this->impl->stageClearTextVm);
        g_AnmManager->DrawNoRotation(&this->impl->stageTransitionSnapshotVm);
        if (-1 < this->impl->stageClearBonusTextVm.activeSpriteIdx)
        {
            this->impl->stageClearBonusTextVm.pos.x = 304.0f;
            this->impl->stageClearBonusTextVm.pos.y = 448.0f;
            this->impl->stageClearBonusTextVm.pos.z = 0.0f;
            g_AnmManager->DrawNoRotation(&this->impl->stageClearBonusTextVm);
        }
    }
    if (this->impl->activeTransitionQuads != 0)
    {
        for (i = 0; i < 0xa8; i += 1)
        {
            g_AnmManager->DrawProjected(this->impl->transitionQuads + i);
            g_AnmManager->currentSprite = NULL;
        }
    }
    if ((this->impl->msg.currentMsgIdx < 0) &&
        ((u32)this->bossPresent + (u32)this->impl->bossHealthBarState != 0))
    {
        local_40.left = 64.0f;
        local_40.top = 19.0f;
        local_40.right = this->bossHealthBarEased * 320.0f + 64.0f;
        local_40.bottom = 23.0f;
        local_60.x = 48.0f;
        local_60.y = 16.0f;
        local_60.z = 0.0f;
        ScreenEffect::DrawColoredQuad(&local_40,
                                      this->bossHealthBarAlpha << 0x18 | 0xffffff,
                                      this->bossHealthBarAlpha << 0x18 | 0xffffff,
                                      this->bossHealthBarAlpha << 0x18 | 0x202060,
                                      this->bossHealthBarAlpha << 0x18 | 0x202060);
        for (local_48 = 0; local_48 < 8; local_48 += 1)
        {
            if ((this->bossHealth[local_48] != 0.0f) &&
                (this->bossHealthEased[local_48] < this->bossHealthBarEased))
            {
                local_4c = this->bossHealth[local_48];
                if (this->bossHealthBarEased < local_4c)
                {
                    local_4c = this->bossHealthBarEased;
                }
                local_40.left = this->bossHealthEased[local_48] * 320.0f + 64.0f;
                local_40.top = 19.0f;
                local_40.right = local_4c * 320.0f + 64.0f;
                local_40.bottom = 23.0f;
                ScreenEffect::DrawColoredQuad(
                    &local_40,
                    this->bossHealthBarAlpha << 0x18 |
                        (this->bossColor[local_48] & 0xffffff),
                    this->bossHealthBarAlpha << 0x18 |
                        (this->bossColor[local_48] & 0xffffff),
                    this->bossHealthBarAlpha << 0x18 |
                        ((i32)this->bossColor[local_48] >> 2 & 0x3f3f3fU),
                    this->bossHealthBarAlpha << 0x18 |
                        ((i32)this->bossColor[local_48] >> 2 & 0x3f3f3fU));
            }
        }
        g_AnmManager->DrawNoRotation(this->impl->vms0 + 0xb);
        local_40.left = 33.0f;
        local_40.top = 19.0f;
        local_40.right = 36.0f;
        local_40.bottom = 23.0f;
        local_44 = this->bossLifeMarkers;
        local_54 = (this->bossLifeMarkers < 6) + 1;
        for (local_48 = 0; local_48 < local_44; local_48 += 1)
        {
            local_40.left = ((f32)local_48 * 26.0f) / (f32)local_44 + 35.0f;
            local_40.right = (((f32)(local_48 + 1) * 26.0f) / (f32)local_44 + 35.0f) -
                             (f32)local_54;
            ScreenEffect::DrawColoredQuad(
                &local_40,
                this->bossHealthBarAlpha << 0x18 | 0xffffffU - (local_48 * 0xff) / 9,
                this->bossHealthBarAlpha << 0x18 | 0xffffffU - (local_48 * 0xff) / 9,
                this->bossHealthBarAlpha << 0x18 | 0x202020,
                this->bossHealthBarAlpha << 0x18 | 0x202020);
        }
        local_60.x = 384.0f;
        local_60.y = 16.0f;
        local_60.z = 0.0f;
        if (this->spellcardSecondsRemaining < 20)
        {
            if (this->spellcardSecondsRemaining < 10)
            {
                if (this->spellcardSecondsRemaining < 5)
                {
                    local_50 = g_SpellcardTimeColors[3];
                }
                else
                {
                    local_50 = g_SpellcardTimeColors[2];
                }
            }
            else
            {
                local_50 = g_SpellcardTimeColors[1];
            }
        }
        else
        {
            local_50 = g_SpellcardTimeColors[0];
        }
        g_AsciiManager.color = this->bossHealthBarAlpha << 0x18 | local_50;
        if (this->spellcardSecondsRemaining < 100)
        {
            local_94 = this->spellcardSecondsRemaining;
        }
        else
        {
            local_94 = 99;
        }
        local_44 = local_94;
        if ((local_94 < 10) && (this->lastSpellcardSecondsRemaining !=
                                this->spellcardSecondsRemaining))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_29, 0);
        }
        AsciiManager::AddFormatText(&g_AsciiManager, &local_60, "%.2d", local_44);
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
    bool bVar1;

    g_AnmManager->ReleaseAnm(0x18);
    g_AnmManager->ReleaseAnm(0x1c);
    g_AnmManager->ReleaseAnm(0x1d);
    g_AnmManager->ReleaseAnm(0x1e);
    g_AnmManager->ReleaseAnm(0x1f);
    arg->FreeMsgFile();
    if (((g_Supervisor.curState == 3) || (g_Supervisor.curState == 0xb)) ||
        (g_Supervisor.curState == 0xc))
    {
        bVar1 = false;
    }
    else
    {
        bVar1 = true;
    }
    if (bVar1)
    {
        g_AnmManager->ReleaseAnm(0x15);
        g_AnmManager->ReleaseAnm(0x17);
        g_AnmManager->ReleaseAnm(0x19);
        g_AnmManager->ReleaseAnm(0x1a);
        g_AnmManager->ReleaseAnm(0x1b);
        g_AnmManager->ReleaseAnm(0x16);
        free(arg->impl);
        arg->impl = NULL;
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0042d136
ZunResult Gui::RegisterChain()
{
    Gui *mgr = &g_Gui;

    if ((u32)(g_Supervisor.curState != 3 && g_Supervisor.curState != 0xb &&
              g_Supervisor.curState != 0xc) != 0)
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
    if (g_Chain.AddToCalcChain(&g_GuiCalcChain, 0xd) != 0)
        return ZUN_ERROR;

    g_GuiDrawChain.callback = (ChainCallback)OnDraw;
    g_GuiDrawChain.addedCallback = NULL;
    g_GuiDrawChain.deletedCallback = NULL;
    g_GuiDrawChain.arg = mgr;
    g_Chain.AddToDrawChain(&g_GuiDrawChain, 0xc);
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

#pragma optimize("s", off)
