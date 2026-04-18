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
#include "dxutil.hpp"
#include "utils.hpp"

// GLOBAL: TH07 0x004b9e44
AnmManager *g_AnmManager;

// GLOBAL: TH07 0x004b9fa8
VertexTex1DiffuseXyzrwh g_PrimitivesToDrawNoVertexBuf[4];

// GLOBAL: TH07 0x004ba018
VertexTex1Xyzrwh g_PrimitivesToDrawVertexBuf[4];

// GLOBAL: TH07 0x004ba078
VertexTex1DiffuseXyz g_PrimitivesToDrawUnknown[4];

// FUNCTION: TH07 0x0044d3e0
AnmManager::AnmManager()
{
    i32 spriteIndex;

    memset(this, 0, sizeof(AnmManager));

    for (spriteIndex = 0; spriteIndex < 0xa00; spriteIndex = spriteIndex + 1)
    {
        this->sprites[spriteIndex].sourceFileIndex = -1;
    }
    g_PrimitivesToDrawVertexBuf[3].w = 1.0f;
    g_PrimitivesToDrawVertexBuf[2].w = g_PrimitivesToDrawVertexBuf[3].w;
    g_PrimitivesToDrawVertexBuf[1].w = g_PrimitivesToDrawVertexBuf[2].w;
    g_PrimitivesToDrawVertexBuf[0].w = g_PrimitivesToDrawVertexBuf[1].w;
    g_PrimitivesToDrawVertexBuf[0].textureUV.x = 0.0f;
    g_PrimitivesToDrawVertexBuf[0].textureUV.y = 0.0f;
    g_PrimitivesToDrawVertexBuf[1].textureUV.x = 1.0f;
    g_PrimitivesToDrawVertexBuf[1].textureUV.y = 0.0f;
    g_PrimitivesToDrawVertexBuf[2].textureUV.x = 0.0f;
    g_PrimitivesToDrawVertexBuf[2].textureUV.y = 1.0f;
    g_PrimitivesToDrawVertexBuf[3].textureUV.x = 1.0f;
    g_PrimitivesToDrawVertexBuf[3].textureUV.y = 1.0f;
    g_PrimitivesToDrawNoVertexBuf[3].w = 1.0f;
    g_PrimitivesToDrawNoVertexBuf[2].w = g_PrimitivesToDrawNoVertexBuf[3].w;
    g_PrimitivesToDrawNoVertexBuf[1].w = g_PrimitivesToDrawNoVertexBuf[2].w;
    g_PrimitivesToDrawNoVertexBuf[0].w = g_PrimitivesToDrawNoVertexBuf[1].w;
    g_PrimitivesToDrawNoVertexBuf[0].textureUV.x = 0.0f;
    g_PrimitivesToDrawNoVertexBuf[0].textureUV.y = 0.0f;
    g_PrimitivesToDrawNoVertexBuf[1].textureUV.x = 1.0f;
    g_PrimitivesToDrawNoVertexBuf[1].textureUV.y = 0.0f;
    g_PrimitivesToDrawNoVertexBuf[2].textureUV.x = 0.0f;
    g_PrimitivesToDrawNoVertexBuf[2].textureUV.y = 1.0f;
    g_PrimitivesToDrawNoVertexBuf[3].textureUV.x = 1.0f;
    g_PrimitivesToDrawNoVertexBuf[3].textureUV.y = 1.0f;
    this->vertexBuffer = NULL;
    this->currentTexture = NULL;
    this->currentBlendMode = 0;
    this->currentColorOp = 0;
    this->currentTextureFactor.color = 1;
    this->currentVertexShader = 0;
    this->currentCameraMode = 0xff;
    this->currentZWriteDisable = 0;
    this->screenshotTextureId = -1;
}

// FUNCTION: TH07 0x0044d620
AnmManager::~AnmManager()
{
}

