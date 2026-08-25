#pragma once

#include <windows.h>

#include "AnmIdx.hpp"
#include "ZunColor.hpp"
#include "ZunMath.hpp"
#include "ZunResult.hpp"
#include "ZunTimer.hpp"
#include "dxutil.hpp"
#include "utils.hpp"

#define GAME_WINDOW_WIDTH 640
#define GAME_WINDOW_HEIGHT 480

enum AnmVarId
{
    ANM_VAR_INT1_1 = 10000,
    ANM_VAR_INT1_2 = 10001,
    ANM_VAR_INT1_3 = 10002,
    ANM_VAR_INT1_4 = 10003,
    ANM_VAR_FLOAT_1 = 10004,
    ANM_VAR_FLOAT_2 = 10005,
    ANM_VAR_FLOAT_3 = 10006,
    ANM_VAR_FLOAT_4 = 10007,
    ANM_VAR_INT2_1 = 10008,
    ANM_VAR_INT2_2 = 10009,
};

enum AnmOpcode
{
    ANM_EXIT_HIDE = -1,
    ANM_EXIT_HIDE2 = 1,
    ANM_EXIT = 2,
    ANM_SET_ACTIVE_SPRITE = 3,
    ANM_JUMP = 4,
    ANM_DEC_JUMP = 5,
    ANM_SET_TRANSLATION = 6,
    ANM_SET_SCALE = 7,
    ANM_SET_ALPHA = 8,
    ANM_SET_COLOR = 9,
    ANM_FLIP_X = 10,
    ANM_FLIP_Y = 11,
    ANM_SET_ROTATION = 12,
    ANM_SET_ANGLE_VEL = 13,
    ANM_SET_SCALE_SPEED = 14,
    ANM_FADE = 15,
    ANM_SET_BLEND = 16,
    ANM_POS_TIME_LINEAR = 17,
    ANM_POS_TIME_DECEL = 18,
    ANM_POS_TIME_ACCEL = 19,
    ANM_STOP = 20,
    ANM_INTERRUPT_LABEL = 21,
    ANM_22 = 22,
    ANM_STOP_HIDE = 23,
    ANM_SET_USE_OFFSET = 24,
    ANM_SET_AUTO_ROTATE = 25,
    ANM_SET_SCROLL_POS_X = 26,
    ANM_SET_SCROLL_POS_Y = 27,
    ANM_SET_VISIBILITY = 28,
    ANM_INTERP_SCALE = 29,
    ANM_SET_ZWRITE_DISABLE = 30,
    ANM_SET_CAMERA_MODE = 31,
    ANM_INTERP_POS = 32,
    ANM_INTERP_COLOR = 33,
    ANM_INTERP_ALPHA = 34,
    ANM_INTERP_ROTATE = 35,
    ANM_INTERP_SCALE_2 = 36,
    ANM_MOV = 37,
    ANM_MOV_FLOAT = 38,
    ANM_ADD = 39,
    ANM_ADD_FLOAT = 40,
    ANM_SUB = 41,
    ANM_SUB_FLOAT = 42,
    ANM_MUL = 43,
    ANM_MUL_FLOAT = 44,
    ANM_DIV = 45,
    ANM_DIV_FLOAT = 46,
    ANM_MOD = 47,
    ANM_MOD_FLOAT = 48,
    ANM_ADD_2 = 49,
    ANM_ADD_FLOAT_2 = 50,
    ANM_SUB_2 = 51,
    ANM_SUB_FLOAT_2 = 52,
    ANM_MUL_2 = 53,
    ANM_MUL_FLOAT_2 = 54,
    ANM_DIV_2 = 55,
    ANM_DIV_FLOAT_2 = 56,
    ANM_MOD_2 = 57,
    ANM_MOD_FLOAT_2 = 58,
    ANM_RAND = 59,
    ANM_RAND_FLOAT = 60,
    ANM_SIN = 61,
    ANM_COS = 62,
    ANM_TAN = 63,
    ANM_ACOS = 64,
    ANM_ATAN = 65,
    ANM_NORMALIZE_ANGLE = 66,
    ANM_JUMP_IF_EQ = 67,
    ANM_JUMP_IF_EQ_FLOAT = 68,
    ANM_JUMP_IF_NEQ = 69,
    ANM_JUMP_IF_NEQ_FLOAT = 70,
    ANM_JUMP_IF_LT = 71,
    ANM_JUMP_IF_LT_FLOAT = 72,
    ANM_JUMP_IF_LEQ = 73,
    ANM_JUMP_IF_LEQ_FLOAT = 74,
    ANM_JUMP_IF_GT = 75,
    ANM_JUMP_IF_GT_FLOAT = 76,
    ANM_JUMP_IF_GEQ = 77,
    ANM_JUMP_IF_GEQ_FLOAT = 78,
    ANM_WAIT = 79,
    ANM_SET_SCROLLVEL_X = 80,
    ANM_SET_SCROLLVEL_Y = 81
};

