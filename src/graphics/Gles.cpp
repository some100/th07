#include "Gles.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <cstddef>

#include "AnmManager.hpp"
#include "GameWindow.hpp"
#include "Supervisor.hpp"

#ifdef USING_GL
#define GLSL_VERSION "#version 330 core\n"
#define GLSL_PRECISION
#else
#define GLSL_VERSION "#version 300 es\n"
#define GLSL_PRECISION "precision mediump float;\n"
#endif

// clang-format off
const char *vertexShaderSource =
    GLSL_VERSION
    "uniform mat4 u_Model;\n"
    "uniform mat4 u_View;\n"
    "uniform mat4 u_Proj;\n"
    "uniform mat4 u_TextureMatrix;\n"
    "uniform bool u_ScreenSpace;\n"
    "uniform vec4 u_Viewport;\n"
    "\n"
    "layout(location = 0) in vec3 a_Position;\n"
    "layout(location = 1) in vec4 a_Color;\n"
    "layout(location = 2) in vec2 a_TexCoord;\n"
    "\n"
    "out vec4 v_Color;\n"
    "out vec2 v_TexCoord;\n"
    "out float v_FogFragCoord;\n"
    "\n"
    "void main() {\n"
    "    v_Color = a_Color.bgra;\n"
    "    if (u_ScreenSpace) {\n"
    "        float x = (a_Position.x - u_Viewport.x) / u_Viewport.z * 2.0 - 1.0;\n"
    "        float y = 1.0 - (a_Position.y - u_Viewport.y) / u_Viewport.w * 2.0;\n"
    "        gl_Position = vec4(x, y, a_Position.z, 1.0);\n"
    "        v_TexCoord = a_TexCoord;\n"
    "        v_FogFragCoord = a_Position.z;\n"
    "    } else {\n"
    "        vec4 worldPos = u_Model * vec4(a_Position, 1.0);\n"
    "        vec4 viewPos = u_View * worldPos;\n"
    "        gl_Position = u_Proj * viewPos;\n"
    "        v_TexCoord = (u_TextureMatrix * vec4(a_TexCoord, 1.0, 0.0)).xy;\n"
    "        v_FogFragCoord = length(viewPos.xyz);\n"
    "    }\n"
    "}\n";

const char *fragmentShaderSource =
    GLSL_VERSION
    GLSL_PRECISION
    "\n"
    "in vec4 v_Color;\n"
    "in vec2 v_TexCoord;\n"
    "in float v_FogFragCoord;\n"
    "\n"
    "uniform sampler2D u_Texture;\n"
    "uniform bool u_UseTexture;\n"
    "uniform int u_ColorOpRgb;\n"
    "uniform int u_ColorOpAlpha;\n"
    "uniform int u_TexArg;\n"
    "uniform vec4 u_TextureFactor;\n"
    "uniform bool u_AlphaTest;\n"
    "uniform float u_AlphaRef;\n"
    "uniform bool u_FogEnabled;\n"
    "uniform vec4 u_FogColor;\n"
    "uniform float u_FogNear;\n"
    "uniform float u_FogFar;\n"
    "\n"
    "out vec4 FragColor;\n"
    "\n"
    "void main() {\n"
    "    vec4 texColor = vec4(1.0);\n"
    "    if (u_UseTexture) {\n"
    "        texColor = texture(u_Texture, v_TexCoord);\n"
    "    }\n"
    "    \n"
    "    vec4 argColor = v_Color;\n"
    "    if (u_TexArg == 1) { // TEXTURE\n"
    "        argColor = vec4(1.0);\n"
    "    } else if (u_TexArg == 2) { // TFACTOR\n"
    "        argColor = u_TextureFactor;\n"
    "    }\n"
    "    \n"
    "    vec4 finalColor = v_Color;\n"
    "    \n"
    "    if (u_UseTexture) {\n"
    "        if (u_ColorOpRgb == 0) finalColor.rgb = texColor.rgb * argColor.rgb;\n"
    "        else if (u_ColorOpRgb == 1) finalColor.rgb = min(texColor.rgb + argColor.rgb, "
    "vec3(1.0));\n"
    "        else if (u_ColorOpRgb == 2) finalColor.rgb = texColor.rgb;\n"
    "        else if (u_ColorOpRgb == 3) finalColor.rgb = argColor.rgb;\n"
    "        \n"
    "        if (u_ColorOpAlpha == 0) finalColor.a = texColor.a * argColor.a;\n"
    "        else if (u_ColorOpAlpha == 1) finalColor.a = min(texColor.a + argColor.a, 1.0);\n"
    "        else if (u_ColorOpAlpha == 2) finalColor.a = texColor.a;\n"
    "        else if (u_ColorOpAlpha == 3) finalColor.a = argColor.a;\n"
    "    } else {\n"
    "        finalColor = argColor;\n"
    "    }\n"
    "    \n"
    "    if (u_AlphaTest && finalColor.a < u_AlphaRef) {\n"
    "        discard;\n"
    "    }\n"
    "    \n"
    "    if (u_FogEnabled) {\n"
    "        float f = (u_FogFar - v_FogFragCoord) / (u_FogFar - u_FogNear);\n"
    "        f = clamp(f, 0.0, 1.0);\n"
    "        finalColor.rgb = mix(u_FogColor.rgb, finalColor.rgb, f);\n"
    "    }\n"
    "    \n"
    "    FragColor = finalColor;\n"
    "}\n";

