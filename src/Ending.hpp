#pragma once

#include "AnmVm.hpp"
#include "Chain.hpp"

typedef enum EndingFadeType
{
    ENDING_FADE_NONE = 0,
    ENDING_FADE_OUT_BLACK = 1,
    ENDING_FADE_IN_BLACK = 2,
    ENDING_FADE_OUT_WHITE = 3,
    ENDING_FADE_IN_WHITE = 4,
} EndingFadeType;

#define MAX_ENDING_SPRITES 15

struct Ending
{
    Ending()
    {
        memset(this, 0, sizeof(Ending));
        this->line2Delay = 8;
        this->timer2 = 0;
        this->timer1 = 0;
        this->backgroundPos.x = 0.0f;
        this->backgroundPos.y = 0.0f;
        this->backgroundScrollSpeed = 0.0f;
    }

    static ZunResult RegisterChain();

    static ZunResult AddedCallback(Ending *arg);
    static ZunResult DeletedCallback(Ending *arg);
    static u32 OnUpdate(Ending *arg);
    static u32 OnDraw(Ending *arg);

    void FadingEffect();
    ZunResult LoadEnding(const char *endFilePath);
    ZunResult ParseEndFile();
    i32 ReadEndFileParameter();

    void UpdatePrev()
    {
        this->prevBackgroundPos = this->backgroundPos;
        for (i32 i = 0; i < 16; i++)
        {
            this->sprites[i].UpdatePrev();
        }
    }

    ChainElem *calcChain;
    ChainElem *drawChain;
    Float2 backgroundPos;
    Float2 prevBackgroundPos;
    f32 backgroundScrollSpeed;
    AnmVm sprites[MAX_ENDING_SPRITES + 1];
    char *endFileData;
    i32 hasSeenEnding;
    ZunTimer timer1;
    ZunTimer timer2;
    ZunTimer timer3;
    i32 minWaitResetFrames;
    i32 minWaitFrames;
    i32 line2Delay;
    i32 topLineDelay;
    i32 unused_2510;
    i32 timesFileParsed;
    ZunColor textColor;
    ZunColor endingFadeRectColor;
    ZunColor prevEndingFadeRectColor;
    i32 timeFading;
    i32 fadeFrames;
    i32 fadeType;
    char *endFileDataPtr;
};
