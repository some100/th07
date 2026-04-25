#pragma once

#include "AnmVm.hpp"
#include "Chain.hpp"

struct Ending
{
    Ending();

    static ZunResult RegisterChain();

    static ZunResult AddedCallback(Ending *arg);
    static ZunResult DeletedCallback(Ending *arg);
    static u32 OnUpdate(Ending *arg);
    static u32 OnDraw(Ending *arg);

    void FadingEffect();
    ZunResult LoadEnding(const char *endFilePath);
    ZunResult ParseEndFile();
    i32 ReadEndFileParameter();
    
    // i have no idea why this exists either
    static void AnInlineFunctionThatAllocates32BytesAndNothingElse()
    {
        i32 idk[8];
    }

    ChainElem *calcChain;
    ChainElem *drawChain;
    D3DXVECTOR2 backgroundPos;
    f32 backgroundScrollSpeed;
    AnmVm sprites[16];
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
    i32 possiblyTimesFileParsed;
    ZunColor textColor;
    ZunColor endingFadeRectColor;
    i32 timeFading;
    i32 fadeFrames;
    i32 fadeType;
    char *endFileDataPtr;
};
C_ASSERT(sizeof(Ending) == 0x2530);
