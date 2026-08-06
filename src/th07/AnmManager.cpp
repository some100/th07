#include "AnmManager.hpp"

#include <d3d8.h>
#include <d3d8types.h>
#include <d3dx8tex.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "AnmVm.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "Rng.hpp"
#include "Stage.hpp"
#include "Supervisor.hpp"
#include "TextHelper.hpp"
#include "ZunMath.hpp"
#include "d3dx8.h"
#include "dxutil.hpp"
#include "utils.hpp"

// GLOBAL: TH07 0x004b9e44
AnmManager *g_AnmManager;

// GLOBAL: TH07 0x004b9fa8
VertexTex1DiffuseXyzrhw g_QuadVertices[4];

// GLOBAL: TH07 0x004ba018
VertexTex1Xyzrhw g_QuadTemplate[4];

// GLOBAL: TH07 0x004ba078
VertexTex1DiffuseXyz g_Quad3DFallback[4];

// FUNCTION: TH07 0x0044d3e0
AnmManager::AnmManager()
{
    memset(this, 0, sizeof(AnmManager));

    for (i32 i = 0; i < 2560; i++)
    {
        this->sprites[i].sourceFileIndex = -1;
    }
    g_QuadTemplate[0].w =
        g_QuadTemplate[1].w =
            g_QuadTemplate[2].w =
                g_QuadTemplate[3].w = 1.0f;
    g_QuadTemplate[0].textureUV.x = 0.0f;
    g_QuadTemplate[0].textureUV.y = 0.0f;
    g_QuadTemplate[1].textureUV.x = 1.0f;
    g_QuadTemplate[1].textureUV.y = 0.0f;
    g_QuadTemplate[2].textureUV.x = 0.0f;
    g_QuadTemplate[2].textureUV.y = 1.0f;
    g_QuadTemplate[3].textureUV.x = 1.0f;
    g_QuadTemplate[3].textureUV.y = 1.0f;
    g_QuadVertices[0].w =
        g_QuadVertices[1].w =
            g_QuadVertices[2].w =
                g_QuadVertices[3].w = 1.0f;
    g_QuadVertices[0].textureUV.x = 0.0f;
    g_QuadVertices[0].textureUV.y = 0.0f;
    g_QuadVertices[1].textureUV.x = 1.0f;
    g_QuadVertices[1].textureUV.y = 0.0f;
    g_QuadVertices[2].textureUV.x = 0.0f;
    g_QuadVertices[2].textureUV.y = 1.0f;
    g_QuadVertices[3].textureUV.x = 1.0f;
    g_QuadVertices[3].textureUV.y = 1.0f;

    this->vertexBuffer = NULL;
    this->currentTexture = NULL;
    this->currentBlendMode = 0;
    this->currentColorOp = 0;
    this->currentTextureFactor.color = 1;
    this->currentVertexShader = 0;
    this->currentCameraMode = 255;
    this->currentZWriteDisable = 0;
    this->screenshotTextureId = -1;
}

// FUNCTION: TH07 0x0044d620 FOLDED
AnmManager::~AnmManager()
{
}

// FUNCTION: TH07 0x0044d630
void AnmManager::SetupVertexBuffer()
{
    RenderVertexInfo *vertexData;

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
    g_Quad3DFallback[0].pos =
        this->vertexBufferContents[0].pos;
    g_Quad3DFallback[1].pos =
        this->vertexBufferContents[1].pos;
    g_Quad3DFallback[2].pos =
        this->vertexBufferContents[2].pos;
    g_Quad3DFallback[3].pos =
        this->vertexBufferContents[3].pos;
    g_Quad3DFallback[0].textureUV.x =
        this->vertexBufferContents[0].textureUV.x;
    g_Quad3DFallback[0].textureUV.y =
        this->vertexBufferContents[0].textureUV.y;
    g_Quad3DFallback[1].textureUV.x =
        this->vertexBufferContents[1].textureUV.x;
    g_Quad3DFallback[1].textureUV.y =
        this->vertexBufferContents[1].textureUV.y;
    g_Quad3DFallback[2].textureUV.x =
        this->vertexBufferContents[2].textureUV.x;
    g_Quad3DFallback[2].textureUV.y =
        this->vertexBufferContents[2].textureUV.y;
    g_Quad3DFallback[3].textureUV.x =
        this->vertexBufferContents[3].textureUV.x;
    g_Quad3DFallback[3].textureUV.y =
        this->vertexBufferContents[3].textureUV.y;
    if (!g_Supervisor.cfg.noVertexBuffers)
    {
        g_Supervisor.d3dDevice->CreateVertexBuffer(
            sizeof(this->vertexBufferContents), 0, D3DFVF_TEX1 | D3DFVF_XYZ,
            D3DPOOL_MANAGED, &this->vertexBuffer);
        this->vertexBuffer->Lock(0, 0, (u8 **)&vertexData, 0);
        memcpy(vertexData, this->vertexBufferContents,
               sizeof(this->vertexBufferContents));
        this->vertexBuffer->Unlock();
        g_Supervisor.d3dDevice->SetStreamSource(0, g_AnmManager->vertexBuffer,
                                                sizeof(RenderVertexInfo));
    }
}

// FUNCTION: TH07 0x0044d8f0
ZunResult AnmManager::LoadTexture(i32 textureIdx, const char *texturePath,
                                  i32 formatIdx, D3DCOLOR colorKey)
{
    u8 *srcData;

    ReleaseTexture(textureIdx);
    if (g_Supervisor.cfg.use16BitTextures)
    {
        if (g_TextureFormatD3D8Mapping[formatIdx] == D3DFMT_A8R8G8B8 ||
            g_TextureFormatD3D8Mapping[formatIdx] == D3DFMT_UNKNOWN)
        {
            formatIdx = 5;
        }
        else if (g_TextureFormatD3D8Mapping[formatIdx] == D3DFMT_R8G8B8)
        {
            formatIdx = 3;
        }
    }
    srcData = FileSystem::OpenFile(texturePath, 1);
    if (!srcData)
    {
        return ZUN_ERROR;
    }

    if (D3DXCreateTextureFromFileInMemoryEx(
            g_Supervisor.d3dDevice, srcData, g_LastFileSize, 0, 0, 0, 0,
            g_TextureFormatD3D8Mapping[formatIdx], D3DPOOL_MANAGED, 3, 0xffffffff,
            colorKey, NULL, NULL, this->textures + textureIdx))
    {
        free(srcData);
        return ZUN_ERROR;
    }
    this->imageDataArray[textureIdx] = srcData;
    return ZUN_SUCCESS;
}

#pragma var_order(surf, texSurf, info, lockedRect, i, src, dst)
// FUNCTION: TH07 0x0044d9e0
ZunResult AnmManager::LoadTextureEmbedded(u32 textureIdx,
                                          ZunImageInfoEmbedded *imageInfo,
                                          D3DCOLOR formatIdx)
{
    u8 *dst;
    u8 *src;
    i32 i;
    D3DLOCKED_RECT lockedRect;
    ZunImageInfoEmbedded *info;
    IDirect3DSurface8 *texSurf;
    IDirect3DSurface8 *surf;

    ReleaseTexture(textureIdx);
    if (g_Supervisor.cfg.use16BitTextures)
    {
        if (g_TextureFormatD3D8Mapping[formatIdx] == D3DFMT_A8R8G8B8 ||
            g_TextureFormatD3D8Mapping[formatIdx] == D3DFMT_UNKNOWN)
        {
            formatIdx = 5;
        }
        else if (g_TextureFormatD3D8Mapping[formatIdx] == D3DFMT_R8G8B8)
        {
            formatIdx = 3;
        }
    }
    info = imageInfo;

    // ZUN landmine: CreateImageSurface is not checked for failure.
    g_Supervisor.d3dDevice->CreateImageSurface(
        (i32)info->width, (i32)info->height,
        g_TextureFormatD3D8Mapping[info->format], &surf);
    surf->LockRect(&lockedRect, NULL, 0);
    for (i = 0; i < info->height; i++)
    {
        dst = (u8 *)lockedRect.pBits + i * lockedRect.Pitch;
        src = &imageInfo->data[i * info->width * g_TextureBytesPerPixel[info->format]];
        memcpy(dst, src, info->width * g_TextureBytesPerPixel[info->format]);
    }
    surf->UnlockRect();
    if (D3DXCreateTexture(g_Supervisor.d3dDevice, (i32)info->width,
                          (i32)info->height, 1, 0,
                          g_TextureFormatD3D8Mapping[formatIdx], D3DPOOL_MANAGED,
                          this->textures + textureIdx))
    {
        return ZUN_ERROR;
    }

    this->textures[textureIdx]->GetSurfaceLevel(0, &texSurf);
    if (D3DXLoadSurfaceFromSurface(texSurf, 0, NULL, surf, 0, NULL, 3, 0) !=
        0)
    {
        return ZUN_ERROR;
    }

    SAFE_RELEASE(surf);
    SAFE_RELEASE(texSurf);
    return ZUN_SUCCESS;
}

