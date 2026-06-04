#ifdef TARGET_GX

#include <malloc.h>
#include <string.h>

#include <ultra64.h>
#include "macros.h" // for UNUSED

#include "gfx_cc.h"
#include "gfx_rendering_api.h"
#include "gfx_gx_wm.h"

#define TEXTURE_POOL_SIZE 4096
#define DEFAULT_FIFO_SIZE 256 * 1024
#define TEXTURE_MEMORY_SIZE 2 * 1024 * 1024

// shaders
struct ShaderProgram {
    uint32_t shader_id;
    uint32_t program_id;
    uint8_t num_floats;
    struct CCFeatures cc_features;
};

static struct ShaderProgram shader_program_pool[32];
static uint8_t shader_program_pool_size;
static uint8_t current_shader;

// textures
static uint16_t texture_index;
static uint8_t texture_units[2];
static uint16_t current_texture;

static GXTexObj texture_pool[TEXTURE_POOL_SIZE];

static uint16_t texture_memory[TEXTURE_MEMORY_SIZE] __attribute__((aligned(32)));
static size_t texture_memory_used;

static unsigned char gp_fifo[DEFAULT_FIFO_SIZE] __attribute__((aligned(32)));

#define GX_NEAR_PLANE 16.0f
#define GX_FAR_PLANE 24000.0f
#define GX_DECAL_BIAS 0.001f

static Mtx44 gx_perspective_mtx; // 3D feeds -w as z, GX divides by real w
static Mtx44 gx_ortho_mtx;       // 2D/HUD: pass-through of already-NDC coords

static bool gfx_gx_z_is_from_0_to_1(void)
{
    return true;
}

static void gfx_gx_vertex_array_set_attribs(UNUSED struct ShaderProgram *prg)
{
}

// http://amnoid.de/gc/tev.html
static void update_tev(struct ShaderProgram *prg)
{
    // default
    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);

    bool hasTex = prg->cc_features.used_textures[0] || prg->cc_features.used_textures[1];
    bool hasColor = prg->cc_features.num_inputs > 0;

    GX_SetNumChans(prg->cc_features.num_inputs);

    if (hasTex)
    {
        GX_SetNumTexGens(prg->cc_features.used_textures[0] + prg->cc_features.used_textures[1]);
    } else
    {
        GX_SetNumTexGens(0);
    }

    // (d (tevop) ((1.0-c)*a + b*c) + tevbias) * tevscale
    if (prg->cc_features.do_single[0])
    {
        if (hasTex)
        {
            GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
            GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
        }
        else if (hasColor)
        {
            GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
            GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CC_ZERO, GX_CA_ZERO, GX_CC_RASA);
        }
    }
}

static void update_vtx_desc(struct ShaderProgram *prg)
{
    // clear description
    GX_InvVtxCache();
    GX_ClearVtxDesc();
    // we always have a position xyz
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    // rgba colours
    for (int i = 0; i < prg->cc_features.num_inputs; i++) // only seen 0, 1 or 2
    {
        GX_SetVtxDesc(GX_VA_CLR0 + i, GX_DIRECT);
        GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0 + i, GX_CLR_RGBA, GX_RGBA8, 0);
    }
    // tex coords
    for (int i = 0; i < prg->cc_features.used_textures[0] + prg->cc_features.used_textures[1]; i++)
    {
        GX_SetVtxDesc(GX_VA_TEX0 + i, GX_DIRECT);
        GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0 + i, GX_TEX_ST, GX_F32, 0);
    }
}

static void gfx_gx_load_shader(struct ShaderProgram *new_prg)
{
    current_shader = new_prg->program_id;

    update_vtx_desc(new_prg);

    update_tev(new_prg);
}

static void gfx_gx_unload_shader(UNUSED struct ShaderProgram *old_prg)
{
}

static struct ShaderProgram *gfx_gx_create_and_load_new_shader(uint32_t shader_id)
{
    int id = shader_program_pool_size;

    struct ShaderProgram *prg = &shader_program_pool[shader_program_pool_size++];

    prg->program_id = id;

    prg->shader_id = shader_id;
    gfx_cc_get_features(shader_id, &prg->cc_features);

    prg->num_floats = 4;