enum AnmEaseMode
{
    ANM_EASE_IN_QUAD = 1,
    ANM_EASE_IN_CUBIC = 2,
    ANM_EASE_IN_QUART = 3,
    ANM_EASE_OUT_QUAD = 4,
    ANM_EASE_OUT_CUBIC = 5,
    ANM_EASE_OUT_QUART = 6,
};

struct VertexDiffuseXyzrhw
{
    Float3 pos;
    f32 w;
    ZunColor diffuse;
};
C_ASSERT(sizeof(VertexDiffuseXyzrhw) == 0x14);

struct VertexTex1DiffuseXyz
{
    Float3 pos;
    ZunColor diffuse;
    Float2 textureUV;
};
C_ASSERT(sizeof(VertexTex1DiffuseXyz) == 0x18);
extern VertexTex1DiffuseXyz g_Quad3DFallback[4];

struct VertexTex1Xyzrhw
{
    Float3 pos;
    f32 w;
    Float2 textureUV;
};
C_ASSERT(sizeof(VertexTex1Xyzrhw) == 0x18);
extern VertexTex1Xyzrhw g_QuadTemplate[4];

struct VertexTex1DiffuseXyzrhw
{
    Float3 pos;
    f32 w;
    ZunColor diffuse;
    Float2 textureUV;
};
C_ASSERT(sizeof(VertexTex1DiffuseXyzrhw) == 0x1c);
extern VertexTex1DiffuseXyzrhw g_QuadVertices[4];

struct RenderVertexInfo
{
    Float3 pos;
    Float2 textureUV;
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

#pragma warning(disable : 4200)
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

#pragma warning(disable : 4200)
struct AnmRawInstr
{
    i16 opcode;
    u16 size;
    i16 time;
    u16 flags;
    AnyArg args[];
};

struct AnmRawScript
{
    u32 id;
    AnmRawInstr *first;
};

struct AnmRawSprite
{
    i32 id;
    Float2 offset;
    Float2 size;
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

struct AnmLoadedSprite
{
    i32 sourceFileIndex;
    Float2 startPixelInclusive;
    Float2 endPixelInclusive;
    f32 textureHeight;
    f32 textureWidth;
    Float2 uvStart;
    Float2 uvEnd;
    f32 heightPx;
    f32 widthPx;
    f32 cols;
    f32 rows;
    i32 spriteId;
};

struct AnmVmBase
{
    void Initialize()
    {
        memset(this, 0, sizeof(AnmVmBase));
        this->scale.x = 1.0f;
        this->scale.y = 1.0f;
        this->color.color = 0xffffffff;
        D3DXMatrixIdentity(&this->matrix);
        *(u16 *)&this->flags = 7;
        this->currentTimeInScript.Initialize();
    }

    void SetInvisible()
    {
        this->visible = 0;
    }

    void SetInterrupt(i16 interrupt)
    {
        this->pendingInterrupt = interrupt;
    }

    void SetRotationZ(f32 z)
    {
        this->rotation.z = z;
    }

