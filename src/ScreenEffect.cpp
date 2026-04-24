#include "ScreenEffect.hpp"

#include "AnmManager.hpp"
#include "GameManager.hpp"
#include "Rng.hpp"
#include "Supervisor.hpp"

// FUNCTION: TH07 0x0044a460
void ScreenEffect::Clear(D3DCOLOR color)
{
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
    if (g_AnmManager != NULL)
    {
        g_AnmManager->Flush();
    }
    g_Supervisor.viewport.X = 0;
    g_Supervisor.viewport.Y = 0;
    g_Supervisor.viewport.Width = 640;
    g_Supervisor.viewport.Height = 480;
    g_Supervisor.viewport.MinZ = 0.0f;
    g_Supervisor.viewport.MaxZ = 1.0f;
    g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    Clear(color);
}

// FUNCTION: TH07 0x0044a5a0
u32 BombEffects::OnUpdateFadeOut(BombEffects *arg)
{
    if (arg->duration != 0)
    {
        arg->alpha =
            (f32)(u32)(255.0f -
                       (((f32)arg->timer.current + arg->timer.subFrame) * 255.0f) /
                           (f32)arg->duration);
        if (arg->alpha < 0)
        {
            arg->alpha = 0.0f;
        }
    }
    bool bVar1 = arg->timer < arg->duration;
    if (bVar1)
    {
        arg->timer++;
    }
    return bVar1;
}

// FUNCTION: TH07 0x0044a650
void ScreenEffect::DrawSquare(ZunRect *rect, D3DCOLOR color)
{
    g_AnmManager->Flush();

    VertexDiffuseXyzrhw vertices[4];

    vertices[0].pos = D3DXVECTOR3(rect->left, rect->top, 0.0f);
    vertices[1].pos = D3DXVECTOR3(rect->right, rect->top, 0.0f);
    vertices[2].pos = D3DXVECTOR3(rect->left, rect->bottom, 0.0f);
    vertices[3].pos = D3DXVECTOR3(rect->right, rect->bottom, 0.0f);
    vertices[0].w = vertices[1].w = vertices[2].w = vertices[3].w = 1.0f;
    vertices[0].diffuse.color =
        vertices[1].diffuse.color =
            vertices[2].diffuse.color =
                vertices[3].diffuse.color = color;
    if ((g_Supervisor.cfg.opts >> 8 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 2);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 2);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 0);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 0);
    if ((g_Supervisor.cfg.opts >> 6 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    }
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_DESTBLEND, 6);
    g_Supervisor.d3dDevice->SetVertexShader(0x44);
    g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices,
                                            sizeof(VertexDiffuseXyzrhw));
    g_AnmManager->SetVertexShader(0xff);
    g_AnmManager->SetSprite(NULL);
    g_AnmManager->SetTexture(NULL);
    g_AnmManager->SetColorOp(0xff);
    g_AnmManager->SetBlendMode(0xff);
    g_AnmManager->SetZWriteDisable(0xff);
    if ((g_Supervisor.cfg.opts >> 8 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 4);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 4);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 2);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 2);
}

// FUNCTION: TH07 0x0044aa20
void ScreenEffect::DrawColoredQuad(ZunRect *rect, D3DCOLOR param_2,
                                   D3DCOLOR param_3, D3DCOLOR param_4,
                                   D3DCOLOR param_5)
{
    g_AnmManager->Flush();

    VertexDiffuseXyzrhw vertices[4];

    vertices[0].pos = D3DXVECTOR3(rect->left, rect->top, 0.0f);
    vertices[1].pos = D3DXVECTOR3(rect->right, rect->top, 0.0f);
    vertices[2].pos = D3DXVECTOR3(rect->left, rect->bottom, 0.0f);
    vertices[3].pos = D3DXVECTOR3(rect->right, rect->bottom, 0.0f);
    vertices[0].w = vertices[1].w = vertices[2].w = vertices[3].w = 1.0f;
    vertices[0].diffuse.color = param_2;
    vertices[1].diffuse.color = param_3;
    vertices[2].diffuse.color = param_4;
    vertices[3].diffuse.color = param_5;
    if ((g_Supervisor.cfg.opts >> 8 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 2);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 2);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 0);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 0);
    if ((g_Supervisor.cfg.opts >> 6 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    }
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_DESTBLEND, 6);
    g_Supervisor.d3dDevice->SetVertexShader(0x44);
    g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices,
                                            sizeof(VertexDiffuseXyzrhw));
    g_AnmManager->SetVertexShader(0xff);
    g_AnmManager->SetSprite(NULL);
    g_AnmManager->SetTexture(NULL);
    g_AnmManager->SetColorOp(0xff);
    g_AnmManager->SetBlendMode(0xff);
    g_AnmManager->SetZWriteDisable(0xff);
    if ((g_Supervisor.cfg.opts >> 8 & 1) == 0)
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 4);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 4);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 2);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 2);
}

