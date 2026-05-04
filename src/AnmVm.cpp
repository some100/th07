#include "AnmVm.hpp"

#include <stddef.h>

// GLOBAL: TH07 0x0049512c
const D3DFORMAT g_TextureFormatD3D8Mapping[6] = {
    D3DFMT_UNKNOWN,
    D3DFMT_A8R8G8B8,
    D3DFMT_A1R5G5B5,
    D3DFMT_R5G6B5,
    D3DFMT_R8G8B8,
    D3DFMT_A4R4G4B4,
};

// GLOBAL: TH07 0x00495144
const i32 g_TextureBytesPerPixel[7] = {4, 4, 2, 2, 3, 2, 0};

// this is here i guess? but why
// FUNCTION: TH07 0x004010b0
i32 ZunTimer::NextTick()
{
    this->Tick();
    return this->current;
}

// FUNCTION: TH07 0x004010f0
void AnmVm::Initialize()
{
    memset(this, 0, offsetof(AnmVm, pos));
    this->scale.x = 1.0f;
    this->scale.y = 1.0f;
    (this->color).color = 0xffffffff;
    D3DXMatrixIdentity(&this->matrix);
    *(u16 *)&this->flags = 7;
    this->currentTimeInScript.Initialize(0);
}
