#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>

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

typedef enum Character
{
    CHAR_REIMU = 0,
    CHAR_MARISA = 1,
    CHAR_SAKUYA = 2
} Character;

typedef enum ShotType
{
    SHOT_REIMU_A = 0,
    SHOT_REIMU_B = 1,
    SHOT_MARISA_A = 2,
    SHOT_MARISA_B = 3,
    SHOT_SAKUYA_A = 4,
    SHOT_SAKUYA_B = 5,
    SHOT_COUNT = 6,
} ShotType;

typedef enum SpellcardNumber
{
    SPELLCARD_ST1_MBOSS_1H,
    SPELLCARD_ST1_MBOSS_1L,
    SPELLCARD_ST1_BOSS_1E,
    SPELLCARD_ST1_BOSS_1N,
    SPELLCARD_ST1_BOSS_1H,
    SPELLCARD_ST1_BOSS_1L,
    SPELLCARD_ST1_BOSS_2E,
    SPELLCARD_ST1_BOSS_2N,
    SPELLCARD_ST1_BOSS_2H,
    SPELLCARD_ST1_BOSS_2L,

    SPELLCARD_ST2_MBOSS_1E,
    SPELLCARD_ST2_MBOSS_1N,
    SPELLCARD_ST2_MBOSS_1H,
    SPELLCARD_ST2_MBOSS_1L,
    SPELLCARD_ST2_BOSS_1E,
    SPELLCARD_ST2_BOSS_1N,
    SPELLCARD_ST2_BOSS_1H,
    SPELLCARD_ST2_BOSS_1L,
    SPELLCARD_ST2_BOSS_2E,
    SPELLCARD_ST2_BOSS_2N,
    SPELLCARD_ST2_BOSS_2H,
    SPELLCARD_ST2_BOSS_2L,
    SPELLCARD_ST2_BOSS_3E,
    SPELLCARD_ST2_BOSS_3N,
    SPELLCARD_ST2_BOSS_3H,
    SPELLCARD_ST2_BOSS_3L,

    SPELLCARD_ST3_MBOSS_1H,
    SPELLCARD_ST3_MBOSS_1L,
    SPELLCARD_ST3_BOSS_1E,
    SPELLCARD_ST3_BOSS_1N,
    SPELLCARD_ST3_BOSS_1H,
    SPELLCARD_ST3_BOSS_1L,
    SPELLCARD_ST3_BOSS_2E,
    SPELLCARD_ST3_BOSS_2N,
    SPELLCARD_ST3_BOSS_2H,
    SPELLCARD_ST3_BOSS_2L,
    SPELLCARD_ST3_BOSS_3E,
    SPELLCARD_ST3_BOSS_3N,
    SPELLCARD_ST3_BOSS_3H,
    SPELLCARD_ST3_BOSS_3L,
    SPELLCARD_ST3_BOSS_4E,
    SPELLCARD_ST3_BOSS_4N,
    SPELLCARD_ST3_BOSS_4H,
    SPELLCARD_ST3_BOSS_4L,

    SPELLCARD_ST4_BOSS_1E,
    SPELLCARD_ST4_BOSS_1N,
    SPELLCARD_ST4_BOSS_1H,
    SPELLCARD_ST4_BOSS_1L,
    SPELLCARD_ST4_LUNASA_2E,
    SPELLCARD_ST4_LUNASA_2N,
    SPELLCARD_ST4_LUNASA_2H,
    SPELLCARD_ST4_LUNASA_2L,
    SPELLCARD_ST4_MERLIN_2E,
    SPELLCARD_ST4_MERLIN_2N,
    SPELLCARD_ST4_MERLIN_2H,
    SPELLCARD_ST4_MERLIN_2L,
    SPELLCARD_ST4_LYRICA_2E,
    SPELLCARD_ST4_LYRICA_2N,
    SPELLCARD_ST4_LYRICA_2H,
    SPELLCARD_ST4_LYRICA_2L,
    SPELLCARD_ST4_BOSS_3E,
    SPELLCARD_ST4_BOSS_3N,
    SPELLCARD_ST4_BOSS_3H,
    SPELLCARD_ST4_BOSS_3L,
    SPELLCARD_ST4_BOSS_4E,
    SPELLCARD_ST4_BOSS_4N,
    SPELLCARD_ST4_BOSS_4H,
    SPELLCARD_ST4_BOSS_4L,

    SPELLCARD_ST5_MBOSS_1E,
    SPELLCARD_ST5_MBOSS_1N,
    SPELLCARD_ST5_MBOSS_1H,
    SPELLCARD_ST5_MBOSS_1L,
    SPELLCARD_ST5_BOSS_1E,
    SPELLCARD_ST5_BOSS_1N,
    SPELLCARD_ST5_BOSS_1H,
    SPELLCARD_ST5_BOSS_1L,
    SPELLCARD_ST5_BOSS_2E,
    SPELLCARD_ST5_BOSS_2N,
    SPELLCARD_ST5_BOSS_2H,
    SPELLCARD_ST5_BOSS_2L,
    SPELLCARD_ST5_BOSS_3E,
    SPELLCARD_ST5_BOSS_3N,
    SPELLCARD_ST5_BOSS_3H,
    SPELLCARD_ST5_BOSS_3L,
    SPELLCARD_ST5_BOSS_4E,
    SPELLCARD_ST5_BOSS_4N,
    SPELLCARD_ST5_BOSS_4H,
    SPELLCARD_ST5_BOSS_4L,

    SPELLCARD_ST6_MBOSS_1E,
    SPELLCARD_ST6_MBOSS_1N,
    SPELLCARD_ST6_MBOSS_1H,
    SPELLCARD_ST6_MBOSS_1L,
    SPELLCARD_ST6_BOSS_1E,
    SPELLCARD_ST6_BOSS_1N,
    SPELLCARD_ST6_BOSS_1H,
    SPELLCARD_ST6_BOSS_1L,
    SPELLCARD_ST6_BOSS_2E,
    SPELLCARD_ST6_BOSS_2N,
    SPELLCARD_ST6_BOSS_2H,
    SPELLCARD_ST6_BOSS_2L,
    SPELLCARD_ST6_BOSS_3E,
    SPELLCARD_ST6_BOSS_3N,
    SPELLCARD_ST6_BOSS_3H,
    SPELLCARD_ST6_BOSS_3L,
    SPELLCARD_ST6_BOSS_4E,
    SPELLCARD_ST6_BOSS_4N,
    SPELLCARD_ST6_BOSS_4H,
    SPELLCARD_ST6_BOSS_4L,
    SPELLCARD_ST6_BOSS_5E,
    SPELLCARD_ST6_BOSS_5N,
    SPELLCARD_ST6_BOSS_5H,
    SPELLCARD_ST6_BOSS_5L,
    SPELLCARD_ST6_BOSS_6E,
    SPELLCARD_ST6_BOSS_6N,
    SPELLCARD_ST6_BOSS_6H,
    SPELLCARD_ST6_BOSS_6L,

    SPELLCARD_EX_MBOSS_1,
    SPELLCARD_EX_MBOSS_2,
    SPELLCARD_EX_BOSS_1,
    SPELLCARD_EX_BOSS_2,
    SPELLCARD_EX_BOSS_3,
    SPELLCARD_EX_BOSS_4,
    SPELLCARD_EX_BOSS_5,
    SPELLCARD_EX_BOSS_6,
    SPELLCARD_EX_BOSS_7,
    SPELLCARD_EX_BOSS_8,
    SPELLCARD_EX_BOSS_9,
    SPELLCARD_EX_BOSS_10,

    SPELLCARD_PH_MBOSS_1,
    SPELLCARD_PH_MBOSS_2,
    SPELLCARD_PH_BOSS_1,
    SPELLCARD_PH_BOSS_2,
    SPELLCARD_PH_BOSS_3,
    SPELLCARD_PH_BOSS_4,
    SPELLCARD_PH_BOSS_5,
    SPELLCARD_PH_BOSS_6,
    SPELLCARD_PH_BOSS_7,
    SPELLCARD_PH_BOSS_8,
    SPELLCARD_PH_BOSS_9,
    SPELLCARD_PH_BOSS_10,
    SPELLCARD_PH_BOSS_11,

    SPELLCARD_COUNT,
} SpellcardNumber;