// FUNCTION: TH07 0x0044adf0
u32 BombEffects::OnDrawFullScreenColor(BombEffects *arg)
{
    ZunRect rect;

    rect.left = 0.0f;
    rect.top = 0.0f;
    rect.right = 640.0f;
    rect.bottom = 480.0f;
    g_AnmManager->Flush();
    g_Supervisor.viewport.X = 0;
    g_Supervisor.viewport.Y = 0;
    g_Supervisor.viewport.Width = 0x280;
    g_Supervisor.viewport.Height = 0x1e0;
    g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    ScreenEffect::DrawSquare(&rect, arg->alpha << 0x18 | arg->args[0]);
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0044ae90
u32 BombEffects::OnUpdateFadeIn(BombEffects *arg)
{
    if (arg->duration != 0)
    {
        arg->alpha =
            (u32)((((f32)arg->timer.current + arg->timer.subFrame) * 255.0f) /
                  (f32)arg->duration);
        if (arg->alpha < 0)
        {
            arg->alpha = 0.0f;
        }
    }
    bool bVar1 = arg->timer < arg->duration;
    if (bVar1)
    {
        arg->timer++;
    }
    return bVar1;
}

// FUNCTION: TH07 0x0044af30
u32 BombEffects::OnDrawPlayAreaColor(BombEffects *arg)
{
    ZunRect rect;

    rect.left = 32.0f;
    rect.top = 16.0f;
    rect.right = 416.0f;
    rect.bottom = 464.0f;
    ScreenEffect::DrawSquare(&rect, arg->alpha << 0x18 | arg->args[0]);
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0044af80
u32 BombEffects::OnUpdatePulse(BombEffects *arg)
{
    if (arg->timer < arg->duration)
    {
        arg->alpha =
            (f32)((u32)((arg->args[1] >> 0x18) -
                        (u32)(((f32)arg->timer.current + arg->timer.subFrame) *
                              (f32)(arg->args[1] >> 0x18)) /
                            (f32)arg->duration));
        if (arg->alpha < 0)
        {
            arg->alpha = 0.0f;
        }
    }
    else
    {
        arg->alpha = 0.0f;
        arg->args[0] = arg->args[0] - 1;
        if ((i32)arg->args[0] < 1)
        {
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
        arg->timer = 0;
    }
    arg->timer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0044b090
u32 BombEffects::OnDrawPlayAreaPulseColor(BombEffects *arg)
{
    ZunRect rect;

    rect.left = 32.0f;
    rect.top = 16.0f;
    rect.right = 416.0f;
    rect.bottom = 464.0f;
    ScreenEffect::DrawSquare(&rect,
                             arg->alpha << 0x18 | (arg->args[1] & 0xffffff));
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0044b0e0
u32 BombEffects::OnUpdateScreenShake(BombEffects *arg)
{
    if (g_GameManager.isTimeStopped != 0)
        return CHAIN_CALLBACK_RESULT_CONTINUE;

    if (g_GameManager.framesThisStage <= 1)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    }

    arg->timer++;
    if (arg->duration >= arg->timer.current)
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;

    f32 fVar1 = (f32)(i32)arg->args[0] +
                (((f32)arg->timer.current + arg->timer.subFrame) *
                 (f32)(i32)(arg->args[1] - arg->args[0])) /
                    (f32)arg->duration;
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
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x0044b280
ZunResult BombEffects::AddedCallback(BombEffects *arg)
{
    arg->timer = 0;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x0044b2c0
ZunResult BombEffects::DeletedCallback(BombEffects *arg)
{
    arg->calcChain->deletedCallback = NULL;
    g_Chain.Cut(arg->drawChain);
    arg->drawChain = NULL;
    delete arg;
    return ZUN_SUCCESS;
}

#pragma var_order(local_8, local_c, bombEffects)
// FUNCTION: TH07 0x0044b310
BombEffects *BombEffects::RegisterChain(i32 type, i32 duration, u32 arg1,
                                        u32 arg2, u32 arg3)
{
    ChainElem *local_8 = NULL;
    ChainElem *local_c = NULL;

    BombEffects *bombEffects = new BombEffects;
    if (bombEffects == NULL)
        return NULL;

    memset(bombEffects, 0, sizeof(BombEffects));
    switch (type)
    {
    case 0:
        local_8 = g_Chain.CreateElem((ChainCallback)OnUpdateFadeOut);
        local_c = g_Chain.CreateElem((ChainCallback)OnDrawFullScreenColor);
        break;
    case 1:
        local_8 = g_Chain.CreateElem((ChainCallback)OnUpdateScreenShake);
        break;
    case 2:
        local_8 = g_Chain.CreateElem((ChainCallback)OnUpdateFadeIn);
        local_c = g_Chain.CreateElem((ChainCallback)OnDrawPlayAreaColor);
        break;
    case 4:
        local_8 = g_Chain.CreateElem((ChainCallback)OnUpdateFadeIn);
        local_c = g_Chain.CreateElem((ChainCallback)OnDrawFullScreenColor);
        break;
    case 3:
        local_8 = g_Chain.CreateElem((ChainCallback)OnUpdatePulse);
        local_c = g_Chain.CreateElem((ChainCallback)OnDrawPlayAreaPulseColor);
    }
    local_8->addedCallback = (ChainLifecycleCallback)AddedCallback;
    local_8->deletedCallback = (ChainLifecycleCallback)DeletedCallback;
    local_8->arg = bombEffects;
    bombEffects->type = type;
    bombEffects->duration = duration;
    bombEffects->args[0] = arg1;
    bombEffects->args[1] = arg2;
    bombEffects->args[2] = arg3;
    if (g_Chain.AddToCalcChain(local_8, 0xf) != 0)
        return NULL;

    if (local_c != NULL)
    {
        local_c->arg = bombEffects;
        g_Chain.AddToDrawChain(local_c, 0x11);
    }
    bombEffects->calcChain = local_8;
    bombEffects->drawChain = local_c;
    return bombEffects;
}
