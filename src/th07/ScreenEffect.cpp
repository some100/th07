#include "ScreenEffect.hpp"

#include "AnmManager.hpp"
#include "GameManager.hpp"
#include "Global.hpp"
#include "Supervisor.hpp"

// FUNCTION: TH07 0x0044a460
void ScreenEffect::Clear(D3DCOLOR color)
{
    // ZUN bloat: This is doing the exact same thing twice
    g_Supervisor.d3dDevice->Clear(0, NULL, 3, color, 1.0f, 0);
    if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
    {
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
    }
    g_Supervisor.d3dDevice->Clear(0, NULL, 3, color, 1.0f, 0);
    if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
    {
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
    }
}

// FUNCTION: TH07 0x0044a520
void ScreenEffect::SetViewport(D3DCOLOR color)
{
    if (g_AnmManager)
    {
        g_AnmManager->Flush();
    }
    g_Supervisor.viewport.X = 0;
    g_Supervisor.viewport.Y = 0;
    g_Supervisor.viewport.Width = GAME_WINDOW_WIDTH;
    g_Supervisor.viewport.Height = GAME_WINDOW_HEIGHT;
    g_Supervisor.viewport.MinZ = 0.0f;
    g_Supervisor.viewport.MaxZ = 1.0f;
    g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    Clear(color);
}

// FUNCTION: TH07 0x0044a5a0
u32 ScreenEffect::OnUpdateFadeOut(ScreenEffect *arg)
{
    if (arg->duration != 0)
    {
        arg->alpha = (i32)(255.0f - arg->timer.AsFloat() * 255.0f /
                                        (f32)arg->duration);
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

// FUNCTION: TH07 0x0044a650
void ScreenEffect::DrawSquare(ZunRect *rect, D3DCOLOR color)
{
    g_AnmManager->Flush();

    VertexDiffuseXyzrhw vertices[4];

    vertices[0].pos = Float3(rect->left, rect->top, 0.0f);
    vertices[1].pos = Float3(rect->right, rect->top, 0.0f);
    vertices[2].pos = Float3(rect->left, rect->bottom, 0.0f);
    vertices[3].pos = Float3(rect->right, rect->bottom, 0.0f);
    vertices[0].w = vertices[1].w = vertices[2].w = vertices[3].w = 1.0f;
    vertices[0].diffuse.color =
        vertices[1].diffuse.color =
            vertices[2].diffuse.color =
                vertices[3].diffuse.color = color;
    if (!g_Supervisor.cfg.disableTextureBlend)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    if (!g_Supervisor.cfg.disableZBuffer)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    }
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_Supervisor.d3dDevice->SetVertexShader(D3DFVF_DIFFUSE | D3DFVF_XYZRHW);
    g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices,
                                            sizeof(VertexDiffuseXyzrhw));
    g_AnmManager->SetVertexShader(255);
    g_AnmManager->SetSprite(NULL);
    g_AnmManager->SetTexture(NULL);
    g_AnmManager->SetColorOp(255);
    g_AnmManager->SetBlendMode(255);
    g_AnmManager->SetZWriteDisable(255);
    if (!g_Supervisor.cfg.disableTextureBlend)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
}

// FUNCTION: TH07 0x0044aa20
void ScreenEffect::DrawColoredQuad(ZunRect *rect, D3DCOLOR param_2,
                                   D3DCOLOR param_3, D3DCOLOR param_4,
                                   D3DCOLOR param_5)
{
    g_AnmManager->Flush();

    VertexDiffuseXyzrhw vertices[4];

    vertices[0].pos = Float3(rect->left, rect->top, 0.0f);
    vertices[1].pos = Float3(rect->right, rect->top, 0.0f);
    vertices[2].pos = Float3(rect->left, rect->bottom, 0.0f);
    vertices[3].pos = Float3(rect->right, rect->bottom, 0.0f);
    vertices[0].w = vertices[1].w = vertices[2].w = vertices[3].w = 1.0f;
    vertices[0].diffuse.color = param_2;
    vertices[1].diffuse.color = param_3;
    vertices[2].diffuse.color = param_4;
    vertices[3].diffuse.color = param_5;
    if (!g_Supervisor.cfg.disableTextureBlend)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    if (!g_Supervisor.cfg.disableZBuffer)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    }
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_Supervisor.d3dDevice->SetVertexShader(D3DFVF_DIFFUSE | D3DFVF_XYZRHW);
    g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices,
                                            sizeof(VertexDiffuseXyzrhw));
    g_AnmManager->SetVertexShader(255);
    g_AnmManager->SetSprite(NULL);
    g_AnmManager->SetTexture(NULL);
    g_AnmManager->SetColorOp(255);
    g_AnmManager->SetBlendMode(255);
    g_AnmManager->SetZWriteDisable(255);
    if (!g_Supervisor.cfg.disableTextureBlend)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
}

