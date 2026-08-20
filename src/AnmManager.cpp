#include "AnmManager.hpp"

#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "AnmVm.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameWindow.hpp"
#include "Rng.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "TextHelper.hpp"
#include "ZunMath.hpp"
#include "graphics/ZunGraphics.hpp"
#include "utils.hpp"

AnmManager *g_AnmManager;

VertexTex1DiffuseXyzrhw g_QuadVertices[4];

VertexTex1Xyzrhw g_QuadTemplate[4];

VertexTex1DiffuseXyz g_Quad3DFallback[4];

AnmManager::AnmManager()
{
    memset((void *)this, 0, sizeof(AnmManager));

    for (i32 i = 0; i < ARRAY_SIZE_SIGNED(this->sprites); i++)
    {
        this->sprites[i].sourceFileIndex = -1;
    }
    g_QuadTemplate[0].w = g_QuadTemplate[1].w = g_QuadTemplate[2].w = g_QuadTemplate[3].w = 1.0f;
    g_QuadTemplate[0].textureUV.x = 0.0f;
    g_QuadTemplate[0].textureUV.y = 0.0f;
    g_QuadTemplate[1].textureUV.x = 1.0f;
    g_QuadTemplate[1].textureUV.y = 0.0f;
    g_QuadTemplate[2].textureUV.x = 0.0f;
    g_QuadTemplate[2].textureUV.y = 1.0f;
    g_QuadTemplate[3].textureUV.x = 1.0f;
    g_QuadTemplate[3].textureUV.y = 1.0f;
    g_QuadVertices[0].w = g_QuadVertices[1].w = g_QuadVertices[2].w = g_QuadVertices[3].w = 1.0f;
    g_QuadVertices[0].textureUV.x = 0.0f;
    g_QuadVertices[0].textureUV.y = 0.0f;
    g_QuadVertices[1].textureUV.x = 1.0f;
    g_QuadVertices[1].textureUV.y = 0.0f;
    g_QuadVertices[2].textureUV.x = 0.0f;
    g_QuadVertices[2].textureUV.y = 1.0f;
    g_QuadVertices[3].textureUV.x = 1.0f;
    g_QuadVertices[3].textureUV.y = 1.0f;

    this->currentTexture = 0;
    this->currentBlendMode = 0;
    this->currentColorOp = 0;
    this->currentTextureFactor.color = 1;
    this->currentVertexShader = 0;
    this->currentCameraMode = 255;
    this->currentZWriteDisable = 0;
    this->screenshotTextureId = -1;
}

AnmManager::~AnmManager()
{
}

void AnmManager::SetupVertexBuffer()
{
    this->vertexBufferContents[2].pos.x = -128.0f;
    this->vertexBufferContents[0].pos.x = -128.0f;
    this->vertexBufferContents[3].pos.x = 128.0f;
    this->vertexBufferContents[1].pos.x = 128.0f;
    this->vertexBufferContents[1].pos.y = -128.0f;
    this->vertexBufferContents[0].pos.y = -128.0f;
    this->vertexBufferContents[3].pos.y = 128.0f;
    this->vertexBufferContents[2].pos.y = 128.0f;
    this->vertexBufferContents[3].pos.z = 0.0f;
    this->vertexBufferContents[2].pos.z = 0.0f;
    this->vertexBufferContents[1].pos.z = 0.0f;
    this->vertexBufferContents[0].pos.z = 0.0f;
    this->vertexBufferContents[2].textureUV.x = 0.0f;
    this->vertexBufferContents[0].textureUV.x = 0.0f;
    this->vertexBufferContents[3].textureUV.x = 1.0f;
    this->vertexBufferContents[1].textureUV.x = 1.0f;
    this->vertexBufferContents[1].textureUV.y = 0.0f;
    this->vertexBufferContents[0].textureUV.y = 0.0f;
    this->vertexBufferContents[3].textureUV.y = 1.0f;
    this->vertexBufferContents[2].textureUV.y = 1.0f;

    g_Quad3DFallback[0].pos = this->vertexBufferContents[0].pos;
    g_Quad3DFallback[1].pos = this->vertexBufferContents[1].pos;
    g_Quad3DFallback[2].pos = this->vertexBufferContents[2].pos;
    g_Quad3DFallback[3].pos = this->vertexBufferContents[3].pos;
    g_Quad3DFallback[0].textureUV.x = this->vertexBufferContents[0].textureUV.x;
    g_Quad3DFallback[0].textureUV.y = this->vertexBufferContents[0].textureUV.y;
    g_Quad3DFallback[1].textureUV.x = this->vertexBufferContents[1].textureUV.x;
    g_Quad3DFallback[1].textureUV.y = this->vertexBufferContents[1].textureUV.y;
    g_Quad3DFallback[2].textureUV.x = this->vertexBufferContents[2].textureUV.x;
    g_Quad3DFallback[2].textureUV.y = this->vertexBufferContents[2].textureUV.y;
    g_Quad3DFallback[3].textureUV.x = this->vertexBufferContents[3].textureUV.x;
    g_Quad3DFallback[3].textureUV.y = this->vertexBufferContents[3].textureUV.y;
}

ZunResult AnmManager::LoadTexture(i32 textureIdx, const char *texturePath, u32 colorKey)
{
    u8 *srcData;

    ReleaseTexture(textureIdx);

    srcData = FileSystem::OpenFile(texturePath, 1);
    if (!srcData)
    {
        return ZUN_ERROR;
    }

    SDL_IOStream *rw = SDL_IOFromMem(srcData, g_LastFileSize);
    SDL_Surface *surface = IMG_Load_IO(rw, 1);
    free(srcData);

    if (!surface)
    {
        return ZUN_ERROR;
    }

    if (colorKey != 0)
    {
        SDL_SetSurfaceColorKey(surface, true,
                               SDL_MapRGB(SDL_GetPixelFormatDetails(surface->format), NULL,
                                          (colorKey >> 16) & 0xFF, (colorKey >> 8) & 0xFF,
                                          colorKey & 0xFF));
    }

    SDL_Surface *converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);

    this->textures[textureIdx] = g_Supervisor.gfxDevice->CreateTexture();
    g_Supervisor.gfxDevice->BindTexture(this->textures[textureIdx]);
    g_Supervisor.gfxDevice->SetTextureImage(converted->w, converted->h, PIXEL_RGBA,
                                            PIXEL_UNSIGNED_BYTE, converted->pixels);

    this->imageDataArray[textureIdx] = malloc(converted->pitch * converted->h);
    memcpy(this->imageDataArray[textureIdx], converted->pixels, converted->pitch * converted->h);

    textureWidths[textureIdx] = converted->w;
    textureHeights[textureIdx] = converted->h;
    texturePitches[textureIdx] = converted->pitch;

    SDL_DestroySurface(converted);
    return ZUN_SUCCESS;
}

ZunResult AnmManager::LoadTextureEmbedded(u32 textureIdx, ZunImageInfoEmbedded *imageInfo)
{
    SDL_Surface *surface;

    ReleaseTexture(textureIdx);

    u32 bpp = g_TextureBytesPerPixel[imageInfo->format];
    u32 depth = bpp * 8;
    u32 pitch = imageInfo->width * bpp;

    SDL_PixelFormat sdlFormat = SDL_PIXELFORMAT_UNKNOWN;
    switch (imageInfo->format)
    {
    case 1:
        sdlFormat = SDL_PIXELFORMAT_ARGB8888;
        break;
    case 2:
        sdlFormat = SDL_PIXELFORMAT_ARGB1555;
        break;
    case 3:
        sdlFormat = SDL_PIXELFORMAT_RGB565;
        break;
    case 4:
        sdlFormat = SDL_PIXELFORMAT_BGR24;
        break;
    case 5:
        sdlFormat = SDL_PIXELFORMAT_ARGB4444;
        break;
    default:
        return ZUN_ERROR;
    }

    surface = SDL_CreateSurfaceFrom(imageInfo->width, imageInfo->height, sdlFormat, imageInfo->data,
                                    pitch);
    if (!surface)
    {
        return ZUN_ERROR;
    }

    SDL_Surface *converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);

    if (!converted)
    {
        return ZUN_ERROR;
    }

    this->textures[textureIdx] = g_Supervisor.gfxDevice->CreateTexture();
    g_Supervisor.gfxDevice->BindTexture(this->textures[textureIdx]);
    g_Supervisor.gfxDevice->SetTextureImage(converted->w, converted->h, PIXEL_RGBA,
                                            PIXEL_UNSIGNED_BYTE, converted->pixels);

    this->imageDataArray[textureIdx] = malloc(converted->pitch * converted->h);
    memcpy(this->imageDataArray[textureIdx], converted->pixels, converted->pitch * converted->h);

    textureWidths[textureIdx] = converted->w;
    textureHeights[textureIdx] = converted->h;
    texturePitches[textureIdx] = converted->pitch;

    SDL_DestroySurface(converted);
    return ZUN_SUCCESS;
}

