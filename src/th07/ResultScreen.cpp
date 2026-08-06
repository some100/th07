#include "ResultScreen.hpp"

#include <direct.h>
#include <stdio.h>
#include <time.h>

#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameManager.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "ZunResult.hpp"
#include "pbg4/Lzss.hpp"
#include "utils.hpp"

// GLOBAL: TH07 0x004964f4
static const f32 g_DifficultyWeightsList[] = {-30.0f, -10.0f, 20.0f, 30.0f, 30.0f};

// GLOBAL: TH07 0x0049ec30
// STRING: TH07 0x004969c8
const char *g_AlphabetList = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.,:;_@abcdefghijklmnopqrstuvwxyz+-/*=%0123456789#!?'\"$(){}[]<>&\\|~^ --";

// GLOBAL: TH07 0x0049ec34
const char *g_CharacterList[6] = {
    // STRING: TH07 0x004969b0
    "博麗 霊夢 (霊)　",
    // STRING: TH07 0x0049699c
    "博麗 霊夢 (夢)　",
    // STRING: TH07 0x00496988
    "霧雨 魔理沙 (魔)",
    // STRING: TH07 0x00496974
    "霧雨 魔理沙 (恋)",
    // STRING: TH07 0x00496960
    "十六夜 咲夜 (幻)",
    // STRING: TH07 0x0049694c
    "十六夜 咲夜 (時)",
};

// GLOBAL: TH07 0x0049ec4c
// STRING: TH07 0x00496938
const char *g_TotalForAllProtagonists = "全主人公合計  　";

// GLOBAL: TH07 0x0049f4ec
const char *g_CharactersAndShotTypesStrings[6] = {
    "ReimuA ",
    "ReimuB ",
    "MarisaA",
    "MarisaB",
    "SakuyaA",
    "SakuyaB",
};

// GLOBAL: TH07 0x0049f504
static const f32 g_DifficultySpellcardWeightsList[] = {1.0f, 1.5f, 1.5f, 2.0f, 2.5f};

// GLOBAL: TH07 0x0049f518
const char *g_DifficultyNameTable[6] = {
    // STRING: TH07 0x00496544
    "      Easy",
    // STRING: TH07 0x00496538
    "    Normal",
    // STRING: TH07 0x0049652c
    "      Hard",
    // STRING: TH07 0x00496520
    "   Lunatic",
    // STRING: TH07 0x00496514
    "     Extra",
    // STRING: TH07 0x00496508
    "  Phantasm",
};

// FUNCTION: TH07 0x00444b56
i32 ResultScreen::LinkScore(ScoreListNode *prevNode, Hscr *hscr)
{
    ScoreListNode *nextNode;
    i32 scoresAmount;

    scoresAmount = 0;
    while (prevNode->next)
    {
        if (prevNode->next->data &&
            prevNode->next->data->score <= hscr->score)
        {
            break;
        }
        prevNode = prevNode->next;
        scoresAmount++;
    }
    nextNode = prevNode->next;
    prevNode->next = (ScoreListNode *)ZunMemory::Alloc2(sizeof(ScoreListNode));
    prevNode->next->prev = prevNode;
    prevNode = prevNode->next;
    prevNode->data = hscr;
    prevNode->next = nextNode;
    return scoresAmount;
}

// FUNCTION: TH07 0x00444bed
void ResultScreen::FreeAllScores(ScoreListNode *scores)
{
    ScoreListNode *next;

    scores = scores->next;
    while (scores)
    {
        next = scores->next;
        free(scores);
        scores = next;
    }
}

#pragma var_order(uncompressedData, scoreData, i, xorValue,                \
                  checksum, idx, remainingData, chunk, cursor, parsedTh7k, \
                  isTh7k, parsedVrsm)
// FUNCTION: TH07 0x00444c20
ScoreDat *ResultScreen::OpenScore(const char *path)
{
    ScoreDat *uncompressedData;
    ScoreDat *scoreData;
    i32 i;
    u8 xorValue;
    u16 checksum;
    u8 *idx;
    i32 remainingData;
    Th7k *chunk;
    i32 cursor;
    Th7k *parsedTh7k;
    i32 isTh7k;
    Vrsm *parsedVrsm;

    Supervisor::DebugPrint2("info : score load\r\n");
    scoreData = (ScoreDat *)FileSystem::OpenFile(path, 1);
    if (!scoreData)
    {
    RECREATE_SCORE:
        Supervisor::DebugPrint2("info : score recreate\r\n");
        if (scoreData)
        {
            free(scoreData);
        }
        scoreData = (ScoreDat *)ZunMemory::Alloc2(sizeof(ScoreDat));
        scoreData->dataOffset = sizeof(ScoreDat);
        scoreData->fileLength = sizeof(ScoreDat);
        goto INIT_SCORES;
    }

    if (g_LastFileSize < sizeof(ScoreDat))
    {
        Supervisor::DebugPrint2("warning : score.dat size is short\r\n");
        free(scoreData);
        goto RECREATE_SCORE;
    }

    remainingData = g_LastFileSize - 2;
    checksum = 0;
    xorValue = 0;
    i = 0;
    idx = (u8 *)scoreData + 1;
    while (remainingData > 0)
    {
        xorValue += idx[0];
        xorValue = (xorValue & 0xe0) >> 5 | (xorValue & 0x1f) << 3;
        idx[1] ^= xorValue;
        if (i >= 2)
        {
            checksum += idx[1];
        }
        idx++;
        remainingData--;
        i++;
    }

    if (scoreData->csum != checksum)
    {
        Supervisor::DebugPrint2("warning : score.dat chksum error\r\n");
        goto RECREATE_SCORE;
    }

    if (scoreData->dataOffset != sizeof(ScoreDat))
    {
        Supervisor::DebugPrint2("warning : header size is mismatch\r\n");
        goto RECREATE_SCORE;
    }

    if (scoreData->magic != 11)
    {
        Supervisor::DebugPrint2("warning : score.dat version mismatch\r\n");
        goto RECREATE_SCORE;
    }

    uncompressedData = (ScoreDat *)ZunMemory::Alloc2(0xa001c);
    memcpy(uncompressedData, scoreData, sizeof(ScoreDat));
    Lzss::Decompress(
        (u8 *)scoreData + sizeof(ScoreDat), scoreData->srcLen,
        (u8 *)uncompressedData + sizeof(ScoreDat), scoreData->dstLen);
    free(scoreData);
    scoreData = uncompressedData;

    cursor = scoreData->fileLength;
    isTh7k = false;
    chunk = (Th7k *)((u8 *)scoreData + scoreData->dataOffset);
    cursor -= scoreData->dataOffset;

    while (cursor > 0)
    {
        if (chunk->magic == TH7K_MAGIC)
        {
            isTh7k = true;
            parsedTh7k = chunk;
        }
        if (chunk->magic == VRSM_MAGIC)
        {
            if (chunk->version == 1)
            {
                parsedVrsm = (Vrsm *)chunk;
                if (g_Supervisor.CheckIntegrity(
                        parsedVrsm->versionStr,
                        parsedVrsm->exeSize,
                        parsedVrsm->exeChecksum) != ZUN_SUCCESS)
                {
                    Supervisor::DebugPrint2("warning : score.dat exesumcheck error\r\n");
                    goto RECREATE_SCORE;
                }
            }
        }
        if (chunk->th7kLen == 0)
        {
            Supervisor::DebugPrint2("warning : score.dat chapter size is ZERO\r\n");
            goto RECREATE_SCORE;
        }
        cursor -= chunk->th7kLen;
        chunk = (Th7k *)((u8 *)chunk + chunk->th7kLen);
    }

    if (!isTh7k || parsedTh7k->version != 1)
    {
        Supervisor::DebugPrint2("warning : score.dat version mismatch\r\n");
        goto RECREATE_SCORE;
    }

INIT_SCORES:
    scoreData->scores = (ScoreListNode *)ZunMemory::Alloc2(sizeof(ScoreListNode));
    scoreData->scores->next = NULL;
    scoreData->scores->data = NULL;
    scoreData->scores->prev = NULL;
    return scoreData;
}

#pragma var_order(parsedHscr, cursor, sd)
// FUNCTION: TH07 0x00444f0d
u32 ResultScreen::GetHighScore(ScoreDat *scoreDat, ScoreListNode *node,
                               u32 character, u32 difficulty, u8 *numRetries)
{
    ScoreDat *sd = scoreDat;
    i32 cursor;
    Hscr *parsedHscr;

    if (!node)
    {
        FreeAllScores(sd->scores);
        sd->scores->next = NULL;
        sd->scores->data = NULL;
        sd->scores->prev = NULL;
    }

    cursor = sd->fileLength;
    parsedHscr = (Hscr *)(sd->xorseed + sd->dataOffset);
    cursor -= sd->dataOffset;
    while (cursor > 0)
    {
        if (parsedHscr->magic == HSCR_MAGIC && parsedHscr->version == 1 &&
            parsedHscr->character == character &&
            parsedHscr->difficulty == difficulty)
        {
            if (node)
            {
                LinkScore(node, parsedHscr);
            }
            else
            {
                LinkScore(sd->scores, parsedHscr);
            }
        }
        cursor -= parsedHscr->th7kLen;
        parsedHscr = (Hscr *)((u8 *)parsedHscr + parsedHscr->th7kLen);
    }
    if (numRetries != 0)
    {
        *numRetries = sd->scores->next ? sd->scores->next->data->numRetries : 0;
    }
    return sd->scores->next
               ? sd->scores->next->data->score > 100000
                     ? sd->scores->next->data->score
                     : 100000
               : 100000;
}

#pragma var_order(parsedCatk, cursor, sd)
// FUNCTION: TH07 0x00445069
ZunResult ResultScreen::ParseCatk(ScoreDat *scoreDat, Catk *outCatk)
{
    Catk *parsedCatk;
    i32 cursor;
    ScoreDat *sd = scoreDat;

    if (!outCatk)
    {
        return ZUN_ERROR;
    }

    parsedCatk = (Catk *)(sd->xorseed + sd->dataOffset);
    cursor = sd->fileLength - sd->dataOffset;
    while (cursor > 0)
    {
        if (parsedCatk->magic == CATK_MAGIC && parsedCatk->version == 1)
        {
            if (parsedCatk->idx >= 141)
            {
                break;
            }
            outCatk[parsedCatk->idx] = *parsedCatk;
        }
        cursor -= parsedCatk->th7kLen;
        parsedCatk = (Catk *)((u8 *)parsedCatk + parsedCatk->th7kLen);
    }
    return ZUN_SUCCESS;
}

#pragma var_order(parsedLsnm, cursor, sd)
// FUNCTION: TH07 0x00445110
i32 ResultScreen::ParseLsnm(ScoreDat *scoreDat, Lsnm *outLsnm)
{
    i32 cursor;
    Lsnm *parsedLsnm;
    ScoreDat *sd = scoreDat;

    parsedLsnm = (Lsnm *)(sd->xorseed + sd->dataOffset);
    cursor = sd->fileLength - sd->dataOffset;
    while (cursor > 0)
    {
        if (parsedLsnm->magic == LSNM_MAGIC && parsedLsnm->version == 1)
        {
            *outLsnm = *parsedLsnm;
            return 1;
        }
        cursor -= parsedLsnm->th7kLen;
        parsedLsnm = (Lsnm *)((u8 *)parsedLsnm + parsedLsnm->th7kLen);
    }
    return 0;
}

#pragma var_order(parsedClrd, i, cursor, j, sd)
// FUNCTION: TH07 0x00445192
ZunResult ResultScreen::ParseClrd(ScoreDat *scoreDat, Clrd *outClrd)
{
    Clrd *parsedClrd;
    i32 i;
    i32 cursor;
    i32 j;
    ScoreDat *sd = scoreDat;

    if (!outClrd)
    {
        return ZUN_ERROR;
    }

    for (i = 0; i < 6; i++)
    {
        memset(outClrd + i, 0, sizeof(Clrd));
        outClrd[i].magic = CLRD_MAGIC;
        outClrd[i].th7kLen2 = sizeof(Clrd);
        outClrd[i].th7kLen = sizeof(Clrd);
        outClrd[i].version = 1;
        outClrd[i].characterShotType = (u8)i;
        for (j = 0; j < 5; j++)
        {
            outClrd[i].difficultyClearedWithRetries[j] = 1;
            outClrd[i].difficultyClearedWithoutRetries[j] = 1;
        }
    }
    parsedClrd = (Clrd *)(sd->xorseed + sd->dataOffset);
    cursor = sd->fileLength - sd->dataOffset;
    while (cursor > 0)
    {
        if (parsedClrd->magic == CLRD_MAGIC && parsedClrd->version == 1)
        {
            if (parsedClrd->characterShotType >= 6)
            {
                break;
            }
            outClrd[parsedClrd->characterShotType] = *parsedClrd;
        }
        cursor -= parsedClrd->th7kLen;
        parsedClrd = (Clrd *)((u8 *)parsedClrd + parsedClrd->th7kLen);
    }
    return ZUN_SUCCESS;
}

#pragma var_order(pscr, parsedPscr, i, j, cursor, k, sd)
// FUNCTION: TH07 0x004452f4
ZunResult ResultScreen::ParsePscr(ScoreDat *scoreDat, Pscr *outPscr)
{
    i32 k;
    i32 cursor;
    i32 j;
    i32 i;
    Pscr *parsedPscr;
    Pscr *pscr;
    ScoreDat *sd = scoreDat;

    if (!outPscr)
    {
        return ZUN_ERROR;
    }

    pscr = outPscr;
    for (i = 0; i < 6; i++)
    {
        for (j = 0; j < 6; j++)
        {
            for (k = 0; k < 4; k++, pscr++)
            {
                memset(pscr, 0, sizeof(Pscr));
                pscr->magic = PSCR_MAGIC;
                pscr->th7kLen2 = sizeof(Pscr);
                pscr->th7kLen = sizeof(Pscr);
                pscr->version = 1;
                pscr->character = i;
                pscr->difficulty = k;
                pscr->stage = j;
                pscr->playCount = 0;
            }
        }
    }
    parsedPscr = (Pscr *)(sd->xorseed + sd->dataOffset);
    cursor = sd->fileLength - sd->dataOffset;
    while (cursor > 0)
    {
        if (parsedPscr->magic == PSCR_MAGIC && parsedPscr->version == 1)
        {
            pscr = parsedPscr;
            if (pscr->character >= 6 ||
                (pscr->difficulty >= 5 || pscr->stage >= 7))
            {
                break;
            }
            outPscr[pscr->character * 6 * 4 + pscr->stage * 4 +
                    pscr->difficulty] = *pscr;
        }
        cursor -= parsedPscr->th7kLen;
        parsedPscr = (Pscr *)((u8 *)parsedPscr + parsedPscr->th7kLen);
    }
    return ZUN_SUCCESS;
}

#pragma var_order(parsedPlst, cursor, sd)
// FUNCTION: TH07 0x0044547f
ZunResult ResultScreen::ParsePlst(ScoreDat *scoreDat, Plst *outPlst)
{
    i32 cursor;
    Plst *parsedPlst;
    ScoreDat *sd = scoreDat;

    parsedPlst = (Plst *)(sd->xorseed + sd->dataOffset);
    cursor = sd->fileLength - sd->dataOffset;
    while (cursor > 0)
    {
        if (parsedPlst->magic == PLST_MAGIC && parsedPlst->version == 1)
        {
            *outPlst = *parsedPlst;
        }
        cursor -= parsedPlst->th7kLen;
        parsedPlst = (Plst *)((u8 *)parsedPlst + parsedPlst->th7kLen);
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x004454fc
void ResultScreen::ReleaseScoreDat(ScoreDat *scoreDat)
{
    FreeAllScores(scoreDat->scores);
    ZunMemory::Free(scoreDat->scores);
    free(scoreDat);
}

#pragma var_order(difficulty, characterSlot, fileBuffer, sizeOfFile,         \
                  currentCharacter, character, clrd, catk, pscr, j, k, vrsm, \
                  compressedBuffer, scoreDat, originalByte, remainingSize,   \
                  xorValue, bytes, sd)
// FUNCTION: TH07 0x0044552c
void ResultScreen::WriteScore()
{
    ScoreDat *sd;
    u8 *bytes;
    u8 xorValue;
    i32 remainingSize;
    u8 originalByte;
    ScoreDat *scoreDat;
    u8 *compressedBuffer;
    Vrsm vrsm;
    i32 k;
    i32 j;
    Pscr *pscr;
    Catk *catk;
    Clrd *clrd;
    i32 character;
    ScoreListNode *currentCharacter;
    size_t sizeOfFile;
    u8 *fileBuffer;
    i32 characterSlot;
    i32 difficulty;

    sizeOfFile = 0;

    fileBuffer = (u8 *)ZunMemory::Alloc2(0xa0000);

    memcpy(fileBuffer + sizeOfFile, this->scoreDat, sizeof(ScoreDat));
    sizeOfFile += sizeof(ScoreDat);

    this->th7kHeader.magic = TH7K_MAGIC;
    this->th7kHeader.th7kLen2 = sizeof(Th7k);
    this->th7kHeader.th7kLen = sizeof(Th7k);
    this->th7kHeader.version = 1;

    memcpy(fileBuffer + sizeOfFile, &this->th7kHeader, sizeof(Th7k));
    sizeOfFile += sizeof(Th7k);

    for (difficulty = 0; difficulty < 6; difficulty++)
    {
        for (character = 0; character < 6; character++)
        {
            currentCharacter = this->scoreLists[difficulty][character].next;
            characterSlot = 0;

            for (;;)
            {
                if (currentCharacter)
                {
                    if (currentCharacter->data->magic == HSCR_MAGIC)
                    {
                        currentCharacter->data->character = character;
                        currentCharacter->data->difficulty = difficulty;
                        currentCharacter->data->th7kLen2 = sizeof(Hscr);
                        currentCharacter->data->th7kLen = sizeof(Hscr);
                        currentCharacter->data->version = 1;
                        currentCharacter->data->isPlayerScore = 0;

                        memcpy(fileBuffer + sizeOfFile, currentCharacter->data, sizeof(Hscr));
                        sizeOfFile += sizeof(Hscr);
                    }

                    currentCharacter = currentCharacter->next;
                    characterSlot++;

                    if (characterSlot >= 10)
                    {
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
                break;
            }
        }
    }

    clrd = g_GameManager.clrd;
    for (difficulty = 0; difficulty < 6; difficulty++, clrd++)
    {
        clrd->magic = CLRD_MAGIC;
        clrd->th7kLen2 = sizeof(Clrd);
        clrd->th7kLen = sizeof(Clrd);
        clrd->version = 1;

        memcpy(fileBuffer + sizeOfFile, clrd, sizeof(Clrd));
        sizeOfFile += sizeof(Clrd);
    }

    catk = g_GameManager.catk;
    for (difficulty = 0; difficulty < 141; difficulty++, catk++)
    {
        if (catk->magic == CATK_MAGIC)
        {
            catk->idx = difficulty;
            catk->th7kLen2 = sizeof(Catk);
            catk->th7kLen = sizeof(Catk);
            catk->version = 1;

            memcpy(fileBuffer + sizeOfFile, catk, sizeof(Catk));
            sizeOfFile += sizeof(Catk);
        }
    }

    pscr = &g_GameManager.pscr[0][0][0];
    for (difficulty = 0; difficulty < 6; difficulty++)
    {
        for (j = 0; j < 6; j++)
        {
            for (k = 0; k < 4; k++, pscr++)
            {
                if (pscr->score != 0)
                {
                    memcpy(fileBuffer + sizeOfFile, pscr, sizeof(Pscr));
                    sizeOfFile += sizeof(Pscr);
                }
            }
        }
    }
    memcpy(fileBuffer + sizeOfFile, &this->lsnmHeader, sizeof(Lsnm));
    sizeOfFile += sizeof(Lsnm);

    g_Supervisor.UpdateStartupTime();

    memcpy(fileBuffer + sizeOfFile, &g_GameManager.plst, sizeof(Plst));
    sizeOfFile += sizeof(Plst);

    vrsm.magic = VRSM_MAGIC;
    vrsm.version = 1;
    vrsm.th7kLen2 = sizeof(Vrsm);
    vrsm.th7kLen = sizeof(Vrsm);
    vrsm.isPlayerScore = 0;
    strcpy(vrsm.versionStr, "0100b");
    vrsm.exeSize = g_Supervisor.exeSize;
    vrsm.exeChecksum = g_Supervisor.exeChecksum;

    memcpy(fileBuffer + sizeOfFile, &vrsm, sizeof(Vrsm));
    sizeOfFile += sizeof(Vrsm);

    scoreDat = (ScoreDat *)fileBuffer;
    scoreDat->dstLen = sizeOfFile - sizeof(ScoreDat);
    scoreDat->fileLength = sizeOfFile;
    compressedBuffer = Lzss::Compress(fileBuffer + sizeof(ScoreDat),
                                      scoreDat->dstLen,
                                      &scoreDat->srcLen);

    memcpy(fileBuffer + sizeof(ScoreDat), compressedBuffer,
           scoreDat->srcLen);
    GlobalFree(compressedBuffer);
    sizeOfFile = scoreDat->srcLen + sizeof(ScoreDat);

    sd = (ScoreDat *)fileBuffer;
    sd->dataOffset = sizeof(ScoreDat);
    sd->csum = 0;
    sd->xorseed[1] = g_Rng.GetRandomU16InRange(256);
    sd->unused_6 = g_Rng.GetRandomU16InRange(256);
    sd->magic = 11;
    for (remainingSize = 4; remainingSize < (i32)sizeOfFile; remainingSize++)
    {
        sd->csum += fileBuffer[remainingSize];
    }
    xorValue = 0;
    originalByte = 0;

    bytes = (u8 *)sd + 1;
    remainingSize = sizeOfFile;
    remainingSize -= 2;
    xorValue = bytes[0];

    while (remainingSize > 0)
    {
        originalByte = bytes[1];
        xorValue = (i32)(xorValue & 0xe0) >> 5 | (xorValue & 0x1f) << 3;
        bytes[1] ^= xorValue;
        xorValue += originalByte;
        bytes++;
        remainingSize--;
    }
    FileSystem::WriteDataToFile("score.dat", fileBuffer, sizeOfFile);
    free(fileBuffer);
}

// FUNCTION: TH07 0x00445a57
i32 ResultScreen::MoveCursor(ResultScreen *screen, i32 max)
{
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_UP))
    {
        screen->cursor--;
        if (screen->cursor < 0)
        {
            screen->cursor = screen->cursor + max;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return -1;
    }
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_DOWN))
    {
        screen->cursor = screen->cursor + 1;
        if (screen->cursor >= max)
        {
            screen->cursor = screen->cursor - max;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return 1;
    }
    return 0;
}

// FUNCTION: TH07 0x00445b56
i32 ResultScreen::MoveCursor2(ResultScreen *screen, i32 max)
{
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_UP))
    {
        screen->spellcardListPage--;
        if (screen->spellcardListPage < 0)
        {
            screen->spellcardListPage = screen->spellcardListPage + max;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return -1;
    }
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_DOWN))
    {
        screen->spellcardListPage = screen->spellcardListPage + 1;
        if (screen->spellcardListPage >= max)
        {
            screen->spellcardListPage = screen->spellcardListPage - max;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return 1;
    }
    return 0;
}

// FUNCTION: TH07 0x00445c55
i32 ResultScreen::MoveCursorHorizontally(ResultScreen *screen, i32 max)
{
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_LEFT))
    {
        screen->cursor--;
        if (screen->cursor < 0)
        {
            screen->cursor = screen->cursor + max;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return -1;
    }
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_RIGHT))
    {
        screen->cursor = screen->cursor + 1;
        if (screen->cursor >= max)
        {
            screen->cursor = screen->cursor - max;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        return 1;
    }
    return 0;
}

// FUNCTION: TH07 0x00445d60
i32 ResultScreen::LinkScoreEx(Hscr *out, i32 difficulty, i32 character)
{
    return LinkScore(this->scoreLists[difficulty] + character, out);
}

// FUNCTION: TH07 0x00445d8b
void ResultScreen::FreeScore(i32 difficulty, i32 character)
{
    FreeAllScores(&this->scoreLists[difficulty][character]);
}

#pragma var_order(vmIdx, vm, i, j)
// FUNCTION: TH07 0x00445db3
u32 ResultScreen::OnUpdate(ResultScreen *arg)
{
    i32 j;
    i32 i;
    AnmVm *vm;
    i32 vmIdx;

    switch (arg->resultScreenState)
    {
    case 18:
        g_Supervisor.curState = 1;
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    case 19:
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    case 0:
    switchD_00445ddb_caseD_0:
        if (arg->frameTimer == 0)
        {
            vm = arg->vms;
            for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
            {
                vm->pendingInterrupt = 1;
                vm->flag6 = 1;
                vm->color.color = vm->color.color;
            }
            vm = arg->vms;
            for (vmIdx = 0; vmIdx <= 8; vmIdx++, vm++)
            {
                if (vmIdx == arg->cursor)
                {
                    vm->color.color = 0xffffffff;
                    vm->offset = Float3(-4.0f, -4.0f, 0.0f);
                }
                else
                {
                    vm->color.color = 0xb0ffffff;
                    vm->offset = Float3(0.0f, 0.0f, 0.0f);
                }
            }
            if (!g_GameManager.HasUnlockedPhantomAndMaxClears())
            {
                arg->vms[5].active = 0;
                arg->vms[6].offset.y += -32.0f;
                arg->vms[7].offset.y += -32.0f;
                arg->vms[8].offset.y += -32.0f;
            }
            else
            {
                arg->vms[5].active = 1;
            }
        }
        if (arg->frameTimer < 20)
        {
            break;
        }
        arg->resultScreenState++;
        arg->frameTimer = 0;
    case 1:
        vmIdx = MoveCursor(arg, 9);
        if (arg->cursor == 5 &&
            !g_GameManager.HasUnlockedPhantomAndMaxClears())
        {
            arg->cursor += vmIdx;
        }
        vm = arg->vms;
        for (vmIdx = 0; vmIdx <= 8; vmIdx++, vm++)
        {
            if (vmIdx == arg->cursor)
            {
                vm->color.color = 0xffffffff;
                vm->offset = Float3(-4.0f, -4.0f, 0.0f);
            }
            else
            {
                vm->color.color = 0xb0ffffff;
                vm->offset = Float3(0.0f, 0.0f, 0.0f);
            }
        }
        if (!g_GameManager.HasUnlockedPhantomAndMaxClears())
        {
            arg->vms[5].active = 0;
            arg->vms[6].offset.y += -32.0f;
            arg->vms[7].offset.y += -32.0f;
            arg->vms[8].offset.y += -32.0f;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            vm = arg->vms;
            if (arg->cursor == 8)
            {
                goto GO_BACK;
            }
            arg->cursor = 8;
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            vm = arg->vms;
            switch (arg->cursor)
            {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
                {
                    vm->pendingInterrupt = arg->cursor + 3;
                }
                arg->diffPlayed = arg->cursor;
                arg->resultScreenState = arg->cursor + 3;
                arg->stateStep = arg->resultScreenState;
                arg->frameTimer = 0;
                arg->cursor = arg->prevCursor;
                arg->charUsed = -1;
                arg->lastSpellcardSelected = -1;
                break;
            case 6:
                for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
                {
                    vm->pendingInterrupt = 10;
                }
                arg->diffPlayed = arg->cursor;
                arg->resultScreenState = 9;
                arg->stateStep = arg->resultScreenState;
                arg->frameTimer = 0;
                arg->charUsed = -1;
                arg->cursor = arg->savedCursor;
                arg->lastSpellcardSelected = -1;
                break;
            case 7:
                for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
                {
                    vm->pendingInterrupt = 9;
                }
                arg->diffPlayed = arg->cursor;
                arg->resultScreenState = 20;
                arg->stateStep = arg->resultScreenState;
                arg->frameTimer = 0;
                arg->charUsed = -1;
                break;
            GO_BACK:
            case 8:
                for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
                {
                    vm->pendingInterrupt = 2;
                }
                arg->resultScreenState = 2;
                g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
                arg->frameTimer = 0;
                break;
            }
        }
        break;
    case 2:
        if (arg->frameTimer < 60)
        {
            break;
        }
        g_Supervisor.curState = 1;
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    case 5:
        if (IS_PRESSED_RAW(TH_BUTTON_FOCUS) || IS_PRESSED_RAW(TH_BUTTON_SKIP))
        {
            if (arg->cheatCodeStep < 3)
            {
                if (WAS_PRESSED_RAW(TH_BUTTON_UP))
                {
                    arg->cheatCodeStep++;
                }
                else if (WAS_PRESSED_RAW(TH_BUTTON_WRONG_CHEATCODE))
                {
                    arg->cheatCodeStep = 0;
                }
            }
            else if (arg->cheatCodeStep < 5)
            {
                if (WAS_PRESSED_RAW(TH_BUTTON_D))
                {
                    arg->cheatCodeStep++;
                }
                else if (WAS_PRESSED_RAW(TH_BUTTON_WRONG_CHEATCODE))
                {
                    arg->cheatCodeStep = 0;
                }
            }
            else if (arg->cheatCodeStep < 7)
            {
                if (WAS_PRESSED_RAW(TH_BUTTON_DOWN))
                {
                    arg->cheatCodeStep++;
                }
                else if (WAS_PRESSED_RAW(TH_BUTTON_WRONG_CHEATCODE))
                {
                    arg->cheatCodeStep = 0;
                }
            }
            else if (arg->cheatCodeStep < 10)
            {
                if (WAS_PRESSED_RAW(TH_BUTTON_Q))
                {
                    arg->cheatCodeStep++;
                }
                else if (WAS_PRESSED_RAW(TH_BUTTON_WRONG_CHEATCODE))
                {
                    arg->cheatCodeStep = 0;
                }
            }
            else
            {
                for (i = 0; i < 6; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        g_GameManager.clrd[i].difficultyClearedWithRetries[j] = 99;
                        g_GameManager.clrd[i].difficultyClearedWithoutRetries[j] = 99;
                    }
                }
                arg->cheatCodeStep = 0;
                g_SoundPlayer.PlaySoundByIdx(SOUND_EXTEND, 0);
            }
        }
        else
        {
            arg->cheatCodeStep = 0;
        }
    case 3:
    case 4:
    case 6:
    case 7:
    case 8:
        if (arg->charUsed != arg->cursor && arg->frameTimer == 20)
        {
            arg->charUsed = arg->cursor;
            g_AnmManager->DrawStringFormat2(arg->spellcardListVms, 0xffffff, 0,
                                            g_CharacterList[arg->charUsed]);
            arg->spellcardListVms[0].color.bytes.a = 255;
        }
        if (arg->frameTimer < 30)
        {
            break;
        }
        if (MoveCursorHorizontally(arg, 6))
        {
            arg->frameTimer = 0;
            vm = arg->vms;
            for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
            {
                vm->pendingInterrupt = arg->diffPlayed + 3;
            }
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            arg->resultScreenState = 0;
            arg->frameTimer = 0;
            vm = arg->vms;
            for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
            {
                vm->pendingInterrupt = 1;
            }
            arg->prevCursor = arg->cursor;
            arg->cursor = arg->diffPlayed;
            goto switchD_00445ddb_caseD_0;
        }
        break;
    case 9:
        if ((arg->lastSpellcardSelected != arg->cursor ||
             arg->prevSpellcardListPage != arg->spellcardListPage) &&
            arg->frameTimer == 20)
        {
            arg->lastSpellcardSelected = arg->cursor;
            arg->prevSpellcardListPage = arg->spellcardListPage;
            for (vmIdx = arg->lastSpellcardSelected * 10;
                 vmIdx < arg->lastSpellcardSelected * 10 + 10;
                 vmIdx++)
            {
                if (vmIdx >= 141)
                {
                    break;
                }
                if (g_GameManager.catk[vmIdx].numAttemptsPerShot[6] == 0)
                {
                    AnmManager::DrawVmTextFmt(g_AnmManager,
                                              arg->spellcardListVms + vmIdx % 10,
                                              // STRING: TH07 0x00496818
                                              0xffffff, 0, "？？？？？");
                }
                else
                {
                    AnmManager::DrawVmTextFmt(
                        g_AnmManager, arg->spellcardListVms + vmIdx % 10, 0xffffff, 0,
                        g_GameManager.catk[vmIdx].name);
                }
                arg->spellcardListVms[vmIdx % 10].color.bytes.a = 255;
            }
            AnmManager::DrawVmTextFmt(
                g_AnmManager, arg->spellcardListVms + 10, 0xffffff, 0,
                // STRING: TH07 0x004967ec
                "%s %3d枚中%3d枚取得（キャラ切り替え↓↑）",
                g_CharacterList[arg->prevSpellcardListPage], 141,
                arg->totalPlayCountPerCharacter[arg->spellcardListPage]);
            arg->spellcardListVms[10].color.bytes.a = 255;
        }
        if (arg->frameTimer < 30)
        {
            break;
        }
        if (MoveCursorHorizontally(arg, 15) != 0)
        {
            arg->frameTimer = 0;
            vm = arg->vms;
            for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
            {
                vm->pendingInterrupt = 10;
            }
        }
        else if (MoveCursor2(arg, 7))
        {
            arg->frameTimer = 0;
            arg->listScrollAnimState = 1;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            arg->resultScreenState = 0;
            arg->frameTimer = 0;
            vm = arg->vms;
            for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
            {
                vm->pendingInterrupt = 1;
            }
            arg->savedCursor = arg->cursor;
            arg->cursor = arg->diffPlayed;
            goto switchD_00445ddb_caseD_0;
        }
        break;
    case 10:
        arg->HandleResultKeyboard();
        break;
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        arg->HandleReplaySaveKeyboard();
        break;
    case 16:
    case 17:
        arg->CheckConfirmButton();
        break;
    case 20:
    case 21:
    case 22:
        if (arg->DrawStats() != ZUN_SUCCESS)
        {
            goto switchD_00445ddb_caseD_0;
        }
        break;
    }
    vm = arg->vms;
    for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
    {
        g_AnmManager->ExecuteScript(vm);
    }
    arg->frameTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(vmIdx, vm, slowRateFactor, cursor, cursor2)
// FUNCTION: TH07 0x00446a66
ZunResult ResultScreen::HandleResultKeyboard()
{
    i32 cursor2;
    i32 cursor;
    f32 slowRateFactor;
    AnmVm *vm;
    i32 vmIdx;

    if (g_Supervisor.IsSlowMode() ||
        (g_Supervisor.flags >> 3 & 1) != 0)
    {
        this->resultScreenState = 16;
        this->frameTimer = 0;
        memcpy(g_GameManager.catk, g_GameManager.catkAgain, 0x4218);
        return ZUN_SUCCESS;
    }
    if (this->frameTimer == 0)
    {
        this->charUsed =
            (u32)g_GameManager.character * 2 + (u32)g_GameManager.shotType;
        this->diffPlayed = g_GameManager.difficulty;
        vm = this->vms;
        for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
        {
            vm->pendingInterrupt = this->diffPlayed + 3;
        }
        g_AnmManager->DrawStringFormat2(this->spellcardListVms, 0xffffff, 0,
                                        g_CharacterList[this->charUsed]);
        this->spellcardListVms[0].color.bytes.a = 255;
        this->curScore.character = (u8)this->charUsed;
        this->curScore.difficulty = (u8)this->diffPlayed;
        this->curScore.score = g_GameManager.globals->score;
        this->curScore.numRetries = g_GameManager.globals->numRetries;
        this->curScore.version = 1;
        this->curScore.magic = *(u32 *)&"HSCR";
        if (!g_GameManager.finished)
        {
            this->curScore.stage = (u8)g_GameManager.currentStage;
        }
        else
        {
            this->curScore.stage = 99;
        }
        this->curScore.isPlayerScore = 1;
        strcpy(this->curScore.name, this->lsnmHeader.name);
        GetDate(this->curScore.date);
        slowRateFactor =
            (g_Supervisor.framerateMultiplier / g_Supervisor.fpsAccumulator - 0.5f) * 2.0f;
        if (slowRateFactor < 0.0f)
        {
            slowRateFactor = 0.0f;
        }
        else if (slowRateFactor >= 1.0f)
        {
            slowRateFactor = 1.0f;
        }
        this->curScore.slowRatePercent = (1.0f - slowRateFactor) * 100.0f;
        if (LinkScoreEx(&this->curScore, this->diffPlayed, this->charUsed) >= 10)
        {
            goto LAB_004470e9;
        }
        this->cursor = 0;
        if (this->isClearingReplayName)
        {
            this->selectedChar = 95;
        }
        strcpy(this->replayName, "");
    }
    if (this->frameTimer < 30)
    {
        return ZUN_SUCCESS;
    }
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_UP))
    {
    WEIRD_ASS_LOOP_WITH_GOTO:
        this->selectedChar = this->selectedChar - 16;
        if (this->selectedChar < 0)
        {
            this->selectedChar = this->selectedChar + 96;
        }
        if (g_AlphabetList[this->selectedChar] == ' ')
        {
            goto WEIRD_ASS_LOOP_WITH_GOTO;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
    }
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_DOWN))
    {
    WEIRD_ASS_LOOP_WITH_GOTO_2:
        this->selectedChar = this->selectedChar + 16;
        if (this->selectedChar >= 96)
        {
            this->selectedChar = this->selectedChar - 96;
        }
        if (g_AlphabetList[this->selectedChar] == ' ')
        {
            goto WEIRD_ASS_LOOP_WITH_GOTO_2;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
    }
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_LEFT))
    {
    WEIRD_ASS_LOOP_WITH_GOTO_3:
        this->selectedChar--;
        if (this->selectedChar % 16 == 15)
        {
            this->selectedChar = this->selectedChar + 16;
        }
        if (this->selectedChar < 0)
        {
            this->selectedChar = 15;
        }
        if (g_AlphabetList[this->selectedChar] == ' ')
        {
            goto WEIRD_ASS_LOOP_WITH_GOTO_3;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
    }
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_RIGHT))
    {
    WEIRD_ASS_LOOP_WITH_GOTO_4:
        this->selectedChar = this->selectedChar + 1;
        if (this->selectedChar % 16 == 0)
        {
            this->selectedChar = this->selectedChar - 16;
        }
        if (g_AlphabetList[this->selectedChar] == ' ')
        {
            goto WEIRD_ASS_LOOP_WITH_GOTO_4;
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
    }
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_SELECTMENU))
    {
        cursor = this->cursor >= 8 ? 7 : this->cursor;
        if (this->selectedChar < 94)
        {
            this->curScore.name[cursor] = g_AlphabetList[this->selectedChar];
        }
        else if (this->selectedChar == 94)
        {
            this->curScore.name[cursor] = ' ';
        }
        else
        {
            goto LAB_004470db;
        }

        if (this->cursor < 8)
        {
            this->cursor++;
            if (this->cursor == 8)
            {
                this->selectedChar = 95;
            }
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
    }
    if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_RETURNMENU))
    {
        cursor2 = this->cursor >= 8 ? 7 : this->cursor;
        if (this->cursor > 0)
        {
            this->cursor--;
            this->curScore.name[cursor2] = ' ';
            this->curScore.name[this->cursor] = ' ';
        }
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
    }
    if (WAS_PRESSED_RAW(TH_BUTTON_MENU))
    {
    LAB_004470db:
        g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
    LAB_004470e9:
        this->resultScreenState = 16;
        this->frameTimer = 0;
        vm = this->vms;
        for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
        {
            vm->pendingInterrupt = 2;
        }
        strcpy(this->replayName, this->curScore.name);
        strcpy(this->lsnmHeader.name, this->replayName);
    }
    return ZUN_SUCCESS;
}

#pragma var_order(timeinfo, seconds)
// FUNCTION: TH07 0x00447161
void ResultScreen::GetDate(char *outDate)
{
    time_t seconds;
    tm *timeinfo;

    // ZUN bug: This is susceptible to the Year 2038 problem, meaning that your
    // game will probably crash when this function is called (like saving a
    // replay). This is because time_t here is a 32 bit signed integer, since
    // it's compiling for a 32 bit target. After 03:14:07, January 19, 2038,
    // UTC, the value stored in seconds will become negative. Passing this into
    // localtime will give you a NULL pointer for timeinfo, which then becomes
    // a null pointer dereference crash in strftime.
    time(&seconds);
    timeinfo = localtime(&seconds);
    // STRING: TH07 0x004967dc
    strftime(outDate, 6, "%m/%d", timeinfo);
}

#pragma var_order(vm, interrupt, vmIdx, replayFile, replayPath, cursor, \
                  replayPath2, cursor2)
// FUNCTION: TH07 0x00447198
ZunResult ResultScreen::HandleReplaySaveKeyboard()
{
    i32 cursor2;
    char replayPath2[64];
    i32 cursor;
    char replayPath[64];
    ReplayFile *replayFile;
    i32 vmIdx;
    i32 interrupt;
    AnmVm *vm;

    switch (this->resultScreenState)
    {
    case 11:
        if (this->frameTimer == 60)
        {
            if (g_Supervisor.IsSlowMode() ||
                (g_Supervisor.flags >> 3 & 1) != 0)
            {
                interrupt = 19;
            }
            else if (g_GameManager.globals->numRetries != 0)
            {
                interrupt = 14;
            }
            else
            {
                interrupt = 11;
            }
            vm = this->vms;
            for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
            {
                vm->pendingInterrupt = (i16)interrupt;
            }
            if (interrupt != 11)
            {
                this->resultScreenState = 12;
            }
            this->cursor = 0;
        }
        vm = this->vms + 19;
        if (this->cursor == 0)
        {
            vm[0].color.color =
                (vm[0].color.color & 0xff000000) | 0xff6060;
            vm[1].color.color =
                (vm[1].color.color & 0xff000000) | 0x606060;
        }
        else
        {
            vm[0].color.color =
                (vm[0].color.color & 0xff000000) | 0x606060;
            vm[1].color.color =
                (vm[1].color.color & 0xff000000) | 0xff6060;
        }
        if (this->frameTimer < 80)
        {
            return ZUN_SUCCESS;
        }
        MoveCursorHorizontally(this, 2);
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU) || WAS_PRESSED_RAW(TH_BUTTON_MENU))
        {
            goto SOUND_BACK_AND_RETURN;
        }
        else if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            if (this->cursor == 0)
            {
            LAB_004473e3:
                g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
                this->resultScreenState = 13;
                vm = this->vms;
                for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
                {
                    vm->pendingInterrupt = 12;
                }
                this->frameTimer = 0;
                goto LAB_0044756a;
            }

        SOUND_BACK_AND_RETURN:
            this->frameTimer = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->resultScreenState = 2;
            vm = this->vms;
            for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
            {
                vm->pendingInterrupt = 2;
            }
        }
        break;
    case 12:
        if (this->frameTimer < 20)
        {
            return ZUN_SUCCESS;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU) || WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            this->frameTimer = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->resultScreenState = 2;
            vm = this->vms;
            for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
            {
                vm->pendingInterrupt = 2;
            }
        }
        break;
    LAB_0044756a:
    case 13:
        if (this->frameTimer == 0)
        {
            // STRING: TH07 0x004967d4
            _mkdir("replay");
            for (vmIdx = 0; vmIdx < 15; vmIdx++)
            {
                sprintf(replayPath, "./replay/th7_%.2d.rpy", vmIdx + 1);
                replayFile = (ReplayFile *)FileSystem::OpenFile(replayPath, 1);
                if (!replayFile)
                {
                    continue;
                }

                replayFile = ReplayManager::ValidateReplayData(replayFile, g_LastFileSize);
                if (replayFile)
                {
                    this->replays[vmIdx] = *replayFile;
                    free(replayFile);
                }
            }
        }
        if (this->frameTimer < 20)
        {
            return ZUN_SUCCESS;
        }

        MoveCursor(this, 15);
        this->chosenReplayIdx = this->cursor;
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
            this->chosenReplayIdx = this->cursor;
            this->frameTimer = 0;
            GetDate(this->defaultReplay.data.date);
            this->defaultReplay.data.score = g_GameManager.globals->score;
            if (*(i32 *)&this->replays[this->cursor].head.magic !=
                    *(i32 *)&"T7RP" ||
                (this->replays[this->cursor].head.version & 0xfff) != 256)
            {
                vm = this->vms;
                for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
                {
                    vm->pendingInterrupt = 17;
                }
                vm = &this->vms[this->chosenReplayIdx + 25];
                vm->pendingInterrupt = 16;
                this->resultScreenState = 14;
            }
            else
            {
                vm = this->vms;
                for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
                {
                    vm->pendingInterrupt = 13;
                }
                vm = &this->vms[this->chosenReplayIdx + 25];
                vm->pendingInterrupt = 16;
                this->resultScreenState = 15;
            }
            this->cursor = 0;
            this->selectedChar = 0;
            if (this->isClearingReplayName)
            {
                this->selectedChar = 95;
            }
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->resultScreenState = 11;
            vm = this->vms;
            for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
            {
                vm->pendingInterrupt = 2;
            }
            this->frameTimer = 0;
        }
        break;
    case 14:
        if (this->frameTimer < 30)
        {
            return ZUN_SUCCESS;
        }
        if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_UP))
        {
        WEIRD_ASS_LOOP_WITH_GOTO:
            this->selectedChar = this->selectedChar - 16;
            if (this->selectedChar < 0)
            {
                this->selectedChar = this->selectedChar + 96;
            }
            if (g_AlphabetList[this->selectedChar] == ' ')
            {
                goto WEIRD_ASS_LOOP_WITH_GOTO;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        }
        if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_DOWN))
        {
        WEIRD_ASS_LOOP_WITH_GOTO_2:
            this->selectedChar = this->selectedChar + 16;
            if (this->selectedChar >= 96)
            {
                this->selectedChar = this->selectedChar - 96;
            }
            if (g_AlphabetList[this->selectedChar] == ' ')
            {
                goto WEIRD_ASS_LOOP_WITH_GOTO_2;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        }
        if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_LEFT))
        {
        WEIRD_ASS_LOOP_WITH_GOTO_3:
            this->selectedChar--;
            if (this->selectedChar % 16 == 15)
            {
                this->selectedChar = this->selectedChar + 16;
            }
            if (this->selectedChar < 0)
            {
                this->selectedChar = 15;
            }
            if (g_AlphabetList[this->selectedChar] == ' ')
            {
                goto WEIRD_ASS_LOOP_WITH_GOTO_3;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        }
        if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_RIGHT))
        {
        WEIRD_ASS_LOOP_WITH_GOTO_4:
            this->selectedChar = this->selectedChar + 1;
            if (this->selectedChar % 16 == 0)
            {
                this->selectedChar = this->selectedChar - 16;
            }
            if (g_AlphabetList[this->selectedChar] == ' ')
            {
                goto WEIRD_ASS_LOOP_WITH_GOTO_4;
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_MOVE_MENU, 0);
        }
        if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_SELECTMENU))
        {
            cursor = this->cursor >= 8 ? 7 : this->cursor;
            if (this->selectedChar < 94)
            {
                this->replayName[cursor] = g_AlphabetList[this->selectedChar];
            }
            else if (this->selectedChar == 94)
            {
                this->replayName[cursor] = ' ';
            }
            else
            {
                sprintf(replayPath2, "./replay/th7_%.2d.rpy",
                        this->chosenReplayIdx + 1);
                ReplayManager::SaveReplay(replayPath2, this->replayName);
                this->frameTimer = 0;
                this->resultScreenState = 2;
                vm = this->vms;
                for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
                {
                    vm->pendingInterrupt = 2;
                }
                strcpy(this->lsnmHeader.name, this->replayName);
            }
            if (this->cursor < 8)
            {
                this->cursor++;
                if (this->cursor == 8)
                {
                    this->selectedChar = 95;
                }
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_SELECT, 0);
        }
        if (WAS_PRESSED_RAW_AND_IS_EIGHTH(TH_BUTTON_RETURNMENU))
        {
            cursor2 = this->cursor >= 8 ? 7 : this->cursor;
            if (this->cursor > 0)
            {
                this->cursor--;
                this->replayName[cursor2] = ' ';
                this->replayName[this->cursor] = ' ';
            }
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_MENU))
        {
            goto LAB_004473e3;
        }
        break;
    case 15:
        vm = this->vms + 19;
        if (this->cursor == 0)
        {
            vm[0].color.color =
                (vm[0].color.color & 0xff000000) | 0xff6060;
            vm[1].color.color =
                (vm[1].color.color & 0xff000000) | 0x606060;
        }
        else
        {
            vm[0].color.color =
                (vm[0].color.color & 0xff000000) | 0x606060;
            vm[1].color.color =
                (vm[1].color.color & 0xff000000) | 0xff6060;
        }
        if (this->frameTimer < 20)
        {
            return ZUN_SUCCESS;
        }
        MoveCursorHorizontally(this, 2);
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU) ||
            WAS_PRESSED_RAW(TH_BUTTON_MENU))
        {
            goto LAB_004473e3;
        }
        else if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            this->frameTimer = 0;
            if (this->cursor == 0)
            {
                vm = this->vms;
                for (vmIdx = 0; vmIdx < 41; vmIdx++, vm++)
                {
                    vm->pendingInterrupt = 17;
                }
                vm = &this->vms[chosenReplayIdx + 25];
                vm->pendingInterrupt = 16;
                this->resultScreenState = 14;
            }
            else
            {
                goto LAB_004473e3;
            }
        }
        break;
    }

    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00447fd0
ZunResult ResultScreen::CheckConfirmButton()
{
    AnmVm *viewport;

    switch (this->resultScreenState)
    {
    case 16:
        if (this->frameTimer <= 30)
        {
            viewport = &this->vms[40];
            viewport->pendingInterrupt = 18;
        }
        if (this->frameTimer >= 90 && WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            viewport = &this->vms[40];
            viewport->pendingInterrupt = 2;
            this->frameTimer = 0;
            this->resultScreenState = 17;
        }
        break;
    case 17:
        if (this->frameTimer >= 30)
        {
            this->frameTimer = 59;
            this->resultScreenState = 11;
        }
        break;
    }
    return ZUN_SUCCESS;
}

i32 ResultScreen::DrawStats()
{
    AnmVm *vm;
    Float3 pos;

    switch (this->resultScreenState)
    {
    case 20:
        if (this->frameTimer == 1)
        {
            pos.x = 56.0f;
            pos.y = 128.0f;
            pos.z = 0.0f;
            vm = this->spellcardListVms;
            vm->pos = pos;
            g_Supervisor.UpdateStartupTime();
            AnmManager::DrawVmTextFmt(
                g_AnmManager, vm, 0xffffff, 0,
                "総起動時間   %.2d:%.2d:%.2d", g_GameManager.plst.totalHours,
                g_GameManager.plst.totalMinutes, g_GameManager.plst.totalSeconds);
            g_Supervisor.UpdateStartupTime();
            this->lastTotalSeconds = g_GameManager.plst.totalSeconds;

            vm++;
            pos.y += 17.0f;
            vm->pos = pos;
            AnmManager::DrawVmTextFmt(
                g_AnmManager, vm, 0xffffff, 0,
                "総プレイ時間 %.2d:%.2d:%.2d", g_GameManager.plst.gameHours,
                g_GameManager.plst.gameMinutes, g_GameManager.plst.gameSeconds);

            vm++;
            pos.y += 17.0f;
            vm->pos = pos;
            if (g_GameManager.HasUnlockedPhantomAndMaxClears())
            {
                AnmManager::DrawVmTextFmt(
                    g_AnmManager, vm, 0xffffff, 0,
                    "プレイ回数　　　 　Easy 　Norm 　Hard 　Luna  Extra Phants  Total");
            }
            else
            {
                AnmManager::DrawVmTextFmt(
                    g_AnmManager, vm, 0xffffff, 0,
                    "プレイ回数　　　 　Easy 　Norm 　Hard 　Luna  Extra  Total");
            }

            for (i32 i = 0; i < 6; i++)
            {
                vm++;
                pos.y += 17.0f;
                vm->pos = pos;
                if (g_GameManager.HasUnlockedPhantomAndMaxClears())
                {
                    AnmManager::DrawVmTextFmt(
                        g_AnmManager, vm, 0xffffff, 0, "%s %6d %6d %6d %6d %6d %6d %6d",
                        g_CharacterList[i],
                        g_GameManager.plst.playDataByDifficulty[0].playCountPerShotType[i],
                        g_GameManager.plst.playDataByDifficulty[1].playCountPerShotType[i],
                        g_GameManager.plst.playDataByDifficulty[2].playCountPerShotType[i],
                        g_GameManager.plst.playDataByDifficulty[3].playCountPerShotType[i],
                        g_GameManager.plst.playDataByDifficulty[4].playCountPerShotType[i],
                        g_GameManager.plst.playDataByDifficulty[5].playCountPerShotType[i],
                        g_GameManager.plst.playDataByDifficulty[6].playCountPerShotType[i]);
                }
                else
                {
                    AnmManager::DrawVmTextFmt(
                        g_AnmManager, vm, 0xffffff, 0, "%s %6d %6d %6d %6d %6d %6d",
                        g_CharacterList[i],
                        g_GameManager.plst.playDataByDifficulty[0].playCountPerShotType[i],
                        g_GameManager.plst.playDataByDifficulty[1].playCountPerShotType[i],
                        g_GameManager.plst.playDataByDifficulty[2].playCountPerShotType[i],
                        g_GameManager.plst.playDataByDifficulty[3].playCountPerShotType[i],
                        g_GameManager.plst.playDataByDifficulty[4].playCountPerShotType[i],
                        g_GameManager.plst.playDataByDifficulty[6].playCountPerShotType[i]);
                }
            }

            vm++;
            pos.y += 17.0f;
            vm->pos = pos;
            if (g_GameManager.HasUnlockedPhantomAndMaxClears())
            {
                AnmManager::DrawVmTextFmt(
                    g_AnmManager, vm, 0xffffff, 0, "%s %6d %6d %6d %6d %6d %6d %6d",
                    g_TotalForAllProtagonists,
                    g_GameManager.plst.playDataByDifficulty[0].playCount,
                    g_GameManager.plst.playDataByDifficulty[1].playCount,
                    g_GameManager.plst.playDataByDifficulty[2].playCount,
                    g_GameManager.plst.playDataByDifficulty[3].playCount,
                    g_GameManager.plst.playDataByDifficulty[4].playCount,
                    g_GameManager.plst.playDataByDifficulty[5].playCount,
                    g_GameManager.plst.playDataByDifficulty[6].playCount);
            }
            else
            {
                AnmManager::DrawVmTextFmt(
                    g_AnmManager, vm, 0xffffff, 0, "%s %6d %6d %6d %6d %6d %6d",
                    g_TotalForAllProtagonists,
                    g_GameManager.plst.playDataByDifficulty[0].playCount,
                    g_GameManager.plst.playDataByDifficulty[1].playCount,
                    g_GameManager.plst.playDataByDifficulty[2].playCount,
                    g_GameManager.plst.playDataByDifficulty[3].playCount,
                    g_GameManager.plst.playDataByDifficulty[4].playCount,
                    g_GameManager.plst.playDataByDifficulty[6].playCount);
            }

            vm++;
            pos.y += 34.0f;
            vm->pos = pos;
            g_GameManager.plst.playDataByDifficulty[6].noContinueClearCount =
                g_GameManager.plst.playDataByDifficulty[0].noContinueClearCount +
                g_GameManager.plst.playDataByDifficulty[1].noContinueClearCount +
                g_GameManager.plst.playDataByDifficulty[2].noContinueClearCount +
                g_GameManager.plst.playDataByDifficulty[3].noContinueClearCount +
                g_GameManager.plst.playDataByDifficulty[4].noContinueClearCount +
                g_GameManager.plst.playDataByDifficulty[5].noContinueClearCount;

            if (g_GameManager.HasUnlockedPhantomAndMaxClears())
            {
                AnmManager::DrawVmTextFmt(
                    g_AnmManager, vm, 0xffffff, 0,
                    "クリア回数  　　 %6d %6d %6d %6d %6d %6d %6d",
                    g_GameManager.plst.playDataByDifficulty[0].noContinueClearCount,
                    g_GameManager.plst.playDataByDifficulty[1].noContinueClearCount,
                    g_GameManager.plst.playDataByDifficulty[2].noContinueClearCount,
                    g_GameManager.plst.playDataByDifficulty[3].noContinueClearCount,
                    g_GameManager.plst.playDataByDifficulty[4].noContinueClearCount,
                    g_GameManager.plst.playDataByDifficulty[5].noContinueClearCount,
                    g_GameManager.plst.playDataByDifficulty[6].noContinueClearCount);
            }
            else
            {
                AnmManager::DrawVmTextFmt(
                    g_AnmManager, vm, 0xffffff, 0,
                    "クリア回数  　　 %6d %6d %6d %6d %6d %6d",
                    g_GameManager.plst.playDataByDifficulty[0].noContinueClearCount,
                    g_GameManager.plst.playDataByDifficulty[1].noContinueClearCount,
                    g_GameManager.plst.playDataByDifficulty[2].noContinueClearCount,
                    g_GameManager.plst.playDataByDifficulty[3].noContinueClearCount,
                    g_GameManager.plst.playDataByDifficulty[4].noContinueClearCount,
                    g_GameManager.plst.playDataByDifficulty[6].noContinueClearCount);
            }

            vm++;
            pos.y += 17.0f;
            vm->pos = pos;
            if (g_GameManager.HasUnlockedPhantomAndMaxClears())
            {
                AnmManager::DrawVmTextFmt(
                    g_AnmManager, vm, 0xffffff, 0,
                    "コンティニュー   %6d %6d %6d %6d %6d %6d %6d",
                    g_GameManager.plst.playDataByDifficulty[0].retryCount,
                    g_GameManager.plst.playDataByDifficulty[1].retryCount,
                    g_GameManager.plst.playDataByDifficulty[2].retryCount,
                    g_GameManager.plst.playDataByDifficulty[3].retryCount,
                    g_GameManager.plst.playDataByDifficulty[4].retryCount,
                    g_GameManager.plst.playDataByDifficulty[5].retryCount,
                    g_GameManager.plst.playDataByDifficulty[6].retryCount);
            }
            else
            {
                AnmManager::DrawVmTextFmt(
                    g_AnmManager, vm, 0xffffff, 0,
                    "コンティニュー   %6d %6d %6d %6d %6d %6d",
                    g_GameManager.plst.playDataByDifficulty[0].retryCount,
                    g_GameManager.plst.playDataByDifficulty[1].retryCount,
                    g_GameManager.plst.playDataByDifficulty[2].retryCount,
                    g_GameManager.plst.playDataByDifficulty[3].retryCount,
                    g_GameManager.plst.playDataByDifficulty[4].retryCount,
                    g_GameManager.plst.playDataByDifficulty[6].retryCount);
            }

            vm++;
            pos.y += 17.0f;
            vm->pos = pos;
            if (g_GameManager.HasUnlockedPhantomAndMaxClears())
            {
                AnmManager::DrawVmTextFmt(
                    g_AnmManager, vm, 0xffffff, 0,
                    "プラクティス　   %6d %6d %6d %6d %6d %6d %6d",
                    g_GameManager.plst.playDataByDifficulty[0].extraClearCount,
                    g_GameManager.plst.playDataByDifficulty[1].extraClearCount,
                    g_GameManager.plst.playDataByDifficulty[2].extraClearCount,
                    g_GameManager.plst.playDataByDifficulty[3].extraClearCount,
                    g_GameManager.plst.playDataByDifficulty[4].extraClearCount,
                    g_GameManager.plst.playDataByDifficulty[5].extraClearCount,
                    g_GameManager.plst.playDataByDifficulty[6].extraClearCount);
            }
            else
            {
                AnmManager::DrawVmTextFmt(
                    g_AnmManager, vm, 0xffffff, 0,
                    "プラクティス　   %6d %6d %6d %6d %6d %6d",
                    g_GameManager.plst.playDataByDifficulty[0].extraClearCount,
                    g_GameManager.plst.playDataByDifficulty[1].extraClearCount,
                    g_GameManager.plst.playDataByDifficulty[2].extraClearCount,
                    g_GameManager.plst.playDataByDifficulty[3].extraClearCount,
                    g_GameManager.plst.playDataByDifficulty[4].extraClearCount,
                    g_GameManager.plst.playDataByDifficulty[6].extraClearCount);
            }

            vm++;
            pos.y += 17.0f;
            vm->pos = pos;
            if (g_GameManager.HasUnlockedPhantomAndMaxClears())
            {
                AnmManager::DrawVmTextFmt(
                    g_AnmManager, vm, 0xffffff, 0,
                    "リトライ回数  　 %6d %6d %6d %6d %6d %6d %6d",
                    g_GameManager.plst.playDataByDifficulty[0].clearCount,
                    g_GameManager.plst.playDataByDifficulty[1].clearCount,
                    g_GameManager.plst.playDataByDifficulty[2].clearCount,
                    g_GameManager.plst.playDataByDifficulty[3].clearCount,
                    g_GameManager.plst.playDataByDifficulty[4].clearCount,
                    g_GameManager.plst.playDataByDifficulty[5].clearCount,
                    g_GameManager.plst.playDataByDifficulty[6].clearCount);
            }
            else
            {
                AnmManager::DrawVmTextFmt(
                    g_AnmManager, vm, 0xffffff, 0,
                    "リトライ回数  　 %6d %6d %6d %6d %6d %6d",
                    g_GameManager.plst.playDataByDifficulty[0].clearCount,
                    g_GameManager.plst.playDataByDifficulty[1].clearCount,
                    g_GameManager.plst.playDataByDifficulty[2].clearCount,
                    g_GameManager.plst.playDataByDifficulty[3].clearCount,
                    g_GameManager.plst.playDataByDifficulty[4].clearCount,
                    g_GameManager.plst.playDataByDifficulty[6].clearCount);
            }
        }

        if (this->frameTimer < 40)
        {
            vm = this->spellcardListVms;
            for (i32 i = 0; i < 14; i++, vm++)
            {
                vm->color.bytes.a = this->frameTimer * 255 / 40;
            }
        }
        else
        {
            this->resultScreenState = 21;
        }
        break;

    case 21:
        if (this->frameTimer % 60 == 0 &&
            (g_Supervisor.UpdateStartupTime(),
             g_GameManager.plst.totalSeconds != this->lastTotalSeconds))
        {
            vm = this->spellcardListVms;
            AnmManager::DrawVmTextFmt(
                g_AnmManager, vm, 0xffffff, 0,
                "総起動時間   %.2d:%.2d:%.2d", g_GameManager.plst.totalHours,
                g_GameManager.plst.totalMinutes, g_GameManager.plst.totalSeconds);
            this->lastTotalSeconds = g_GameManager.plst.totalSeconds;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SHOOT | TH_BUTTON_BOMB |
                            TH_BUTTON_MENU | TH_BUTTON_ENTER))
        {
            this->resultScreenState = 22;
            this->frameTimer = 0;
        }
        break;

    case 22:
        if (this->frameTimer < 20)
        {
            vm = this->spellcardListVms;
            for (i32 i = 0; i < 14; i++, vm++)
            {
                vm->color.bytes.a = 255 - this->frameTimer * 255 / 20;
            }
            break;
        }
        this->resultScreenState = 0;
        this->frameTimer = 0;
        return 1;
    }

    return 0;
}

#pragma var_order(vm, pos, rankingProbably, clearPercent, slowdown, color)
// FUNCTION: TH07 0x004488a9
ZunResult ResultScreen::DrawFinalStats()
{
    AnmVm *vm;
    Float3 pos;
    f32 rankingProbably;
    f32 clearPercent;
    f32 slowdown;
    D3DCOLOR color;

    switch (this->resultScreenState)
    {
    case 16:
    case 17:
        vm = &this->vms[40];
        color = vm->color.color;
        g_AsciiManager.color = color;
        rankingProbably = 0.0f;

        // Please do not write code like this
        clearPercent = g_GameManager.difficulty < DIFF_EXTRA
                           ? (f32)g_GameManager.playTimeAll / 180621.0f
                           : clearPercent = g_GameManager.difficulty == DIFF_EXTRA
                                                ? (f32)g_GameManager.playTimeAll / 80000.0f
                                                : (f32)g_GameManager.playTimeAll / 85000.0f;

        pos = vm->pos;
        pos.x += 210.0f;
        pos.y += 32.0f;

        AsciiManager::AddFormatText(&g_AsciiManager, &pos, "%9d",
                                    g_GameManager.globals->guiScore);

        pos.x += 126.0f;
        AsciiManager::AddFormatText(&g_AsciiManager, &pos, "%1d",
                                    g_GameManager.globals->numRetries);
        pos.x -= 126.0f;

        if (g_GameManager.globals->guiScore < 2000000)
        {
            rankingProbably -= 20.0f;
        }
        else if (g_GameManager.globals->guiScore < 200000000)
        {
            rankingProbably += (f32)(u32)(g_GameManager.globals->guiScore - 2000000) / 198000000.0f * 60.0f - 20.0f;
        }
        else
        {
            rankingProbably += 40.0f;
        }

        pos.y += 22.0f;
        g_AsciiManager.AddString(&pos,
                                 g_DifficultyNameTable[g_GameManager.difficulty]);

        rankingProbably += g_DifficultyWeightsList[g_GameManager.difficulty];

        pos.x += 14.0f;
        pos.y += 22.0f;

        if (!g_GameManager.finished)
        {
            if (clearPercent >= 1.0f)
            {
                clearPercent = 0.99f;
            }
            AsciiManager::AddFormatText(&g_AsciiManager, &pos, "    %3.2f%%",
                                        (f64)(clearPercent * 100.0f));
            rankingProbably += clearPercent * 70.0f;
        }
        else
        {
            AsciiManager::AddFormatText(&g_AsciiManager, &pos, "      100%%");
            rankingProbably += 70.0f;
        }

        pos.y += 22.0f;
        AsciiManager::AddFormatText(&g_AsciiManager, &pos, "%9d",
                                    g_GameManager.globals->numRetries);
        rankingProbably -= (f32)g_GameManager.globals->numRetries * 10.0f;

        pos.y += 22.0f;
        AsciiManager::AddFormatText(&g_AsciiManager, &pos, "%9d",
                                    (i32)g_GameManager.globals->deaths);
        rankingProbably -= (f32)(i32)g_GameManager.globals->deaths * 5.0f - 10.0f;

        pos.y += 22.0f;
        AsciiManager::AddFormatText(&g_AsciiManager, &pos, "%9d",
                                    (i32)g_GameManager.globals->bombsUsed);
        rankingProbably -= (f32)(i32)g_GameManager.globals->bombsUsed * 2.0f - 10.0f;

        pos.y += 22.0f;
        AsciiManager::AddFormatText(&g_AsciiManager, &pos, "%9d",
                                    g_GameManager.globals->spellCardsCaptured);
        rankingProbably += (f32)g_GameManager.globals->spellCardsCaptured *
                           g_DifficultySpellcardWeightsList[g_GameManager.difficulty];

        slowdown =
            (g_Supervisor.framerateMultiplier / g_Supervisor.fpsAccumulator - 0.5f) * 2;

        if (slowdown < 0.0f)
        {
            slowdown = 0.0f;
        }
        else if (slowdown >= 1.0f)
        {
            slowdown = 1.0f;
        }

        slowdown = (1.0f - slowdown) * 100.0f;

        pos.y += 22.0f;
        AsciiManager::AddFormatText(&g_AsciiManager, &pos, "    %3.2f%%", slowdown);

        if (slowdown < 50.0f)
        {
            rankingProbably -= 70.0f * slowdown / 100.0f;
        }
        else
        {
            rankingProbably = -999.0f;
        }

        if (g_GameManager.globals->pointItemsCollectedForExtend < 800)
        {
            rankingProbably += 0.01f * g_GameManager.globals->pointItemsCollectedForExtend;
        }
        else
        {
            rankingProbably += 8.0f;
        }

        if (g_GameManager.globals->grazeInTotal < 5000)
        {
            rankingProbably += 0.0025f * g_GameManager.globals->grazeInTotal;
        }
        else
        {
            rankingProbably += 12.5f;
        }

        g_AsciiManager.color = 0xffffffff;
        break;
    }
    return ZUN_SUCCESS;
}

#pragma var_order(charPos, i, name, vm, node, j, \
                  pos, oldX, spellcardIdx, offsetX, offsetY, charBuf)
// FUNCTION: TH07 0x00448d40
u32 ResultScreen::OnDraw(ResultScreen *arg)
{
    char charBuf[16];
    f32 offsetY;
    f32 offsetX;
    i32 spellcardIdx;
    f32 oldX;
    Float3 pos;
    i32 j;
    ScoreListNode *node;
    AnmVm *vm;
    char name[9];
    i32 i;
    Float3 charPos;

    vm = arg->vms;
    g_AnmManager->Flush();
    g_Supervisor.viewport.X = 0;
    g_Supervisor.viewport.Y = 0;
    g_Supervisor.viewport.Width = 640;
    g_Supervisor.viewport.Height = 480;
    g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
    for (i = 0; i < 41; i++, vm++)
    {
        pos = vm->pos;
        vm->pos += vm->offset;
        g_AnmManager->DrawNoRotation(vm);
        vm->pos = pos;
    }
    vm = arg->vms + 16;
    if (vm->pos.x < 640.0f)
    {
        if (arg->stateStep != 9)
        {
            pos = vm->pos;
            arg->spellcardListVms[0].pos = pos;
            arg->spellcardListVms[0].pos.x += 64.0f;
            g_AnmManager->DrawNoRotation(arg->spellcardListVms);
            pos[1] += 18.0f;
            pos[0] += 24.0f;
            g_AsciiManager.color = 0xffe0e0ef;
            AsciiManager::AddFormatText(&g_AsciiManager, &pos,
                                        // STRING: TH07 0x004964b4
                                        "No  Name      Score(Stage)  Date   Slow");
            pos[1] += 18.0f;
            node = arg->scoreLists[arg->diffPlayed][arg->charUsed].next;
            for (i = 0; i < 10; i++)
            {
                if (arg->resultScreenState == 10)
                {
                    if (node->data->isPlayerScore)
                    {
                        g_AsciiManager.color = 0xfff0f0ff;
                    }
                    else
                    {
                        g_AsciiManager.color = 0xc0ffc0c0;
                    }
                }
                else
                {
                    g_AsciiManager.color = 0xffffc0c0;
                }
                AsciiManager::AddFormatText(&g_AsciiManager, &pos, "%2d",
                                            i + 1);
                pos.x += 48.0f;
                if (arg->resultScreenState == 10 &&
                    node->data->isPlayerScore)
                {
                    // ZUN quirk: VIRGIN strcpy vs CHAD whatever tf this is
                    *(u32 *)&name[0] = *(u32 *)"    ";
                    *(u32 *)&name[4] = *(u32 *)"    ";
                    name[8] = '\0';
                    name[arg->cursor >= 8 ? 7 : arg->cursor] = '_';
                    // STRING: TH07 0x004964a8
                    AsciiManager::AddFormatText(&g_AsciiManager, &pos, "%8s", name);
                }
                if (node->data->stage <= 6)
                {
                    AsciiManager::AddFormatText(
                        // STRING: TH07 0x00496498
                        &g_AsciiManager, &pos, "%8s %9d%1d(%d)",
                        node->data->name, node->data->score,
                        (i32)node->data->numRetries, (u32)node->data->stage);
                }
                else if (node->data->stage == 7 ||
                         node->data->stage == 8)
                {
                    AsciiManager::AddFormatText(
                        // STRING: TH07 0x00496488
                        &g_AsciiManager, &pos, "%8s %9d%1d(1)", node->data->name,
                        node->data->score, (i32)node->data->numRetries);
                }
                else
                {
                    AsciiManager::AddFormatText(
                        // STRING: TH07 0x00496478
                        &g_AsciiManager, &pos, "%8s %9d%1d(C)", node->data->name,
                        node->data->score, (i32)node->data->numRetries);
                }
                pos.x += 320.0f;
                // STRING: TH07 0x00496468
                AsciiManager::AddFormatText(&g_AsciiManager, &pos, " %5s   %3.2f",
                                            node->data->date, node->data->slowRatePercent);
                pos[1] += 18.0f;
                pos[0] -= 368.0f;
                node = node->next;
            }
        }
        else
        {
            pos = vm->pos;
            arg->spellcardListVms[10].pos = pos;
            g_AnmManager->DrawNoRotation(arg->spellcardListVms + 10);
            pos[1] += 16.0f;
            for (i = 0; i < 10; i++)
            {
                spellcardIdx = arg->lastSpellcardSelected * 10 + i;
                if (spellcardIdx >= 141)
                {
                    break;
                }
                oldX = pos.x;
                pos[0] += 320.0f;
                pos[1] += 16.0f;
                arg->rightArrowVm.pos = pos;
                arg->rightArrowVm.scale.x = 2.375f;
                g_AnmManager->DrawNoRotation(&arg->rightArrowVm);
                pos[1] -= 16.0f;
                pos.x = oldX;
                arg->spellcardListVms[i].pos = pos;
                if (g_GameManager.catk[spellcardIdx]
                        .numAttemptsPerShot[arg->prevSpellcardListPage] == 0)
                {
                    g_AsciiManager.color = 0xc0c0c0ff;
                }
                else if (g_GameManager.catk[spellcardIdx]
                             .numSuccessesPerShot[arg->prevSpellcardListPage] == 0)
                {
                    g_AsciiManager.color = 0xffc0a0a0;
                }
                else
                {
                    g_AsciiManager.color = 0xfff0f0ff - i * 0x80800;
                }
                AsciiManager::AddFormatText(&g_AsciiManager, &pos, "No.%.2d",
                                            spellcardIdx + 1);
                arg->spellcardListVms[i].pos[0] += 96.0f;
                g_AnmManager->DrawNoRotation(arg->spellcardListVms + i);
                pos[0] += 496.0f;
                if (g_GameManager.catk[spellcardIdx]
                        .numAttemptsPerShot[arg->prevSpellcardListPage] == 0)
                {
                    // STRING: TH07 0x00496458
                    AsciiManager::AddFormatText(&g_AsciiManager, &pos, "---/---",
                                                g_GameManager.catk[spellcardIdx].numSuccessesPerShot[arg->prevSpellcardListPage],
                                                g_GameManager.catk[spellcardIdx].numAttemptsPerShot[arg->prevSpellcardListPage]);
                }
                else
                {
                    AsciiManager::AddFormatText(
                        &g_AsciiManager, &pos, "%3d/%3d",
                        g_GameManager.catk[spellcardIdx]
                            .numSuccessesPerShot[arg->prevSpellcardListPage],
                        g_GameManager.catk[spellcardIdx]
                            .numAttemptsPerShot[arg->prevSpellcardListPage]);
                }
                pos[0] -= 496.0f;
                pos[0] += 424.0f;
                pos[1] -= 13.0f;
                g_AsciiManager.color = 0xffa08090;
                g_AsciiManager.scale.x = 0.8f;
                g_AsciiManager.scale.y = 0.8f;
                if (g_GameManager.catk[spellcardIdx]
                        .numAttemptsPerShot[arg->prevSpellcardListPage] != 0)
                {
                    AsciiManager::AddFormatText(
                        // STRING: TH07 0x00496440
                        &g_AsciiManager, &pos, "MaxBonus %8d",
                        g_GameManager.catk[spellcardIdx]
                            .highScorePerShot[arg->prevSpellcardListPage]);
                }
                pos[0] -= 424.0f;
                pos[1] += 13.0f;
                g_AsciiManager.scale.x = 1.0f;
                g_AsciiManager.scale.y = 1.0f;
                if (arg->listScrollAnimState == 0)
                {
                    pos[1] += 33.0f;
                }
                else if (arg->frameTimer < 20)
                {
                    pos[1] +=
                        (f32)((20 - arg->frameTimer) * 33 / 20);
                }
                else
                {
                    pos[1] +=
                        (f32)((arg->frameTimer - 20) * 33 / 20);
                }
            }
            if (arg->frameTimer >= 40)
            {
                arg->listScrollAnimState = 0;
            }
        }
    }
    if (arg->resultScreenState == 10 || arg->resultScreenState == 14)
    {
        pos = Float3(160.0f, 356.0f, 0.0f);
        for (i = 0; i < 6; i++)
        {
            for (j = 0; j < 16; j++)
            {
                offsetX = 0.0f;
                offsetY = 0.0f;
                if (arg->selectedChar == i * 16 + j)
                {
                    g_AsciiManager.color = 0xffffffc0;
                    if (arg->frameTimer % 64 < 32)
                    {
                        offsetX = (f32)(arg->frameTimer % 32) * 0.8f / 32.0f + 1.2f;
                    }
                    else
                    {
                        offsetX = 2.0f - (f32)(arg->frameTimer % 32) * 0.8f / 32.0f;
                    }
                    g_AsciiManager.scale.x = offsetX;
                    g_AsciiManager.scale.y = offsetX;
                    offsetX = -(offsetX - 1.0f) * 8.0f;
                    offsetY = offsetX;
                }
                else
                {
                    g_AsciiManager.color = 0xc0c0c0c0;
                    g_AsciiManager.scale.x = 1.0f;
                    g_AsciiManager.scale.y = 1.0f;
                }
                charPos = pos;
                charPos.x += offsetX;
                charPos.y += offsetY;
                charBuf[0] = g_AlphabetList[i * 16 + j];
                charBuf[1] = 0;
                if (i == 5)
                {
                    if (j == 14)
                    {
                        charBuf[0] = (char)128;
                    }
                    else if (j == 15)
                    {
                        charBuf[0] = (char)0x81;
                    }
                }
                g_AsciiManager.AddString(&charPos, charBuf);
                pos[0] += 20.0f;
            }
            pos[0] -= (f32)(j * 20);
            pos[1] += 18.0f;
        }
    }
    g_AsciiManager.scale.x = 1.0f;
    g_AsciiManager.scale.y = 1.0f;
    if (arg->resultScreenState >= 11 && arg->resultScreenState <= 15)
    {
        vm = &arg->vms[18];
        for (i = 0; i < 6; i++, vm++)
        {
            g_AnmManager->DrawNoRotation(vm);
        }
        vm = &arg->vms[24];
        pos = vm->pos;
        vm++;
        AsciiManager::AddFormatText(&g_AsciiManager, &pos,
                                    // STRING: TH07 0x0049641c
                                    "No.   Name     Date   Player Score");
        for (i = 0; i < 15; i++)
        {
            pos = vm->pos;
            vm++;
            if (i == arg->chosenReplayIdx)
            {
                g_AsciiManager.color = 0xffff8080;
            }
            else
            {
                g_AsciiManager.color = 0xff808080;
            }
            if (arg->resultScreenState == 14)
            {
                AsciiManager::AddFormatText(
                    // STRING: TH07 0x00496400
                    &g_AsciiManager, &pos, "No.%.2d %8s %5s  %7s %9d0",
                    i + 1, arg->replayName, arg->defaultReplay.data.date,
                    g_CharactersAndShotTypesStrings[(u32)g_GameManager.character * 2 +
                                                    (u32)g_GameManager.shotType],
                    arg->defaultReplay.data.score);
                g_AsciiManager.color = 0xfff0f0ff;
                *(u32 *)&name[0] = *(u32 *)"    ";
                *(u32 *)&name[4] = *(u32 *)"    ";
                name[8] = '\0';
                name[arg->cursor >= 8 ? 7 : arg->cursor] = '_';
                // STRING: TH07 0x004963f4
                AsciiManager::AddFormatText(&g_AsciiManager, &pos, "      %8s",
                                            name);
            }
            else if (*(i32 *)&arg->replays[i].head.magic !=
                         *(i32 *)&"T7RP" ||
                     (arg->replays[i].head.version & 0xfff) != 256)
            {
                AsciiManager::AddFormatText(
                    &g_AsciiManager, &pos,
                    // STRING: TH07 0x004963c8
                    "No.%.2d -------- --/--  -------          0",
                    i + 1);
            }
            else
            {
                AsciiManager::AddFormatText(
                    &g_AsciiManager, &pos, "No.%.2d %8s %5s  %7s %9d0",
                    i + 1, arg->replays[i].data.name,
                    arg->replays[i].data.date,
                    g_CharactersAndShotTypesStrings[arg->replays[i]
                                                        .data.shotType],
                    arg->replays[i].data.score);
            }
        }
    }
    g_AsciiManager.color = 0xffffffff;
    arg->DrawFinalStats();
    if (arg->resultScreenState == 20 || arg->resultScreenState == 21 ||
        arg->resultScreenState == 22)
    {
        vm = arg->spellcardListVms;
        for (i = 0; i < 14; i++, vm++)
        {
            g_AnmManager->DrawNoRotation(vm);
        }
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(i, vm, j, k, catk, catkIdx)
// FUNCTION: TH07 0x00449b05
ZunResult ResultScreen::AddedCallback(ResultScreen *arg)
{
    i32 k;
    i32 j;
    i32 catkIdx;
    Catk *catk;
    AnmVm *vm;
    i32 i;

    g_GameManager.HasUnlockedPhantomAndMaxClears();
    for (i = 0; i < 6; i++)
    {
        for (j = 0; j < 6; j++)
        {
            for (k = 0; k < 10; k++)
            {
                arg->defaultScores[i][j][k].score = 100000 - k * 10000;
                arg->defaultScores[i][j][k].slowRatePercent = 0.0f;
                // STRING: TH07 0x004963c0
                arg->defaultScores[i][j][k].magic = *(u32 *)&"DMYS";
                arg->defaultScores[i][j][k].difficulty = (u8)i;
                arg->defaultScores[i][j][k].version = 1;
                arg->defaultScores[i][j][k].th7kLen2 = sizeof(Hscr);
                arg->defaultScores[i][j][k].th7kLen = sizeof(Hscr);
                arg->defaultScores[i][j][k].stage = 1;
                arg->defaultScores[i][j][k].isPlayerScore = 0;
                arg->defaultScores[i][j][k].numRetries = 0;
                arg->LinkScoreEx(arg->defaultScores[i][j] + k, i, j);
                // STRING: TH07 0x004963b4
                strcpy(arg->defaultScores[i][j][k].name, "--------");
                // STRING: TH07 0x004963ac
                strcpy(arg->defaultScores[i][j][k].date, "--/--");
            }
        }
    }
    if (arg->resultScreenState != 19)
    {
        // STRING: TH07 0x00496394
        if (g_AnmManager->LoadSurface(0, "data/result/result.jpg") != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        // STRING: TH07 0x00496380
        if (g_AnmManager->LoadAnms(ANM_FILE_RESULT, "data/result00.anm", ANM_OFFSET_RESULT) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        vm = arg->vms;
        for (i = 0; i < 41; i++, vm++)
        {
            vm->pos = Float3(0.0f, 0.0f, 0.0f);
            vm->offset = Float3(0.0f, 0.0f, 0.0f);
            g_AnmManager->SetAnmIdxAndExecuteScript(vm, i + 2304);
        }
        UselessStack::FourBytes();
        g_AnmManager->InitializeAndSetActiveSprite(&arg->rightArrowVm, 2320);
        vm = arg->spellcardListVms;
        for (i = 0; i < 15; i++, vm++)
        {
            UselessStack::FourBytes();
            g_AnmManager->InitializeAndSetActiveSprite(vm, i + 1813);
            vm->pos = Float3(0.0f, 0.0f, 0.0f);
            vm->anchor = 3;
            vm->fontWidth = 15;
            vm->fontHeight = 15;
        }
    }
    arg->prevCursor = 0;
    arg->scoreDat = OpenScore("score.dat");
    for (i = 0; i < 6; i++)
    {
        for (j = 0; j < 6; j++)
        {
            GetHighScore(arg->scoreDat, arg->scoreLists[i] + j, j, i, NULL);
        }
    }
    arg->lsnmHeader.magic = LSNM_MAGIC;
    arg->lsnmHeader.version = 1;
    arg->lsnmHeader.th7kLen2 = sizeof(Lsnm);
    arg->lsnmHeader.th7kLen = sizeof(Lsnm);
    // STRING: TH07 0x00496374
    strcpy(arg->lsnmHeader.name, "        ");
    arg->isClearingReplayName = ParseLsnm(arg->scoreDat, &arg->lsnmHeader);
    if (arg->resultScreenState != 10 && arg->resultScreenState != 18)
    {
        ParseCatk(arg->scoreDat, g_GameManager.catk);
        ParseClrd(arg->scoreDat, g_GameManager.clrd);
        g_GameManager.HasUnlockedPhantomAndMaxClears();
        ParsePscr(arg->scoreDat, &g_GameManager.pscr[0][0][0]);
    }
    if (arg->resultScreenState == 18)
    {
        if ((u32)g_GameManager
                .pscr[g_GameManager.character * 2 + g_GameManager.shotType]
                     [g_GameManager.currentStage - 1][g_GameManager.difficulty]
                .score < g_GameManager.globals->score)
        {
            g_GameManager
                .pscr[g_GameManager.character * 2 + g_GameManager.shotType]
                     [g_GameManager.currentStage - 1][g_GameManager.difficulty]
                .score = g_GameManager.globals->score;
        }
        arg->resultScreenState = 11;
        strcpy(arg->replayName, arg->lsnmHeader.name);
    }
    for (i = 0; i < 7; i++)
    {
        catk = g_GameManager.catk;
        arg->totalPlayCountPerCharacter[i] = 0;
        for (catkIdx = 0; catkIdx < 141; catkIdx++, catk++)
        {
            if (catk->magic != CATK_MAGIC || catk->version != 1)
            {
                continue;
            }
            if (catk->numSuccessesPerShot[i] != 0)
            {
                arg->totalPlayCountPerCharacter[i]++;
            }
        }
    }
    arg->spellcardListPage = 6;
    arg->prevSpellcardListPage = 6;
    arg->listScrollAnimState = 0;
    arg->leftArrowVm.activeSpriteIdx = -1;
    if (arg->resultScreenState == 19)
    {
        DeletedCallback(arg);
        return ZUN_ERROR;
    }

    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0044a1f9
ZunResult ResultScreen::DeletedCallback(ResultScreen *arg)
{
    i32 i;
    i32 j;

    if (arg->scoreDat)
    {
        arg->WriteScore();
        ReleaseScoreDat(arg->scoreDat);
    }
    arg->scoreDat = NULL;
    for (i = 0; i < 6; i++)
    {
        for (j = 0; j < 6; j++)
        {
            arg->FreeScore(i, j);
        }
    }
    g_AnmManager->ReleaseAnm(42);
    g_AnmManager->ReleaseAnm(43);
    g_AnmManager->ReleaseAnm(44);
    g_AnmManager->ReleaseAnm(45);
    g_AnmManager->ReleaseSurface(0);
    g_Chain.Cut(arg->drawChain);
    arg->drawChain = NULL;

    delete arg;
    arg = NULL;

    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0044a302
ZunResult ResultScreen::RegisterChain(u32 type)
{
    ResultScreen *resultScreen = new ResultScreen;

    // STRING: TH07 0x0049635c
    Supervisor::DebugPrint2("Stg.PlayTimeAll = %d\r\n",
                            g_GameManager.playTimeAll);
    if (type == 1)
    {
        if (!g_GameManager.practice)
        {
            resultScreen->resultScreenState = 10;
        }
        else
        {
            resultScreen->resultScreenState = 18;
        }
    }
    else if (type == 2)
    {
        resultScreen->resultScreenState = 19;
        AddedCallback(resultScreen);
        return ZUN_SUCCESS;
    }
    resultScreen->calcChain = g_Chain.CreateElem((ChainCallback)OnUpdate);
    resultScreen->calcChain->addedCallback =
        (ChainLifecycleCallback)AddedCallback;
    resultScreen->calcChain->deletedCallback =
        (ChainLifecycleCallback)DeletedCallback;
    resultScreen->calcChain->arg = resultScreen;
    if (g_Chain.AddToCalcChain(resultScreen->calcChain, 14))
    {
        return ZUN_ERROR;
    }

    resultScreen->drawChain = g_Chain.CreateElem((ChainCallback)OnDraw);
    resultScreen->drawChain->arg = resultScreen;
    g_Chain.AddToDrawChain(resultScreen->drawChain, 13);

    return ZUN_SUCCESS;
}