// FUNCTION: TH07 0x0044adf0
u32 ScreenEffect::OnDrawFullScreenColor(ScreenEffect *arg)
{
    ZunRect rect;

    rect.left = 0.0f;
    rect.top = 0.0f;
    rect.right = (f32)GAME_WINDOW_WIDTH;
    rect.bottom = (f32)GAME_WINDOW_HEIGHT;
    g_AnmManager->Flush();
    g_Supervisor.viewport.X = 0;
    g_Supervisor.viewport.Y = 0;
    g_Supervisor.viewport.Width = GAME_WINDOW_WIDTH;
    g_Supervisor.viewport.Height = GAME_WINDOW_HEIGHT;
    g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    ScreenEffect::DrawSquare(&rect, arg->alpha << 24 | arg->args[0]);
    return 1;
}

// FUNCTION: TH07 0x0044ae90
u32 ScreenEffect::OnUpdateFadeIn(ScreenEffect *arg)
{
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

// FUNCTION: TH07 0x0044af30
u32 ScreenEffect::OnDrawPlayAreaColor(ScreenEffect *arg)
{
    ZunRect rect;

    rect.left = 32.0f;
    rect.top = 16.0f;
    rect.right = 416.0f;
    rect.bottom = 464.0f;
    ScreenEffect::DrawSquare(&rect, arg->alpha << 24 | arg->args[0]);
    return 1;
}

// FUNCTION: TH07 0x0044af80
u32 ScreenEffect::OnUpdatePulse(ScreenEffect *arg)
{
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

// FUNCTION: TH07 0x0044b090
u32 ScreenEffect::OnDrawPlayAreaPulseColor(ScreenEffect *arg)
{
    ZunRect rect;

    rect.left = 32.0f;
    rect.top = 16.0f;
    rect.right = 416.0f;
    rect.bottom = 464.0f;
    ScreenEffect::DrawSquare(&rect,
                             arg->alpha << 24 | (arg->args[1] & 0xffffff));
    return 1;
}

// FUNCTION: TH07 0x0044b0e0
u32 ScreenEffect::OnUpdateScreenShake(ScreenEffect *arg)
{
    if (g_GameManager.isTimeStopped)
    {
        return 1;
    }

    if (g_GameManager.framesThisStage <= 1)
    {
        return 0;
    }

    arg->timer++;
    if (arg->timer >= arg->duration)
    {
        return 0;
    }

    f32 fVar1 = (f32)(i32)(arg->args[1] - arg->args[0]) * arg->timer.AsFloat();
    fVar1 /= (f32)arg->duration;
    fVar1 += (f32)(i32)arg->args[0];
    switch (g_Rng.GetRandomU32InRange(3))
    {
    case 0:
        g_AnmManager->offset.x = 0.0f;
        break;
    case 1:
        g_AnmManager->offset.x = fVar1;
        break;
    case 2:
        g_AnmManager->offset.x = -fVar1;
    }
    switch (g_Rng.GetRandomU32InRange(3))
    {
    case 0:
        g_AnmManager->offset.y = 0.0f;
        break;
    case 1:
        g_AnmManager->offset.y = fVar1;
        break;
    case 2:
        g_AnmManager->offset.y = -fVar1;
    }
    return 1;
}

// FUNCTION: TH07 0x0044b280
ZunResult ScreenEffect::AddedCallback(ScreenEffect *arg)
{
    arg->timer = 0;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0044b2c0
ZunResult ScreenEffect::DeletedCallback(ScreenEffect *arg)
{
    arg->calcChain->deletedCallback = NULL;
    g_Chain.Cut(arg->drawChain);
    arg->drawChain = NULL;
    ZUN_DELETE(arg);
    return ZUN_SUCCESS;
}

#pragma var_order(calcChain, drawChain, mgr)
// FUNCTION: TH07 0x0044b310
ScreenEffect *ScreenEffect::RegisterChain(i32 type, i32 duration, u32 arg1,
                                          u32 arg2, u32 arg3)
{
    ChainElem *calcChain = NULL;
    ChainElem *drawChain = NULL;

    ScreenEffect *mgr = ZUN_NEW(ScreenEffect, "ScreenInf");
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
    if (g_Chain.AddToCalcChain(calcChain, CHAIN_PRIO_CALC_SCREENEFFECT))
    {
        return NULL;
    }

    if (drawChain)
    {
        drawChain->arg = mgr;
        g_Chain.AddToDrawChain(drawChain, CHAIN_PRIO_DRAW_SCREENEFFECT);
    }
    mgr->calcChain = calcChain;
    mgr->drawChain = drawChain;
    return mgr;
}
