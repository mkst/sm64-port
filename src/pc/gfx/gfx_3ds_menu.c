#ifdef TARGET_N3DS

#include "gfx_3ds.h"
#include "gfx_3ds_menu.h"

#ifdef ENABLE_N3DS_3D_MODE
struct gfx_configuration gfx_config = {false, false}; // AA off, 800px off
#else
struct gfx_configuration gfx_config = {true, false}; // AA on, 800px off
#endif

static C3D_Mtx modelView, projection;
static int buffer_offset;

static C3D_Tex mode_400_tex, mode_800_tex;
static C3D_Tex aa_off_tex, aa_on_tex;
static C3D_Tex resume_tex, exit_tex;

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
    C3D_TexEnvColor(env, 0x66000000);
    C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, 0, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    C3D_DrawArrays(GPU_TRIANGLES, buffer_offset, 6); // 2 triangles

    buffer_offset += 6;
}

static void gfx_3ds_menu_draw_button(float *vbo_buffer, int x, int y, C3D_Tex texture)
{
    Mtx_Identity(&modelView);
    Mtx_Translate(&modelView, x, 240 - y, 0.0f, false);

    Mtx_OrthoTilt(&projection, 0.0, 320.0, 0.0, 240.0, 0.0, 1.0, true);

    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &modelView);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);

    memcpy(vbo_buffer + buffer_offset * VERTEX_SHADER_SIZE,
           vertex_list_button,
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
    gfx_3ds_menu_draw_button(vertex_buffer, 48, 96, gfx_config.useAA ? aa_on_tex : aa_off_tex);
    // screen mode
    gfx_3ds_menu_draw_button(vertex_buffer, 208, 96, gfx_config.useWide ? mode_800_tex : mode_400_tex);
    // resume
    gfx_3ds_menu_draw_button(vertex_buffer, 48, 208, resume_tex);
    // exit game
    gfx_3ds_menu_draw_button(vertex_buffer, 208, 208, exit_tex);
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
    if (is_inside_box(touch_x, touch_y, 48, 32, 64, 64))
    {
#ifndef ENABLE_N3DS_3D_MODE
        gfx_config.useAA = !gfx_config.useAA;
        return CONFIG_CHANGED;
#else
        return DO_NOTHING;
#endif
    }
    // screen mode
    if (is_inside_box(touch_x, touch_y, 208, 32, 64, 64))
    {
        gfx_config.useWide = !gfx_config.useWide;
        return CONFIG_CHANGED;
    }
    // resume
    if (is_inside_box(touch_x, touch_y, 48, 144, 64, 64))
    {
        return EXIT_MENU;
    }
    // exit?
    if (is_inside_box(touch_x, touch_y, 208, 144, 64, 64))
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
}

uint32_t gfx_3ds_menu_draw(float *vertex_buffer, int vertex_offset, bool enabled)
{
    buffer_offset = vertex_offset;
    gfx_3ds_menu_draw_background(vertex_buffer);
    if (enabled)
        gfx_3ds_menu_draw_buttons(vertex_buffer);
    if (debounce)
        debounce--;

    return buffer_offset - vertex_offset;
}

#endif