    if (prg->cc_features.used_textures[0] || prg->cc_features.used_textures[1])
    {
        prg->num_floats += 2;
    }
    if (prg->cc_features.opt_fog)
    {
        prg->num_floats += 4;
    }
    prg->num_floats += prg->cc_features.num_inputs * (prg->cc_features.opt_alpha ? 4 : 3);

    gfx_gx_load_shader(prg);

    return prg;
}

static struct ShaderProgram *gfx_gx_lookup_shader(uint32_t shader_id)
{
    for (uint8_t i = 0; i < shader_program_pool_size; i++)
    {
        if (shader_program_pool[i].shader_id == shader_id)
        {
            return &shader_program_pool[i];
        }
    }
    return NULL;
}

static void gfx_gx_shader_get_info(struct ShaderProgram *prg, uint8_t *num_inputs, bool used_textures[2])
{
    *num_inputs = prg->cc_features.num_inputs;
    used_textures[0] = prg->cc_features.used_textures[0];
    used_textures[1] = prg->cc_features.used_textures[1];
}

static uint32_t gfx_gx_new_texture(void)
{
    if (texture_index == TEXTURE_POOL_SIZE)
        return 0; // out of textures

    return texture_index++;
}

static void gfx_gx_select_texture(int tile, uint32_t texture_id)
{
    GX_LoadTexObj(&texture_pool[texture_id], tile == 0 ? GX_TEXMAP0 : GX_TEXMAP1);

    current_texture = texture_id;
    texture_units[tile] = texture_id;
}

// from https://github.com/camthesaxman/neverball-wii/blob/master/share/wiigl.c
static uint32_t round_up(uint32_t number, uint32_t multiple)
{
    return ((number + multiple - 1) / multiple) * multiple;
}

static void convert_to_rgb5a3(uint16_t *dest, const uint8_t *data, uint32_t width, uint32_t height)
{
    for (uint32_t x = 0; x < width; x++)
    {
        uint32_t blockX = x / 4;
        uint32_t remX = x % 4;

        for (uint32_t y = 0; y < height; y++)
        {
            uint8_t r, g, b, a;
            uint16_t pixel;

            if (data[4 * (x + y * width) + 3] == 255)
            {
                r = (data[4 * (x + y * width) + 0] >> 3) & 31;
                g = (data[4 * (x + y * width) + 1] >> 3) & 31;
                b = (data[4 * (x + y * width) + 2] >> 3) & 31;
                pixel = (1 << 15) | (r << 10) | (g << 5) | b;
            }
            else
            {
                r = (data[4 * (x + y * width) + 0] >> 4) & 15;
                g = (data[4 * (x + y * width) + 1] >> 4) & 15;
                b = (data[4 * (x + y * width) + 2] >> 4) & 15;
                a = (data[4 * (x + y * width) + 3] >> 5) & 7;
                pixel = (a << 12) | (r << 8) | (g << 4) | b;
            }

            uint32_t blockY = y / 4;
            uint32_t remY = y % 4;
            uint32_t index = 16 * (blockX + blockY * width / 4) + (remY * 4 + remX);
            dest[index] = pixel;
        }
    }
}

static void gfx_gx_upload_texture(const uint8_t *rgba32_buf, int width, int height)
{
    uint32_t buffer_width = round_up(width, 4);
    uint32_t buffer_height = round_up(height, 4);
    uint32_t texture_size = buffer_width * buffer_height;

    if ((texture_memory_used + texture_size) > TEXTURE_MEMORY_SIZE)
        texture_memory_used = 0; // it'll do for now

    uint16_t *dest = &texture_memory[texture_memory_used];

    convert_to_rgb5a3(dest, rgba32_buf, width, height);

    DCFlushRange(dest, texture_size * sizeof(uint16_t));

    GX_InitTexObj(&texture_pool[current_texture], dest, buffer_width, buffer_height, GX_TF_RGB5A3, GX_REPEAT, GX_REPEAT, GX_FALSE);

    texture_memory_used += texture_size;
}

static uint32_t gfx_cm_to_gx(uint32_t val)
{
    return (val & G_TX_CLAMP) ? GX_CLAMP : (val & G_TX_MIRROR) ? GX_MIRROR : GX_REPEAT;
}

