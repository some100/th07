#pragma once

#include "AnmVm.hpp"
#include "ZunResult.hpp"

#define TRANSITION_QUAD_ROWS 14
#define TRANSITION_QUAD_COLS 12

enum GuiDisplayArg
{
    GUI_DISPLAY_HIDDEN = 0,
    GUI_DISPLAY_SHOWN = 1,
    GUI_DISPLAY_FULL_POWER = 1,
    GUI_DISPLAY_BORDER = 2,
    GUI_DISPLAY_CHERRY_MAX = 3,
    GUI_DISPLAY_BORDER_BONUS = 4,
};

// values from https://pytouhou.linkmauve.fr/doc/06/msg.xml
enum MsgOpcode
{
    MSG_DELETE,
    MSG_SHOW_PORTRAIT,
    MSG_CHANGE_FACE,
    MSG_DIALOGUE,
    MSG_PAUSE,
    MSG_SWITCH,
    MSG_APPEAR_ENEMY,
    MSG_MUSIC,
    MSG_TEXT_INTRODUCE,
    MSG_STAGERESULTS,
    MSG_FREEZE,
    MSG_NEXT_LEVEL,
    MSG_FADEOUT_MUSIC,
    MSG_ALLOW_SKIP,
    MSG_FADE_IN_EFFECT,
};

struct MsgRawInstrArgPortrait
{
    i16 portraitIdx;
    i16 anmScriptIdx;
};

struct MsgRawInstrArgDialogue
{
    i16 textColor;
    i16 textLine;
    char text[5];
};

struct MsgRawInstrArgPause
{
    i32 duration;
};

struct MsgRawInstrArgSwitch
{
    i16 unkIdx;
    u8 interrupt;
};

struct MsgRawInstrArgMusic
{
    i32 musicIdx;
};

union MsgRawInstrArgs {
    MsgRawInstrArgPortrait portrait;
    MsgRawInstrArgDialogue dialogue;
    MsgRawInstrArgPause pause;
    MsgRawInstrArgSwitch msgSwitch;
    MsgRawInstrArgMusic music;
};

struct MsgRawInstr
{
    u16 time;
    u8 opcode;
    u8 argsize;
    MsgRawInstrArgs args;
};
static_assert(sizeof(MsgRawInstr) == 0x10);

struct MsgRawHeader
{
    i32 numInstrs;
    u32 offsets[];
};
static_assert(sizeof(MsgRawHeader) == 0x4);

struct GuiFormattedText
{
    ZunVec3 pos;
    ZunVec3 prevPos;
    i32 fmtArg;
    i32 displayArg;
    ZunTimer timer;
};

struct GuiMsgVm
{
    GuiMsgVm();

    void UpdatePrev()
    {
        portraits[0].UpdatePrev();
        portraits[1].UpdatePrev();
        dialogueLines[0].UpdatePrev();
        dialogueLines[1].UpdatePrev();
        introLines[0].UpdatePrev();
        introLines[1].UpdatePrev();
    }

    MsgRawHeader *msgFile;
    MsgRawInstr *curInstr;
    i32 currentMsgIdx;
    ZunTimer timer;
    i32 framesElapsedDuringPause;
    AnmVm portraits[2];
    AnmVm dialogueLines[2];
    AnmVm introLines[2];
    u32 textColorsA[4];
    u32 textColorsB[4];
    u32 fontSize;
    u32 ignoreWaitCounter;
    u8 dialogueSkippable;
};

struct GuiImpl
{
    GuiImpl();

    ZunResult DrawDialogue();
    void MsgRead(i32 msgIdx);
    ZunResult RunMsg();

    void UpdatePrev()
    {
        for (int i = 0; i < 33; i++)
        {
            vms0[i].UpdatePrev();
        }
        for (int i = 0; i < 5; i++)
        {
            stageTextVm[i].UpdatePrev();
        }
        bombSpellcardPortrait.UpdatePrev();
        enemySpellcardPortrait.UpdatePrev();
        bombSpellcardDecorLeft.UpdatePrev();
        enemySpellcardDecorHorizontalUp.UpdatePrev();
        bombSpellcardDecorRight.UpdatePrev();
        enemySpellcardDecorHorizontalDown.UpdatePrev();
        bombSpellcardName.UpdatePrev();
        enemySpellcardName.UpdatePrev();
        bombSpellcardNameBg.UpdatePrev();
        enemySpellcardNameBg.UpdatePrev();
        stageClearBg.UpdatePrev();
        loadingSprite.UpdatePrev();
        stageTransitionSnapshotVm.UpdatePrev();
        captureBonusVm.UpdatePrev();
        spellcardBonusIndicator.UpdatePrev();
        for (i32 i = 0; i < 168; i++)
        {
            transitionQuads[i].UpdatePrev();
        }
        msg.UpdatePrev();
    }

