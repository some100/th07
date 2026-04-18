#pragma once

#include "inttypes.hpp"

struct ZunTimer
{
    i32 previous;
    f32 subFrame;
    i32 current;

    ZunTimer()
    {
        this->Initialize(0);
    }

    inline void Initialize(i32 current)
    {
        this->current = current;
        this->previous = -999;
        this->subFrame = 0.0f;
    }

    void InitializeForPopup()
    {
        this->current = 0;
        this->subFrame = 0.0f;
        this->previous = -999;
    }

    inline void Initialize2(i32 current)
    {
        this->current = current;
        this->subFrame = 0.0f;
        this->previous = -999;
    }

    void Decrement(i32 value);
    void Increment(i32 value);
    i32 NextTick();
};
