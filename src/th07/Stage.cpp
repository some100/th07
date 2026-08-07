#include "Stage.hpp"

#include "AnmManager.hpp"
#include "Chain.hpp"
#include "EffectManager.hpp"
#include "FileSystem.hpp"
#include "GameErrorContext.hpp"
#include "GameManager.hpp"
#include "Gui.hpp"
#include "ScreenEffect.hpp"
#include "Supervisor.hpp"
#include "ZunResult.hpp"
#include "dxutil.hpp"
#include "utils.hpp"

// GLOBAL: TH07 0x0049f588
StageAnms g_EnemyAnmStageFiles[9] = {
    {"dummy", "dummy"},
    // STRING: TH07 0x00497ffc
    {"data/stg1enm.anm", NULL},
    // STRING: TH07 0x00497fe8
    {"data/stg2enm.anm", NULL},
    // STRING: TH07 0x00497fd4
    {"data/stg3enm.anm", NULL},
    // STRING: TH07 0x00497fc0
    {"data/stg4enm.anm", NULL},
    // STRING: TH07 0x00497fac
    {"data/stg5enm.anm", NULL},
    // STRING: TH07 0x00497f98
    {"data/stg6enm.anm", NULL},
    // STRING: TH07 0x00497f84
    {"data/stg7enm.anm", NULL},
    // STRING: TH07 0x00497f70
    {"data/stg8enm.anm", NULL},
};

// GLOBAL: TH07 0x0049f64c
const char *g_StageFiles[9] = {
    "dummy",
    // STRING: TH07 0x00498a04
    "data/stage1.std",
    // STRING: TH07 0x004989f4
    "data/stage2.std",
    // STRING: TH07 0x004989e4
    "data/stage3.std",
    // STRING: TH07 0x004989d4
    "data/stage4.std",
    // STRING: TH07 0x004989c4
    "data/stage5.std",
    // STRING: TH07 0x004989b4
    "data/stage6.std",
    // STRING: TH07 0x004989a4
    "data/stage7.std",
    // STRING: TH07 0x00498994
    "data/stage8.std",
};

// GLOBAL: TH07 0x01347ac0
ChainElem g_UnusedChain;

// GLOBAL: TH07 0x01347ae0
ChainElem g_StageOnDrawHighPrioChain;

// GLOBAL: TH07 0x01347b00
Stage g_Stage;

// GLOBAL: TH07 0x0134cdb4
ChainElem g_StageOnDrawLowPrioChain;

// GLOBAL: TH07 0x0134cdd4
ChainElem g_StageCalcChain;

// FUNCTION: TH07 0x00405080
Stage::Stage()
{
    memset(this, NULL, sizeof(Stage));
    this->cam.pos = Float3(0.0f, 0.0f, 1000.0f);
    this->cam.lookAt = Float3(0.0f, 0.0f, 0.0f);
    this->cam.up = Float3(0.0f, 1.0f, 0.0f);
    this->cam.fov = ZUN_PI / 6.0f;
    this->camEnd = this->cam;
    this->camStart = this->cam;
}

// FUNCTION: TH07 0x004052d0
f32 InterpCubic(f32 p0, f32 p1, f32 p2, f32 p3, f32 t)
{
    f32 v[4];
    v[0] = (t - 1.0f) * (t - 1.0f) * (2.0f * t + 1.0f);
    v[2] = t * t * (3.0f - 2.0f * t);
    v[1] = (1.0f - t) * (1.0f - t) * t;
    v[3] = (t - 1.0f) * t * t;

    return v[0] * p0 + v[2] * p1 + v[1] * p2 + v[3] * p3;
}

// FUNCTION: TH07 0x00405370
void Stage::UpdateScriptAndCamera(Stage *stage, i32 param_2,
                                  Float3 *param_3, Float3 *param_4,
                                  Float3 *param_5, Float3 *param_6,
                                  Float3 *param_7)
{
    f32 t;

    if (stage->timers[param_2] < stage->timersMax[param_2])
    {
        stage->timers[param_2]++;
        t = stage->timers[param_2].AsFloat() /
            (f32)stage->timersMax[param_2];
    }
    else
    {
        stage->timers[param_2] = stage->timersMax[param_2];
        t = 1.0f;
        stage->timersMax[param_2] = 0;
    }
    switch (stage->interpModes[param_2])
    {
    case 1:
        t = 1.0f - t;
        t = 1.0f - t * t;
        break;
    case 2:
        t = 1.0f - t;
        t = 1.0f - t * t * t;
        break;
    case 3:
        t = 1.0f - t;
        t = 1.0f - t * t * t * t;
        break;
    case 4:
        t = t * t;
        break;
    case 5:
        t = t * t * t;
        break;
    case 6:
        t = t * t * t * t;
    }
    if (stage->interpModes[param_2] != 7)
    {
        *param_3 = *param_5 - *param_4;
        *param_3 = t * *param_3 + *param_4;
    }
    else
    {
        param_3->x =
            InterpCubic(param_4->x, param_5->x, param_6->x, param_7->x, t);
        param_3->y =
            InterpCubic(param_4->y, param_5->y, param_6->y, param_7->y, t);
        param_3->z =
            InterpCubic(param_4->z, param_5->z, param_6->z, param_7->z, t);
    }
}

