#ifdef TARGET_N3DS

#include "macros.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "gfx_3ds.h"
#include "gfx_3ds_menu.h"

#include "gfx_cc.h"
#include "gfx_rendering_api.h"

#define TEXTURE_POOL_SIZE 4096
#define FOG_LUT_SIZE 32

static Gfx3DSMode sCurrentGfx3DSMode = GFX_3DS_MODE_NORMAL;

static DVLB_s* sVShaderDvlb;
static shaderProgram_s sShaderProgram;
static float* sVboBuffer;

extern const u8 shader_shbin[];
extern const u32 shader_shbin_size;

struct ShaderProgram {
    uint32_t shader_id;
    uint32_t program_id;
    uint8_t num_floats;
    struct CCFeatures cc_features;
    bool swap_input;
    C3D_TexEnv texenv0;
    C3D_TexEnv texenv1;
};

static struct ShaderProgram sShaderProgramPool[32];
static uint8_t sShaderProgramPoolSize;

struct FogLut {
    uint32_t id;
    C3D_FogLut lut;
};

static struct FogLut fog_lut[FOG_LUT_SIZE];
static uint8_t fog_lut_size;
static uint8_t current_fog_idx;
static uint32_t fog_color;

static u32 sTexBuf[16 * 1024] __attribute__((aligned(32)));
static C3D_Tex sTexturePool[TEXTURE_POOL_SIZE];
static float sTexturePoolScaleS[TEXTURE_POOL_SIZE];
static float sTexturePoolScaleT[TEXTURE_POOL_SIZE];
static u32 sTextureIndex;
static int sTexUnits[2];

static int sCurTex = 0;
static int sCurShader = 0;

static int sVtxUnitSize = 0;
static int sBufIdx = 0;

static bool sDepthTestOn = false;
static bool sDepthUpdateOn = true;
static bool sDepthDecal = false;
static bool sUseBlend = false;

// calling FrameDrawOn resets viewport
static int viewport_x, viewport_y;
static int viewport_width, viewport_height;
// calling SetViewport resets scissor!
static int scissor_x, scissor_y;
static int scissor_width, scissor_height;
static bool scissor;

static C3D_Mtx modelView, projection;

#ifdef ENABLE_N3DS_3D_MODE
static int sOrigBufIdx;
static float iod; // determined by 3D slider position
static const float focalLen = 0.75f;
static const float fov = 54.0f*M_TAU/360.0f;
#endif

static bool gfx_citro3d_z_is_from_0_to_1(void)
{
    return true;
}

static void gfx_citro3d_vertex_array_set_attribs(struct ShaderProgram *prg)
{
    sVtxUnitSize = prg->num_floats;
}

static void gfx_citro3d_unload_shader(UNUSED struct ShaderProgram *old_prg)
{
}

static GPU_TEVSRC getTevSrc(int input, bool swapInput01)
{
    switch (input)
    {
        case SHADER_0:
            return GPU_CONSTANT;
        case SHADER_INPUT_1:
            return swapInput01 ? GPU_PREVIOUS : GPU_PRIMARY_COLOR;
        case SHADER_INPUT_2:
            return swapInput01 ? GPU_PRIMARY_COLOR : GPU_PREVIOUS;
        case SHADER_INPUT_3:
            return GPU_CONSTANT;
        case SHADER_INPUT_4:
            return GPU_CONSTANT;
        case SHADER_TEXEL0:
        case SHADER_TEXEL0A:
            return GPU_TEXTURE0;
        case SHADER_TEXEL1:
            return GPU_TEXTURE1;
    }
    return GPU_CONSTANT;
}