#pragma var_order(surfaceDesc, data, lockedRectDst, lockedRectSrc, textureSrc,      \
                  dstData0, srcData0, y0, x0, dstData1, srcData1, y1, x1, dstData2, \
                  srcData2, y2, x2)
// FUNCTION: TH07 0x0044dbe0
ZunResult AnmManager::LoadTextureAlphaChannel(i32 textureIdx,
                                              const char *texturePath,
                                              i32 formatIdx, D3DCOLOR colorKey)
{
    struct Argb1555Pixel
    {
        u16 b : 5;
        u16 g : 5;
        u16 r : 5;
        u16 a : 1;
    };

    struct Argb4444Pixel
    {
        u16 b : 4;
        u16 g : 4;
        u16 r : 4;
        u16 a : 4;
    };

    u32 x2;
    u32 y2;
    Argb4444Pixel *srcData2;
    Argb4444Pixel *dstData2;
    u32 x1;
    u32 y1;
    Argb1555Pixel *srcData1;
    Argb1555Pixel *dstData1;
    u32 x0;
    u32 y0;
    u8 *srcData0;
    u8 *dstData0;
    LPDIRECT3DTEXTURE8 textureSrc;
    D3DLOCKED_RECT lockedRectSrc;
    D3DLOCKED_RECT lockedRectDst;
    u8 *data;
    D3DSURFACE_DESC surfaceDesc;

    textureSrc = NULL;
    data = FileSystem::OpenFile(texturePath, 0);
    if (!data)
    {
        return ZUN_ERROR;
    }

    this->textures[textureIdx]->GetLevelDesc(0, &surfaceDesc);
    if (surfaceDesc.Format != D3DFMT_A8R8G8B8 &&
        surfaceDesc.Format != D3DFMT_A4R4G4B4 &&
        surfaceDesc.Format != D3DFMT_A1R5G5B5)
    {
        // STRING: TH07 0x00495cb8
        g_GameErrorContext.Fatal("error : イメージがαを持っていません\r\n");
        goto err;
    }

    if (D3DXCreateTextureFromFileInMemoryEx(
            g_Supervisor.d3dDevice, data, g_LastFileSize, 0, 0, 0, 0,
            surfaceDesc.Format, D3DPOOL_SYSTEMMEM, 3, 0xffffffff, colorKey,
            NULL, NULL, &textureSrc))
    {
        goto err;
    }

    if (this->textures[textureIdx]->LockRect(0, &lockedRectDst, NULL, 0))
    {
        goto err;
    }

    if (textureSrc->LockRect(0, &lockedRectSrc, NULL, D3DLOCK_NO_DIRTY_UPDATE))
    {
        goto err;
    }

    switch (surfaceDesc.Format)
    {
    case D3DFMT_A8R8G8B8:
        for (y0 = 0; y0 < surfaceDesc.Height; y0 = y0 + 1)
        {
            dstData0 =
                (u8 *)lockedRectDst.pBits + y0 * lockedRectDst.Pitch;
            srcData0 =
                (u8 *)lockedRectSrc.pBits + y0 * lockedRectSrc.Pitch;
            for (x0 = 0; x0 < surfaceDesc.Width; x0++, srcData0 += 4, dstData0 += 4)
            {
                dstData0[3] = srcData0[0];
            }
        }
        break;
    case D3DFMT_A1R5G5B5:
        for (y1 = 0; y1 < surfaceDesc.Height; y1 = y1 + 1)
        {
            dstData1 =
                (Argb1555Pixel *)((u8 *)lockedRectDst.pBits + y1 * lockedRectDst.Pitch);
            srcData1 =
                (Argb1555Pixel *)((u8 *)lockedRectSrc.pBits + y1 * lockedRectSrc.Pitch);
            for (x1 = 0; x1 < surfaceDesc.Width; x1++, srcData1++, dstData1++)
            {
                dstData1->a = srcData1->b >> 4;
            }
        }
        break;
    case D3DFMT_A4R4G4B4:
        for (y2 = 0; y2 < surfaceDesc.Height; y2 = y2 + 1)
        {
            dstData2 =
                (Argb4444Pixel *)((u8 *)lockedRectDst.pBits + y2 * lockedRectDst.Pitch);
            srcData2 =
                (Argb4444Pixel *)((u8 *)lockedRectSrc.pBits + y2 * lockedRectSrc.Pitch);
            for (x2 = 0; x2 < surfaceDesc.Width; x2++, srcData2++, dstData2++)
            {
                dstData2->a = srcData2->b;
            }
        }
        break;
    }
    textureSrc->UnlockRect(0);
    this->textures[textureIdx]->UnlockRect(0);
    SAFE_RELEASE(textureSrc);
    free(data);
    return ZUN_SUCCESS;
err:
    SAFE_RELEASE(textureSrc);
    free(data);
    return ZUN_ERROR;
}

// FUNCTION: TH07 0x0044df40
ZunResult AnmManager::CreateEmptyTexture(i32 textureIdx, u32 width, u32 height,
                                         i32 textureFormat)
{
    D3DXCreateTexture(g_Supervisor.d3dDevice, width, height, 1, 0,
                      g_TextureFormatD3D8Mapping[textureFormat], D3DPOOL_MANAGED,
                      this->textures + textureIdx);
    return ZUN_SUCCESS;
}

#pragma var_order(res, startIdx, ownsMemory, entry)
// FUNCTION: TH07 0x0044df90
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
        // STRING: TH07 0x00495c7c
        g_GameErrorContext.Fatal("アニメが読み込めません。データが失われてるか壊れています\r\n");
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

#pragma var_order(id, data, desc, name, rawSprite, i, curSprite, \
                  loadedSprite)
// FUNCTION: TH07 0x0044e070
i32 AnmManager::LoadAnm(i32 textureIdx, AnmRawEntry *rawEntry,
                        i32 spriteIdxOffset, u32 ownsMemory)
{
    char *name;
    AnmRawSprite *rawSprite;
    AnmLoadedSprite loadedSprite;
    i32 *curSprite;
    i32 i;
    D3DSURFACE_DESC desc;
    AnmRawEntry *data;
    i32 id;

    id = 0;
    if (!rawEntry)
    {
        g_GameErrorContext.Fatal("アニメが読み込めません。データが失われてるか壊れています\r\n");
        return ZUN_ERROR;
    }
    if (textureIdx >= 50)
    {
        // STRING: TH07 0x00495c5c
        g_GameErrorContext.Fatal("テクスチャ格納先が足りません\r\n");
        return ZUN_ERROR;
    }
    ReleaseAnm(textureIdx);
    data = rawEntry;
    if (data->version != 2)
    {
        // STRING: TH07 0x00495c3c
        g_GameErrorContext.Fatal("アニメのバージョンが違います\r\n");
        return ZUN_ERROR;
    }
    data->textureIdx = textureIdx;
    data->ownsMemory = ownsMemory;
    if (!data->hasData)
    {
        name = (char *)((u8 *)data + data->nameOffset);
        if (*name == '@')
        {
            CreateEmptyTexture(data->textureIdx, data->width, data->height,
                               data->format);
        }
        else
        {
            if (LoadTexture(data->textureIdx, name, data->format, data->color_key) !=
                ZUN_SUCCESS)
            {
                // STRING: TH07 0x00495bf8
                g_GameErrorContext.Fatal("テクスチャ %s が読み込めません。データが失われてるか壊れています\r\n", name);
                return ZUN_ERROR;
            }
        }
        if (data->mipmapNameOffset != 0)
        {
            name = (char *)((u8 *)data + data->mipmapNameOffset);
            if (LoadTextureAlphaChannel(data->textureIdx, name, data->format,
                                        data->color_key) != ZUN_SUCCESS)
            {
                g_GameErrorContext.Fatal("テクスチャ %s が読み込めません。データが失われてるか壊れています\r\n", name);
                return ZUN_ERROR;
            }
        }
    }
    else
    {
        if (LoadTextureEmbedded(
                data->textureIdx,
                (ZunImageInfoEmbedded *)((u8 *)data + data->textureOffset),
                data->format) != ZUN_SUCCESS)
        {
            // STRING: TH07 0x00495bb8
            g_GameErrorContext.Fatal("テクスチャが読み込めません。データが失われてるか壊れています\r\n");
            return ZUN_ERROR;
        }
    }
    this->textureNames[textureIdx] = (char *)((u8 *)data + data->nameOffset);
    this->textures[textureIdx]->SetPriority(data->priority);
    this->textures[textureIdx]->PreLoad();
    this->textures[textureIdx]->GetLevelDesc(0, &desc);
    data->spriteIdxOffset = spriteIdxOffset;
    curSprite = data->spriteOffsets;
    for (i = 0; i < data->numSprites; i++, curSprite++)
    {
        rawSprite = (AnmRawSprite *)((u8 *)data + *curSprite);
        loadedSprite.sourceFileIndex = data->textureIdx;
        loadedSprite.cols = (f32)desc.Width / (f32)data->width;
        loadedSprite.rows = (f32)desc.Height / (f32)data->height;
        loadedSprite.startPixelInclusive.x =
            loadedSprite.cols * rawSprite->offset.x;
        loadedSprite.startPixelInclusive.y =
            loadedSprite.rows * rawSprite->offset.y;
        loadedSprite.endPixelInclusive.x =
            (rawSprite->offset.x + rawSprite->size.x) * loadedSprite.cols;
        loadedSprite.endPixelInclusive.y =
            (rawSprite->offset.y + rawSprite->size.y) * loadedSprite.rows;
        loadedSprite.textureWidth = (f32)desc.Width;
        loadedSprite.textureHeight = (f32)desc.Height;
        if (id < rawSprite->id)
        {
            id = rawSprite->id;
        }
        if (rawSprite->id + spriteIdxOffset >= 2560)
        {
            // STRING: TH07 0x00495b80
            g_GameErrorContext.Fatal("スプライトが格納できません。テーブルが不足しています\r\n");
            return ZUN_ERROR;
        }
        LoadSprite(rawSprite->id + spriteIdxOffset, &loadedSprite);
    }
    for (i = 0; i < data->numScripts; i++, curSprite += 2)
    {
        if (*curSprite + spriteIdxOffset >= 2560)
        {
            // STRING: TH07 0x00495b4c
            g_GameErrorContext.Fatal("アニメが格納できません。テーブルが不足しています\r\n");
            return ZUN_ERROR;
        }
        if (id < *curSprite)
        {
            id = *curSprite;
        }
        this->scripts[*curSprite + spriteIdxOffset] =
            (AnmRawInstr *)((u8 *)data + curSprite[1]);
        this->spriteIndices[*curSprite + spriteIdxOffset] = spriteIdxOffset;
    }
    this->anmFiles[textureIdx].raw = data;
    this->anmFiles[textureIdx].spriteIndexOffset = spriteIdxOffset;
    return id + 1;
}

