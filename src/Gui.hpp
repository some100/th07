#pragma once

#include "AnmVm.hpp"
#include "ZunResult.hpp"

// values from https://pytouhou.linkmauve.fr/doc/06/msg.xml
typedef enum MsgOpcode
{
    MSG_DELETE = 0,
    MSG_SHOW_PORTRAIT = 1,
    MSG_CHANGE_FACE = 2,
    MSG_DIALOGUE = 3,
    MSG_PAUSE = 4,
    MSG_SWITCH = 5,
    MSG_APPEAR_ENEMY = 6,
    MSG_MUSIC = 7,
    MSG_TEXT_INTRODUCE = 8,
    MSG_STAGERESULTS = 9,
    MSG_FREEZE = 10,
    MSG_NEXT_LEVEL = 11,
    MSG_FADEOUT_MUSIC = 12,
    MSG_ALLOW_SKIP = 13,
    MSG_FADE_IN_EFFECT = 14
} MsgOpcode;

struct MsgRawInstrArgPortrait
{
    u32 portrait;
    u32 anmScriptIdx;
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
    i16 portraitIdx;
    u8 interrupt;
};

union MsgRawInstrArgs {
    MsgRawInstrArgPortrait portrait;
    MsgRawInstrArgDialogue dialogue;
    MsgRawInstrArgPause pause;
    MsgRawInstrArgSwitch msgSwitch;
};

struct MsgRawInstr
{
    u16 time;
    u8 opcode;
    u8 argsize;
    MsgRawInstrArgs args;
};

struct MsgRawHeader
{
    i32 numEntries;
    MsgRawInstr *entries;
};

struct GuiImplChildB
{
    D3DXVECTOR3 pos;
    i32 fmtArg;
    i32 isShown;
    ZunTimer timer;
};

struct GuiMsgVm
{
    GuiMsgVm();

    MsgRawHeader *msgFile;
    MsgRawInstr *curInstr;
    i32 currentMsgIdx;
    ZunTimer timer;
    i32 framesElapsedDuringPause;
    AnmVm portraits[2];
    AnmVm dialogueLines[2];
    AnmVm introLines[2];
    ZunColor textColorsA[4];
    ZunColor textColorsB[4];
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

    AnmVm vms0[33];
    u8 bossHealthBarState;
    // pad 3
    AnmVm vms1[5];
    AnmVm bombSpellcardPortrait;
    AnmVm enemySpellcardPortrait;
    AnmVm bombSpellcardDecorLeft;
    AnmVm enemySpellcardRelated1;
    AnmVm bombSpellcardDecorRight;
    AnmVm enemySpellcardRelated2;
    AnmVm bombSpellcardName;
    AnmVm enemySpellcardName;
    AnmVm bombSpellcardNameBg;
    AnmVm enemySpellcardNameBg;
    AnmVm stageClearTextVm;
    AnmVm stageClearBonusTextVm;
    AnmVm stageTransitionSnapshotVm;
    AnmVm captureBonusVm;
    AnmVm spellcardBonusIndicator;
    AnmVm transitionQuads[168];
    i32 activeTransitionQuads;
    GuiMsgVm msg;
    // pad 3
    i32 finishedStage;
    i32 stageClearBonus;
    i32 transitionToScoreScreen;
    GuiImplChildB bonusScore;
    GuiImplChildB fullPowerMode;
    GuiImplChildB spellCardBonus;
    i32 clearPower;
    i32 clearPointItems;
    i32 clearCherryMax;
    i32 clearGraze;
};
C_ASSERT(sizeof(GuiImpl) == 0x20a30);

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
    static void CopyTemplateSpriteToSprite(i32 spriteIdx);
    void DrawGameScene();
    void DrawStageElements();
    void FreeMsgFile();
    i32 HasCurrentMsgIdx();
    i32 IsDialogueSkippable();
    i32 IsStageFinished();
    ZunResult LoadMsg(const char *param_1);
    void MsgRead(i32 param_1);
    i32 MsgWait();

    void EndEnemySpellcard();
    void EndPlayerSpellcard();
    void ShowBombNamePortrait(i32 sprite, const char *name);
    void ShowBonusScore(i32 score);
    void ShowFullPowerMode(i32 fmtArg, i32 isShown);
    void ShowSpellcard(i32 spellcardSprite, const char *spellcardName);
    void ShowSpellcardBonus(i32 fmtArg);
    void UpdateGui();

    i32 frameCounter;
    u32 flags;
    struct GuiImpl *impl;
    f32 bombNameBarLength;
    f32 spellcardBarLength;
    u32 bossHealthBarAlpha;
    i32 bossLifeMarkers;
    i32 spellcardSecondsRemaining;
    i32 lastSpellcardSecondsRemaining;
    u8 bossPresent;
    // pad 3
    f32 bossHealthBar;
    f32 bossHealthBarEased;
    i32 unused_30;
    f32 bossHealth[8];
    f32 bossHealthEased[8];
    u32 bossColor[8];
};
C_ASSERT(sizeof(Gui) == 0x94);

extern Gui g_Gui;
