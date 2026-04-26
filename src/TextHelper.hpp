#pragma once

#include <d3d8.h>

#include "inttypes.hpp"

struct FormatInfo
{
    D3DFORMAT format;
    i32 bitCount;
    u32 alphaMask;
    u32 redMask;
    u32 greenMask;
    u32 blueMask;
};

struct TextHelper
{
    TextHelper();
    ~TextHelper();
    bool AllocateBufferWithFallback(i32 width, i32 height, D3DFORMAT format);
    bool CopyTextToSurface(IDirect3DSurface8 *surface);
    static void CreateTextBuffer();
    FormatInfo *GetFormatInfo(D3DFORMAT format);
    bool InvertAlpha(i32 x, i32 y, i32 spriteWidth, i32 fontHeight, i32 someFlag);
    bool ReleaseBuffer();
    static void ReleaseTextBuffer();
    static void RenderTextToTextureBold(i32 xPos, i32 yPos, i32 spriteWidth,
                                        i32 spriteHeight, i32 fontHeight,
                                        i32 fontWidth, D3DCOLOR textColor,
                                        u32 outlineType, char *string,
                                        IDirect3DTexture8 *outTexture);
    bool TryAllocateBuffer(i32 width, i32 height, D3DFORMAT format);
    
    D3DFORMAT GetFormat()
    {
        return this->format;
    }
    
    i32 GetWidth()
    {
        return this->width;
    }
    
    i32 GetHeight()
    {
        return this->height;
    }

    D3DFORMAT format;
    i32 width;
    i32 height;
    u32 imageSizeInBytes;
    i32 imageWidthInBytes;
    HDC hdc;
    HGDIOBJ gdiobj;
    HGDIOBJ gdiobj2;
    u16 *buffer;
};