static void update_tex_env(struct ShaderProgram *prg, bool swap_input)
{
    if (prg->cc_features.num_inputs == 2)
    {
        C3D_TexEnvInit(&prg->texenv1);
        C3D_TexEnvColor(&prg->texenv1, 0);
        C3D_TexEnvFunc(&prg->texenv1, C3D_Both, GPU_REPLACE);
        C3D_TexEnvSrc(&prg->texenv1, C3D_Both, GPU_CONSTANT, 0, 0);
        C3D_TexEnvOpRgb(&prg->texenv1, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
        C3D_TexEnvOpAlpha(&prg->texenv1, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);
    }

    C3D_TexEnvInit(&prg->texenv0);
    C3D_TexEnvColor(&prg->texenv0, 0);
    if (prg->cc_features.opt_alpha && !prg->cc_features.color_alpha_same)
    {
        // RGB first
        if (prg->cc_features.do_single[0])
        {
            C3D_TexEnvFunc(&prg->texenv0, C3D_RGB, GPU_REPLACE);
            C3D_TexEnvSrc(&prg->texenv0, C3D_RGB, getTevSrc(prg->cc_features.c[0][3], swap_input), 0, 0);
            if (prg->cc_features.c[0][3] == SHADER_TEXEL0A)
                C3D_TexEnvOpRgb(&prg->texenv0, GPU_TEVOP_RGB_SRC_ALPHA, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
            else
                C3D_TexEnvOpRgb(&prg->texenv0, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
        }
        else if (prg->cc_features.do_multiply[0])
        {
            C3D_TexEnvFunc(&prg->texenv0, C3D_RGB, GPU_MODULATE);
            C3D_TexEnvSrc(&prg->texenv0, C3D_RGB, getTevSrc(prg->cc_features.c[0][0], swap_input),
                                        getTevSrc(prg->cc_features.c[0][2], swap_input), 0);
            C3D_TexEnvOpRgb(&prg->texenv0,
                prg->cc_features.c[0][0] == SHADER_TEXEL0A ? GPU_TEVOP_RGB_SRC_ALPHA : GPU_TEVOP_RGB_SRC_COLOR,
                prg->cc_features.c[0][2] == SHADER_TEXEL0A ? GPU_TEVOP_RGB_SRC_ALPHA : GPU_TEVOP_RGB_SRC_COLOR,
                GPU_TEVOP_RGB_SRC_COLOR);
        }
        else if (prg->cc_features.do_mix[0])
        {
            C3D_TexEnvFunc(&prg->texenv0, C3D_RGB, GPU_INTERPOLATE);
            C3D_TexEnvSrc(&prg->texenv0, C3D_RGB, getTevSrc(prg->cc_features.c[0][0], swap_input),
                                        getTevSrc(prg->cc_features.c[0][1], swap_input),
                                        getTevSrc(prg->cc_features.c[0][2], swap_input));
            C3D_TexEnvOpRgb(&prg->texenv0,
                prg->cc_features.c[0][0] == SHADER_TEXEL0A ? GPU_TEVOP_RGB_SRC_ALPHA : GPU_TEVOP_RGB_SRC_COLOR,
                prg->cc_features.c[0][1] == SHADER_TEXEL0A ? GPU_TEVOP_RGB_SRC_ALPHA : GPU_TEVOP_RGB_SRC_COLOR,
                prg->cc_features.c[0][2] == SHADER_TEXEL0A ? GPU_TEVOP_RGB_SRC_ALPHA : GPU_TEVOP_RGB_SRC_COLOR);
        }
        // now Alpha
        C3D_TexEnvOpAlpha(&prg->texenv0,  GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);
        if (prg->cc_features.do_single[1])
        {
            C3D_TexEnvFunc(&prg->texenv0, C3D_Alpha, GPU_REPLACE);
            C3D_TexEnvSrc(&prg->texenv0, C3D_Alpha, getTevSrc(prg->cc_features.c[1][3], swap_input), 0, 0);
        }
        else if (prg->cc_features.do_multiply[1])
        {
            C3D_TexEnvFunc(&prg->texenv0, C3D_Alpha, GPU_MODULATE);
            C3D_TexEnvSrc(&prg->texenv0, C3D_Alpha, getTevSrc(prg->cc_features.c[1][0], swap_input),
                                          getTevSrc(prg->cc_features.c[1][2], swap_input), 0);
        }
        else if (prg->cc_features.do_mix[1])
        {
            C3D_TexEnvFunc(&prg->texenv0, C3D_Alpha, GPU_INTERPOLATE);
            C3D_TexEnvSrc(&prg->texenv0, C3D_Alpha, getTevSrc(prg->cc_features.c[1][0], swap_input),
                                          getTevSrc(prg->cc_features.c[1][1], swap_input),
                                          getTevSrc(prg->cc_features.c[1][2], swap_input));
        }
    }
    else
    {
        // RBGA
        C3D_TexEnvOpAlpha(&prg->texenv0, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);
        if (prg->cc_features.do_single[0])
        {
            C3D_TexEnvFunc(&prg->texenv0, C3D_Both, GPU_REPLACE);
            C3D_TexEnvSrc(&prg->texenv0, C3D_Both, getTevSrc(prg->cc_features.c[0][3], swap_input), 0, 0);
            if (prg->cc_features.c[0][3] == SHADER_TEXEL0A)
                C3D_TexEnvOpRgb(&prg->texenv0, GPU_TEVOP_RGB_SRC_ALPHA, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
            else
                C3D_TexEnvOpRgb(&prg->texenv0, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
        }
        else if (prg->cc_features.do_multiply[0])
        {
            C3D_TexEnvFunc(&prg->texenv0, C3D_Both, GPU_MODULATE);
            C3D_TexEnvSrc(&prg->texenv0, C3D_Both, getTevSrc(prg->cc_features.c[0][0], swap_input),
                                         getTevSrc(prg->cc_features.c[0][2], swap_input), 0);
            C3D_TexEnvOpRgb(&prg->texenv0,
                prg->cc_features.c[0][0] == SHADER_TEXEL0A ? GPU_TEVOP_RGB_SRC_ALPHA : GPU_TEVOP_RGB_SRC_COLOR,
                prg->cc_features.c[0][2] == SHADER_TEXEL0A ? GPU_TEVOP_RGB_SRC_ALPHA : GPU_TEVOP_RGB_SRC_COLOR,
                GPU_TEVOP_RGB_SRC_COLOR);
        }
        else if (prg->cc_features.do_mix[0])
        {
            C3D_TexEnvFunc(&prg->texenv0, C3D_Both, GPU_INTERPOLATE);
            C3D_TexEnvSrc(&prg->texenv0, C3D_Both, getTevSrc(prg->cc_features.c[0][0], swap_input),
                                         getTevSrc(prg->cc_features.c[0][1], swap_input),
                                         getTevSrc(prg->cc_features.c[0][2], swap_input));
            C3D_TexEnvOpRgb(&prg->texenv0,
                prg->cc_features.c[0][0] == SHADER_TEXEL0A ? GPU_TEVOP_RGB_SRC_ALPHA : GPU_TEVOP_RGB_SRC_COLOR,
                prg->cc_features.c[0][1] == SHADER_TEXEL0A ? GPU_TEVOP_RGB_SRC_ALPHA : GPU_TEVOP_RGB_SRC_COLOR,
                prg->cc_features.c[0][2] == SHADER_TEXEL0A ? GPU_TEVOP_RGB_SRC_ALPHA : GPU_TEVOP_RGB_SRC_COLOR);
        }
    }
    if (!prg->cc_features.opt_alpha)
    {
        C3D_TexEnvColor(&prg->texenv0, 0xFF000000);
        C3D_TexEnvFunc(&prg->texenv0, C3D_Alpha, GPU_REPLACE);
        C3D_TexEnvSrc(&prg->texenv0, C3D_Alpha, GPU_CONSTANT, 0, 0);
    }

    prg->swap_input = swap_input;
}

static void update_shader(bool swap_input)
{
    struct ShaderProgram *prg = &sShaderProgramPool[sCurShader];

    // only Goddard
    if (prg->swap_input != swap_input)
    {
        update_tex_env(prg, swap_input);
    }

    if (prg->cc_features.num_inputs == 2)
    {
        C3D_SetTexEnv(0, &prg->texenv1);
        C3D_SetTexEnv(1, &prg->texenv0);
    } else {
        C3D_SetTexEnv(0, &prg->texenv0);
        C3D_TexEnvInit(C3D_GetTexEnv(1));
    }

    if (prg->cc_features.opt_fog)
    {
        C3D_FogGasMode(GPU_FOG, GPU_PLAIN_DENSITY, true);
        C3D_FogColor(fog_color);
        C3D_FogLutBind(&fog_lut[current_fog_idx].lut);
    } else {
        C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
    }

    if (prg->cc_features.opt_texture_edge && prg->cc_features.opt_alpha)
        C3D_AlphaTest(true, GPU_GREATER, 77);
    else
        C3D_AlphaTest(true, GPU_GREATER, 0);
}

static void gfx_citro3d_load_shader(struct ShaderProgram *new_prg)
{
    sCurShader = new_prg->program_id;
    gfx_citro3d_vertex_array_set_attribs(new_prg);

    update_shader(false);
}

static struct ShaderProgram *gfx_citro3d_create_and_load_new_shader(uint32_t shader_id)
{
    int id = sShaderProgramPoolSize;
    struct ShaderProgram *prg = &sShaderProgramPool[sShaderProgramPoolSize++];

    prg->program_id = id;

    prg->shader_id = shader_id;
    gfx_cc_get_features(shader_id, &prg->cc_features);

    update_tex_env(prg, false);

    prg->num_floats = 4; // vertex position (xyzw)

    if (prg->cc_features.used_textures[0] || prg->cc_features.used_textures[1])
    {
        prg->num_floats += 2;
    }
    prg->num_floats += prg->cc_features.num_inputs * (prg->cc_features.opt_alpha ? 4 : 3);

    gfx_citro3d_load_shader(prg);

    return prg;
}

static struct ShaderProgram *gfx_citro3d_lookup_shader(uint32_t shader_id)
{
    for (size_t i = 0; i < sShaderProgramPoolSize; i++)
    {
        if (sShaderProgramPool[i].shader_id == shader_id)
        {
            return &sShaderProgramPool[i];
        }
    }
    return NULL;
}

static void gfx_citro3d_shader_get_info(struct ShaderProgram *prg, uint8_t *num_inputs, bool used_textures[2])
{
    *num_inputs = prg->cc_features.num_inputs;
    used_textures[0] = prg->cc_features.used_textures[0];
    used_textures[1] = prg->cc_features.used_textures[1];
}

static uint32_t gfx_citro3d_new_texture(void)
{
    if (sTextureIndex == TEXTURE_POOL_SIZE)
    {
        printf("Out of textures!\n");
        return 0;
    }
    return sTextureIndex++;
}

static void gfx_citro3d_select_texture(int tile, uint32_t texture_id)
{
    C3D_TexBind(tile, &sTexturePool[texture_id]);
    sCurTex = texture_id;
    sTexUnits[tile] = texture_id;
}

static int sTileOrder[] =
{
    0,  1,   4,  5,
    2,  3,   6,  7,

    8,  9,  12, 13,
    10, 11, 14, 15
};

static void performTexSwizzle(const u8* src, u32* dst, u32 w, u32 h)
{
    int offs = 0;
    for (u32 y = 0; y < h; y += 8)
    {
        for (u32 x = 0; x < w; x += 8)
        {
            for (int i = 0; i < 64; i++)
            {
                int x2 = i & 7;
                int y2 = i >> 3;
                int pos = sTileOrder[(x2 & 3) + ((y2 & 3) << 2)] + ((x2 >> 2) << 4) + ((y2 >> 2) << 5);
                u32 c = ((const u32*)src)[(y + y2) * w + x + x2];
                dst[offs + pos] = ((c & 0xFF) << 24) | (((c >> 8) & 0xFF) << 16) | (((c >> 16) & 0xFF) << 8) | (c >> 24);
            }
            dst += 64;
        }
    }
}

static void gfx_citro3d_upload_texture(const uint8_t *rgba32_buf, int width, int height)
{
    if (width < 8 || height < 8 || (width & (width - 1)) || (height & (height - 1)))
    {
        u32 newWidth = width < 8 ? 8 : (1 << (32 - __builtin_clz(width - 1)));
        u32 newHeight = height < 8 ? 8 : (1 << (32 - __builtin_clz(height - 1)));
        if (newWidth * newHeight * 4 > sizeof(sTexBuf))
        {
            printf("Tex buffer overflow!\n");
            return;
        }
        int offs = 0;
        for (u32 y = 0; y < newHeight; y += 8)
        {
            for (u32 x = 0; x < newWidth; x += 8)
            {
                for (int i = 0; i < 64; i++)
                {
                    int x2 = i % 8;
                    int y2 = i / 8;

                    int realX = x + x2;
                    if (realX >= width)
                        realX -= width;

                    int realY = y + y2;
                    if (realY >= height)
                        realY -= height;

                    int pos = sTileOrder[x2 % 4 + y2 % 4 * 4] + 16 * (x2 / 4) + 32 * (y2 / 4);
                    u32 c = ((u32*)rgba32_buf)[realY * width + realX];
                    ((u32*)sTexBuf)[offs + pos] = ((c & 0xFF) << 24) | (((c >> 8) & 0xFF) << 16) | (((c >> 16) & 0xFF) << 8) | (c >> 24);
                }
                offs += 64;
            }
        }
        sTexturePoolScaleS[sCurTex] = width / (float)newWidth;
        sTexturePoolScaleT[sCurTex] = height / (float)newHeight;
        width = newWidth;
        height = newHeight;
    }
    else
    {
        sTexturePoolScaleS[sCurTex] = 1.f;
        sTexturePoolScaleT[sCurTex] = 1.f;
        performTexSwizzle(rgba32_buf, sTexBuf, width, height);
    }
    C3D_TexInit(&sTexturePool[sCurTex], width, height, GPU_RGBA8);
    C3D_TexUpload(&sTexturePool[sCurTex], sTexBuf);
    C3D_TexFlush(&sTexturePool[sCurTex]);
}

static uint32_t gfx_cm_to_opengl(uint32_t val)
{
    if (val & G_TX_CLAMP)
        return GPU_CLAMP_TO_EDGE;
    return (val & G_TX_MIRROR) ? GPU_MIRRORED_REPEAT : GPU_REPEAT;
}

static void gfx_citro3d_set_sampler_parameters(int tile, bool linear_filter, uint32_t cms, uint32_t cmt)
{
    C3D_TexSetFilter(&sTexturePool[sTexUnits[tile]], linear_filter ? GPU_LINEAR : GPU_NEAREST, linear_filter ? GPU_LINEAR : GPU_NEAREST);
    C3D_TexSetWrap(&sTexturePool[sTexUnits[tile]], gfx_cm_to_opengl(cms), gfx_cm_to_opengl(cmt));
}

static void update_depth()
{
    C3D_DepthTest(sDepthTestOn, GPU_LEQUAL, sDepthUpdateOn ? GPU_WRITE_ALL : GPU_WRITE_COLOR);
    C3D_DepthMap(true, -1.0f, sDepthDecal ? -0.001f : 0);
}

static void gfx_citro3d_set_depth_test(bool depth_test)
{
    sDepthTestOn = depth_test;
    update_depth();
}

static void gfx_citro3d_set_depth_mask(bool z_upd)
{
    sDepthUpdateOn = z_upd;
    update_depth();
}

static void gfx_citro3d_set_zmode_decal(bool zmode_decal)
{
    sDepthDecal = zmode_decal;
    update_depth();
}

static void gfx_citro3d_set_viewport(int x, int y, int width, int height)
{
    if (gGfx3DSMode == GFX_3DS_MODE_AA_22 || gGfx3DSMode == GFX_3DS_MODE_WIDE_AA_12)
    {
        viewport_x = x * 2;
        viewport_y = y * 2;
        viewport_width = width * 2;
        viewport_height = height * 2;
    }
    else if (gGfx3DSMode == GFX_3DS_MODE_WIDE)
    {
        viewport_x = x * 2;
        viewport_y = y;
        viewport_width = width * 2;
        viewport_height = height;
    }
    else // gGfx3DSMode == GFX_3DS_MODE_NORMAL
    {
        viewport_x = x;
        viewport_y = y;
        viewport_width = width;
        viewport_height = height;
    }
}

static void gfx_citro3d_set_scissor(int x, int y, int width, int height)
{
    scissor = true;
    if (gGfx3DSMode == GFX_3DS_MODE_AA_22 || gGfx3DSMode == GFX_3DS_MODE_WIDE_AA_12)
    {
        scissor_x = x * 2;
        scissor_y = y * 2;
        scissor_width = (x + width) * 2;
        scissor_height = (y + height) * 2;
    }
    else if (gGfx3DSMode == GFX_3DS_MODE_WIDE)
    {
        scissor_x = x * 2;
        scissor_y = y;
        scissor_width = (x + width) * 2;
        scissor_height = y + height;
    }
    else // gGfx3DSMode == GFX_3DS_MODE_NORMAL
    {
        scissor_x = x;
        scissor_y = y;
        scissor_width = x + width;
        scissor_height = y + height;
    }
}

static void applyBlend()
{
    if (sUseBlend)
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
    else
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE, GPU_ZERO);
}

static void gfx_citro3d_set_use_alpha(bool use_alpha)
{
    sUseBlend = use_alpha;
    applyBlend();
}

static u32 vec4ToU32Color(float r, float g, float b, float a)
{
    int r2 = r * 255;
    if (r2 < 0)
        r2 = 0;
    else if (r2 > 255)
        r2 = 255;
    int g2 = g * 255;
    if (g2 < 0)
        g2 = 0;
    else if (g2 > 255)
        g2 = 255;
    int b2 = b * 255;
    if (b2 < 0)
        b2 = 0;
    else if (b2 > 255)
        b2= 255;
    int a2 = a * 255;
    if (a2 < 0)
        a2 = 0;
    else if (a2 > 255)
        a2 = 255;
    return (a2 << 24) | (b2 << 16) | (g2 << 8) | r2;
}



static void renderTwoColorTris(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris)
{
    int offset = 0;
    float* dst = &((float*)sVboBuffer)[sBufIdx * VERTEX_SHADER_SIZE];
    bool hasTex = sShaderProgramPool[sCurShader].cc_features.used_textures[0] || sShaderProgramPool[sCurShader].cc_features.used_textures[1];
    bool hasColor = sShaderProgramPool[sCurShader].cc_features.num_inputs > 0;
    bool hasAlpha = sShaderProgramPool[sCurShader].cc_features.opt_alpha;
    if (sShaderProgramPool[sCurShader].cc_features.opt_fog)
        C3D_TexEnvColor(C3D_GetTexEnv(2), vec4ToU32Color(buf_vbo[hasTex ? 6 : 4], buf_vbo[hasTex ? 7 : 5], buf_vbo[hasTex ? 8 : 6], buf_vbo[hasTex ? 9 : 7]));
    u32 firstColor0, firstColor1;
    bool color0Constant = true;
    bool color1Constant = true;
    //determine which color is constant over all vertices
    for (u32 i = 0; i < buf_vbo_num_tris * 3 && color0Constant && color1Constant; i++)
    {
        int vtxOffs = 4;
        if (hasTex)
            vtxOffs += 2;
        u32 color0 = vec4ToU32Color(
            buf_vbo[offset + vtxOffs],
            buf_vbo[offset + vtxOffs + 1],
            buf_vbo[offset + vtxOffs + 2],
            hasAlpha ? buf_vbo[offset + vtxOffs + 3] : 1.0f);
        vtxOffs += hasAlpha ? 4 : 3;
        u32 color1 = vec4ToU32Color(
            buf_vbo[offset + vtxOffs],
            buf_vbo[offset + vtxOffs + 1],
            buf_vbo[offset + vtxOffs + 2],
            hasAlpha ? buf_vbo[offset + vtxOffs + 3] : 1.0f);
        if (i == 0)
        {
            firstColor0 = color0;
            firstColor1 = color1;
        }
        else
        {
            if (firstColor0 != color0)
                color0Constant = false;
            if (firstColor1 != color1)
                color1Constant = false;
        }
        offset += sVtxUnitSize;
    }
    offset = 0;
    update_shader(!color1Constant);
    C3D_TexEnvColor(C3D_GetTexEnv(0), color1Constant ? firstColor1 : firstColor0);
    for (u32 i = 0; i < 3 * buf_vbo_num_tris; i++)
    {
        *dst++ = buf_vbo[offset + 0];
        *dst++ = buf_vbo[offset + 1];
        *dst++ = buf_vbo[offset + 2];
        *dst++ = buf_vbo[offset + 3];
        int vtxOffs = 4;
        if (hasTex)
        {
            *dst++ = buf_vbo[offset + vtxOffs++] * sTexturePoolScaleS[sCurTex];
            *dst++ = 1 - (buf_vbo[offset + vtxOffs++] * sTexturePoolScaleT[sCurTex]);
        }
        else
        {
            *dst++ = 0;
            *dst++ = 0;
        }
        if (color0Constant)
            vtxOffs += hasAlpha ? 4 : 3;
        if (hasColor)
        {
            *dst++ = buf_vbo[offset + vtxOffs++];
            *dst++ = buf_vbo[offset + vtxOffs++];
            *dst++ = buf_vbo[offset + vtxOffs++];
            *dst++ = hasAlpha ? buf_vbo[offset + vtxOffs++] : 1.0f;
        }
        else
        {
            *dst++ = 1.0f;
            *dst++ = 1.0f;
            *dst++ = 1.0f;
            *dst++ = 1.0f;
        }

        offset += sVtxUnitSize;
    }

    C3D_DrawArrays(GPU_TRIANGLES, sBufIdx, buf_vbo_num_tris * 3);
    sBufIdx += buf_vbo_num_tris * 3;
}

static void gfx_citro3d_draw_triangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris)
{
    if (sBufIdx * VERTEX_SHADER_SIZE > 1 * 1024 * 1024 / 4)
    {
        printf("Vertex buffer full!\n");
        return;
    }

    if (sShaderProgramPool[sCurShader].cc_features.num_inputs > 1)
    {
        renderTwoColorTris(buf_vbo, buf_vbo_len, buf_vbo_num_tris);
        return;
    }

    int offset = 0;
    float* dst = &((float*)sVboBuffer)[sBufIdx * VERTEX_SHADER_SIZE];
    bool hasTex = sShaderProgramPool[sCurShader].cc_features.used_textures[0] || sShaderProgramPool[sCurShader].cc_features.used_textures[1];
    bool hasColor = sShaderProgramPool[sCurShader].cc_features.num_inputs > 0;
    bool hasAlpha = sShaderProgramPool[sCurShader].cc_features.opt_alpha;
    for (u32 i = 0; i < 3 * buf_vbo_num_tris; i++)
    {
        *dst++ = buf_vbo[offset + 0];
        *dst++ = buf_vbo[offset + 1];
        *dst++ = buf_vbo[offset + 2];
        *dst++ = buf_vbo[offset + 3];
        int vtxOffs = 4;
        if (hasTex)
        {
            *dst++ = buf_vbo[offset + vtxOffs++] * sTexturePoolScaleS[sCurTex];
            *dst++ = 1 - (buf_vbo[offset + vtxOffs++] * sTexturePoolScaleT[sCurTex]);
        }
        else
        {
            *dst++ = 0;
            *dst++ = 0;
        }
        if (hasColor)
        {
            *dst++ = buf_vbo[offset + vtxOffs++];
            *dst++ = buf_vbo[offset + vtxOffs++];
            *dst++ = buf_vbo[offset + vtxOffs++];
            *dst++ = hasAlpha ? buf_vbo[offset + vtxOffs++] : 1.0f;
        }
        else
        {
            *dst++ = 1.0f;
            *dst++ = 1.0f;
            *dst++ = 1.0f;
            *dst++ = 1.0f;
        }

        offset += sVtxUnitSize;
    }

    C3D_DrawArrays(GPU_TRIANGLES, sBufIdx, buf_vbo_num_tris * 3);
    sBufIdx += buf_vbo_num_tris * 3;
}

#ifdef ENABLE_N3DS_3D_MODE
static bool sIs2D;
static void gfx_citro3d_is_2d(bool is_2d)
{
    sIs2D = is_2d;
}
#endif

void gfx_citro3d_frame_draw_on(C3D_RenderTarget* target)
{
    target->used = true;
    C3D_SetFrameBuf(&target->frameBuf);
    C3D_SetViewport(viewport_y, viewport_x, viewport_height, viewport_width);
    if (scissor)
        C3D_SetScissor(GPU_SCISSOR_NORMAL, scissor_y, scissor_x, scissor_height, scissor_width);
}

static void gfx_citro3d_draw_triangles_helper(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris)
{
#ifdef ENABLE_N3DS_3D_MODE
    Mtx_Identity(&projection);
    if ((gGfx3DSMode == GFX_3DS_MODE_NORMAL || gGfx3DSMode == GFX_3DS_MODE_AA_22) && gSliderLevel > 0.0f)
    {
        sOrigBufIdx = sBufIdx;
        iod = gSliderLevel / 10.0f;

        if (!sIs2D)
        {
            projection.r[1].z = -iod * (1.0f + iod); // left/right
        }
    }
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
#endif
    // left screen
    gfx_citro3d_frame_draw_on(gTarget);
    gfx_citro3d_draw_triangles(buf_vbo, buf_vbo_len, buf_vbo_num_tris);
#ifdef ENABLE_N3DS_3D_MODE
    if ((gGfx3DSMode == GFX_3DS_MODE_NORMAL || gGfx3DSMode == GFX_3DS_MODE_AA_22) && gSliderLevel > 0.0f)
    {
        // restore buffer index
        sBufIdx = sOrigBufIdx;

        if (!sIs2D)
        {
            projection.r[1].z = iod * (1.0f + iod); // left/right
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
        }
        // draw right screen
        gfx_citro3d_frame_draw_on(gTargetRight);
        gfx_citro3d_draw_triangles(buf_vbo, buf_vbo_len, buf_vbo_num_tris);
    }
#endif
}

static void gfx_citro3d_init(void)
{
    sVShaderDvlb = DVLB_ParseFile((u32*)shader_shbin, shader_shbin_size);
    shaderProgramInit(&sShaderProgram);
    shaderProgramSetVsh(&sShaderProgram, &sVShaderDvlb->DVLE[0]);
    C3D_BindProgram(&sShaderProgram);

    uLoc_projection = shaderInstanceGetUniformLocation((&sShaderProgram)->vertexShader, "projection");
    uLoc_modelView = shaderInstanceGetUniformLocation((&sShaderProgram)->vertexShader, "modelView");

    // Configure attributes for use with the vertex shader
    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 4); // v0=position
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord
    AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 4); // v2=color

    // Create 1MB VBO (vertex buffer object)
    sVboBuffer = linearAlloc(1 * 1024 * 1024);

    // Configure buffers
    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, sVboBuffer, VERTEX_SHADER_SIZE * 4, 3, 0x210);

    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthMap(true, -1.0f, 0);
    C3D_DepthTest(false, GPU_LEQUAL, GPU_WRITE_ALL);
    C3D_AlphaTest(true, GPU_GREATER, 0x00);

    C3D_FrameRate(30);
}