#pragma var_order(spriteIdx, spriteIdxOffset, i, uvX, afterHdr, rawEntry)
// FUNCTION: TH07 0x0044e4e0
void AnmManager::ReleaseAnm(i32 anmIdx)
{
    AnmRawEntry *rawEntry;
    i32 *afterHdr;
    i32 uvX;
    i32 i;
    i32 spriteIdxOffset;
    i32 *spriteIdx;

    if (anmIdx < 0 || (u32)anmIdx >= 50)
    {
        return;
    }

    if (this->anmFiles[anmIdx].raw)
    {
        afterHdr = this->anmFiles[anmIdx].raw->spriteOffsets;
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
            memset(&this->sprites[*spriteIdx + spriteIdxOffset], 0,
                   sizeof(AnmLoadedSprite));
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
        this->currentTexture = NULL;
        this->anmFiles[anmIdx].childCount = 0;
    }
}

// FUNCTION: TH07 0x0044e6f0
void AnmManager::ReleaseTexture(i32 textureIdx)
{
    if (textureIdx < 0 || (u32)textureIdx >= 264)
    {
        return;
    }

    SAFE_RELEASE(this->textures[textureIdx]);
    ZunMemory::Free(this->imageDataArray[textureIdx]);
    this->imageDataArray[textureIdx] = NULL;
}

// FUNCTION: TH07 0x0044e780
void AnmManager::LoadSprite(u32 spriteIdx, AnmLoadedSprite *sprite)
{
    this->sprites[spriteIdx] = *sprite;
    this->sprites[spriteIdx].spriteId = this->loadedSpriteCount++;

    this->sprites[spriteIdx].uvStart.x =
        this->sprites[spriteIdx].startPixelInclusive.x /
        (this->sprites[spriteIdx].textureWidth);
    this->sprites[spriteIdx].uvEnd.x =
        this->sprites[spriteIdx].endPixelInclusive.x /
        (this->sprites[spriteIdx].textureWidth);
    this->sprites[spriteIdx].uvStart.y =
        this->sprites[spriteIdx].startPixelInclusive.y /
        (this->sprites[spriteIdx].textureHeight);
    this->sprites[spriteIdx].uvEnd.y =
        this->sprites[spriteIdx].endPixelInclusive.y /
        (this->sprites[spriteIdx].textureHeight);
    this->sprites[spriteIdx].widthPx =
        (this->sprites[spriteIdx].endPixelInclusive.x -
         this->sprites[spriteIdx].startPixelInclusive.x) /
        sprite->cols;
    this->sprites[spriteIdx].heightPx =
        (this->sprites[spriteIdx].endPixelInclusive.y -
         this->sprites[spriteIdx].startPixelInclusive.y) /
        sprite->rows;
}

// FUNCTION: TH07 0x0044e8e0
ZunResult AnmManager::SetActiveSprite(AnmVm *vm, i32 spriteIdx)
{
    if (this->sprites[spriteIdx].sourceFileIndex < 0)
    {
        return ZUN_ERROR;
    }

    vm->activeSpriteIdx = (i16)spriteIdx;
    vm->sprite = &this->sprites[spriteIdx];
    D3DXMatrixIdentity(&vm->matrix);
    D3DXMatrixIdentity(&vm->uvMatrix);
    if (vm->sprite->cols < 1.0f)
    {
        // ZUN bloat: This is despite the fact that spriteIdx is never used ever again
        spriteIdx = 0;
    }
    vm->matrix.m[0][0] = vm->sprite->widthPx / 256.0f;
    vm->matrix.m[1][1] = vm->sprite->heightPx / 256.0f;
    vm->uvMatrix.m[0][0] =
        vm->sprite->widthPx / vm->sprite->textureWidth * vm->sprite->cols;
    vm->uvMatrix.m[1][1] =
        vm->sprite->heightPx / vm->sprite->textureHeight * vm->sprite->rows;
    vm->worldTransformMatrix = vm->matrix;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0044ea20
void AnmManager::SetAndExecuteScript(AnmVm *vm, AnmRawInstr *beginningOfScript)
{
    i32 idk;

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
        this->scriptsExecutedThisFrame++;
    }
}

// FUNCTION: TH07 0x0044eae0
void AnmManager::SetRenderStateForVm(AnmVm *vm)
{
    ZunColor color;

    if ((u32)this->currentBlendMode != vm->blendMode)
    {
        this->currentBlendMode = vm->blendMode;
        if (!this->currentBlendMode)
        {
            g_Supervisor.d3dDevice->SetRenderState(D3DRS_DESTBLEND, 6);
        }
        else
        {
            g_Supervisor.d3dDevice->SetRenderState(D3DRS_DESTBLEND, 2);
        }
    }
    color.color = vm->useColor2 ? vm->color2.color : vm->color.color;
    if (g_Supervisor.cfg.noVertexBuffers)
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
            g_Supervisor.d3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR,
                                                   this->currentTextureFactor.color);
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
    if (!g_Supervisor.cfg.disableZBuffer &&
        (u32)this->currentZWriteDisable != vm->zWriteDisable)
    {
        this->currentZWriteDisable = vm->zWriteDisable;
        if (!this->currentZWriteDisable)
        {
            g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, 1);
        }
        else
        {
            g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, 0);
        }
    }
    if ((u32)this->currentCameraMode != vm->cameraMode)
    {
        g_AnmManager->Flush();
        this->currentCameraMode = vm->cameraMode;
        if (!this->currentCameraMode)
        {
            g_Stage.SetupCameraStageBackground();
            g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
        }
        else
        {
            g_Stage.UpdateCamera();
            g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
        }
    }
    this->renderStateChangesThisFrame++;
}

// FUNCTION: TH07 0x0044eec0
void AnmManager::SyncRenderState(AnmVm *vm)
{
    if ((u32)this->currentBlendMode != vm->blendMode)
    {
        this->currentBlendMode = vm->blendMode;
        if (!this->currentBlendMode)
        {
            g_Supervisor.SetRenderState(D3DRS_DESTBLEND, 6);
        }
        else
        {
            g_Supervisor.SetRenderState(D3DRS_DESTBLEND, 2);
        }
    }
    if (!g_Supervisor.cfg.disableZBuffer &&
        (u32)this->currentZWriteDisable != vm->zWriteDisable)
    {
        this->currentZWriteDisable = vm->zWriteDisable;
        if (!this->currentZWriteDisable)
        {
            g_Supervisor.SetRenderState(D3DRS_ZWRITEENABLE, 1);
        }
        else
        {
            g_Supervisor.SetRenderState(D3DRS_ZWRITEENABLE, 0);
        }
    }
    this->renderStateChangesThisFrame++;
}

static const f32 g_ZeroPointFive = 0.5;