// FUNCTION: TH07 0x0044d630
void AnmManager::SetupVertexBuffer()
{
    RenderVertexInfo *local_8;

    this->vertexBufferContents[2].position.x = -128.0f;
    this->vertexBufferContents[0].position.x = -128.0f;
    this->vertexBufferContents[3].position.x = 128.0f;
    this->vertexBufferContents[1].position.x = 128.0f;
    this->vertexBufferContents[1].position.y = -128.0f;
    this->vertexBufferContents[0].position.y = -128.0f;
    this->vertexBufferContents[3].position.y = 128.0f;
    this->vertexBufferContents[2].position.y = 128.0f;
    this->vertexBufferContents[3].position.z = 0.0f;
    this->vertexBufferContents[2].position.z = 0.0f;
    this->vertexBufferContents[1].position.z = 0.0f;
    this->vertexBufferContents[0].position.z = 0.0f;
    this->vertexBufferContents[2].textureUV.x = 0.0f;
    this->vertexBufferContents[0].textureUV.x = 0.0f;
    this->vertexBufferContents[3].textureUV.x = 1.0f;
    this->vertexBufferContents[1].textureUV.x = 1.0f;
    this->vertexBufferContents[1].textureUV.y = 0.0f;
    this->vertexBufferContents[0].textureUV.y = 0.0f;
    this->vertexBufferContents[3].textureUV.y = 1.0f;
    this->vertexBufferContents[2].textureUV.y = 1.0f;
    g_PrimitivesToDrawUnknown[0].position =
        this->vertexBufferContents[0].position;
    g_PrimitivesToDrawUnknown[1].position =
        this->vertexBufferContents[1].position;
    g_PrimitivesToDrawUnknown[2].position =
        this->vertexBufferContents[2].position;
    g_PrimitivesToDrawUnknown[3].position =
        this->vertexBufferContents[3].position;
    g_PrimitivesToDrawUnknown[0].textureUV.x =
        this->vertexBufferContents[0].textureUV.x;
    g_PrimitivesToDrawUnknown[0].textureUV.y =
        this->vertexBufferContents[0].textureUV.y;
    g_PrimitivesToDrawUnknown[1].textureUV.x =
        this->vertexBufferContents[1].textureUV.x;
    g_PrimitivesToDrawUnknown[1].textureUV.y =
        this->vertexBufferContents[1].textureUV.y;
    g_PrimitivesToDrawUnknown[2].textureUV.x =
        this->vertexBufferContents[2].textureUV.x;
    g_PrimitivesToDrawUnknown[2].textureUV.y =
        this->vertexBufferContents[2].textureUV.y;
    g_PrimitivesToDrawUnknown[3].textureUV.x =
        this->vertexBufferContents[3].textureUV.x;
    g_PrimitivesToDrawUnknown[3].textureUV.y =
        this->vertexBufferContents[3].textureUV.y;
    if ((g_Supervisor.cfg.opts >> 1 & 1) == 0)
    {
        g_Supervisor.d3dDevice->CreateVertexBuffer(
            sizeof(this->vertexBufferContents), 0, 0x102, D3DPOOL_MANAGED,
            &this->vertexBuffer);
        this->vertexBuffer->Lock(0, 0, (u8 **)&local_8, 0);
        memcpy(local_8, this->vertexBufferContents,
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
    if ((g_Supervisor.cfg.opts >> 2 & 1) != 0)
    {
        if ((g_TextureFormatD3D8Mapping[formatIdx] == D3DFMT_A8R8G8B8) ||
            (g_TextureFormatD3D8Mapping[formatIdx] == D3DFMT_UNKNOWN))
        {
            formatIdx = 5;
        }
        else if (g_TextureFormatD3D8Mapping[formatIdx] == D3DFMT_R8G8B8)
        {
            formatIdx = 3;
        }
    }
    srcData = FileSystem::OpenFile(texturePath, 1);
    if (srcData == NULL)
        return ZUN_ERROR;

    if (D3DXCreateTextureFromFileInMemoryEx(
            g_Supervisor.d3dDevice, srcData, g_LastFileSize, 0, 0, 0, 0,
            g_TextureFormatD3D8Mapping[formatIdx], D3DPOOL_MANAGED, 3, 0xffffffff,
            colorKey, NULL, NULL, this->textures + textureIdx) != 0)
    {
        free(srcData);
        return ZUN_ERROR;
    }
    this->imageDataArray[textureIdx] = srcData;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0044d9e0
ZunResult AnmManager::LoadTextureEmbedded(u32 textureIdx,
                                          ZunImageInfoEmbedded *imageInfo,
                                          D3DCOLOR formatIdx)
{
    D3DLOCKED_RECT lockedRect;
    ZunImageInfoEmbedded *info;
    IDirect3DSurface8 *local_c;
    IDirect3DSurface8 *surf;

    ReleaseTexture(textureIdx);
    if ((g_Supervisor.cfg.opts >> 2 & 1) != 0)
    {
        if ((g_TextureFormatD3D8Mapping[formatIdx] == D3DFMT_A8R8G8B8) ||
            (g_TextureFormatD3D8Mapping[formatIdx] == D3DFMT_UNKNOWN))
        {
            formatIdx = 5;
        }
        else if (g_TextureFormatD3D8Mapping[formatIdx] == D3DFMT_R8G8B8)
        {
            formatIdx = 3;
        }
    }
    info = imageInfo;
    g_Supervisor.d3dDevice->CreateImageSurface(
        (i32)imageInfo->width, (i32)imageInfo->height,
        g_TextureFormatD3D8Mapping[imageInfo->format], &surf);
    surf->LockRect(&lockedRect, NULL, 0);
    for (i32 i = 0; i < info->height; i = i + 1)
    {
        u32 uVar3 = (i32)info->width * g_TextureBytesPerPixel[info->format];
        memcpy((u8 *)((i32)lockedRect.pBits + i * lockedRect.Pitch),
               &imageInfo
                    ->data[i * info->width * g_TextureBytesPerPixel[info->format]],
               uVar3);
    }
    surf->UnlockRect();
    if (D3DXCreateTexture(g_Supervisor.d3dDevice, (i32)info->width,
                          (i32)info->height, 1, 0,
                          g_TextureFormatD3D8Mapping[formatIdx], D3DPOOL_MANAGED,
                          this->textures + textureIdx) == 0)
    {
        this->textures[textureIdx]->GetSurfaceLevel(0, &local_c);
        if (D3DXLoadSurfaceFromSurface(local_c, 0, NULL, surf, 0, NULL, 3, 0) ==
            0)
        {
            SAFE_RELEASE(surf);
            if (local_c != NULL)
            {
                local_c->Release();
            }
            return ZUN_SUCCESS;
        }
        else
        {
            return ZUN_ERROR;
        }
    }
    else
    {
        return ZUN_ERROR;
    }
}

// FUNCTION: TH07 0x0044dbe0
ZunResult AnmManager::LoadTextureAlphaChannel(i32 textureIdx,
                                              const char *texturePath,
                                              i32 formatIdx, D3DCOLOR colorKey)
{
    u32 x2;
    u32 y2;
    i16 *a4r4g4b4_src_data;
    u16 *a4r4g4b4_dst_data;
    u32 x1;
    u32 y1;
    u16 *a1r5g5b5_src_data;
    u16 *a1r5g5b5_dst_data;
    u32 x0;
    u32 y0;
    u8 *a8r8g8b8_src_data;
    u8 *a8r8g8b8_dst_data;
    LPDIRECT3DTEXTURE8 textureSrc;
    D3DLOCKED_RECT lockedRectSrc;
    D3DLOCKED_RECT lockedRectDst;
    u8 *data;
    D3DSURFACE_DESC surfaceDesc;

    textureSrc = NULL;
    data = FileSystem::OpenFile(texturePath, 0);
    if (data != NULL)
    {
        this->textures[textureIdx]->GetLevelDesc(0, &surfaceDesc);
        if (((surfaceDesc.Format == D3DFMT_A8R8G8B8) ||
             (surfaceDesc.Format == D3DFMT_A4R4G4B4)) ||
            (surfaceDesc.Format == D3DFMT_A1R5G5B5))
        {
            if (((D3DXCreateTextureFromFileInMemoryEx(
                      g_Supervisor.d3dDevice, data, g_LastFileSize, 0, 0, 0, 0,
                      surfaceDesc.Format, D3DPOOL_SYSTEMMEM, 3, 0xffffffff, colorKey,
                      NULL, NULL, &textureSrc) == 0) &&
                 (this->textures[textureIdx]->LockRect(0, &lockedRectDst, NULL, 0) ==
                  0)) &&
                (textureSrc->LockRect(0, &lockedRectSrc, NULL, 0x8000) == 0))
            {
                if (surfaceDesc.Format == D3DFMT_A8R8G8B8)
                {
                    for (y0 = 0; y0 < surfaceDesc.Height; y0 = y0 + 1)
                    {
                        a8r8g8b8_dst_data =
                            (u8 *)((u8 *)lockedRectDst.pBits + y0 * lockedRectDst.Pitch);
                        a8r8g8b8_src_data =
                            (u8 *)((u8 *)lockedRectSrc.pBits + y0 * lockedRectSrc.Pitch);
                        for (x0 = 0; x0 < surfaceDesc.Width; x0 = x0 + 1)
                        {
                            a8r8g8b8_dst_data[3] = *a8r8g8b8_src_data;
                            a8r8g8b8_src_data = a8r8g8b8_src_data + 4;
                            a8r8g8b8_dst_data = a8r8g8b8_dst_data + 4;
                        }
                    }
                }
                else if (surfaceDesc.Format == D3DFMT_A1R5G5B5)
                {
                    for (y1 = 0; y1 < surfaceDesc.Height; y1 = y1 + 1)
                    {
                        a1r5g5b5_dst_data =
                            (u16 *)((u8 *)lockedRectDst.pBits + y1 * lockedRectDst.Pitch);
                        a1r5g5b5_src_data =
                            (u16 *)((u8 *)lockedRectSrc.pBits + y1 * lockedRectSrc.Pitch);
                        for (x1 = 0; x1 < surfaceDesc.Width; x1 = x1 + 1)
                        {
                            *a1r5g5b5_dst_data =
                                (*a1r5g5b5_dst_data & 0x7fff) |
                                (i16)((i32)(u32)(*a1r5g5b5_src_data & 0x1f) >> 4) << 0xf;
                            a1r5g5b5_src_data = a1r5g5b5_src_data + 1;
                            a1r5g5b5_dst_data = a1r5g5b5_dst_data + 1;
                        }
                    }
                }
                else if (surfaceDesc.Format == D3DFMT_A4R4G4B4)
                {
                    for (y2 = 0; y2 < surfaceDesc.Height; y2 = y2 + 1)
                    {
                        a4r4g4b4_dst_data =
                            (u16 *)((i32)lockedRectDst.pBits + y2 * lockedRectDst.Pitch);
                        a4r4g4b4_src_data =
                            (i16 *)((i32)lockedRectSrc.pBits + y2 * lockedRectSrc.Pitch);
                        for (x2 = 0; x2 < surfaceDesc.Width; x2 = x2 + 1)
                        {
                            *a4r4g4b4_dst_data =
                                (*a4r4g4b4_dst_data & 0xfff) | *a4r4g4b4_src_data << 0xc;
                            a4r4g4b4_src_data = a4r4g4b4_src_data + 1;
                            a4r4g4b4_dst_data = a4r4g4b4_dst_data + 1;
                        }
                    }
                }
                textureSrc->UnlockRect(0);
                this->textures[textureIdx]->UnlockRect(0);
                SAFE_RELEASE(textureSrc);
                free(data);
                return ZUN_SUCCESS;
            }
        }
        else
        {
            // STRING: TH07 0x00495cb8
            g_GameErrorContext.Fatal("error : イメージがαを持っていません\r\n");
        }
        SAFE_RELEASE(textureSrc);
        free(data);
    }
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
    if (entry == NULL)
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
        anmIdx = anmIdx + 1;
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

#pragma var_order(local_8, data, desc, name, rawSprite, i, curSprite, \
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
    i32 local_8;

    local_8 = 0;
    if (rawEntry == NULL)
    {
        g_GameErrorContext.Fatal("アニメが読み込めません。データが失われてるか壊れています\r\n");
        return ZUN_ERROR;
    }
    if (textureIdx >= 0x32)
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
    if (data->hasData == 0)
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
            ((rawSprite->offset).x + (rawSprite->size).x) * loadedSprite.cols;
        loadedSprite.endPixelInclusive.y =
            ((rawSprite->offset).y + (rawSprite->size).y) * loadedSprite.rows;
        loadedSprite.textureWidth = (f32)desc.Width;
        loadedSprite.textureHeight = (f32)desc.Height;
        if (local_8 < rawSprite->id)
        {
            local_8 = rawSprite->id;
        }
        if (rawSprite->id + spriteIdxOffset >= 0xa00)
        {
            // STRING: TH07 0x00495b80
            g_GameErrorContext.Fatal("スプライトが格納できません。テーブルが不足しています\r\n");
            return ZUN_ERROR;
        }
        LoadSprite(rawSprite->id + spriteIdxOffset, &loadedSprite);
    }
    for (i = 0; i < data->numScripts; i++, curSprite += 2)
    {
        if (*curSprite + spriteIdxOffset >= 0xa00)
        {
            // STRING: TH07 0x00495b4c
            g_GameErrorContext.Fatal("アニメが格納できません。テーブルが不足しています\r\n");
            return ZUN_ERROR;
        }
        if (local_8 < *curSprite)
        {
            local_8 = *curSprite;
        }
        this->scripts[*curSprite + spriteIdxOffset] =
            (AnmRawInstr *)((u8 *)data + curSprite[1]);
        this->spriteIndices[*curSprite + spriteIdxOffset] = spriteIdxOffset;
    }
    this->anmFiles[textureIdx].raw = data;
    this->anmFiles[textureIdx].spriteIndexOffset = spriteIdxOffset;
    return local_8 + 1;
}

#pragma var_order(spriteIdx, spriteIdxOffset, i, local_14, after_hdr, rawEntry)
// FUNCTION: TH07 0x0044e4e0
void AnmManager::ReleaseAnm(i32 anmIdx)
{
    AnmRawEntry *rawEntry;
    i32 *afterHdr;
    i32 local_14;
    i32 i;
    i32 spriteIdxOffset;
    i32 *spriteIdx;

    if (anmIdx < 0 || (u32)anmIdx >= 0x32)
        return;

    if (this->anmFiles[anmIdx].raw != NULL)
    {
        afterHdr = (this->anmFiles[anmIdx].raw)->spriteOffsets;
        spriteIdxOffset = this->anmFiles[anmIdx].spriteIndexOffset;
        rawEntry = this->anmFiles[anmIdx].raw;
        local_14 = anmIdx + 1;
        for (i = 1; local_14 = local_14 + 1, i < this->anmFiles[anmIdx].childCount;
             i++)
        {
            ReleaseAnm(local_14);
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
        if (rawEntry->ownsMemory != 0)
        {
            free(rawEntry);
        }
        this->anmFiles[anmIdx].raw = NULL;
        this->currentBlendMode = 0xff;
        this->currentColorOp = 0xff;
        this->currentVertexShader = 0x0;
        this->currentTexture = NULL;
        this->anmFiles[anmIdx].childCount = 0;
    }
}

// FUNCTION: TH07 0x0044e6f0
void AnmManager::ReleaseTexture(i32 textureIdx)
{
    void *imageData;
    if (textureIdx < 0 || (u32)textureIdx >= 0x108)
        return;

    SAFE_RELEASE(this->textures[textureIdx]);
    imageData = this->imageDataArray[textureIdx];
    free(imageData);
    this->imageDataArray[textureIdx] = NULL;
}

// FUNCTION: TH07 0x0044e780
void AnmManager::LoadSprite(u32 spriteIdx, AnmLoadedSprite *sprite)
{
    this->sprites[spriteIdx] = *sprite;
    this->sprites[spriteIdx].spriteId = this->loadedSpriteCount++;

    this->sprites[spriteIdx].uvStart.x =
        this->sprites[spriteIdx].startPixelInclusive.x /
        this->sprites[spriteIdx].textureWidth;
    this->sprites[spriteIdx].uvEnd.x =
        this->sprites[spriteIdx].endPixelInclusive.x /
        this->sprites[spriteIdx].textureWidth;
    this->sprites[spriteIdx].uvStart.y =
        this->sprites[spriteIdx].startPixelInclusive.y /
        this->sprites[spriteIdx].textureHeight;
    this->sprites[spriteIdx].uvEnd.y =
        this->sprites[spriteIdx].endPixelInclusive.y /
        this->sprites[spriteIdx].textureHeight;
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
    else
    {
        vm->activeSpriteIdx = (i16)spriteIdx;
        vm->sprite = &this->sprites[spriteIdx];
        D3DXMatrixIdentity(&vm->matrix);
        D3DXMatrixIdentity(&vm->uvMatrix);
        vm->matrix.m[0][0] = vm->sprite->widthPx / 256.0f;
        vm->matrix.m[1][1] = vm->sprite->heightPx / 256.0f;
        vm->uvMatrix.m[0][0] =
            (vm->sprite->widthPx / vm->sprite->textureWidth) * vm->sprite->cols;
        vm->uvMatrix.m[1][1] =
            (vm->sprite->heightPx / vm->sprite->textureHeight) * vm->sprite->rows;
        vm->worldTransformMatrix = vm->matrix;
        return ZUN_SUCCESS;
    }
}

// FUNCTION: TH07 0x0044ea20
void AnmManager::SetAndExecuteScript(AnmVm *vm, AnmRawInstr *beginningOfScript)
{
    i32 idk;

    if (beginningOfScript == NULL)
    {
        memset(vm, 0, sizeof(AnmVm));
    }
    else
    {
        vm->flip = 0;
        vm->Initialize();
        vm->beginningOfScript = beginningOfScript;
        vm->currentInstruction = vm->beginningOfScript;
        vm->currentTimeInScript.Initialize2(0);
        vm->visible = 0;
        ExecuteScript(vm);
        this->scriptsExecutedThisFrame = this->scriptsExecutedThisFrame + 1;
    }
}

// FUNCTION: TH07 0x0044eae0
void AnmManager::SetRenderStateForVm(AnmVm *vm)
{
    ZunColor DVar2;
    ZunColor local_30;
    u32 local_28;
    u32 local_24;
    u32 local_20;
    u32 local_1c;
    u32 local_18;
    u32 local_14;
    u32 local_10;
    u32 local_c;
    ZunColor color;

    if ((u32)this->currentBlendMode != vm->blendMode)
    {
        this->currentBlendMode = vm->blendMode;
        if (this->currentBlendMode == 0)
        {
            g_Supervisor.d3dDevice->SetRenderState(D3DRS_DESTBLEND, 6);
        }
        else
        {
            g_Supervisor.d3dDevice->SetRenderState(D3DRS_DESTBLEND, 2);
        }
    }
    if (vm->useColor2 == 0)
    {
        local_30 = vm->color;
    }
    else
    {
        local_30 = vm->color2;
    }
    color = local_30;
    DVar2 = color;
    color.bytes.r = local_30.bytes.r;
    color.bytes.a = local_30.bytes.a;
    color.bytes.b = local_30.bytes.b;
    color.bytes.g = local_30.bytes.g;
    if ((g_Supervisor.cfg.opts >> 1 & 1) == 0)
    {
        if (this->colorMulEnabled != 0)
        {
            local_c = (u32)color.bytes.r * (u32)(this->color).bytes.r >> 7;
            if (0xff < local_c)
            {
                local_c = 0xff;
            }
            color.bytes.r = (u8)local_c;
            local_10 = (u32)color.bytes.g * (u32)(this->color).bytes.g >> 7;
            if (0xff < local_10)
            {
                local_10 = 0xff;
            }
            color.bytes.g = (u8)local_10;
            local_14 = (u32)color.bytes.b * (u32)(this->color).bytes.b >> 7;
            if (0xff < local_14)
            {
                local_14 = 0xff;
            }
            color.bytes.b = (u8)local_14;
            local_18 = (u32)color.bytes.a * (u32)(this->color).bytes.a >> 7;
            if (0xff < local_18)
            {
                local_18 = 0xff;
            }
            color.bytes.a = (u8)local_18;
            DVar2 = color;
        }
        color = DVar2;
        if (this->currentTextureFactor.color != color.color)
        {
            this->currentTextureFactor.color = color.color;
            g_Supervisor.d3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR,
                                                   this->currentTextureFactor.color);
        }
    }
    else
    {
        if (this->colorMulEnabled != 0)
        {
            local_1c = (u32)color.bytes.r * (u32)(this->color).bytes.r >> 7;
            if (0xff < local_1c)
            {
                local_1c = 0xff;
            }
            color.bytes.r = (u8)local_1c;
            local_20 = (u32)color.bytes.g * (u32)(this->color).bytes.g >> 7;
            if (0xff < local_20)
            {
                local_20 = 0xff;
            }
            color.bytes.g = (u8)local_20;
            local_24 = (u32)color.bytes.b * (u32)(this->color).bytes.b >> 7;
            if (0xff < local_24)
            {
                local_24 = 0xff;
            }
            color.bytes.b = (u8)local_24;
            local_28 = (u32)color.bytes.a * (u32)(this->color).bytes.a >> 7;
            if (0xff < local_28)
            {
                local_28 = 0xff;
            }
            color.bytes.a = (u8)local_28;
            DVar2 = color;
        }
        color = DVar2;
        g_PrimitivesToDrawNoVertexBuf[0].color = color;
        g_PrimitivesToDrawNoVertexBuf[1].color = color;
        g_PrimitivesToDrawNoVertexBuf[2].color = color;
        g_PrimitivesToDrawNoVertexBuf[3].color = color;
        g_PrimitivesToDrawUnknown[0].diffuse = color;
        g_PrimitivesToDrawUnknown[1].diffuse = color;
        g_PrimitivesToDrawUnknown[2].diffuse = color;
        g_PrimitivesToDrawUnknown[3].diffuse = color;
    }
    if (((g_Supervisor.cfg.opts >> 6 & 1) == 0) &&
        ((u32)this->currentZWriteDisable != vm->zWriteDisable))
    {
        this->currentZWriteDisable = vm->zWriteDisable;
        if (this->currentZWriteDisable == 0)
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
        if (this->currentCameraMode == 0)
        {
            Stage::SetupCameraStageBackground();
            g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
        }
        else
        {
            g_Stage.UpdateCamera();
            g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
        }
    }
    this->renderStateChangesThisFrame += 1;
}

// FUNCTION: TH07 0x0044eec0
void AnmManager::SyncRenderState(AnmVm *vm)
{
    if ((u32)this->currentBlendMode != vm->blendMode)
    {
        this->currentBlendMode = vm->blendMode;
        if (this->currentBlendMode == 0)
        {
            g_Supervisor.SetRenderState(D3DRS_DESTBLEND, 6);
        }
        else
        {
            g_Supervisor.SetRenderState(D3DRS_DESTBLEND, 2);
        }
    }
    if (((g_Supervisor.cfg.opts >> 6 & 1) == 0) &&
        ((u32)this->currentZWriteDisable != vm->zWriteDisable))
    {
        this->currentZWriteDisable = vm->zWriteDisable;
        if (this->currentZWriteDisable == 0)
        {
            g_Supervisor.SetRenderState(D3DRS_ZWRITEENABLE, 1);
        }
        else
        {
            g_Supervisor.SetRenderState(D3DRS_ZWRITEENABLE, 0);
        }
    }
    this->renderStateChangesThisFrame += 1;
}

static f32 g_ZeroPointFive = 0.5;

// FUNCTION: TH07 0x0044efb0
ZunResult AnmManager::DrawInner(AnmVm *vm, u32 param2)
{
    g_PrimitivesToDrawNoVertexBuf[0].pos.x += this->offset.x;
    g_PrimitivesToDrawNoVertexBuf[0].pos.y += this->offset.y;
    g_PrimitivesToDrawNoVertexBuf[1].pos.x += this->offset.x;
    g_PrimitivesToDrawNoVertexBuf[1].pos.y += this->offset.y;
    g_PrimitivesToDrawNoVertexBuf[2].pos.x += this->offset.x;
    g_PrimitivesToDrawNoVertexBuf[2].pos.y += this->offset.y;
    g_PrimitivesToDrawNoVertexBuf[3].pos.x += this->offset.x;
    g_PrimitivesToDrawNoVertexBuf[3].pos.y += this->offset.y;

    if ((param2 & 1) != 0)
    {
        /*g_PrimitivesToDrawNoVertexBuf[0].pos.x =
            roundf(g_PrimitivesToDrawNoVertexBuf[0].pos.x) - 0.5f;
        g_PrimitivesToDrawNoVertexBuf[1].pos.x =
            roundf(g_PrimitivesToDrawNoVertexBuf[1].pos.x) - 0.5f;
        g_PrimitivesToDrawNoVertexBuf[0].pos.y =
            roundf(g_PrimitivesToDrawNoVertexBuf[0].pos.y) - 0.5f;
        g_PrimitivesToDrawNoVertexBuf[2].pos.y =
            roundf(g_PrimitivesToDrawNoVertexBuf[2].pos.y) - 0.5f;
        g_PrimitivesToDrawNoVertexBuf[1].pos.y =
            g_PrimitivesToDrawNoVertexBuf[0].pos.y;
        g_PrimitivesToDrawNoVertexBuf[2].pos.x =
            g_PrimitivesToDrawNoVertexBuf[0].pos.x;
        g_PrimitivesToDrawNoVertexBuf[3].pos.x =
            g_PrimitivesToDrawNoVertexBuf[1].pos.x;
        g_PrimitivesToDrawNoVertexBuf[3].pos.y =
        g_PrimitivesToDrawNoVertexBuf[2].pos.y;*/
        __asm {
        fld g_PrimitivesToDrawNoVertexBuf[0 * TYPE g_PrimitivesToDrawNoVertexBuf].pos.x
        frndint
        fsub g_ZeroPointFive
        fld g_PrimitivesToDrawNoVertexBuf[1 * TYPE g_PrimitivesToDrawNoVertexBuf].pos.x
        frndint
        fsub g_ZeroPointFive
        fld g_PrimitivesToDrawNoVertexBuf[0 * TYPE g_PrimitivesToDrawNoVertexBuf].pos.y
        frndint
        fsub g_ZeroPointFive
        fld g_PrimitivesToDrawNoVertexBuf[2 * TYPE g_PrimitivesToDrawNoVertexBuf].pos.y
        frndint
        fsub g_ZeroPointFive
        fst g_PrimitivesToDrawNoVertexBuf[2 * TYPE g_PrimitivesToDrawNoVertexBuf].pos.y
        fstp g_PrimitivesToDrawNoVertexBuf[3 * TYPE g_PrimitivesToDrawNoVertexBuf].pos.y
        fst g_PrimitivesToDrawNoVertexBuf[0 * TYPE g_PrimitivesToDrawNoVertexBuf].pos.y
        fstp g_PrimitivesToDrawNoVertexBuf[1 * TYPE g_PrimitivesToDrawNoVertexBuf].pos.y
        fst g_PrimitivesToDrawNoVertexBuf[1 * TYPE g_PrimitivesToDrawNoVertexBuf].pos.x
        fstp g_PrimitivesToDrawNoVertexBuf[3 * TYPE g_PrimitivesToDrawNoVertexBuf].pos.x
        fst g_PrimitivesToDrawNoVertexBuf[0 * TYPE g_PrimitivesToDrawNoVertexBuf].pos.x
        fstp g_PrimitivesToDrawNoVertexBuf[2 * TYPE g_PrimitivesToDrawNoVertexBuf].pos.x
        }
    }

    g_PrimitivesToDrawNoVertexBuf[0].textureUV.x =
        vm->sprite->uvStart.x + vm->uvScrollPos.x;
    g_PrimitivesToDrawNoVertexBuf[1].textureUV.x =
        vm->sprite->uvEnd.x + vm->uvScrollPos.x;
    g_PrimitivesToDrawNoVertexBuf[0].textureUV.y =
        vm->sprite->uvStart.y + vm->uvScrollPos.y;
    g_PrimitivesToDrawNoVertexBuf[2].textureUV.y =
        vm->sprite->uvEnd.y + vm->uvScrollPos.y;

    f32 local_30 = g_PrimitivesToDrawNoVertexBuf[0].pos.x <=
                           g_PrimitivesToDrawNoVertexBuf[1].pos.x
                       ? g_PrimitivesToDrawNoVertexBuf[1].pos.x
                       : g_PrimitivesToDrawNoVertexBuf[0].pos.x;
    f32 local_34 = g_PrimitivesToDrawNoVertexBuf[2].pos.x <= local_30
                       ? local_30
                       : g_PrimitivesToDrawNoVertexBuf[2].pos.x;
    f32 maxX = g_PrimitivesToDrawNoVertexBuf[3].pos.x <= local_34
                   ? local_34
                   : g_PrimitivesToDrawNoVertexBuf[3].pos.x;

    f32 local_3c = g_PrimitivesToDrawNoVertexBuf[0].pos.y <=
                           g_PrimitivesToDrawNoVertexBuf[1].pos.y
                       ? g_PrimitivesToDrawNoVertexBuf[1].pos.y
                       : g_PrimitivesToDrawNoVertexBuf[0].pos.y;
    f32 local_40 = g_PrimitivesToDrawNoVertexBuf[2].pos.y <= local_3c
                       ? local_3c
                       : g_PrimitivesToDrawNoVertexBuf[2].pos.y;
    f32 maxY = g_PrimitivesToDrawNoVertexBuf[3].pos.y <= local_40
                   ? local_40
                   : g_PrimitivesToDrawNoVertexBuf[3].pos.y;

    f32 local_48 = g_PrimitivesToDrawNoVertexBuf[1].pos.x <=
                           g_PrimitivesToDrawNoVertexBuf[0].pos.x
                       ? g_PrimitivesToDrawNoVertexBuf[1].pos.x
                       : g_PrimitivesToDrawNoVertexBuf[0].pos.x;
    f32 local_4c = local_48 <= g_PrimitivesToDrawNoVertexBuf[2].pos.x
                       ? local_48
                       : g_PrimitivesToDrawNoVertexBuf[2].pos.x;
    f32 minX = local_4c <= g_PrimitivesToDrawNoVertexBuf[3].pos.x
                   ? local_4c
                   : g_PrimitivesToDrawNoVertexBuf[3].pos.x;

    f32 local_54 = g_PrimitivesToDrawNoVertexBuf[1].pos.y <=
                           g_PrimitivesToDrawNoVertexBuf[0].pos.y
                       ? g_PrimitivesToDrawNoVertexBuf[1].pos.y
                       : g_PrimitivesToDrawNoVertexBuf[0].pos.y;
    f32 local_58 = local_54 <= g_PrimitivesToDrawNoVertexBuf[2].pos.y
                       ? local_54
                       : g_PrimitivesToDrawNoVertexBuf[2].pos.y;
    f32 minY = local_58 <= g_PrimitivesToDrawNoVertexBuf[3].pos.y
                   ? local_58
                   : g_PrimitivesToDrawNoVertexBuf[3].pos.y;

    g_PrimitivesToDrawNoVertexBuf[1].textureUV.y =
        g_PrimitivesToDrawNoVertexBuf[0].textureUV.y;
    g_PrimitivesToDrawNoVertexBuf[2].textureUV.x =
        g_PrimitivesToDrawNoVertexBuf[0].textureUV.x;
    g_PrimitivesToDrawNoVertexBuf[3].textureUV.x =
        g_PrimitivesToDrawNoVertexBuf[1].textureUV.x;
    g_PrimitivesToDrawNoVertexBuf[3].textureUV.y =
        g_PrimitivesToDrawNoVertexBuf[2].textureUV.y;

    if ((f32)g_Supervisor.viewport.X <= maxX &&
        (f32)g_Supervisor.viewport.Y <= maxY &&
        minX <= (f32)(g_Supervisor.viewport.X + g_Supervisor.viewport.Width) &&
        minY <= (f32)(g_Supervisor.viewport.Y + g_Supervisor.viewport.Height))
    {
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
        if ((param2 & 2) == 0)
        {
            ZunColor baseColor =
                vm->useColor2 == 0 ? vm->color : vm->color2;
            ZunColor finalColor = baseColor;
            if (this->colorMulEnabled != 0)
            {
                u32 r = baseColor.bytes.r * (u32)(this->color).bytes.r >> 7;
                if (0xff < r)
                {
                    r = 0xff;
                }
                finalColor.bytes.a = baseColor.bytes.a;
                finalColor.bytes.r = (u8)r;
                u32 g = baseColor.bytes.g * (u32)(this->color).bytes.g >> 7;
                if (0xff < g)
                {
                    g = 0xff;
                }
                finalColor.bytes.b = baseColor.bytes.b;
                finalColor.bytes.g = (u8)g;
                u32 b = baseColor.bytes.b * (u32)(this->color).bytes.b >> 7;
                if (0xff < b)
                {
                    b = 0xff;
                }
                finalColor.bytes.b = (u8)b;
                u32 a = finalColor.bytes.a * (u32)(this->color).bytes.a >> 7;
                if (0xff < a)
                {
                    a = 0xff;
                }
                finalColor.bytes.a = (u8)a;
            }
            g_PrimitivesToDrawNoVertexBuf[0].color = finalColor;
            g_PrimitivesToDrawNoVertexBuf[1].color = finalColor;
            g_PrimitivesToDrawNoVertexBuf[2].color = finalColor;
            g_PrimitivesToDrawNoVertexBuf[3].color = finalColor;
        }
        SyncRenderState(vm);
        PushSprite(g_PrimitivesToDrawNoVertexBuf);
    }
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
    if (this->spritesToDraw == 0)
        return;

    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, 0);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, 0);
    g_Supervisor.d3dDevice->SetVertexShader(0x144);
    g_Supervisor.d3dDevice->DrawPrimitiveUP(
        D3DPT_TRIANGLELIST, this->spritesToDraw << 1, this->vertexBufferStartPtr,
        sizeof(VertexTex1DiffuseXyzrwh));
    this->vertexBufferStartPtr = this->vertexBufferCurPtr;
    this->spritesToDraw = 0;
    this->flushesThisFrame = this->flushesThisFrame + 1;
}