    Float3 rotation;
    Float3 angleVel;
    Float2 scale;
    Float2 scaleGrowth;
    Float2 uvScrollPos;
    ZunTimer currentTimeInScript;
    ZunTimer waitTimer;
    ZunTimer interpStartTimes[5]; /* pos = 0, color, alpha, rotate, scale
                                            in that order */
    ZunTimer interpEndTimes[5];
    u8 easeModes[5];
    // pad 3
    i32 intVars1[4];
    f32 floatVars[4];
    i32 intVars2[2];
    Float2 uvScrollVel;
    D3DXMATRIX matrix;
    D3DXMATRIX worldTransformMatrix;
    D3DXMATRIX uvMatrix;
    ZunColor color;
    ZunColor color2;
    union {
        u32 flags;
        struct
        {
            u32 visible : 1;
            u32 active : 1;
            u32 updateRotation : 1;
            u32 updateScale : 1;
            u32 blendMode : 1;
            u32 flag6 : 1;
            u32 flag7 : 1;
            u32 useOffset : 1;
            u32 flip : 2;
            u32 anchor : 2;
            u32 zWriteDisable : 1;
            u32 isStopped : 1;
            u32 cameraMode : 1;
            u32 skipTransform : 1;
            u32 useColor2 : 1;
        };
    };
    i16 autoRotate;
    i16 pendingInterrupt;
};
C_ASSERT(sizeof(AnmVmBase) == 0x1c8);

struct AnmVm : AnmVmBase
{
    AnmVm()
    {
        memset(this, 0, sizeof(AnmVm));
        this->activeSpriteIdx = -1;
    }

    static void AssignVm(AnmVm *out, AnmVm *vm)
    {
        if (out->anmFileIdx != vm->anmFileIdx || vm->currentInstruction)
        {
            *out = *vm;
        }
    }

    i32 *GetVar(i32 *paramId, u16 mask, u32 idx);
    f32 *GetFloatVar(f32 *paramId, u16 mask, u32 idx);
    f32 GetFloatVarValue(f32 arg);
    i32 GetVarValue(i32 arg);

    Float3 pos;
    i16 activeSpriteIdx;
    i16 baseSpriteIdx;
    i16 anmFileIdx;
    // pad 2
    AnmRawInstr *beginningOfScript;
    AnmRawInstr *currentInstruction;
    AnmLoadedSprite *sprite;
    Float3 posInterpInitial;
    Float3 posInterpFinal;
    Float3 rotateInterpInitial;
    Float3 rotateInterpFinal;
    Float2 scaleInterpInitial;
    Float2 scaleInterpFinal;
    ZunColor colorInterpInitialColor;
    ZunColor colorInterpFinalColor;
    Float3 offset;
    i32 timeOfLastSpriteSet;
    u8 fontWidth;
    u8 fontHeight;
    u8 unused_242[10];
};
C_ASSERT(sizeof(AnmVm) == 0x24c);

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
    void CopyTexture(i32 dstIdx, i32 srcIdx, RECT *dstRect, RECT *srcRect);
    ZunResult CreateEmptyTexture(i32 textureIdx, u32 width, u32 height,
                                 i32 textureFormat);
    ZunResult Draw(AnmVm *vm);
    ZunResult DrawBillboard(AnmVm *vm);
    ZunResult Draw3(AnmVm *vm);
    void DrawEndingRect(i32 surfaceIdx, i32 rectX, i32 rectY, i32 rectLeft,
                        i32 rectTop, i32 width, i32 height);
    ZunResult DrawFacingCamera(AnmVm *vm);
    ZunResult DrawInner(AnmVm *vm, u32 drawFlags);
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
    ZunResult DrawTriangleStrip(AnmVm *vm, VertexTex1DiffuseXyzrhw *vertices,
                                i32 count);
    static void DrawVmTextFmt(AnmManager *manager, AnmVm *vm, D3DCOLOR textColor,
                              u32 outlineType, const char *str, ...);
    i32 ExecuteScript(AnmVm *vm);
    void Flush();
    i32 LoadAnm(i32 textureIdx, AnmRawEntry *rawEntry, i32 spriteIdxOffset,
                u32 ownsMemory);
    i32 LoadAnms(i32 anmIdx, const char *path, i32 spriteIdxOffset);
    void LoadSprite(u32 spriteIdx, AnmLoadedSprite *sprite);
    ZunResult LoadSurface(i32 surfaceIdx, const char *path);
    ZunResult LoadTexture(i32 textureIdx, const char *texturePath, i32 formatIdx,
                          D3DCOLOR colorKey);
    ZunResult LoadTextureAlphaChannel(i32 textureIdx, const char *texturePath,
                                      i32 formatIdx, D3DCOLOR colorKey);
    ZunResult LoadTextureEmbedded(u32 textureIdx, ZunImageInfoEmbedded *imageInfo,
                                  D3DCOLOR formatIdx);
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
    void TakeScreenshot(i32 textureId, i32 srcLeft, i32 srcTop, i32 srcWidth,
                        i32 srcHeight, i32 dstLeft, i32 dstTop, i32 dstWidth,
                        i32 dstHeight);
    void TakeScreenshotIfRequested();
    void TranslateRotation(VertexTex1DiffuseXyzrhw *vertex, f32 width,
                           f32 height, f32 sine, f32 cosine,
                           f32 xOffset, f32 yOffset);