#pragma var_order(triangleY1, triangleY2, triangleX2, triangleX1, color)
// FUNCTION: TH07 0x0044efb0
ZunResult AnmManager::DrawInner(AnmVm *vm, u32 drawFlags)
{
    ZunColor color;
    f32 triangleX1, triangleX2, triangleY1, triangleY2;

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
        /*g_QuadVertices[0].pos.x =
            roundf(g_QuadVertices[0].pos.x) - 0.5f;
        g_QuadVertices[1].pos.x =
            roundf(g_QuadVertices[1].pos.x) - 0.5f;
        g_QuadVertices[0].pos.y =
            roundf(g_QuadVertices[0].pos.y) - 0.5f;
        g_QuadVertices[2].pos.y =
            roundf(g_QuadVertices[2].pos.y) - 0.5f;
        g_QuadVertices[1].pos.y =
            g_QuadVertices[0].pos.y;
        g_QuadVertices[2].pos.x =
            g_QuadVertices[0].pos.x;
        g_QuadVertices[3].pos.x =
            g_QuadVertices[1].pos.x;
        g_QuadVertices[3].pos.y =
        g_QuadVertices[2].pos.y;*/
        __asm {
        fld g_QuadVertices[0 * TYPE g_QuadVertices].pos.x
        frndint
        fsub g_ZeroPointFive
        fld g_QuadVertices[1 * TYPE g_QuadVertices].pos.x
        frndint
        fsub g_ZeroPointFive
        fld g_QuadVertices[0 * TYPE g_QuadVertices].pos.y
        frndint
        fsub g_ZeroPointFive
        fld g_QuadVertices[2 * TYPE g_QuadVertices].pos.y
        frndint
        fsub g_ZeroPointFive
        fst g_QuadVertices[2 * TYPE g_QuadVertices].pos.y
        fstp g_QuadVertices[3 * TYPE g_QuadVertices].pos.y
        fst g_QuadVertices[0 * TYPE g_QuadVertices].pos.y
        fstp g_QuadVertices[1 * TYPE g_QuadVertices].pos.y
        fst g_QuadVertices[1 * TYPE g_QuadVertices].pos.x
        fstp g_QuadVertices[3 * TYPE g_QuadVertices].pos.x
        fst g_QuadVertices[0 * TYPE g_QuadVertices].pos.x
        fstp g_QuadVertices[2 * TYPE g_QuadVertices].pos.x
        }
    }

    g_QuadVertices[0].textureUV.x = g_QuadVertices[2].textureUV.x =
        vm->sprite->uvStart.x + vm->uvScrollPos.x;
    g_QuadVertices[1].textureUV.x = g_QuadVertices[3].textureUV.x =
        vm->sprite->uvEnd.x + vm->uvScrollPos.x;
    g_QuadVertices[0].textureUV.y = g_QuadVertices[1].textureUV.y =
        vm->sprite->uvStart.y + vm->uvScrollPos.y;
    g_QuadVertices[2].textureUV.y = g_QuadVertices[3].textureUV.y =
        vm->sprite->uvEnd.y + vm->uvScrollPos.y;

    triangleX1 = max(g_QuadVertices[0].pos.x,
                     g_QuadVertices[1].pos.x);
    triangleX1 = max(g_QuadVertices[2].pos.x, triangleX1);
    triangleX1 = max(g_QuadVertices[3].pos.x, triangleX1);

    triangleY1 = max(g_QuadVertices[0].pos.y,
                     g_QuadVertices[1].pos.y);
    triangleY1 = max(g_QuadVertices[2].pos.y, triangleY1);
    triangleY1 = max(g_QuadVertices[3].pos.y, triangleY1);

    triangleX2 = min(g_QuadVertices[0].pos.x,
                     g_QuadVertices[1].pos.x);
    triangleX2 = min(g_QuadVertices[2].pos.x, triangleX2);
    triangleX2 = min(g_QuadVertices[3].pos.x, triangleX2);

    triangleY2 = min(g_QuadVertices[0].pos.y,
                     g_QuadVertices[1].pos.y);
    triangleY2 = min(g_QuadVertices[2].pos.y, triangleY2);
    triangleY2 = min(g_QuadVertices[3].pos.y, triangleY2);

    if (triangleX1 < g_Supervisor.viewport.X ||
        triangleY1 < g_Supervisor.viewport.Y ||
        triangleX2 > g_Supervisor.viewport.X + g_Supervisor.viewport.Width ||
        triangleY2 > g_Supervisor.viewport.Y + g_Supervisor.viewport.Height)
    {
        return ZUN_SUCCESS;
    }

    if (this->currentTexture != this->textures[vm->sprite->sourceFileIndex])
    {
        this->currentTexture = this->textures[vm->sprite->sourceFileIndex];
        this->Flush();
        g_Supervisor.d3dDevice->SetTexture(
            0, (IDirect3DBaseTexture8 *)this->currentTexture);
    }
    if (this->currentVertexShader != 1)
    {
        this->Flush();
        this->currentVertexShader = 1;
    }
    if ((drawFlags & 2) == 0)
    {
        color.color =
            vm->useColor2 ? vm->color2.color : vm->color.color;
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

// FUNCTION: TH07 0x0044f580
void AnmManager::ResetVertexBuffer()
{
    this->spritesToDraw = 0;
    this->vertexBufferCurPtr = this->spriteVertexBuffer;
    this->vertexBufferStartPtr = this->vertexBufferCurPtr;
}

// FUNCTION: TH07 0x0044f5c0
void AnmManager::Flush()
{
    if (!this->spritesToDraw)
    {
        return;
    }

    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, 0);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, 0);
    g_Supervisor.d3dDevice->SetVertexShader(D3DFVF_TEX1 | D3DFVF_DIFFUSE |
                                            D3DFVF_XYZRHW);
    g_Supervisor.d3dDevice->DrawPrimitiveUP(
        D3DPT_TRIANGLELIST, this->spritesToDraw << 1, this->vertexBufferStartPtr,
        sizeof(VertexTex1DiffuseXyzrhw));
    this->vertexBufferStartPtr = this->vertexBufferCurPtr;
    this->spritesToDraw = 0;
    this->flushesThisFrame++;
}

// FUNCTION: TH07 0x0044f690
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

// FUNCTION: TH07 0x0044f770
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

    centerX = vm->sprite->widthPx * vm->scale.x / 2.0f;
    centerY = vm->sprite->heightPx * vm->scale.y / 2.0f;

    if ((vm->anchor & 1) == 0)
    {
        g_QuadVertices[0].pos.x =
            g_QuadVertices[2].pos.x = vm->pos.x - centerX;
        g_QuadVertices[1].pos.x =
            g_QuadVertices[3].pos.x = centerX + vm->pos.x;
    }
    else
    {
        g_QuadVertices[0].pos.x =
            g_QuadVertices[2].pos.x = vm->pos.x;
        g_QuadVertices[1].pos.x =
            g_QuadVertices[3].pos.x = centerX + vm->pos.x +
                                      centerX;
    }

    if ((vm->anchor & 2) == 0)
    {
        g_QuadVertices[0].pos.y =
            g_QuadVertices[1].pos.y = vm->pos.y - centerY;
        g_QuadVertices[2].pos.y =
            g_QuadVertices[3].pos.y = centerY + vm->pos.y;
    }
    else
    {
        g_QuadVertices[0].pos.y =
            g_QuadVertices[1].pos.y = vm->pos.y;
        g_QuadVertices[2].pos.y =
            g_QuadVertices[3].pos.y = centerY + vm->pos.y +
                                      centerY;
    }

    g_QuadVertices[0].pos.z =
        g_QuadVertices[1].pos.z =
            g_QuadVertices[2].pos.z =
                g_QuadVertices[3].pos.z = vm->pos.z;

    return DrawInner(vm, 1);
}

// FUNCTION: TH07 0x0044f960
void AnmManager::TranslateRotation(VertexTex1DiffuseXyzrhw *vertex, f32 width,
                                   f32 height, f32 sine, f32 cosine,
                                   f32 xOffset, f32 yOffset)
{
    vertex->pos.x = width * cosine - height * sine + xOffset;
    vertex->pos.y = width * sine + height * cosine + yOffset;
}

