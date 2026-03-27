#include "AnmVm.hpp"

#include <stddef.h>

#include "Supervisor.hpp"

// this is here i guess? but why
// FUNCTION: TH07 0x004010b0
i32 ZunTimer::NextTick()

{
  this->previous = this->current;
  g_Supervisor.TickTimer(&this->current, &this->subFrame);
  return this->current;
}

// FUNCTION: TH07 0x004010f0
void AnmVm::Initialize()

{
  memset(this, 0, offsetof(AnmVm, pos));
  (this->scale).x = 1.0f;
  (this->scale).y = 1.0f;
  (this->color).color = 0xffffffff;
  D3DXMatrixIdentity(&this->matrix);
  *(u16 *)&this->flags = 7;
  this->currentTimeInScript.Initialize(0);
}
