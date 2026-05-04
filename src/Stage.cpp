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
StageAnms g_AnmStageFiles[9] = {
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
    this->camPos = D3DXVECTOR3(0.0f, 0.0f, 1000.0f);
    this->camLookAt = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    this->camUp = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
    this->fov = ZUN_PI / 6.0f;
    this->camPosEnd = this->camPos;
    this->camPosStart = this->camPos;
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
                                  D3DXVECTOR3 *param_3, D3DXVECTOR3 *param_4,
                                  D3DXVECTOR3 *param_5, D3DXVECTOR3 *param_6,
                                  D3DXVECTOR3 *param_7)
{
    f32 local_8;

    if (stage->timers[param_2] < stage->timersMax[param_2])
    {
        stage->timers[param_2]++;
        local_8 = ((f32)stage->timers[param_2].current +
                   stage->timers[param_2].subFrame) /
                  (f32)stage->timersMax[param_2];
    }
    else
    {
        stage->timers[param_2] = stage->timersMax[param_2];
        local_8 = 1.0f;
        stage->timersMax[param_2] = 0;
    }
    switch (stage->interpModes[param_2])
    {
    case 1:
        local_8 = 1.0f - (1.0f - local_8) * (1.0f - local_8);
        break;
    case 2:
        local_8 = 1.0f - local_8;
        local_8 = 1.0f - local_8 * local_8 * local_8;
        break;
    case 3:
        local_8 = 1.0f - local_8;
        local_8 = 1.0f - local_8 * local_8 * local_8 * local_8;
        break;
    case 4:
        local_8 = local_8 * local_8;
        break;
    case 5:
        local_8 = local_8 * local_8 * local_8;
        break;
    case 6:
        local_8 = local_8 * local_8 * local_8 * local_8;
    }
    if (stage->interpModes[param_2] == 7)
    {
        param_3->x =
            InterpCubic(param_4->x, param_5->x, param_6->x, param_7->x, local_8);
        param_3->y =
            InterpCubic(param_4->y, param_5->y, param_6->y, param_7->y, local_8);
        param_3->z =
            InterpCubic(param_4->z, param_5->z, param_6->z, param_7->z, local_8);
    }
    else
    {
        param_3->x = param_5->x - param_4->x;
        param_3->y = param_5->y - param_4->y;
        param_3->z = param_5->z - param_4->z;
        param_3->x = local_8 * param_3->x + param_4->x;
        param_3->y = local_8 * param_3->y + param_4->y;
        param_3->z = local_8 * param_3->z + param_4->z;
    }
}