static void gfx_citro3d_start_frame(void)
{
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    sBufIdx = 0;
    scissor = false;
    // reset viewport if video mode changed
    if (gGfx3DSMode != sCurrentGfx3DSMode)
    {
        gfx_citro3d_set_viewport(0, 0, 400, 240);
        sCurrentGfx3DSMode = gGfx3DSMode;
    }

    C3D_RenderTargetClear(gTarget, C3D_CLEAR_ALL, 0x000000FF, 0xFFFFFFFF);
    C3D_RenderTargetClear(gTargetBottom, C3D_CLEAR_ALL, 0x000000FF, 0xFFFFFFFF);
#ifdef ENABLE_N3DS_3D_MODE
    if (gGfx3DSMode == GFX_3DS_MODE_NORMAL || gGfx3DSMode == GFX_3DS_MODE_AA_22)
        C3D_RenderTargetClear(gTargetRight, C3D_CLEAR_ALL, 0x000000FF, 0xFFFFFFFF);
#endif

    // reset model
    Mtx_Identity(&modelView);
    // 3DS screen is rotated 90 degrees
    Mtx_RotateZ(&modelView, 0.75f*M_TAU, false);
    // reset projection
    Mtx_Identity(&projection);
    // set uniforms
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &modelView);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
}

static void gfx_citro3d_on_resize(void)
{
}