struct Th7k
{
    u32 magic;
    u16 th7kLen;
    u16 th7kLen2;
    u8 version;
    u8 isPlayerScore;
    u8 pad[2];
};
static_assert(sizeof(Th7k) == 0xc);

struct Catk
{
    Th7k base;
    u32 highScorePerShot[SHOT_COUNT + 1];
    u16 idx;
    u8 nameCsum;
    char name[49];
    u16 numAttemptsPerShot[SHOT_COUNT + 1];
    u16 numSuccessesPerShot[SHOT_COUNT + 1];
};
static_assert(sizeof(Catk) == 0x78);

struct Hscr
{
    Th7k base;
    u32 score;
    f32 slowRatePercent;
    u8 character;
    u8 difficulty;
    u8 stage;
    char name[9];
    char date[6];
    i8 numRetries;
    u8 pad;
};
static_assert(sizeof(Hscr) == 0x28);

struct Clrd
{
    Th7k base;
    u8 difficultyClearedWithRetries[DIFF_COUNT];
    u8 difficultyClearedWithoutRetries[DIFF_COUNT];
    u8 characterShotType;
    u8 pad[3];
};
static_assert(sizeof(Clrd) == 0x1c);

struct Pscr
{
    Th7k base;
    i32 playCount;
    i32 score;
    u8 character;
    u8 difficulty;
    u8 stage;
    u8 pad;
};
static_assert(sizeof(Pscr) == 0x18);

