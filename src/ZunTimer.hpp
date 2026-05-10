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

    i32 GetCurrent()
    {
        return this->current;
    }

    f32 AsFloat()
    {
        return (f32)this->current + this->subFrame;
    }

    void operator=(i32 current)
    {
        this->current = current;
        this->subFrame = 0.0f;
        this->previous = -999;
    }

    void operator+=(i32 value)
    {
        this->Increment(value);
    }

    void operator-=(i32 value)
    {
        this->Decrement(value);
    }

    i32 operator==(i32 value)
    {
        return this->current == value;
    }

    i32 operator!=(i32 value)
    {
        return this->current != value;
    }

    i32 operator<(i32 value)
    {
        return this->current < value;
    }

    i32 operator<=(i32 value)
    {
        return this->current <= value;
    }

    i32 operator>(i32 value)
    {
        return this->current > value;
    }

    i32 operator>=(i32 value)
    {
        return this->current >= value;
    }

    friend i32 operator<(i32 value, ZunTimer& timer)
    {
        return timer.current < value;
    }

    void operator++(int)
    {
        this->Tick();
    }

    void operator--(int)
    {
        this->Decrement(1);
    }

    i32 HasTicked()
    {
        return this->current != this->previous;
    }

    i32 HasTickedAndIsEq(i32 value)
    {
        return this->current != this->previous && this->current == value;
    }

    void Decrement(i32 value);
    void Increment(i32 value);
    i32 NextTick();
};
