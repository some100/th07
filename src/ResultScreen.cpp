#include "ResultScreen.hpp"

#include <cstdio>
#include <filesystem>
#include <time.h>

#include "AnmIdx.hpp"
#include "AnmManager.hpp"
#include "AsciiManager.hpp"
#include "Chain.hpp"
#include "Controller.hpp"
#include "FileSystem.hpp"
#include "GameManager.hpp"
#include "GameWindow.hpp"
#include "Rng.hpp"
#include "SoundPlayer.hpp"
#include "Touch.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"
#include "pbg4/Lzss.hpp"

namespace fs = std::filesystem;

static const f32 g_DifficultyWeightsList[] = {-30.0f, -10.0f, 20.0f, 30.0f, 30.0f};

const char *g_AlphabetList = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.,:;_@abcdefghijklmnopqrstuvwxyz+-/"
                             "*=%0123456789#!?'\"$(){}[]<>&\\|~^ --";

const char *g_CharacterList[6] = {
    "博麗 霊夢 (霊)　", "博麗 霊夢 (夢)　", "霧雨 魔理沙 (魔)",
    "霧雨 魔理沙 (恋)", "十六夜 咲夜 (幻)", "十六夜 咲夜 (時)",
};

const char *g_TotalForAllProtagonists = "全主人公合計  　";

const char *g_CharactersAndShotTypesStrings[6] = {
    "ReimuA ", "ReimuB ", "MarisaA", "MarisaB", "SakuyaA", "SakuyaB",
};

static const f32 g_DifficultySpellcardWeightsList[] = {1.0f, 1.5f, 1.5f, 2.0f, 2.5f};

const char *g_DifficultyNameTable[6] = {
    "      Easy", "    Normal", "      Hard", "   Lunatic", "     Extra", "  Phantasm",
};

i32 ResultScreen::LinkScore(ScoreListNode *prevNode, Hscr *hscr)
{
    ScoreListNode *nextNode;
    i32 scoresAmount;

    scoresAmount = 0;
    while (prevNode->next)
    {
        if (prevNode->next->data && prevNode->next->data->score <= hscr->score)
        {
            break;
        }
        prevNode = prevNode->next;
        scoresAmount++;
    }
    nextNode = prevNode->next;
    prevNode->next = (ScoreListNode *)malloc(sizeof(ScoreListNode));
    prevNode->next->prev = prevNode;
    prevNode = prevNode->next;
    prevNode->data = hscr;
    prevNode->next = nextNode;
    return scoresAmount;
}

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

ScoreDat *ResultScreen::OpenScore(const char *path)
{
    ScoreDatRaw *rawData;
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

    Supervisor::DebugPrint("info : score load\r\n");
    rawData = (ScoreDatRaw *)FileSystem::OpenFile(path, 1);
    scoreData = new ScoreDat;
    scoreData->decodedData = NULL;

    if (!rawData)
    {
    RECREATE_SCORE:
        Supervisor::DebugPrint("info : score recreate\r\n");
        SAFE_FREE(rawData);
        SAFE_DELETE(scoreData);
        scoreData = new ScoreDat;
        scoreData->decodedData = NULL;
        scoreData->raw.dataOffset = sizeof(ScoreDatRaw);
        scoreData->raw.fileLength = sizeof(ScoreDatRaw);
        goto INIT_SCORES;
    }

    if (g_LastFileSize < sizeof(ScoreDatRaw))
    {
        Supervisor::DebugPrint("warning : score.dat size is short\r\n");
        delete scoreData;
        goto RECREATE_SCORE;
    }

    remainingData = g_LastFileSize - 2;
    checksum = 0;
    xorValue = 0;
    i = 0;
    idx = (u8 *)rawData + 1;
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

    if (rawData->csum != checksum)
    {
        Supervisor::DebugPrint("warning : score.dat chksum error\r\n");
        goto RECREATE_SCORE;
    }

    if (rawData->dataOffset != sizeof(ScoreDatRaw))
    {
        Supervisor::DebugPrint("warning : header size is mismatch\r\n");
        goto RECREATE_SCORE;
    }

    if (rawData->magic != 11)
    {
        Supervisor::DebugPrint("warning : score.dat version mismatch\r\n");
        goto RECREATE_SCORE;
    }

    scoreData->raw = *rawData;
    scoreData->decodedData = (u8 *)malloc(scoreData->raw.dstLen + sizeof(ScoreDatRaw));
    memcpy(scoreData->decodedData, rawData, sizeof(ScoreDatRaw));
    Lzss::Decompress((u8 *)rawData + sizeof(ScoreDatRaw), scoreData->raw.srcLen,
                     scoreData->decodedData + sizeof(ScoreDatRaw), scoreData->raw.dstLen);
    free(rawData);
    rawData = NULL;

    cursor = scoreData->raw.fileLength;
    isTh7k = false;
    chunk = (Th7k *)(scoreData->decodedData + scoreData->raw.dataOffset);
    cursor -= scoreData->raw.dataOffset;

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
                (void)parsedVrsm;
            }
        }
        if (chunk->th7kLen == 0)
        {
            Supervisor::DebugPrint("warning : score.dat chapter size is ZERO\r\n");
            goto RECREATE_SCORE;
        }
        cursor -= chunk->th7kLen;
        chunk = (Th7k *)((u8 *)chunk + chunk->th7kLen);
    }

    if (!isTh7k || parsedTh7k->version != 1)
    {
        Supervisor::DebugPrint("warning : score.dat version mismatch\r\n");
        goto RECREATE_SCORE;
    }

INIT_SCORES:
    scoreData->scores = new ScoreListNode;
    return scoreData;
}

u32 ResultScreen::GetHighScore(ScoreDat *scoreDat, ScoreListNode *node, u32 character,
                               u32 difficulty, u8 *numRetries)
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

    cursor = sd->raw.fileLength;
    if (!sd->decodedData)
    {
        return 100000;
    }

    parsedHscr = (Hscr *)(sd->decodedData + sd->raw.dataOffset);
    cursor -= sd->raw.dataOffset;
    while (cursor > 0)
    {
        if (parsedHscr->base.magic == HSCR_MAGIC && parsedHscr->base.version == 1 &&
            parsedHscr->character == character && parsedHscr->difficulty == difficulty)
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
        cursor -= parsedHscr->base.th7kLen;
        parsedHscr = (Hscr *)((u8 *)parsedHscr + parsedHscr->base.th7kLen);
    }
    if (numRetries != 0)
    {
        *numRetries = sd->scores->next ? sd->scores->next->data->numRetries : 0;
    }
    return sd->scores->next
               ? sd->scores->next->data->score > 100000 ? sd->scores->next->data->score : 100000
               : 100000;
}