#pragma var_order(sinZ, z, cosZ, width, height, yOffset, xOffset)
// FUNCTION: TH07 0x0044f9a0
ZunResult AnmManager::Draw(AnmVm *vm)
{
    f32 cosZ;
    f32 sinZ;
    f32 xOffset;
    f32 yOffset;
    f32 z;
    f32 width;
    f32 height;

    if (vm->rotation.z == 0.0f)
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

    z = vm->rotation.z;
    sincosf_macro(sinZ, cosZ, z);
    xOffset = vm->pos.x;
    yOffset = vm->pos.y;
    width = vm->sprite->widthPx * vm->scale.x / 2.0f;
    height = vm->sprite->heightPx * vm->scale.y / 2.0f;

    TranslateRotation(&g_QuadVertices[0], -width, -height, sinZ,
                      cosZ, xOffset, yOffset);
    TranslateRotation(&g_QuadVertices[1], width, -height, sinZ,
                      cosZ, xOffset, yOffset);
    TranslateRotation(&g_QuadVertices[2], -width, height, sinZ,
                      cosZ, xOffset, yOffset);
    TranslateRotation(&g_QuadVertices[3], width, height, sinZ,
                      cosZ, xOffset, yOffset);

    g_QuadVertices[0].pos.z =
        g_QuadVertices[1].pos.z =
            g_QuadVertices[2].pos.z =
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

// FUNCTION: TH07 0x0044fc10
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

    centerX = vm->sprite->widthPx * vm->scale.x / 2.0f;
    centerY = vm->sprite->heightPx * vm->scale.y / 2.0f;

    if ((vm->anchor & 1) == 0)
    {
        g_QuadVertices[0].pos.x =
            g_QuadVertices[2].pos.x = vm->pos.x - centerX;
        g_QuadVertices[1].pos.x =
            g_QuadVertices[3].pos.x = centerX + vm->pos.x;
    }
    else
    {
        g_QuadVertices[0].pos.x =
            g_QuadVertices[2].pos.x = vm->pos.x;
        g_QuadVertices[1].pos.x =
            g_QuadVertices[3].pos.x = centerX + vm->pos.x + centerX;
    }

    if ((vm->anchor & 2) == 0)
    {
        g_QuadVertices[0].pos.y =
            g_QuadVertices[1].pos.y = vm->pos.y - centerY;
        g_QuadVertices[2].pos.y =
            g_QuadVertices[3].pos.y = centerY + vm->pos.y;
    }
    else
    {
        g_QuadVertices[0].pos.y =
            g_QuadVertices[1].pos.y = vm->pos.y;
        g_QuadVertices[2].pos.y =
            g_QuadVertices[3].pos.y = centerY + vm->pos.y + centerY;
    }

    g_QuadVertices[0].pos.z =
        g_QuadVertices[1].pos.z =
            g_QuadVertices[2].pos.z =
                g_QuadVertices[3].pos.z = vm->pos.z;

    return DrawInner(vm, 0);
}

#pragma var_order(halfWidth, halfHeight, screenCenterY, halfLength, sinZ, matrix, z, \
                  projectRight, projectCenter, projectRightOffset, cosZ, origin)
// FUNCTION: TH07 0x0044fe00
ZunResult AnmManager::CalcBillboardTransform(AnmVm *vm)
{
    f32 halfWidth;
    f32 halfHeight;
    f32 screenCenterY;
    f32 halfLength; // also used as screen center x
    f32 sinZ;
    D3DXMATRIX matrix;
    f32 z = vm->rotation.z;
    Float3 projectRight;
    Float3 projectCenter;
    Float3 projectRightOffset;
    f32 cosZ;

    sincosf_macro(sinZ, cosZ, z);

    Float3 origin(0.0f, 0.0f, 0.0f);

    D3DXMatrixIdentity(&matrix);
    matrix.m[3][0] = vm->pos.x;
    matrix.m[3][1] = vm->pos.y;
    matrix.m[3][2] = vm->pos.z;

    D3DXVec3Project(projectCenter.asD3DX(), origin.asD3DX(), &g_Supervisor.viewport,
                    &g_Supervisor.projectionMatrix, &g_Supervisor.viewMatrix,
                    &matrix);

    if (projectCenter.z < 0.0f || projectCenter.z > 1.0f)
    {
        return ZUN_ERROR;
    }

    D3DXVec3Project(projectRight.asD3DX(), g_Stage.cam.right.asD3DX(), &g_Supervisor.viewport,
                    &g_Supervisor.projectionMatrix, &g_Supervisor.viewMatrix,
                    &matrix);

    projectRightOffset = projectRight - projectCenter;

    halfLength = D3DXVec3Length(projectRightOffset.asD3DX()) * 0.5f;
    halfWidth = halfLength * vm->sprite->widthPx * vm->scale.x;
    halfHeight = halfLength * vm->sprite->heightPx * vm->scale.y;

    halfLength = projectCenter.x; // used as screen center x here
    screenCenterY = projectCenter.y;

    TranslateRotation(&g_QuadVertices[0], -halfWidth, -halfHeight, sinZ,
                      cosZ, halfLength, screenCenterY);
    TranslateRotation(&g_QuadVertices[1], halfWidth, -halfHeight, sinZ,
                      cosZ, halfLength, screenCenterY);
    TranslateRotation(&g_QuadVertices[2], -halfWidth, halfHeight, sinZ,
                      cosZ, halfLength, screenCenterY);
    TranslateRotation(&g_QuadVertices[3], halfWidth, halfHeight, sinZ,
                      cosZ, halfLength, screenCenterY);

    g_QuadVertices[0].pos.z =
        g_QuadVertices[1].pos.z =
            g_QuadVertices[2].pos.z =
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

// FUNCTION: TH07 0x00450130
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

#pragma var_order(rot, world)
// FUNCTION: TH07 0x004501a0
void AnmManager::CalcProjectedTransform(AnmVm *vm)
{
    D3DXMATRIX world;
    D3DXMATRIX rot;

    if (vm->skipTransform == 0 &&
        (vm->updateScale || vm->updateRotation))
    {
        vm->worldTransformMatrix = vm->matrix;
        vm->worldTransformMatrix.m[0][0] *= vm->scale.x;
        vm->worldTransformMatrix.m[1][1] *= vm->scale.y;
        vm->updateScale = 0;
        if (vm->rotation.x != 0.0)
        {
            D3DXMatrixRotationX(&rot, vm->rotation.x);
            D3DXMatrixMultiply(&vm->worldTransformMatrix, &vm->worldTransformMatrix,
                               &rot);
        }
        if (vm->rotation.y != 0.0)
        {
            D3DXMatrixRotationY(&rot, vm->rotation.y);
            D3DXMatrixMultiply(&vm->worldTransformMatrix, &vm->worldTransformMatrix,
                               &rot);
        }
        if (vm->rotation.z != 0.0)
        {
            D3DXMatrixRotationZ(&rot, vm->rotation.z);
            D3DXMatrixMultiply(&vm->worldTransformMatrix, &vm->worldTransformMatrix,
                               &rot);
        }
        vm->updateRotation = 0;
    }

    world = vm->worldTransformMatrix;
    if ((vm->anchor & 1) == 0)
    {
        world.m[3][0] = vm->pos.x;
    }
    else
    {
        world.m[3][0] = fabsf(vm->sprite->widthPx * vm->scale.x / 2.0f) + vm->pos.x;
    }

    if ((vm->anchor & 2) == 0)
    {
        world.m[3][1] = vm->pos.y;
    }
    else
    {
        world.m[3][1] = fabsf(vm->sprite->heightPx * vm->scale.y / 2.0f) + vm->pos.y;
    }
    world.m[3][2] = vm->pos.z;

    D3DXVec3Project((D3DXVECTOR3 *)&g_QuadVertices[0].pos,
                    this->vertexBufferContents[0].pos.asD3DX(),
                    &g_Supervisor.viewport, &g_Supervisor.projectionMatrix,
                    &g_Supervisor.viewMatrix, &world);
    D3DXVec3Project((D3DXVECTOR3 *)&g_QuadVertices[1].pos,
                    this->vertexBufferContents[1].pos.asD3DX(),
                    &g_Supervisor.viewport, &g_Supervisor.projectionMatrix,
                    &g_Supervisor.viewMatrix, &world);
    D3DXVec3Project((D3DXVECTOR3 *)&g_QuadVertices[2].pos,
                    this->vertexBufferContents[2].pos.asD3DX(),
                    &g_Supervisor.viewport, &g_Supervisor.projectionMatrix,
                    &g_Supervisor.viewMatrix, &world);
    D3DXVec3Project((D3DXVECTOR3 *)&g_QuadVertices[3].pos,
                    this->vertexBufferContents[3].pos.asD3DX(),
                    &g_Supervisor.viewport, &g_Supervisor.projectionMatrix,
                    &g_Supervisor.viewMatrix, &world);

    this->matrix = world;
}

// FUNCTION: TH07 0x004504b0
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

#pragma var_order(uv, rot, world)
// FUNCTION: TH07 0x00450520
ZunResult AnmManager::Draw3(AnmVm *vm)
{
    D3DXMATRIX world;
    D3DXMATRIX rot;
    D3DXMATRIX uv;

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

    if (vm->skipTransform == 0 &&
        (vm->updateScale || vm->updateRotation))
    {
        vm->worldTransformMatrix = vm->matrix;
        vm->worldTransformMatrix.m[0][0] *= vm->scale.x;
        vm->worldTransformMatrix.m[1][1] *= vm->scale.y;
        vm->updateScale = 0;

        // double intentionally used here
        if (vm->rotation.x != 0.0)
        {
            D3DXMatrixRotationX(&rot, vm->rotation.x);
            D3DXMatrixMultiply(&vm->worldTransformMatrix, &vm->worldTransformMatrix,
                               &rot);
        }
        if (vm->rotation.y != 0.0)
        {
            D3DXMatrixRotationY(&rot, vm->rotation.y);
            D3DXMatrixMultiply(&vm->worldTransformMatrix, &vm->worldTransformMatrix,
                               &rot);
        }
        if (vm->rotation.z != 0.0)
        {
            D3DXMatrixRotationZ(&rot, vm->rotation.z);
            D3DXMatrixMultiply(&vm->worldTransformMatrix, &vm->worldTransformMatrix,
                               &rot);
        }
        vm->updateRotation = 0;
    }

    world = vm->worldTransformMatrix;
    if ((vm->anchor & 1) == 0)
    {
        world.m[3][0] = vm->pos.x;
    }
    else
    {
        world.m[3][0] = fabsf(vm->sprite->widthPx * vm->scale.x / 2.0f) + vm->pos.x;
    }

    if ((vm->anchor & 2) == 0)
    {
        world.m[3][1] = vm->pos.y;
    }
    else
    {
        world.m[3][1] = fabsf(vm->sprite->heightPx * vm->scale.y / 2.0f) + vm->pos.y;
    }

    world.m[3][0] += this->offset.x;
    world.m[3][1] += this->offset.y;

    SetRenderStateForVm(vm);
    world.m[3][2] = vm->pos.z;

    g_Supervisor.d3dDevice->SetTransform((D3DTRANSFORMSTATETYPE)256, &world);

    if (this->currentSprite != vm->sprite)
    {
        this->currentSprite = vm->sprite;
        uv = vm->uvMatrix;
        uv.m[2][0] = vm->sprite->uvStart.x + vm->uvScrollPos.x;
        uv.m[2][1] = vm->sprite->uvStart.y + vm->uvScrollPos.y;
        g_Supervisor.d3dDevice->SetTransform(D3DTS_TEXTURE0, &uv);

        if (this->currentTexture != this->textures[vm->sprite->sourceFileIndex])
        {
            this->currentTexture = this->textures[vm->sprite->sourceFileIndex];
            g_Supervisor.d3dDevice->SetTexture(
                0, (IDirect3DBaseTexture8 *)this->currentTexture);
        }
    }

    if (this->currentVertexShader != 2)
    {
        if (!g_Supervisor.cfg.noVertexBuffers)
        {
            g_Supervisor.d3dDevice->SetVertexShader(D3DFVF_TEX1 | D3DFVF_XYZ);
            g_Supervisor.d3dDevice->SetStreamSource(0, this->vertexBuffer, 20);
        }
        else
        {
            g_Supervisor.d3dDevice->SetVertexShader(D3DFVF_TEX1 |
                                                    D3DFVF_DIFFUSE |
                                                    D3DFVF_XYZ);
        }
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, 3);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, 3);
        this->currentVertexShader = 2;
    }

    if (!g_Supervisor.cfg.noVertexBuffers)
    {
        g_Supervisor.d3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    }
    else
    {
        g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2,
                                                g_Quad3DFallback,
                                                sizeof(VertexTex1DiffuseXyz));
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00450a50
f32 AnmVm::GetFloatVarValue(f32 arg)
{
    switch ((i32)arg)
    {
    case 10000:
        return (f32)this->intVars1[0];
    case 10001:
        return (f32)this->intVars1[1];
    case 10002:
        return (f32)this->intVars1[2];
    case 10003:
        return (f32)this->intVars1[3];
    case 10004:
        return this->floatVars[0];
    case 10005:
        return this->floatVars[1];
    case 10006:
        return this->floatVars[2];
    case 10007:
        return this->floatVars[3];
    case 10008:
        return (f32)this->intVars2[0];
    case 10009:
        return (f32)this->intVars2[1];
    default:
        return arg;
    }
}