const char *blitVSSource =
    GLSL_VERSION
    "out vec2 v_TexCoord;\n"
    "void main() {\n"
    "    float x = float((gl_VertexID & 1) << 2) - 1.0;\n"
    "    float y = float((gl_VertexID & 2) << 1) - 1.0;\n"
    "    v_TexCoord = vec2((x + 1.0) * 0.5, (y + 1.0) * 0.5);\n"
    "    gl_Position = vec4(x, y, 0.0, 1.0);\n"
    "}\n";

const char *blitFSSource =
    GLSL_VERSION
    GLSL_PRECISION
    "in vec2 v_TexCoord;\n"
    "uniform sampler2D u_Texture;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = texture(u_Texture, v_TexCoord);\n"
    "}\n";
// clang-format on

ZunGraphics *GlesGraphics::Init()
{
    GlesGraphics *gfx = new GlesGraphics;

    SDL_GLContext ctx = SDL_GL_CreateContext(g_GameWindow.window);
    if (!ctx)
    {
        delete gfx;
        Supervisor::DebugPrint("gles renderer create failed: %s\n", SDL_GetError());
        return nullptr;
    }
    gfx->ctx = ctx;

    SDL_GL_MakeCurrent(g_GameWindow.window, ctx);

    glGenFramebuffers(1, &gfx->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gfx->fbo);

    glGenTextures(1, &gfx->fboColor);
    glBindTexture(GL_TEXTURE_2D, gfx->fboColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 640, 480, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gfx->fboColor, 0);

    glGenRenderbuffers(1, &gfx->fboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, gfx->fboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 640, 480);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              gfx->fboDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, gfx->fbo);

    RenderVertexInfo unitQuadData[4] = {{{-128.0f, -128.0f, 0.0f}, {0.0f, 0.0f}},
                                        {{128.0f, -128.0f, 0.0f}, {1.0f, 0.0f}},
                                        {{-128.0f, 128.0f, 0.0f}, {0.0f, 1.0f}},
                                        {{128.0f, 128.0f, 0.0f}, {1.0f, 1.0f}}};

    glGenVertexArrays(1, &gfx->unitQuadVao);
    glGenBuffers(1, &gfx->unitQuadVbo);
    glBindVertexArray(gfx->unitQuadVao);
    glBindBuffer(GL_ARRAY_BUFFER, gfx->unitQuadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(unitQuadData), unitQuadData, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertexInfo),
                          (void *)offsetof(RenderVertexInfo, pos));
    glDisableVertexAttribArray(1);
    glVertexAttrib4f(1, 1.0f, 1.0f, 1.0f, 1.0f);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(RenderVertexInfo),
                          (void *)offsetof(RenderVertexInfo, textureUV));
    glBindVertexArray(0);

    if (!SDL_GL_SetSwapInterval(-1) && !SDL_GL_SetSwapInterval(1))
    {
        // technically this isnt fatal we just go into 60 fps later on in gamewindow::render
        Supervisor::DebugPrint("SDL_GL_SetSwapInterval failed: %s\n", SDL_GetError());
    }

    u32 vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    u32 fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (vertexShader == 0 || fragmentShader == 0)
    {
        return nullptr;
    }

    gfx->shaderProgram = glCreateProgram();
    glAttachShader(gfx->shaderProgram, vertexShader);
    glAttachShader(gfx->shaderProgram, fragmentShader);
    glLinkProgram(gfx->shaderProgram);
    glUseProgram(gfx->shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    vertexShader = CompileShader(GL_VERTEX_SHADER, blitVSSource);
    fragmentShader = CompileShader(GL_FRAGMENT_SHADER, blitFSSource);
    if (vertexShader == 0 || fragmentShader == 0)
    {
        return nullptr;
    }

    gfx->blitProgram = glCreateProgram();
    glAttachShader(gfx->blitProgram, vertexShader);
    glAttachShader(gfx->blitProgram, fragmentShader);
    glLinkProgram(gfx->blitProgram);
    glUseProgram(gfx->blitProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    gfx->u_Model = glGetUniformLocation(gfx->shaderProgram, "u_Model");
    gfx->u_View = glGetUniformLocation(gfx->shaderProgram, "u_View");
    gfx->u_Proj = glGetUniformLocation(gfx->shaderProgram, "u_Proj");
    gfx->u_TextureMatrix = glGetUniformLocation(gfx->shaderProgram, "u_TextureMatrix");
    gfx->u_ScreenSpace = glGetUniformLocation(gfx->shaderProgram, "u_ScreenSpace");
    gfx->u_Viewport = glGetUniformLocation(gfx->shaderProgram, "u_Viewport");
    gfx->u_UseTexture = glGetUniformLocation(gfx->shaderProgram, "u_UseTexture");
    gfx->u_Texture = glGetUniformLocation(gfx->shaderProgram, "u_Texture");
    gfx->u_ColorOpRgb = glGetUniformLocation(gfx->shaderProgram, "u_ColorOpRgb");
    gfx->u_ColorOpAlpha = glGetUniformLocation(gfx->shaderProgram, "u_ColorOpAlpha");
    gfx->u_TexArg = glGetUniformLocation(gfx->shaderProgram, "u_TexArg");
    gfx->u_TextureFactor = glGetUniformLocation(gfx->shaderProgram, "u_TextureFactor");
    gfx->u_AlphaTest = glGetUniformLocation(gfx->shaderProgram, "u_AlphaTest");
    gfx->u_AlphaRef = glGetUniformLocation(gfx->shaderProgram, "u_AlphaRef");
    gfx->u_FogEnabled = glGetUniformLocation(gfx->shaderProgram, "u_FogEnabled");
    gfx->u_FogColor = glGetUniformLocation(gfx->shaderProgram, "u_FogColor");
    gfx->u_FogNear = glGetUniformLocation(gfx->shaderProgram, "u_FogNear");
    gfx->u_FogFar = glGetUniformLocation(gfx->shaderProgram, "u_FogFar");
    gfx->u_BlitTexture = glGetUniformLocation(gfx->blitProgram, "u_Texture");

    glUseProgram(gfx->shaderProgram);
    glUniform1i(gfx->u_Texture, 0);

    glUseProgram(gfx->blitProgram);
    glUniform1i(gfx->u_BlitTexture, 0);

    glGenVertexArrays(9, &gfx->vaos[0][0]);
    glGenBuffers(3, gfx->vbos);

    for (i32 i = 0; i < 3; i++)
    {
        glBindBuffer(GL_ARRAY_BUFFER, gfx->vbos[i]);
        glBufferData(GL_ARRAY_BUFFER, VBO_CAPACITY, nullptr, GL_DYNAMIC_DRAW);
    }

    for (i32 i = 0; i < 3; i++)
    {
        glBindVertexArray(gfx->vaos[0][i]);
        glBindBuffer(GL_ARRAY_BUFFER, gfx->vbos[i]);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTex1DiffuseXyzrhw),
                              (void *)offsetof(VertexTex1DiffuseXyzrhw, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexTex1DiffuseXyzrhw),
                              (void *)offsetof(VertexTex1DiffuseXyzrhw, diffuse));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTex1DiffuseXyzrhw),
                              (void *)offsetof(VertexTex1DiffuseXyzrhw, textureUV));

        glBindVertexArray(gfx->vaos[1][i]);
        glBindBuffer(GL_ARRAY_BUFFER, gfx->vbos[i]);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTex1DiffuseXyz),
                              (void *)offsetof(VertexTex1DiffuseXyz, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexTex1DiffuseXyz),
                              (void *)offsetof(VertexTex1DiffuseXyz, diffuse));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTex1DiffuseXyz),
                              (void *)offsetof(VertexTex1DiffuseXyz, textureUV));

        glBindVertexArray(gfx->vaos[2][i]);
        glBindBuffer(GL_ARRAY_BUFFER, gfx->vbos[i]);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexDiffuseXyzrhw),
                              (void *)offsetof(VertexDiffuseXyzrhw, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexDiffuseXyzrhw),
                              (void *)offsetof(VertexDiffuseXyzrhw, diffuse));
        glDisableVertexAttribArray(2);
    }
    glBindVertexArray(0);

    glGenVertexArrays(1, &gfx->blitVao);

    for (i32 i = 0; i < 4; i++)
    {
        gfx->transforms[i].Identity();
    }

    Supervisor::DebugPrint("using gles rendering.\n");

    return gfx;
}

