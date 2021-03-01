#ifdef TARGET_N3DS

#include "gfx_3ds.h"
#include "gfx_3ds_menu.h"

struct gfx_configuration gfx_config = {false, false}; // AA off, 800px off

static C3D_Mtx modelView, projection;
static int buffer_offset;

static C3D_Tex mode_400_tex, mode_800_tex;
static C3D_Tex aa_off_tex, aa_on_tex;
static C3D_Tex resume_tex, exit_tex;
static C3D_Tex menu_cleft_tex, menu_cright_tex, menu_cdown_tex, menu_cup_tex;

static u8 debounce;

static int touch_x;
static int touch_y;


static void gfx_3ds_menu_draw_background(float *vbo_buffer)
{
    Mtx_Identity(&modelView);
    Mtx_OrthoTilt(&projection, 0.0, 320.0, 0.0, 240.0, 0.0, 1.0, true);

    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &modelView);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);

    memcpy(vbo_buffer + buffer_offset * VERTEX_SHADER_SIZE,
           vertex_list_color,
           sizeof(vertex_list_color));

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvColor(env, 0xFF070707);
    C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, 0, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    C3D_DrawArrays(GPU_TRIANGLES, buffer_offset, 6); // 2 triangles

    buffer_offset += 6;
}

static void gfx_3ds_menu_draw_button(float *vbo_buffer, int x, int y, C3D_Tex texture, bool thin)
{
    Mtx_Identity(&modelView);
    Mtx_Translate(&modelView, x, 240 - y, 0.0f, false);

    Mtx_OrthoTilt(&projection, 0.0, 320.0, 0.0, 240.0, 0.0, 1.0, true);

    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &modelView);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);

    const vertex *vertex_list = thin ? vertex_list_button_thin : vertex_list_button;

    memcpy(vbo_buffer + buffer_offset * VERTEX_SHADER_SIZE,
           vertex_list,
           sizeof(vertex_list_button));

    C3D_TexBind(0, &texture);
    C3D_TexFlush(&texture);

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvColor(env, 0);
    C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    C3D_DrawArrays(GPU_TRIANGLES, buffer_offset, 6); // 2 triangles
    buffer_offset += 6;
}

static void gfx_3ds_menu_draw_buttons(float * vertex_buffer)
{
    // aa
    gfx_3ds_menu_draw_button(vertex_buffer, 11, 96, gfx_config.useAA ? aa_on_tex : aa_off_tex, false);
    // screen mode
    gfx_3ds_menu_draw_button(vertex_buffer, 86, 96, gfx_config.useWide ? mode_800_tex : mode_400_tex, false);
    // resume
    gfx_3ds_menu_draw_button(vertex_buffer, 11, 208, resume_tex, false);
    // exit game
    gfx_3ds_menu_draw_button(vertex_buffer, 86, 208, exit_tex, false);
}

static void gfx_3ds_menu_draw_cbuttons(float * vertex_buffer)
{
    // cleft
    gfx_3ds_menu_draw_button(vertex_buffer, 170, 122+64, menu_cleft_tex, false);
    // cright
    gfx_3ds_menu_draw_button(vertex_buffer, 245, 122+64, menu_cright_tex, false);
    // cdown
    gfx_3ds_menu_draw_button(vertex_buffer, 207, 197+32, menu_cdown_tex, true);
    // cup
    gfx_3ds_menu_draw_button(vertex_buffer, 207, 79+32, menu_cup_tex, true);
}

static bool is_inside_box(int pos_x, int pos_y, int x, int y, int width, int height)
{
    return pos_x >= x && pos_x <= (x+width) && pos_y >= y && pos_y <= (y+height);
}

menu_action gfx_3ds_menu_on_touch(int x, int y)
{
    if (debounce)
        return DO_NOTHING;

    touch_x = x;
    touch_y = y;
    debounce = 8; // wait quarter second between mashing

    // aa
    if (is_inside_box(touch_x, touch_y, 11, 32, 64, 64))
    {
        // cannot use AA in 3D mode
        if (gfx_config.useWide)
        {
            gfx_config.useAA = !gfx_config.useAA;
            return CONFIG_CHANGED;
        }
        return DO_NOTHING;
    }
    // screen mode
    if (is_inside_box(touch_x, touch_y, 86, 32, 64, 64))
    {
        gfx_config.useWide = !gfx_config.useWide;
        // disable AA if 3D mode
        if (!gfx_config.useWide && gfx_config.useAA)
            gfx_config.useAA = false;
        return CONFIG_CHANGED;
    }
    // resume
    if (is_inside_box(touch_x, touch_y, 11, 144, 64, 64))
    {
        return EXIT_MENU;
    }
    // exit?
    if (is_inside_box(touch_x, touch_y, 86, 144, 64, 64))
    {
        gShouldRun = false;
    }

    return DO_NOTHING;
}

void gfx_3ds_menu_init()
{
    // load all the textures
    load_t3x_texture(&mode_400_tex, NULL, mode_400_t3x, mode_400_t3x_size);
    C3D_TexSetFilter(&mode_400_tex, GPU_LINEAR, GPU_NEAREST);

    load_t3x_texture(&mode_800_tex, NULL, mode_800_t3x, mode_800_t3x_size);
    C3D_TexSetFilter(&mode_800_tex, GPU_LINEAR, GPU_NEAREST);

    load_t3x_texture(&aa_on_tex, NULL, aa_on_t3x, aa_on_t3x_size);
    C3D_TexSetFilter(&aa_on_tex, GPU_LINEAR, GPU_NEAREST);

    load_t3x_texture(&aa_off_tex, NULL, aa_off_t3x, aa_off_t3x_size);
    C3D_TexSetFilter(&aa_off_tex, GPU_LINEAR, GPU_NEAREST);

    load_t3x_texture(&resume_tex, NULL, resume_t3x, resume_t3x_size);
    C3D_TexSetFilter(&resume_tex, GPU_LINEAR, GPU_NEAREST);

    load_t3x_texture(&exit_tex, NULL, exit_t3x, exit_t3x_size);
    C3D_TexSetFilter(&exit_tex, GPU_LINEAR, GPU_NEAREST);

    load_t3x_texture(&menu_cleft_tex, NULL, menu_cleft_t3x, menu_cleft_t3x_size);
    C3D_TexSetFilter(&menu_cleft_tex, GPU_LINEAR, GPU_NEAREST);

    load_t3x_texture(&menu_cright_tex, NULL, menu_cright_t3x, menu_cright_t3x_size);
    C3D_TexSetFilter(&menu_cright_tex, GPU_LINEAR, GPU_NEAREST);

    load_t3x_texture(&menu_cdown_tex, NULL, menu_cdown_t3x, menu_cdown_t3x_size);
    C3D_TexSetFilter(&menu_cdown_tex, GPU_LINEAR, GPU_NEAREST);

    load_t3x_texture(&menu_cup_tex, NULL, menu_cup_t3x, menu_cup_t3x_size);
    C3D_TexSetFilter(&menu_cup_tex, GPU_LINEAR, GPU_NEAREST);
}

void gfx_3ds_menu_draw(float *vertex_buffer, int vertex_offset, bool enabled)
{
    C3D_FrameDrawOn(gTargetBottom);

    buffer_offset = vertex_offset;
    gfx_3ds_menu_draw_background(vertex_buffer);
    if (enabled)
        gfx_3ds_menu_draw_buttons(vertex_buffer);
    if (debounce)
        debounce--;

    gfx_3ds_menu_draw_cbuttons(vertex_buffer);
}

#endif