// FUNCTION: TH07 0x00450b20
i32 AnmVm::GetVarValue(i32 arg)
{
    switch (arg)
    {
    case 10000:
        return this->intVars1[0];
    case 10001:
        return this->intVars1[1];
    case 10002:
        return this->intVars1[2];
    case 10003:
        return this->intVars1[3];
    case 10004:
        return this->floatVars[0];
    case 10005:
        return this->floatVars[1];
    case 10006:
        return this->floatVars[2];
    case 10007:
        return this->floatVars[3];
    case 10008:
        return this->intVars2[0];
    case 10009:
        return this->intVars2[1];
    default:
        return arg;
    }
}

// FUNCTION: TH07 0x00450c10
f32 *AnmVm::GetFloatVar(f32 *paramId, u16 mask, u32 idx)
{
    if (((u32)mask & 1 << idx) == 0)
    {
        return paramId;
    }

    switch ((u32)*paramId)
    {
    case 10004:
        return &this->floatVars[0];
    case 10005:
        return &this->floatVars[1];
    case 10006:
        return &this->floatVars[2];
    case 10007:
        return &this->floatVars[3];
    default:
        return paramId;
    }
}

// FUNCTION: TH07 0x00450ca0
i32 *AnmVm::GetVar(i32 *paramId, u16 mask, u32 idx)
{
    if (((u32)mask & 1 << idx) == 0)
    {
        return paramId;
    }

    switch (*paramId)
    {
    case 10000:
        return &this->intVars1[0];
    case 10001:
        return &this->intVars1[1];
    case 10002:
        return &this->intVars1[2];
    case 10003:
        return &this->intVars1[3];
    case 10008:
        return &this->intVars2[0];
    case 10009:
        return &this->intVars2[1];
    default:
        return paramId;
    }
}