struct PlstPlayCounts
{
    u32 playCount;
    u32 playCountPerShotType[SHOT_COUNT];
    u32 clearCount;
    u32 noContinueClearCount;
    u32 retryCount;
    u32 extraClearCount;
};
static_assert(sizeof(PlstPlayCounts) == 0x2c);

struct Plst
{
    Th7k base;
    u32 totalHours;
    u32 totalMinutes;
    u32 totalSeconds;
    u32 totalMilliseconds;
    u32 gameHours;
    u32 gameMinutes;
    u32 gameSeconds;
    u32 gameMilliseconds;
    PlstPlayCounts playDataByDifficulty[DIFF_COUNT + 1]; // 7 is Total
};
static_assert(sizeof(Plst) == 0x160);

struct Lsnm
{
    Th7k base;
    char name[12];
};
static_assert(sizeof(Lsnm) == 0x18);

struct Vrsm
{
    Th7k base;
    char versionStr[6];
    i32 exeSize;
    i32 exeChecksum;
};
static_assert(sizeof(Vrsm) == 0x1c);

struct ScoreDatRaw
{
    u8 xorseed[2];
    u16 csum;
    u16 magic;
    u8 unused_6;
    u8 pad1;
    i32 dataOffset;
    u32 reservedPtr;
    i32 fileLength;
    u32 dstLen;
    i32 srcLen;
};
static_assert(sizeof(ScoreDatRaw) == 0x1c);

struct ScoreListNode
{
    ScoreListNode()
    {
        prev = NULL;
        next = NULL;
        data = NULL;
    }

    ScoreListNode *prev;
    ScoreListNode *next;
    Hscr *data;
};

struct ScoreDat
{
    ScoreDatRaw raw;
    ScoreListNode *scores;
    u8 *decodedData;
};

struct ResultScreen
{
    ResultScreen()
    {
        memset((void *)this, 0, sizeof(ResultScreen));
        this->cursor = 1;
    }

    ~ResultScreen()
    {
        free(this->scoreDat);
    }

    static ZunResult RegisterChain(u32 type);

    static ZunResult AddedCallback(ResultScreen *arg);
    static ZunResult DeletedCallback(ResultScreen *arg);
    static u32 OnUpdate(ResultScreen *arg);
    static u32 OnDraw(ResultScreen *arg);

    ZunResult CheckConfirmButton();
    ZunResult DrawFinalStats();
    i32 DrawStats();
    static void GetDate(char *outDate);
    ZunResult HandleReplaySaveKeyboard();
    ZunResult HandleResultKeyboard();
    static i32 MoveCursor(ResultScreen *screen, i32 max);
    static i32 MoveCursor2(ResultScreen *screen, i32 max);
    static i32 MoveCursorHorizontally(ResultScreen *screen, i32 max);

    static ScoreDat *OpenScore(const char *path);
    static i32 LinkScore(ScoreListNode *prevNode, Hscr *hscr);
    i32 LinkScoreEx(Hscr *out, i32 difficulty, i32 character);
    static u32 GetHighScore(ScoreDat *scoreDat, ScoreListNode *node, u32 character, u32 difficulty,
                            u8 *numRetries);
    static ZunResult ParseCatk(ScoreDat *scoreDat, Catk *outCatk);
    static ZunResult ParseClrd(ScoreDat *scoreDat, Clrd *outClrd);
    static ZunResult ParsePlst(ScoreDat *scoreDat, Plst *outPlst);
    static ZunResult ParsePscr(ScoreDat *scoreDat, Pscr *outPscr);
    static ZunResult ParseScores();
    static void ReleaseScoreDat(ScoreDat *scoreDat);
    void FreeScore(i32 difficulty, i32 character);
    static void FreeAllScores(ScoreListNode *scores);
    static i32 ParseLsnm(ScoreDat *scoreDat, Lsnm *outLsnm);
    void WriteScore();

    void UpdatePrev()
    {
        for (i32 i = 0; i < 41; i++)
        {
            this->vms[i].UpdatePrev();
        }
        for (i32 i = 0; i < 15; i++)
        {
            this->spellcardListVms[i].UpdatePrev();
        }
        this->leftArrowVm.UpdatePrev();
        this->rightArrowVm.UpdatePrev();
    }

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
    i32 unused_4c;
    i32 totalPlayCountPerShot[SHOT_COUNT + 1];
    u8 lastTotalSeconds;
    u8 pad[3];
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
    ReplayFile replays[15];
    ReplayFile defaultReplay;
};
