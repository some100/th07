#pragma once

#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_surface.h>

#include "ZunResult.hpp"
#include "graphics/ZunGraphics.hpp"
#include "inttypes.hpp"

struct TextHelper
{
    TextHelper();
    ~TextHelper();

    bool AllocateBuffer(i32 width, i32 height);
    bool ReleaseBuffer();
    bool InvertAlpha(i32 x, i32 y, i32 spriteWidth, i32 fontHeight, i32 param5);
    bool CopyTextToTexture(i32 yPos, i32 spriteWidth, i32 spriteHeight, i32 fontHeight,
                           i32 fontWidth, GfxTextureHandle outTexture);

    static ZunResult CreateTextBuffer();
    static void ReleaseTextBuffer();
    static void RenderTextToTextureBold(i32 xPos, i32 yPos, i32 spriteWidth, i32 spriteHeight,
                                        i32 fontHeight, i32 fontWidth, u32 textColor,
                                        u32 outlineType, char *string, GfxTextureHandle outTexture);
    static i32 GetLogicalStringWidth(const char *str);

    SDL_Surface *buffer;
    i32 width;
    i32 height;
};