ZunResult ResultScreen::ParseCatk(ScoreDat *scoreDat, Catk *outCatk)
{
    Catk *parsedCatk;
    i32 cursor;
    ScoreDat *sd = scoreDat;

    if (!outCatk)
    {
        return ZUN_ERROR;
    }

    parsedCatk = (Catk *)(sd->decodedData + sd->raw.dataOffset);
    cursor = sd->raw.fileLength - sd->raw.dataOffset;
    while (cursor > 0)
    {
        if (parsedCatk->base.magic == CATK_MAGIC && parsedCatk->base.version == 1)
        {
            if (parsedCatk->idx >= SPELLCARD_COUNT)
            {
                break;
            }
            outCatk[parsedCatk->idx] = *parsedCatk;
        }
        cursor -= parsedCatk->base.th7kLen;
        parsedCatk = (Catk *)((u8 *)parsedCatk + parsedCatk->base.th7kLen);
    }
    return ZUN_SUCCESS;
}

i32 ResultScreen::ParseLsnm(ScoreDat *scoreDat, Lsnm *outLsnm)
{
    i32 cursor;
    Lsnm *parsedLsnm;
    ScoreDat *sd = scoreDat;

    parsedLsnm = (Lsnm *)(sd->decodedData + sd->raw.dataOffset);
    cursor = sd->raw.fileLength - sd->raw.dataOffset;
    while (cursor > 0)
    {
        if (parsedLsnm->base.magic == LSNM_MAGIC && parsedLsnm->base.version == 1)
        {
            *outLsnm = *parsedLsnm;
            return 1;
        }
        cursor -= parsedLsnm->base.th7kLen;
        parsedLsnm = (Lsnm *)((u8 *)parsedLsnm + parsedLsnm->base.th7kLen);
    }
    return 0;
}

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
        outClrd[i].base.magic = CLRD_MAGIC;
        outClrd[i].base.th7kLen2 = sizeof(Clrd);
        outClrd[i].base.th7kLen = sizeof(Clrd);
        outClrd[i].base.version = 1;
        outClrd[i].characterShotType = (u8)i;
        for (j = 0; j < 5; j++)
        {
            outClrd[i].difficultyClearedWithRetries[j] = 1;
            outClrd[i].difficultyClearedWithoutRetries[j] = 1;
        }
    }
    parsedClrd = (Clrd *)(sd->decodedData + sd->raw.dataOffset);
    cursor = sd->raw.fileLength - sd->raw.dataOffset;
    while (cursor > 0)
    {
        if (parsedClrd->base.magic == CLRD_MAGIC && parsedClrd->base.version == 1)
        {
            if (parsedClrd->characterShotType >= 6)
            {
                break;
            }
            outClrd[parsedClrd->characterShotType] = *parsedClrd;
        }
        cursor -= parsedClrd->base.th7kLen;
        parsedClrd = (Clrd *)((u8 *)parsedClrd + parsedClrd->base.th7kLen);
    }
    return ZUN_SUCCESS;
}

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
    for (i = 0; i < SHOT_COUNT; i++)
    {
        for (j = 0; j < 6; j++)
        {
            for (k = 0; k < 4; k++, pscr++)
            {
                memset(pscr, 0, sizeof(Pscr));
                pscr->base.magic = PSCR_MAGIC;
                pscr->base.th7kLen2 = sizeof(Pscr);
                pscr->base.th7kLen = sizeof(Pscr);
                pscr->base.version = 1;
                pscr->character = i;
                pscr->difficulty = k;
                pscr->stage = j;
                pscr->playCount = 0;
            }
        }
    }
    parsedPscr = (Pscr *)(sd->decodedData + sd->raw.dataOffset);
    cursor = sd->raw.fileLength - sd->raw.dataOffset;
    while (cursor > 0)
    {
        if (parsedPscr->base.magic == PSCR_MAGIC && parsedPscr->base.version == 1)
        {
            pscr = parsedPscr;
            if (pscr->character >= 6 || (pscr->difficulty >= 5 || pscr->stage >= 7))
            {
                break;
            }
            outPscr[pscr->character * 6 * 4 + pscr->stage * 4 + pscr->difficulty] = *pscr;
        }
        cursor -= parsedPscr->base.th7kLen;
        parsedPscr = (Pscr *)((u8 *)parsedPscr + parsedPscr->base.th7kLen);
    }
    return ZUN_SUCCESS;
}

ZunResult ResultScreen::ParsePlst(ScoreDat *scoreDat, Plst *outPlst)
{
    i32 cursor;
    Plst *parsedPlst;
    ScoreDat *sd = scoreDat;

    parsedPlst = (Plst *)(sd->decodedData + sd->raw.dataOffset);
    cursor = sd->raw.fileLength - sd->raw.dataOffset;
    while (cursor > 0)
    {
        if (parsedPlst->base.magic == PLST_MAGIC && parsedPlst->base.version == 1)
        {
            *outPlst = *parsedPlst;
        }
        cursor -= parsedPlst->base.th7kLen;
        parsedPlst = (Plst *)((u8 *)parsedPlst + parsedPlst->base.th7kLen);
    }
    return ZUN_SUCCESS;
}