static void gfx_gx_set_sampler_parameters(int tile, bool linear_filter, uint32_t cms, uint32_t cmt)
{
    GX_InitTexObjWrapMode(&texture_pool[texture_units[tile]], gfx_cm_to_gx(cms), gfx_cm_to_gx(cmt));
    GX_InitTexObjFilterMode(&texture_pool[texture_units[tile]], linear_filter ? GX_LINEAR : GX_NEAR, linear_filter ? GX_LINEAR : GX_NEAR);
}

static bool depth_test_on;
static bool depth_mask_on;
static bool zmode_decal_on;

static int vp_x, vp_y, vp_w, vp_h;

static f32 applied_far_z = 1.0f;

static void set_z_mode()
{
    GX_SetZMode(depth_test_on, GX_LEQUAL, depth_mask_on);
}

static void gfx_gx_set_depth_test(bool depth_test)
{
    depth_test_on = depth_test;
    set_z_mode();
}

static void gfx_gx_set_depth_mask(bool z_upd)
{
    depth_mask_on = z_upd;
    set_z_mode();
}

static void gx_issue_viewport(f32 far_z)
{
    GX_SetViewport((f32)vp_x, (f32)vp_y, (f32)vp_w, (f32)vp_h, 0.0f, far_z); // near z, far z
    applied_far_z = far_z;
}

static void gfx_gx_set_zmode_decal(bool zmode_decal)
{
    zmode_decal_on = zmode_decal;
}

static void gfx_gx_set_viewport(int x, int y, int width, int height)
{
    vp_x = x;
    vp_y = y;
    vp_w = width;
    vp_h = height;
    gx_issue_viewport(1.0f);
}

static void gfx_gx_set_scissor(int x, int y, int width, int height)
{
    GX_SetScissor(x, y, width, height);
}

static void gfx_gx_set_use_alpha(bool use_alpha)
{
    if (use_alpha)
    {
        GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); // guess
    }
    else
    {
        GX_SetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); // another guess
    }
}

static uint8_t float_to_u8(float x)
{
    return MAX(0, MIN(255, x * 255));
}

static void gfx_gx_draw_triangles(float buf_vbo[], UNUSED size_t buf_vbo_len, size_t buf_vbo_num_tris)
{
    bool hasAlpha = shader_program_pool[current_shader].cc_features.opt_alpha;
    bool hasFog = shader_program_pool[current_shader].cc_features.opt_fog;

    uint8_t num_floats =  shader_program_pool[current_shader].num_floats;
    uint8_t num_inputs = shader_program_pool[current_shader].cc_features.num_inputs;
    uint8_t num_tex = shader_program_pool[current_shader].cc_features.used_textures[0] + shader_program_pool[current_shader].cc_features.used_textures[1];

    // gfx_pc emits 2D rectangles with w == 1
    bool is_2d = (buf_vbo[3] == 1.0f);

    if (is_2d)
        GX_LoadProjectionMtx(gx_ortho_mtx, GX_ORTHOGRAPHIC);
    else
        GX_LoadProjectionMtx(gx_perspective_mtx, GX_PERSPECTIVE);

    const f32 want_far_z = (!is_2d && zmode_decal_on) ? (1.0f - GX_DECAL_BIAS) : 1.0f;
    if (want_far_z != applied_far_z)
        gx_issue_viewport(want_far_z);

    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3 * buf_vbo_num_tris);
    {
        uint32_t offset = 0;
        float s[num_tex];
        float t[num_tex];
        for (size_t i = 0; i < 3 * buf_vbo_num_tris; i++)
        {
            if (is_2d)
            {
                GX_Position3f32(buf_vbo[offset + 0],
                                buf_vbo[offset + 1],
                                buf_vbo[offset + 2]);
            }
            else
            {
                GX_Position3f32(buf_vbo[offset + 0],
                                buf_vbo[offset + 1],
                                -buf_vbo[offset + 3]);
            }
            int vtxOffs = 4;

            for (int j = 0; j < num_tex; j++)
            {
                s[j] = buf_vbo[offset + vtxOffs + 0];
                t[j] = buf_vbo[offset + vtxOffs + 1];
                vtxOffs += 2;
            }
            if (hasFog)
                vtxOffs += 4; // TODO: same as 3DS
            for (int j = 0; j < num_inputs; j++)
            {
                GX_Color4u8(float_to_u8(buf_vbo[offset + vtxOffs + 0]),
                            float_to_u8(buf_vbo[offset + vtxOffs + 1]),
                            float_to_u8(buf_vbo[offset + vtxOffs + 2]),
                            hasAlpha ? float_to_u8(buf_vbo[offset + vtxOffs + 3]) : 255);
                vtxOffs += 4;
            }
            for (int j = 0; j < num_tex; j++)
            {
                GX_TexCoord2f32(s[j], t[j]);
            }
            offset += num_floats;
        }
    }
    GX_End();
}