void GlesGraphics::Exit()
{
    SDL_GL_DestroyContext(this->ctx);
}

void GlesGraphics::BeginFrame()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    curVbo = (curVbo + 1) % 3;

    glBindBuffer(GL_ARRAY_BUFFER, vbos[curVbo]);
    glBufferData(GL_ARRAY_BUFFER, VBO_CAPACITY, nullptr, GL_STREAM_DRAW);
    vboOffset = 0;

    stateCache.Invalidate();
}

void GlesGraphics::EndFrame()
{
    Flush();
}

void GlesGraphics::SetFogRange(f32 nearPlane, f32 farPlane)
{
    if (fogNear != nearPlane || fogFar != farPlane)
    {
        fogNear = nearPlane;
        fogFar = farPlane;
        stateCache.dirtyFog = true;
    }
}

void GlesGraphics::SetFogColor(ZunColor color)
{
    if (fogColor.color != color.color)
    {
        fogColor = color;
        stateCache.dirtyFog = true;
    }
}

void GlesGraphics::SetColorOp(TextureOpComponent component, ColorOp op)
{
    if (component == COMPONENT_RGB)
    {
        if (colorOpRgb != op)
        {
            colorOpRgb = op;
            stateCache.dirtyColorOp = true;
        }
    }
    else
    {
        if (colorOpAlpha != op)
        {
            colorOpAlpha = op;
            stateCache.dirtyColorOp = true;
        }
    }
}

