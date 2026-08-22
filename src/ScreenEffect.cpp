#include "ScreenEffect.hpp"

#include "AnmManager.hpp"
#include "GameManager.hpp"
#include "GameWindow.hpp"
#include "Rng.hpp"
#include "Supervisor.hpp"
#include "graphics/ZunGraphics.hpp"

void ScreenEffect::Clear(ZunColor color)
{
    g_Supervisor.gfxDevice->SetClearColor(color);
    g_Supervisor.gfxDevice->Clear(CLEAR_COLOR_BUFFER | CLEAR_DEPTH_BUFFER);
    g_Supervisor.gfxDevice->SwapBuffers();
}

void ScreenEffect::SetViewport(u32 color)
{
    if (g_AnmManager)
    {
        g_AnmManager->Flush();
    }
    g_Supervisor.viewport.x = 0;
    g_Supervisor.viewport.y = 0;
    g_Supervisor.viewport.width = GAME_WINDOW_WIDTH;
    g_Supervisor.viewport.height = GAME_WINDOW_HEIGHT;
    g_Supervisor.viewport.minZ = 0.0f;
    g_Supervisor.viewport.maxZ = 1.0f;

    g_Supervisor.gfxDevice->SetViewport(g_Supervisor.viewport);
    Clear(*(ZunColor *)&color);
}

u32 ScreenEffect::OnUpdateFadeOut(ScreenEffect *arg)
{
    if (arg->timer == 0)
    {
        arg->alpha = 255;
    }
    arg->prevAlpha = arg->alpha;
    if (arg->duration != 0)
    {
        arg->alpha = (i32)(255.0f - arg->timer.AsFloat() * 255.0f / (f32)arg->duration);
        if (arg->alpha < 0)
        {
            arg->alpha = (i32)0.0f;
        }
    }
    if (arg->timer >= arg->duration)
    {
        return 0;
    }

    arg->timer++;
    return 1;
}

void ScreenEffect::DrawSquare(ZunRect *rect, u32 color)
{
    g_AnmManager->Flush();

    VertexDiffuseXyzrhw vertices[4];

    vertices[0].pos = ZunVec3(rect->left, rect->top, 0.0f);
    vertices[1].pos = ZunVec3(rect->right, rect->top, 0.0f);
    vertices[2].pos = ZunVec3(rect->left, rect->bottom, 0.0f);
    vertices[3].pos = ZunVec3(rect->right, rect->bottom, 0.0f);
    vertices[0].w = vertices[1].w = vertices[2].w = vertices[3].w = 1.0f;
    vertices[0].diffuse.color = vertices[1].diffuse.color = vertices[2].diffuse.color =
        vertices[3].diffuse.color = color;
    g_Supervisor.gfxDevice->SetTextureArg(TEX_ARG_DIFFUSE);
    if (!g_Supervisor.cfg.disableZBuffer)
    {
        g_Supervisor.gfxDevice->SetDepthMask(false);
    }
    g_Supervisor.gfxDevice->SetBlendMode(BLEND_ALPHA, BLEND_ALPHA);
    g_Supervisor.gfxDevice->SetColorOp(COMPONENT_RGB, COLOR_OP_DISABLE);
    g_Supervisor.gfxDevice->SetColorOp(COMPONENT_ALPHA, COLOR_OP_DISABLE);
    g_Supervisor.gfxDevice->DrawPrimitiveUP(PRIM_TRIANGLE_STRIP, 2, vertices,
                                            sizeof(VertexDiffuseXyzrhw));
    g_AnmManager->SetVertexShader(255);
    g_AnmManager->SetSprite(NULL);
    g_AnmManager->SetTexture(0);
    g_AnmManager->SetColorOp(255);
    g_AnmManager->SetBlendMode(255);
    g_AnmManager->SetZWriteDisable(255);
    g_Supervisor.gfxDevice->SetTextureArg(TEX_ARG_TEXTURE);
}