    AnmVm vms0[33];
    u8 bossHealthBarState;
    // pad 3
    AnmVm stageTextVm[5];
    AnmVm bombSpellcardPortrait;
    AnmVm enemySpellcardPortrait;
    AnmVm bombSpellcardDecorLeft;
    AnmVm enemySpellcardDecorHorizontalUp;
    AnmVm bombSpellcardDecorRight;
    AnmVm enemySpellcardDecorHorizontalDown;
    AnmVm bombSpellcardName;
    AnmVm enemySpellcardName;
    AnmVm bombSpellcardNameBg;
    AnmVm enemySpellcardNameBg;
    AnmVm stageClearBg;
    AnmVm loadingSprite;
    AnmVm stageTransitionSnapshotVm;
    AnmVm captureBonusVm;
    AnmVm spellcardBonusIndicator;
    AnmVm transitionQuads[TRANSITION_QUAD_ROWS * TRANSITION_QUAD_COLS];
    i32 activeTransitionQuads;
    GuiMsgVm msg;
    // pad 3
    i32 finishedStage;
    i32 stageClearBonus;
    i32 transitionToScoreScreen;
    GuiFormattedText bonusScore;
    GuiFormattedText statusPopup;
    GuiFormattedText spellCardBonus;
    i32 clearPower;
    i32 clearPointItems;
    i32 clearCherryMax;
    i32 clearGraze;
};

struct Gui
{
    static ZunResult RegisterChain();
    static void CutChain();

    static ZunResult AddedCallback(Gui *arg);
    static ZunResult DeletedCallback(Gui *arg);
    static u32 OnUpdate(Gui *arg);
    static u32 OnDraw(Gui *arg);

    ZunResult ActualAddedCallback();
    void ClearActiveSprites();
    static void CopyEnemyNameTexture(i32 spriteIdx);
    void DrawGameScene();
    void DrawStageElements();
    void FreeMsgFile();
    i32 HasCurrentMsgIdx();
    i32 IsDialogueSkippable();
    i32 IsStageFinished();
    ZunResult LoadMsg(const char *filename);
    void MsgRead(i32 msgIdx);
    i32 MsgWait();

    void EndEnemySpellcard();
    void EndPlayerSpellcard();
    void ShowBombNamePortrait(i32 sprite, const char *name);
    void ShowBonusScore(i32 score);
    void ShowStatusPopup(i32 fmtArg, i32 popupType);
    void ShowSpellcard(i32 spellcardSprite, const char *spellcardName);
    void ShowSpellcardBonus(i32 fmtArg);
    void UpdateGui();

    void SetSpellcardSecondsRemaining(i32 seconds)
    {
        this->spellcardSecondsRemaining = seconds;
    }

    void SetBossHealth(i32 idx, f32 eased, f32 health)
    {
        this->bossHealthEased[idx] = eased;
        this->bossHealth[idx] = health;
    }

    void SetBossHealthBar(f32 amount)
    {
        this->bossHealthBar = amount;
    }

    bool BossPresent()
    {
        return this->bossPresent;
    }

    void UpdatePrev()
    {
        this->impl->UpdatePrev();
        this->prevBossHealthBarAlpha = this->bossHealthBarAlpha;
        this->prevBossHealthBarEased = this->bossHealthBarEased;
        for (i32 i = 0; i < 8; i++)
        {
            this->prevBossHealthEased[i] = this->bossHealthEased[i];
        }
    }

    i32 frameCounter;
    union {
        u32 flags;
        struct
        {
            u32 lifeDisplayUpdateFrames : 2;
            u32 bombDisplayUpdateFrames : 2;
            u32 powerDisplayUpdateFrames : 2;
            u32 grazeDisplayUpdateFrames : 2;
            u32 pointDisplayUpdateFrames : 2;
        };
    };
    GuiImpl *impl;
    f32 bombNameBarLength;
    f32 spellcardBarLength;
    u32 bossHealthBarAlpha;
    u32 prevBossHealthBarAlpha;
    i32 bossLifeMarkers;
    i32 spellcardSecondsRemaining;
    i32 lastSpellcardSecondsRemaining;
    bool bossPresent;
    // pad 3
    f32 bossHealthBar;
    f32 bossHealthBarEased;
    f32 prevBossHealthBarEased;
    i32 unused_30;
    f32 bossHealth[8];
    f32 bossHealthEased[8];
    f32 prevBossHealthEased[8];
    u32 bossColor[8];
};

extern Gui g_Gui;