void GlesGraphics::SetTextureFactor(ZunColor factor)
{
    if (textureFactor.color != factor.color)
    {
        textureFactor = factor;
        stateCache.dirtyTexFactor = true;
    }
}

void GlesGraphics::SetTextureArg(TextureArg arg)
{
    if (texArg != arg)
    {
        texArg = arg;
        stateCache.dirtyTexArg = true;
    }
}

void GlesGraphics::SetTransformMatrix(TransformMatrix type, const ZunMatrix &matrix)
{
    transforms[type] = matrix;
    stateCache.dirtyMatrix = true;
}

void GlesGraphics::SetTextureFilter()
{
}

void GlesGraphics::GetViewport(ZunViewport &viewport)
{
    viewport = this->viewport;
}

void GlesGraphics::SetViewport(const ZunViewport &viewport)
{
    this->viewport = viewport;
    glViewport(viewport.x, 480 - (viewport.y + viewport.height), viewport.width, viewport.height);
    stateCache.dirtyViewport = true;
}

void GlesGraphics::Enable(Capabilities cap)
{
    switch (cap)
    {
    case CAPS_BLEND:
        if (!blendEnabled)
        {
            glEnable(GL_BLEND);
            blendEnabled = true;
        }
        break;
    case CAPS_DEPTH_TEST:
        if (!depthTestEnabled)
        {
            glEnable(GL_DEPTH_TEST);
            depthTestEnabled = true;
        }
        break;
    case CAPS_ALPHA_TEST:
        if (!alphaTestEnabled)
        {
            alphaTestEnabled = true;
            stateCache.dirtyAlphaTest = true;
        }
        break;
    case CAPS_FOG:
        if (!fogEnabled)
        {
            fogEnabled = true;
            stateCache.dirtyFog = true;
        }
        break;
    }
}