void ScreenEffect::DrawColoredQuad(ZunRect *rect, u32 param_2, u32 param_3, u32 param_4,
                                   u32 param_5)
{
    g_AnmManager->Flush();

    VertexDiffuseXyzrhw vertices[4];

    vertices[0].pos = ZunVec3(rect->left, rect->top, 0.0f);
    vertices[1].pos = ZunVec3(rect->right, rect->top, 0.0f);
    vertices[2].pos = ZunVec3(rect->left, rect->bottom, 0.0f);
    vertices[3].pos = ZunVec3(rect->right, rect->bottom, 0.0f);
    vertices[0].w = vertices[1].w = vertices[2].w = vertices[3].w = 1.0f;
    vertices[0].diffuse.color = param_2;
    vertices[1].diffuse.color = param_3;
    vertices[2].diffuse.color = param_4;
    vertices[3].diffuse.color = param_5;
    g_Supervisor.gfxDevice->SetTextureArg(TEX_ARG_DIFFUSE);
    if (!g_Supervisor.cfg.disableZBuffer)
    {
        g_Supervisor.gfxDevice->SetDepthMask(false);
    }

    g_Supervisor.gfxDevice->SetBlendMode(BLEND_ALPHA, BLEND_ALPHA);
    g_Supervisor.gfxDevice->SetColorOp(COMPONENT_RGB, COLOR_OP_DISABLE);
    g_Supervisor.gfxDevice->SetColorOp(COMPONENT_ALPHA, COLOR_OP_DISABLE);
    g_Supervisor.gfxDevice->DrawPrimitiveUP(PRIM_TRIANGLE_STRIP, 2, vertices,
                                            sizeof(VertexDiffuseXyzrhw));
    g_AnmManager->SetVertexShader(255);
    g_AnmManager->SetSprite(NULL);
    g_AnmManager->SetTexture(0);
    g_AnmManager->SetColorOp(255);
    g_AnmManager->SetBlendMode(255);
    g_AnmManager->SetZWriteDisable(255);
    g_Supervisor.gfxDevice->SetTextureArg(TEX_ARG_TEXTURE);
}

u32 ScreenEffect::OnDrawFullScreenColor(ScreenEffect *arg)
{
    ZunRect rect;

    rect.left = 0.0f;
    rect.top = 0.0f;
    rect.right = (f32)GAME_WINDOW_WIDTH;
    rect.bottom = (f32)GAME_WINDOW_HEIGHT;
    g_AnmManager->Flush();
    g_Supervisor.viewport.x = 0;
    g_Supervisor.viewport.y = 0;
    g_Supervisor.viewport.width = GAME_WINDOW_WIDTH;
    g_Supervisor.viewport.height = GAME_WINDOW_HEIGHT;
    g_Supervisor.gfxDevice->SetViewport(g_Supervisor.viewport);

    u32 drawAlpha = utils::Lerp(arg->prevAlpha, arg->alpha, g_RenderAlpha);
    ScreenEffect::DrawSquare(&rect, drawAlpha << 24 | arg->args[0]);
    return 1;
}

u32 ScreenEffect::OnUpdateFadeIn(ScreenEffect *arg)
{
    arg->prevAlpha = arg->alpha;
    if (arg->duration != 0)
    {
        arg->alpha = (i32)(arg->timer.AsFloat() * 255.0f / (f32)arg->duration);
        if (arg->alpha < 0)
        {
            arg->alpha = (i32)0.0f;
        }
    }
    if (arg->timer >= arg->duration)
    {
        return 0;
    }

    arg->timer++;
    return 1;
}

u32 ScreenEffect::OnDrawPlayAreaColor(ScreenEffect *arg)
{
    ZunRect rect;

    rect.left = 32.0f;
    rect.top = 16.0f;
    rect.right = 416.0f;
    rect.bottom = 464.0f;

    u32 drawAlpha = utils::Lerp(arg->prevAlpha, arg->alpha, g_RenderAlpha);
    ScreenEffect::DrawSquare(&rect, drawAlpha << 24 | arg->args[0]);
    return 1;
}

u32 ScreenEffect::OnUpdatePulse(ScreenEffect *arg)
{
    if (arg->timer == 0)
    {
        arg->alpha = (arg->args[1] >> 24) & 255;
    }
    arg->prevAlpha = arg->alpha;
    if (arg->timer < arg->duration)
    {
        arg->alpha = ((arg->args[1] >> 24) & 255) -
                     (i32)(((arg->args[1] >> 24) & 255) * arg->timer.AsFloat() / arg->duration);
        if (arg->alpha < 0)
        {
            arg->alpha = 0;
        }
    }
    else
    {
        arg->alpha = 0;
        arg->args[0]--;
        if ((i32)arg->args[0] <= 0)
        {
            return 0;
        }
        arg->timer = 0;
    }
    arg->timer++;
    return 1;
}

u32 ScreenEffect::OnDrawPlayAreaPulseColor(ScreenEffect *arg)
{
    ZunRect rect;

    rect.left = 32.0f;
    rect.top = 16.0f;
    rect.right = 416.0f;
    rect.bottom = 464.0f;
    u32 drawAlpha = utils::Lerp(arg->prevAlpha, arg->alpha, g_RenderAlpha);
    ScreenEffect::DrawSquare(&rect, drawAlpha << 24 | (arg->args[1] & 0xffffff));
    return 1;
}