#pragma var_order(instr, nextInstr, i, t)
// FUNCTION: TH07 0x00450d60
i32 AnmManager::ExecuteScript(AnmVm *vm)
{
    AnmRawInstr *instr;
    AnmRawInstr *nextInstr;
    i32 i;
    f32 t;

#define GET_INT_PTR(argIdx) \
    vm->GetVar(&instr->args[argIdx].i, instr->flags, argIdx)

#define GET_FLOAT_PTR(argIdx) \
    vm->GetFloatVar(&instr->args[argIdx].f, instr->flags, argIdx)

#define GET_INT_VALUE(argIdx) \
    (((instr->flags & (1 << argIdx)) != 0) ? vm->GetVarValue(instr->args[argIdx].i) : instr->args[argIdx].i)

#define GET_FLOAT_VALUE(argIdx) \
    (((instr->flags & (1 << argIdx)) != 0) ? vm->GetFloatVarValue(instr->args[argIdx].f) : instr->args[argIdx].f)

    if (!vm->currentInstruction)
    {
        return 1;
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
            vm->color.color =
                (vm->color.color & 0xff000000) | (instr->args[0].i & 0xffffff);
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
            vm->interpModes[4] = 0;
            vm->scaleInterpInitial = vm->scale;
            vm->scaleInterpFinal.x = GET_FLOAT_VALUE(0);
            vm->scaleInterpFinal.y = GET_FLOAT_VALUE(1);
            break;
        case ANM_FADE:
            vm->colorInterpInitialColor.bytes.a = vm->color.bytes.a;
            vm->colorInterpFinalColor.bytes.a = instr->args[0].b[0];
            vm->interpStartTimes[2] = 0;
            vm->interpEndTimes[2] = GET_INT_VALUE(1);
            vm->interpModes[2] = 0;
            break;
        case ANM_SET_BLEND:
            vm->blendMode = instr->args[0].i;
            break;
        case ANM_SET_TRANSLATION:
            if (!vm->useOffset)
            {
                vm->pos =
                    Float3(GET_FLOAT_VALUE(0), GET_FLOAT_VALUE(1), GET_FLOAT_VALUE(2));
            }
            else
            {
                vm->offset =
                    Float3(GET_FLOAT_VALUE(0), GET_FLOAT_VALUE(1), GET_FLOAT_VALUE(2));
            }
            break;
        case ANM_POS_TIME_ACCEL:
            vm->interpModes[0] = 6;
            goto interp_pos;
        case ANM_POS_TIME_DECEL:
            vm->interpModes[0] = 4;
            goto interp_pos;
        case ANM_POS_TIME_LINEAR:
            vm->interpModes[0] = 0;
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
                Float3(GET_FLOAT_VALUE(0), GET_FLOAT_VALUE(1), GET_FLOAT_VALUE(2));
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
                if (instr->opcode == ANM_INTERRUPT_LABEL &&
                    instr->args[0].i == 0xffffffff)
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
            vm->interpModes[0] = instr->args[1].b[0];
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
            vm->interpModes[1] = instr->args[1].b[0];
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
            vm->interpModes[2] = instr->args[1].b[0];
            vm->colorInterpInitialColor.bytes.a = vm->color.bytes.a;
            vm->colorInterpFinalColor.bytes.a = instr->args[2].b[0];
            break;
        case ANM_INTERP_ROTATE:
            vm->interpStartTimes[3] = 0;
            vm->interpEndTimes[3] = GET_INT_VALUE(0);
            vm->interpModes[3] = instr->args[1].b[0];
            vm->rotateInterpInitial = vm->rotation;
            vm->rotateInterpFinal.x = GET_FLOAT_VALUE(2);
            vm->rotateInterpFinal.y = GET_FLOAT_VALUE(3);
            vm->rotateInterpFinal.z = GET_FLOAT_VALUE(4);
            vm->updateRotation = 1;
            break;
        case ANM_INTERP_SCALE_2:
            vm->interpStartTimes[4] = 0;
            vm->interpEndTimes[4] = GET_INT_VALUE(0);
            vm->interpModes[4] = instr->args[1].b[0];
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
        case ANM_ADD_NORMALIZE_ANGLE:
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
            vm->currentInstruction = (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[2].i);
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
            vm->rotation.x,
            g_Supervisor.effectiveFramerateMultiplier * vm->angleVel.x);
        vm->updateRotation = 1;
    }
    if (vm->angleVel.y != 0.0f)
    {
        vm->rotation.y = utils::AddNormalizeAngle(
            vm->rotation.y,
            g_Supervisor.effectiveFramerateMultiplier * vm->angleVel.y);
        vm->updateRotation = 1;
    }
    if (vm->angleVel.z != 0.0f)
    {
        vm->rotation.z = utils::AddNormalizeAngle(
            vm->rotation.z,
            g_Supervisor.effectiveFramerateMultiplier * vm->angleVel.z);
        vm->updateRotation = 1;
    }
    for (i = 0; i < 5; i++)
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
                t = vm->interpStartTimes[i].AsFloat() /
                    vm->interpEndTimes[i].AsFloat();
            }
            switch (vm->interpModes[i])
            {
            case 1:
                t = t * t;
                break;
            case 2:
                t = t * t * t;
                break;
            case 3:
                t = t * t;
                t = t * t;
                break;
            case 4:
                t = 1.0f - t;
                t = t * t;
                t = 1.0f - t;
                break;
            case 5:
                t = 1.0f - t;
                t = t * t * t;
                t = 1.0f - t;
                break;
            case 6:
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
                    vm->pos.x = (vm->posInterpFinal.x - vm->posInterpInitial.x) * t + vm->posInterpInitial.x;
                    vm->pos.y = (vm->posInterpFinal.y - vm->posInterpInitial.y) * t + vm->posInterpInitial.y;
                    vm->pos.z = (vm->posInterpFinal.z - vm->posInterpInitial.z) * t + vm->posInterpInitial.z;
                }
                else
                {
                    vm->offset.x = (vm->posInterpFinal.x - vm->posInterpInitial.x) * t + vm->posInterpInitial.x;
                    vm->offset.y = (vm->posInterpFinal.y - vm->posInterpInitial.y) * t + vm->posInterpInitial.y;
                    vm->offset.z = (vm->posInterpFinal.z - vm->posInterpInitial.z) * t + vm->posInterpInitial.z;
                }
                break;
            case 1:
                vm->color.bytes.r =
                    (u8)((f32)((i32)vm->colorInterpFinalColor.bytes.r -
                               (i32)vm->colorInterpInitialColor.bytes.r) *
                             t +
                         (f32)vm->colorInterpInitialColor.bytes.r);
                vm->color.bytes.g =
                    (u8)((f32)((i32)vm->colorInterpFinalColor.bytes.g -
                               (i32)vm->colorInterpInitialColor.bytes.g) *
                             t +
                         (f32)vm->colorInterpInitialColor.bytes.g);
                vm->color.bytes.b =
                    (u8)((f32)((i32)vm->colorInterpFinalColor.bytes.b -
                               (i32)vm->colorInterpInitialColor.bytes.b) *
                             t +
                         (f32)vm->colorInterpInitialColor.bytes.b);
                break;
            case 2:
                vm->color.bytes.a =
                    (u8)((f32)((i32)vm->colorInterpFinalColor.bytes.a -
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
        vm->scale.y +=
            g_Supervisor.effectiveFramerateMultiplier * vm->scaleGrowth.y;
        vm->updateScale = 1;
    }
    if (vm->scaleGrowth.x != 0.0f)
    {
        vm->scale.x +=
            g_Supervisor.effectiveFramerateMultiplier * vm->scaleGrowth.x;
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

// FUNCTION: TH07 0x00454260
void AnmManager::DrawTextToSprite(u32 spriteDstIdx, i32 x, i32 y, i32 width,
                                  i32 height, i32 fontWidth, i32 fontHeight,
                                  D3DCOLOR textColor, u32 outlineType,
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
    TextHelper::RenderTextToTextureBold(x, y, width, height,
                                        (f32)fontWidth * scaleY,
                                        (f32)fontHeight * scaleX, textColor,
                                        outlineType, strToPrint, this->textures[spriteDstIdx]);
}

#pragma var_order(args, text, fontWidth)
// FUNCTION: TH07 0x004542d0
void AnmManager::DrawVmTextFmt(AnmManager *manager, AnmVm *vm,
                               D3DCOLOR textColor, u32 outlineType,
                               const char *str, ...)
{
    u32 fontWidth;
    char text[72];
    va_list args;

    fontWidth = vm->fontWidth;

    va_start(args, str);
    vsprintf(text, str, args);
    va_end(args);

    manager->DrawTextToSprite(
        vm->sprite->sourceFileIndex,
        vm->sprite->startPixelInclusive.x,
        vm->sprite->startPixelInclusive.y,
        vm->sprite->textureWidth,
        vm->sprite->textureHeight,
        fontWidth,
        vm->fontHeight,
        textColor,
        outlineType,
        text,
        vm->sprite->cols,
        vm->sprite->rows);

    vm->visible = 1;
}

#pragma var_order(args, x, buf, fontWidth)
// FUNCTION: TH07 0x004543b0
void AnmManager::DrawStringFormat(AnmVm *vm, D3DCOLOR textColor,
                                  u32 outlineType, const char *text, ...)
{
    i32 fontWidth;
    char buf[72];
    i32 x;
    va_list args;

    fontWidth = vm->fontWidth <= 0 ? 15 : (u32)vm->fontWidth;
    va_start(args, text);
    vsprintf(buf, text, args);
    va_end(args);

    this->DrawTextToSprite(vm->sprite->sourceFileIndex, vm->sprite->startPixelInclusive.x,
                           vm->sprite->startPixelInclusive.y, vm->sprite->textureWidth,
                           vm->sprite->textureHeight, fontWidth, vm->fontHeight, textColor,
                           outlineType, (char *)" ", vm->sprite->cols, vm->sprite->rows);

    x = vm->sprite->startPixelInclusive.x + vm->sprite->widthPx * vm->sprite->cols -
        (f32)strlen(buf) * (f32)fontWidth * vm->sprite->cols / 2.0f;

    this->DrawTextToSprite(vm->sprite->sourceFileIndex, x, vm->sprite->startPixelInclusive.y,
                           vm->sprite->textureWidth, vm->sprite->textureHeight, fontWidth,
                           vm->fontHeight, textColor, outlineType, buf, vm->sprite->cols,
                           vm->sprite->rows);

    vm->visible = 1;
}

#pragma var_order(args, x, buf, fontWidth)
// FUNCTION: TH07 0x004545b0
void AnmManager::DrawStringFormat2(AnmVm *vm, D3DCOLOR textColor,
                                   u32 outlineType, const char *text, ...)
{
    i32 fontWidth;
    char buf[72];
    i32 x;
    va_list args;

    fontWidth = vm->fontWidth <= 0 ? 15 : (i32)vm->fontWidth;
    va_start(args, text);
    vsprintf(buf, text, args);
    va_end(args);

    this->DrawTextToSprite(vm->sprite->sourceFileIndex, vm->sprite->startPixelInclusive.x,
                           vm->sprite->startPixelInclusive.y, vm->sprite->textureWidth,
                           vm->sprite->textureHeight, fontWidth, vm->fontHeight, textColor,
                           outlineType, (char *)" ", vm->sprite->cols, vm->sprite->rows);

    x = (i32)(vm->sprite->startPixelInclusive.x + vm->sprite->widthPx * vm->sprite->cols / 2.0f -
              (f32)strlen(buf) * fontWidth * vm->sprite->cols / 4.0f);

    this->DrawTextToSprite(vm->sprite->sourceFileIndex, x, vm->sprite->startPixelInclusive.y,
                           vm->sprite->textureWidth, vm->sprite->textureHeight, fontWidth, vm->fontHeight,
                           textColor, outlineType, buf, vm->sprite->cols, vm->sprite->rows);

    vm->visible = 1;
}

// FUNCTION: TH07 0x004547b0
ZunResult AnmManager::LoadSurface(i32 surfaceIdx, const char *path)
{
    IDirect3DSurface8 *surface;

    if (this->surfaces[surfaceIdx])
    {
        ReleaseSurface(surfaceIdx);
    }
    u8 *data = FileSystem::OpenFile(path, 0);
    if (!data)
    {
        // STRING: TH07 0x00495b30
        g_GameErrorContext.Fatal("%sが読み込めないです。\r\n", path);
        return ZUN_ERROR;
    }
    if (g_Supervisor.d3dDevice->CreateImageSurface(
            640, 1024, g_Supervisor.presentParameters.BackBufferFormat,
            &surface))
    {
        return ZUN_ERROR;
    }

    if (D3DXLoadSurfaceFromFileInMemory(
            surface, NULL, NULL, data, g_LastFileSize, NULL, 1, 0,
            (D3DXIMAGE_INFO *)&this->surfaceSourceInfo[surfaceIdx]))
    {
        goto err;
    }

    if (g_Supervisor.d3dDevice->CreateRenderTarget(
            this->surfaceSourceInfo[surfaceIdx].width,
            this->surfaceSourceInfo[surfaceIdx].height,
            g_Supervisor.presentParameters.BackBufferFormat, D3DMULTISAMPLE_NONE,
            1, this->surfaces + surfaceIdx))
    {
        if (g_Supervisor.d3dDevice->CreateImageSurface(
                this->surfaceSourceInfo[surfaceIdx].width,
                this->surfaceSourceInfo[surfaceIdx].height,
                g_Supervisor.presentParameters.BackBufferFormat,
                this->surfaces + surfaceIdx))
        {
            goto err;
        }
    }

    if (g_Supervisor.d3dDevice->CreateImageSurface(
            this->surfaceSourceInfo[surfaceIdx].width,
            this->surfaceSourceInfo[surfaceIdx].height,
            g_Supervisor.presentParameters.BackBufferFormat,
            this->surfacesBis + surfaceIdx))
    {
        goto err;
    }

    if (D3DXLoadSurfaceFromSurface(this->surfaces[surfaceIdx], 0, NULL, surface,
                                   0, NULL, 1, 0))
    {
        goto err;
    }
    if (D3DXLoadSurfaceFromSurface(this->surfacesBis[surfaceIdx], 0, NULL,
                                   surface, 0, NULL, 1, 0))
    {
        goto err;
    }

    SAFE_RELEASE(surface);
    free(data);
    return ZUN_SUCCESS;

err:
    SAFE_RELEASE(surface);
    free(data);
    return ZUN_ERROR;
}

// FUNCTION: TH07 0x00454a10
void AnmManager::ReleaseSurface(i32 surfaceIdx)
{
    SAFE_RELEASE(this->surfaces[surfaceIdx]);
    SAFE_RELEASE(this->surfacesBis[surfaceIdx]);
}

// FUNCTION: TH07 0x00454aa0
void AnmManager::CopySurfaceToBackBuffer(i32 surfaceIdx, i32 left, i32 top,
                                         i32 x, i32 y)
{
    if (!this->surfacesBis[surfaceIdx])
    {
        return;
    }
    IDirect3DSurface8 *dstSurface;
    if (g_Supervisor.d3dDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO,
                                              &dstSurface))
    {
        return;
    }

    if (!this->surfaces[surfaceIdx])
    {
        if (g_Supervisor.d3dDevice->CreateRenderTarget(
                this->surfaceSourceInfo[surfaceIdx].width,
                this->surfaceSourceInfo[surfaceIdx].height,
                g_Supervisor.presentParameters.BackBufferFormat,
                D3DMULTISAMPLE_NONE, 1, this->surfaces + surfaceIdx) &&
            g_Supervisor.d3dDevice->CreateImageSurface(
                this->surfaceSourceInfo[surfaceIdx].width,
                this->surfaceSourceInfo[surfaceIdx].height,
                g_Supervisor.presentParameters.BackBufferFormat,
                this->surfaces + surfaceIdx))
        {
            dstSurface->Release();
            return;
        }
        if (D3DXLoadSurfaceFromSurface(this->surfaces[surfaceIdx], NULL, NULL,
                                       this->surfacesBis[surfaceIdx], NULL, NULL,
                                       1, 0))
        {
            dstSurface->Release();
            return;
        }
    }
    RECT srcRect = {left, top, (LONG)this->surfaceSourceInfo[surfaceIdx].width,
                    (LONG)this->surfaceSourceInfo[surfaceIdx].height};
    POINT dstPoint = {x, y};
    g_Supervisor.d3dDevice->CopyRects(this->surfaces[surfaceIdx], &srcRect, 1,
                                      dstSurface, &dstPoint);
    dstSurface->Release();
}

// FUNCTION: TH07 0x00454c60
void AnmManager::DrawEndingRect(i32 surfaceIdx, i32 rectX, i32 rectY,
                                i32 rectLeft, i32 rectTop, i32 width,
                                i32 height)
{
    if (!this->surfacesBis[surfaceIdx])
    {
        return;
    }
    IDirect3DSurface8 *backBuffer;
    if (g_Supervisor.d3dDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO,
                                              &backBuffer))
    {
        return;
    }
    if (!this->surfaces[surfaceIdx])
    {
        if (g_Supervisor.d3dDevice->CreateRenderTarget(
                this->surfaceSourceInfo[surfaceIdx].width,
                this->surfaceSourceInfo[surfaceIdx].height,
                g_Supervisor.presentParameters.BackBufferFormat,
                D3DMULTISAMPLE_NONE, 1, this->surfaces + surfaceIdx) &&
            g_Supervisor.d3dDevice->CreateImageSurface(
                this->surfaceSourceInfo[surfaceIdx].width,
                this->surfaceSourceInfo[surfaceIdx].height,
                g_Supervisor.presentParameters.BackBufferFormat,
                this->surfaces + surfaceIdx))
        {
            backBuffer->Release();
            return;
        }
        if (D3DXLoadSurfaceFromSurface(this->surfaces[surfaceIdx], 0, NULL,
                                       this->surfacesBis[surfaceIdx], 0, NULL, 1,
                                       0))
        {
            backBuffer->Release();
            return;
        }
    }
    RECT rect = {rectLeft, rectTop, rectLeft + width, rectTop + height};
    POINT point = {rectX, rectY};
    g_Supervisor.d3dDevice->CopyRects(this->surfaces[surfaceIdx], &rect, 1,
                                      backBuffer, &point);
    backBuffer->Release();
}