// FUNCTION: TH07 0x00405690
u32 Stage::OnUpdate(Stage *arg)
{
    StdRawInstr *pSVar1;
    i32 local_3c;
    i32 local_38;
    f32 local_34;
    f32 local_30;
    D3DXVECTOR3 local_24;
    i32 local_18;
    f32 local_14;
    f32 local_10;
    f32 local_c;
    StdRawInstr *local_8;

    if (arg->stdData == NULL)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    if (arg->scriptWaitTime != 0)
    {
        local_18 = 0;
        local_8 = arg->beginningOfScript;
        arg->instructionIndex = 0;
        while ((local_8->opcode != 0x1f ||
                (arg->scriptWaitTime != local_8->args[0].i)) &&
               local_8->frame != -1)
        {
            local_18 += 1;
            local_8 = local_8 + 1;
        }
        if (local_8->frame != -1)
        {
            arg->instructionIndex = local_18 + 1;
            arg->scriptTime = local_8->frame;
            arg->scriptWaitTime = 0;
        }
    }
LAB_0040578a:
    while (true)
    {
        local_8 = arg->beginningOfScript + arg->instructionIndex;
        if ((arg->scriptTime < local_8->frame) || (local_8->frame == -1))
        {
            goto LAB_004061aa;
        }
        switch (local_8->opcode)
        {
        case 0:
            if (local_8->frame == -1)
            {
                arg->positionEnd = *(D3DXVECTOR3 *)local_8->args;
                arg->position = arg->positionEnd;
            }
            else
            {
                local_14 = local_8->args[0].f;
                local_10 = local_8->args[1].f;
                local_c = local_8->args[2].f;
                arg->position.x = local_14;
                arg->position.y = local_10;
                arg->position.z = local_c;
                arg->positionEnd.x = local_14;
                arg->positionEnd.y = local_10;
                arg->positionEnd.z = local_c;
                arg->positionInterpMode = local_8->frame;
                do
                {
                    pSVar1 = local_8;
                    local_8 = pSVar1 + 1;
                } while (pSVar1[1].opcode != 0);
                arg->positionInterpTimeMax = local_8->frame;
                arg->positionStart.x = pSVar1[1].args[0].f;
                arg->positionStart.y = pSVar1[1].args[1].f;
                arg->positionStart.z = pSVar1[1].args[2].f;
            }
            break;
        case 1:
            arg->fogColor.color = local_8->args[0].u;
            arg->skyFog.nearPlane = local_8->args[1].f;
            arg->skyFog.farPlane = local_8->args[2].f;
            arg->fogNearPlaneStart = arg->skyFog.nearPlane;
            arg->fogFarPlaneStart = arg->skyFog.farPlane;
            arg->fogColorStart = arg->fogColor;
            break;
        case 2:
            arg->fogNearPlaneEnd = arg->skyFog.nearPlane;
            arg->fogFarPlaneEnd = arg->skyFog.farPlane;
            arg->fogColorEnd = arg->fogColor;
            arg->skyFogInterpDuration = local_8->args[0].i;
            arg->skyFogInterpTimer = 0;
            break;
        case 3:
            if (arg->scriptWaitTime == 0)
            {
            LAB_004061aa:
                if (arg->timersMax[0] != 0)
                {
                    UpdateScriptAndCamera(arg, 0, &arg->camPos, &arg->camPosStart,
                                          &arg->camPosEnd, &arg->camPosBezier2,
                                          &arg->camPosBezier1);
                }
                if (arg->timersMax[1] != 0)
                {
                    UpdateScriptAndCamera(arg, 1, &arg->camLookAt, &arg->camLookAtStart,
                                          &arg->camLookAtEnd, &arg->camLookAtBezier2,
                                          &arg->camLookAtBezier1);
                }
                if (arg->timersMax[2] != 0)
                {
                    UpdateScriptAndCamera(arg, 2, &arg->camUp, &arg->camUpStart,
                                          &arg->camUpEnd, &arg->camUpBezier2,
                                          &arg->camUpBezier1);
                }
                if (arg->timersMax[3] != 0)
                {
                    if (arg->timers[3] < arg->timersMax[3])
                    {
                        arg->timers[3]++;
                        local_30 = ((f32)arg->timers[3].current + arg->timers[3].subFrame) /
                                   (f32)arg->timersMax[3];
                    }
                    else
                    {
                        arg->timers[3] = arg->timersMax[3];
                        local_30 = 1.0f;
                        arg->timersMax[3] = 0;
                    }
                    switch (arg->interpModes[3])
                    {
                    case 1:
                        local_30 = 1.0f - (1.0f - local_30) * (1.0f - local_30);
                        break;
                    case 2:
                        local_30 = 1.0f - local_30;
                        local_30 = 1.0f - local_30 * local_30 * local_30;
                        break;
                    case 3:
                        local_30 = 1.0f - local_30;
                        local_30 = 1.0f - local_30 * local_30 * local_30 * local_30;
                        break;
                    case 4:
                        local_30 = local_30 * local_30;
                        break;
                    case 5:
                        local_30 = local_30 * local_30 * local_30;
                        break;
                    case 6:
                        local_30 = local_30 * local_30 * local_30 * local_30;
                    }
                    arg->fov = (arg->fovEnd - arg->fovStart) * local_30 + arg->fovStart;
                }
                D3DXVec3Normalize(&arg->camLookAtDir, &arg->camLookAt);
                if (arg->skyFogInterpDuration != 0)
                {
                    arg->skyFogInterpTimer++;
                    local_34 = ((f32)arg->skyFogInterpTimer.current +
                                arg->skyFogInterpTimer.subFrame) /
                               (f32)arg->skyFogInterpDuration;
                    if (1.0f <= local_34)
                    {
                        local_34 = 1.0f;
                    }
                    for (local_38 = 0; local_38 < 4; local_38 += 1)
                    {
                        arg->fogColor.raw[local_38] =
                            ((f32)arg->fogColorStart.raw[local_38] -
                             (f32)arg->fogColorEnd.raw[local_38]) *
                                local_34 +
                            (f32)arg->fogColorEnd.raw[local_38];
                    }
                    arg->skyFog.nearPlane =
                        (arg->fogNearPlaneStart - arg->fogNearPlaneEnd) * local_34 +
                        arg->fogNearPlaneEnd;
                    arg->skyFog.farPlane =
                        (arg->fogFarPlaneStart - arg->fogFarPlaneEnd) * local_34 +
                        arg->fogFarPlaneEnd;
                    if (arg->skyFogInterpDuration <= arg->skyFogInterpTimer.current)
                    {
                        arg->skyFogInterpDuration = 0;
                    }
                }
                if (local_8->opcode != 3)
                {
                    arg->scriptTime++;
                }
                arg->UpdateObjects();
                if (0 < arg->spellCardState)
                {
                    if (arg->ticksSinceSpellcardStarted == 60)
                    {
                        arg->spellCardState = arg->spellCardState + 1;
                    }
                    arg->ticksSinceSpellcardStarted = arg->ticksSinceSpellcardStarted + 1;
                    for (local_3c = 0; local_3c < arg->numSpellcardVms; local_3c += 1)
                    {
                        g_AnmManager->ExecuteScript(arg->spellcardVms + local_3c);
                    }
                }
                if (0 < arg->vm1.activeSpriteIdx)
                {
                    g_AnmManager->ExecuteScript(&arg->vm1);
                }
                if (0 < arg->vm2.activeSpriteIdx)
                {
                    g_AnmManager->ExecuteScript(&arg->vm2);
                }
                arg->stageFrameCounter = arg->stageFrameCounter + 1;
                if ((arg->stageFrameCounter % 500 == 0xfa) &&
                    (g_GameManager.CheckGameIntegrity() != 0))
                {
                    return CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS;
                }
                return CHAIN_CALLBACK_RESULT_CONTINUE;
            }
            arg->scriptWaitTime = 0;
            break;
        case 4:
            goto switchD_004057f2_caseD_4;
        case 5:
            if (arg->pendingCameraShake != 0)
            {
                local_24.z = local_8->args[2].f - arg->camPosEnd.z;
                local_24.y = local_8->args[1].f - arg->camPosEnd.y;
                local_24.x = local_8->args[0].f - arg->camPosEnd.x;
                EffectManager::DoSomethingWithEffects(&local_24);
                arg->pendingCameraShake = 0;
            }
            arg->camPosStart = arg->camPosEnd;
            arg->camPosEnd = *(D3DXVECTOR3 *)local_8->args;
            if (arg->timersMax[0] == 0)
            {
                arg->camPos = *(D3DXVECTOR3 *)local_8->args;
            }
            break;
        case 6:
            arg->timersMax[0] = local_8->args[0].i;
            arg->timers[0] = 0;
            arg->interpModes[0] = local_8->args[1].i;
            break;
        case 7:
            arg->camLookAtStart = arg->camLookAtEnd;
            arg->camLookAtEnd = *(D3DXVECTOR3 *)local_8->args;
            if (arg->timersMax[1] == 0)
            {
                arg->camLookAt = *(D3DXVECTOR3 *)local_8->args;
            }
            break;
        case 8:
            arg->timersMax[1] = local_8->args[0].i;
            arg->timers[1] = 0;
            arg->interpModes[1] = local_8->args[1].i;
            break;
        case 9:
            arg->camUpStart = arg->camUpEnd;
            arg->camUpEnd = *(D3DXVECTOR3 *)local_8->args;
            if (arg->timersMax[2] == 0)
            {
                arg->camUp = *(D3DXVECTOR3 *)local_8->args;
            }
            break;
        case 10:
            arg->timersMax[2] = local_8->args[0].i;
            arg->interpModes[2] = local_8->args[1].i;
            arg->timers[2] = 0;
            break;
        case 0xb:
            arg->fovStart = arg->fovEnd;
            arg->fovEnd = local_8->args[0].f;
            if (arg->timersMax[3] == 0)
            {
                arg->fov = local_8->args[0].f;
            }
            break;
        case 0xc:
            arg->timersMax[3] = local_8->args[0].i;
            arg->timers[3] = 0;
            arg->interpModes[3] = local_8->args[1].i;
            break;
        case 0xd:
            arg->color = local_8->args[0].u;
            break;
        case 0xe:
            arg->camPosStart = *(D3DXVECTOR3 *)local_8->args;
            break;
        case 0xf:
            arg->camPosEnd = *(D3DXVECTOR3 *)local_8->args;
            break;
        case 0x10:
            (arg->camPosBezier2) = *(D3DXVECTOR3 *)local_8->args;
            break;
        case 0x11:
            (arg->camPosBezier1) = *(D3DXVECTOR3 *)local_8->args;
            break;
        case 0x12:
            arg->timersMax[0] = local_8->args[0].i;
            arg->timers[0] = 0;
            arg->interpModes[0] = 7;
            break;
        case 0x13:
            arg->camLookAtStart = *(D3DXVECTOR3 *)local_8->args;
            break;
        case 0x14:
            arg->camLookAtEnd = *(D3DXVECTOR3 *)local_8->args;
            break;
        case 0x15:
            arg->camLookAtBezier2 = *(D3DXVECTOR3 *)local_8->args;
            break;
        case 0x16:
            arg->camLookAtBezier1 = *(D3DXVECTOR3 *)local_8->args;
            break;
        case 0x17:
            arg->timersMax[1] = local_8->args[0].i;
            arg->timers[1] = 0;
            arg->interpModes[1] = 7;
            break;
        case 0x18:
            arg->camUpStart = *(D3DXVECTOR3 *)local_8->args;
            break;
        case 0x19:
            arg->camUpEnd = *(D3DXVECTOR3 *)local_8->args;
            break;
        case 0x1a:
            arg->camUpBezier2 = *(D3DXVECTOR3 *)local_8->args;
            break;
        case 0x1b:
            arg->camUpBezier1 = *(D3DXVECTOR3 *)local_8->args;
            break;
        case 0x1c:
            arg->timersMax[2] = local_8->args[0].i;
            arg->timers[2] = 0;
            arg->interpModes[2] = 7;
            break;
        case 0x1d:
            if (local_8->args[0].i < 0)
            {
                arg->vm1.activeSpriteIdx = -1;
            }
            else
            {
                g_AnmManager->ExecuteAnmIdx(&arg->vm1, local_8->args[0].i + 0x300);
            }
            break;
        case 0x1e:
            if (local_8->args[0].i < 0)
            {
                arg->vm1.activeSpriteIdx = -1;
            }
            else
            {
                g_AnmManager->ExecuteAnmIdx(&arg->vm2, local_8->args[0].i + 0x300);
            }
        }
        arg->instructionIndex = arg->instructionIndex + 1;
    }
switchD_004057f2_caseD_4:
    arg->instructionIndex = local_8->args[0].i;
    arg->scriptTime = local_8->args[1].i;
    arg->timersMax[0] = 0;
    arg->pendingCameraShake = 1;
    goto LAB_0040578a;
}

