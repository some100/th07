#pragma once

#include <cstring>

#include "AnmVm.hpp"
#include "Chain.hpp"

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

    void UpdatePrev()
    {
        this->vm[0].UpdatePrev();
        for (i32 i = 0; i < 31; i++)
        {
            this->titleSprites[i].UpdatePrev();
        }
        for (i32 i = 0; i < 8; i++)
        {
            this->descriptionSprites[i].UpdatePrev();
        }
    }

    ChainElem *calcChain;
    ChainElem *drawChain;
    i32 waitFramesCounter;
    i32 enableInput;
    i32 cursor;
    i32 selectedIdx;
    i32 listingOffset;
    i32 numDescriptors;
    TrackDescriptor *trackDescriptors;
    AnmVm vm[1]; // ZUN quirk: WHY is this an array
    AnmVm titleSprites[31];
    AnmVm descriptionSprites[8];
};