#pragma var_order(srcRect, dstSurface, srcSurface, dstRect)
// FUNCTION: TH07 0x00454e10
void AnmManager::TakeScreenshot(i32 textureId, i32 srcLeft, i32 srcTop,
                                i32 srcWidth, i32 srcHeight, i32 dstLeft,
                                i32 dstTop, i32 dstWidth, i32 dstHeight)
{
    RECT dstRect;
    IDirect3DSurface8 *srcSurface;
    IDirect3DSurface8 *dstSurface;
    RECT srcRect;

    if (!this->textures[textureId])
    {
        return;
    }

    Flush();
    if (g_Supervisor.d3dDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO,
                                              &srcSurface))
    {
        return;
    }

    if (this->textures[textureId]->GetSurfaceLevel(0, &dstSurface))
    {
        srcSurface->Release();
        return;
    }

    srcRect.left = srcLeft;
    srcRect.top = srcTop;
    srcRect.right = srcLeft + srcWidth;
    srcRect.bottom = srcTop + srcHeight;
    dstRect.left = dstLeft;
    dstRect.top = dstTop;
    dstRect.right = dstLeft + dstWidth;
    dstRect.bottom = dstTop + dstHeight;
    if (D3DXLoadSurfaceFromSurface(dstSurface, 0, &dstRect, srcSurface, 0,
                                   &srcRect, 0xffffffff, 0))
    {
        dstSurface->Release();
        srcSurface->Release();
    }
    else
    {
        dstSurface->Release();
        srcSurface->Release();
    }
}

#pragma var_order(dstSurf, srcSurf)
// FUNCTION: TH07 0x00454f30
void AnmManager::CopyTexture(i32 dstIdx, i32 srcIdx, RECT *dstRect, RECT *srcRect)
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
    IDirect3DSurface8 *dstSurf, *srcSurf;
    if (this->textures[dstIdx]->GetSurfaceLevel(0, &dstSurf))
    {
        return;
    }

    if (this->textures[srcIdx]->GetSurfaceLevel(0, &srcSurf))
    {
        dstSurf->Release();
        return;
    }

    if (D3DXLoadSurfaceFromSurface(dstSurf, 0, dstRect, srcSurf, 0, srcRect,
                                   0xffffffff, 0))
    {
        dstSurf->Release();
        srcSurf->Release();
    }
    else
    {
        dstSurf->Release();
        srcSurf->Release();
    }
}

// FUNCTION: TH07 0x00455030
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
            shouldSetInterrupt = g_AnmManager->textures[vm->sprite->sourceFileIndex] != NULL;
        }
        if (shouldSetInterrupt)
        {
            vm->pendingInterrupt = interrupt;
        }
        vm++;
        vmCount--;
    }
}

// FUNCTION: TH07 0x004550c0
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

// FUNCTION: TH07 0x00455110
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

#pragma var_order(uvY, i, vertex, startuvX, uvX, fVar4, num)
// FUNCTION: TH07 0x00455170
ZunResult AnmManager::UpdateTrail(AnmVm *vm, VertexTex1DiffuseXyzrhw *vertices,
                                  i32 count)
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
    fVar4 = num / (float)((count + 1) / 2 - 1);

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

// FUNCTION: TH07 0x004552d0
ZunResult AnmManager::DrawTriangleStrip(AnmVm *vm,
                                        VertexTex1DiffuseXyzrhw *vertices,
                                        i32 count)
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
        g_Supervisor.d3dDevice->SetTexture(
            0, (IDirect3DBaseTexture8 *)this->currentTexture);
    }

    if (this->currentVertexShader != 3)
    {
        g_Supervisor.d3dDevice->SetVertexShader(0x144);
        this->currentVertexShader = 3;
    }

    SetRenderStateForVm(vm);
    g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, count - 2,
                                            vertices,
                                            sizeof(VertexTex1DiffuseXyzrhw));
    return ZUN_SUCCESS;
}