// FUNCTION: TH07 0x0044f690
ZunResult AnmManager::PushSprite(VertexTex1DiffuseXyzrwh *spriteVertex)
{
    this->vertexBufferCurPtr[0] = spriteVertex[0];
    this->vertexBufferCurPtr[1] = spriteVertex[1];
    this->vertexBufferCurPtr[2] = spriteVertex[2];
    this->vertexBufferCurPtr[3] = spriteVertex[1];
    this->vertexBufferCurPtr[4] = spriteVertex[2];
    this->vertexBufferCurPtr[5] = spriteVertex[3];

    this->vertexBufferCurPtr = this->vertexBufferCurPtr + 6;
    this->spritesToDraw = this->spritesToDraw + 1;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0044f770
ZunResult AnmManager::DrawNoRotation(AnmVm *vm)
{
    if (vm->visible == 0 || vm->active == 0 ||
        vm->color.bytes.a == 0)
        return ZUN_ERROR;

    f32 centerX = (vm->sprite->widthPx * vm->scale.x) / 2.0f;
    f32 centerY = (vm->sprite->heightPx * vm->scale.y) / 2.0f;
    f32 fVar1;

    if ((vm->anchor & 1) == 0)
    {
        g_PrimitivesToDrawNoVertexBuf[2].pos.x = vm->pos.x - centerX;
        fVar1 = vm->pos.x;
    }
    else
    {
        g_PrimitivesToDrawNoVertexBuf[2].pos.x = vm->pos.x;
        fVar1 = centerX + vm->pos.x;
    }
    g_PrimitivesToDrawNoVertexBuf[3].pos.x = centerX + fVar1;

    if ((vm->anchor & 2) == 0)
    {
        g_PrimitivesToDrawNoVertexBuf[1].pos.y = vm->pos.y - centerY;
        centerX = vm->pos.y;
    }
    else
    {
        g_PrimitivesToDrawNoVertexBuf[1].pos.y = vm->pos.y;
        centerX = centerY + vm->pos.y;
    }
    g_PrimitivesToDrawNoVertexBuf[3].pos.y = centerY + centerX;

    g_PrimitivesToDrawNoVertexBuf[0].pos.z = vm->pos.z;
    g_PrimitivesToDrawNoVertexBuf[0].pos.x =
        g_PrimitivesToDrawNoVertexBuf[2].pos.x;
    g_PrimitivesToDrawNoVertexBuf[0].pos.y =
        g_PrimitivesToDrawNoVertexBuf[1].pos.y;
    g_PrimitivesToDrawNoVertexBuf[1].pos.x =
        g_PrimitivesToDrawNoVertexBuf[3].pos.x;
    g_PrimitivesToDrawNoVertexBuf[1].pos.z =
        g_PrimitivesToDrawNoVertexBuf[0].pos.z;
    g_PrimitivesToDrawNoVertexBuf[2].pos.y =
        g_PrimitivesToDrawNoVertexBuf[3].pos.y;
    g_PrimitivesToDrawNoVertexBuf[2].pos.z =
        g_PrimitivesToDrawNoVertexBuf[0].pos.z;
    g_PrimitivesToDrawNoVertexBuf[3].pos.z =
        g_PrimitivesToDrawNoVertexBuf[0].pos.z;

    return DrawInner(vm, 1);
}

// FUNCTION: TH07 0x0044f960
void AnmManager::TranslateRotation(VertexTex1DiffuseXyzrwh *param_1, f32 width,
                                   f32 height, f32 param_4, f32 param_5,
                                   f32 xOffset, f32 yOffset)
{
    (param_1->pos).x = (width * param_5 - height * param_4) + xOffset;
    (param_1->pos).y = height * param_5 + width * param_4 + yOffset;
}

// FUNCTION: TH07 0x0044f9a0
ZunResult AnmManager::Draw(AnmVm *vm)
{
    if (vm->rotation.z == 0.0f)
    {
        return DrawNoRotation(vm);
    }
    else if (vm->visible == 0 || vm->active == 0 ||
             vm->color.bytes.a == 0)
    {
        return ZUN_ERROR;
    }
    else
    {
        f32 cosZ;
        f32 sinZ;

        f32 z = vm->rotation.z;
        sincosf_macro(sinZ, cosZ, z);
        f32 width = (vm->sprite->widthPx * vm->scale.x) / 2.0f;
        f32 height = (vm->sprite->heightPx * vm->scale.y) / 2.0f;

        TranslateRotation(&g_PrimitivesToDrawNoVertexBuf[0], -width, -height, sinZ,
                          cosZ, vm->pos.x, vm->pos.y);
        TranslateRotation(&g_PrimitivesToDrawNoVertexBuf[1], width, -height, sinZ,
                          cosZ, vm->pos.x, vm->pos.y);
        TranslateRotation(&g_PrimitivesToDrawNoVertexBuf[2], -width, height, sinZ,
                          cosZ, vm->pos.x, vm->pos.y);
        TranslateRotation(&g_PrimitivesToDrawNoVertexBuf[3], width, height, sinZ,
                          cosZ, vm->pos.x, vm->pos.y);

        g_PrimitivesToDrawNoVertexBuf[0].pos.z = vm->pos.z;
        if ((vm->anchor & 1) != 0)
        {
            g_PrimitivesToDrawNoVertexBuf[0].pos.x += width;
            g_PrimitivesToDrawNoVertexBuf[1].pos.x += width;
            g_PrimitivesToDrawNoVertexBuf[2].pos.x += width;
            g_PrimitivesToDrawNoVertexBuf[3].pos.x += width;
        }
        if ((vm->anchor & 2) != 0)
        {
            g_PrimitivesToDrawNoVertexBuf[0].pos.y += height;
            g_PrimitivesToDrawNoVertexBuf[1].pos.y += height;
            g_PrimitivesToDrawNoVertexBuf[2].pos.y += height;
            g_PrimitivesToDrawNoVertexBuf[3].pos.y += height;
        }
        g_PrimitivesToDrawNoVertexBuf[1].pos.z =
            g_PrimitivesToDrawNoVertexBuf[0].pos.z;
        g_PrimitivesToDrawNoVertexBuf[2].pos.z =
            g_PrimitivesToDrawNoVertexBuf[0].pos.z;
        g_PrimitivesToDrawNoVertexBuf[3].pos.z =
            g_PrimitivesToDrawNoVertexBuf[0].pos.z;

        return DrawInner(vm, 0);
    }
}

// FUNCTION: TH07 0x0044fc10
ZunResult AnmManager::DrawFacingCamera(AnmVm *vm)
{
    if (vm->visible == 0 || vm->active == 0 ||
        vm->color.bytes.a == 0)
        return ZUN_ERROR;

    f32 centerX = (vm->sprite->widthPx * vm->scale.x) / 2.0f;
    f32 centerY = (vm->sprite->heightPx * vm->scale.y) / 2.0f;
    f32 fVar1;

    if ((vm->anchor & 1) == 0)
    {
        g_PrimitivesToDrawNoVertexBuf[2].pos.x = vm->pos.x - centerX;
        fVar1 = vm->pos.x;
    }
    else
    {
        g_PrimitivesToDrawNoVertexBuf[2].pos.x = vm->pos.x;
        fVar1 = centerX + vm->pos.x;
    }
    g_PrimitivesToDrawNoVertexBuf[3].pos.x = centerX + fVar1;

    if ((vm->anchor & 2) == 0)
    {
        g_PrimitivesToDrawNoVertexBuf[1].pos.y = vm->pos.y - centerY;
        centerX = vm->pos.y;
    }
    else
    {
        g_PrimitivesToDrawNoVertexBuf[1].pos.y = vm->pos.y;
        centerX = centerY + vm->pos.y;
    }
    g_PrimitivesToDrawNoVertexBuf[3].pos.y = centerY + centerX;

    g_PrimitivesToDrawNoVertexBuf[0].pos.z = vm->pos.z;
    g_PrimitivesToDrawNoVertexBuf[0].pos.x =
        g_PrimitivesToDrawNoVertexBuf[2].pos.x;
    g_PrimitivesToDrawNoVertexBuf[0].pos.y =
        g_PrimitivesToDrawNoVertexBuf[1].pos.y;
    g_PrimitivesToDrawNoVertexBuf[1].pos.x =
        g_PrimitivesToDrawNoVertexBuf[3].pos.x;
    g_PrimitivesToDrawNoVertexBuf[1].pos.z =
        g_PrimitivesToDrawNoVertexBuf[0].pos.z;
    g_PrimitivesToDrawNoVertexBuf[2].pos.y =
        g_PrimitivesToDrawNoVertexBuf[3].pos.y;
    g_PrimitivesToDrawNoVertexBuf[2].pos.z =
        g_PrimitivesToDrawNoVertexBuf[0].pos.z;
    g_PrimitivesToDrawNoVertexBuf[3].pos.z =
        g_PrimitivesToDrawNoVertexBuf[0].pos.z;

    return DrawInner(vm, 0);
}

// FUNCTION: TH07 0x0044fe00
ZunResult AnmManager::CalcBillboardTransform(AnmVm *vm)
{
    f32 z = vm->rotation.z;
    f32 cosZ;
    f32 sinZ;

    sincosf_macro(sinZ, cosZ, z);
    D3DXVECTOR3 local_94(0.0f, 0.0f, 0.0f);
    D3DXMATRIX matrix;
    D3DXMatrixIdentity(&matrix);
    matrix.m[3][0] = vm->pos.x;
    matrix.m[3][1] = vm->pos.y;
    matrix.m[3][2] = vm->pos.z;

    D3DXVECTOR3 local_78;
    D3DXVec3Project(&local_78, &local_94, &g_Supervisor.viewport,
                    &g_Supervisor.projectionMatrix, &g_Supervisor.viewMatrix,
                    &matrix);
    if (local_78.z < 0.0f || 1.0f < local_78.z)
    {
        return ZUN_ERROR;
    }

    D3DXVECTOR3 local_6c;
    D3DXVec3Project(&local_6c, &g_Stage.camRight, &g_Supervisor.viewport,
                    &g_Supervisor.projectionMatrix, &g_Supervisor.viewMatrix,
                    &matrix);

    D3DXVECTOR3 local_84(local_6c.x - local_78.x, local_6c.y - local_78.y,
                         local_6c.z - local_78.z);
    f32 local_8 =
        D3DXVec3Length(&local_84) * 0.5f * vm->sprite->widthPx * vm->scale.x;
    f32 local_c =
        D3DXVec3Length(&local_84) * 0.5f * vm->sprite->heightPx * vm->scale.y;

    TranslateRotation(&g_PrimitivesToDrawNoVertexBuf[0], -local_8, -local_c, sinZ,
                      cosZ, local_78.x, local_78.y);
    TranslateRotation(&g_PrimitivesToDrawNoVertexBuf[1], local_8, -local_c, sinZ,
                      cosZ, local_78.x, local_78.y);
    TranslateRotation(&g_PrimitivesToDrawNoVertexBuf[2], -local_8, local_c, sinZ,
                      cosZ, local_78.x, local_78.y);
    TranslateRotation(&g_PrimitivesToDrawNoVertexBuf[3], local_8, local_c, sinZ,
                      cosZ, local_78.x, local_78.y);

    g_PrimitivesToDrawNoVertexBuf[3].pos.z = local_78.z;
    g_PrimitivesToDrawNoVertexBuf[2].pos.z = local_78.z;
    g_PrimitivesToDrawNoVertexBuf[1].pos.z = local_78.z;
    g_PrimitivesToDrawNoVertexBuf[0].pos.z = local_78.z;

    if ((vm->anchor & 1) != 0)
    {
        g_PrimitivesToDrawNoVertexBuf[0].pos.x += local_8;
        g_PrimitivesToDrawNoVertexBuf[1].pos.x += local_8;
        g_PrimitivesToDrawNoVertexBuf[2].pos.x += local_8;
        g_PrimitivesToDrawNoVertexBuf[3].pos.x += local_8;
    }
    if ((vm->anchor & 2) != 0)
    {
        g_PrimitivesToDrawNoVertexBuf[0].pos.y += local_c;
        g_PrimitivesToDrawNoVertexBuf[1].pos.y += local_c;
        g_PrimitivesToDrawNoVertexBuf[2].pos.y += local_c;
        g_PrimitivesToDrawNoVertexBuf[3].pos.y += local_c;
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00450130
ZunResult AnmManager::DrawBillboard(AnmVm *vm)
{
    if (vm->visible == 0)
    {
        return ZUN_ERROR;
    }
    else if (vm->active == 0)
    {
        return ZUN_ERROR;
    }
    else if (vm->color.bytes.a == 0)
    {
        return ZUN_ERROR;
    }
    else
    {
        if (CalcBillboardTransform(vm) == ZUN_SUCCESS)
        {
            return DrawInner(vm, 0);
        }
        else
        {
            return ZUN_ERROR;
        }
    }
}

// FUNCTION: TH07 0x004501a0
void AnmManager::CalcProjectedTransform(AnmVm *vm)
{
    if (vm->skipTransform == 0 &&
        (vm->updateScale != 0 || vm->updateRotation != 0))
    {
        vm->worldTransformMatrix = vm->matrix;
        vm->worldTransformMatrix.m[0][0] *= vm->scale.x;
        vm->worldTransformMatrix.m[1][1] *= vm->scale.y;
        vm->updateScale = 0;
        D3DXMATRIX rot;
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

    D3DXMATRIX local_84 = vm->worldTransformMatrix;
    local_84.m[3][0] =
        (vm->anchor & 1) == 0
            ? vm->pos.x
            : fabsf((vm->sprite->widthPx * vm->scale.x) / 2.0f) + vm->pos.x;
    local_84.m[3][1] =
        (vm->anchor & 2) == 0
            ? vm->pos.y
            : fabsf((vm->sprite->heightPx * vm->scale.y) / 2.0f) + vm->pos.y;
    local_84.m[3][2] = vm->pos.z;

    D3DXVec3Project((D3DXVECTOR3 *)&g_PrimitivesToDrawNoVertexBuf[0].pos,
                    &this->vertexBufferContents[0].position,
                    &g_Supervisor.viewport, &g_Supervisor.projectionMatrix,
                    &g_Supervisor.viewMatrix, &local_84);
    D3DXVec3Project((D3DXVECTOR3 *)&g_PrimitivesToDrawNoVertexBuf[1].pos,
                    &this->vertexBufferContents[1].position,
                    &g_Supervisor.viewport, &g_Supervisor.projectionMatrix,
                    &g_Supervisor.viewMatrix, &local_84);
    D3DXVec3Project((D3DXVECTOR3 *)&g_PrimitivesToDrawNoVertexBuf[2].pos,
                    &this->vertexBufferContents[2].position,
                    &g_Supervisor.viewport, &g_Supervisor.projectionMatrix,
                    &g_Supervisor.viewMatrix, &local_84);
    D3DXVec3Project((D3DXVECTOR3 *)&g_PrimitivesToDrawNoVertexBuf[3].pos,
                    &this->vertexBufferContents[3].position,
                    &g_Supervisor.viewport, &g_Supervisor.projectionMatrix,
                    &g_Supervisor.viewMatrix, &local_84);

    this->matrix = local_84;
}

// FUNCTION: TH07 0x004504b0
ZunResult AnmManager::DrawProjected(AnmVm *vm)
{
    if (vm->visible == 0)
    {
        return ZUN_ERROR;
    }
    else if (vm->active == 0)
    {
        return ZUN_ERROR;
    }
    else if (vm->color.bytes.a == 0)
    {
        return ZUN_ERROR;
    }
    else
    {
        CalcProjectedTransform(vm);
        return DrawInner(vm, 0);
    }
}

// FUNCTION: TH07 0x00450520
ZunResult AnmManager::Draw3(AnmVm *vm)
{
    if (vm->visible == 0 || vm->active == 0 ||
        vm->color.bytes.a == 0)
        return ZUN_ERROR;

    if (this->spritesToDraw != 0)
    {
        this->Flush();
    }

    if (vm->skipTransform == 0 &&
        (vm->updateScale != 0 || vm->updateRotation != 0))
    {
        vm->worldTransformMatrix = vm->matrix;
        vm->worldTransformMatrix.m[0][0] *= vm->scale.x;
        vm->worldTransformMatrix.m[1][1] *= vm->scale.y;
        vm->updateScale = 0;
        D3DXMATRIX rot;

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

    D3DXMATRIX local_c4 = vm->worldTransformMatrix;
    local_c4.m[3][0] =
        (vm->anchor & 1) == 0
            ? vm->pos.x
            : fabsf((vm->sprite->widthPx * vm->scale.x) / 2.0f) + vm->pos.x;
    local_c4.m[3][1] =
        (vm->anchor & 2) == 0
            ? vm->pos.y
            : fabsf((vm->sprite->heightPx * vm->scale.y) / 2.0f) + vm->pos.y;

    local_c4.m[3][0] += this->offset.x;
    local_c4.m[3][1] += this->offset.y;

    SetRenderStateForVm(vm);
    local_c4.m[3][2] = vm->pos.z;

    g_Supervisor.d3dDevice->SetTransform((D3DTRANSFORMSTATETYPE)256, &local_c4);

    if (this->currentSprite != vm->sprite)
    {
        this->currentSprite = vm->sprite;
        D3DXMATRIX local_44 = vm->uvMatrix;
        local_44.m[2][0] = vm->sprite->uvStart.x + vm->uvScrollPos.x;
        local_44.m[2][1] = vm->sprite->uvStart.y + vm->uvScrollPos.y;
        g_Supervisor.d3dDevice->SetTransform(D3DTS_TEXTURE0, &local_44);

        if (this->currentTexture != this->textures[vm->sprite->sourceFileIndex])
        {
            this->currentTexture = this->textures[vm->sprite->sourceFileIndex];
            g_Supervisor.d3dDevice->SetTexture(
                0, (IDirect3DBaseTexture8 *)this->currentTexture);
        }
    }

    if (this->currentVertexShader != 2)
    {
        if (((g_Supervisor.cfg.opts >> 1) & 1) == 0)
        {
            g_Supervisor.d3dDevice->SetVertexShader(0x102);
            g_Supervisor.d3dDevice->SetStreamSource(0, this->vertexBuffer, 20);
        }
        else
        {
            g_Supervisor.d3dDevice->SetVertexShader(0x142);
        }
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, 3);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, 3);
        this->currentVertexShader = 2;
    }

    if (((g_Supervisor.cfg.opts >> 1) & 1) == 0)
    {
        g_Supervisor.d3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    }
    else
    {
        g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2,
                                                g_PrimitivesToDrawUnknown,
                                                sizeof(VertexTex1DiffuseXyz));
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00450a50
f32 AnmVm::GetFloatVarValue(f32 param_1)
{
    switch ((i32)param_1)
    {
    case 10000:
        param_1 = (f32)this->intVars1[0];
        break;
    case 0x2711:
        param_1 = (f32)this->intVars1[1];
        break;
    case 0x2712:
        param_1 = (f32)this->intVars1[2];
        break;
    case 0x2713:
        param_1 = (f32)this->intVars1[3];
        break;
    case 0x2714:
        param_1 = this->floatVars[0];
        break;
    case 0x2715:
        param_1 = this->floatVars[1];
        break;
    case 0x2716:
        param_1 = this->floatVars[2];
        break;
    case 0x2717:
        param_1 = this->floatVars[3];
        break;
    case 0x2718:
        param_1 = (f32)this->intVars2[0];
        break;
    case 0x2719:
        param_1 = (f32)this->intVars2[1];
    }
    return param_1;
}

// FUNCTION: TH07 0x00450b20
i32 AnmVm::GetVarValue(i32 arg)
{
    switch (arg)
    {
    case 10000:
        arg = this->intVars1[0];
        break;
    case 0x2711:
        arg = this->intVars1[1];
        break;
    case 0x2712:
        arg = this->intVars1[2];
        break;
    case 0x2713:
        arg = this->intVars1[3];
        break;
    case 0x2714:
        arg = this->floatVars[0];
        break;
    case 0x2715:
        arg = this->floatVars[1];
        break;
    case 0x2716:
        arg = this->floatVars[2];
        break;
    case 0x2717:
        arg = this->floatVars[3];
        break;
    case 0x2718:
        arg = this->intVars2[0];
        break;
    case 0x2719:
        arg = this->intVars2[1];
    }
    return arg;
}

// FUNCTION: TH07 0x00450c10
f32 *AnmVm::GetFloatVar(f32 *param_1, u16 mask, u8 idx)
{
    if (((u32)mask & 1 << (idx & 0x1f)) != 0)
    {
        u32 uVar1 = *param_1;
        switch (uVar1)
        {
        case 0x2714:
            param_1 = this->floatVars;
            break;
        case 0x2715:
            param_1 = this->floatVars + 1;
            break;
        case 0x2716:
            param_1 = this->floatVars + 2;
            break;
        case 0x2717:
            param_1 = this->floatVars + 3;
        }
    }
    return param_1;
}

// FUNCTION: TH07 0x00450ca0
i32 *AnmVm::GetVar(i32 *paramId, u16 mask, u8 idx)
{
    if (((u32)mask & 1 << (idx & 0x1f)) != 0)
    {
        switch (*paramId)
        {
        case 10000:
            paramId = this->intVars1;
            break;
        case 0x2711:
            paramId = this->intVars1 + 1;
            break;
        case 0x2712:
            paramId = this->intVars1 + 2;
            break;
        case 0x2713:
            paramId = this->intVars1 + 3;
            break;
        default:
            break;
        case 0x2718:
            paramId = this->intVars2;
            break;
        case 0x2719:
            paramId = this->intVars2 + 1;
        }
    }
    return paramId;
}

// FUNCTION: TH07 0x00450d60
i32 AnmManager::ExecuteScript(AnmVm *vm)
{
    if (vm->currentInstruction == NULL)
        return 1;

    while (true)
    {
        if (vm->pendingInterrupt != 0)
        {
        handle_interrupt:
            AnmRawInstr *nextInstr = NULL;
            AnmRawInstr *beginInstr = vm->beginningOfScript;
            while ((beginInstr->opcode != ANM_INTERRUPT_LABEL ||
                    (i32)vm->pendingInterrupt != beginInstr->args[0].i) &&
                   (beginInstr->opcode != ANM_EXIT_HIDE))
            {
                if (beginInstr->opcode == ANM_INTERRUPT_LABEL &&
                    beginInstr->args[0].i == 0xffffffff)
                {
                    nextInstr = beginInstr;
                }
                beginInstr = (AnmRawInstr *)((u8 *)beginInstr + beginInstr->size);
            }
            vm->pendingInterrupt = 0;
            vm->isStopped = 0;
            if (beginInstr->opcode != ANM_INTERRUPT_LABEL)
            {
                if (nextInstr == NULL)
                {
                    vm->currentTimeInScript.Decrement(1);
                    goto execute_timers;
                }
                beginInstr = nextInstr;
            }

            vm->currentInstruction =
                (AnmRawInstr *)((u8 *)beginInstr + beginInstr->size);
            vm->currentTimeInScript.Initialize((i32)vm->currentInstruction->time);
            vm->visible = 1;
        }

        AnmRawInstr *instr = vm->currentInstruction;
        if (vm->currentTimeInScript.current < (i32)instr->time)
        {
            goto execute_timers;
        }

        switch (instr->opcode)
        {
        case ANM_SET_ACTIVE_SPRITE: {
            vm->visible = 1;
            i32 spriteIdx = (instr->flags & 1) == 0
                                ? instr->args[0].i
                                : vm->GetVarValue(instr->args[0].i);
            SetActiveSprite(vm, spriteIdx + this->spriteIndices[vm->anmFileIdx]);
            vm->timeOfLastSpriteSet = vm->currentTimeInScript.current;
            break;
        }
        case ANM_JUMP:
            vm->currentTimeInScript.Initialize(instr->args[1].i);
            vm->currentInstruction =
                (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[0].i);
            continue;
        case ANM_DEC_JUMP: {
            i32 *reg = vm->GetVar(&instr->args[0].i, instr->flags, 0);
            (*reg)--;
            i32 val = (instr->flags & 1) == 0 ? instr->args[0].i
                                              : vm->GetVarValue(instr->args[0].i);
            if (val > 0)
            {
                vm->currentTimeInScript.Initialize(instr->args[2].i);
                vm->currentInstruction =
                    (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[1].i);
                continue;
            }
            break;
        }
        case ANM_SET_TRANSLATION: {
            f32 z = (instr->flags & 4) == 0 ? instr->args[2].f
                                            : vm->GetFloatVarValue(instr->args[2].f);
            f32 y = (instr->flags & 2) == 0 ? instr->args[1].f
                                            : vm->GetFloatVarValue(instr->args[1].f);
            f32 x = (instr->flags & 1) == 0 ? instr->args[0].f
                                            : vm->GetFloatVarValue(instr->args[0].f);
            if (vm->useOffset == 0)
            {
                vm->pos.x = x;
                vm->pos.y = y;
                vm->pos.z = z;
            }
            else
            {
                vm->offset.x = x;
                vm->offset.y = y;
                vm->offset.z = z;
            }
            break;
        }
        case ANM_SET_SCALE:
            vm->scale.x = (instr->flags & 1) == 0
                              ? instr->args[0].f
                              : vm->GetFloatVarValue(instr->args[0].f);
            vm->scale.y = (instr->flags & 2) == 0
                              ? instr->args[1].f
                              : vm->GetFloatVarValue(instr->args[1].f);
            vm->updateScale = 1;
            break;
        case ANM_SET_ALPHA:
            vm->color.bytes.a = instr->args[0].b[0];
            break;
        case ANM_SET_COLOR:
            vm->color.color =
                (vm->color.color & 0xff000000) | (instr->args[0].i & 0xffffff);
            break;
        case ANM_FLIP_X:
            vm->flip ^= 1;
            vm->scale.x = -vm->scale.x;
            vm->updateScale = 1;
            break;
        case ANM_FLIP_Y:
            vm->flip ^= 2;
            vm->scale.y = -vm->scale.y;
            vm->updateScale = 1;
            break;
        case ANM_SET_ROTATION:
            vm->rotation.x = (instr->flags & 1) == 0
                                 ? instr->args[0].f
                                 : vm->GetFloatVarValue(instr->args[0].f);
            vm->rotation.y = (instr->flags & 2) == 0
                                 ? instr->args[1].f
                                 : vm->GetFloatVarValue(instr->args[1].f);
            vm->rotation.z = (instr->flags & 4) == 0
                                 ? instr->args[2].f
                                 : vm->GetFloatVarValue(instr->args[2].f);
            vm->updateRotation = 1;
            break;
        case ANM_SET_ANGLE_VEL:
            vm->angleVel.x = (instr->flags & 1) == 0
                                 ? instr->args[0].f
                                 : vm->GetFloatVarValue(instr->args[0].f);
            vm->angleVel.y = (instr->flags & 2) == 0
                                 ? instr->args[1].f
                                 : vm->GetFloatVarValue(instr->args[1].f);
            vm->angleVel.z = (instr->flags & 4) == 0
                                 ? instr->args[2].f
                                 : vm->GetFloatVarValue(instr->args[2].f);
            vm->updateRotation = 1;
            break;
        case ANM_SET_SCALE_SPEED:
            vm->scaleGrowth.x = (instr->flags & 1) == 0
                                    ? instr->args[0].f
                                    : vm->GetFloatVarValue(instr->args[0].f);
            vm->scaleGrowth.y = (instr->flags & 2) == 0
                                    ? instr->args[1].f
                                    : vm->GetFloatVarValue(instr->args[1].f);
            break;
        case ANM_FADE:
            vm->colorInterpInitialColor.bytes.a = vm->color.bytes.a;
            vm->colorInterpFinalColor.bytes.a = instr->args[0].b[0];
            vm->interpStartTimes[2].Initialize(0);
            vm->interpEndTimes[2].Initialize((instr->flags & 2) == 0
                                                 ? instr->args[1].i
                                                 : vm->GetVarValue(instr->args[1].i));
            vm->interpModes[2] = 0;
            break;
        case ANM_SET_BLEND:
            vm->blendMode = instr->args[0].i & 1;
            break;
        case ANM_POS_TIME_LINEAR:
            vm->interpModes[0] = 0;
            goto interp_pos;
        case ANM_POS_TIME_DECEL:
            vm->interpModes[0] = 4;
            goto interp_pos;
        case ANM_POS_TIME_ACCEL:
            vm->interpModes[0] = 6;
        interp_pos:
            if (vm->useOffset == 0)
            {
                vm->posInterpInitial = vm->pos;
            }
            else
            {
                vm->posInterpInitial = vm->offset;
            }
            vm->posInterpFinal.x = (instr->flags & 1) == 0
                                       ? instr->args[0].f
                                       : vm->GetFloatVarValue(instr->args[0].f);
            vm->posInterpFinal.y = (instr->flags & 2) == 0
                                       ? instr->args[1].f
                                       : vm->GetFloatVarValue(instr->args[1].f);
            vm->posInterpFinal.z = (instr->flags & 4) == 0
                                       ? instr->args[2].f
                                       : vm->GetFloatVarValue(instr->args[2].f);
            vm->interpEndTimes[0].Initialize((instr->flags & 8) == 0
                                                 ? instr->args[3].i
                                                 : vm->GetVarValue(instr->args[3].i));
            vm->interpStartTimes[0].Initialize(0);
            break;
        case ANM_22:
            vm->anchor = 3;
            break;
        case ANM_STOP_HIDE:
            vm->visible = 0;
        case ANM_STOP:
            if (vm->pendingInterrupt == 0)
            {
                vm->isStopped = 1;
                vm->currentTimeInScript.Decrement(1);
                goto execute_timers;
            }
            else
            {
                goto handle_interrupt;
            }
            break;
        case ANM_SET_USE_OFFSET:
            vm->useOffset = instr->args[0].i & 1;
            break;
        case ANM_SET_AUTO_ROTATE:
            vm->autoRotate = instr->args[0].us[0];
            break;
        case ANM_SET_SCROLL_POS_X:
            vm->uvScrollPos.x += (instr->flags & 1) == 0
                                     ? instr->args[0].f
                                     : vm->GetFloatVarValue(instr->args[0].f);
            if (vm->uvScrollPos.x < 1.0f)
            {
                if (vm->uvScrollPos.x < 0.0f)
                    vm->uvScrollPos.x += 1.0f;
            }
            else
                vm->uvScrollPos.x -= 1.0f;
            break;
        case ANM_SET_SCROLL_POS_Y:
            vm->uvScrollPos.y += (instr->flags & 1) == 0
                                     ? instr->args[0].f
                                     : vm->GetFloatVarValue(instr->args[0].f);
            if (vm->uvScrollPos.y < 1.0f)
            {
                if (vm->uvScrollPos.y < 0.0f)
                    vm->uvScrollPos.y += 1.0f;
            }
            else
                vm->uvScrollPos.y -= 1.0f;
            break;
        case ANM_SET_VISIBILITY:
            vm->visible = (instr->args[0].i & 1);
            break;
        case ANM_INTERP_SCALE:
            vm->interpStartTimes[4].Initialize(0);
            vm->interpEndTimes[4].Initialize((instr->flags & 4) == 0
                                                 ? instr->args[2].i
                                                 : vm->GetVarValue(instr->args[2].i));
            vm->interpModes[4] = 0;
            vm->scaleInterpInitial = vm->scale;
            vm->scaleInterpFinal.x = (instr->flags & 1) == 0
                                         ? instr->args[0].f
                                         : vm->GetFloatVarValue(instr->args[0].f);
            vm->scaleInterpFinal.y = (instr->flags & 2) == 0
                                         ? instr->args[1].f
                                         : vm->GetFloatVarValue(instr->args[1].f);
            break;
        case ANM_SET_ZWRITE_DISABLE:
            vm->zWriteDisable = instr->args[0].i & 1;
            break;
        case ANM_SET_CAMERA_MODE:
            vm->cameraMode = instr->args[0].i & 1;
            break;
        case ANM_INTERP_POS:
            vm->interpStartTimes[0].Initialize(0);
            vm->interpEndTimes[0].Initialize((instr->flags & 1) == 0
                                                 ? instr->args[0].i
                                                 : vm->GetVarValue(instr->args[0].i));
            vm->interpModes[0] = instr->args[1].b[0];
            if (vm->useOffset == 0)
                vm->posInterpInitial = vm->pos;
            else
                vm->posInterpInitial = vm->offset;
            vm->posInterpFinal.x = (instr->flags & 4) == 0
                                       ? instr->args[2].f
                                       : vm->GetFloatVarValue(instr->args[2].f);
            vm->posInterpFinal.y = (instr->flags & 8) == 0
                                       ? instr->args[3].f
                                       : vm->GetFloatVarValue(instr->args[3].f);
            vm->posInterpFinal.z = (instr->flags & 16) == 0
                                       ? instr->args[4].f
                                       : vm->GetFloatVarValue(instr->args[4].f);
            break;
        case ANM_INTERP_COLOR:
            vm->interpStartTimes[1].Initialize(0);
            vm->interpEndTimes[1].Initialize((instr->flags & 1) == 0
                                                 ? instr->args[0].i
                                                 : vm->GetVarValue(instr->args[0].i));
            vm->interpModes[1] = instr->args[1].b[0];
            vm->colorInterpInitialColor.bytes.r = vm->color.bytes.r;
            vm->colorInterpInitialColor.bytes.g = vm->color.bytes.g;
            vm->colorInterpInitialColor.bytes.b = vm->color.bytes.b;
            vm->colorInterpFinalColor.bytes.r = instr->args[2].b[0];
            vm->colorInterpFinalColor.bytes.g = instr->args[2].b[1];
            vm->colorInterpFinalColor.bytes.b = instr->args[2].b[2];
            break;
        case ANM_INTERP_ALPHA:
            vm->interpStartTimes[2].Initialize(0);
            vm->interpEndTimes[2].Initialize((instr->flags & 1) == 0
                                                 ? instr->args[0].i
                                                 : vm->GetVarValue(instr->args[0].i));
            vm->interpModes[2] = instr->args[1].b[0];
            vm->colorInterpInitialColor.bytes.a = vm->color.bytes.a;
            vm->colorInterpFinalColor.bytes.a = instr->args[2].b[0];
            break;
        case ANM_INTERP_ROTATE:
            vm->interpStartTimes[3].Initialize(0);
            vm->interpEndTimes[3].Initialize((instr->flags & 1) == 0
                                                 ? instr->args[0].i
                                                 : vm->GetVarValue(instr->args[0].i));
            vm->interpModes[3] = instr->args[1].b[0];
            vm->rotateInterpInitial = vm->rotation;
            vm->rotateInterpFinal.x = (instr->flags & 4) == 0
                                          ? instr->args[2].f
                                          : vm->GetFloatVarValue(instr->args[2].f);
            vm->rotateInterpFinal.y = (instr->flags & 8) == 0
                                          ? instr->args[3].f
                                          : vm->GetFloatVarValue(instr->args[3].f);
            vm->rotateInterpFinal.z = (instr->flags & 16) == 0
                                          ? instr->args[4].f
                                          : vm->GetFloatVarValue(instr->args[4].f);
            vm->updateRotation = 1;
            break;
        case ANM_INTERP_SCALE_2:
            vm->interpStartTimes[4].Initialize(0);
            vm->interpEndTimes[4].Initialize((instr->flags & 1) == 0
                                                 ? instr->args[0].i
                                                 : vm->GetVarValue(instr->args[0].i));
            vm->interpModes[4] = instr->args[1].b[0];
            vm->scaleInterpInitial = vm->scale;
            vm->scaleInterpFinal.x = (instr->flags & 4) == 0
                                         ? instr->args[2].f
                                         : vm->GetFloatVarValue(instr->args[2].f);
            vm->scaleInterpFinal.y = (instr->flags & 8) == 0
                                         ? instr->args[3].f
                                         : vm->GetFloatVarValue(instr->args[3].f);
            vm->updateScale = 1;
            break;
        case ANM_MOV:
            *vm->GetVar(&instr->args[0].i, instr->flags, 0) =
                (instr->flags & 2) == 0 ? instr->args[1].i
                                        : vm->GetVarValue(instr->args[1].i);
            break;
        case ANM_MOV_FLOAT:
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) =
                (instr->flags & 2) == 0 ? instr->args[1].f
                                        : vm->GetFloatVarValue(instr->args[1].f);
            break;
        case ANM_ADD:
            *vm->GetVar(&instr->args[0].i, instr->flags, 0) +=
                (instr->flags & 2) == 0 ? instr->args[1].i
                                        : vm->GetVarValue(instr->args[1].i);
            break;
        case ANM_ADD_FLOAT:
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) +=
                (instr->flags & 2) == 0 ? instr->args[1].f
                                        : vm->GetFloatVarValue(instr->args[1].f);
            break;
        case ANM_SUB:
            *vm->GetVar(&instr->args[0].i, instr->flags, 0) -=
                (instr->flags & 2) == 0 ? instr->args[1].i
                                        : vm->GetVarValue(instr->args[1].i);
            break;
        case ANM_SUB_FLOAT:
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) -=
                (instr->flags & 2) == 0 ? instr->args[1].f
                                        : vm->GetFloatVarValue(instr->args[1].f);
            break;
        case ANM_MUL:
            *vm->GetVar(&instr->args[0].i, instr->flags, 0) *=
                (instr->flags & 2) == 0 ? instr->args[1].i
                                        : vm->GetVarValue(instr->args[1].i);
            break;
        case ANM_MUL_FLOAT:
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) *=
                (instr->flags & 2) == 0 ? instr->args[1].f
                                        : vm->GetFloatVarValue(instr->args[1].f);
            break;
        case ANM_DIV:
            *vm->GetVar(&instr->args[0].i, instr->flags, 0) /=
                (instr->flags & 2) == 0 ? instr->args[1].i
                                        : vm->GetVarValue(instr->args[1].i);
            break;
        case ANM_DIV_FLOAT:
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) /=
                (instr->flags & 2) == 0 ? instr->args[1].f
                                        : vm->GetFloatVarValue(instr->args[1].f);
            break;
        case ANM_MOD:
            *vm->GetVar(&instr->args[0].i, instr->flags, 0) %=
                (instr->flags & 2) == 0 ? instr->args[1].i
                                        : vm->GetVarValue(instr->args[1].i);
            break;
        case ANM_MOD_FLOAT: {
            f32 a = (instr->flags & 1) == 0 ? instr->args[0].f
                                            : vm->GetFloatVarValue(instr->args[0].f);
            f32 b = (instr->flags & 2) == 0 ? instr->args[1].f
                                            : vm->GetFloatVarValue(instr->args[1].f);
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) = fmodf(a, b);
            break;
        }
        case ANM_ADD_2:
            *vm->GetVar(&instr->args[0].i, instr->flags, 0) =
                ((instr->flags & 2) == 0 ? instr->args[1].i
                                         : vm->GetVarValue(instr->args[1].i)) +
                ((instr->flags & 4) == 0 ? instr->args[2].i
                                         : vm->GetVarValue(instr->args[2].i));
            break;
        case ANM_ADD_FLOAT_2:
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) =
                ((instr->flags & 2) == 0 ? instr->args[1].f
                                         : vm->GetFloatVarValue(instr->args[1].f)) +
                ((instr->flags & 4) == 0 ? instr->args[2].f
                                         : vm->GetFloatVarValue(instr->args[2].f));
            break;
        case ANM_SUB_2:
            *vm->GetVar(&instr->args[0].i, instr->flags, 0) =
                ((instr->flags & 2) == 0 ? instr->args[1].i
                                         : vm->GetVarValue(instr->args[1].i)) -
                ((instr->flags & 4) == 0 ? instr->args[2].i
                                         : vm->GetVarValue(instr->args[2].i));
            break;
        case ANM_SUB_FLOAT_2:
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) =
                ((instr->flags & 2) == 0 ? instr->args[1].f
                                         : vm->GetFloatVarValue(instr->args[1].f)) -
                ((instr->flags & 4) == 0 ? instr->args[2].f
                                         : vm->GetFloatVarValue(instr->args[2].f));
            break;
        case ANM_MUL_2:
            *vm->GetVar(&instr->args[0].i, instr->flags, 0) =
                ((instr->flags & 2) == 0 ? instr->args[1].i
                                         : vm->GetVarValue(instr->args[1].i)) *
                ((instr->flags & 4) == 0 ? instr->args[2].i
                                         : vm->GetVarValue(instr->args[2].i));
            break;
        case ANM_MUL_FLOAT_2:
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) =
                ((instr->flags & 2) == 0 ? instr->args[1].f
                                         : vm->GetFloatVarValue(instr->args[1].f)) *
                ((instr->flags & 4) == 0 ? instr->args[2].f
                                         : vm->GetFloatVarValue(instr->args[2].f));
            break;
        case ANM_DIV_2:
            *vm->GetVar(&instr->args[0].i, instr->flags, 0) =
                ((instr->flags & 2) == 0 ? instr->args[1].i
                                         : vm->GetVarValue(instr->args[1].i)) /
                ((instr->flags & 4) == 0 ? instr->args[2].i
                                         : vm->GetVarValue(instr->args[2].i));
            break;
        case ANM_DIV_FLOAT_2:
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) =
                ((instr->flags & 2) == 0 ? instr->args[1].f
                                         : vm->GetFloatVarValue(instr->args[1].f)) /
                ((instr->flags & 4) == 0 ? instr->args[2].f
                                         : vm->GetFloatVarValue(instr->args[2].f));
            break;
        case ANM_MOD_2:
            *vm->GetVar(&instr->args[0].i, instr->flags, 0) =
                ((instr->flags & 2) == 0 ? instr->args[1].i
                                         : vm->GetVarValue(instr->args[1].i)) %
                ((instr->flags & 4) == 0 ? instr->args[2].i
                                         : vm->GetVarValue(instr->args[2].i));
            break;
        case ANM_MOD_FLOAT_2: {
            f32 a = (instr->flags & 2) == 0 ? instr->args[1].f
                                            : vm->GetFloatVarValue(instr->args[1].f);
            f32 b = (instr->flags & 4) == 0 ? instr->args[2].f
                                            : vm->GetFloatVarValue(instr->args[2].f);
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) = fmodf(a, b);
            break;
        }
        case ANM_RAND: {
            u32 maxv = (instr->flags & 2) == 0 ? instr->args[1].i
                                               : vm->GetVarValue(instr->args[1].i);
            *vm->GetVar(&instr->args[0].i, instr->flags, 0) = g_Rng.GetRandomU32InRange(maxv);
            break;
        }
        case ANM_RAND_FLOAT: {
            f32 maxv = (instr->flags & 2) == 0
                           ? instr->args[1].f
                           : vm->GetFloatVarValue(instr->args[1].f);
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) =
                g_Rng.GetRandomFloatInRange(maxv);
            break;
        }
        case ANM_SIN: {
            f32 val = (instr->flags & 2) == 0
                          ? instr->args[1].f
                          : vm->GetFloatVarValue(instr->args[1].f);
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) = sinf(val);
            break;
        }
        case ANM_COS: {
            f32 val = (instr->flags & 2) == 0
                          ? instr->args[1].f
                          : vm->GetFloatVarValue(instr->args[1].f);
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) = cosf(val);
            break;
        }
        case ANM_TAN: {
            f32 val = (instr->flags & 2) == 0
                          ? instr->args[1].f
                          : vm->GetFloatVarValue(instr->args[1].f);
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) = tanf(val);
            break;
        }
        case ANM_ACOS: {
            f32 val = (instr->flags & 2) == 0
                          ? instr->args[1].f
                          : vm->GetFloatVarValue(instr->args[1].f);
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) = acosf(val);
            break;
        }
        case ANM_ATAN: {
            f32 val = (instr->flags & 2) == 0
                          ? instr->args[1].f
                          : vm->GetFloatVarValue(instr->args[1].f);
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) = atanf(val);
            break;
        }
        case ANM_ADD_NORMALIZE_ANGLE: {
            f32 val = (instr->flags & 1) == 0
                          ? instr->args[0].f
                          : vm->GetFloatVarValue(instr->args[0].f);
            *vm->GetFloatVar(&instr->args[0].f, instr->flags, 0) =
                utils::AddNormalizeAngle(val, 0.0f);
            break;
        }
        case ANM_JUMP_IF_EQ: {
            i32 a = (instr->flags & 1) == 0 ? instr->args[0].i
                                            : vm->GetVarValue(instr->args[0].i);
            i32 b = (instr->flags & 2) == 0 ? instr->args[1].i
                                            : vm->GetVarValue(instr->args[1].i);
            if (a == b)
            {
                vm->currentTimeInScript.Initialize(instr->args[3].i);
                vm->currentInstruction =
                    (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[2].i);
                continue;
            }
            break;
        }
        case ANM_JUMP_IF_EQ_FLOAT: {
            f32 a = (instr->flags & 1) == 0 ? instr->args[0].f
                                            : vm->GetFloatVarValue(instr->args[0].f);
            f32 b = (instr->flags & 2) == 0 ? instr->args[1].f
                                            : vm->GetFloatVarValue(instr->args[1].f);
            if (a == b)
            {
                vm->currentTimeInScript.Initialize(instr->args[3].i);
                vm->currentInstruction =
                    (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[2].i);
                continue;
            }
            break;
        }
        case ANM_JUMP_IF_NEQ: {
            i32 a = (instr->flags & 1) == 0 ? instr->args[0].i
                                            : vm->GetVarValue(instr->args[0].i);
            i32 b = (instr->flags & 2) == 0 ? instr->args[1].i
                                            : vm->GetVarValue(instr->args[1].i);
            if (a != b)
            {
                vm->currentTimeInScript.Initialize(instr->args[3].i);
                vm->currentInstruction =
                    (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[2].i);
                continue;
            }
            break;
        }
        case ANM_JUMP_IF_NEQ_FLOAT: {
            f32 a = (instr->flags & 1) == 0 ? instr->args[0].f
                                            : vm->GetFloatVarValue(instr->args[0].f);
            f32 b = (instr->flags & 2) == 0 ? instr->args[1].f
                                            : vm->GetFloatVarValue(instr->args[1].f);
            if (a != b)
            {
                vm->currentTimeInScript.Initialize(instr->args[3].i);
                vm->currentInstruction =
                    (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[2].i);
                continue;
            }
            break;
        }
        case ANM_JUMP_IF_LT: {
            i32 a = (instr->flags & 1) == 0 ? instr->args[0].i
                                            : vm->GetVarValue(instr->args[0].i);
            i32 b = (instr->flags & 2) == 0 ? instr->args[1].i
                                            : vm->GetVarValue(instr->args[1].i);
            if (a < b)
            {
                vm->currentTimeInScript.Initialize(instr->args[3].i);
                vm->currentInstruction =
                    (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[2].i);
                continue;
            }
            break;
        }
        case ANM_JUMP_IF_LT_FLOAT: {
            f32 a = (instr->flags & 1) == 0 ? instr->args[0].f
                                            : vm->GetFloatVarValue(instr->args[0].f);
            f32 b = (instr->flags & 2) == 0 ? instr->args[1].f
                                            : vm->GetFloatVarValue(instr->args[1].f);
            if (a < b)
            {
                vm->currentTimeInScript.Initialize(instr->args[3].i);
                vm->currentInstruction =
                    (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[2].i);
                continue;
            }
            break;
        }
        case ANM_JUMP_IF_LEQ: {
            i32 a = (instr->flags & 1) == 0 ? instr->args[0].i
                                            : vm->GetVarValue(instr->args[0].i);
            i32 b = (instr->flags & 2) == 0 ? instr->args[1].i
                                            : vm->GetVarValue(instr->args[1].i);
            if (a <= b)
            {
                vm->currentTimeInScript.Initialize(instr->args[3].i);
                vm->currentInstruction =
                    (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[2].i);
                continue;
            }
            break;
        }
        case ANM_JUMP_IF_LEQ_FLOAT: {
            f32 a = (instr->flags & 1) == 0 ? instr->args[0].f
                                            : vm->GetFloatVarValue(instr->args[0].f);
            f32 b = (instr->flags & 2) == 0 ? instr->args[1].f
                                            : vm->GetFloatVarValue(instr->args[1].f);
            if (a <= b)
            {
                vm->currentTimeInScript.Initialize(instr->args[3].i);
                vm->currentInstruction =
                    (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[2].i);
                continue;
            }
            break;
        }
        case ANM_JUMP_IF_LT_2: {
            i32 a = (instr->flags & 1) == 0 ? instr->args[0].i
                                            : vm->GetVarValue(instr->args[0].i);
            i32 b = (instr->flags & 2) == 0 ? instr->args[1].i
                                            : vm->GetVarValue(instr->args[1].i);
            if (b < a)
            {
                vm->currentTimeInScript.Initialize(instr->args[3].i);
                vm->currentInstruction =
                    (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[2].i);
                continue;
            }
            break;
        }
        case ANM_JUMP_IF_FLOAT_2: {
            f32 a = (instr->flags & 1) == 0 ? instr->args[0].f
                                            : vm->GetFloatVarValue(instr->args[0].f);
            f32 b = (instr->flags & 2) == 0 ? instr->args[1].f
                                            : vm->GetFloatVarValue(instr->args[1].f);
            if (b < a)
            {
                vm->currentTimeInScript.Initialize(instr->args[3].i);
                vm->currentInstruction =
                    (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[2].i);
                continue;
            }
            break;
        }
        case ANM_JUMP_IF_LEQ_2: {
            i32 a = (instr->flags & 1) == 0 ? instr->args[0].i
                                            : vm->GetVarValue(instr->args[0].i);
            i32 b = (instr->flags & 2) == 0 ? instr->args[1].i
                                            : vm->GetVarValue(instr->args[1].i);
            if (b <= a)
            {
                vm->currentTimeInScript.Initialize(instr->args[3].i);
                vm->currentInstruction =
                    (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[2].i);
                continue;
            }
            break;
        }
        case ANM_JUMP_IF_LEQ_FLOAT_2: {
            f32 a = (instr->flags & 1) == 0 ? instr->args[0].f
                                            : vm->GetFloatVarValue(instr->args[0].f);
            f32 b = (instr->flags & 2) == 0 ? instr->args[1].f
                                            : vm->GetFloatVarValue(instr->args[1].f);
            if (b <= a)
            {
                vm->currentTimeInScript.Initialize(instr->args[3].i);
                vm->currentInstruction =
                    (AnmRawInstr *)((u8 *)vm->beginningOfScript + instr->args[2].i);
                continue;
            }
            break;
        }
        case ANM_WAIT: {
            if (vm->waitTimer.current == 0)
            {
                u32 maxWait = (instr->flags & 1) == 0
                                  ? instr->args[0].i
                                  : vm->GetVarValue(instr->args[0].i);
                vm->waitTimer.Initialize(maxWait);
            }
            else
            {
                vm->waitTimer.Decrement(1);
            }
            if (vm->waitTimer.current > 0)
            {
                vm->currentTimeInScript.Decrement(1);
                goto execute_timers;
            }
            vm->waitTimer.Initialize(0);
            break;
        }
        case ANM_47:
            vm->uvScrollVel.x = (instr->flags & 1) == 0
                                    ? instr->args[0].f
                                    : vm->GetFloatVarValue(instr->args[0].f);
            break;
        case ANM_48:
            vm->uvScrollVel.y = (instr->flags & 1) == 0
                                    ? instr->args[0].f
                                    : vm->GetFloatVarValue(instr->args[0].f);
            break;
        case ANM_EXIT_HIDE:
        case ANM_EXIT_HIDE2:
            vm->visible = 0;
        case ANM_EXIT:
            vm->currentInstruction = NULL;
            return 1;
        }
        vm->currentInstruction = (AnmRawInstr *)((u8 *)instr + instr->size);
    }