void GlesGraphics::Disable(Capabilities cap)
{
    switch (cap)
    {
    case CAPS_BLEND:
        if (blendEnabled)
        {
            glDisable(GL_BLEND);
            blendEnabled = false;
        }
        break;
    case CAPS_DEPTH_TEST:
        if (depthTestEnabled)
        {
            glDisable(GL_DEPTH_TEST);
            depthTestEnabled = false;
        }
        break;
    case CAPS_ALPHA_TEST:
        if (alphaTestEnabled)
        {
            alphaTestEnabled = false;
            stateCache.dirtyAlphaTest = true;
        }
        break;
    case CAPS_FOG:
        if (fogEnabled)
        {
            fogEnabled = false;
            stateCache.dirtyFog = true;
        }
        break;
    }
}

void GlesGraphics::SetBlendMode(BlendMode srcMode, BlendMode dstMode)
{
    GLenum glSrcMode = GL_SRC_ALPHA;
    switch (srcMode)
    {
    case BLEND_ALPHA:
        glSrcMode = GL_SRC_ALPHA;
        break;
    case BLEND_ONE:
        glSrcMode = GL_ONE;
        break;
    case BLEND_NONE:
        glSrcMode = GL_ONE;
        break;
    }

    GLenum glDstMode = GL_ONE_MINUS_SRC_ALPHA;
    switch (dstMode)
    {
    case BLEND_ALPHA:
        glDstMode = GL_ONE_MINUS_SRC_ALPHA;
        break;
    case BLEND_ONE:
        glDstMode = GL_ONE;
        break;
    case BLEND_NONE:
        glDstMode = GL_ZERO;
        break;
    }
    glBlendFunc(glSrcMode, glDstMode);
}

void GlesGraphics::SetDepthMask(bool enable)
{
    depthMaskEnabled = enable;
    glDepthMask(enable);
}

void GlesGraphics::SetDepthFunc(DepthFunc func)
{
    switch (func)
    {
    case DEPTH_FUNC_LEQUAL:
        glDepthFunc(GL_LEQUAL);
        break;
    case DEPTH_FUNC_ALWAYS:
        glDepthFunc(GL_ALWAYS);
        break;
    }
}

void GlesGraphics::SetClearDepth(f32 depth)
{
#ifdef USING_GL
    glClearDepth(depth);
#else
    glClearDepthf(depth);
#endif
}

void GlesGraphics::SetClearColor(ZunColor color)
{
    clearColor = color;
    glClearColor(color.bytes.r / 255.0f, color.bytes.g / 255.0f, color.bytes.b / 255.0f,
                 color.bytes.a / 255.0f);
}

void GlesGraphics::SetAlphaTestRef(u8 ref)
{
    if (alphaRef != ref)
    {
        alphaRef = ref;
        stateCache.dirtyAlphaTest = true;
    }
}

void GlesGraphics::Clear(u32 clearBits)
{
    GLbitfield bits = 0;
    if (clearBits & CLEAR_COLOR_BUFFER)
    {
        bits |= GL_COLOR_BUFFER_BIT;
    }
    if (clearBits & CLEAR_DEPTH_BUFFER)
    {
        bits |= GL_DEPTH_BUFFER_BIT;
        if (!depthMaskEnabled)
        {
            glDepthMask(GL_TRUE);
        }
    }
    glClear(bits);
    if ((clearBits & CLEAR_DEPTH_BUFFER) && !depthMaskEnabled)
    {
        glDepthMask(GL_FALSE);
    }
}

GfxTextureHandle GlesGraphics::CreateTexture()
{
    GLuint tex;
    glGenTextures(1, &tex);
    return GfxTextureHandle(tex);
}

void GlesGraphics::BindTexture(GfxTextureHandle handle)
{
    glBindTexture(GL_TEXTURE_2D, handle.id);
}

void GlesGraphics::DeleteTexture(GfxTextureHandle handle)
{
    GLuint tex = handle.id;
    glDeleteTextures(1, &tex);
}

