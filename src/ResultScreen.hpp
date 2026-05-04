#pragma once

#include <windows.h>

#include "AnmVm.hpp"
#include "Chain.hpp"
#include "ReplayManager.hpp"
#include "ZunResult.hpp"
#include "inttypes.hpp"

#define TH7K_MAGIC 'K7HT'
#define CATK_MAGIC 'KTAC'
#define HSCR_MAGIC 'RCSH'
#define CLRD_MAGIC 'DRLC'
#define PSCR_MAGIC 'RCSP'
#define PLST_MAGIC 'TSLP'
#define LSNM_MAGIC 'MNSL'
#define VRSM_MAGIC 'MSRV'

#pragma optimize("s", on)

struct Th7k
{
    u32 magic;
    u16 th7kLen;
    u16 th7kLen2;
    u8 version;
    u8 isPlayerScore;
    // pad 2
};
C_ASSERT(sizeof(Th7k) == 0xc);

struct Catk : Th7k
{
    u32 highScorePerShot[7];
    i16 idx;
    char nameCsum;
    char name[49];
    u16 numAttemptsPerShot[7];
    u16 numSuccessesPerShot[7];
};
C_ASSERT(sizeof(Catk) == 0x78);

struct Hscr : Th7k
{
    u32 score;
    u32 slowRatePercent;
    u8 character;
    u8 difficulty;
    u8 stage;
    char name[9];
    char date[6];
    i8 numRetries;
    // pad 1
};
C_ASSERT(sizeof(Hscr) == 0x28);

struct Clrd : Th7k
{
    u8 difficultyClearedWithRetries[6];
    u8 difficultyClearedWithoutRetries[6];
    u8 characterShotType;
    // pad 3
};
C_ASSERT(sizeof(Clrd) == 0x1c);

struct Pscr : Th7k
{
    i32 playCount;
    i32 score;
    u8 character;
    u8 difficulty;
    u8 stage;
    // pad 1
};
C_ASSERT(sizeof(Pscr) == 0x18);

struct PlstPlayCounts
{
    u32 playCount;
    u32 playCountPerShotType[6];
    u32 clearCount;
    u32 noContinueClearCount;
    u32 retryCount;
    u32 extraClearCount;
};

struct Plst : Th7k
{
    u32 totalHours;
    u32 totalMinutes;
    u32 totalSeconds;
    u32 totalMilliseconds;
    u32 gameHours;
    u32 gameMinutes;
    u32 gameSeconds;
    u32 gameMilliseconds;
    PlstPlayCounts playDataByDifficulty[6];
    PlstPlayCounts playDataTotals;
};

struct Lsnm : Th7k
{
    char name[12];
};
C_ASSERT(sizeof(Lsnm) == 0x18);

struct Vrsm : Th7k
{
    char versionStr[6];
    i32 exeSize;
    i32 exeChecksum;
};
C_ASSERT(sizeof(Vrsm) == 0x1c);

struct ScoreListNode
{
    ScoreListNode *prev;
    ScoreListNode *next;
    Hscr *data;
};
C_ASSERT(sizeof(ScoreListNode) == 0xc);

struct ScoreDat
{
    u8 xorseed[2];
    u16 csum;
    i16 magic;
    u8 unused_6;
    // pad 1
    i32 dataOffset;
    ScoreListNode *scores;
    i32 fileLength;
    SIZE_T dstLen;
    i32 srcLen;
};
C_ASSERT(sizeof(ScoreDat) == 0x1c);

struct ResultScreen
{
    ResultScreen()
    {
        memset(this, 0, sizeof(ResultScreen));
        this->cursor = 1;
    }

    static ZunResult RegisterChain(u32 param_1);

    static ZunResult AddedCallback(ResultScreen *arg);
    static ZunResult DeletedCallback(ResultScreen *arg);
    static u32 OnUpdate(ResultScreen *arg);
    static u32 OnDraw(ResultScreen *arg);

    ZunResult CheckConfirmButton();
    ZunResult DrawFinalStats();
    i32 DrawStats();
    static void GetDate(char *out);
    ZunResult HandleReplaySaveKeyboard();
    ZunResult HandleResultKeyboard();
    static i32 MoveCursor(ResultScreen *screen, i32 max);
    static i32 MoveCursor2(ResultScreen *screen, i32 max);
    static i32 MoveCursorHorizontally(ResultScreen *screen, i32 max);

    static ScoreDat *OpenScore(const char *path);
    static i32 LinkScore(ScoreListNode *prevNode, Hscr *hscr);
    i32 LinkScoreEx(Hscr *out, i32 difficulty, i32 character);
    static u32 GetHighScore(ScoreDat *scoreDat, ScoreListNode *node,
                            u32 character, u32 difficulty, u8 *noClue);
    static ZunResult ParseCatk(ScoreDat *scoreDat, Catk *catk);
    static ZunResult ParseClrd(ScoreDat *scoreDat, Clrd *clrd);
    static ZunResult ParsePlst(ScoreDat *scoreDat, Plst *plst);
    static ZunResult ParsePscr(ScoreDat *scoreDat, Pscr *pscr);
    static ZunResult ParseScores();
    static void ReleaseScoreDat(ScoreDat *scoreDat);
    void FreeScore(i32 param_1, i32 param_2);
    static void FreeAllScores(ScoreListNode *scores);
    static ZunResult ParseLsnm(ScoreDat *scoreDat, Lsnm *param_2);
    void WriteScore();

    ScoreDat *scoreDat;
    i32 frameTimer;
    i32 resultScreenState;
    i32 stateStep;
    i32 cursor;
    i32 prevCursor;
    i32 savedCursor;
    i32 chosenReplayIdx;
    i32 selectedChar;
    i32 spellcardListPage;
    i32 prevSpellcardListPage;
    i32 listScrollAnimState;
    i32 charUsed;
    i32 lastSpellcardSelected;
    i32 diffPlayed;
    i32 cheatCodeStep;
    i32 isClearingReplayName;
    char replayName[8];
    i32 totalPlayCountPerCharacter[7];
    i32 savedPlaytimeCharacter;
    u8 lastTotalSeconds;
    // pad 3
    AnmVm vms[41];
    AnmVm spellcardListVms[15];
    AnmVm leftArrowVm;
    AnmVm rightArrowVm;
    ScoreListNode scoreLists[6][6];
    Hscr defaultScores[6][6][10];
    Hscr curScore;
    Th7k th7kHeader;
    Lsnm lsnmHeader;
    ChainElem *calcChain;
    ChainElem *drawChain;
    ReplayHeaderAndData replays[15];
    ReplayHeaderAndData defaultReplay;
};
C_ASSERT(sizeof(ResultScreen) == 0xce6c);

#pragma optimize("s", off)
