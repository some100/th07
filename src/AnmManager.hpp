#pragma once

#include "AnmVm.hpp"

#include <assert.h>
#include <windef.h>

#include "ZunColor.hpp"
#include "ZunMath.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"

struct VertexDiffuseXyzrhw
{
    D3DXVECTOR3 pos;
    f32 w;
    ZunColor diffuse;
};
C_ASSERT(sizeof(VertexDiffuseXyzrhw) == 0x14);

struct VertexTex1DiffuseXyz
{
    D3DXVECTOR3 position;
    ZunColor diffuse;
    D3DXVECTOR2 textureUV;
};
C_ASSERT(sizeof(VertexTex1DiffuseXyz) == 0x18);

struct VertexTex1Xyzrwh
{
    D3DXVECTOR3 pos;
    f32 w;
    D3DXVECTOR2 textureUV;
};

struct VertexTex1DiffuseXyzrwh
{
    /* this is a zunvec3 because for some reason msvc 2002 doesnt like it
       when its a d3dxvector3 in the inline assembly */
    ZunVec3 pos;
    f32 w;
    ZunColor color;
    D3DXVECTOR2 textureUV;
};
C_ASSERT(sizeof(VertexTex1DiffuseXyzrwh) == 0x1c);

struct RenderVertexInfo
{
    D3DXVECTOR3 position;
    D3DXVECTOR2 textureUV;
};
C_ASSERT(sizeof(RenderVertexInfo) == 0x14);

struct ZunImageInfo
{
    u32 width;
    u32 height;
    u32 depth;
    u32 mipLevels;
    D3DFORMAT format;
};

struct ZunImageInfoEmbedded
{
    i16 magic;
    i16 colorDepth;
    i16 imageType;
    i16 format;
    i16 width;
    i16 height;
    i32 unused_c;
    u8 data[];
};

struct AnmRawScript
{
    u32 id;
    AnmRawInstr *first;
};

struct AnmRawSprite
{
    i32 id;
    D3DXVECTOR2 offset;
    D3DXVECTOR2 size;
};

struct AnmRawEntry
{
    i32 numSprites;
    i32 numScripts;
    i32 textureIdx;
    i32 width;
    i32 height;
    i32 format;
    i32 color_key;
    i32 nameOffset;
    i32 spriteIdxOffset;
    i32 mipmapNameOffset;
    i32 version;
    i32 priority;
    i32 textureOffset;
    u8 hasData;
    u8 ownsMemory;
    i16 unused_36;
    i32 nextOffset;
    i32 unused_3c;
    i32 spriteOffsets[10];
    AnmRawScript scripts[10];
};

struct AnmEntry
{
    AnmRawEntry *raw;
    i32 spriteIndexOffset;
    i32 childCount;
};

struct AnmManager
{
    AnmManager();
    ~AnmManager();