static void gfx_citro3d_end_frame(void)
{
    // TOOD: draw the minimap here
    gfx_3ds_menu_draw(sVboBuffer, sBufIdx, gShowConfigMenu);
    // set the texenv back
    update_shader(false);

    C3D_FrameEnd(0);
}

static void gfx_citro3d_finish_render(void)
{
}

static void gfx_citro3d_set_fog(uint16_t from, uint16_t to)
{
    // dumb enough
    uint32_t id = (from << 16) | to;
    // current already loaded
    if (fog_lut[current_fog_idx].id == id)
        return;
    // lut already calculated
    for (int i = 0; i < fog_lut_size; i++)
    {
        if (fog_lut[i].id == id)
        {
            current_fog_idx = i;
            return;
        }
    }
    // new lut required
    if (fog_lut_size == FOG_LUT_SIZE)
    {
        printf("Fog exhausted!\n");
        return;
    }

    current_fog_idx = fog_lut_size++;
    (&fog_lut[current_fog_idx])->id = id;

    // FIXME: The near/far factors are personal preference
    // BOB:  6400, 59392 => 0.16, 116
    // JRB:  1280, 64512 => 0.80, 126
    FogLut_Exp(&fog_lut[current_fog_idx].lut, 0.05f, 1.5f, 1024 / (float)from, ((float)to) / 512);
}