// FUNCTION: TH07 0x00405690
u32 Stage::OnUpdate(Stage *arg)
{
    Float3 pos;
    StdRawInstr *curInstr;

    if (!arg->stdData)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    if (arg->scriptWaitTime != 0)
    {
        i32 instrIdx = 0;
        curInstr = arg->beginningOfScript;
        arg->instructionIndex = 0;
        while ((curInstr->opcode != 31 ||
                arg->scriptWaitTime != curInstr->args.args[0].i) &&
               curInstr->frame != -1)
        {
            curInstr++;
            instrIdx++;
        }
        if (curInstr->frame != -1)
        {
            arg->instructionIndex = instrIdx + 1;
            arg->scriptTime = curInstr->frame;
            arg->scriptWaitTime = 0;
        }
    }
loop_begin:
    curInstr = arg->beginningOfScript + arg->instructionIndex;
    if (arg->scriptTime >= curInstr->frame && curInstr->frame != -1)
    {
        switch (curInstr->opcode)
        {
        case 0:
            if (curInstr->frame == -1)
            {
                arg->positionInterpInitial = *curInstr->args.AsVec();
                arg->pos.x = arg->positionInterpInitial.x;
                arg->pos.y = arg->positionInterpInitial.y;
                arg->pos.z = arg->positionInterpInitial.z;
            }
            else
            {
                pos = *curInstr->args.AsVec();
                arg->pos.x = pos.x;
                arg->pos.y = pos.y;
                arg->pos.z = pos.z;
                arg->positionInterpInitial = pos;
                arg->positionInterpStartTime = curInstr->frame;
                curInstr++;
                while (curInstr->opcode != 0)
                {
                    curInstr++;
                }
                arg->positionInterpEndTime = curInstr->frame;
                arg->positionStart = *curInstr->args.AsVec();
            }
            break;
        case 1:
            arg->skyFog.color.color = curInstr->args.args[0].u;
            arg->skyFog.nearPlane = curInstr->args.args[1].f;
            arg->skyFog.farPlane = curInstr->args.args[2].f;
            arg->fogStart = arg->skyFog;
            break;
        case 2:
            arg->fogEnd = arg->skyFog;
            arg->skyFogInterpDuration = curInstr->args.args[0].i;
            arg->skyFogInterpTimer = 0;
            break;
        case 5:
            if (arg->cameraTeleported)
            {
                Float3 diff = *curInstr->args.AsVec() - arg->camEnd.pos;
                EffectManager::DoSomethingWithEffects(&diff);
                arg->cameraTeleported = 0;
            }
            arg->camStart.pos = arg->camEnd.pos;
            arg->camEnd.pos = *curInstr->args.AsVec();
            if (arg->timersMax[0] == 0)
            {
                arg->cam.pos = *curInstr->args.AsVec();
            }
            break;
        case 6:
            arg->timersMax[0] = curInstr->args.args[0].i;
            arg->timers[0] = 0;
            arg->interpModes[0] = curInstr->args.args[1].i;
            break;
        case 7:
            arg->camStart.lookAt = arg->camEnd.lookAt;
            arg->camEnd.lookAt = *curInstr->args.AsVec();
            if (arg->timersMax[1] == 0)
            {
                arg->cam.lookAt = *curInstr->args.AsVec();
            }
            break;
        case 8:
            arg->timersMax[1] = curInstr->args.args[0].i;
            arg->timers[1] = 0;
            arg->interpModes[1] = curInstr->args.args[1].i;
            break;
        case 9:
            arg->camStart.up = arg->camEnd.up;
            arg->camEnd.up = *curInstr->args.AsVec();
            if (arg->timersMax[2] == 0)
            {
                arg->cam.up = *curInstr->args.AsVec();
            }
            break;
        case 10:
            arg->timersMax[2] = curInstr->args.args[0].i;
            arg->interpModes[2] = curInstr->args.args[1].i;
            arg->timers[2] = 0;
            break;
        case 11:
            arg->camStart.fov = arg->camEnd.fov;
            arg->camEnd.fov = curInstr->args.args[0].f;
            if (arg->timersMax[3] == 0)
            {
                arg->cam.fov = curInstr->args.args[0].f;
            }
            break;
        case 12:
            arg->timersMax[3] = curInstr->args.args[0].i;
            arg->timers[3] = 0;
            arg->interpModes[3] = curInstr->args.args[1].i;
            break;
        case 13:
            arg->color = curInstr->args.args[0].u;
            break;
        case 3:
            if (arg->scriptWaitTime != 0)
            {
                arg->scriptWaitTime = 0;
                break;
            }
            goto LAB_004061aa;
        case 4:
            arg->instructionIndex = curInstr->args.args[0].i;
            arg->scriptTime = curInstr->args.args[1].i;
            arg->timersMax[0] = 0;
            arg->cameraTeleported = 1;
            goto loop_begin;
        case 14:
            arg->camStart.pos = *curInstr->args.AsVec();
            break;
        case 15:
            arg->camEnd.pos = *curInstr->args.AsVec();
            break;
        case 16:
            arg->camTangentStart.pos = *curInstr->args.AsVec();
            break;
        case 17:
            arg->camTangentEnd.pos = *curInstr->args.AsVec();
            break;
        case 18:
            arg->timersMax[0] = curInstr->args.args[0].i;
            arg->timers[0] = 0;
            arg->interpModes[0] = 7;
            break;
        case 19:
            arg->camStart.lookAt = *curInstr->args.AsVec();
            break;
        case 20:
            arg->camEnd.lookAt = *curInstr->args.AsVec();
            break;
        case 21:
            arg->camTangentStart.lookAt = *curInstr->args.AsVec();
            break;
        case 22:
            arg->camTangentEnd.lookAt = *curInstr->args.AsVec();
            break;
        case 23:
            arg->timersMax[1] = curInstr->args.args[0].i;
            arg->timers[1] = 0;
            arg->interpModes[1] = 7;
            break;
        case 24:
            arg->camStart.up = *curInstr->args.AsVec();
            break;
        case 25:
            arg->camEnd.up = *curInstr->args.AsVec();
            break;
        case 26:
            arg->camTangentStart.up = *curInstr->args.AsVec();
            break;
        case 27:
            arg->camTangentEnd.up = *curInstr->args.AsVec();
            break;
        case 28:
            arg->timersMax[2] = curInstr->args.args[0].i;
            arg->timers[2] = 0;
            arg->interpModes[2] = 7;
            break;
        case 29:
            if (curInstr->args.args[0].i >= 0)
            {
                g_AnmManager->ExecuteAnmIdx(&arg->vm1, curInstr->args.args[0].i + ANM_OFFSET_STAGE_BG1);
            }
            else
            {
                arg->vm1.activeSpriteIdx = -1;
            }
            break;
        case 30:
            if (curInstr->args.args[0].i >= 0)
            {
                g_AnmManager->ExecuteAnmIdx(&arg->vm2, curInstr->args.args[0].i + ANM_OFFSET_STAGE_BG1);
            }
            else
            {
                arg->vm1.activeSpriteIdx = -1;
            }
            break;
        }
        arg->instructionIndex++;
        goto loop_begin;
    }
LAB_004061aa: {
    i32 camIdx = 0;
    if (arg->timersMax[camIdx] != 0)
    {
        UpdateScriptAndCamera(arg, camIdx, &arg->cam.pos, &arg->camStart.pos,
                              &arg->camEnd.pos, &arg->camTangentStart.pos,
                              &arg->camTangentEnd.pos);
    }
    camIdx = 1;
    if (arg->timersMax[camIdx] != 0)
    {
        UpdateScriptAndCamera(arg, camIdx, &arg->cam.lookAt, &arg->camStart.lookAt,
                              &arg->camEnd.lookAt, &arg->camTangentStart.lookAt,
                              &arg->camTangentEnd.lookAt);
    }
    camIdx = 2;
    if (arg->timersMax[camIdx] != 0)
    {
        UpdateScriptAndCamera(arg, camIdx, &arg->cam.up, &arg->camStart.up,
                              &arg->camEnd.up, &arg->camTangentStart.up,
                              &arg->camTangentEnd.up);
    }
    camIdx = 3;

#pragma var_order(fovDiff, t)
    if (arg->timersMax[camIdx] != 0)
    {
        f32 t;
        f32 fovDiff;

        if (arg->timers[camIdx] < arg->timersMax[camIdx])
        {
            arg->timers[camIdx]++;
            t = arg->timers[camIdx].AsFloat() /
                (f32)arg->timersMax[camIdx];
        }
        else
        {
            arg->timers[camIdx] = arg->timersMax[camIdx];
            t = 1.0f;
            arg->timersMax[camIdx] = 0;
        }
        switch (arg->interpModes[camIdx])
        {
        case 1:
            t = 1.0f - t;
            t = 1.0f - t * t;
            break;
        case 2:
            t = 1.0f - t;
            t = 1.0f - t * t * t;
            break;
        case 3:
            t = 1.0f - t;
            t = 1.0f - t * t * t * t;
            break;
        case 4:
            t = t * t;
            break;
        case 5:
            t = t * t * t;
            break;
        case 6:
            t = t * t * t * t;
        }
        fovDiff = arg->camEnd.fov - arg->camStart.fov;
        arg->cam.fov = fovDiff * t + arg->camStart.fov;
    }
}
    D3DXVec3Normalize(arg->cam.lookAtDir.asD3DX(), arg->cam.lookAt.asD3DX());
    if (arg->skyFogInterpDuration != 0)
    {
        arg->skyFogInterpTimer++;
        f32 t = arg->skyFogInterpTimer.AsFloat() /
                (f32)arg->skyFogInterpDuration;
        if (t >= 1.0f)
        {
            t = 1.0f;
        }
        for (i32 i = 0; i < 4; i++)
        {
            arg->skyFog.color.raw[i] =
                (u8)(((f32)arg->fogStart.color.raw[i] -
                      (f32)arg->fogEnd.color.raw[i]) *
                         t +
                     (f32)arg->fogEnd.color.raw[i]);
        }
        arg->skyFog.nearPlane =
            (arg->fogStart.nearPlane - arg->fogEnd.nearPlane) * t +
            arg->fogEnd.nearPlane;
        arg->skyFog.farPlane =
            (arg->fogStart.farPlane - arg->fogEnd.farPlane) * t +
            arg->fogEnd.farPlane;

        if (arg->skyFogInterpTimer >= arg->skyFogInterpDuration)
        {
            arg->skyFogInterpDuration = 0;
        }
    }
    if (curInstr->opcode != 3)
    {
        arg->scriptTime++;
    }
    arg->UpdateObjects();
    if (arg->spellCardState >= 1)
    {
        if (arg->ticksSinceSpellcardStarted == 60)
        {
            arg->spellCardState++;
        }
        arg->ticksSinceSpellcardStarted++;
        for (i32 j = 0; j < arg->numSpellcardVms; j++)
        {
            g_AnmManager->ExecuteScript(&arg->spellcardVms[j]);
        }
    }
    if (arg->vm1.activeSpriteIdx > 0)
    {
        g_AnmManager->ExecuteScript(&arg->vm1);
    }
    if (arg->vm2.activeSpriteIdx > 0)
    {
        g_AnmManager->ExecuteScript(&arg->vm2);
    }
    arg->stageFrameCounter++;
    if (arg->stageFrameCounter % 500 == 250)
    {
        if (g_GameManager.CheckGameIntegrity())
        {
            return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
        }
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00406930
void Stage::SmoothBlendColor(ZunColor param_1)
{
    ZunColor color;

    if (!this->color2.bytes.a)
    {
        this->color2 = param_1;
    }
    else
    {
        color = param_1;
        this->color2.bytes.r =
            (u8)((color.bytes.r + (u32)this->color2.bytes.r) >> 1);
        this->color2.bytes.g =
            (u8)((color.bytes.g + (u32)this->color2.bytes.g) >> 1);
        this->color2.bytes.b =
            (u8)((color.bytes.b + (u32)this->color2.bytes.b) >> 1);
        this->color2.bytes.a =
            (u8)((color.bytes.a + (u32)this->color2.bytes.a) >> 1);
    }
}

// FUNCTION: TH07 0x004069d0
u32 Stage::OnDrawHighPrio(Stage *arg)
{
    ZunColor fogColor;
    D3DVIEWPORT8 viewport;

    g_AnmManager->ResetVertexBuffer();
    g_AnmManager->SetVertexShader(255);
    g_AnmManager->SetSprite(NULL);
    g_AnmManager->SetTexture(NULL);
    g_AnmManager->SetColorOp(255);
    g_AnmManager->SetBlendMode(255);
    g_AnmManager->SetZWriteDisable(255);
    g_AnmManager->ClearFrameState();
    g_AnmManager->SetCameraMode(255);
    if (!g_Supervisor.cfg.disableFog)
    {
        g_Supervisor.DisableFog();
    }
    g_AnmManager->Flush();
    if (arg->clearBackground)
    {
        viewport.X = 32;
        viewport.Y = 16;
        viewport.Width = 384;
        viewport.Height = 448;
        g_Supervisor.d3dDevice->SetViewport(&viewport);
        g_Supervisor.d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f,
                                      0);
        arg->clearBackground = 0;
    }
    g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    if (arg->color2.bytes.a > 0)
    {
        g_AnmManager->SetColorWithMulEnabled(arg->color2.color);
    }
    arg->color2.bytes.a = 0;
    arg->color2.bytes.r = 128;
    arg->color2.bytes.g = 128;
    arg->color2.bytes.b = 128;
    if (arg->spellCardState <= 1)
    {
        if (!g_Gui.IsStageFinished())
        {
            if (0 < arg->vm1.activeSpriteIdx)
            {
                g_AnmManager->DrawAndFlush(&arg->vm1);
            }
            if (0 < arg->vm2.activeSpriteIdx)
            {
                g_AnmManager->DrawAndFlush(&arg->vm2);
            }
        }
    }
    if (arg->color != 0)
    {
        g_Supervisor.d3dDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET,
                                      arg->color, 1.0f, 0);
    }
    else
    {
        g_Supervisor.d3dDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, arg->color, 1.0f,
                                      0);
    }
    g_Supervisor.SetRenderState(D3DRS_ZFUNC, 4);
    if (!g_AnmManager->colorMulEnabled)
    {
        g_Supervisor.SetRenderState(D3DRS_FOGCOLOR, arg->skyFog.color.color);
    }
    else
    {
        fogColor.color = arg->skyFog.color.color;
        fogColor.bytes.r = ZunColor::Multiply(fogColor.bytes.r, g_AnmManager->color.bytes.r);
        fogColor.bytes.g = ZunColor::Multiply(fogColor.bytes.g, g_AnmManager->color.bytes.g);
        fogColor.bytes.b = ZunColor::Multiply(fogColor.bytes.b, g_AnmManager->color.bytes.b);
        g_Supervisor.SetRenderState(D3DRS_FOGCOLOR, fogColor.color);
    }
    g_Supervisor.SetRenderState(D3DRS_FOGSTART, *(DWORD *)&arg->skyFog.nearPlane);
    g_Supervisor.SetRenderState(D3DRS_FOGEND, *(DWORD *)&arg->skyFog.farPlane);
    if (!g_Supervisor.cfg.disableFog)
    {
        g_Supervisor.EnableFog();
    }
    if (arg->spellCardState <= 1)
    {
        if (!g_Gui.IsStageFinished())
        {
            arg->RenderObjects(0);
            arg->RenderObjects(1);
        }
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(fog, alpha, local_1c, i)
// FUNCTION: TH07 0x00406de0
u32 Stage::OnDrawLowPrio(Stage *arg)
{
    i32 i;
    ZunRect local_1c;
    i32 alpha;
    f32 fog;

    if (arg->spellCardState <= 1)
    {
        if (g_Gui.IsStageFinished() == 0)
        {
            arg->RenderObjects(2);
            arg->RenderObjects(3);
            if (!g_Supervisor.cfg.disableFog)
            {
                g_Supervisor.DisableFog();
            }
            g_EffectManager.UpdateSpecialEffect();
            if (arg->spellCardState == 1)
            {
                local_1c.left = 32.0f;
                local_1c.top = 16.0f;
                local_1c.right = 416.0f;
                local_1c.bottom = 464.0f;
                alpha = arg->ticksSinceSpellcardStarted * 255 / 60;
                g_AnmManager->Flush();
                g_Supervisor.SetRenderState(D3DRS_ZFUNC, 8);
                if (!g_Supervisor.cfg.disableFog)
                {
                    g_Supervisor.SetRenderState(D3DRS_FOGENABLE, 0);
                }
                ScreenEffect::DrawSquare(&local_1c, alpha << 24);
            }
        }
    }
    g_AnmManager->Flush();
    g_Supervisor.SetRenderState(D3DRS_ZFUNC, 8);
    if (!g_Supervisor.cfg.disableFog)
    {
        g_Supervisor.DisableFog();
    }
    if (arg->spellCardState >= 1)
    {
        for (i = 0; i < arg->numSpellcardVms; i++)
        {
            g_AnmManager->DrawAndFlush(&arg->spellcardVms[i]);
        }
    }
    AnmManager::SetCameraModeStatic(g_AnmManager, 0);
    arg->SetupCameraStageBackground();
    g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    fog = 1000.0f;
    g_Supervisor.SetRenderState(D3DRS_FOGSTART, *(DWORD *)&fog);
    fog = 2000.0f;
    g_Supervisor.SetRenderState(D3DRS_FOGEND, *(DWORD *)&fog);
    if (!arg->isDarkening)
    {
        g_AnmManager->SetColor(0x80808080);
    }
    arg->isDarkening = 0;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00407000
ZunResult Stage::AddedCallback(Stage *arg)
{
    i32 i;

    arg->scriptTime = 0;
    arg->instructionIndex = 0;
    arg->pos.x = 0.0f;
    arg->pos.y = 0.0f;
    arg->pos.z = 0.0f;
    arg->spellCardState = 0;
    arg->skyFogInterpDuration = 0;
    switch (g_GameManager.currentStage)
    {
    case 1:
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_BG1, "data/stg1bg.anm", ANM_OFFSET_STAGE_BG1) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 2:
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_BG1, "data/stg2bg.anm", ANM_OFFSET_STAGE_BG1) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 3:
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_BG1, "data/stg3bg.anm", ANM_OFFSET_STAGE_BG1) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 4:
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_BG1, "data/stg4bg.anm", ANM_OFFSET_STAGE_BG1) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }

        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_BG2, "data/stg4bg2.anm", ANM_OFFSET_STAGE_BG2) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }

        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_BG3, "data/stg4bg3.anm", ANM_OFFSET_STAGE_BG3) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }

        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_BG4, "data/stg4bg4.anm", ANM_OFFSET_STAGE_BG4) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }

        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_BG5, "data/stg4bg5.anm", ANM_OFFSET_STAGE_BG5) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 5:
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_BG1, "data/stg5bg.anm", ANM_OFFSET_STAGE_BG1) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 6:
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_BG1, "data/stg6bg.anm", ANM_OFFSET_STAGE_BG1) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 7:
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_BG1, "data/stg7bg.anm", ANM_OFFSET_STAGE_BG1) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 8:
        if (g_AnmManager->LoadAnms(ANM_FILE_STAGE_BG1, "data/stg8bg.anm", ANM_OFFSET_STAGE_BG1) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
    }
    if (arg->LoadStageData(g_StageFiles[g_GameManager.currentStage]) !=
        ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    arg->skyFog.color.color = 0xff000000;
    arg->skyFog.nearPlane = 200.0f;
    arg->skyFog.farPlane = 500.0f;
    arg->cam.pos = Float3(0.0f, 0.0f, 1000.0f);
    arg->cam.lookAt = Float3(0.0f, 0.0f, 0.0f);
    arg->cam.up = Float3(0.0f, 1.0f, 0.0f);
    arg->cam.fov = ZUN_PI / 6.0f;
    arg->camEnd = arg->cam;
    arg->camStart = arg->cam;
    for (i = 0; i < 4; i++)
    {
        arg->timersMax[i] = 0;
        arg->timers[i] = 0;
    }
    arg->scriptWaitTime = 0;
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00407410
ZunResult Stage::DeletedCallback(Stage *arg)
{
    g_AnmManager->ReleaseAnm(5);
    g_AnmManager->ReleaseAnm(6);
    g_AnmManager->ReleaseAnm(7);
    g_AnmManager->ReleaseAnm(8);
    g_AnmManager->ReleaseAnm(9);
    SAFE_FREE(arg->quadVms);
    SAFE_FREE(arg->stdData);
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x004074c0
ZunResult Stage::RegisterChain(i32 stage)
{
    Stage *mgr = &g_Stage;
    memset(mgr, 0, sizeof(Stage));
    mgr->stdData = NULL;
    mgr->stageFrameCounter = 0;
    mgr->stage = stage;
    g_StageCalcChain.callback = (ChainCallback)OnUpdate;
    g_StageCalcChain.addedCallback = NULL;
    g_StageCalcChain.deletedCallback = NULL;
    g_StageCalcChain.addedCallback = (ChainLifecycleCallback)AddedCallback;
    g_StageCalcChain.deletedCallback = (ChainLifecycleCallback)DeletedCallback;
    g_StageCalcChain.arg = mgr;
    if (g_Chain.AddToCalcChain(&g_StageCalcChain, 7))
    {
        return ZUN_ERROR;
    }

    g_StageOnDrawHighPrioChain.callback = (ChainCallback)OnDrawHighPrio;
    g_StageOnDrawHighPrioChain.addedCallback = NULL;
    g_StageOnDrawHighPrioChain.deletedCallback = NULL;
    g_StageOnDrawHighPrioChain.arg = mgr;
    g_Chain.AddToDrawChain(&g_StageOnDrawHighPrioChain, 3);
    g_StageOnDrawLowPrioChain.callback = (ChainCallback)OnDrawLowPrio;
    g_StageOnDrawLowPrioChain.addedCallback = NULL;
    g_StageOnDrawLowPrioChain.deletedCallback = NULL;
    g_StageOnDrawLowPrioChain.arg = mgr;
    g_Chain.AddToDrawChain(&g_StageOnDrawLowPrioChain, 4);
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x004075d0
void Stage::CutChain()
{
    g_Chain.Cut(&g_StageCalcChain);
    g_Chain.Cut(&g_StageOnDrawHighPrioChain);
    g_Chain.Cut(&g_StageOnDrawLowPrioChain);
}

#pragma var_order(vmIdx, i, obj, quad)
// FUNCTION: TH07 0x00407610
ZunResult Stage::LoadStageData(const char *stdPath)
{
    StdRawQuadBasic *quad;
    StdRawObject *obj;
    i32 i;
    i32 vmIdx;

    this->stdData = (StdRawHeader *)FileSystem::OpenFile(stdPath, 0);
    if (!this->stdData)
    {
        // STRING: TH07 0x0049888c
        g_GameErrorContext.Log("ステージデータが見つかりません。データが壊れています\r\n");
        return ZUN_ERROR;
    }

    this->objectsCount = this->stdData->objectsCount;
    this->quadCount = this->stdData->quadCount;
    this->objectInstances =
        (StdRawInstance *)(this->stdData->facesOffset + (i32)this->stdData);
    this->beginningOfScript =
        (StdRawInstr *)(this->stdData->scriptOffset + (i32)this->stdData);
    this->objects = (StdRawObject **)(this->stdData + 1);
    for (i = 0; i < this->objectsCount; i++)
    {
        this->objects[i] =
            (StdRawObject *)((i32)this->objects[i] + (i32)this->stdData);
    }
    this->quadVms = (AnmVm *)ZunMemory::Alloc(this->quadCount * sizeof(AnmVm));
    for (i = 0, vmIdx = 0; i < this->objectsCount; i++)
    {
        obj = this->objects[i];
        obj->flags = 1;
        quad = &obj->firstQuad;
        while (quad->type >= 0)
        {
            g_AnmManager->ExecuteAnmIdx(&this->quadVms[vmIdx],
                                        quad->anmScript + ANM_OFFSET_STAGE_BG1);
            quad->vmIndex = vmIdx++;
            quad = (StdRawQuadBasic *)((i32)quad + quad->byteSize);
        }
    }
    return ZUN_SUCCESS;
}

#pragma var_order(local_8, vmCount, i, vm, object, quad)
// FUNCTION: TH07 0x004077f0
ZunResult Stage::UpdateObjects()
{
    StdRawQuadBasic *quad;
    StdRawObject *object;
    AnmVm *vm;
    i32 i;
    i32 vmCount;
    StdRawQuadBasic *local_8;

    for (i = 0; i < this->objectsCount; i++)
    {
        object = this->objects[i];
        if ((object->flags & 1) != 0)
        {
            vmCount = 0;
            quad = &object->firstQuad;
            while (0 <= quad->type)
            {
                vm = &this->quadVms[quad->vmIndex];
                switch (quad->type)
                {
                case 0:
                    g_AnmManager->ExecuteScript(vm);
                    break;
                case 1:
                    local_8 = quad;
                    g_AnmManager->ExecuteScript(vm);
                    break;
                }
                if (vm->currentInstruction)
                {
                    vmCount++;
                }
                quad =
                    (StdRawQuadBasic *)((u8 *)quad + quad->byteSize);
            }
            if (vmCount == 0)
            {
                object->flags &= 0xfffffffe;
            }
        }
    }
    return ZUN_SUCCESS;
}

#pragma var_order(curQuadVm, instancesDrawn, instance, fogState, worldMatrix, \
                  obj, dotProd, viewDir, quadPos, diffPos, curQuad, didDraw,  \
                  radius, projectSrc, var_98, origColor)
// FUNCTION: TH07 0x00407900
i32 Stage::RenderObjects(i32 zLevel)
{
    ZunColor origColor;
    f32 var_98;
    Float3 projectSrc;
    f32 radius;
    i32 didDraw;
    StdRawQuadBasic *curQuad;
    Float3 diffPos;
    Float3 quadPos;
    Float3 viewDir;
    f32 dotProd;
    StdRawObject *obj;
    D3DXMATRIX worldMatrix;
    i32 fogState;
    StdRawInstance *instance;
    i32 instancesDrawn;
    AnmVm *curQuadVm;

    instance = this->objectInstances;
    instancesDrawn = 0;
    didDraw = 0;
    projectSrc.x = 0.0f;
    projectSrc.y = 0.0f;
    projectSrc.z = 0.0f;
    fogState = 255;

    UpdateCamera();

    AnmManager::SetCameraModeStatic(g_AnmManager, 1);

    D3DXMatrixIdentity(&worldMatrix);

    while (instance->id >= 0)
    {
        obj = this->objects[instance->id];
        if (obj->zLevel == zLevel)
        {
            curQuad = &obj->firstQuad;

            quadPos.x = obj->pos.x + instance->pos.x - this->pos.x + obj->size.x / 2.0f;
            quadPos.y = obj->pos.y + instance->pos.y - this->pos.y + obj->size.y / 2.0f;
            quadPos.z = obj->pos.z + instance->pos.z - this->pos.z + obj->size.z / 2.0f;

            quadPos = quadPos - this->cam.pos;

            if (D3DXVec3LengthSq(quadPos.asD3DX()) > 1690000.0f)
            {
                // empty branch
            }
            else
            {
                dotProd = D3DXVec3Dot(quadPos.asD3DX(), this->cam.lookAtDir.asD3DX());
                radius = D3DXVec3Length(obj->size.asD3DX()) / 2.0f + 880.0f;

                if (dotProd > radius || dotProd < 60.0f)
                {
                    // empty branch
                }
                else
                {
                    obj->flags |= 2;
                    didDraw = 1;

                    while (curQuad->type >= 0)
                    {
                        curQuadVm = &this->quadVms[curQuad->vmIndex];
                        switch (curQuad->type)
                        {
                        case 0:
                            curQuadVm->pos.x = curQuadVm->offset.x + curQuad->pos.x + instance->pos.x - this->pos.x;
                            curQuadVm->pos.y = curQuadVm->offset.y + curQuad->pos.y + instance->pos.y - this->pos.y;
                            curQuadVm->pos.z = curQuadVm->offset.z + curQuad->pos.z + instance->pos.z - this->pos.z;

                            if (curQuad->size.x != 0.0f)
                            {
                                curQuadVm->scale.x = curQuad->size.x / curQuadVm->sprite->widthPx;
                            }
                            if (curQuad->size.y != 0.0f)
                            {
                                curQuadVm->scale.y = curQuad->size.y / curQuadVm->sprite->heightPx;
                            }

                            if (curQuadVm->autoRotate == 2)
                            {
                                worldMatrix.m[3][0] = curQuadVm->pos.x;
                                worldMatrix.m[3][1] = curQuadVm->pos.y;
                                worldMatrix.m[3][2] = curQuadVm->pos.z;

                                D3DXVec3Project(quadPos.asD3DX(), projectSrc.asD3DX(), &g_Supervisor.viewport, &g_Supervisor.projectionMatrix, &g_Supervisor.viewMatrix, &worldMatrix);

                                viewDir.x = g_Supervisor.viewMatrix.m[0][0];
                                viewDir.y = g_Supervisor.viewMatrix.m[0][1];
                                viewDir.z = g_Supervisor.viewMatrix.m[0][2];
                                D3DXVec3Normalize(viewDir.asD3DX(), viewDir.asD3DX());

                                if (curQuad->size.x != 0.0f)
                                {
                                    var_98 = curQuad->size.x;
                                }
                                else
                                {
                                    var_98 = curQuadVm->sprite->widthPx;
                                }

                                worldMatrix.m[3][0] += viewDir.x * var_98 * curQuadVm->scale.x;
                                worldMatrix.m[3][1] += viewDir.y * var_98 * curQuadVm->scale.x;
                                worldMatrix.m[3][2] += viewDir.z * var_98 * curQuadVm->scale.x;

                                D3DXVec3Project(viewDir.asD3DX(), projectSrc.asD3DX(), &g_Supervisor.viewport, &g_Supervisor.projectionMatrix, &g_Supervisor.viewMatrix, &worldMatrix);

                                diffPos = viewDir - quadPos;

                                curQuadVm->scale.x = D3DXVec3Length(diffPos.asD3DX()) / var_98;
                                curQuadVm->scale.y = curQuadVm->scale.x;

                                diffPos = curQuadVm->pos - this->cam.pos;

                                var_98 = D3DXVec3Length(diffPos.asD3DX());
                                origColor = curQuadVm->color;

                                if (this->skyFog.nearPlane < var_98)
                                {
                                    var_98 = (this->skyFog.nearPlane - var_98) / (this->skyFog.nearPlane - this->skyFog.farPlane);
                                    if (var_98 >= 1.0f)
                                    {
                                        goto skip_draw;
                                    }

                                    curQuadVm->color.bytes.b = curQuadVm->color.bytes.b - (u8)((curQuadVm->color.bytes.b - this->skyFog.color.bytes.b) * var_98);
                                    curQuadVm->color.bytes.g = curQuadVm->color.bytes.g - (u8)((curQuadVm->color.bytes.g - this->skyFog.color.bytes.g) * var_98);
                                    curQuadVm->color.bytes.r = curQuadVm->color.bytes.r - (u8)((curQuadVm->color.bytes.r - this->skyFog.color.bytes.r) * var_98);
                                    curQuadVm->color.bytes.a = (u8)(curQuadVm->color.bytes.a * (1.0f - var_98));
                                }

                                curQuadVm->pos = quadPos;

                                if (curQuadVm->pos.z < 0.0f || curQuadVm->pos.z > 1.0f)
                                {
                                    // empty branch
                                }
                                else
                                {
                                    if (fogState != 0)
                                    {
                                        if (!g_Supervisor.cfg.disableFog)
                                        {
                                            g_Supervisor.DisableFog();
                                        }
                                        fogState = 0;
                                    }
                                    g_AnmManager->DrawFacingCamera(curQuadVm);
                                }
                                curQuadVm->color = origColor;
                            }
                            else
                            {
                                if (!g_Supervisor.cfg.disableFog && fogState != 1)
                                {
                                    if (!g_Supervisor.cfg.disableFog)
                                    {
                                        g_Supervisor.EnableFog();
                                    }
                                    fogState = 1;
                                }
                                g_AnmManager->Draw3(curQuadVm);
                            }
                            break;
                        }
                    skip_draw:
                        curQuad = (StdRawQuadBasic *)((u8 *)curQuad + curQuad->byteSize);
                    }
                    instancesDrawn++;
                }
            }
        }
        instance++;
    }
    return 0;
}

#pragma var_order(eyeZ, centerY, centerX, aspectRatio, fov, upVec, atVec, eyeVec)
// FUNCTION: TH07 0x00408180
void Stage::SetupCameraStageBackground()
{
    Float3 eyeVec;
    Float3 atVec;
    Float3 upVec;
    f32 fov;
    f32 aspectRatio;
    f32 centerX;
    f32 centerY;
    f32 eyeZ;

    centerX = (f32)g_Supervisor.viewport.Width / 2.0f;
    centerY = (f32)g_Supervisor.viewport.Height / 2.0f;
    aspectRatio = (f32)g_Supervisor.viewport.Width / (f32)g_Supervisor.viewport.Height;
    fov = ZUN_PI / 10.0f;
    eyeZ = centerY / tanf(fov / 2.0f);

    upVec.x = 0.0f;
    upVec.y = -1.0f;
    upVec.z = 0.0f;

    atVec.x = centerX;
    atVec.y = centerY;
    atVec.z = 0.0f;

    eyeVec.x = centerX;
    eyeVec.y = centerY;
    eyeVec.z = eyeZ;

    D3DXMatrixLookAtLH(&g_Supervisor.viewMatrix, eyeVec.asD3DX(), atVec.asD3DX(), upVec.asD3DX());
    D3DXMatrixPerspectiveFovLH(
        &g_Supervisor.projectionMatrix, fov,
        aspectRatio,
        1.0f, 10000.0f);
    g_Supervisor.d3dDevice->SetTransform(D3DTS_VIEW, &g_Supervisor.viewMatrix);
    g_Supervisor.d3dDevice->SetTransform(D3DTS_PROJECTION,
                                         &g_Supervisor.projectionMatrix);
}

// FUNCTION: TH07 0x004082b0
void Stage::UpdateCamera()
{
    D3DXMatrixLookAtLH(&g_Supervisor.viewMatrix, this->cam.pos.asD3DX(),
                       (this->cam.lookAt + this->cam.pos).asD3DX(), this->cam.up.asD3DX());
    D3DXMatrixPerspectiveFovLH(&g_Supervisor.projectionMatrix, this->cam.fov,
                               (f32)g_Supervisor.viewport.Width /
                                   (f32)g_Supervisor.viewport.Height,
                               30.0f, 1800.0f);
    g_Supervisor.d3dDevice->SetTransform(D3DTS_VIEW, &g_Supervisor.viewMatrix);
    g_Supervisor.d3dDevice->SetTransform(D3DTS_PROJECTION,
                                         &g_Supervisor.projectionMatrix);
    D3DXVec3Cross(this->cam.right.asD3DX(), this->cam.lookAt.asD3DX(), this->cam.up.asD3DX());
    D3DXVec3Normalize(this->cam.right.asD3DX(), this->cam.right.asD3DX());
}