execute_timers:
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
    for (i32 i = 0; i < 5; i++)
    {
        if (vm->interpEndTimes[i].current > 0)
        {
            vm->interpStartTimes[i].previous = vm->interpStartTimes[i].current;
            g_Supervisor.TickTimer(&vm->interpStartTimes[i].current,
                                   &vm->interpStartTimes[i].subFrame);
            f32 t;
            if (vm->interpStartTimes[i].current < vm->interpEndTimes[i].current)
            {
                t = ((f32)vm->interpStartTimes[i].current +
                     vm->interpStartTimes[i].subFrame) /
                    ((f32)vm->interpEndTimes[i].current +
                     vm->interpEndTimes[i].subFrame);
            }
            else
            {
                t = 1.0f;
                vm->interpEndTimes[i].Initialize(0);
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
                t = t * t * t * t;
                break;
            case 4:
                t = 1.0f - (1.0f - t) * (1.0f - t);
                break;
            case 5:
                t = 1.0f - t;
                t = 1.0f - t * t * t;
                break;
            case 6: {
                f32 fVar9 = (1.0f - t) * (1.0f - t);
                t = 1.0f - fVar9 * fVar9;
                break;
            }
            }
            switch (i)
            {
            case 0:
                if (vm->useOffset == 0)
                {
                    vm->pos = (vm->posInterpFinal - vm->posInterpInitial) * t +
                              vm->posInterpInitial;
                }
                else
                {
                    vm->offset = (vm->posInterpFinal - vm->posInterpInitial) * t +
                                 vm->posInterpInitial;
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
                vm->scale = (vm->scaleInterpFinal - vm->scaleInterpInitial) * t +
                            vm->scaleInterpInitial;
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
    if (vm->uvScrollPos.x < 1.0f)
    {
        if (vm->uvScrollPos.x < 0.0f)
            vm->uvScrollPos.x += 1.0f;
    }
    else
        vm->uvScrollPos.x -= 1.0f;
    vm->uvScrollPos.y += vm->uvScrollVel.y;
    if (vm->uvScrollPos.y < 1.0f)
    {
        if (vm->uvScrollPos.y < 0.0f)
            vm->uvScrollPos.y += 1.0f;
    }
    else
        vm->uvScrollPos.y -= 1.0f;
    vm->currentTimeInScript.previous = vm->currentTimeInScript.current;
    g_Supervisor.TickTimer(&vm->currentTimeInScript.current,
                           &vm->currentTimeInScript.subFrame);
    this->scriptTicksThisFrame += 1;
    return 0;
}

// FUNCTION: TH07 0x00454260
void AnmManager::DrawTextToSprite(u32 spriteDstIdx, i32 x, i32 y, i32 width,
                                  i32 height, i32 fontWidth, i32 fontHeight,
                                  D3DCOLOR textColor, u32 outlineType,
                                  char *strToPrint, f32 scaleY, f32 scaleX)
{
    if (fontWidth < 1)
        fontWidth = 15;
    if (fontHeight < 1)
        fontHeight = 15;
    IDirect3DTexture8 *outTexture = this->textures[spriteDstIdx];
    TextHelper::RenderTextToTextureBold(x, y, width, height,
                                        ((f32)fontWidth * scaleY),
                                        ((f32)fontHeight * scaleX), textColor,
                                        outlineType, strToPrint, outTexture);
}

// FUNCTION: TH07 0x004542d0
void AnmManager::DrawVmTextFmt(AnmManager *manager, AnmVm *vm,
                               D3DCOLOR textColor, u32 outlineType,
                               const char *param5, ...)
{
    u32 fontWidth = vm->fontWidth;
    char local_54[516];
    va_list args;
    va_start(args, param5);
    vsprintf(local_54, param5, args);
    va_end(args);

    f32 scaleX = vm->sprite->rows;
    f32 scaleY = vm->sprite->cols;
    u32 fontHeight = vm->fontHeight;
    u32 height = vm->sprite->textureHeight;
    u32 width = vm->sprite->textureWidth;
    u32 y = vm->sprite->startPixelInclusive.y;
    u32 x = vm->sprite->startPixelInclusive.x;

    manager->DrawTextToSprite(vm->sprite->sourceFileIndex, x, y, width, height,
                              fontWidth, fontHeight, textColor, outlineType,
                              local_54, scaleY, scaleX);
    vm->visible = 1;
}

// FUNCTION: TH07 0x004543b0
void AnmManager::DrawStringFormat(AnmVm *vm, D3DCOLOR textColor,
                                  u32 outlineType, const char *text, ...)
{
    char buf[64];
    va_list args;

    u32 fontWidth = vm->fontWidth <= 0 ? 15 : (u32)vm->fontWidth;
    va_start(args, text);
    vsprintf(buf, text, args);
    va_end(args);

    this->DrawTextToSprite(vm->sprite->sourceFileIndex, vm->sprite->startPixelInclusive.x,
                           vm->sprite->startPixelInclusive.y, vm->sprite->textureWidth,
                           vm->sprite->textureHeight, fontWidth, vm->fontHeight, textColor,
                           outlineType, (char *)" ", vm->sprite->cols, vm->sprite->rows);

    u32 local_c = (vm->sprite->widthPx * vm->sprite->cols +
                   vm->sprite->startPixelInclusive.x) -
                  ((f32)strlen(buf) * (f32)fontWidth * vm->sprite->cols) / 2.0f;

    this->DrawTextToSprite(vm->sprite->sourceFileIndex, local_c, vm->sprite->startPixelInclusive.y,
                           vm->sprite->textureWidth, vm->sprite->textureHeight, fontWidth,
                           vm->fontHeight, textColor, outlineType, buf, vm->sprite->cols,
                           vm->sprite->rows);

    vm->visible = 1;
}

// FUNCTION: TH07 0x004545b0
void AnmManager::DrawStringFormat2(AnmVm *vm, D3DCOLOR textColor,
                                   u32 outlineType, const char *text, ...)
{
    char buf[64];
    va_list args;

    u32 fontWidth = vm->fontWidth <= 0 ? 15 : (u32)vm->fontWidth;
    va_start(args, text);
    vsprintf(buf, text, args);
    va_end(args);

    this->DrawTextToSprite(vm->sprite->sourceFileIndex, vm->sprite->startPixelInclusive.x,
                           vm->sprite->startPixelInclusive.y, vm->sprite->textureWidth,
                           vm->sprite->textureHeight, fontWidth, vm->fontHeight, textColor,
                           outlineType, (char *)" ", vm->sprite->cols, vm->sprite->rows);

    u32 x = (u32)(((vm->sprite->widthPx * vm->sprite->cols) / 2.0f +
                   vm->sprite->startPixelInclusive.x) -
                  ((f32)strlen(buf) * (f32)fontWidth * vm->sprite->cols) / 4.0f);

    this->DrawTextToSprite(vm->sprite->sourceFileIndex, x, vm->sprite->startPixelInclusive.y,
                           vm->sprite->textureWidth, vm->sprite->textureHeight, fontWidth, vm->fontHeight,
                           textColor, outlineType, buf, vm->sprite->cols, vm->sprite->rows);

    vm->visible = 1;
}

// FUNCTION: TH07 0x004547b0
ZunResult AnmManager::LoadSurface(i32 surfaceIdx, const char *path)
{
    IDirect3DSurface8 *surface;

    if (this->surfaces[surfaceIdx] != NULL)
    {
        ReleaseSurface(surfaceIdx);
    }
    u8 *data = FileSystem::OpenFile(path, 0);
    if (data == NULL)
    {
        // STRING: TH07 0x00495b30
        g_GameErrorContext.Fatal("%sが読み込めないです。\r\n", path);
        return ZUN_ERROR;
    }
    if (g_Supervisor.d3dDevice->CreateImageSurface(
            640, 1024, g_Supervisor.presentParameters.BackBufferFormat,
            &surface) != 0)
        return ZUN_ERROR;

    if (D3DXLoadSurfaceFromFileInMemory(
            surface, NULL, NULL, data, g_LastFileSize, NULL, 1, 0,
            (D3DXIMAGE_INFO *)&this->surfaceSourceInfo[surfaceIdx]) != 0)
        goto err;

    if (g_Supervisor.d3dDevice->CreateRenderTarget(
            this->surfaceSourceInfo[surfaceIdx].width,
            this->surfaceSourceInfo[surfaceIdx].height,
            g_Supervisor.presentParameters.BackBufferFormat, D3DMULTISAMPLE_NONE,
            1, this->surfaces + surfaceIdx) != 0)
        if (g_Supervisor.d3dDevice->CreateImageSurface(
                this->surfaceSourceInfo[surfaceIdx].width,
                this->surfaceSourceInfo[surfaceIdx].height,
                g_Supervisor.presentParameters.BackBufferFormat,
                this->surfaces + surfaceIdx) != 0)
            goto err;

    if (g_Supervisor.d3dDevice->CreateImageSurface(
            this->surfaceSourceInfo[surfaceIdx].width,
            this->surfaceSourceInfo[surfaceIdx].height,
            g_Supervisor.presentParameters.BackBufferFormat,
            this->surfacesBis + surfaceIdx) != 0)
        goto err;

    if (D3DXLoadSurfaceFromSurface(this->surfaces[surfaceIdx], 0, NULL, surface,
                                   0, NULL, 1, 0) != 0)
        goto err;
    if ((D3DXLoadSurfaceFromSurface(this->surfacesBis[surfaceIdx], 0, NULL,
                                    surface, 0, NULL, 1, 0) != 0))
        goto err;

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
    if (this->surfacesBis[surfaceIdx] == NULL)
        return;
    IDirect3DSurface8 *dstSurface;
    if (g_Supervisor.d3dDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO,
                                              &dstSurface) != 0)
        return;

    if (this->surfaces[surfaceIdx] == NULL)
    {
        if (g_Supervisor.d3dDevice->CreateRenderTarget(
                this->surfaceSourceInfo[surfaceIdx].width,
                this->surfaceSourceInfo[surfaceIdx].height,
                g_Supervisor.presentParameters.BackBufferFormat,
                D3DMULTISAMPLE_NONE, 1, this->surfaces + surfaceIdx) != 0 &&
            g_Supervisor.d3dDevice->CreateImageSurface(
                this->surfaceSourceInfo[surfaceIdx].width,
                this->surfaceSourceInfo[surfaceIdx].height,
                g_Supervisor.presentParameters.BackBufferFormat,
                this->surfaces + surfaceIdx) != 0)
        {
            dstSurface->Release();
            return;
        }
        if (D3DXLoadSurfaceFromSurface(this->surfaces[surfaceIdx], NULL, NULL,
                                       this->surfacesBis[surfaceIdx], NULL, NULL,
                                       1, 0) != 0)
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
    if (this->surfacesBis[surfaceIdx] == NULL)
        return;
    IDirect3DSurface8 *local_20;
    if (g_Supervisor.d3dDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO,
                                              &local_20) != 0)
        return;
    if (this->surfaces[surfaceIdx] == NULL)
    {
        if (g_Supervisor.d3dDevice->CreateRenderTarget(
                this->surfaceSourceInfo[surfaceIdx].width,
                this->surfaceSourceInfo[surfaceIdx].height,
                g_Supervisor.presentParameters.BackBufferFormat,
                D3DMULTISAMPLE_NONE, 1, this->surfaces + surfaceIdx) != 0 &&
            g_Supervisor.d3dDevice->CreateImageSurface(
                this->surfaceSourceInfo[surfaceIdx].width,
                this->surfaceSourceInfo[surfaceIdx].height,
                g_Supervisor.presentParameters.BackBufferFormat,
                this->surfaces + surfaceIdx) != 0)
        {
            local_20->Release();
            return;
        }
        if (D3DXLoadSurfaceFromSurface(this->surfaces[surfaceIdx], 0, NULL,
                                       this->surfacesBis[surfaceIdx], 0, NULL, 1,
                                       0) != 0)
        {
            local_20->Release();
            return;
        }
    }
    RECT rect = {rectLeft, rectTop, rectLeft + width, rectTop + height};
    POINT point = {rectX, rectY};
    g_Supervisor.d3dDevice->CopyRects(this->surfaces[surfaceIdx], &rect, 1,
                                      local_20, &point);
    local_20->Release();
}

// FUNCTION: TH07 0x00454e10
void AnmManager::TakeScreenshot(i32 textureId, i32 srcLeft, i32 srcTop,
                                i32 srcWidth, i32 srcHeight, i32 dstLeft,
                                i32 dstTop, i32 dstWidth, i32 dstHeight)
{
    RECT dstRect;
    IDirect3DSurface8 *srcSurface;
    IDirect3DSurface8 *dstSurface;
    RECT srcRect;

    if (this->textures[textureId] != NULL)
    {
        Flush();
        if (g_Supervisor.d3dDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO,
                                                  &srcSurface) == 0)
        {
            if (this->textures[textureId]->GetSurfaceLevel(0, &dstSurface) == 0)
            {
                srcRect.left = srcLeft;
                srcRect.top = srcTop;
                srcRect.right = srcLeft + srcWidth;
                srcRect.bottom = srcTop + srcHeight;
                dstRect.left = dstLeft;
                dstRect.top = dstTop;
                dstRect.right = dstLeft + dstWidth;
                dstRect.bottom = dstTop + dstHeight;
                if (D3DXLoadSurfaceFromSurface(dstSurface, 0, &dstRect, srcSurface, 0,
                                               &srcRect, 0xffffffff, 0) == 0)
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
            else
            {
                srcSurface->Release();
            }
        }
    }
}

#pragma var_order(local_8, local_c)
// FUNCTION: TH07 0x00454f30
void AnmManager::CopyTexture(i32 param1, i32 param2, RECT *param3, RECT *param4)
{
    if (this->textures[param1] == NULL)
        return;
    if (this->textures[param2] == NULL)
        return;

    this->Flush();
    IDirect3DSurface8 *local_8, *local_c;
    if (this->textures[param1]->GetSurfaceLevel(0, &local_8) != 0)
        return;

    if (this->textures[param2]->GetSurfaceLevel(0, &local_c) != 0)
    {
        local_8->Release();
        return;
    }

    if (D3DXLoadSurfaceFromSurface(local_8, 0, param3, local_c, 0, param4,
                                   0xffffffff, 0) != 0)
    {
        local_8->Release();
        local_c->Release();
    }
    else
    {
        local_8->Release();
        local_c->Release();
    }
}

// FUNCTION: TH07 0x00455030
void AnmManager::SetInterruptActiveVms(AnmVm *vm, i32 vmCount, i16 interrupt)
{
    bool bVar1;

    while (vmCount != 0)
    {
        if (vm->sprite == NULL)
        {
            bVar1 = false;
        }
        else if (vm->sprite->sourceFileIndex < 0)
        {
            bVar1 = false;
        }
        else
        {
            bVar1 = g_AnmManager->textures[vm->sprite->sourceFileIndex] != NULL;
        }
        if (bVar1)
        {
            vm->pendingInterrupt = interrupt;
        }
        ++vm;
        --vmCount;
    }
}

// FUNCTION: TH07 0x004550c0
void AnmManager::ExecuteScripts(AnmVm *startVm, i32 count)
{
    while (count != 0)
    {
        if (-1 < startVm->anmFileIdx)
        {
            g_AnmManager->ExecuteScript(startVm);
        }
        startVm += 1;
        --count;
    }
}

// FUNCTION: TH07 0x00455110
void AnmManager::ExecuteVmsAnms(AnmVm *vm, i32 idx, i32 vmCount)
{
    while (vmCount != 0)
    {
        g_AnmManager->ExecuteAnmIdx(vm, idx);
        vm->baseSpriteIdx = vm->activeSpriteIdx;
        --vmCount;
        ++idx;
        ++vm;
    }
}

// FUNCTION: TH07 0x00455170
ZunResult AnmManager::UpdateTrail(AnmVm *vm, VertexTex1DiffuseXyzrwh *vertices,
                                  i32 count)
{
    f32 local_18;
    VertexTex1DiffuseXyzrwh *local_10;
    i32 local_c;

    if (count < 3)
    {
        return ZUN_ERROR;
    }
    else
    {
        local_10 = vertices;
        local_18 = vm->sprite->uvEnd.x + vm->uvScrollPos.x;
        for (local_c = 0; local_c < count; local_c += 2)
        {
            local_10->textureUV.x = local_18;
            local_10->textureUV.y = vm->sprite->uvStart.y + vm->uvScrollPos.y;
            local_10->color = vm->color;
            local_10->w = 1.0f;
            local_10 = local_10 + 2;
            local_18 = local_18 - vm->sprite->uvEnd.x -
                       vm->sprite->uvStart.x / (f32)((count + 1) / 2 - 1);
        }
        local_10 = vertices + 1;
        local_18 = vm->sprite->uvEnd.x + vm->uvScrollPos.x;
        for (local_c = 1; local_c < count; local_c += 2)
        {
            local_10->textureUV.x = local_18;
            local_10->textureUV.y = vm->sprite->uvEnd.y + vm->uvScrollPos.y;
            local_10->color = vm->color;
            local_10->w = 1.0f;
            local_10 += 2;
            local_18 = local_18 - vm->sprite->uvEnd.x -
                       vm->sprite->uvStart.x / (f32)((count + 1) / 2 - 1);
        }
        return ZUN_SUCCESS;
    }
}

// FUNCTION: TH07 0x004552d0
ZunResult AnmManager::DrawTriangleStrip(AnmVm *vm,
                                        VertexTex1DiffuseXyzrwh *vertices,
                                        i32 param3)
{
    if (vm->visible == 0 || vm->active == 0 ||
        vm->color.bytes.a == 0)
        return ZUN_ERROR;

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
    g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, param3 - 2,
                                            vertices,
                                            sizeof(VertexTex1DiffuseXyzrwh));
    return ZUN_SUCCESS;
}