    void SetInterruptActiveVms(AnmVm *vm, i32 vmCount,
                               i16 interrupt);
    void ExecuteScripts(AnmVm *startVm, i32 count);
    void ExecuteVmsAnms(AnmVm *vm, i32 idx, i32 vmCount);
    ZunResult UpdateTrail(AnmVm *vm, VertexTex1DiffuseXyzrhw *vertices, i32 count);

    // FUNCTION: TH07 0x00404f30
    void ExecuteAnmIdx(AnmVm *vm, i32 anmFileIdx)
    {
        vm->anmFileIdx = anmFileIdx;
        vm->pos = Float3(0.0f, 0.0f, 0.0f);
        vm->offset = Float3(0.0f, 0.0f, 0.0f);
        vm->fontHeight = 15;
        vm->fontWidth = 15;
        SetAndExecuteScript(vm, this->scripts[anmFileIdx]);
    }

    // FUNCTION: TH07 0x00433f20
    void ReleaseSurfaces()
    {
        for (i32 i = 0; i < ARRAY_SIZE_SIGNED(this->surfaces); i++)
        {
            SAFE_RELEASE(this->surfaces[i]);
        }
    }

    void SetColor(D3DCOLOR color)
    {
        this->colorMulEnabled = 0;
        this->color.color = color;
    }

    void SetColorWithMulEnabled(D3DCOLOR color)
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
            return this->textures[vm->sprite->sourceFileIndex] != NULL;
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

    ZunColor color;
    i32 colorMulEnabled;
    i32 scriptsExecutedThisFrame;
    i32 scriptTicksThisFrame;
    i32 renderStateChangesThisFrame;
    u32 flushesThisFrame;
    Float2 offset;
    D3DXMATRIX matrix;
    AnmLoadedSprite sprites[MAX_SCRIPTS_SPRITES];
    AnmVm vm;
    IDirect3DTexture8 *textures[MAX_TEXTURES];
    void *imageDataArray[256];
    char *textureNames[MAX_TEXTURES];
    i32 loadedSpriteCount;
    AnmRawInstr *scripts[MAX_SCRIPTS_SPRITES];
    i32 spriteIndices[MAX_SCRIPTS_SPRITES];
    AnmEntry anmFiles[MAX_ANM_FILES];
    IDirect3DSurface8 *surfaces[MAX_SURFACES];
    IDirect3DSurface8 *surfacesBis[MAX_SURFACES];
    ZunImageInfo surfaceSourceInfo[MAX_SURFACES];
    ZunColor currentTextureFactor;
    IDirect3DTexture8 *currentTexture;
    u8 currentBlendMode;
    u8 currentColorOp;
    u8 currentVertexShader;
    u8 currentZWriteDisable;
    u8 currentCameraMode;
    // pad 3
    AnmLoadedSprite *currentSprite;
    IDirect3DVertexBuffer8 *vertexBuffer;
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
C_ASSERT(sizeof(AnmManager) == 0x17e560);
extern AnmManager *g_AnmManager;
