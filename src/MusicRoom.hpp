#pragma once

#include <string.h>

#include "AnmVm.hpp"
#include "Chain.hpp"

#pragma optimize("s", on)

struct TrackDescriptor
{
    TrackDescriptor()
    {
        memset(this, 0, sizeof(TrackDescriptor));
    }

    char path[64];
    char title[66];
    char description[8][66];
};
C_ASSERT(sizeof(TrackDescriptor) == 0x292);

struct MusicRoom
{
    MusicRoom()
    {
        memset(this, 0, sizeof(MusicRoom));
    }

    static ZunResult RegisterChain();

    static ZunResult AddedCallback(MusicRoom *arg);
    static ZunResult DeletedCallback(MusicRoom *arg);
    static u32 OnUpdate(MusicRoom *arg);
    static u32 OnDraw(MusicRoom *arg);

    ZunResult CheckInputEnable();
    i32 ProcessInput();

    struct ChainElem *calcChain;
    struct ChainElem *drawChain;
    i32 waitFramesCounter;
    i32 enableInput;
    i32 cursor;
    i32 selectedIdx;
    i32 listingOffset;
    i32 numDescriptors;
    TrackDescriptor *trackDescriptors;
    AnmVm vm;
    AnmVm titleSprites[31];
    AnmVm descriptionSprites[8];
};
C_ASSERT(sizeof(MusicRoom) == 0x5c04);

#pragma optimize("s", off)