    ZunResult CalcBillboardTransform(AnmVm *vm);
    void CalcProjectedTransform(AnmVm *vm);
    void CopySurfaceToBackBuffer(i32 surfaceIdx, i32 left, i32 top, i32 x, i32 y);
    void CopyTexture(i32 param1, i32 param2, RECT *param3, RECT *param4);
    ZunResult CreateEmptyTexture(i32 textureIdx, u32 width, u32 height,
                                 i32 textureFormat);
    ZunResult Draw(AnmVm *vm);
    ZunResult DrawBillboard(AnmVm *vm);
    ZunResult Draw3(AnmVm *vm);
    void DrawEndingRect(i32 surfaceIdx, i32 rectX, i32 rectY, i32 rectLeft,
                        i32 rectTop, i32 width, i32 height);
    ZunResult DrawFacingCamera(AnmVm *vm);
    ZunResult DrawInner(AnmVm *vm, u32 param2);
    ZunResult DrawNoRotation(AnmVm *vm);
    ZunResult DrawProjected(AnmVm *vm);
    void DrawStringFormat(AnmVm *vm, D3DCOLOR textColor, u32 outlineType,
                          const char *text, ...);
    void DrawStringFormat2(AnmVm *vm, D3DCOLOR textColor, u32 outlineType,
                           const char *text, ...);
    void DrawTextToSprite(u32 spriteDstIdx, i32 x, i32 y, i32 width, i32 height,
                          i32 fontWidth, i32 fontHeight, D3DCOLOR textColor,
                          u32 outlineType, char *strToPrint, f32 scaleY,
                          f32 scaleX);
    ZunResult DrawTriangleStrip(AnmVm *vm, VertexTex1DiffuseXyzrwh *vertices,
                                i32 param3);
    static void DrawVmTextFmt(AnmManager *manager, AnmVm *vm, D3DCOLOR textColor,
                              u32 outlineType, const char *param5, ...);
    i32 ExecuteScript(AnmVm *vm);
    void Flush();
    i32 LoadAnm(i32 textureIdx, AnmRawEntry *rawEntry, i32 spriteIdxOffset,
                u32 param_4);
    i32 LoadAnms(i32 anmIdx, const char *path, i32 spriteIdxOffset);
    void LoadSprite(u32 spriteIdx, AnmLoadedSprite *sprite);
    ZunResult LoadSurface(i32 surfaceIdx, const char *path);
    ZunResult LoadTexture(i32 textureIdx, const char *texturePath, i32 formatIdx,
                          D3DCOLOR colorKey);
    ZunResult LoadTextureAlphaChannel(i32 textureIdx, const char *texturePath,
                                      i32 formatIdx, D3DCOLOR colorKey);
    ZunResult LoadTextureEmbedded(u32 textureIdx, ZunImageInfoEmbedded *imageInfo,
                                  D3DCOLOR formatIdx);
    ZunResult PushSprite(VertexTex1DiffuseXyzrwh *spriteVertex);
    void ReleaseAnm(i32 anmIdx);
    void ReleaseSurface(i32 surfaceIdx);
    void ReleaseTexture(i32 textureIdx);
    void ReleaseVertexBuffer();
    void ResetVertexBuffer();
    ZunResult SetActiveSprite(AnmVm *vm, i32 spriteIdx);
    void SetAndExecuteScript(AnmVm *vm, AnmRawInstr *beginningOfScript);
    void SetRenderStateForVm(AnmVm *vm);
    void SetupVertexBuffer();
    void SyncRenderState(AnmVm *vm);
    void TakeScreenshot(i32 textureId, i32 srcLeft, i32 srcTop, i32 srcWidth,
                        i32 srcHeight, i32 dstLeft, i32 dstTop, i32 dstWidth,
                        i32 dstHeight);
    void TakeScreenshotIfRequested();
    void TranslateRotation(VertexTex1DiffuseXyzrwh *param_1, f32 width,
                           f32 height, f32 param_4, f32 param_5,
                           f32 xOffset, f32 yOffset);

    void SetInterruptActiveVms(AnmVm *vm, i32 vmCount,
                               i16 interrupt);
    void ExecuteScripts(AnmVm *startVm, i32 count);
    void ExecuteVmsAnms(AnmVm *vm, i32 idx, i32 vmCount);
    ZunResult UpdateTrail(AnmVm *vm, VertexTex1DiffuseXyzrwh *vertices, i32 count);

    // FUNCTION: TH07 0x00404f30
    void ExecuteAnmIdx(AnmVm *vm, i32 anmFileIdx)
    {
        vm->anmFileIdx = anmFileIdx;
        vm->pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        vm->offset = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        vm->fontHeight = 15;
        vm->fontWidth = 15;
        SetAndExecuteScript(vm, this->scripts[anmFileIdx]);
    }

    // FUNCTION: TH07 0x00433f20
    void ReleaseSurfaces()
    {
        for (i32 i = 0; i < 0x20; i++)
        {
            SAFE_RELEASE(this->surfaces[i]);
        }
    }

    void SetColor(D3DCOLOR color)
    {
        this->colorMulEnabled = 0;
        this->color.color = color;
    }

    void SetAnmIdxAndExecuteScript(AnmVm *vm, i32 anmIdx)
    {
        vm->anmFileIdx = anmIdx;
        this->SetAndExecuteScript(vm, this->scripts[anmIdx]);
    }

    void InitializeAndSetActiveSprite(AnmVm *vm, i32 spriteIdx)
    {
        vm->Initialize();
        this->SetActiveSprite(vm, spriteIdx);
    }