ZunResult AnmManager::LoadTextureAlphaChannel(i32 textureIdx, const char *texturePath)
{
    u8 *data;

    u8 *basePixels = (u8 *)this->imageDataArray[textureIdx];
    if (!basePixels)
    {
        return ZUN_ERROR;
    }

    data = FileSystem::OpenFile(texturePath, 0);
    if (!data)
    {
        return ZUN_ERROR;
    }
    SDL_IOStream *rw = SDL_IOFromMem(data, g_LastFileSize);
    SDL_Surface *alphaSurface = IMG_Load_IO(rw, 1);
    free(data);

    if (!alphaSurface)
    {
        return ZUN_ERROR;
    }

    if (alphaSurface->w != (i32)this->textureWidths[textureIdx] ||
        alphaSurface->h != (i32)this->textureHeights[textureIdx])
    {
        SDL_DestroySurface(alphaSurface);
        return ZUN_ERROR;
    }

    SDL_Surface *converted = SDL_ConvertSurface(alphaSurface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(alphaSurface);

    if (!converted)
    {
        return ZUN_ERROR;
    }

    u8 *alphaPixels = (u8 *)converted->pixels;

    for (u32 y = 0; y < this->textureHeights[textureIdx]; y++)
    {
        for (u32 x = 0; x < this->textureWidths[textureIdx]; x++)
        {
            u32 baseOffset = (y * this->texturePitches[textureIdx]) + (x * 4);
            u32 alphaOffset = (y * converted->pitch) + (x * 4);

            basePixels[baseOffset + 3] = alphaPixels[alphaOffset + 2];
        }
    }

    g_Supervisor.gfxDevice->BindTexture(this->textures[textureIdx]);
    g_Supervisor.gfxDevice->SetTextureImage(this->textureWidths[textureIdx],
                                            this->textureHeights[textureIdx], PIXEL_RGBA,
                                            PIXEL_UNSIGNED_BYTE, basePixels);

    SDL_DestroySurface(converted);
    return ZUN_SUCCESS;
}

ZunResult AnmManager::CreateEmptyTexture(i32 textureIdx, u32 width, u32 height)
{
    ReleaseTexture(textureIdx);
    this->textures[textureIdx] = g_Supervisor.gfxDevice->CreateTexture();

    void *emptyData = calloc(width * height, 4);
    g_Supervisor.gfxDevice->BindTexture(this->textures[textureIdx]);
    g_Supervisor.gfxDevice->SetTextureImage(width, height, PIXEL_RGBA, PIXEL_UNSIGNED_BYTE,
                                            emptyData);

    this->imageDataArray[textureIdx] = emptyData;

    textureWidths[textureIdx] = width;
    textureHeights[textureIdx] = height;
    texturePitches[textureIdx] = width * 4;

    return ZUN_SUCCESS;
}

i32 AnmManager::LoadAnms(i32 anmIdx, const char *path, i32 spriteIdxOffset)
{
    i32 res;
    u32 ownsMemory;
    AnmRawEntry *entry;

    entry = (AnmRawEntry *)FileSystem::OpenFile(path, 0);
    ownsMemory = 1;
    i32 startIdx = anmIdx;
    if (!entry)
    {
        g_GameErrorContext.Fatal("アニメが読み込めません。データが失われてるか壊れています\n");
        return ZUN_ERROR;
    }
    while (true)
    {
        res = LoadAnm(anmIdx, entry, spriteIdxOffset, ownsMemory);
        if (res < 0)
        {
            this->anmFiles[startIdx].childCount = anmIdx - startIdx;
            return res;
        }
        anmIdx++;
        if (entry->nextOffset == 0)
        {
            this->anmFiles[startIdx].childCount = anmIdx - startIdx;
            return ZUN_SUCCESS;
        }
        entry = (AnmRawEntry *)((u8 *)entry + entry->nextOffset);
        ownsMemory = 0;
        spriteIdxOffset = spriteIdxOffset + res;
    }
}

i32 AnmManager::LoadAnm(i32 textureIdx, AnmRawEntry *rawEntry, i32 spriteIdxOffset, u32 ownsMemory)
{
    char *name;
    AnmRawSprite *rawSprite;
    AnmLoadedSprite loadedSprite;
    i32 *curSprite;
    i32 i;
    AnmRawEntry *data;
    i32 id;

    id = 0;
    if (!rawEntry)
    {
        g_GameErrorContext.Fatal("アニメが読み込めません。データが失われてるか壊れています\n");
        return ZUN_ERROR;
    }
    if (textureIdx >= ARRAY_SIZE_SIGNED(this->anmFiles))
    {
        g_GameErrorContext.Fatal("テクスチャ格納先が足りません\n");
        return ZUN_ERROR;
    }
    ReleaseAnm(textureIdx);
    data = rawEntry;
    if (data->version != 2)
    {
        g_GameErrorContext.Fatal("アニメのバージョンが違います\n");
        return ZUN_ERROR;
    }
    data->textureIdx = textureIdx;
    data->ownsMemory = ownsMemory;
    if (!data->hasData)
    {
        name = (char *)((u8 *)data + data->nameOffset);
        if (*name == '@')
        {
            CreateEmptyTexture(data->textureIdx, data->width, data->height);
        }
        else
        {
            if (LoadTexture(data->textureIdx, name, data->format) != ZUN_SUCCESS)
            {
                g_GameErrorContext.Fatal(
                    "テクスチャ %s が読み込めません。データが失われてるか壊れています\n", name);
                return ZUN_ERROR;
            }
        }
        if (data->mipmapNameOffset != 0)
        {
            name = (char *)((u8 *)data + data->mipmapNameOffset);
            if (LoadTextureAlphaChannel(data->textureIdx, name) != ZUN_SUCCESS)
            {
                g_GameErrorContext.Fatal(
                    "テクスチャ %s が読み込めません。データが失われてるか壊れています\n", name);
                return ZUN_ERROR;
            }
        }
    }
    else
    {
        if (LoadTextureEmbedded(data->textureIdx,
                                (ZunImageInfoEmbedded *)((u8 *)data + data->textureOffset)) !=
            ZUN_SUCCESS)
        {
            g_GameErrorContext.Fatal(
                "テクスチャが読み込めません。データが失われてるか壊れています\n");
            return ZUN_ERROR;
        }
    }
    this->textureNames[textureIdx] = (char *)((u8 *)data + data->nameOffset);

    u32 texWidth = this->textureWidths[textureIdx] ? this->textureWidths[textureIdx] : data->width;
    u32 texHeight =
        this->textureHeights[textureIdx] ? this->textureHeights[textureIdx] : data->height;

    data->spriteIdxOffset = spriteIdxOffset;
    curSprite = data->dataOffsets;
    for (i = 0; i < data->numSprites; i++, curSprite++)
    {
        rawSprite = (AnmRawSprite *)((u8 *)data + *curSprite);
        loadedSprite.sourceFileIndex = data->textureIdx;
        loadedSprite.cols = (f32)texWidth / (f32)data->width;
        loadedSprite.rows = (f32)texHeight / (f32)data->height;
        loadedSprite.startPixelInclusive.x = loadedSprite.cols * rawSprite->offset.x;
        loadedSprite.startPixelInclusive.y = loadedSprite.rows * rawSprite->offset.y;
        loadedSprite.endPixelInclusive.x =
            (rawSprite->offset.x + rawSprite->size.x) * loadedSprite.cols;
        loadedSprite.endPixelInclusive.y =
            (rawSprite->offset.y + rawSprite->size.y) * loadedSprite.rows;
        loadedSprite.textureWidth = (f32)texWidth;
        loadedSprite.textureHeight = (f32)texHeight;
        if (id < rawSprite->id)
        {
            id = rawSprite->id;
        }
        if (rawSprite->id + spriteIdxOffset >= ARRAY_SIZE_SIGNED(this->sprites))
        {
            g_GameErrorContext.Fatal("スプライトが格納できません。テーブルが不足しています\n");
            return ZUN_ERROR;
        }
        LoadSprite(rawSprite->id + spriteIdxOffset, &loadedSprite);
    }
    for (i = 0; i < data->numScripts; i++, curSprite += 2)
    {
        if (*curSprite + spriteIdxOffset >= ARRAY_SIZE_SIGNED(this->sprites))
        {
            g_GameErrorContext.Fatal("アニメが格納できません。テーブルが不足しています\n");
            return ZUN_ERROR;
        }
        if (id < *curSprite)
        {
            id = *curSprite;
        }
        this->scripts[*curSprite + spriteIdxOffset] = (AnmRawInstr *)((u8 *)data + curSprite[1]);
        this->spriteIndices[*curSprite + spriteIdxOffset] = spriteIdxOffset;
    }
    this->anmFiles[textureIdx].raw = data;
    this->anmFiles[textureIdx].spriteIndexOffset = spriteIdxOffset;
    return id + 1;
}

void AnmManager::ReleaseAnm(i32 anmIdx)
{
    AnmRawEntry *rawEntry;
    i32 *afterHdr;
    i32 uvX;
    i32 i;
    i32 spriteIdxOffset;
    i32 *spriteIdx;

    if (anmIdx < 0 || (u32)anmIdx >= ARRAY_SIZE_SIGNED(this->anmFiles))
    {
        return;
    }

    if (this->anmFiles[anmIdx].raw)
    {
        afterHdr = this->anmFiles[anmIdx].raw->dataOffsets;
        spriteIdxOffset = this->anmFiles[anmIdx].spriteIndexOffset;
        rawEntry = this->anmFiles[anmIdx].raw;
        uvX = anmIdx + 1;
        for (i = 1; i < this->anmFiles[anmIdx].childCount; i++, uvX++)
        {
            ReleaseAnm(uvX);
        }
        for (i = 0; i < rawEntry->numSprites; i++, afterHdr++)
        {
            spriteIdx = (i32 *)((u8 *)rawEntry + *afterHdr);
            memset(&this->sprites[*spriteIdx + spriteIdxOffset], 0, sizeof(AnmLoadedSprite));
            this->sprites[*spriteIdx + spriteIdxOffset].sourceFileIndex = -1;
        }
        for (i = 0; i < rawEntry->numScripts; i++, afterHdr += 2)
        {
            this->scripts[*afterHdr + spriteIdxOffset] = NULL;
            this->spriteIndices[*afterHdr + spriteIdxOffset] = 0;
        }
        this->anmFiles[anmIdx].spriteIndexOffset = 0;
        ReleaseTexture(rawEntry->textureIdx);
        if (rawEntry->ownsMemory)
        {
            free(rawEntry);
        }
        this->anmFiles[anmIdx].raw = NULL;
        this->currentBlendMode = 255;
        this->currentColorOp = 255;
        this->currentVertexShader = 0;
        this->currentTexture = 0;
        this->anmFiles[anmIdx].childCount = 0;
    }
}

void AnmManager::ReleaseTexture(i32 textureIdx)
{
    if (textureIdx < 0 || textureIdx >= ARRAY_SIZE(this->textures))
    {
        return;
    }

    g_Supervisor.gfxDevice->DeleteTexture(this->textures[textureIdx]);
    this->textures[textureIdx].id = 0;
    this->textureWidths[textureIdx] = 0;
    this->textureHeights[textureIdx] = 0;
    this->texturePitches[textureIdx] = 0;

    free(this->imageDataArray[textureIdx]);
    this->imageDataArray[textureIdx] = NULL;
}

void AnmManager::LoadSprite(u32 spriteIdx, AnmLoadedSprite *sprite)
{
    this->sprites[spriteIdx] = *sprite;
    this->sprites[spriteIdx].spriteId = this->loadedSpriteCount++;

    this->sprites[spriteIdx].uvStart.x =
        this->sprites[spriteIdx].startPixelInclusive.x / (this->sprites[spriteIdx].textureWidth);
    this->sprites[spriteIdx].uvEnd.x =
        this->sprites[spriteIdx].endPixelInclusive.x / (this->sprites[spriteIdx].textureWidth);
    this->sprites[spriteIdx].uvStart.y =
        this->sprites[spriteIdx].startPixelInclusive.y / (this->sprites[spriteIdx].textureHeight);
    this->sprites[spriteIdx].uvEnd.y =
        this->sprites[spriteIdx].endPixelInclusive.y / (this->sprites[spriteIdx].textureHeight);
    this->sprites[spriteIdx].widthPx = (this->sprites[spriteIdx].endPixelInclusive.x -
                                        this->sprites[spriteIdx].startPixelInclusive.x) /
                                       sprite->cols;
    this->sprites[spriteIdx].heightPx = (this->sprites[spriteIdx].endPixelInclusive.y -
                                         this->sprites[spriteIdx].startPixelInclusive.y) /
                                        sprite->rows;
}

ZunResult AnmManager::SetActiveSprite(AnmVm *vm, i32 spriteIdx)
{
    if (this->sprites[spriteIdx].sourceFileIndex < 0)
    {
        return ZUN_ERROR;
    }

    vm->activeSpriteIdx = (i16)spriteIdx;
    vm->sprite = &this->sprites[spriteIdx];
    vm->baseTransformMatrix.Identity();
    vm->baseTransformMatrix.m[0][0] = vm->sprite->widthPx / 256.0f;
    vm->baseTransformMatrix.m[1][1] = vm->sprite->heightPx / 256.0f;
    vm->uvMatrix.Identity();
    vm->uvMatrix.m[0][0] = vm->sprite->widthPx / vm->sprite->textureWidth * vm->sprite->cols;
    vm->uvMatrix.m[1][1] = vm->sprite->heightPx / vm->sprite->textureHeight * vm->sprite->rows;
    vm->worldTransformMatrix = vm->matrix = vm->baseTransformMatrix;
    return ZUN_SUCCESS;
}

void AnmManager::SetAndExecuteScript(AnmVm *vm, AnmRawInstr *beginningOfScript)
{
    if (!beginningOfScript)
    {
        memset(vm, 0, sizeof(AnmVm));
    }
    else
    {
        vm->flip = 0;
        vm->Initialize();
        vm->beginningOfScript = beginningOfScript;
        vm->currentInstruction = vm->beginningOfScript;
        vm->currentTimeInScript = 0;
        vm->visible = 0;
        ExecuteScript(vm);
        vm->UpdatePrev();
        this->scriptsExecutedThisFrame++;
    }
}

void AnmManager::SetRenderStateForVm(AnmVm *vm)
{
    ZunColor color;
    ZunColor prevColor;

    if ((u32)this->currentBlendMode != vm->blendMode)
    {
        Flush();
        this->currentBlendMode = vm->blendMode;
        if (!this->currentBlendMode)
        {
            g_Supervisor.gfxDevice->SetBlendMode(BLEND_ALPHA, BLEND_ALPHA);
        }
        else
        {
            g_Supervisor.gfxDevice->SetBlendMode(BLEND_ALPHA, BLEND_ONE);
        }
    }
    color.color = vm->useColor2 ? vm->color2.color : vm->color.color;
    prevColor.color = vm->prevUseColor2 ? vm->prevColor2.color : vm->prevColor.color;

    color = ZunColor::Lerp(prevColor, color, g_RenderAlpha);

    if (!g_Supervisor.cfg.noVertexBuffers)
    {
        if (this->colorMulEnabled)
        {
            color.bytes.r = ZunColor::Multiply(color.bytes.r, this->color.bytes.r);
            color.bytes.g = ZunColor::Multiply(color.bytes.g, this->color.bytes.g);
            color.bytes.b = ZunColor::Multiply(color.bytes.b, this->color.bytes.b);
            color.bytes.a = ZunColor::Multiply(color.bytes.a, this->color.bytes.a);
        }
        if (this->currentTextureFactor.color != color.color)
        {
            this->currentTextureFactor.color = color.color;
            g_Supervisor.gfxDevice->SetTextureFactor(this->currentTextureFactor);
        }
    }
    else
    {
        if (this->colorMulEnabled)
        {
            color.bytes.r = ZunColor::Multiply(color.bytes.r, this->color.bytes.r);
            color.bytes.g = ZunColor::Multiply(color.bytes.g, this->color.bytes.g);
            color.bytes.b = ZunColor::Multiply(color.bytes.b, this->color.bytes.b);
            color.bytes.a = ZunColor::Multiply(color.bytes.a, this->color.bytes.a);
        }
        g_QuadVertices[0].color = color;
        g_QuadVertices[1].color = color;
        g_QuadVertices[2].color = color;
        g_QuadVertices[3].color = color;
        g_Quad3DFallback[0].diffuse = color;
        g_Quad3DFallback[1].diffuse = color;
        g_Quad3DFallback[2].diffuse = color;
        g_Quad3DFallback[3].diffuse = color;
    }
    if (!g_Supervisor.cfg.disableZBuffer && (u32)this->currentZWriteDisable != vm->zWriteDisable)
    {
        Flush();
        this->currentZWriteDisable = vm->zWriteDisable;
        g_Supervisor.gfxDevice->SetDepthMask(this->currentZWriteDisable == 0);
    }
    if ((u32)this->currentCameraMode != vm->cameraMode)
    {
        Flush();
        this->currentCameraMode = vm->cameraMode;
        if (!this->currentCameraMode)
        {
            g_Stage.SetupCameraStageBackground();
            g_Supervisor.gfxDevice->SetViewport(g_Supervisor.viewport);
        }
        else
        {
            g_Stage.UpdateCamera();
            g_Supervisor.gfxDevice->SetViewport(g_Supervisor.viewport);
        }
    }
    this->renderStateChangesThisFrame++;
}

void AnmManager::SyncRenderState(AnmVm *vm)
{
    if ((u32)this->currentBlendMode != vm->blendMode)
    {
        Flush();
        this->currentBlendMode = vm->blendMode;
        if (!this->currentBlendMode)
        {
            g_Supervisor.gfxDevice->SetBlendMode(BLEND_ALPHA, BLEND_ALPHA);
        }
        else
        {
            g_Supervisor.gfxDevice->SetBlendMode(BLEND_ALPHA, BLEND_ONE);
        }
    }
    if (!g_Supervisor.cfg.disableZBuffer && (u32)this->currentZWriteDisable != vm->zWriteDisable)
    {
        Flush();
        this->currentZWriteDisable = vm->zWriteDisable;
        g_Supervisor.gfxDevice->SetDepthMask(this->currentZWriteDisable == 0);
    }
    this->renderStateChangesThisFrame++;
}

ZunResult AnmManager::DrawInner(AnmVm *vm, u32 drawFlags)
{
    ZunColor color, prevColor;
    f32 triangleX1, triangleX2, triangleY1, triangleY2;

    Float2 drawUv = vm->prevUvScrollPos.LerpUv(vm->uvScrollPos, g_RenderAlpha);
    g_QuadVertices[0].pos.x += this->offset.x;
    g_QuadVertices[0].pos.y += this->offset.y;
    g_QuadVertices[1].pos.x += this->offset.x;
    g_QuadVertices[1].pos.y += this->offset.y;
    g_QuadVertices[2].pos.x += this->offset.x;
    g_QuadVertices[2].pos.y += this->offset.y;
    g_QuadVertices[3].pos.x += this->offset.x;
    g_QuadVertices[3].pos.y += this->offset.y;

    if ((drawFlags & 1) != 0)
    {
        g_QuadVertices[0].pos.x = floorf(g_QuadVertices[0].pos.x + 0.5f);
        g_QuadVertices[1].pos.x = floorf(g_QuadVertices[1].pos.x + 0.5f);
        g_QuadVertices[0].pos.y = floorf(g_QuadVertices[0].pos.y + 0.5f);
        g_QuadVertices[2].pos.y = floorf(g_QuadVertices[2].pos.y + 0.5f);
        g_QuadVertices[1].pos.y = g_QuadVertices[0].pos.y;
        g_QuadVertices[2].pos.x = g_QuadVertices[0].pos.x;
        g_QuadVertices[3].pos.x = g_QuadVertices[1].pos.x;
        g_QuadVertices[3].pos.y = g_QuadVertices[2].pos.y;
    }

    g_QuadVertices[0].textureUV.x = g_QuadVertices[2].textureUV.x =
        vm->sprite->uvStart.x + drawUv.x;
    g_QuadVertices[1].textureUV.x = g_QuadVertices[3].textureUV.x = vm->sprite->uvEnd.x + drawUv.x;
    g_QuadVertices[0].textureUV.y = g_QuadVertices[1].textureUV.y =
        vm->sprite->uvStart.y + drawUv.y;
    g_QuadVertices[2].textureUV.y = g_QuadVertices[3].textureUV.y = vm->sprite->uvEnd.y + drawUv.y;

    triangleX1 = std::max(g_QuadVertices[0].pos.x, g_QuadVertices[1].pos.x);
    triangleX1 = std::max(g_QuadVertices[2].pos.x, triangleX1);
    triangleX1 = std::max(g_QuadVertices[3].pos.x, triangleX1);

    triangleY1 = std::max(g_QuadVertices[0].pos.y, g_QuadVertices[1].pos.y);
    triangleY1 = std::max(g_QuadVertices[2].pos.y, triangleY1);
    triangleY1 = std::max(g_QuadVertices[3].pos.y, triangleY1);

    triangleX2 = std::min(g_QuadVertices[0].pos.x, g_QuadVertices[1].pos.x);
    triangleX2 = std::min(g_QuadVertices[2].pos.x, triangleX2);
    triangleX2 = std::min(g_QuadVertices[3].pos.x, triangleX2);

    triangleY2 = std::min(g_QuadVertices[0].pos.y, g_QuadVertices[1].pos.y);
    triangleY2 = std::min(g_QuadVertices[2].pos.y, triangleY2);
    triangleY2 = std::min(g_QuadVertices[3].pos.y, triangleY2);

    if (triangleX1 < g_Supervisor.viewport.x || triangleY1 < g_Supervisor.viewport.y ||
        triangleX2 > g_Supervisor.viewport.x + g_Supervisor.viewport.width ||
        triangleY2 > g_Supervisor.viewport.y + g_Supervisor.viewport.height)
    {
        return ZUN_SUCCESS;
    }

    if (this->currentTexture != this->textures[vm->sprite->sourceFileIndex])
    {
        this->currentTexture = this->textures[vm->sprite->sourceFileIndex];
        this->Flush();
        g_Supervisor.gfxDevice->BindTexture(this->currentTexture);
    }
    if (this->currentVertexShader != 1)
    {
        this->Flush();
        this->currentVertexShader = 1;
    }
    if ((drawFlags & 2) == 0)
    {
        color.color = vm->useColor2 ? vm->color2.color : vm->color.color;
        prevColor.color = vm->prevUseColor2 ? vm->prevColor2.color : vm->prevColor.color;

        color = ZunColor::Lerp(prevColor, color, g_RenderAlpha);
        if (this->colorMulEnabled)
        {
            color.bytes.r = ZunColor::Multiply(color.bytes.r, this->color.bytes.r);
            color.bytes.g = ZunColor::Multiply(color.bytes.g, this->color.bytes.g);
            color.bytes.b = ZunColor::Multiply(color.bytes.b, this->color.bytes.b);
            color.bytes.a = ZunColor::Multiply(color.bytes.a, this->color.bytes.a);
        }
        g_QuadVertices[0].color = color;
        g_QuadVertices[1].color = color;
        g_QuadVertices[2].color = color;
        g_QuadVertices[3].color = color;
    }
    SyncRenderState(vm);
    PushSprite(g_QuadVertices);
    return ZUN_SUCCESS;
}

void AnmManager::ResetVertexBuffer()
{
    this->spritesToDraw = 0;
    this->vertexBufferCurPtr = this->spriteVertexBuffer;
    this->vertexBufferStartPtr = this->vertexBufferCurPtr;
}

void AnmManager::Flush()
{
    if (!this->spritesToDraw)
    {
        return;
    }

    g_Supervisor.gfxDevice->SetTextureArg(TEX_ARG_DIFFUSE);
    g_Supervisor.gfxDevice->SetColorOp(COMPONENT_ALPHA, COLOR_OP_MODULATE);
    g_Supervisor.gfxDevice->SetColorOp(COMPONENT_RGB, COLOR_OP_MODULATE);

    g_Supervisor.gfxDevice->DrawPrimitiveUP(PRIM_TRIANGLES, this->spritesToDraw << 1,
                                            this->vertexBufferStartPtr,
                                            sizeof(VertexTex1DiffuseXyzrhw));
    this->vertexBufferStartPtr = this->vertexBufferCurPtr;
    this->spritesToDraw = 0;
    this->flushesThisFrame++;
}

ZunResult AnmManager::PushSprite(VertexTex1DiffuseXyzrhw *spriteVertex)
{
    this->vertexBufferCurPtr[0] = spriteVertex[0];
    this->vertexBufferCurPtr[1] = spriteVertex[1];
    this->vertexBufferCurPtr[2] = spriteVertex[2];
    this->vertexBufferCurPtr[3] = spriteVertex[1];
    this->vertexBufferCurPtr[4] = spriteVertex[2];
    this->vertexBufferCurPtr[5] = spriteVertex[3];

    this->vertexBufferCurPtr = this->vertexBufferCurPtr + 6;
    this->spritesToDraw++;
    return ZUN_SUCCESS;
}

ZunResult AnmManager::DrawNoRotation(AnmVm *vm)
{
    f32 centerY;
    f32 centerX;

    if (!vm->visible)
    {
        return ZUN_ERROR;
    }

    if (!vm->active)
    {
        return ZUN_ERROR;
    }

    if (!vm->color.bytes.a)
    {
        return ZUN_ERROR;
    }

    Float2 drawScale = vm->prevScale.Lerp(vm->scale, g_RenderAlpha);

    centerX = vm->sprite->widthPx * drawScale.x / 2.0f;
    centerY = vm->sprite->heightPx * drawScale.y / 2.0f;

    if ((vm->anchor & 1) == 0)
    {
        g_QuadVertices[0].pos.x = g_QuadVertices[2].pos.x = vm->pos.x - centerX;
        g_QuadVertices[1].pos.x = g_QuadVertices[3].pos.x = centerX + vm->pos.x;
    }
    else
    {
        g_QuadVertices[0].pos.x = g_QuadVertices[2].pos.x = vm->pos.x;
        g_QuadVertices[1].pos.x = g_QuadVertices[3].pos.x = centerX + vm->pos.x + centerX;
    }

    if ((vm->anchor & 2) == 0)
    {
        g_QuadVertices[0].pos.y = g_QuadVertices[1].pos.y = vm->pos.y - centerY;
        g_QuadVertices[2].pos.y = g_QuadVertices[3].pos.y = centerY + vm->pos.y;
    }
    else
    {
        g_QuadVertices[0].pos.y = g_QuadVertices[1].pos.y = vm->pos.y;
        g_QuadVertices[2].pos.y = g_QuadVertices[3].pos.y = centerY + vm->pos.y + centerY;
    }

    g_QuadVertices[0].pos.z = g_QuadVertices[1].pos.z = g_QuadVertices[2].pos.z =
        g_QuadVertices[3].pos.z = vm->pos.z;

    return DrawInner(vm, 1);
}

void AnmManager::TranslateRotation(VertexTex1DiffuseXyzrhw *vertex, f32 width, f32 height, f32 sine,
                                   f32 cosine, f32 xOffset, f32 yOffset)
{
    vertex->pos.x = width * cosine - height * sine + xOffset;
    vertex->pos.y = width * sine + height * cosine + yOffset;
}

ZunResult AnmManager::Draw(AnmVm *vm)
{
    f32 cosZ;
    f32 sinZ;
    f32 xOffset;
    f32 yOffset;
    f32 z;
    f32 width;
    f32 height;

    if (vm->rotation.z == 0.0f && vm->prevRotation.z == 0.0f)
    {
        return DrawNoRotation(vm);
    }
    if (!vm->visible)
    {
        return ZUN_ERROR;
    }
    if (!vm->active)
    {
        return ZUN_ERROR;
    }
    if (!vm->color.bytes.a)
    {
        return ZUN_ERROR;
    }

    f32 drawRotZ = utils::LerpAngle(vm->prevRotation.z, vm->rotation.z, g_RenderAlpha);
    Float2 drawScale = vm->prevScale.Lerp(vm->scale, g_RenderAlpha);

    z = drawRotZ;
    sincosf(&sinZ, &cosZ, z);
    xOffset = vm->pos.x;
    yOffset = vm->pos.y;
    width = vm->sprite->widthPx * drawScale.x / 2.0f;
    height = vm->sprite->heightPx * drawScale.y / 2.0f;

    TranslateRotation(&g_QuadVertices[0], -width, -height, sinZ, cosZ, xOffset, yOffset);
    TranslateRotation(&g_QuadVertices[1], width, -height, sinZ, cosZ, xOffset, yOffset);
    TranslateRotation(&g_QuadVertices[2], -width, height, sinZ, cosZ, xOffset, yOffset);
    TranslateRotation(&g_QuadVertices[3], width, height, sinZ, cosZ, xOffset, yOffset);

    g_QuadVertices[0].pos.z = g_QuadVertices[1].pos.z = g_QuadVertices[2].pos.z =
        g_QuadVertices[3].pos.z = vm->pos.z;
    if ((vm->anchor & 1) != 0)
    {
        g_QuadVertices[0].pos.x += width;
        g_QuadVertices[1].pos.x += width;
        g_QuadVertices[2].pos.x += width;
        g_QuadVertices[3].pos.x += width;
    }
    if ((vm->anchor & 2) != 0)
    {
        g_QuadVertices[0].pos.y += height;
        g_QuadVertices[1].pos.y += height;
        g_QuadVertices[2].pos.y += height;
        g_QuadVertices[3].pos.y += height;
    }

    return DrawInner(vm, 0);
}

ZunResult AnmManager::DrawFacingCamera(AnmVm *vm)
{
    f32 centerY;
    f32 centerX;

    if (!vm->visible)
    {
        return ZUN_ERROR;
    }
    if (!vm->active)
    {
        return ZUN_ERROR;
    }
    if (!vm->color.bytes.a)
    {
        return ZUN_ERROR;
    }

    Float2 drawScale = vm->prevScale.Lerp(vm->scale, g_RenderAlpha);

    centerX = vm->sprite->widthPx * drawScale.x / 2.0f;
    centerY = vm->sprite->heightPx * drawScale.y / 2.0f;

    if ((vm->anchor & 1) == 0)
    {
        g_QuadVertices[0].pos.x = g_QuadVertices[2].pos.x = vm->pos.x - centerX;
        g_QuadVertices[1].pos.x = g_QuadVertices[3].pos.x = centerX + vm->pos.x;
    }
    else
    {
        g_QuadVertices[0].pos.x = g_QuadVertices[2].pos.x = vm->pos.x;
        g_QuadVertices[1].pos.x = g_QuadVertices[3].pos.x = centerX + vm->pos.x + centerX;
    }

    if ((vm->anchor & 2) == 0)
    {
        g_QuadVertices[0].pos.y = g_QuadVertices[1].pos.y = vm->pos.y - centerY;
        g_QuadVertices[2].pos.y = g_QuadVertices[3].pos.y = centerY + vm->pos.y;
    }
    else
    {
        g_QuadVertices[0].pos.y = g_QuadVertices[1].pos.y = vm->pos.y;
        g_QuadVertices[2].pos.y = g_QuadVertices[3].pos.y = centerY + vm->pos.y + centerY;
    }

    g_QuadVertices[0].pos.z = g_QuadVertices[1].pos.z = g_QuadVertices[2].pos.z =
        g_QuadVertices[3].pos.z = vm->pos.z;

    return DrawInner(vm, 0);
}

ZunResult AnmManager::CalcBillboardTransform(AnmVm *vm)
{
    f32 halfWidth;
    f32 halfHeight;
    f32 screenCenterY;
    f32 halfLength; // also used as screen center x
    f32 sinZ;
    ZunMatrix matrix;
    f32 z = vm->rotation.z;
    ZunVec3 projectRight;
    ZunVec3 projectCenter;
    ZunVec3 projectRightOffset;
    f32 cosZ;

    f32 drawRotZ = utils::LerpAngle(vm->prevRotation.z, vm->rotation.z, g_RenderAlpha);
    Float2 drawScale = vm->prevScale.Lerp(vm->scale, g_RenderAlpha);

    sincosf(&sinZ, &cosZ, drawRotZ);

    ZunVec3 origin(0.0f, 0.0f, 0.0f);

    matrix.Identity();
    matrix.m[3][0] = vm->pos.x;
    matrix.m[3][1] = vm->pos.y;
    matrix.m[3][2] = vm->pos.z;

    ZunMatrix wvp = matrix * g_Supervisor.viewProjectionMatrix;
    projectCenter.Project(&origin, &g_Supervisor.viewport, &wvp);

    if (projectCenter.z < 0.0f || projectCenter.z > 1.0f)
    {
        return ZUN_ERROR;
    }

    projectRight.Project(&g_Stage.cam.right, &g_Supervisor.viewport, &wvp);

    projectRightOffset = projectRight - projectCenter;

    halfLength = projectRightOffset.Length() * 0.5f;
    halfWidth = halfLength * vm->sprite->widthPx * drawScale.x;
    halfHeight = halfLength * vm->sprite->heightPx * drawScale.y;

    halfLength = projectCenter.x; // used as screen center x here
    screenCenterY = projectCenter.y;

    TranslateRotation(&g_QuadVertices[0], -halfWidth, -halfHeight, sinZ, cosZ, halfLength,
                      screenCenterY);
    TranslateRotation(&g_QuadVertices[1], halfWidth, -halfHeight, sinZ, cosZ, halfLength,
                      screenCenterY);
    TranslateRotation(&g_QuadVertices[2], -halfWidth, halfHeight, sinZ, cosZ, halfLength,
                      screenCenterY);
    TranslateRotation(&g_QuadVertices[3], halfWidth, halfHeight, sinZ, cosZ, halfLength,
                      screenCenterY);

    g_QuadVertices[0].pos.z = g_QuadVertices[1].pos.z = g_QuadVertices[2].pos.z =
        g_QuadVertices[3].pos.z = projectCenter.z;

    if ((vm->anchor & 1) != 0)
    {
        g_QuadVertices[0].pos.x += halfWidth;
        g_QuadVertices[1].pos.x += halfWidth;
        g_QuadVertices[2].pos.x += halfWidth;
        g_QuadVertices[3].pos.x += halfWidth;
    }
    if ((vm->anchor & 2) != 0)
    {
        g_QuadVertices[0].pos.y += halfHeight;
        g_QuadVertices[1].pos.y += halfHeight;
        g_QuadVertices[2].pos.y += halfHeight;
        g_QuadVertices[3].pos.y += halfHeight;
    }
    return ZUN_SUCCESS;
}

ZunResult AnmManager::DrawBillboard(AnmVm *vm)
{
    if (!vm->visible)
    {
        return ZUN_ERROR;
    }

    if (!vm->active)
    {
        return ZUN_ERROR;
    }

    if (!vm->color.bytes.a)
    {
        return ZUN_ERROR;
    }

    if (CalcBillboardTransform(vm) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    return DrawInner(vm, 0);
}

static ZunMatrix BuildInterpolatedTransform(const AnmVm *vm)
{
    const Float2 drawScale = vm->prevScale.Lerp(vm->scale, g_RenderAlpha);
    const ZunVec3 drawRotation = {
        utils::LerpAngle(vm->prevRotation.x, vm->rotation.x, g_RenderAlpha),
        utils::LerpAngle(vm->prevRotation.y, vm->rotation.y, g_RenderAlpha),
        utils::LerpAngle(vm->prevRotation.z, vm->rotation.z, g_RenderAlpha),
    };

    ZunMatrix world = vm->baseTransformMatrix;

    world.m[0][0] *= drawScale.x;
    world.m[1][1] *= drawScale.y;

    ZunMatrix rot;

    if (drawRotation.x != 0.0f)
    {
        rot.RotateX(drawRotation.x);
        world *= rot;
    }

    if (drawRotation.y != 0.0f)
    {
        rot.RotateY(drawRotation.y);
        world *= rot;
    }

    if (drawRotation.z != 0.0f)
    {
        rot.RotateZ(drawRotation.z);
        world *= rot;
    }

    return world;
}

void AnmManager::CalcProjectedTransform(AnmVm *vm)
{
    ZunMatrix world;
    ZunMatrix rot;

    if (vm->skipTransform == 0 && (vm->updateScale || vm->updateRotation))
    {
        vm->worldTransformMatrix = vm->baseTransformMatrix;
        vm->worldTransformMatrix.m[0][0] *= vm->scale.x;
        vm->worldTransformMatrix.m[1][1] *= vm->scale.y;
        vm->updateScale = 0;
        if (vm->rotation.x != 0.0)
        {
            rot.RotateX(vm->rotation.x);
            vm->worldTransformMatrix *= rot;
        }
        if (vm->rotation.y != 0.0)
        {
            rot.RotateY(vm->rotation.y);
            vm->worldTransformMatrix *= rot;
        }
        if (vm->rotation.z != 0.0)
        {
            rot.RotateZ(vm->rotation.z);
            vm->worldTransformMatrix *= rot;
        }
        vm->updateRotation = 0;
    }

    Float2 drawScale = vm->prevScale.Lerp(vm->scale, g_RenderAlpha);

    world = vm->skipTransform == 0 ? BuildInterpolatedTransform(vm) : vm->worldTransformMatrix;

    if ((vm->anchor & 1) == 0)
    {
        world.m[3][0] = vm->pos.x;
    }
    else
    {
        world.m[3][0] = fabsf(vm->sprite->widthPx * drawScale.x / 2.0f) + vm->pos.x;
    }

    if ((vm->anchor & 2) == 0)
    {
        world.m[3][1] = vm->pos.y;
    }
    else
    {
        world.m[3][1] = fabsf(vm->sprite->heightPx * drawScale.y / 2.0f) + vm->pos.y;
    }
    world.m[3][2] = vm->pos.z;

    ZunMatrix wvp = world * g_Supervisor.viewProjectionMatrix;

    g_QuadVertices[0].pos.Project(&this->vertexBufferContents[0].pos, &g_Supervisor.viewport, &wvp);
    g_QuadVertices[1].pos.Project(&this->vertexBufferContents[1].pos, &g_Supervisor.viewport, &wvp);
    g_QuadVertices[2].pos.Project(&this->vertexBufferContents[2].pos, &g_Supervisor.viewport, &wvp);
    g_QuadVertices[3].pos.Project(&this->vertexBufferContents[3].pos, &g_Supervisor.viewport, &wvp);
}

ZunResult AnmManager::DrawProjected(AnmVm *vm)
{
    if (!vm->visible)
    {
        return ZUN_ERROR;
    }

    if (!vm->active)
    {
        return ZUN_ERROR;
    }

    if (!vm->color.bytes.a)
    {
        return ZUN_ERROR;
    }

    CalcProjectedTransform(vm);
    return DrawInner(vm, 0);
}

ZunResult AnmManager::Draw3(AnmVm *vm)
{
    ZunMatrix world;
    ZunMatrix rot;
    ZunMatrix uv;

    if (!vm->visible)
    {
        return ZUN_ERROR;
    }

    if (!vm->active)
    {
        return ZUN_ERROR;
    }

    if (!vm->color.bytes.a)
    {
        return ZUN_ERROR;
    }

    if (this->spritesToDraw != 0)
    {
        this->Flush();
    }

    if (vm->skipTransform == 0 && (vm->updateScale || vm->updateRotation))
    {
        vm->worldTransformMatrix = vm->baseTransformMatrix;
        vm->worldTransformMatrix.m[0][0] *= vm->scale.x;
        vm->worldTransformMatrix.m[1][1] *= vm->scale.y;
        vm->updateScale = 0;

        // double intentionally used here
        if (vm->rotation.x != 0.0)
        {
            rot.RotateX(vm->rotation.x);
            vm->worldTransformMatrix *= rot;
        }
        if (vm->rotation.y != 0.0)
        {
            rot.RotateY(vm->rotation.y);
            vm->worldTransformMatrix *= rot;
        }
        if (vm->rotation.z != 0.0)
        {
            rot.RotateZ(vm->rotation.z);
            vm->worldTransformMatrix *= rot;
        }
        vm->updateRotation = 0;
    }

    Float2 drawScale = vm->prevScale.Lerp(vm->scale, g_RenderAlpha);

    world = vm->skipTransform == 0 ? BuildInterpolatedTransform(vm) : vm->worldTransformMatrix;

    if ((vm->anchor & 1) == 0)
    {
        world.m[3][0] = vm->pos.x;
    }
    else
    {
        world.m[3][0] = fabsf(vm->sprite->widthPx * drawScale.x / 2.0f) + vm->pos.x;
    }

    if ((vm->anchor & 2) == 0)
    {
        world.m[3][1] = vm->pos.y;
    }
    else
    {
        world.m[3][1] = fabsf(vm->sprite->heightPx * drawScale.y / 2.0f) + vm->pos.y;
    }

    world.m[3][0] += this->offset.x;
    world.m[3][1] += this->offset.y;

    SetRenderStateForVm(vm);
    world.m[3][2] = vm->pos.z;

    g_Supervisor.gfxDevice->SetTransformMatrix(MATRIX_MODEL, world);

    if (this->currentSprite != vm->sprite)
    {
        this->currentSprite = vm->sprite;
        uv = vm->uvMatrix;

        Float2 drawUv = vm->prevUvScrollPos.LerpUv(vm->uvScrollPos, g_RenderAlpha);

        uv.m[2][0] = vm->sprite->uvStart.x + drawUv.x;
        uv.m[2][1] = vm->sprite->uvStart.y + drawUv.y;

        g_Supervisor.gfxDevice->SetTransformMatrix(MATRIX_TEXTURE, uv);

        if (this->currentTexture != this->textures[vm->sprite->sourceFileIndex])
        {
            this->currentTexture = this->textures[vm->sprite->sourceFileIndex];
            g_Supervisor.gfxDevice->BindTexture(this->currentTexture);
        }
    }

    if (this->currentVertexShader != 2)
    {
        g_Supervisor.gfxDevice->SetColorOp(COMPONENT_ALPHA, COLOR_OP_MODULATE);
        g_Supervisor.gfxDevice->SetColorOp(COMPONENT_RGB, COLOR_OP_MODULATE);
        g_Supervisor.gfxDevice->SetTextureArg(TEX_ARG_TFACTOR);
        this->currentVertexShader = 2;
    }

    if (!g_Supervisor.cfg.noVertexBuffers)
    {
        g_Supervisor.gfxDevice->DrawPrimitive(PRIM_TRIANGLE_STRIP, 0, 2);
    }
    else
    {
        g_Supervisor.gfxDevice->DrawPrimitiveUP(PRIM_TRIANGLE_STRIP, 2, g_Quad3DFallback,
                                                sizeof(VertexTex1DiffuseXyz));
    }
    return ZUN_SUCCESS;
}

f32 AnmVm::GetFloatVarValue(f32 arg)
{
    switch ((i32)arg)
    {
    case ANM_VAR_INT1_1:
        return (f32)this->intVars1[0];
    case ANM_VAR_INT1_2:
        return (f32)this->intVars1[1];
    case ANM_VAR_INT1_3:
        return (f32)this->intVars1[2];
    case ANM_VAR_INT1_4:
        return (f32)this->intVars1[3];
    case ANM_VAR_FLOAT_1:
        return this->floatVars[0];
    case ANM_VAR_FLOAT_2:
        return this->floatVars[1];
    case ANM_VAR_FLOAT_3:
        return this->floatVars[2];
    case ANM_VAR_FLOAT_4:
        return this->floatVars[3];
    case ANM_VAR_INT2_1:
        return (f32)this->intVars2[0];
    case ANM_VAR_INT2_2:
        return (f32)this->intVars2[1];
    default:
        return arg;
    }
}

i32 AnmVm::GetVarValue(i32 arg)
{
    switch (arg)
    {
    case ANM_VAR_INT1_1:
        return this->intVars1[0];
    case ANM_VAR_INT1_2:
        return this->intVars1[1];
    case ANM_VAR_INT1_3:
        return this->intVars1[2];
    case ANM_VAR_INT1_4:
        return this->intVars1[3];
    case ANM_VAR_FLOAT_1:
        return this->floatVars[0];
    case ANM_VAR_FLOAT_2:
        return this->floatVars[1];
    case ANM_VAR_FLOAT_3:
        return this->floatVars[2];
    case ANM_VAR_FLOAT_4:
        return this->floatVars[3];
    case ANM_VAR_INT2_1:
        return this->intVars2[0];
    case ANM_VAR_INT2_2:
        return this->intVars2[1];
    default:
        return arg;
    }
}

f32 *AnmVm::GetFloatVar(f32 *paramId, u16 mask, u32 idx)
{
    if (((u32)mask & 1 << idx) == 0)
    {
        return paramId;
    }

    switch ((u32)*paramId)
    {
    case ANM_VAR_FLOAT_1:
        return &this->floatVars[0];
    case ANM_VAR_FLOAT_2:
        return &this->floatVars[1];
    case ANM_VAR_FLOAT_3:
        return &this->floatVars[2];
    case ANM_VAR_FLOAT_4:
        return &this->floatVars[3];
    default:
        return paramId;
    }
}

i32 *AnmVm::GetVar(i32 *paramId, u16 mask, u32 idx)
{
    if (((u32)mask & 1 << idx) == 0)
    {
        return paramId;
    }

    switch (*paramId)
    {
    case ANM_VAR_INT1_1:
        return &this->intVars1[0];
    case ANM_VAR_INT1_2:
        return &this->intVars1[1];
    case ANM_VAR_INT1_3:
        return &this->intVars1[2];
    case ANM_VAR_INT1_4:
        return &this->intVars1[3];
    case ANM_VAR_INT2_1:
        return &this->intVars2[0];
    case ANM_VAR_INT2_2:
        return &this->intVars2[1];
    default:
        return paramId;
    }
}

i32 AnmManager::ExecuteScript(AnmVm *vm)
{
    AnmRawInstr *instr;
    AnmRawInstr *nextInstr;
    i32 i;
    f32 t;

#define GET_INT_PTR(argIdx) vm->GetVar(&instr->args[argIdx].i, instr->flags, argIdx)

#define GET_FLOAT_PTR(argIdx) vm->GetFloatVar(&instr->args[argIdx].f, instr->flags, argIdx)

#define GET_INT_VALUE(argIdx)                                                                      \
    (((instr->flags & (1 << argIdx)) != 0) ? vm->GetVarValue(instr->args[argIdx].i)                \
                                           : instr->args[argIdx].i)

#define GET_FLOAT_VALUE(argIdx)                                                                    \
    (((instr->flags & (1 << argIdx)) != 0) ? vm->GetFloatVarValue(instr->args[argIdx].f)           \
                                           : instr->args[argIdx].f)

    if (!vm->currentInstruction)
    {
        return 1;
    }

    if (g_SuppressAnmAdvance)
    {
        return 0;
    }

    if (vm->pendingInterrupt != 0)
    {
        goto handle_interrupt;
    }

WHY_NOT_JUST_CONTINUE:
    instr = vm->currentInstruction;
    while (instr->time <= vm->currentTimeInScript.GetCurrent())
    {
        switch (instr->opcode)
        {
        case ANM_EXIT_HIDE:
        case ANM_EXIT_HIDE2:
            vm->visible = 0;
        case ANM_EXIT:
            vm->currentInstruction = NULL;
            return 1;
        case ANM_SET_ACTIVE_SPRITE:
            vm->visible = 1;
            SetActiveSprite(vm, GET_INT_VALUE(0) + this->spriteIndices[vm->anmFileIdx]);
            vm->timeOfLastSpriteSet = vm->currentTimeInScript.GetCurrent();
            break;
        case ANM_SET_SCALE:
            vm->scale.x = GET_FLOAT_VALUE(0);
            vm->scale.y = GET_FLOAT_VALUE(1);
            vm->updateScale = 1;
            break;
        case ANM_SET_ALPHA:
            vm->color.bytes.a = instr->args[0].i & 255;
            break;
        case ANM_SET_COLOR:
            vm->color.color = (vm->color.color & 0xff000000) | (instr->args[0].i & 0xffffff);
            break;
        case ANM_JUMP:
            vm->currentTimeInScript = instr->args[1].i;
            vm->currentInstruction =
                (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[0].i);
            goto WHY_NOT_JUST_CONTINUE;
        case ANM_DEC_JUMP:
            (*GET_INT_PTR(0))--;
            if (GET_INT_VALUE(0) > 0)
            {
                vm->currentTimeInScript = instr->args[2].i;
                vm->currentInstruction =
                    (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[1].i);
                goto WHY_NOT_JUST_CONTINUE;
            }
            break;
        case ANM_FLIP_X:
            vm->flip ^= 1;
            vm->scale.x *= -1.0f;
            vm->updateScale = 1;
            break;
        case ANM_SET_USE_OFFSET:
            vm->useOffset = instr->args[0].i;
            break;
        case ANM_FLIP_Y:
            vm->flip ^= 2;
            vm->scale.y *= -1.0f;
            vm->updateScale = 1;
            break;
        case ANM_SET_ROTATION:
            vm->rotation.x = GET_FLOAT_VALUE(0);
            vm->rotation.y = GET_FLOAT_VALUE(1);
            vm->rotation.z = GET_FLOAT_VALUE(2);
            vm->updateRotation = 1;
            break;
        case ANM_SET_ANGLE_VEL:
            vm->angleVel.x = GET_FLOAT_VALUE(0);
            vm->angleVel.y = GET_FLOAT_VALUE(1);
            vm->angleVel.z = GET_FLOAT_VALUE(2);
            vm->updateRotation = 1;
            break;
        case ANM_SET_SCALE_SPEED:
            vm->scaleGrowth.x = GET_FLOAT_VALUE(0);
            vm->scaleGrowth.y = GET_FLOAT_VALUE(1);
            break;
        case ANM_INTERP_SCALE:
            vm->interpStartTimes[4] = 0;
            vm->interpEndTimes[4] = GET_INT_VALUE(2);
            vm->easeModes[4] = 0;
            vm->scaleInterpInitial = vm->scale;
            vm->scaleInterpFinal.x = GET_FLOAT_VALUE(0);
            vm->scaleInterpFinal.y = GET_FLOAT_VALUE(1);
            break;
        case ANM_FADE:
            vm->colorInterpInitialColor.bytes.a = vm->color.bytes.a;
            vm->colorInterpFinalColor.bytes.a = instr->args[0].b[0];
            vm->interpStartTimes[2] = 0;
            vm->interpEndTimes[2] = GET_INT_VALUE(1);
            vm->easeModes[2] = 0;
            break;
        case ANM_SET_BLEND:
            vm->blendMode = instr->args[0].i;
            break;
        case ANM_SET_TRANSLATION:
            if (!vm->useOffset)
            {
                vm->pos = ZunVec3(GET_FLOAT_VALUE(0), GET_FLOAT_VALUE(1), GET_FLOAT_VALUE(2));
            }
            else
            {
                vm->offset = ZunVec3(GET_FLOAT_VALUE(0), GET_FLOAT_VALUE(1), GET_FLOAT_VALUE(2));
            }
            break;
        case ANM_POS_TIME_ACCEL:
            vm->easeModes[0] = 6;
            goto interp_pos;
        case ANM_POS_TIME_DECEL:
            vm->easeModes[0] = 4;
            goto interp_pos;
        case ANM_POS_TIME_LINEAR:
            vm->easeModes[0] = 0;
        interp_pos:
            if (!vm->useOffset)
            {
                vm->posInterpInitial = vm->pos;
            }
            else
            {
                vm->posInterpInitial = vm->offset;
            }
            vm->posInterpFinal =
                ZunVec3(GET_FLOAT_VALUE(0), GET_FLOAT_VALUE(1), GET_FLOAT_VALUE(2));
            vm->interpEndTimes[0] = GET_INT_VALUE(3);
            vm->interpStartTimes[0] = 0;
            break;
        case ANM_WAIT:
            if (vm->waitTimer == 0)
            {
                vm->waitTimer = GET_INT_VALUE(0);
            }
            else
            {
                vm->waitTimer--;
            }
            if (vm->waitTimer <= 0)
            {
                vm->waitTimer = 0;
                break;
            }
            vm->currentTimeInScript--;
            goto stop;
        case ANM_STOP_HIDE:
            vm->visible = 0;
        case ANM_STOP:
            if (!vm->pendingInterrupt)
            {
                vm->isStopped = 1;
                vm->currentTimeInScript--;
                goto stop;
            }
        handle_interrupt:
            nextInstr = NULL;
            instr = vm->beginningOfScript;
            while ((instr->opcode != ANM_INTERRUPT_LABEL ||
                    (i32)vm->pendingInterrupt != instr->args[0].i) &&
                   instr->opcode != ANM_EXIT_HIDE)
            {
                if (instr->opcode == ANM_INTERRUPT_LABEL && instr->args[0].i == -1)
                {
                    nextInstr = instr;
                }
                instr = (AnmRawInstr *)((u8 *)instr + instr->size);
            }
            vm->pendingInterrupt = 0;
            vm->isStopped = 0;
            if (instr->opcode != ANM_INTERRUPT_LABEL)
            {
                if (!nextInstr)
                {
                    vm->currentTimeInScript--;
                    goto stop;
                }
                instr = nextInstr;
            }

            instr = (AnmRawInstr *)((u8 *)instr + instr->size);
            vm->currentInstruction = instr;
            vm->currentTimeInScript = vm->currentInstruction->time;
            vm->visible = 1;
            goto WHY_NOT_JUST_CONTINUE;
        case ANM_SET_VISIBILITY:
            vm->visible = instr->args[0].i;
            break;
        case ANM_22:
            vm->anchor = 3;
            break;
        case ANM_SET_AUTO_ROTATE:
            vm->autoRotate = instr->args[0].us[0];
            break;
        case ANM_SET_SCROLL_POS_X:
            vm->uvScrollPos.x += GET_FLOAT_VALUE(0);
            if (vm->uvScrollPos.x >= 1.0f)
            {
                vm->uvScrollPos.x -= 1.0f;
            }
            else
            {
                if (vm->uvScrollPos.x < 0.0f)
                {
                    vm->uvScrollPos.x += 1.0f;
                }
            }
            break;
        case ANM_SET_SCROLL_POS_Y:
            vm->uvScrollPos.y += GET_FLOAT_VALUE(0);
            if (vm->uvScrollPos.y >= 1.0f)
            {
                vm->uvScrollPos.y -= 1.0f;
            }
            else
            {
                if (vm->uvScrollPos.y < 0.0f)
                {
                    vm->uvScrollPos.y += 1.0f;
                }
            }
            break;
        case ANM_SET_SCROLLVEL_X:
            vm->uvScrollVel.x = GET_FLOAT_VALUE(0);
            break;
        case ANM_SET_SCROLLVEL_Y:
            vm->uvScrollVel.y = GET_FLOAT_VALUE(0);
            break;
        case ANM_SET_ZWRITE_DISABLE:
            vm->zWriteDisable = instr->args[0].i;
            break;
        case ANM_SET_CAMERA_MODE:
            vm->cameraMode = instr->args[0].i;
            break;
        case ANM_INTERP_POS:
            vm->interpStartTimes[0] = 0;
            vm->interpEndTimes[0] = GET_INT_VALUE(0);
            vm->easeModes[0] = instr->args[1].b[0];
            if (!vm->useOffset)
            {
                vm->posInterpInitial = vm->pos;
            }
            else
            {
                vm->posInterpInitial = vm->offset;
            }
            vm->posInterpFinal.x = GET_FLOAT_VALUE(2);
            vm->posInterpFinal.y = GET_FLOAT_VALUE(3);
            vm->posInterpFinal.z = GET_FLOAT_VALUE(4);
            break;
        case ANM_INTERP_COLOR:
            vm->interpStartTimes[1] = 0;
            vm->interpEndTimes[1] = GET_INT_VALUE(0);
            vm->easeModes[1] = instr->args[1].b[0];
            vm->colorInterpInitialColor.bytes.r = vm->color.bytes.r;
            vm->colorInterpInitialColor.bytes.g = vm->color.bytes.g;
            vm->colorInterpInitialColor.bytes.b = vm->color.bytes.b;
            vm->colorInterpFinalColor.bytes.r = instr->args[2].b[0];
            vm->colorInterpFinalColor.bytes.g = instr->args[2].b[1];
            vm->colorInterpFinalColor.bytes.b = instr->args[2].b[2];
            break;
        case ANM_INTERP_ALPHA:
            vm->interpStartTimes[2] = 0;
            vm->interpEndTimes[2] = GET_INT_VALUE(0);
            vm->easeModes[2] = instr->args[1].b[0];
            vm->colorInterpInitialColor.bytes.a = vm->color.bytes.a;
            vm->colorInterpFinalColor.bytes.a = instr->args[2].b[0];
            break;
        case ANM_INTERP_ROTATE:
            vm->interpStartTimes[3] = 0;
            vm->interpEndTimes[3] = GET_INT_VALUE(0);
            vm->easeModes[3] = instr->args[1].b[0];
            vm->rotateInterpInitial = vm->rotation;
            vm->rotateInterpFinal.x = GET_FLOAT_VALUE(2);
            vm->rotateInterpFinal.y = GET_FLOAT_VALUE(3);
            vm->rotateInterpFinal.z = GET_FLOAT_VALUE(4);
            vm->updateRotation = 1;
            break;
        case ANM_INTERP_SCALE_2:
            vm->interpStartTimes[4] = 0;
            vm->interpEndTimes[4] = GET_INT_VALUE(0);
            vm->easeModes[4] = instr->args[1].b[0];
            vm->scaleInterpInitial = vm->scale;
            vm->scaleInterpFinal.x = GET_FLOAT_VALUE(2);
            vm->scaleInterpFinal.y = GET_FLOAT_VALUE(3);
            vm->updateScale = 1;
            break;
        case ANM_MOV:
            *GET_INT_PTR(0) = GET_INT_VALUE(1);
            break;
        case ANM_MOV_FLOAT:
            *GET_FLOAT_PTR(0) = GET_FLOAT_VALUE(1);
            break;
        case ANM_ADD_2:
            *GET_INT_PTR(0) = GET_INT_VALUE(1) + GET_INT_VALUE(2);
            break;
        case ANM_ADD_FLOAT_2:
            *GET_FLOAT_PTR(0) = GET_FLOAT_VALUE(1) + GET_FLOAT_VALUE(2);
            break;
        case ANM_SUB_2:
            *GET_INT_PTR(0) = GET_INT_VALUE(1) - GET_INT_VALUE(2);
            break;
        case ANM_SUB_FLOAT_2:
            *GET_FLOAT_PTR(0) = GET_FLOAT_VALUE(1) - GET_FLOAT_VALUE(2);
            break;
        case ANM_MUL_2:
            *GET_INT_PTR(0) = GET_INT_VALUE(1) * GET_INT_VALUE(2);
            break;
        case ANM_MUL_FLOAT_2:
            *GET_FLOAT_PTR(0) = GET_FLOAT_VALUE(1) * GET_FLOAT_VALUE(2);
            break;
        case ANM_DIV_2:
            *GET_INT_PTR(0) = GET_INT_VALUE(1) / GET_INT_VALUE(2);
            break;
        case ANM_DIV_FLOAT_2:
            *GET_FLOAT_PTR(0) = GET_FLOAT_VALUE(1) / GET_FLOAT_VALUE(2);
            break;
        case ANM_MOD_2:
            *GET_INT_PTR(0) = GET_INT_VALUE(1) % GET_INT_VALUE(2);
            break;
        case ANM_MOD_FLOAT_2:
            *GET_FLOAT_PTR(0) = fmodf(GET_FLOAT_VALUE(1), GET_FLOAT_VALUE(2));
            break;
        case ANM_ADD:
            *GET_INT_PTR(0) += GET_INT_VALUE(1);
            break;
        case ANM_ADD_FLOAT:
            *GET_FLOAT_PTR(0) += GET_FLOAT_VALUE(1);
            break;
        case ANM_SUB:
            *GET_INT_PTR(0) -= GET_INT_VALUE(1);
            break;
        case ANM_SUB_FLOAT:
            *GET_FLOAT_PTR(0) -= GET_FLOAT_VALUE(1);
            break;
        case ANM_MUL:
            *GET_INT_PTR(0) *= GET_INT_VALUE(1);
            break;
        case ANM_MUL_FLOAT:
            *GET_FLOAT_PTR(0) *= GET_FLOAT_VALUE(1);
            break;
        case ANM_DIV:
            *GET_INT_PTR(0) /= GET_INT_VALUE(1);
            break;
        case ANM_DIV_FLOAT:
            *GET_FLOAT_PTR(0) /= GET_FLOAT_VALUE(1);
            break;
        case ANM_MOD:
            *GET_INT_PTR(0) %= GET_INT_VALUE(1);
            break;
        case ANM_MOD_FLOAT:
            *GET_FLOAT_PTR(0) = fmodf(GET_FLOAT_VALUE(0), GET_FLOAT_VALUE(1));
            break;
        case ANM_RAND:
            *GET_INT_PTR(0) = g_Rng.GetRandomU32InRange(GET_INT_VALUE(1));
            break;
        case ANM_RAND_FLOAT:
            *GET_FLOAT_PTR(0) = g_Rng.GetRandomFloatInRange(GET_FLOAT_VALUE(1));
            break;
        case ANM_SIN:
            *GET_FLOAT_PTR(0) = sinf(GET_FLOAT_VALUE(1));
            break;
        case ANM_COS:
            *GET_FLOAT_PTR(0) = cosf(GET_FLOAT_VALUE(1));
            break;
        case ANM_TAN:
            *GET_FLOAT_PTR(0) = tanf(GET_FLOAT_VALUE(1));
            break;
        case ANM_ACOS:
            *GET_FLOAT_PTR(0) = acosf(GET_FLOAT_VALUE(1));
            break;
        case ANM_ATAN:
            *GET_FLOAT_PTR(0) = atanf(GET_FLOAT_VALUE(1));
            break;
        case ANM_NORMALIZE_ANGLE:
            *GET_FLOAT_PTR(0) = utils::AddNormalizeAngle(GET_FLOAT_VALUE(0), 0.0f);
            break;
        case ANM_JUMP_IF_EQ:
            if (GET_INT_VALUE(0) == GET_INT_VALUE(1))
            {
                goto jump;
            }
            break;
        case ANM_JUMP_IF_EQ_FLOAT:
            if (GET_FLOAT_VALUE(0) == GET_FLOAT_VALUE(1))
            {
                goto jump;
            }
            break;
        case ANM_JUMP_IF_NEQ:
            if (GET_INT_VALUE(0) != GET_INT_VALUE(1))
            {
                goto jump;
            }
            break;
        case ANM_JUMP_IF_NEQ_FLOAT:
            if (GET_FLOAT_VALUE(0) != GET_FLOAT_VALUE(1))
            {
                goto jump;
            }
            break;
        case ANM_JUMP_IF_LT:
            if (GET_INT_VALUE(0) < GET_INT_VALUE(1))
            {
                goto jump;
            }
            break;
        case ANM_JUMP_IF_LT_FLOAT:
            if (GET_FLOAT_VALUE(0) < GET_FLOAT_VALUE(1))
            {
                goto jump;
            }
            break;
        case ANM_JUMP_IF_LEQ:
            if (GET_INT_VALUE(0) <= GET_INT_VALUE(1))
            {
                goto jump;
            }
            break;
        case ANM_JUMP_IF_LEQ_FLOAT:
            if (GET_FLOAT_VALUE(0) <= GET_FLOAT_VALUE(1))
            {
                goto jump;
            }
            break;
        case ANM_JUMP_IF_GT:
            if (GET_INT_VALUE(0) > GET_INT_VALUE(1))
            {
                goto jump;
            }
            break;
        case ANM_JUMP_IF_GT_FLOAT:
            if (GET_FLOAT_VALUE(0) > GET_FLOAT_VALUE(1))
            {
                goto jump;
            }
            break;
        case ANM_JUMP_IF_GEQ:
            if (GET_INT_VALUE(0) >= GET_INT_VALUE(1))
            {
                goto jump;
            }
            break;
        case ANM_JUMP_IF_GEQ_FLOAT:
            if (GET_FLOAT_VALUE(0) >= GET_FLOAT_VALUE(1))
            {
                goto jump;
            }
            break;
        jump:
            vm->currentTimeInScript = instr->args[3].i;
            vm->currentInstruction =
                (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[2].i);
            goto WHY_NOT_JUST_CONTINUE;
        default:
            break;
        }
        vm->currentInstruction = (AnmRawInstr *)((u8 *)instr + instr->size);
        goto WHY_NOT_JUST_CONTINUE;
    }

stop:
    if (vm->angleVel.x != 0.0f)
    {
        vm->rotation.x = utils::AddNormalizeAngle(
            vm->rotation.x, g_Supervisor.effectiveFramerateMultiplier * vm->angleVel.x);
        vm->updateRotation = 1;
    }
    if (vm->angleVel.y != 0.0f)
    {
        vm->rotation.y = utils::AddNormalizeAngle(
            vm->rotation.y, g_Supervisor.effectiveFramerateMultiplier * vm->angleVel.y);
        vm->updateRotation = 1;
    }
    if (vm->angleVel.z != 0.0f)
    {
        vm->rotation.z = utils::AddNormalizeAngle(
            vm->rotation.z, g_Supervisor.effectiveFramerateMultiplier * vm->angleVel.z);
        vm->updateRotation = 1;
    }
    for (i = 0; i < ARRAY_SIZE_SIGNED(vm->interpStartTimes); i++)
    {
        if (vm->interpEndTimes[i] > 0)
        {
            vm->interpStartTimes[i]++;
            if (vm->interpStartTimes[i] >= vm->interpEndTimes[i].GetCurrent())
            {
                t = 1.0f;
                vm->interpEndTimes[i] = 0;
            }
            else
            {
                t = vm->interpStartTimes[i].AsFloat() / vm->interpEndTimes[i].AsFloat();
            }
            switch (vm->easeModes[i])
            {
            case ANM_EASE_IN_QUAD:
                t = t * t;
                break;
            case ANM_EASE_IN_CUBIC:
                t = t * t * t;
                break;
            case ANM_EASE_IN_QUART:
                t = t * t;
                t = t * t;
                break;
            case ANM_EASE_OUT_QUAD:
                t = 1.0f - t;
                t = t * t;
                t = 1.0f - t;
                break;
            case ANM_EASE_OUT_CUBIC:
                t = 1.0f - t;
                t = t * t * t;
                t = 1.0f - t;
                break;
            case ANM_EASE_OUT_QUART:
                t = 1.0f - t;
                t = t * t;
                t = t * t;
                t = 1.0f - t;
                break;
            }
            switch (i)
            {
            case 0:
                if (!vm->useOffset)
                {
                    vm->pos.x = (vm->posInterpFinal.x - vm->posInterpInitial.x) * t +
                                vm->posInterpInitial.x;
                    vm->pos.y = (vm->posInterpFinal.y - vm->posInterpInitial.y) * t +
                                vm->posInterpInitial.y;
                    vm->pos.z = (vm->posInterpFinal.z - vm->posInterpInitial.z) * t +
                                vm->posInterpInitial.z;
                }
                else
                {
                    vm->offset.x = (vm->posInterpFinal.x - vm->posInterpInitial.x) * t +
                                   vm->posInterpInitial.x;
                    vm->offset.y = (vm->posInterpFinal.y - vm->posInterpInitial.y) * t +
                                   vm->posInterpInitial.y;
                    vm->offset.z = (vm->posInterpFinal.z - vm->posInterpInitial.z) * t +
                                   vm->posInterpInitial.z;
                }
                break;
            case 1:
                vm->color.bytes.r = (u8)((f32)((i32)vm->colorInterpFinalColor.bytes.r -
                                               (i32)vm->colorInterpInitialColor.bytes.r) *
                                             t +
                                         (f32)vm->colorInterpInitialColor.bytes.r);
                vm->color.bytes.g = (u8)((f32)((i32)vm->colorInterpFinalColor.bytes.g -
                                               (i32)vm->colorInterpInitialColor.bytes.g) *
                                             t +
                                         (f32)vm->colorInterpInitialColor.bytes.g);
                vm->color.bytes.b = (u8)((f32)((i32)vm->colorInterpFinalColor.bytes.b -
                                               (i32)vm->colorInterpInitialColor.bytes.b) *
                                             t +
                                         (f32)vm->colorInterpInitialColor.bytes.b);
                break;
            case 2:
                vm->color.bytes.a = (u8)((f32)((i32)vm->colorInterpFinalColor.bytes.a -
                                               (i32)vm->colorInterpInitialColor.bytes.a) *
                                             t +
                                         (f32)vm->colorInterpInitialColor.bytes.a);
                break;
            case 3:
                vm->rotation.x = utils::AddNormalizeAngle(
                    (vm->rotateInterpFinal.x - vm->rotateInterpInitial.x) * t,
                    vm->rotateInterpInitial.x);
                vm->rotation.y = utils::AddNormalizeAngle(
                    (vm->rotateInterpFinal.y - vm->rotateInterpInitial.y) * t,
                    vm->rotateInterpInitial.y);
                vm->rotation.z = utils::AddNormalizeAngle(
                    (vm->rotateInterpFinal.z - vm->rotateInterpInitial.z) * t,
                    vm->rotateInterpInitial.z);
                vm->updateRotation = 1;
                break;
            case 4:
                vm->scale.x = (vm->scaleInterpFinal.x - vm->scaleInterpInitial.x) * t +
                              vm->scaleInterpInitial.x;
                vm->scale.y = (vm->scaleInterpFinal.y - vm->scaleInterpInitial.y) * t +
                              vm->scaleInterpInitial.y;
                vm->updateScale = 1;
                break;
            }
        }
    }
    if (vm->scaleGrowth.y != 0.0f)
    {
        vm->scale.y += g_Supervisor.effectiveFramerateMultiplier * vm->scaleGrowth.y;
        vm->updateScale = 1;
    }
    if (vm->scaleGrowth.x != 0.0f)
    {
        vm->scale.x += g_Supervisor.effectiveFramerateMultiplier * vm->scaleGrowth.x;
        vm->updateScale = 1;
        vm->updateRotation = 1;
    }
    vm->uvScrollPos.x += vm->uvScrollVel.x;
    if (vm->uvScrollPos.x >= 1.0f)
    {
        vm->uvScrollPos.x -= 1.0f;
    }
    else if (vm->uvScrollPos.x < 0.0f)
    {
        vm->uvScrollPos.x += 1.0f;
    }
    vm->uvScrollPos.y += vm->uvScrollVel.y;
    if (vm->uvScrollPos.y >= 1.0f)
    {
        vm->uvScrollPos.y -= 1.0f;
    }
    else if (vm->uvScrollPos.y < 0.0f)
    {
        vm->uvScrollPos.y += 1.0f;
    }
    vm->currentTimeInScript++;
    this->scriptTicksThisFrame++;
    return 0;
}

void AnmManager::DrawTextToSprite(u32 spriteDstIdx, i32 x, i32 y, i32 width, i32 height,
                                  i32 fontWidth, i32 fontHeight, u32 textColor, u32 outlineType,
                                  char *strToPrint, f32 scaleY, f32 scaleX)
{
    if (fontWidth <= 0)
    {
        fontWidth = 15;
    }
    if (fontHeight <= 0)
    {
        fontHeight = 15;
    }
    TextHelper::RenderTextToTextureBold(x, y, width, height, (f32)fontWidth * scaleY,
                                        (f32)fontHeight * scaleX, textColor, outlineType,
                                        strToPrint, this->textures[spriteDstIdx]);
}

void AnmManager::DrawVmTextFmt(AnmManager *manager, AnmVm *vm, u32 textColor, u32 outlineType,
                               const char *str, ...)
{
    u32 fontWidth;
    char text[256];
    va_list args;

    fontWidth = vm->fontWidth;

    va_start(args, str);
    vsnprintf(text, sizeof(text), str, args);
    va_end(args);

    manager->DrawTextToSprite(vm->sprite->sourceFileIndex, vm->sprite->startPixelInclusive.x,
                              vm->sprite->startPixelInclusive.y, vm->sprite->textureWidth,
                              vm->sprite->textureHeight, fontWidth, vm->fontHeight, textColor,
                              outlineType, text, vm->sprite->cols, vm->sprite->rows);

    vm->visible = 1;
}

void AnmManager::DrawStringFormat(AnmVm *vm, u32 textColor, u32 outlineType, const char *text, ...)
{
    i32 fontWidth;
    char buf[256];
    i32 x;
    va_list args;

    fontWidth = vm->fontWidth <= 0 ? 15 : (u32)vm->fontWidth;
    va_start(args, text);
    vsnprintf(buf, sizeof(buf), text, args);
    va_end(args);

    this->DrawTextToSprite(vm->sprite->sourceFileIndex, vm->sprite->startPixelInclusive.x,
                           vm->sprite->startPixelInclusive.y, vm->sprite->textureWidth,
                           vm->sprite->textureHeight, fontWidth, vm->fontHeight, textColor,
                           outlineType, (char *)" ", vm->sprite->cols, vm->sprite->rows);

    x = vm->sprite->startPixelInclusive.x + vm->sprite->widthPx * vm->sprite->cols -
        (f32)TextHelper::GetLogicalStringWidth(buf) * (f32)fontWidth * vm->sprite->cols / 2.0f;

    this->DrawTextToSprite(vm->sprite->sourceFileIndex, x, vm->sprite->startPixelInclusive.y,
                           vm->sprite->textureWidth, vm->sprite->textureHeight, fontWidth,
                           vm->fontHeight, textColor, outlineType, buf, vm->sprite->cols,
                           vm->sprite->rows);

    vm->visible = 1;
}

void AnmManager::DrawStringFormat2(AnmVm *vm, u32 textColor, u32 outlineType, const char *text, ...)
{
    i32 fontWidth;
    char buf[256];
    i32 x;
    va_list args;

    fontWidth = vm->fontWidth <= 0 ? 15 : (i32)vm->fontWidth;
    va_start(args, text);
    vsnprintf(buf, sizeof(buf), text, args);
    va_end(args);

    this->DrawTextToSprite(vm->sprite->sourceFileIndex, vm->sprite->startPixelInclusive.x,
                           vm->sprite->startPixelInclusive.y, vm->sprite->textureWidth,
                           vm->sprite->textureHeight, fontWidth, vm->fontHeight, textColor,
                           outlineType, (char *)" ", vm->sprite->cols, vm->sprite->rows);

    x = (i32)(vm->sprite->startPixelInclusive.x + vm->sprite->widthPx * vm->sprite->cols / 2.0f -
              (f32)TextHelper::GetLogicalStringWidth(buf) * fontWidth * vm->sprite->cols / 4.0f);

    this->DrawTextToSprite(vm->sprite->sourceFileIndex, x, vm->sprite->startPixelInclusive.y,
                           vm->sprite->textureWidth, vm->sprite->textureHeight, fontWidth,
                           vm->fontHeight, textColor, outlineType, buf, vm->sprite->cols,
                           vm->sprite->rows);

    vm->visible = 1;
}

ZunResult AnmManager::LoadSurface(i32 surfaceIdx, const char *path)
{
    if (this->surfaces[surfaceIdx])
    {
        ReleaseSurface(surfaceIdx);
    }
    u8 *data = FileSystem::OpenFile(path, 0);
    if (!data)
    {
        g_GameErrorContext.Fatal("%sが読み込めないです。\n", path);
        return ZUN_ERROR;
    }

    SDL_IOStream *rw = SDL_IOFromMem(data, g_LastFileSize);
    SDL_Surface *surf = IMG_Load_IO(rw, 1);
    free(data);

    if (!surf)
    {
        return ZUN_ERROR;
    }

    this->surfaces[surfaceIdx] = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surf);

    this->surfaceSourceInfo[surfaceIdx].width = this->surfaces[surfaceIdx]->w;
    this->surfaceSourceInfo[surfaceIdx].height = this->surfaces[surfaceIdx]->h;

    this->surfacesBis[surfaceIdx] =
        SDL_ConvertSurface(this->surfaces[surfaceIdx], SDL_PIXELFORMAT_RGBA32);
    if (this->surfaceTextures[surfaceIdx].id != 0)
    {
        g_Supervisor.gfxDevice->DeleteTexture(this->surfaceTextures[surfaceIdx]);
    }
    this->surfaceTextures[surfaceIdx] = g_Supervisor.gfxDevice->CreateTexture();
    g_Supervisor.gfxDevice->BindTexture(this->surfaceTextures[surfaceIdx]);
    g_Supervisor.gfxDevice->SetTextureImage(
        this->surfacesBis[surfaceIdx]->w, this->surfacesBis[surfaceIdx]->h, PIXEL_RGBA,
        PIXEL_UNSIGNED_BYTE, this->surfacesBis[surfaceIdx]->pixels);
    return ZUN_SUCCESS;
}

void AnmManager::ReleaseSurface(i32 surfaceIdx)
{
    if (this->surfaces[surfaceIdx])
    {
        SDL_DestroySurface(this->surfaces[surfaceIdx]);
        this->surfaces[surfaceIdx] = nullptr;
    }
    if (this->surfacesBis[surfaceIdx])
    {
        SDL_DestroySurface(this->surfacesBis[surfaceIdx]);
        this->surfacesBis[surfaceIdx] = nullptr;
    }
    if (this->surfaceTextures[surfaceIdx].id != 0)
    {
        g_Supervisor.gfxDevice->DeleteTexture(this->surfaceTextures[surfaceIdx]);
        this->surfaceTextures[surfaceIdx] = 0;
    }
}

void AnmManager::CopySurfaceToBackBuffer(i32 surfaceIdx, i32 left, i32 top, i32 x, i32 y)
{
    if (!this->surfacesBis[surfaceIdx])
    {
        return;
    }

    SDL_Surface *surf = this->surfacesBis[surfaceIdx];

    g_Supervisor.gfxDevice->BindTexture(this->surfaceTextures[surfaceIdx]);

    VertexTex1DiffuseXyzrhw vertices[4];
    f32 width = (f32)this->surfaceSourceInfo[surfaceIdx].width;
    f32 height = (f32)this->surfaceSourceInfo[surfaceIdx].height;

    vertices[0].pos = ZunVec3(x, y, 0.0f);
    vertices[1].pos = ZunVec3(x + width, y, 0.0f);
    vertices[2].pos = ZunVec3(x, y + height, 0.0f);
    vertices[3].pos = ZunVec3(x + width, y + height, 0.0f);
    vertices[0].w = vertices[1].w = vertices[2].w = vertices[3].w = 1.0f;

    f32 u0 = (f32)left / surf->w;
    f32 v0 = (f32)top / surf->h;
    f32 u1 = (f32)this->surfaceSourceInfo[surfaceIdx].width / surf->w;
    f32 v1 = (f32)this->surfaceSourceInfo[surfaceIdx].height / surf->h;

    vertices[0].textureUV = {u0, v0};
    vertices[1].textureUV = {u1, v0};
    vertices[2].textureUV = {u0, v1};
    vertices[3].textureUV = {u1, v1};

    vertices[0].color.color = vertices[1].color.color = vertices[2].color.color =
        vertices[3].color.color = 0xFFFFFFFF;

    g_Supervisor.gfxDevice->SetDepthMask(false);
    g_Supervisor.gfxDevice->SetBlendMode(BLEND_NONE, BLEND_NONE);
    g_Supervisor.gfxDevice->SetColorOp(COMPONENT_RGB, COLOR_OP_MODULATE);
    g_Supervisor.gfxDevice->SetColorOp(COMPONENT_ALPHA, COLOR_OP_MODULATE);
    g_Supervisor.gfxDevice->Disable(CAPS_ALPHA_TEST);
    g_Supervisor.gfxDevice->DrawPrimitiveUP(PRIM_TRIANGLE_STRIP, 2, vertices,
                                            sizeof(VertexTex1DiffuseXyzrhw));
    g_Supervisor.gfxDevice->Enable(CAPS_ALPHA_TEST);
    this->SetBlendMode(255);
}

void AnmManager::DrawEndingRect(i32 surfaceIdx, f32 rectX, f32 rectY, f32 rectLeft, f32 rectTop,
                                i32 width, i32 height)
{
    if (!this->surfacesBis[surfaceIdx])
    {
        return;
    }

    SDL_Surface *surf =
        this->surfaces[surfaceIdx] ? this->surfaces[surfaceIdx] : this->surfacesBis[surfaceIdx];

    g_Supervisor.gfxDevice->BindTexture(this->surfaceTextures[surfaceIdx]);

    VertexTex1DiffuseXyzrhw vertices[4];
    f32 drawWidth = width;
    f32 drawHeight = height;

    vertices[0].pos = ZunVec3(rectX, rectY, 0.0f);
    vertices[1].pos = ZunVec3(rectX + drawWidth, rectY, 0.0f);
    vertices[2].pos = ZunVec3(rectX, rectY + drawHeight, 0.0f);
    vertices[3].pos = ZunVec3(rectX + drawWidth, rectY + drawHeight, 0.0f);
    vertices[0].w = vertices[1].w = vertices[2].w = vertices[3].w = 1.0f;

    f32 u0 = rectLeft / surf->w;
    f32 v0 = rectTop / surf->h;
    f32 u1 = (rectLeft + width) / surf->w;
    f32 v1 = (rectTop + height) / surf->h;

    vertices[0].textureUV = {u0, v0};
    vertices[1].textureUV = {u1, v0};
    vertices[2].textureUV = {u0, v1};
    vertices[3].textureUV = {u1, v1};

    vertices[0].color.color = vertices[1].color.color = vertices[2].color.color =
        vertices[3].color.color = 0xFFFFFFFF;

    g_Supervisor.gfxDevice->SetDepthMask(false);
    g_Supervisor.gfxDevice->SetBlendMode(BLEND_NONE, BLEND_NONE);
    g_Supervisor.gfxDevice->SetColorOp(COMPONENT_RGB, COLOR_OP_MODULATE);
    g_Supervisor.gfxDevice->SetColorOp(COMPONENT_ALPHA, COLOR_OP_MODULATE);
    g_Supervisor.gfxDevice->DrawPrimitiveUP(PRIM_TRIANGLE_STRIP, 2, vertices,
                                            sizeof(VertexTex1DiffuseXyzrhw));
    this->SetBlendMode(255);
}

void AnmManager::TakeScreenshot(i32 textureId, i32 srcLeft, i32 srcTop, i32 srcWidth, i32 srcHeight,
                                i32 dstLeft, i32 dstTop, i32 dstWidth, i32 dstHeight)
{
    if (!this->textures[textureId])
    {
        return;
    }

    Flush();

    u32 *pixelData = new u32[srcWidth * srcHeight];
    g_Supervisor.gfxDevice->ReadPixels(srcLeft, srcTop, srcWidth, srcHeight, pixelData);

    if (srcWidth == dstWidth && srcHeight == dstHeight)
    {
        g_Supervisor.gfxDevice->BindTexture(this->textures[textureId]);
        g_Supervisor.gfxDevice->SetTextureSubImage(dstLeft, dstTop, dstWidth, dstHeight, pixelData);
    }
    else
    {
        SDL_Surface *srcSurf = SDL_CreateSurfaceFrom(srcWidth, srcHeight, SDL_PIXELFORMAT_RGBA32,
                                                     pixelData, srcWidth * 4);
        SDL_Surface *dstSurf = SDL_CreateSurface(dstWidth, dstHeight, SDL_PIXELFORMAT_RGBA32);

        if (srcSurf && dstSurf)
        {
            SDL_BlitSurfaceScaled(srcSurf, NULL, dstSurf, NULL, SDL_SCALEMODE_LINEAR);
            g_Supervisor.gfxDevice->BindTexture(this->textures[textureId]);
            g_Supervisor.gfxDevice->SetTextureSubImage(dstLeft, dstTop, dstWidth, dstHeight,
                                                       dstSurf->pixels);
        }

        if (srcSurf)
        {
            SDL_DestroySurface(srcSurf);
        }
        if (dstSurf)
        {
            SDL_DestroySurface(dstSurf);
        }
    }

    delete[] pixelData;
}

void AnmManager::CopyTexture(i32 dstIdx, i32 srcIdx, SDL_Rect *dstRect, SDL_Rect *srcRect)
{
    if (!this->textures[dstIdx])
    {
        return;
    }
    if (!this->textures[srcIdx])
    {
        return;
    }

    this->Flush();

    u8 *dstPixels = (u8 *)this->imageDataArray[dstIdx];
    u8 *srcPixels = (u8 *)this->imageDataArray[srcIdx];

    if (!srcPixels || !dstPixels)
    {
        return;
    }

    SDL_Surface *srcSurface =
        SDL_CreateSurfaceFrom(this->textureWidths[srcIdx], this->textureHeights[srcIdx],
                              SDL_PIXELFORMAT_RGBA32, srcPixels, this->texturePitches[srcIdx]);

    SDL_Surface *dstSurface =
        SDL_CreateSurfaceFrom(this->textureWidths[dstIdx], this->textureHeights[dstIdx],
                              SDL_PIXELFORMAT_RGBA32, dstPixels, this->texturePitches[dstIdx]);

    if (srcSurface && dstSurface)
    {
        SDL_SetSurfaceBlendMode(srcSurface, SDL_BLENDMODE_NONE);

        SDL_BlitSurfaceScaled(srcSurface, srcRect, dstSurface, dstRect, SDL_SCALEMODE_LINEAR);

        g_Supervisor.gfxDevice->BindTexture(this->textures[dstIdx]);
        g_Supervisor.gfxDevice->SetTextureImage(this->textureWidths[dstIdx],
                                                this->textureHeights[dstIdx], PIXEL_RGBA,
                                                PIXEL_UNSIGNED_BYTE, dstPixels);
    }

    if (srcSurface)
    {
        SDL_DestroySurface(srcSurface);
    }
    if (dstSurface)
    {
        SDL_DestroySurface(dstSurface);
    }
}

void AnmManager::SetInterruptActiveVms(AnmVm *vm, i32 vmCount, i16 interrupt)
{
    i32 shouldSetInterrupt;

    while (vmCount != 0)
    {
        if (!vm->sprite)
        {
            shouldSetInterrupt = false;
        }
        else if (vm->sprite->sourceFileIndex < 0)
        {
            shouldSetInterrupt = false;
        }
        else
        {
            shouldSetInterrupt = g_AnmManager->textures[vm->sprite->sourceFileIndex].id != 0;
        }
        if (shouldSetInterrupt)
        {
            vm->pendingInterrupt = interrupt;
        }
        vm++;
        vmCount--;
    }
}

void AnmManager::ExecuteScripts(AnmVm *startVm, i32 count)
{
    while (count != 0)
    {
        if (startVm->anmFileIdx >= 0)
        {
            g_AnmManager->ExecuteScript(startVm);
        }
        startVm++;
        count--;
    }
}

void AnmManager::ExecuteVmsAnms(AnmVm *vm, i32 idx, i32 vmCount)
{
    while (vmCount != 0)
    {
        g_AnmManager->ExecuteAnmIdx(vm, idx);
        vm->baseSpriteIdx = vm->activeSpriteIdx;
        idx++;
        vm++;
        vmCount--;
    }
}

ZunResult AnmManager::UpdateTrail(AnmVm *vm, VertexTex1DiffuseXyzrhw *vertices, i32 count)
{
    f32 num;
    f32 fVar4;
    f32 uvX;
    f32 startuvX;
    VertexTex1DiffuseXyzrhw *vertex;
    i32 i;
    f32 uvY;
    if (count < 3)
    {
        return ZUN_ERROR;
    }

    startuvX = vm->sprite->uvEnd.x + vm->uvScrollPos.x;
    num = vm->sprite->uvEnd.x - vm->sprite->uvStart.x;
    uvY = vm->sprite->uvStart.y + vm->uvScrollPos.y;
    vertex = vertices;
    fVar4 = num / (f32)((i32)((count + 1) / 2) - 1);

    for (i = 0, uvX = startuvX; i < count; i += 2, vertex += 2, uvX = uvX - fVar4)
    {
        vertex->textureUV.x = uvX;
        vertex->textureUV.y = uvY;
        vertex->color.color = vm->color.color;
        vertex->w = 1.0f;
    }

    uvY = vm->sprite->uvEnd.y + vm->uvScrollPos.y;
    vertex = vertices + 1;

    for (i = 1, uvX = startuvX; i < count; i += 2, vertex += 2, uvX = uvX - fVar4)
    {
        vertex->textureUV.x = uvX;
        vertex->textureUV.y = uvY;
        vertex->color.color = vm->color.color;
        vertex->w = 1.0f;
    }

    return ZUN_SUCCESS;
}

ZunResult AnmManager::DrawTriangleStrip(AnmVm *vm, VertexTex1DiffuseXyzrhw *vertices, i32 count)
{
    if (!vm->visible)
    {
        return ZUN_ERROR;
    }

    if (!vm->active)
    {
        return ZUN_ERROR;
    }

    if (!vm->color.bytes.a)
    {
        return ZUN_ERROR;
    }

    if (this->spritesToDraw != 0)
    {
        this->Flush();
    }

    if (this->currentTexture != this->textures[vm->sprite->sourceFileIndex])
    {
        this->currentTexture = this->textures[vm->sprite->sourceFileIndex];
        g_Supervisor.gfxDevice->BindTexture(this->currentTexture);
    }

    if (this->currentVertexShader != 3)
    {
        this->currentVertexShader = 3;
    }

    SetRenderStateForVm(vm);
    g_Supervisor.gfxDevice->DrawPrimitiveUP(PRIM_TRIANGLE_STRIP, count - 2, vertices,
                                            sizeof(VertexTex1DiffuseXyzrhw));
    return ZUN_SUCCESS;
}