void ResultScreen::ReleaseScoreDat(ScoreDat *scoreDat)
{
// for reasons inexplicable to myself, this makes emscripten die with a memory access oob error.
// meaning that we _have_ to leak this
// Sorry in advance
#ifndef __EMSCRIPTEN__
    if (scoreDat->decodedData)
    {
        free(scoreDat->decodedData);
    }
#endif
    FreeAllScores(scoreDat->scores);
    delete scoreDat->scores;
    delete scoreDat;
}

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

    fileBuffer = (u8 *)malloc(0xa0000);

    ScoreDatRaw rawHead;
    memset(&rawHead, 0, sizeof(rawHead));
    memcpy(fileBuffer + sizeOfFile, &rawHead, sizeof(ScoreDatRaw));
    sizeOfFile += sizeof(ScoreDatRaw);

    this->th7kHeader.magic = TH7K_MAGIC;
    this->th7kHeader.th7kLen2 = sizeof(Th7k);
    this->th7kHeader.th7kLen = sizeof(Th7k);
    this->th7kHeader.version = 1;

    memcpy(fileBuffer + sizeOfFile, &this->th7kHeader, sizeof(Th7k));
    sizeOfFile += sizeof(Th7k);

    for (difficulty = 0; difficulty < DIFF_COUNT; difficulty++)
    {
        for (character = 0; character < SHOT_COUNT; character++)
        {
            currentCharacter = this->scoreLists[difficulty][character].next;
            characterSlot = 0;

            for (;;)
            {
                if (currentCharacter)
                {
                    if (currentCharacter->data->base.magic == HSCR_MAGIC)
                    {
                        currentCharacter->data->character = character;
                        currentCharacter->data->difficulty = difficulty;
                        currentCharacter->data->base.th7kLen2 = sizeof(Hscr);
                        currentCharacter->data->base.th7kLen = sizeof(Hscr);
                        currentCharacter->data->base.version = 1;
                        currentCharacter->data->base.isPlayerScore = 0;

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
        clrd->base.magic = CLRD_MAGIC;
        clrd->base.th7kLen2 = sizeof(Clrd);
        clrd->base.th7kLen = sizeof(Clrd);
        clrd->base.version = 1;

        memcpy(fileBuffer + sizeOfFile, clrd, sizeof(Clrd));
        sizeOfFile += sizeof(Clrd);
    }

    catk = g_GameManager.catk;
    for (difficulty = 0; difficulty < SPELLCARD_COUNT; difficulty++, catk++)
    {
        if (catk->base.magic == CATK_MAGIC)
        {
            catk->idx = difficulty;
            catk->base.th7kLen2 = sizeof(Catk);
            catk->base.th7kLen = sizeof(Catk);
            catk->base.version = 1;

            memcpy(fileBuffer + sizeOfFile, catk, sizeof(Catk));
            sizeOfFile += sizeof(Catk);
        }
    }

    pscr = &g_GameManager.pscr[0][0][0];
    for (difficulty = 0; difficulty < DIFF_COUNT; difficulty++)
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

    vrsm.base.magic = VRSM_MAGIC;
    vrsm.base.version = 1;
    vrsm.base.th7kLen2 = sizeof(Vrsm);
    vrsm.base.th7kLen = sizeof(Vrsm);
    vrsm.base.isPlayerScore = 0;
    strcpy(vrsm.versionStr, "0100b");
    vrsm.exeSize = g_Supervisor.exeSize;
    vrsm.exeChecksum = g_Supervisor.exeChecksum;

    memcpy(fileBuffer + sizeOfFile, &vrsm, sizeof(Vrsm));
    sizeOfFile += sizeof(Vrsm);

    scoreDat = (ScoreDat *)fileBuffer;
    scoreDat->raw.dstLen = sizeOfFile - sizeof(ScoreDatRaw);
    scoreDat->raw.fileLength = sizeOfFile;
    compressedBuffer = Lzss::Compress(fileBuffer + sizeof(ScoreDatRaw), scoreDat->raw.dstLen,
                                      &scoreDat->raw.srcLen);

    memcpy(fileBuffer + sizeof(ScoreDatRaw), compressedBuffer, scoreDat->raw.srcLen);
    free(compressedBuffer);
    sizeOfFile = scoreDat->raw.srcLen + sizeof(ScoreDatRaw);

    sd = (ScoreDat *)fileBuffer;
    sd->raw.dataOffset = sizeof(ScoreDatRaw);
    sd->raw.csum = 0;
    sd->raw.xorseed[1] = g_Rng.GetRandomU16InRange(256);
    sd->raw.unused_6 = g_Rng.GetRandomU16InRange(256);
    sd->raw.magic = 11;

    for (remainingSize = 4; remainingSize < (i32)sizeOfFile; remainingSize++)
    {
        sd->raw.csum += fileBuffer[remainingSize];
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

i32 ResultScreen::LinkScoreEx(Hscr *out, i32 difficulty, i32 character)
{
    return LinkScore(this->scoreLists[difficulty] + character, out);
}

void ResultScreen::FreeScore(i32 difficulty, i32 character)
{
    FreeAllScores(&this->scoreLists[difficulty][character]);
}

u32 ResultScreen::OnUpdate(ResultScreen *arg)
{
    i32 j;
    i32 i;
    AnmVm *vm;
    i32 vmIdx;

    arg->UpdatePrev();

    switch (arg->resultScreenState)
    {
    case RESULT_STATE_PRACTICE_END:
        g_Supervisor.curState = SUPERVISOR_STATE_MAINMENU;
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    case RESULT_STATE_INIT_PARSE_ONLY:
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    case RESULT_STATE_INIT:
    CASE_RESULT_STATE_INIT:
        if (arg->frameTimer == 0)
        {
            vm = arg->vms;
            for (vmIdx = 0; vmIdx < ARRAY_SIZE_SIGNED(arg->vms); vmIdx++, vm++)
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
                    vm->offset = ZunVec3(-4.0f, -4.0f, 0.0f);
                }
                else
                {
                    vm->color.color = 0xb0ffffff;
                    vm->offset = ZunVec3(0.0f, 0.0f, 0.0f);
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
    case RESULT_STATE_DIFFICULTY_SELECT:
        vmIdx = MoveCursor(arg, 9);
        if (arg->cursor == 5 && !g_GameManager.HasUnlockedPhantomAndMaxClears())
        {
            arg->cursor += vmIdx;
        }
        vm = arg->vms;
        for (vmIdx = 0; vmIdx <= 8; vmIdx++, vm++)
        {
            if (vmIdx == arg->cursor)
            {
                vm->color.color = 0xffffffff;
                vm->offset = ZunVec3(-4.0f, -4.0f, 0.0f);
            }
            else
            {
                vm->color.color = 0xb0ffffff;
                vm->offset = ZunVec3(0.0f, 0.0f, 0.0f);
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
                for (vmIdx = 0; vmIdx < ARRAY_SIZE_SIGNED(arg->vms); vmIdx++, vm++)
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
                for (vmIdx = 0; vmIdx < ARRAY_SIZE_SIGNED(arg->vms); vmIdx++, vm++)
                {
                    vm->pendingInterrupt = 10;
                }
                arg->diffPlayed = arg->cursor;
                arg->resultScreenState = RESULT_STATE_SPELLCARD_LIST;
                arg->stateStep = arg->resultScreenState;
                arg->frameTimer = 0;
                arg->charUsed = -1;
                arg->cursor = arg->savedCursor;
                arg->lastSpellcardSelected = -1;
                break;
            case 7:
                for (vmIdx = 0; vmIdx < ARRAY_SIZE_SIGNED(arg->vms); vmIdx++, vm++)
                {
                    vm->pendingInterrupt = 9;
                }
                arg->diffPlayed = arg->cursor;
                arg->resultScreenState = RESULT_STATE_OVERALL_STATS_INIT;
                arg->stateStep = arg->resultScreenState;
                arg->frameTimer = 0;
                arg->charUsed = -1;
                break;
            GO_BACK:
            case 8:
                for (vmIdx = 0; vmIdx < ARRAY_SIZE_SIGNED(arg->vms); vmIdx++, vm++)
                {
                    vm->pendingInterrupt = 2;
                }
                arg->resultScreenState = RESULT_STATE_EXITING;
                g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
                arg->frameTimer = 0;
                break;
            }
        }
        break;
    case RESULT_STATE_EXITING:
        if (arg->frameTimer < 60)
        {
            break;
        }
        g_Supervisor.curState = SUPERVISOR_STATE_MAINMENU;
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    case RESULT_STATE_SCORE_HARD:
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
                for (i = 0; i < SHOT_COUNT; i++)
                {
                    for (j = 0; j < DIFF_COUNT; j++)
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
    case RESULT_STATE_SCORE_EASY:
    case RESULT_STATE_SCORE_NORMAL:
    case RESULT_STATE_SCORE_LUNATIC:
    case RESULT_STATE_SCORE_EXTRA:
    case RESULT_STATE_SCORE_PHANTASM:
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
            for (vmIdx = 0; vmIdx < ARRAY_SIZE_SIGNED(arg->vms); vmIdx++, vm++)
            {
                vm->pendingInterrupt = arg->diffPlayed + 3;
            }
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            arg->resultScreenState = RESULT_STATE_INIT;
            arg->frameTimer = 0;
            vm = arg->vms;
            for (vmIdx = 0; vmIdx < ARRAY_SIZE_SIGNED(arg->vms); vmIdx++, vm++)
            {
                vm->pendingInterrupt = 1;
            }
            arg->prevCursor = arg->cursor;
            arg->cursor = arg->diffPlayed;
            goto CASE_RESULT_STATE_INIT;
        }
        break;
    case RESULT_STATE_SPELLCARD_LIST:
        if ((arg->lastSpellcardSelected != arg->cursor ||
             arg->prevSpellcardListPage != arg->spellcardListPage) &&
            arg->frameTimer == 20)
        {
            arg->lastSpellcardSelected = arg->cursor;
            arg->prevSpellcardListPage = arg->spellcardListPage;
            for (vmIdx = arg->lastSpellcardSelected * 10;
                 vmIdx < arg->lastSpellcardSelected * 10 + 10; vmIdx++)
            {
                if (vmIdx >= SPELLCARD_COUNT)
                {
                    break;
                }
                if (g_GameManager.catk[vmIdx].numAttemptsPerShot[SHOT_COUNT] == 0)
                {
                    AnmManager::DrawVmTextFmt(g_AnmManager, arg->spellcardListVms + vmIdx % 10,
                                              0xffffff, 0, "？？？？？");
                }
                else
                {
                    AnmManager::DrawVmTextFmt(g_AnmManager, arg->spellcardListVms + vmIdx % 10,
                                              0xffffff, 0, g_GameManager.catk[vmIdx].name);
                }
                arg->spellcardListVms[vmIdx % 10].color.bytes.a = 255;
            }
            AnmManager::DrawVmTextFmt(g_AnmManager, arg->spellcardListVms + 10, 0xffffff, 0,
                                      "%s %3d枚中%3d枚取得（キャラ切り替え↓↑）",
                                      g_CharacterList[arg->prevSpellcardListPage], SPELLCARD_COUNT,
                                      arg->totalPlayCountPerShot[arg->spellcardListPage]);
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
            for (vmIdx = 0; vmIdx < ARRAY_SIZE_SIGNED(arg->vms); vmIdx++, vm++)
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
            arg->resultScreenState = RESULT_STATE_INIT;
            arg->frameTimer = 0;
            vm = arg->vms;
            for (vmIdx = 0; vmIdx < ARRAY_SIZE_SIGNED(arg->vms); vmIdx++, vm++)
            {
                vm->pendingInterrupt = 1;
            }
            arg->savedCursor = arg->cursor;
            arg->cursor = arg->diffPlayed;
            goto CASE_RESULT_STATE_INIT;
        }
        break;
    case RESULT_STATE_ENTER_NAME:
        arg->HandleResultKeyboard();
        break;
    case RESULT_STATE_REPLAY_SAVE_PROMPT:
    case RESULT_STATE_REPLAY_CANNOT_SAVE:
    case RESULT_STATE_REPLAY_SELECT_SAVE_SLOT:
    case RESULT_STATE_REPLAY_SAVING:
    case RESULT_STATE_REPLAY_OVERWRITE:
        arg->HandleReplaySaveKeyboard();
        break;
    case RESULT_STATE_FINAL_STATS_SHOW:
    case RESULT_STATE_FINAL_STATS_WAIT:
        arg->CheckConfirmButton();
        break;
    case RESULT_STATE_OVERALL_STATS_INIT:
    case RESULT_STATE_OVERALL_STATS_INPUT:
    case RESULT_STATE_OVERALL_STATS_EXIT:
        if (arg->DrawStats() != ZUN_SUCCESS)
        {
            goto CASE_RESULT_STATE_INIT;
        }
        break;
    }
    vm = arg->vms;
    for (vmIdx = 0; vmIdx < ARRAY_SIZE_SIGNED(arg->vms); vmIdx++, vm++)
    {
        g_AnmManager->ExecuteScript(vm);
    }
    arg->frameTimer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult ResultScreen::HandleResultKeyboard()
{
    i32 cursor2;
    i32 cursor;
    f32 slowRateFactor;
    AnmVm *vm;
    i32 vmIdx;

    if (g_Supervisor.IsSlowMode() || g_Supervisor.timingBad)
    {
        this->resultScreenState = RESULT_STATE_FINAL_STATS_SHOW;
        this->frameTimer = 0;
        memcpy(g_GameManager.catk, g_GameManager.catkAgain, 0x4218);
        return ZUN_SUCCESS;
    }
    if (this->frameTimer == 0)
    {
        this->charUsed = (u32)g_GameManager.character * 2 + (u32)g_GameManager.shotType;
        this->diffPlayed = g_GameManager.difficulty;
        vm = this->vms;
        for (vmIdx = 0; vmIdx < ARRAY_SIZE_SIGNED(this->vms); vmIdx++, vm++)
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
        this->curScore.base.version = 1;
        memcpy(&this->curScore.base.magic, "HSCR", 4);
        if (!g_GameManager.finished)
        {
            this->curScore.stage = (u8)g_GameManager.currentStage;
        }
        else
        {
            this->curScore.stage = 99;
        }
        this->curScore.base.isPlayerScore = 1;
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
        this->resultScreenState = RESULT_STATE_FINAL_STATS_SHOW;
        this->frameTimer = 0;
        vm = this->vms;
        for (vmIdx = 0; vmIdx < ARRAY_SIZE_SIGNED(this->vms); vmIdx++, vm++)
        {
            vm->pendingInterrupt = 2;
        }
        strcpy(this->replayName, this->curScore.name);
        strcpy(this->lsnmHeader.name, this->replayName);
    }
    return ZUN_SUCCESS;
}

void ResultScreen::GetDate(char *outDate)
{
    time_t seconds;
    tm *timeinfo;

    time(&seconds);
    timeinfo = localtime(&seconds);
    strftime(outDate, 6, "%m/%d", timeinfo);
}

ZunResult ResultScreen::HandleReplaySaveKeyboard()
{
    i32 cursor2;
    i32 cursor;
    ReplayFile *replayFile;
    i32 i;
    i32 interrupt;
    AnmVm *vm;

    switch (this->resultScreenState)
    {
    case RESULT_STATE_REPLAY_SAVE_PROMPT:
        if (this->frameTimer == 60)
        {
            if (g_Supervisor.IsSlowMode() || g_Supervisor.timingBad)
            {
                interrupt = 19;
            }
            else if (g_GameManager.globals->numRetries != 0 ||
                     Touch::WasUsedThisRun()) // it probably goes without saying that the touch mode
                                              // is wholly incompatible with replays
            {
                interrupt = 14;
            }
            else
            {
                interrupt = 11;
            }
            vm = this->vms;
            for (i = 0; i < ARRAY_SIZE_SIGNED(this->vms); i++, vm++)
            {
                vm->pendingInterrupt = (i16)interrupt;
            }
            if (interrupt != 11)
            {
                this->resultScreenState = RESULT_STATE_REPLAY_CANNOT_SAVE;
            }
            this->cursor = 0;
        }
        vm = this->vms + 19;
        if (this->cursor == 0)
        {
            vm[0].color.color = (vm[0].color.color & 0xff000000) | 0xff6060;
            vm[1].color.color = (vm[1].color.color & 0xff000000) | 0x606060;
        }
        else
        {
            vm[0].color.color = (vm[0].color.color & 0xff000000) | 0x606060;
            vm[1].color.color = (vm[1].color.color & 0xff000000) | 0xff6060;
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
                this->resultScreenState = RESULT_STATE_REPLAY_SELECT_SAVE_SLOT;
                vm = this->vms;
                for (i = 0; i < ARRAY_SIZE_SIGNED(this->vms); i++, vm++)
                {
                    vm->pendingInterrupt = 12;
                }
                this->frameTimer = 0;
                goto CASE_RESULT_STATE_REPLAY_SELECT_SAVE_SLOT;
            }

        SOUND_BACK_AND_RETURN:
            this->frameTimer = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->resultScreenState = RESULT_STATE_EXITING;
            vm = this->vms;
            for (i = 0; i < ARRAY_SIZE_SIGNED(this->vms); i++, vm++)
            {
                vm->pendingInterrupt = 2;
            }
        }
        break;
    case RESULT_STATE_REPLAY_CANNOT_SAVE:
        if (this->frameTimer < 20)
        {
            return ZUN_SUCCESS;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU) || WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU))
        {
            this->frameTimer = 0;
            g_SoundPlayer.PlaySoundByIdx(SOUND_BACK, 0);
            this->resultScreenState = RESULT_STATE_EXITING;
            vm = this->vms;
            for (i = 0; i < ARRAY_SIZE_SIGNED(this->vms); i++, vm++)
            {
                vm->pendingInterrupt = 2;
            }
        }
        break;
    CASE_RESULT_STATE_REPLAY_SELECT_SAVE_SLOT:
    case RESULT_STATE_REPLAY_SELECT_SAVE_SLOT:
        if (this->frameTimer == 0)
        {
            std::filesystem::create_directory(FileSystem::GetPrefPath("replay"));

            for (i = 0; i < ARRAY_SIZE_SIGNED(this->replays); i++)
            {
                char filename[32];
                snprintf(filename, sizeof(filename), "th7_%.2d.rpy", i + 1);
                std::string replayPath = fs::path(FileSystem::GetPrefPath("replay")) / filename;
                replayFile = (ReplayFile *)FileSystem::OpenFile(replayPath.c_str(), 1);
                if (!replayFile)
                {
                    continue;
                }

                replayFile = ReplayManager::ValidateReplayData(replayFile, g_LastFileSize);
                if (replayFile)
                {
                    this->replays[i] = *replayFile;
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
            if (*(i32 *)&this->replays[this->cursor].head.magic != *(i32 *)&"T7RP" ||
                (this->replays[this->cursor].head.version & 0xfff) != 256)
            {
                vm = this->vms;
                for (i = 0; i < ARRAY_SIZE_SIGNED(this->vms); i++, vm++)
                {
                    vm->pendingInterrupt = 17;
                }
                vm = &this->vms[this->chosenReplayIdx + 25];
                vm->pendingInterrupt = 16;
                this->resultScreenState = RESULT_STATE_REPLAY_SAVING;
            }
            else
            {
                vm = this->vms;
                for (i = 0; i < ARRAY_SIZE_SIGNED(this->vms); i++, vm++)
                {
                    vm->pendingInterrupt = 13;
                }
                vm = &this->vms[this->chosenReplayIdx + 25];
                vm->pendingInterrupt = 16;
                this->resultScreenState = RESULT_STATE_REPLAY_OVERWRITE;
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
            this->resultScreenState = RESULT_STATE_REPLAY_SAVE_PROMPT;
            vm = this->vms;
            for (i = 0; i < ARRAY_SIZE_SIGNED(this->vms); i++, vm++)
            {
                vm->pendingInterrupt = 2;
            }
            this->frameTimer = 0;
        }
        break;
    case RESULT_STATE_REPLAY_SAVING:
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
                char filename[32];
                snprintf(filename, sizeof(filename), "th7_%.2d.rpy", this->chosenReplayIdx + 1);
                std::string replayPath = fs::path(FileSystem::GetPrefPath("replay")) / filename;
                ReplayManager::SaveReplay(replayPath.c_str(), this->replayName);
                this->frameTimer = 0;
                this->resultScreenState = RESULT_STATE_EXITING;
                vm = this->vms;
                for (i = 0; i < ARRAY_SIZE_SIGNED(this->vms); i++, vm++)
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
    case RESULT_STATE_REPLAY_OVERWRITE:
        vm = this->vms + 19;
        if (this->cursor == 0)
        {
            vm[0].color.color = (vm[0].color.color & 0xff000000) | 0xff6060;
            vm[1].color.color = (vm[1].color.color & 0xff000000) | 0x606060;
        }
        else
        {
            vm[0].color.color = (vm[0].color.color & 0xff000000) | 0x606060;
            vm[1].color.color = (vm[1].color.color & 0xff000000) | 0xff6060;
        }
        if (this->frameTimer < 20)
        {
            return ZUN_SUCCESS;
        }
        MoveCursorHorizontally(this, 2);
        if (WAS_PRESSED_RAW(TH_BUTTON_RETURNMENU) || WAS_PRESSED_RAW(TH_BUTTON_MENU))
        {
            goto LAB_004473e3;
        }
        else if (WAS_PRESSED_RAW(TH_BUTTON_SELECTMENU))
        {
            this->frameTimer = 0;
            if (this->cursor == 0)
            {
                vm = this->vms;
                for (i = 0; i < ARRAY_SIZE_SIGNED(this->vms); i++, vm++)
                {
                    vm->pendingInterrupt = 17;
                }
                vm = &this->vms[chosenReplayIdx + 25];
                vm->pendingInterrupt = 16;
                this->resultScreenState = RESULT_STATE_REPLAY_SAVING;
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

ZunResult ResultScreen::CheckConfirmButton()
{
    AnmVm *viewport;

    switch (this->resultScreenState)
    {
    case RESULT_STATE_FINAL_STATS_SHOW:
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
            this->resultScreenState = RESULT_STATE_FINAL_STATS_WAIT;
        }
        break;
    case RESULT_STATE_FINAL_STATS_WAIT:
        if (this->frameTimer >= 30)
        {
            this->frameTimer = 59;
            this->resultScreenState = RESULT_STATE_REPLAY_SAVE_PROMPT;
        }
        break;
    }
    return ZUN_SUCCESS;
}

i32 ResultScreen::DrawStats()
{
    AnmVm *vm;
    ZunVec3 pos;

    switch (this->resultScreenState)
    {
    case RESULT_STATE_OVERALL_STATS_INIT:
        if (this->frameTimer == 1)
        {
            pos.x = 56.0f;
            pos.y = 128.0f;
            pos.z = 0.0f;
            vm = this->spellcardListVms;
            vm->pos = pos;
            g_Supervisor.UpdateStartupTime();
            AnmManager::DrawVmTextFmt(g_AnmManager, vm, 0xffffff, 0, "総起動時間   %.2d:%.2d:%.2d",
                                      g_GameManager.plst.totalHours,
                                      g_GameManager.plst.totalMinutes,
                                      g_GameManager.plst.totalSeconds);
            g_Supervisor.UpdateStartupTime();
            this->lastTotalSeconds = g_GameManager.plst.totalSeconds;

            vm++;
            pos.y += 17.0f;
            vm->pos = pos;
            AnmManager::DrawVmTextFmt(g_AnmManager, vm, 0xffffff, 0, "総プレイ時間 %.2d:%.2d:%.2d",
                                      g_GameManager.plst.gameHours, g_GameManager.plst.gameMinutes,
                                      g_GameManager.plst.gameSeconds);

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

            for (i32 i = 0; i < ARRAY_SIZE_SIGNED(g_CharacterList); i++)
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
                    g_TotalForAllProtagonists, g_GameManager.plst.playDataByDifficulty[0].playCount,
                    g_GameManager.plst.playDataByDifficulty[1].playCount,
                    g_GameManager.plst.playDataByDifficulty[2].playCount,
                    g_GameManager.plst.playDataByDifficulty[3].playCount,
                    g_GameManager.plst.playDataByDifficulty[4].playCount,
                    g_GameManager.plst.playDataByDifficulty[5].playCount,
                    g_GameManager.plst.playDataByDifficulty[6].playCount);
            }
            else
            {
                AnmManager::DrawVmTextFmt(g_AnmManager, vm, 0xffffff, 0,
                                          "%s %6d %6d %6d %6d %6d %6d", g_TotalForAllProtagonists,
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
                    g_AnmManager, vm, 0xffffff, 0, "クリア回数  　　 %6d %6d %6d %6d %6d %6d %6d",
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
                    g_AnmManager, vm, 0xffffff, 0, "クリア回数  　　 %6d %6d %6d %6d %6d %6d",
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
                AnmManager::DrawVmTextFmt(g_AnmManager, vm, 0xffffff, 0,
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
                AnmManager::DrawVmTextFmt(g_AnmManager, vm, 0xffffff, 0,
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
                    g_AnmManager, vm, 0xffffff, 0, "プラクティス　   %6d %6d %6d %6d %6d %6d %6d",
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
                    g_AnmManager, vm, 0xffffff, 0, "プラクティス　   %6d %6d %6d %6d %6d %6d",
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
                AnmManager::DrawVmTextFmt(g_AnmManager, vm, 0xffffff, 0,
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
                AnmManager::DrawVmTextFmt(g_AnmManager, vm, 0xffffff, 0,
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
            this->resultScreenState = RESULT_STATE_OVERALL_STATS_INPUT;
        }
        break;

    case RESULT_STATE_OVERALL_STATS_INPUT:
        if (this->frameTimer % 60 == 0 &&
            (g_Supervisor.UpdateStartupTime(),
             g_GameManager.plst.totalSeconds != this->lastTotalSeconds))
        {
            vm = this->spellcardListVms;
            AnmManager::DrawVmTextFmt(g_AnmManager, vm, 0xffffff, 0, "総起動時間   %.2d:%.2d:%.2d",
                                      g_GameManager.plst.totalHours,
                                      g_GameManager.plst.totalMinutes,
                                      g_GameManager.plst.totalSeconds);
            this->lastTotalSeconds = g_GameManager.plst.totalSeconds;
        }
        if (WAS_PRESSED_RAW(TH_BUTTON_SHOOT | TH_BUTTON_BOMB | TH_BUTTON_MENU | TH_BUTTON_ENTER))
        {
            this->resultScreenState = RESULT_STATE_OVERALL_STATS_EXIT;
            this->frameTimer = 0;
        }
        break;

    case RESULT_STATE_OVERALL_STATS_EXIT:
        if (this->frameTimer < 20)
        {
            vm = this->spellcardListVms;
            for (i32 i = 0; i < 14; i++, vm++)
            {
                vm->color.bytes.a = 255 - this->frameTimer * 255 / 20;
            }
            break;
        }
        this->resultScreenState = RESULT_STATE_INIT;
        this->frameTimer = 0;
        return 1;
    }

    return 0;
}

ZunResult ResultScreen::DrawFinalStats()
{
    AnmVm *vm;
    ZunVec3 pos;
    f32 rankingProbably;
    f32 clearPercent;
    f32 slowdown;
    u32 color;

    switch (this->resultScreenState)
    {
    case RESULT_STATE_FINAL_STATS_SHOW:
    case RESULT_STATE_FINAL_STATS_WAIT:
        vm = &this->vms[40];
        color = vm->color.color;
        g_AsciiManager.color = color;
        rankingProbably = 0.0f;

        clearPercent =
            g_GameManager.difficulty < DIFF_EXTRA    ? (f32)g_GameManager.playTimeAll / 180621.0f
            : g_GameManager.difficulty == DIFF_EXTRA ? (f32)g_GameManager.playTimeAll / 80000.0f
                                                     : (f32)g_GameManager.playTimeAll / 85000.0f;

        pos = vm->pos;
        pos.x += 210.0f;
        pos.y += 32.0f;

        AsciiManager::AddFormatText(&g_AsciiManager, &pos, "%9d", g_GameManager.globals->guiScore);

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
            rankingProbably +=
                (f32)(u32)(g_GameManager.globals->guiScore - 2000000) / 198000000.0f * 60.0f -
                20.0f;
        }
        else
        {
            rankingProbably += 40.0f;
        }

        pos.y += 22.0f;
        g_AsciiManager.AddString(&pos, g_DifficultyNameTable[g_GameManager.difficulty]);

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

        slowdown = (g_Supervisor.framerateMultiplier / g_Supervisor.fpsAccumulator - 0.5f) * 2;

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

        (void)rankingProbably; // only for it to NEVER be used

        g_AsciiManager.color = 0xffffffff;
        break;
    }
    return ZUN_SUCCESS;
}

u32 ResultScreen::OnDraw(ResultScreen *arg)
{
    char charBuf[16];
    f32 offsetY;
    f32 offsetX;
    i32 spellcardIdx;
    f32 oldX;
    ZunVec3 pos;
    i32 j;
    ScoreListNode *node;
    AnmVm *vm;
    char name[9];
    i32 i;
    ZunVec3 charPos;

    vm = arg->vms;
    g_AnmManager->Flush();
    g_Supervisor.viewport.x = 0;
    g_Supervisor.viewport.y = 0;
    g_Supervisor.viewport.width = 640;
    g_Supervisor.viewport.height = 480;
    g_Supervisor.gfxDevice->SetViewport(g_Supervisor.viewport);
    g_AnmManager->CopySurfaceToBackBuffer(0, 0, 0, 0, 0);
    for (i = 0; i < ARRAY_SIZE_SIGNED(arg->vms); i++, vm++)
    {
        pos = vm->pos;
        ZunVec3 drawPos = vm->prevPos.Lerp(vm->pos, g_RenderAlpha);
        vm->pos = drawPos + vm->offset;
        g_AnmManager->DrawNoRotation(vm);
        vm->pos = pos;
    }
    vm = arg->vms + 16;
    if (vm->pos.x < 640.0f)
    {
        pos = vm->prevPos.Lerp(vm->pos, g_RenderAlpha);
        if (arg->stateStep != 9)
        {
            arg->spellcardListVms[0].pos = pos;
            arg->spellcardListVms[0].pos.x += 64.0f;
            g_AnmManager->DrawNoRotation(arg->spellcardListVms);
            pos.y += 18.0f;
            pos.x += 24.0f;
            g_AsciiManager.color = 0xffe0e0ef;
            AsciiManager::AddFormatText(&g_AsciiManager, &pos,
                                        "No  Name      Score(Stage)  Date   Slow");
            pos.y += 18.0f;
            node = arg->scoreLists[arg->diffPlayed][arg->charUsed].next;
            for (i = 0; i < 10; i++)
            {
                if (arg->resultScreenState == RESULT_STATE_ENTER_NAME)
                {
                    if (node->data->base.isPlayerScore)
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
                AsciiManager::AddFormatText(&g_AsciiManager, &pos, "%2d", i + 1);
                pos.x += 48.0f;
                if (arg->resultScreenState == RESULT_STATE_ENTER_NAME &&
                    node->data->base.isPlayerScore)
                {
                    memset(name, ' ', 8);
                    name[8] = '\0';
                    name[arg->cursor >= 8 ? 7 : arg->cursor] = '_';
                    AsciiManager::AddFormatText(&g_AsciiManager, &pos, "%8s", name);
                }
                if (node->data->stage <= 6)
                {
                    AsciiManager::AddFormatText(
                        &g_AsciiManager, &pos, "%8s %9d%1d(%d)", node->data->name,
                        node->data->score, (i32)node->data->numRetries, (u32)node->data->stage);
                }
                else if (node->data->stage == 7 || node->data->stage == 8)
                {
                    AsciiManager::AddFormatText(&g_AsciiManager, &pos, "%8s %9d%1d(1)",
                                                node->data->name, node->data->score,
                                                (i32)node->data->numRetries);
                }
                else
                {
                    AsciiManager::AddFormatText(&g_AsciiManager, &pos, "%8s %9d%1d(C)",
                                                node->data->name, node->data->score,
                                                (i32)node->data->numRetries);
                }
                pos.x += 320.0f;
                AsciiManager::AddFormatText(&g_AsciiManager, &pos, " %5s   %3.2f", node->data->date,
                                            node->data->slowRatePercent);
                pos.y += 18.0f;
                pos.x -= 368.0f;
                node = node->next;
            }
        }
        else
        {
            arg->spellcardListVms[10].pos = pos;
            g_AnmManager->DrawNoRotation(arg->spellcardListVms + 10);
            pos.y += 16.0f;
            for (i = 0; i < 10; i++)
            {
                spellcardIdx = arg->lastSpellcardSelected * 10 + i;
                if (spellcardIdx >= SPELLCARD_COUNT)
                {
                    break;
                }
                oldX = pos.x;
                pos.x += 320.0f;
                pos.y += 16.0f;
                arg->rightArrowVm.pos = pos;
                arg->rightArrowVm.scale.x = 2.375f;
                g_AnmManager->DrawNoRotation(&arg->rightArrowVm);
                pos.y -= 16.0f;
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
                AsciiManager::AddFormatText(&g_AsciiManager, &pos, "No.%.2d", spellcardIdx + 1);
                arg->spellcardListVms[i].pos.x += 96.0f;
                g_AnmManager->DrawNoRotation(arg->spellcardListVms + i);
                pos.x += 496.0f;
                if (g_GameManager.catk[spellcardIdx]
                        .numAttemptsPerShot[arg->prevSpellcardListPage] == 0)
                {
                    AsciiManager::AddFormatText(
                        &g_AsciiManager, &pos, "---/---",
                        g_GameManager.catk[spellcardIdx]
                            .numSuccessesPerShot[arg->prevSpellcardListPage],
                        g_GameManager.catk[spellcardIdx]
                            .numAttemptsPerShot[arg->prevSpellcardListPage]);
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
                pos.x -= 496.0f;
                pos.x += 424.0f;
                pos.y -= 13.0f;
                g_AsciiManager.color = 0xffa08090;
                g_AsciiManager.scale.x = 0.8f;
                g_AsciiManager.scale.y = 0.8f;
                if (g_GameManager.catk[spellcardIdx]
                        .numAttemptsPerShot[arg->prevSpellcardListPage] != 0)
                {
                    AsciiManager::AddFormatText(&g_AsciiManager, &pos, "MaxBonus %8d",
                                                g_GameManager.catk[spellcardIdx]
                                                    .highScorePerShot[arg->prevSpellcardListPage]);
                }
                pos.x -= 424.0f;
                pos.y += 13.0f;
                g_AsciiManager.scale.x = 1.0f;
                g_AsciiManager.scale.y = 1.0f;
                if (arg->listScrollAnimState == 0)
                {
                    pos.y += 33.0f;
                }
                else
                {
                    f32 animTime = (f32)arg->frameTimer + g_RenderAlpha;
                    if (animTime < 20.0f)
                    {
                        pos.y += (20.0f - animTime) * (33.0f / 20.0f);
                    }
                    else
                    {
                        pos.y += (animTime - 20.0f) * (33.0f / 20.0f);
                    }
                }
            }
            if (arg->frameTimer >= 40)
            {
                arg->listScrollAnimState = 0;
            }
        }
    }
    if (arg->resultScreenState == RESULT_STATE_ENTER_NAME ||
        arg->resultScreenState == RESULT_STATE_REPLAY_SAVING)
    {
        pos = ZunVec3(160.0f, 356.0f, 0.0f);
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
                pos.x += 20.0f;
            }
            pos.x -= (f32)(j * 20);
            pos.y += 18.0f;
        }
    }
    g_AsciiManager.scale.x = 1.0f;
    g_AsciiManager.scale.y = 1.0f;
    if (arg->resultScreenState >= RESULT_STATE_REPLAY_SAVE_PROMPT &&
        arg->resultScreenState <= RESULT_STATE_REPLAY_OVERWRITE)
    {
        vm = &arg->vms[18];
        for (i = 0; i < 6; i++, vm++)
        {
            g_AnmManager->DrawNoRotation(vm);
        }
        vm = &arg->vms[24];
        pos = vm->pos;
        vm++;
        AsciiManager::AddFormatText(&g_AsciiManager, &pos, "No.   Name     Date   Player Score");
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
            if (arg->resultScreenState == RESULT_STATE_REPLAY_SAVING)
            {
                AsciiManager::AddFormatText(
                    &g_AsciiManager, &pos, "No.%.2d %8s %5s  %7s %9d0", i + 1, arg->replayName,
                    arg->defaultReplay.data.date,
                    g_CharactersAndShotTypesStrings[(u32)g_GameManager.character * 2 +
                                                    (u32)g_GameManager.shotType],
                    arg->defaultReplay.data.score);
                g_AsciiManager.color = 0xfff0f0ff;
                memset(name, ' ', 8);
                name[8] = '\0';
                name[arg->cursor >= 8 ? 7 : arg->cursor] = '_';
                AsciiManager::AddFormatText(&g_AsciiManager, &pos, "      %8s", name);
            }
            else if (*(i32 *)&arg->replays[i].head.magic != *(i32 *)&"T7RP" ||
                     (arg->replays[i].head.version & 0xfff) != 256)
            {
                AsciiManager::AddFormatText(&g_AsciiManager, &pos,
                                            "No.%.2d -------- --/--  -------          0", i + 1);
            }
            else
            {
                AsciiManager::AddFormatText(
                    &g_AsciiManager, &pos, "No.%.2d %8s %5s  %7s %9d0", i + 1,
                    arg->replays[i].data.name, arg->replays[i].data.date,
                    g_CharactersAndShotTypesStrings[arg->replays[i].data.shotType],
                    arg->replays[i].data.score);
            }
        }
    }
    g_AsciiManager.color = 0xffffffff;
    arg->DrawFinalStats();
    if (arg->resultScreenState == RESULT_STATE_OVERALL_STATS_INIT ||
        arg->resultScreenState == RESULT_STATE_OVERALL_STATS_INPUT ||
        arg->resultScreenState == RESULT_STATE_OVERALL_STATS_EXIT)
    {
        vm = arg->spellcardListVms;
        for (i = 0; i < ARRAY_SIZE_SIGNED(arg->spellcardListVms) - 1; i++, vm++)
        {
            g_AnmManager->DrawNoRotation(vm);
        }
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ZunResult ResultScreen::AddedCallback(ResultScreen *arg)
{
    i32 k;
    i32 j;
    i32 catkIdx;
    Catk *catk;
    AnmVm *vm;
    i32 i;

    g_GameManager.HasUnlockedPhantomAndMaxClears();
    for (i = 0; i < DIFF_COUNT; i++)
    {
        for (j = 0; j < SHOT_COUNT; j++)
        {
            for (k = 0; k < 10; k++)
            {
                arg->defaultScores[i][j][k].score = 100000 - k * 10000;
                arg->defaultScores[i][j][k].slowRatePercent = 0.0f;
                memcpy(&arg->defaultScores[i][j][k].base.magic, "DMYS", 4);
                arg->defaultScores[i][j][k].base.version = 1;
                arg->defaultScores[i][j][k].base.th7kLen2 = sizeof(Hscr);
                arg->defaultScores[i][j][k].base.th7kLen = sizeof(Hscr);
                arg->defaultScores[i][j][k].stage = 1;
                arg->defaultScores[i][j][k].base.isPlayerScore = 0;
                arg->defaultScores[i][j][k].numRetries = 0;
                arg->LinkScoreEx(arg->defaultScores[i][j] + k, i, j);
                strcpy(arg->defaultScores[i][j][k].name, "--------");
                strcpy(arg->defaultScores[i][j][k].date, "--/--");
            }
        }
    }
    if (arg->resultScreenState != RESULT_STATE_INIT_PARSE_ONLY)
    {
        if (g_AnmManager->LoadSurface(0, "data/result/result.jpg") != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        if (g_AnmManager->LoadAnms(ANM_FILE_RESULT, "data/result00.anm", ANM_OFFSET_RESULT) !=
            ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        vm = arg->vms;
        for (i = 0; i < ARRAY_SIZE_SIGNED(arg->vms); i++, vm++)
        {
            vm->pos = ZunVec3(0.0f, 0.0f, 0.0f);
            vm->offset = ZunVec3(0.0f, 0.0f, 0.0f);
            g_AnmManager->SetAnmIdxAndExecuteScript(vm, i + 2304);
        }
        g_AnmManager->InitializeAndSetActiveSprite(&arg->rightArrowVm, 2320);
        vm = arg->spellcardListVms;
        for (i = 0; i < ARRAY_SIZE_SIGNED(arg->spellcardListVms); i++, vm++)
        {
            g_AnmManager->InitializeAndSetActiveSprite(vm, i + 1813);
            vm->pos = ZunVec3(0.0f, 0.0f, 0.0f);
            vm->anchor = 3;
            vm->fontWidth = 15;
            vm->fontHeight = 15;
        }
    }
    arg->prevCursor = 0;
    arg->scoreDat = OpenScore(FileSystem::GetPrefPath("score.dat").c_str());
    for (i = 0; i < DIFF_COUNT; i++)
    {
        for (j = 0; j < SHOT_COUNT; j++)
        {
            GetHighScore(arg->scoreDat, arg->scoreLists[i] + j, j, i, NULL);
        }
    }
    arg->lsnmHeader.base.magic = LSNM_MAGIC;
    arg->lsnmHeader.base.version = 1;
    arg->lsnmHeader.base.th7kLen2 = sizeof(Lsnm);
    arg->lsnmHeader.base.th7kLen = sizeof(Lsnm);
    strcpy(arg->lsnmHeader.name, "        ");
    arg->isClearingReplayName = ParseLsnm(arg->scoreDat, &arg->lsnmHeader);
    if (arg->resultScreenState != RESULT_STATE_ENTER_NAME &&
        arg->resultScreenState != RESULT_STATE_PRACTICE_END)
    {
        ParseCatk(arg->scoreDat, g_GameManager.catk);
        ParseClrd(arg->scoreDat, g_GameManager.clrd);
        g_GameManager.HasUnlockedPhantomAndMaxClears();
        ParsePscr(arg->scoreDat, &g_GameManager.pscr[0][0][0]);
    }
    if (arg->resultScreenState == RESULT_STATE_PRACTICE_END)
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
        arg->resultScreenState = RESULT_STATE_REPLAY_SAVE_PROMPT;
        strcpy(arg->replayName, arg->lsnmHeader.name);
    }
    for (i = 0; i < SHOT_COUNT + 1; i++)
    {
        catk = g_GameManager.catk;
        arg->totalPlayCountPerShot[i] = 0;
        for (catkIdx = 0; catkIdx < SPELLCARD_COUNT; catkIdx++, catk++)
        {
            if (catk->base.magic != CATK_MAGIC || catk->base.version != 1)
            {
                continue;
            }
            if (catk->numSuccessesPerShot[i] != 0)
            {
                arg->totalPlayCountPerShot[i]++;
            }
        }
    }
    arg->spellcardListPage = 6;
    arg->prevSpellcardListPage = 6;
    arg->listScrollAnimState = 0;
    arg->leftArrowVm.activeSpriteIdx = -1;
    if (arg->resultScreenState == RESULT_STATE_INIT_PARSE_ONLY)
    {
        DeletedCallback(arg);
        return ZUN_ERROR;
    }

    return ZUN_SUCCESS;
}

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

ZunResult ResultScreen::RegisterChain(u32 type)
{
    ResultScreen *resultScreen = new ResultScreen;
    Supervisor::DebugPrint("Stg.PlayTimeAll = %d\r\n", g_GameManager.playTimeAll);
    if (type == 1)
    {
        if (!g_GameManager.practice)
        {
            resultScreen->resultScreenState = RESULT_STATE_ENTER_NAME;
        }
        else
        {
            resultScreen->resultScreenState = RESULT_STATE_PRACTICE_END;
        }
    }
    else if (type == 2)
    {
        resultScreen->resultScreenState = RESULT_STATE_INIT_PARSE_ONLY;
        AddedCallback(resultScreen);
        return ZUN_SUCCESS;
    }
    resultScreen->calcChain = g_Chain.CreateElem((ChainCallback)OnUpdate);
    resultScreen->calcChain->addedCallback = (ChainLifecycleCallback)AddedCallback;
    resultScreen->calcChain->deletedCallback = (ChainLifecycleCallback)DeletedCallback;
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
