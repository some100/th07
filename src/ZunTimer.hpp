#pragma once

#include "Supervisor.hpp"
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

    void Initialize(i32 current)
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

    void Tick()
    {
        this->previous = this->current;
        g_Supervisor.TickTimer(&this->current, &this->subFrame);
    }

    void Initialize2(i32 current)
    {
        this->current = current;
        this->subFrame = 0.0f;
        this->previous = -999;
    }

    i32 GetCurrent()
    {
        return this->current;
    }

    void Decrement(i32 value);
    void Increment(i32 value);
    i32 NextTick();
};