// FUNCTION: TH07 0x00406930
void Stage::SmoothBlendColor(ZunColor param_1)
{
    if ((this->color2).bytes.a == 0)
    {
        this->color2 = param_1;
    }
    else
    {
        (this->color2).bytes.r =
            (u8)((param_1.bytes.r + (u32)(this->color2).bytes.r) >> 1);
        (this->color2).bytes.g =
            (u8)((param_1.bytes.g + (u32)(this->color2).bytes.g) >> 1);
        (this->color2).bytes.b =
            (u8)((param_1.bytes.b + (u32)(this->color2).bytes.b) >> 1);
        (this->color2).bytes.a =
            (u8)((param_1.bytes.a + (u32)(this->color2).bytes.a) >> 1);
    }
}

// FUNCTION: TH07 0x004069d0
u32 Stage::OnDrawHighPrio(Stage *arg)
{
    u32 local_5c;
    u32 local_58;
    u32 local_54;
    ZunColor local_20;
    D3DVIEWPORT8 viewport;

    g_AnmManager->ResetVertexBuffer();
    g_AnmManager->currentVertexShader = 0xff;
    g_AnmManager->currentSprite = NULL;
    g_AnmManager->currentTexture = NULL;
    g_AnmManager->currentColorOp = 0xff;
    g_AnmManager->currentBlendMode = 0xff;
    g_AnmManager->currentZWriteDisable = 0xff;
    g_AnmManager->scriptTicksThisFrame = 0;
    g_AnmManager->renderStateChangesThisFrame = 0;
    g_AnmManager->scriptsExecutedThisFrame = 0;
    g_AnmManager->flushesThisFrame = 0;
    g_AnmManager->currentCameraMode = 0xff;
    if ((g_Supervisor.cfg.opts >> 10 & 1) == 0)
    {
        g_Supervisor.DisableFog();
    }
    g_AnmManager->Flush();
    if (arg->clearBackground != 0)
    {
        viewport.X = 0x20;
        viewport.Y = 0x10;
        viewport.Width = 0x180;
        viewport.Height = 0x1c0;
        g_Supervisor.d3dDevice->SetViewport(&viewport);
        g_Supervisor.d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0xff000000, 1.0f,
                                      0);
        arg->clearBackground = 0;
    }
    g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    if (arg->color2.bytes.a != 0)
    {
        g_AnmManager->colorMulEnabled = 1;
        g_AnmManager->color = arg->color2;
    }
    arg->color2.bytes.a = 0;
    arg->color2.bytes.r = 0x80;
    arg->color2.bytes.g = 0x80;
    arg->color2.bytes.b = 0x80;
    if (arg->spellCardState < 2)
    {
        if (g_Gui.IsStageFinished() == 0)
        {
            if (0 < arg->vm1.activeSpriteIdx)
            {
                g_AnmManager->Draw(&arg->vm1);
                g_AnmManager->Flush();
            }
            if (0 < arg->vm2.activeSpriteIdx)
            {
                g_AnmManager->Draw(&arg->vm2);
                g_AnmManager->Flush();
            }
        }
    }
    if (arg->color == 0)
    {
        g_Supervisor.d3dDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, arg->color, 1.0f,
                                      0);
    }
    else
    {
        g_Supervisor.d3dDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET,
                                      arg->color, 1.0f, 0);
    }
    g_Supervisor.SetRenderState(D3DRS_ZFUNC, 4);
    if (g_AnmManager->colorMulEnabled == 0)
    {
        g_Supervisor.SetRenderState(D3DRS_FOGCOLOR, (arg->fogColor).color);
    }
    else
    {
        local_54 = arg->fogColor.bytes.r * (g_AnmManager->color).bytes.r >> 7;
        if (0xff < local_54)
        {
            local_54 = 0xff;
        }
        local_20.bytes.a = arg->fogColor.bytes.a;
        local_20.bytes.r = (u8)local_54;
        local_58 = arg->fogColor.bytes.g * (g_AnmManager->color).bytes.g >> 7;
        if (0xff < local_58)
        {
            local_58 = 0xff;
        }
        local_20.bytes.g = (u8)local_58;
        local_5c = arg->fogColor.bytes.b * (g_AnmManager->color).bytes.b >> 7;
        if (0xff < local_5c)
        {
            local_5c = 0xff;
        }
        local_20.bytes.b = (u8)local_5c;
        g_Supervisor.SetRenderState(D3DRS_FOGCOLOR, local_20.color);
    }
    g_Supervisor.SetRenderState(D3DRS_FOGSTART, *(DWORD *)&arg->skyFog.nearPlane);
    g_Supervisor.SetRenderState(D3DRS_FOGEND, *(DWORD *)&arg->skyFog.farPlane);
    if ((g_Supervisor.cfg.opts >> 10 & 1) == 0)
    {
        g_Supervisor.EnableFog();
    }
    if (arg->spellCardState < 2)
    {
        if (g_Gui.IsStageFinished() == 0)
        {
            arg->RenderObjects(0);
            arg->RenderObjects(1);
        }
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: TH07 0x00406de0
u32 Stage::OnDrawLowPrio(Stage *arg)
{
    ZunRect local_1c;

    if (arg->spellCardState < 2)
    {
        if (g_Gui.IsStageFinished() == 0)
        {
            arg->RenderObjects(2);
            arg->RenderObjects(3);
            if ((g_Supervisor.cfg.opts >> 10 & 1) == 0)
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
                g_AnmManager->Flush();
                g_Supervisor.SetRenderState(D3DRS_ZFUNC, 8);
                if ((g_Supervisor.cfg.opts >> 10 & 1) == 0)
                {
                    g_Supervisor.SetRenderState(D3DRS_FOGENABLE, 0);
                }
                ScreenEffect::DrawSquare(
                    &local_1c, (arg->ticksSinceSpellcardStarted * 0xff) / 60 << 0x18);
            }
        }
    }
    g_AnmManager->Flush();
    g_Supervisor.SetRenderState(D3DRS_ZFUNC, 8);
    if ((g_Supervisor.cfg.opts >> 10 & 1) == 0)
    {
        g_Supervisor.DisableFog();
    }
    if (0 < arg->spellCardState)
    {
        for (i32 i = 0; i < arg->numSpellcardVms; i++)
        {
            g_AnmManager->Draw(arg->spellcardVms + i);
            g_AnmManager->Flush();
        }
    }
    g_AnmManager->currentCameraMode = 0;
    arg->SetupCameraStageBackground();
    g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    g_Supervisor.SetRenderState(D3DRS_FOGSTART, 0x447a0000);
    g_Supervisor.SetRenderState(D3DRS_FOGEND, 0x44fa0000);
    if (arg->isDarkening == 0)
    {
        g_AnmManager->colorMulEnabled = 0;
        g_AnmManager->color.color = 0x80808080;
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
    arg->position.x = 0.0f;
    arg->position.y = 0.0f;
    arg->position.z = 0.0f;
    arg->spellCardState = 0;
    arg->skyFogInterpDuration = 0;
    switch (g_GameManager.currentStage)
    {
    case 1:

        if (g_AnmManager->LoadAnms(5, "data/stg1bg.anm", 0x300) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 2:

        if (g_AnmManager->LoadAnms(5, "data/stg2bg.anm", 0x300) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 3:

        if (g_AnmManager->LoadAnms(5, "data/stg3bg.anm", 0x300) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 4:

        if (g_AnmManager->LoadAnms(5, "data/stg4bg.anm", 0x300) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }

        if (g_AnmManager->LoadAnms(6, "data/stg4bg2.anm", 0x310) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }

        if (g_AnmManager->LoadAnms(7, "data/stg4bg3.anm", 800) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }

        if (g_AnmManager->LoadAnms(8, "data/stg4bg4.anm", 0x330) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }

        if (g_AnmManager->LoadAnms(9, "data/stg4bg5.anm", 0x340) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 5:

        if (g_AnmManager->LoadAnms(5, "data/stg5bg.anm", 0x300) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 6:

        if (g_AnmManager->LoadAnms(5, "data/stg6bg.anm", 0x300) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 7:

        if (g_AnmManager->LoadAnms(5, "data/stg7bg.anm", 0x300) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
        break;
    case 8:

        if (g_AnmManager->LoadAnms(5, "data/stg8bg.anm", 0x300) != ZUN_SUCCESS)
        {
            return ZUN_ERROR;
        }
    }
    if (arg->LoadStageData(g_StageFiles[g_GameManager.currentStage]) ==
        ZUN_SUCCESS)
    {
        (arg->fogColor).color = 0xff000000;
        arg->skyFog.nearPlane = 200.0f;
        arg->skyFog.farPlane = 500.0f;
        arg->camPos.x = 0.0f;
        arg->camPos.y = 0.0f;
        arg->camPos.z = 1000.0f;
        arg->camLookAt.x = 0.0f;
        arg->camLookAt.y = 0.0f;
        arg->camLookAt.z = 0.0f;
        arg->camUp.x = 0.0f;
        arg->camUp.y = 1.0f;
        arg->camUp.z = 0.0f;
        arg->fov = ZUN_PI / 6.0f;
        arg->camPosEnd = arg->camPos;
        arg->camPosStart = arg->camPos;
        for (i = 0; i < 4; i++)
        {
            arg->timersMax[i] = 0;
            arg->timers[i] = 0;
        }
        arg->scriptWaitTime = 0;
        return ZUN_SUCCESS;
    }
    else
    {
        return ZUN_ERROR;
    }
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
    if (g_Chain.AddToCalcChain(&g_StageCalcChain, 7) != 0)
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
    if (this->stdData == NULL)
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
                                        quad->anmScript + 0x300);
            quad->vmIndex = vmIdx++;
            quad = (StdRawQuadBasic *)((i32)quad + quad->byteSize);
        }
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x004077f0
ZunResult Stage::UpdateObjects()
{
    AnmVm *vm;
    StdRawQuadBasic *local_1c;
    i32 i;
    i32 local_c;
    StdRawObject *object;

    for (i = 0; i < this->objectsCount; i++)
    {
        object = this->objects[i];
        if ((object->flags & 1) != 0)
        {
            local_c = 0;
            for (local_1c = &object->firstQuad; -1 < local_1c->type;
                 local_1c =
                     (StdRawQuadBasic *)((u8 *)local_1c + local_1c->byteSize))
            {
                vm = &this->quadVms[local_1c->vmIndex];
                if (local_1c->type == 0)
                {
                    g_AnmManager->ExecuteScript(vm);
                }
                else if (local_1c->type == 1)
                {
                    g_AnmManager->ExecuteScript(vm);
                }
                if (vm->currentInstruction != NULL)
                {
                    local_c++;
                }
            }
            if (local_c == 0)
            {
                object->flags &= 0xfe;
            }
        }
    }
    return ZUN_SUCCESS;
}

// FUNCTION: TH07 0x00407900
i32 Stage::RenderObjects(i32 param_1)
{
    ZunColor ZVar2;
    AnmVm *vm;
    f32 fVar4;
    f32 local_9c;
    D3DXVECTOR3 local_98;
    f32 local_8c;
    StdRawQuadBasic *local_84;
    D3DXVECTOR3 local_80;
    D3DXVECTOR3 local_74;
    D3DXVECTOR3 local_68;
    f32 local_5c;
    StdRawObject *local_58;
    D3DXMATRIX local_54;
    i32 local_14;
    StdRawInstance *local_10;

    local_10 = this->objectInstances;
    local_98.x = 0.0f;
    local_98.y = 0.0f;
    local_98.z = 0.0f;
    local_14 = 0xff;
    UpdateCamera();
    g_AnmManager->currentCameraMode = 1;
    D3DXMatrixIdentity(&local_54);
    while (local_10->id >= 0)
    {
        local_58 = this->objects[local_10->id];
        if (local_58->zLevel == param_1)
        {
            local_84 = &local_58->firstQuad;
            local_74.z =
                ((local_58->size).z / 2.0f +
                 (((local_58->pos).z + (local_10->pos).z) - (this->position).z)) -
                this->camPos.z;
            local_74.y =
                ((local_58->size).y / 2.0f +
                 (((local_58->pos).y + (local_10->pos).y) - (this->position).y)) -
                this->camPos.y;
            local_74.x =
                ((local_58->size).x / 2.0f +
                 (((local_58->pos).x + (local_10->pos).x) - (this->position).x)) -
                this->camPos.x;
            if (local_74.x * local_74.x + local_74.y * local_74.y +
                    local_74.z * local_74.z <=
                1690000.0f)
            {
                local_5c = local_74.x * (this->camLookAtDir).x +
                           local_74.y * (this->camLookAtDir).y +
                           local_74.z * (this->camLookAtDir).z;
                local_8c = D3DXVec3Length(&local_58->size) / 2.0f + 880.0f;
                if ((local_5c <= local_8c) && (60.0f <= local_5c))
                {
                    local_58->flags = local_58->flags | 2;
                    while (-1 < local_84->type)
                    {
                        vm = this->quadVms + local_84->vmIndex;
                        if (local_84->type == 0)
                        {
                            vm->pos.x =
                                ((vm->offset).x + (local_84->pos).x + (local_10->pos).x) -
                                (this->position).x;
                            vm->pos.y =
                                ((vm->offset).y + (local_84->pos).y + (local_10->pos).y) -
                                (this->position).y;
                            vm->pos.z =
                                ((vm->offset).z + (local_84->pos).z + (local_10->pos).z) -
                                (this->position).z;
                            if ((local_84->size).x != 0.0f)
                            {
                                vm->scale.x = (local_84->size).x / vm->sprite->widthPx;
                            }
                            if ((local_84->size).y != 0.0f)
                            {
                                vm->scale.y = (local_84->size).y / vm->sprite->heightPx;
                            }
                            if (vm->autoRotate == 2)
                            {
                                local_54.m[3][0] = vm->pos.x;
                                local_54.m[3][1] = vm->pos.y;
                                local_54.m[3][2] = vm->pos.z;
                                D3DXVec3Project(&local_74, &local_98, &g_Supervisor.viewport,
                                                &g_Supervisor.projectionMatrix,
                                                &g_Supervisor.viewMatrix, &local_54);
                                local_68.x = g_Supervisor.viewMatrix.m[0][0];
                                local_68.y = g_Supervisor.viewMatrix.m[0][1];
                                local_68.z = g_Supervisor.viewMatrix.m[0][2];
                                D3DXVec3Normalize(&local_68, &local_68);
                                if ((local_84->size).x == 0.0f)
                                {
                                    local_9c = vm->sprite->widthPx;
                                }
                                else
                                {
                                    local_9c = (local_84->size).x;
                                }
                                local_54.m[3][0] =
                                    local_68.x * local_9c * vm->scale.x + local_54.m[3][0];
                                local_54.m[3][1] =
                                    local_68.y * local_9c * vm->scale.x + local_54.m[3][1];
                                local_54.m[3][2] =
                                    local_68.z * local_9c * vm->scale.x + local_54.m[3][2];
                                D3DXVec3Project(&local_68, &local_98, &g_Supervisor.viewport,
                                                &g_Supervisor.projectionMatrix,
                                                &g_Supervisor.viewMatrix, &local_54);
                                local_80.z = local_68.z - local_74.z;
                                local_80.y = local_68.y - local_74.y;
                                local_80.x = local_68.x - local_74.x;
                                vm->scale.x = D3DXVec3Length(&local_80) / local_9c;
                                vm->scale.y = vm->scale.x;
                                local_80.z = vm->pos.z - this->camPos.z;
                                local_80.y = vm->pos.y - this->camPos.y;
                                local_80.x = vm->pos.x - this->camPos.x;
                                fVar4 = D3DXVec3Length(&local_80);
                                ZVar2 = vm->color;
                                if ((this->skyFog).nearPlane < fVar4)
                                {
                                    fVar4 = ((this->skyFog).nearPlane - fVar4) /
                                            ((this->skyFog).nearPlane - (this->skyFog).farPlane);
                                    if (1.0f <= fVar4)
                                    {
                                        goto skip;
                                    }
                                    vm->color.bytes.b -=
                                        (f32)(i32)((u32)vm->color.bytes.b -
                                                   (u32)(this->fogColor).bytes.b) *
                                        fVar4;
                                    vm->color.bytes.g -=
                                        (f32)(i32)((u32)vm->color.bytes.g -
                                                   (u32)(this->fogColor).bytes.g) *
                                        fVar4;
                                    vm->color.bytes.r -=
                                        (f32)(i32)((u32)vm->color.bytes.r -
                                                   (u32)(this->fogColor).bytes.r) *
                                        fVar4;
                                    vm->color.bytes.a =
                                        (u8)((1.0f - fVar4) * (f32)vm->color.bytes.a);
                                }
                                vm->pos = local_74;
                                if ((0.0f <= vm->pos.z) && (vm->pos.z <= 1.0f))
                                {
                                    if (local_14 != 0)
                                    {
                                        if ((g_Supervisor.cfg.opts >> 10 & 1) == 0)
                                        {
                                            g_Supervisor.DisableFog();
                                        }
                                        local_14 = 0;
                                    }
                                    g_AnmManager->DrawFacingCamera(vm);
                                }
                                vm->color = ZVar2;
                            }
                            else
                            {
                                if (((g_Supervisor.cfg.opts >> 10 & 1) == 0) &&
                                    (local_14 != 1))
                                {
                                    if ((g_Supervisor.cfg.opts >> 10 & 1) == 0)
                                    {
                                        g_Supervisor.EnableFog();
                                    }
                                    local_14 = 1;
                                }
                                g_AnmManager->Draw3(vm);
                            }
                        }
                        local_84 = (StdRawQuadBasic *)((u8 *)local_84 + local_84->byteSize);
                    }
                }
            }
        }
    skip:
        local_10 = local_10 + 1;
    };
    return 0;
}

// FUNCTION: TH07 0x00408180
void Stage::SetupCameraStageBackground()
{
    D3DXVECTOR3 eyeVec;
    D3DXVECTOR3 atVec;
    D3DXVECTOR3 upVec;

    eyeVec.z = (g_Supervisor.viewport.Height / 2.0f) / tanf(ZUN_PI / 20.0f);
    upVec.x = 0.0f;
    upVec.y = -1.0f;
    upVec.z = 0.0f;
    atVec.z = 0.0f;
    eyeVec.x = (f32)g_Supervisor.viewport.Width / 2.0f;
    eyeVec.y = (f32)g_Supervisor.viewport.Height / 2.0f;
    atVec.x = (f32)g_Supervisor.viewport.Width / 2.0f;
    atVec.y = (f32)g_Supervisor.viewport.Height / 2.0f;
    D3DXMatrixLookAtLH(&g_Supervisor.viewMatrix, &eyeVec, &atVec, &upVec);
    D3DXMatrixPerspectiveFovLH(
        &g_Supervisor.projectionMatrix, ZUN_PI / 10.0f,
        (f32)g_Supervisor.viewport.Width / (f32)g_Supervisor.viewport.Height,
        1.0f, 10000.0f);
    g_Supervisor.d3dDevice->SetTransform(D3DTS_VIEW, &g_Supervisor.viewMatrix);
    g_Supervisor.d3dDevice->SetTransform(D3DTS_PROJECTION,
                                         &g_Supervisor.projectionMatrix);
}

// FUNCTION: TH07 0x004082b0
void Stage::UpdateCamera()
{
    D3DXVECTOR3 local_10;

    local_10.z = this->camLookAt.z + this->camPos.z;
    local_10.y = this->camLookAt.y + this->camPos.y;
    local_10.x = this->camLookAt.x + this->camPos.x;
    D3DXMatrixLookAtLH(&g_Supervisor.viewMatrix, &this->camPos, &local_10,
                       &this->camUp);
    D3DXMatrixPerspectiveFovLH(&g_Supervisor.projectionMatrix, this->fov,
                               (f32)g_Supervisor.viewport.Width /
                                   (f32)g_Supervisor.viewport.Height,
                               30.0f, 1800.0f);
    g_Supervisor.d3dDevice->SetTransform(D3DTS_VIEW, &g_Supervisor.viewMatrix);
    g_Supervisor.d3dDevice->SetTransform(D3DTS_PROJECTION,
                                         &g_Supervisor.projectionMatrix);
    D3DXVec3Cross(&this->camRight, &this->camLookAt, &this->camUp);
    D3DXVec3Normalize(&this->camRight, &this->camRight);
}