void GlesGraphics::SetTextureImage(u32 width, u32 height, PixelFormat fmt, PixelDataType type,
                                   const void *data)
{
    GLenum internalformat;
    GLenum format;
    GLenum datatype;

    switch (fmt)
    {
    case PIXEL_RGB:
        internalformat = GL_RGB8;
        format = GL_RGB;
        break;
    case PIXEL_RGBA:
    default:
        internalformat = GL_RGBA8;
        format = GL_RGBA;
        break;
    }

    switch (type)
    {
    case PIXEL_UNSIGNED_BYTE:
        datatype = GL_UNSIGNED_BYTE;
        break;
    case PIXEL_UNSIGNED_SHORT_5_5_5_1:
        datatype = GL_UNSIGNED_SHORT_5_5_5_1;
        break;
    case PIXEL_UNSIGNED_SHORT_5_6_5:
        datatype = GL_UNSIGNED_SHORT_5_6_5;
        break;
    case PIXEL_UNSIGNED_SHORT_4_4_4_4:
        datatype = GL_UNSIGNED_SHORT_4_4_4_4;
        break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, datatype, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void GlesGraphics::SetTextureSubImage(i32 xoffset, i32 yoffset, i32 width, i32 height,
                                      const void *data)
{
    glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, yoffset, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                    data);
}

void GlesGraphics::ReadPixels(i32 x, i32 y, i32 width, i32 height, void *pixels)
{
    glReadPixels(x, 480 - (y + height), width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    u32 rowSize = width * 4;
    u8 *p = (u8 *)pixels;
    u8 *tempRow = new u8[rowSize];
    for (i32 i = 0; i < height / 2; ++i)
    {
        u8 *top = p + i * rowSize;
        u8 *bottom = p + (height - 1 - i) * rowSize;
        memcpy(tempRow, top, rowSize);
        memcpy(top, bottom, rowSize);
        memcpy(bottom, tempRow, rowSize);
    }
    delete[] tempRow;
}

void GlesGraphics::DrawPrimitive(PrimitiveType type, i32 startVertex, i32 primitiveCount)
{
    i32 vertexCount = 0;
    GLenum glMode = GL_TRIANGLES;

    if (type == PRIM_TRIANGLES)
    {
        vertexCount = primitiveCount * 3;
        glMode = GL_TRIANGLES;
    }
    else if (type == PRIM_TRIANGLE_STRIP)
    {
        vertexCount = primitiveCount + 2;
        glMode = GL_TRIANGLE_STRIP;
    }
    else if (type == PRIM_TRIANGLE_FAN)
    {
        vertexCount = primitiveCount + 2;
        glMode = GL_TRIANGLE_FAN;
    }

    if (stateCache.currentVao != unitQuadVao)
    {
        glBindVertexArray(unitQuadVao);
        stateCache.currentVao = unitQuadVao;
    }

    if (stateCache.currentStride != sizeof(RenderVertexInfo))
    {
        glUniform1i(u_ScreenSpace, false);
        glUniform1i(u_UseTexture, true);
        stateCache.currentStride = sizeof(RenderVertexInfo);
    }

    if (stateCache.dirtyViewport)
    {
        glUniform4f(u_Viewport, (f32)viewport.x, (f32)viewport.y, (f32)viewport.width,
                    (f32)viewport.height);
        stateCache.dirtyViewport = false;
    }

    if (stateCache.dirtyMatrix)
    {
        glUniformMatrix4fv(u_Model, 1, GL_FALSE, (GLfloat *)&transforms[MATRIX_MODEL]);
        glUniformMatrix4fv(u_View, 1, GL_FALSE, (GLfloat *)&transforms[MATRIX_VIEW]);
        glUniformMatrix4fv(u_Proj, 1, GL_FALSE, (GLfloat *)&transforms[MATRIX_PROJECTION]);
        glUniformMatrix4fv(u_TextureMatrix, 1, GL_FALSE, (GLfloat *)&transforms[MATRIX_TEXTURE]);
        stateCache.dirtyMatrix = false;
    }

    if (stateCache.dirtyColorOp)
    {
        glUniform1i(u_ColorOpRgb, colorOpRgb);
        glUniform1i(u_ColorOpAlpha, colorOpAlpha);
        stateCache.dirtyColorOp = false;
    }

    if (stateCache.dirtyTexArg)
    {
        glUniform1i(u_TexArg, texArg);
        stateCache.dirtyTexArg = false;
    }

    if (stateCache.dirtyTexFactor)
    {
        glUniform4f(u_TextureFactor, textureFactor.bytes.r / 255.0f, textureFactor.bytes.g / 255.0f,
                    textureFactor.bytes.b / 255.0f, textureFactor.bytes.a / 255.0f);
        stateCache.dirtyTexFactor = false;
    }

    if (stateCache.dirtyAlphaTest)
    {
        glUniform1i(u_AlphaTest, alphaTestEnabled);
        glUniform1f(u_AlphaRef, alphaRef / 255.0f);
        stateCache.dirtyAlphaTest = false;
    }

    if (stateCache.dirtyFog)
    {
        glUniform1i(u_FogEnabled, fogEnabled);
        glUniform4f(u_FogColor, fogColor.bytes.r / 255.0f, fogColor.bytes.g / 255.0f,
                    fogColor.bytes.b / 255.0f, fogColor.bytes.a / 255.0f);
        glUniform1f(u_FogNear, fogNear);
        glUniform1f(u_FogFar, fogFar);
        stateCache.dirtyFog = false;
    }

    glDrawArrays(glMode, startVertex, vertexCount);
}

void GlesGraphics::DrawPrimitiveUP(PrimitiveType type, i32 primitiveCount, const void *vertexData,
                                   i32 vertexStride)
{
    i32 vertexCount = 0;
    GLenum glMode = GL_TRIANGLES;

    if (type == PRIM_TRIANGLES)
    {
        vertexCount = primitiveCount * 3;
        glMode = GL_TRIANGLES;
    }
    else if (type == PRIM_TRIANGLE_STRIP)
    {
        vertexCount = primitiveCount + 2;
        glMode = GL_TRIANGLE_STRIP;
    }
    else if (type == PRIM_TRIANGLE_FAN)
    {
        vertexCount = primitiveCount + 2;
        glMode = GL_TRIANGLE_FAN;
    }

    GLsizeiptr bytesNeeded = vertexCount * vertexStride;
    vboOffset = ((vboOffset + vertexStride - 1) / vertexStride) * vertexStride;
    GLuint vbo = vbos[curVbo];

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (vboOffset + bytesNeeded > VBO_CAPACITY)
    {
        glBufferData(GL_ARRAY_BUFFER, VBO_CAPACITY, nullptr, GL_STREAM_DRAW);
        vboOffset = 0;
    }
    glBufferSubData(GL_ARRAY_BUFFER, vboOffset, bytesNeeded, vertexData);

    GLint firstVertex = (GLint)(vboOffset / vertexStride);

    bool isScreenSpace = false;
    bool hasTex = false;
    GLuint targetVao = 0;
    switch (vertexStride)
    {
    case sizeof(VertexTex1DiffuseXyzrhw):
        isScreenSpace = true;
        hasTex = true;
        targetVao = vaos[0][curVbo];
        break;
    case sizeof(VertexTex1DiffuseXyz):
        isScreenSpace = false;
        hasTex = true;
        targetVao = vaos[1][curVbo];
        break;
    case sizeof(VertexDiffuseXyzrhw):
        isScreenSpace = true;
        hasTex = false;
        targetVao = vaos[2][curVbo];
        break;
    }

    vboOffset += bytesNeeded;

    if (stateCache.currentVao != targetVao)
    {
        glBindVertexArray(targetVao);
        stateCache.currentVao = targetVao;
    }

    if (stateCache.currentStride != vertexStride)
    {
        glUniform1i(u_ScreenSpace, isScreenSpace);
        glUniform1i(u_UseTexture, hasTex);
        stateCache.currentStride = vertexStride;
    }

    if (stateCache.dirtyViewport)
    {
        glUniform4f(u_Viewport, (f32)viewport.x, (f32)viewport.y, (f32)viewport.width,
                    (f32)viewport.height);
        stateCache.dirtyViewport = false;
    }

    if (stateCache.dirtyMatrix)
    {
        glUniformMatrix4fv(u_Model, 1, GL_FALSE, (GLfloat *)&transforms[MATRIX_MODEL]);
        glUniformMatrix4fv(u_View, 1, GL_FALSE, (GLfloat *)&transforms[MATRIX_VIEW]);
        glUniformMatrix4fv(u_Proj, 1, GL_FALSE, (GLfloat *)&transforms[MATRIX_PROJECTION]);
        glUniformMatrix4fv(u_TextureMatrix, 1, GL_FALSE, (GLfloat *)&transforms[MATRIX_TEXTURE]);
        stateCache.dirtyMatrix = false;
    }

    if (stateCache.dirtyColorOp)
    {
        glUniform1i(u_ColorOpRgb, colorOpRgb);
        glUniform1i(u_ColorOpAlpha, colorOpAlpha);
        stateCache.dirtyColorOp = false;
    }

    if (stateCache.dirtyTexArg)
    {
        glUniform1i(u_TexArg, texArg);
        stateCache.dirtyTexArg = false;
    }

    if (stateCache.dirtyTexFactor)
    {
        glUniform4f(u_TextureFactor, textureFactor.bytes.r / 255.0f, textureFactor.bytes.g / 255.0f,
                    textureFactor.bytes.b / 255.0f, textureFactor.bytes.a / 255.0f);
        stateCache.dirtyTexFactor = false;
    }

    if (stateCache.dirtyAlphaTest)
    {
        glUniform1i(u_AlphaTest, alphaTestEnabled);
        glUniform1f(u_AlphaRef, alphaRef / 255.0f);
        stateCache.dirtyAlphaTest = false;
    }

    if (stateCache.dirtyFog)
    {
        glUniform1i(u_FogEnabled, fogEnabled);
        glUniform4f(u_FogColor, fogColor.bytes.r / 255.0f, fogColor.bytes.g / 255.0f,
                    fogColor.bytes.b / 255.0f, fogColor.bytes.a / 255.0f);
        glUniform1f(u_FogNear, fogNear);
        glUniform1f(u_FogFar, fogFar);
        stateCache.dirtyFog = false;
    }

    glDrawArrays(glMode, firstVertex, vertexCount);
}

void GlesGraphics::SwapBuffers()
{
    i32 drawableWidth, drawableHeight;
    SDL_GetWindowSizeInPixels(g_GameWindow.window, &drawableWidth, &drawableHeight);

#if defined(__APPLE__) && TARGET_OS_IPHONE
    SDL_PropertiesID props = SDL_GetWindowProperties(g_GameWindow.window);
    this->defaultFbo = (GLuint)SDL_GetNumberProperty(
        props, SDL_PROP_WINDOW_UIKIT_OPENGL_FRAMEBUFFER_NUMBER, this->defaultFbo);
#endif

    glBindFramebuffer(GL_READ_FRAMEBUFFER, this->fbo);
#ifndef USING_GL
    const GLenum attachments[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, 2, attachments);
#endif

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->defaultFbo);

    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, drawableWidth, drawableHeight);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // the original game didnt pillarbox but that looks really ugly so im pillarboxing anyways
    f32 targetAspect = 640.0f / 480.0f;
    f32 windowAspect = (f32)drawableWidth / (f32)drawableHeight;

    i32 dstWidth;
    i32 dstHeight;
    i32 dstX;
    i32 dstY;

    if (windowAspect > targetAspect)
    {
        dstHeight = drawableHeight;
        dstWidth = (i32)(dstHeight * targetAspect);
        dstX = (drawableWidth - dstWidth) / 2;
        dstY = 0;
    }
    else
    {
        dstWidth = drawableWidth;
        dstHeight = (i32)(dstWidth / targetAspect);
        dstX = 0;
        dstY = (drawableHeight - dstHeight) / 2;
    }

    glViewport(dstX, dstY, dstWidth, dstHeight);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    glUseProgram(this->blitProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->fboColor);

    glBindVertexArray(this->blitVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

#if defined(__APPLE__) && TARGET_OS_IPHONE
    glBindRenderbuffer(
        GL_RENDERBUFFER,
        SDL_GetNumberProperty(props, SDL_PROP_WINDOW_UIKIT_OPENGL_RENDERBUFFER_NUMBER, 0));
#endif
    SDL_GL_SwapWindow(g_GameWindow.window);

    glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
    glViewport(viewport.x, 480 - (viewport.y + viewport.height), viewport.width, viewport.height);

    if (blendEnabled)
    {
        glEnable(GL_BLEND);
    }
    else
    {
        glDisable(GL_BLEND);
    }
    if (depthTestEnabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(depthMaskEnabled ? GL_TRUE : GL_FALSE);

    glClearColor(clearColor.bytes.r / 255.0f, clearColor.bytes.g / 255.0f,
                 clearColor.bytes.b / 255.0f, clearColor.bytes.a / 255.0f);

    glUseProgram(this->shaderProgram);
    stateCache.Invalidate();
}