// Configure the embedded framebuffer and the EFB->XFB copy
static void gx_setup_efb(void)
{
    GXRModeObj *rmode = gfx_gx_wm_get_rmode();

    GX_SetCopyClear((GXColor){ 0, 0, 0, 0xff }, GX_MAX_Z24);

    vp_x = 0;
    vp_y = 0;
    vp_w = rmode->fbWidth;
    vp_h = rmode->efbHeight;
    gx_issue_viewport(1.0f); // plain (0, 1) range
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyYScale((f32)rmode->xfbHeight / (f32)rmode->efbHeight);
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);
    GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

    if (rmode->aa)
        GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
    else
        GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

    GX_SetDispCopyGamma(GX_GM_1_0);
}

// Builds the two projection matrices used by gfx_gx_draw_triangles
static void gx_build_projection(void)
{
    const f32 n = GX_NEAR_PLANE;
    const f32 f = GX_FAR_PLANE;

    memset(gx_perspective_mtx, 0, sizeof(gx_perspective_mtx));
    gx_perspective_mtx[0][0] = 1.0f;
    gx_perspective_mtx[1][1] = 1.0f;
    gx_perspective_mtx[2][2] = -n / (f - n);       // near (w=n) -> NDC z -1
    gx_perspective_mtx[2][3] = -(n * f) / (f - n); // far  (w=f) -> NDC z  0
    gx_perspective_mtx[3][2] = -1.0f;
    gx_perspective_mtx[3][3] = 0.0f;

    memset(gx_ortho_mtx, 0, sizeof(gx_ortho_mtx));
    gx_ortho_mtx[0][0] = 1.0f;
    gx_ortho_mtx[1][1] = 1.0f;
    gx_ortho_mtx[2][2] = 1.0f;
    gx_ortho_mtx[2][3] = -1.0f; // z [0,1] -> [-1,0]
    gx_ortho_mtx[3][3] = 1.0f;
}

static void gx_init()
{
    // initialise graphics
    GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);

    gx_setup_efb();

    // other gx setup
    GX_SetCullMode(GX_CULL_NONE);
    GX_SetColorUpdate(GX_TRUE);
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);

    __Mtx ident;
    guMtxIdentity(ident);
    GX_LoadPosMtxImm(ident, GX_PNMTX0);

    gx_build_projection();
}

static void gfx_gx_init(void)
{
    gx_init();
}

static void gfx_gx_on_resize(void)
{
}

static void gfx_gx_start_frame(void)
{
    GX_SetCopyClear((GXColor){ 0, 0, 0, 0xff }, GX_MAX_Z24);

    GX_InvalidateTexAll();
    set_z_mode();
}

static void gfx_gx_end_frame(void)
{
    GX_DrawDone();

    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
}

static void gfx_gx_finish_render(void)
{
}

struct GfxRenderingAPI gfx_gx_api = {
    gfx_gx_z_is_from_0_to_1,
    gfx_gx_unload_shader,
    gfx_gx_load_shader,
    gfx_gx_create_and_load_new_shader,
    gfx_gx_lookup_shader,
    gfx_gx_shader_get_info,
    gfx_gx_new_texture,
    gfx_gx_select_texture,
    gfx_gx_upload_texture,
    gfx_gx_set_sampler_parameters,
    gfx_gx_set_depth_test,
    gfx_gx_set_depth_mask,
    gfx_gx_set_zmode_decal,
    gfx_gx_set_viewport,
    gfx_gx_set_scissor,
    gfx_gx_set_use_alpha,
    gfx_gx_draw_triangles,
    gfx_gx_init,
    gfx_gx_on_resize,
    gfx_gx_start_frame,
    gfx_gx_end_frame,
    gfx_gx_finish_render
};

#endif
