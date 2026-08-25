#pragma once

#include "AnmVm.hpp"

#include <SDL3/SDL.h>
#include <assert.h>

#include "GameWindow.hpp"
#include "ZunColor.hpp"
#include "ZunMath.hpp"
#include "ZunResult.hpp"
#include "graphics/ZunGraphics.hpp"

#define GAME_WINDOW_WIDTH 640
#define GAME_WINDOW_HEIGHT 480

struct VertexDiffuseXyzrhw
{
    ZunVec3 pos;
    f32 w;
    ZunColor diffuse;
};

struct VertexTex1DiffuseXyz
{
    ZunVec3 pos;
    ZunColor diffuse;
    Float2 textureUV;
};

extern VertexTex1DiffuseXyz g_Quad3DFallback[4];

struct VertexTex1Xyzrhw
{
    ZunVec3 pos;
    f32 w;
    Float2 textureUV;
};

extern VertexTex1Xyzrhw g_QuadTemplate[4];

struct VertexTex1DiffuseXyzrhw
{
    VertexTex1DiffuseXyzrhw()
    {
    }

    ZunVec3 pos;
    f32 w;
    ZunColor diffuse;
    Float2 textureUV;
};

extern VertexTex1DiffuseXyzrhw g_QuadVertices[4];

struct RenderVertexInfo
{
    ZunVec3 pos;
    Float2 textureUV;
};

struct ZunImageInfo
{
    u32 width;
    u32 height;
    u32 depth;
    u32 mipLevels;
    u32 format;
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
static_assert(sizeof(ZunImageInfoEmbedded) == 0x10);

struct AnmRawSprite
{
    i32 id;
    Float2 offset;
    Float2 size;
};
static_assert(sizeof(AnmRawSprite) == 0x14);

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
    i32 dataOffsets[];
};
static_assert(sizeof(AnmRawEntry) == 0x40);

struct AnmEntry
{
    AnmRawEntry *raw;
    i32 spriteIndexOffset;
    i32 childCount;
};

#define MAX_SCRIPTS_SPRITES 2560
#define MAX_TEXTURES 264
#define MAX_ANM_FILES 50
#define MAX_SURFACES 32

struct AnmManager
{
    AnmManager();
    ~AnmManager();

    ZunResult CalcBillboardTransform(AnmVm *vm);
    void CalcProjectedTransform(AnmVm *vm);
    void CopySurfaceToBackBuffer(i32 surfaceIdx, i32 left, i32 top, i32 x, i32 y);
    void CopyTexture(i32 dstIdx, i32 srcIdx, SDL_Rect *dstRect, SDL_Rect *srcRect);
    ZunResult CreateEmptyTexture(i32 textureIdx, u32 width, u32 height);
    ZunResult Draw(AnmVm *vm);
    ZunResult DrawBillboard(AnmVm *vm);
    ZunResult Draw3(AnmVm *vm);
    void DrawEndingRect(i32 surfaceIdx, f32 rectX, f32 rectY, f32 rectLeft, f32 rectTop, i32 width,
                        i32 height);
    ZunResult DrawFacingCamera(AnmVm *vm);
    ZunResult DrawInner(AnmVm *vm, u32 drawFlags);
    ZunResult DrawNoRotation(AnmVm *vm);
    ZunResult DrawProjected(AnmVm *vm);
    void DrawStringFormat(AnmVm *vm, u32 textColor, u32 outlineType, const char *text, ...);
    void DrawStringFormat2(AnmVm *vm, u32 textColor, u32 outlineType, const char *text, ...);
    void DrawTextToSprite(u32 spriteDstIdx, i32 x, i32 y, i32 width, i32 height, i32 fontWidth,
                          i32 fontHeight, u32 textColor, u32 outlineType, char *strToPrint,
                          f32 scaleY, f32 scaleX);
    ZunResult DrawTriangleStrip(AnmVm *vm, VertexTex1DiffuseXyzrhw *vertices, i32 count);
    static void DrawVmTextFmt(AnmManager *manager, AnmVm *vm, u32 textColor, u32 outlineType,
                              const char *str, ...);
    i32 ExecuteScript(AnmVm *vm);
    void Flush();
    i32 LoadAnm(i32 textureIdx, AnmRawEntry *rawEntry, i32 spriteIdxOffset, u32 ownsMemory);
    i32 LoadAnms(i32 anmIdx, const char *path, i32 spriteIdxOffset);
    void LoadSprite(u32 spriteIdx, AnmLoadedSprite *sprite);
    ZunResult LoadSurface(i32 surfaceIdx, const char *path);
    ZunResult LoadTexture(i32 textureIdx, const char *texturePath, u32 colorKey);
    ZunResult LoadTextureAlphaChannel(i32 textureIdx, const char *texturePath);
    ZunResult LoadTextureEmbedded(u32 textureIdx, ZunImageInfoEmbedded *imageInfo);
    ZunResult PushSprite(VertexTex1DiffuseXyzrhw *spriteVertex);
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
    void TakeScreenshot(i32 textureId, i32 srcLeft, i32 srcTop, i32 srcWidth, i32 srcHeight,
                        i32 dstLeft, i32 dstTop, i32 dstWidth, i32 dstHeight);
    void TakeScreenshotIfRequested();
    void TranslateRotation(VertexTex1DiffuseXyzrhw *vertex, f32 width, f32 height, f32 sine,
                           f32 cosine, f32 xOffset, f32 yOffset);