static void gfx_citro3d_set_fog_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    fog_color = (a << 24) | (b << 16) | (g << 8) | r;
}

struct GfxRenderingAPI gfx_citro3d_api = {
    gfx_citro3d_z_is_from_0_to_1,
    gfx_citro3d_unload_shader,
    gfx_citro3d_load_shader,
    gfx_citro3d_create_and_load_new_shader,
    gfx_citro3d_lookup_shader,
    gfx_citro3d_shader_get_info,
    gfx_citro3d_new_texture,
    gfx_citro3d_select_texture,
    gfx_citro3d_upload_texture,
    gfx_citro3d_set_sampler_parameters,
    gfx_citro3d_set_depth_test,
    gfx_citro3d_set_depth_mask,
    gfx_citro3d_set_zmode_decal,
    gfx_citro3d_set_viewport,
    gfx_citro3d_set_scissor,
    gfx_citro3d_set_use_alpha,
    gfx_citro3d_draw_triangles_helper,
    gfx_citro3d_init,
    gfx_citro3d_on_resize,
    gfx_citro3d_start_frame,
    gfx_citro3d_end_frame,
    gfx_citro3d_finish_render,
    gfx_citro3d_set_fog,
    gfx_citro3d_set_fog_color,
#ifdef ENABLE_N3DS_3D_MODE
    gfx_citro3d_is_2d
#endif
};

#endif
