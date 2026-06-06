#ifdef TARGET_GX

#include "macros.h" // for UNUSED

#include "gfx_gx_wm.h"
#include "gfx_screen_config.h"
#include "../configfile.h" // for config60Fps, config240p, configAntialias

static GXRModeObj *rmode;
static void *framebuffer[2];
static bool fb;

// Pick the video mode to render in
// Currently, options are 240p or 480p (or 576i for PAL)
static GXRModeObj *gfx_gx_wm_select_mode(void)
{
    if (!config240p)
        return VIDEO_GetPreferredMode(NULL);

    switch (VIDEO_GetCurrentTvMode())
    {
        case VI_PAL:
            return configAntialias ? &TVPal264DsAa : &TVPal264Ds;
        case VI_MPAL:
            return configAntialias ? &TVMpal240DsAa : &TVMpal240Ds;
        case VI_EURGB60:
            return configAntialias ? &TVEurgb60Hz240DsAa : &TVEurgb60Hz240Ds;
        case VI_NTSC:
        default:
            return configAntialias ? &TVNtsc240DsAa : &TVNtsc240Ds;
    }
}

static void gfx_gx_wm_init(UNUSED const char *game_name, UNUSED bool start_in_fullscreen)
{
    VIDEO_Init();
    VIDEO_SetBlack(true);

    rmode = gfx_gx_wm_select_mode();

    // double-buffering
    framebuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    framebuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_Configure(rmode);

    VIDEO_SetNextFramebuffer(framebuffer[fb]);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();
}

GXRModeObj *gfx_gx_wm_get_rmode(void)
{
    return rmode;
}

static void gfx_gx_wm_set_keyboard_callbacks(UNUSED bool (*on_key_down)(int scancode), UNUSED bool (*on_key_up)(int scancode), UNUSED void (*on_all_keys_up)(void))
{
}

static void gfx_gx_wm_set_fullscreen_changed_callback(UNUSED void (*on_fullscreen_changed)(bool is_now_fullscreen))
{
}

static void gfx_gx_wm_set_fullscreen(UNUSED bool enable)
{
}

static void gfx_gx_wm_main_loop(void (*run_one_game_iter)(void))
{
    run_one_game_iter();
}

static void gfx_gx_wm_get_dimensions(uint32_t *width, uint32_t *height)
{
    if (config240p) {
        // GX handles all scaling, so we just lie here
        *width = DESIRED_SCREEN_WIDTH;
        *height = DESIRED_SCREEN_HEIGHT;
    } else {
        *width = rmode->fbWidth;
        *height = rmode->xfbHeight;
    }
}

static void gfx_gx_wm_handle_events(void)
{
}

static bool gfx_gx_wm_start_frame(void)
{
    return true;
}

static void gfx_gx_wm_swap_buffers_begin(void)
{
    fb ^= 1; // flip frame buffer

    GX_CopyDisp(framebuffer[fb], GX_TRUE);
    GX_Flush();

    VIDEO_SetNextFramebuffer(framebuffer[fb]);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    // Hold each rendered frame for an extra field so the 30Hz game logic maps to a 30Hz display
    // Skipped at 60fps
    if (!config60Fps) {
        VIDEO_WaitVSync();
    }
}

static void gfx_gx_wm_swap_buffers_end(void)
{
}

static double gfx_gx_wm_get_time(void)
{
    return 0.0;
}

struct GfxWindowManagerAPI gfx_gx_wm_api =
{
    gfx_gx_wm_init,
    gfx_gx_wm_set_keyboard_callbacks,
    gfx_gx_wm_set_fullscreen_changed_callback,
    gfx_gx_wm_set_fullscreen,
    gfx_gx_wm_main_loop,
    gfx_gx_wm_get_dimensions,
    gfx_gx_wm_handle_events,
    gfx_gx_wm_start_frame,
    gfx_gx_wm_swap_buffers_begin,
    gfx_gx_wm_swap_buffers_end,
    gfx_gx_wm_get_time
};

#endif