    void SetInterruptActiveVms(AnmVm *vm, i32 vmCount, i16 interrupt);
    void ExecuteScripts(AnmVm *startVm, i32 count);
    void ExecuteVmsAnms(AnmVm *vm, i32 idx, i32 vmCount);
    ZunResult UpdateTrail(AnmVm *vm, VertexTex1DiffuseXyzrhw *vertices, i32 count);

    void ExecuteAnmIdx(AnmVm *vm, i32 anmFileIdx)
    {
        vm->anmFileIdx = anmFileIdx;
        vm->pos = ZunVec3(0.0f, 0.0f, 0.0f);
        vm->offset = ZunVec3(0.0f, 0.0f, 0.0f);
        vm->fontHeight = 15;
        vm->fontWidth = 15;
        SetAndExecuteScript(vm, this->scripts[anmFileIdx]);
    }

    void ReleaseSurfaces()
    {
        for (i32 i = 0; i < MAX_SURFACES; i++)
        {
            if (this->surfaces[i])
            {
                SDL_DestroySurface(this->surfaces[i]);
                this->surfaces[i] = nullptr;
            }
            if (this->surfacesBis[i])
            {
                SDL_DestroySurface(this->surfacesBis[i]);
                this->surfacesBis[i] = nullptr;
            }
        }
    }

    void SetColor(u32 color)
    {
        this->colorMulEnabled = 0;
        this->color.color = color;
    }

    void SetColorWithMulEnabled(u32 color)
    {
        this->colorMulEnabled = 1;
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
        vm->UpdatePrev();
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
            this->screenshotSrcLeft = 32;
            this->screenshotSrcTop = 16;
            this->screenshotSrcWidth = 384;
            this->screenshotSrcHeight = 448;
            this->screenshotDstLeft = x;
            this->screenshotDstTop = y;

            this->screenshotDstWidth = width;
            this->screenshotDstHeight = height;
            return 0;
        }
    }

    i32 ShouldDraw(AnmVm *vm)
    {
        if (!vm->sprite)
        {
            return false;
        }
        else if (vm->sprite->sourceFileIndex < 0)
        {
            return false;
        }
        else
        {
            return this->textures[vm->sprite->sourceFileIndex].id != 0;
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

    void SetTexture(GfxTextureHandle value)
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

    void ClearFrameState()
    {
        this->scriptTicksThisFrame = 0;
        this->renderStateChangesThisFrame = 0;
        this->scriptsExecutedThisFrame = 0;
        this->flushesThisFrame = 0;
    }

    AnmLoadedSprite *GetSprite(i32 spriteIdx)
    {
        return &this->sprites[spriteIdx];
    }

    static void SetCameraModeStatic(AnmManager *mgr, i32 cameraMode)
    {
        mgr->currentCameraMode = cameraMode;
    }

    void DrawAndFlush(AnmVm *vm)
    {
        Draw(vm);
        Flush();
    }

    ZunResult DrawInterpNoRotation(AnmVm *vm)
    {
        ZunVec3 savedPos = vm->pos;
        vm->pos = vm->prevPos.Lerp(vm->pos, g_RenderAlpha);
        ZunResult result = DrawNoRotation(vm);
        vm->pos = savedPos;
        return result;
    }

    ZunResult DrawInterp(AnmVm *vm)
    {
        ZunVec3 savedPos = vm->pos;
        vm->pos = vm->prevPos.Lerp(vm->pos, g_RenderAlpha);
        ZunResult result = Draw(vm);
        vm->pos = savedPos;
        return result;
    }

    void DrawInterpAndFlush(AnmVm *vm)
    {
        DrawInterp(vm);
        Flush();
    }

    ZunColor color;
    i32 colorMulEnabled;
    i32 scriptsExecutedThisFrame;
    i32 scriptTicksThisFrame;
    i32 renderStateChangesThisFrame;
    u32 flushesThisFrame;
    Float2 offset;
    Float2 shakeOffset;
    Float2 prevShakeOffset;
    ZunMatrix matrix;
    AnmLoadedSprite sprites[MAX_SCRIPTS_SPRITES];
    AnmVm vm;
    GfxTextureHandle textures[MAX_TEXTURES];
    void *imageDataArray[256];
    char *textureNames[MAX_TEXTURES];
    i32 loadedSpriteCount;
    AnmRawInstr *scripts[MAX_SCRIPTS_SPRITES];
    i32 spriteIndices[MAX_SCRIPTS_SPRITES];
    AnmEntry anmFiles[MAX_ANM_FILES];
    SDL_Surface *surfaces[MAX_SURFACES];
    SDL_Surface *surfacesBis[MAX_SURFACES];
    GfxTextureHandle surfaceTextures[MAX_SURFACES];
    u32 textureWidths[MAX_TEXTURES];
    u32 textureHeights[MAX_TEXTURES];
    u32 texturePitches[MAX_TEXTURES];
    ZunImageInfo surfaceSourceInfo[MAX_SURFACES];
    ZunColor currentTextureFactor;
    GfxTextureHandle currentTexture;
    u8 currentBlendMode;
    u8 currentColorOp;
    u8 currentVertexShader;
    u8 currentZWriteDisable;
    u8 currentCameraMode;
    // pad 3
    AnmLoadedSprite *currentSprite;
    RenderVertexInfo vertexBufferContents[4];
    u32 spritesToDraw;
    VertexTex1DiffuseXyzrhw spriteVertexBuffer[49152];
    VertexTex1DiffuseXyzrhw *vertexBufferCurPtr;
    VertexTex1DiffuseXyzrhw *vertexBufferStartPtr;
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

extern AnmManager *g_AnmManager;
