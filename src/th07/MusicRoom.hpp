#pragma once

#include <string.h>

#include "AnmManager.hpp"
#include "Chain.hpp"
#include "ZunBool.hpp"

#define MAX_TRACK_DESCRIPTORS 32

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

    ChainElem *calcChain;
    ChainElem *drawChain;
    i32 waitFramesCounter;
    ZunBool enableInput;
    i32 cursor;
    i32 selectedIdx;
    i32 listingOffset;
    i32 numDescriptors;
    TrackDescriptor *trackDescriptors;
    AnmVm vm[1]; // ZUN quirk: WHY is this an array
    AnmVm titleSprites[31];
    AnmVm descriptionSprites[8];
};
C_ASSERT(sizeof(MusicRoom) == 0x5c04);