u32 ScreenEffect::OnUpdateScreenShake(ScreenEffect *arg)
{
    if (g_GameManager.isTimeStopped)
    {
        return 1;
    }

    if (g_GameManager.framesThisStage <= 1)
    {
        g_AnmManager->shakeOffset.x = 0.0f;
        g_AnmManager->shakeOffset.y = 0.0f;
        g_AnmManager->prevShakeOffset = g_AnmManager->shakeOffset;
        return 0;
    }

    arg->timer++;
    if (arg->timer >= arg->duration)
    {
        g_AnmManager->shakeOffset.x = 0.0f;
        g_AnmManager->shakeOffset.y = 0.0f;
        g_AnmManager->prevShakeOffset = g_AnmManager->shakeOffset;
        return 0;
    }

    g_AnmManager->prevShakeOffset = g_AnmManager->shakeOffset;

    f32 fVar1 = (f32)(i32)(arg->args[1] - arg->args[0]) * arg->timer.AsFloat();
    fVar1 /= (f32)arg->duration;
    fVar1 += (f32)(i32)arg->args[0];
    switch (g_Rng.GetRandomU32InRange(3))
    {
    case 0:
        g_AnmManager->shakeOffset.x = 0.0f;
        break;
    case 1:
        g_AnmManager->shakeOffset.x = fVar1;
        break;
    case 2:
        g_AnmManager->shakeOffset.x = -fVar1;
    }
    switch (g_Rng.GetRandomU32InRange(3))
    {
    case 0:
        g_AnmManager->shakeOffset.y = 0.0f;
        break;
    case 1:
        g_AnmManager->shakeOffset.y = fVar1;
        break;
    case 2:
        g_AnmManager->shakeOffset.y = -fVar1;
    }
    return 1;
}

ZunResult ScreenEffect::AddedCallback(ScreenEffect *arg)
{
    arg->timer = 0;
    return ZUN_SUCCESS;
}

ZunResult ScreenEffect::DeletedCallback(ScreenEffect *arg)
{
    if (arg->type == 1)
    {
        g_AnmManager->shakeOffset.x = 0.0f;
        g_AnmManager->shakeOffset.y = 0.0f;
    }
    arg->calcChain->deletedCallback = NULL;
    g_Chain.Cut(arg->drawChain);
    arg->drawChain = NULL;
    delete arg;
    arg = NULL;
    return ZUN_SUCCESS;
}

ScreenEffect *ScreenEffect::RegisterChain(i32 type, i32 duration, u32 arg1,
                                          u32 arg2, u32 arg3)
{
    ChainElem *calcChain = NULL;
    ChainElem *drawChain = NULL;

    ScreenEffect *mgr = new ScreenEffect;
    if (!mgr)
    {
        return NULL;
    }

    memset(mgr, 0, sizeof(ScreenEffect));
    switch (type)
    {
    case SCREEN_EFFECT_FADE_OUT:
        calcChain = g_Chain.CreateElem((ChainCallback)OnUpdateFadeOut);
        drawChain = g_Chain.CreateElem((ChainCallback)OnDrawFullScreenColor);
        break;
    case SCREEN_EFFECT_SHAKE:
        calcChain = g_Chain.CreateElem((ChainCallback)OnUpdateScreenShake);
        break;
    case SCREEN_EFFECT_FADE_IN_PLAY_AREA:
        calcChain = g_Chain.CreateElem((ChainCallback)OnUpdateFadeIn);
        drawChain = g_Chain.CreateElem((ChainCallback)OnDrawPlayAreaColor);
        break;
    case SCREEN_EFFECT_FADE_IN_FULLSCREEN:
        calcChain = g_Chain.CreateElem((ChainCallback)OnUpdateFadeIn);
        drawChain = g_Chain.CreateElem((ChainCallback)OnDrawFullScreenColor);
        break;
    case SCREEN_EFFECT_PULSE:
        calcChain = g_Chain.CreateElem((ChainCallback)OnUpdatePulse);
        drawChain = g_Chain.CreateElem((ChainCallback)OnDrawPlayAreaPulseColor);
    }
    calcChain->addedCallback = (ChainLifecycleCallback)AddedCallback;
    calcChain->deletedCallback = (ChainLifecycleCallback)DeletedCallback;
    calcChain->arg = mgr;
    mgr->type = type;
    mgr->duration = duration;
    mgr->args[0] = arg1;
    mgr->args[1] = arg2;
    mgr->args[2] = arg3;
    if (g_Chain.AddToCalcChain(calcChain, 15))
    {
        return NULL;
    }

    if (drawChain)
    {
        drawChain->arg = mgr;
        g_Chain.AddToDrawChain(drawChain, 17);
    }
    mgr->calcChain = calcChain;
    mgr->drawChain = drawChain;
    return mgr;
}