    i32 CreateScreenshotTexture(i32 x, i32 y, i32 width, i32 height)
    {
        if (this->screenshotTextureId >= 0)
        {
            return -1;
        }
        else
        {
            this->screenshotTextureId = 4;
            this->screenshotSrcLeft = 0x20;
            this->screenshotSrcTop = 0x10;
            this->screenshotSrcWidth = 0x180;
            this->screenshotSrcHeight = 0x1c0;
            this->screenshotDstLeft = x;
            this->screenshotDstTop = y;

            this->screenshotDstWidth = width;
            this->screenshotDstHeight = height;
            return 0;
        }
    }

    void SetVertexShader(u8 value)
    {
        this->currentVertexShader = value;
    }

    void SetSprite(AnmLoadedSprite *value)
    {
        this->currentSprite = value;
    }

    void SetTexture(IDirect3DTexture8 *value)
    {
        this->currentTexture = value;
    }

    void SetColorOp(u8 value)
    {
        this->currentColorOp = value;
    }

    void SetBlendMode(u8 value)
    {
        this->currentBlendMode = value;
    }

    void SetZWriteDisable(u8 value)
    {
        this->currentZWriteDisable = value;
    }

    void SetCameraMode(u8 value)
    {
        this->currentCameraMode = value;
    }

    void SetScriptTicks(i32 value)
    {
        this->scriptTicksThisFrame = value;
    }

    void SetRenderStateChanges(i32 value)
    {
        this->renderStateChangesThisFrame = value;
    }

    void SetScriptsExecuted(i32 value)
    {
        this->scriptsExecutedThisFrame = value;
    }
    
    void SetFlushes(i32 value)
    {
        this->flushesThisFrame = value;
    }

    ZunColor color;
    i32 colorMulEnabled;
    i32 scriptsExecutedThisFrame;
    i32 scriptTicksThisFrame;
    i32 renderStateChangesThisFrame;
    u32 flushesThisFrame;
    D3DXVECTOR2 offset;
    D3DXMATRIX matrix;
    struct AnmLoadedSprite sprites[2560];
    struct AnmVm vm;
    struct IDirect3DTexture8 *textures[264];
    void *imageDataArray[256];
    char *textureNames[264];
    i32 loadedSpriteCount;
    struct AnmRawInstr *scripts[2560];
    i32 spriteIndices[2560];
    struct AnmEntry anmFiles[50];
    struct IDirect3DSurface8 *surfaces[32];
    struct IDirect3DSurface8 *surfacesBis[32];
    struct ZunImageInfo surfaceSourceInfo[32];
    ZunColor currentTextureFactor;
    struct IDirect3DTexture8 *currentTexture;
    u8 currentBlendMode;
    u8 currentColorOp;
    u8 currentVertexShader;
    u8 currentZWriteDisable;
    u8 currentCameraMode;
    // pad 3
    struct AnmLoadedSprite *currentSprite;
    struct IDirect3DVertexBuffer8 *vertexBuffer;
    struct RenderVertexInfo vertexBufferContents[4];
    u32 spritesToDraw;
    struct VertexTex1DiffuseXyzrwh spriteVertexBuffer[49152];
    struct VertexTex1DiffuseXyzrwh *vertexBufferCurPtr;
    struct VertexTex1DiffuseXyzrwh *vertexBufferStartPtr;
    i32 screenshotTextureId;
    i32 screenshotSrcLeft;
    i32 screenshotSrcTop;
    i32 screenshotSrcWidth;
    i32 screenshotSrcHeight;
    i32 screenshotDstLeft;
    i32 screenshotDstTop;
    i32 screenshotDstWidth;
    i32 screenshotDstHeight;
};
C_ASSERT(sizeof(AnmManager) == 0x17e560);

extern AnmManager *g_AnmManager;
extern VertexTex1DiffuseXyzrwh g_PrimitivesToDrawNoVertexBuf[4];
extern VertexTex1Xyzrwh g_PrimitivesToDrawVertexBuf[4];
extern VertexTex1DiffuseXyz g_PrimitivesToDrawUnknown[4];
