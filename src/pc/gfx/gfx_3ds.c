#ifdef TARGET_N3DS

#include "macros.h"

#include "gfx_3ds.h"
#include "gfx_3ds_menu.h"

C3D_RenderTarget *gTarget;
C3D_RenderTarget *gTargetRight;
C3D_RenderTarget *gTargetBottom;

int uLoc_projection, uLoc_modelView;

float gSliderLevel;

Gfx3DSMode gGfx3DSMode;

bool gShowConfigMenu = false;
bool gShouldRun = true;

static u8 n3ds_model = 0;

static bool checkN3DS()
{
    bool isNew3DS = false;
    if (R_SUCCEEDED(APT_CheckNew3DS(&isNew3DS)))
        return isNew3DS;

    return false;
}

static void deinitialise_screens()
{
    if (gTarget != NULL)
    {
        C3D_RenderTargetDelete(gTarget);
        gTarget = NULL;
    }
    if (gTargetRight != NULL)
    {
        C3D_RenderTargetDelete(gTargetRight);
        gTargetRight = NULL;
    }
    if (gTargetBottom != NULL)
    {
        C3D_RenderTargetDelete(gTargetBottom);
        gTargetBottom = NULL;
    }
    C3D_Fini();
}

static void initialise_screens()
{
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    bool useAA = gfx_config.useAA;
    bool useWide =  gfx_config.useWide && n3ds_model != 3; // old 2DS does not support 800px

    u32 transferFlags = DISPLAY_TRANSFER_FLAGS;

    if (useAA && !useWide)
        transferFlags |= GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_XY);
    else if (useAA && useWide)
        transferFlags |= GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_X);
    else
        transferFlags |= GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO);

    int width = useAA || useWide ? 800 : 400;
    int height = useAA ? 480 : 240;

    gTarget = C3D_RenderTargetCreate(height, width, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(gTarget, GFX_TOP, GFX_LEFT, transferFlags);

    if (!useWide)
    {
        gfxSetWide(false);
        // TODO: reinitialise screens when iod changes from/to 0.0
        gTargetRight = C3D_RenderTargetCreate(height, width, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
        C3D_RenderTargetSetOutput(gTargetRight, GFX_TOP, GFX_RIGHT, transferFlags);
        gfxSet3D(true);
    }
    else
    {
        gfxSet3D(false);
        gfxSetWide(true);
    }

    // used to determine scissoring
    if (!useAA && !useWide)
        gGfx3DSMode = GFX_3DS_MODE_NORMAL;
    else if (useAA && !useWide)
        gGfx3DSMode = GFX_3DS_MODE_AA_22;
    else if (!useAA && useWide)
        gGfx3DSMode = GFX_3DS_MODE_WIDE;
    else
        gGfx3DSMode = GFX_3DS_MODE_WIDE_AA_12;

    // TODO: refactor; this is (also) set in gfx_citro3d_init,
    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthMap(true, -1.0f, 0);
    C3D_DepthTest(false, GPU_LEQUAL, GPU_WRITE_ALL);
    C3D_AlphaTest(true, GPU_GREATER, 0x00);

    gTargetBottom = C3D_RenderTargetCreate(240, 320, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(gTargetBottom, GFX_BOTTOM, GFX_LEFT,
        DISPLAY_TRANSFER_FLAGS | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    // consoleInit(GFX_BOTTOM, NULL);
}

static void gfx_3ds_init(UNUSED const char *game_name, UNUSED bool start_in_fullscreen)
{
    if (checkN3DS())
        osSetSpeedupEnable(true);

    gfxInitDefault();

    Result rc = cfguInit();
    if (R_SUCCEEDED(rc))
    {
        u8 model;
        rc = CFGU_GetSystemModel(&model);
        if (R_SUCCEEDED(rc))
            n3ds_model = model;
        cfguExit();
    }

    gfx_3ds_menu_init();

    initialise_screens();
}

static void gfx_set_keyboard_callbacks(UNUSED bool (*on_key_down)(int scancode), UNUSED bool (*on_key_up)(int scancode), UNUSED void (*on_all_keys_up)(void))
{
}

static void gfx_set_fullscreen_changed_callback(UNUSED void (*on_fullscreen_changed)(bool is_now_fullscreen))
{
}

static void gfx_set_fullscreen(UNUSED bool enable)
{
}

static void gfx_3ds_main_loop(void (*run_one_game_iter)(void))
{
    while (aptMainLoop() && gShouldRun)
    {
        run_one_game_iter();
    }

    C3D_Fini();
    gfxExit();
}

static void gfx_3ds_get_dimensions(uint32_t *width, uint32_t *height)
{
    *width = 400;
    *height = 240;
}

static int debounce;
static void gfx_3ds_handle_events(void)
{
    // as good a time as any
    gSliderLevel = osGet3DSliderState();

    if (debounce > 0)
    {
        debounce--;
        return;
    }

    // check if screen is pressed
    hidScanInput();
    touchPosition pos;
    hidTouchRead(&pos);

    if ((pos.px || pos.py) && (pos.px < 160))
    {
        debounce = 8;
        if (gShowConfigMenu)
        {
            menu_action res = gfx_3ds_menu_on_touch(pos.px, pos.py);
            if (res == CONFIG_CHANGED)
            {
                deinitialise_screens();
                initialise_screens();
            } else if (res == EXIT_MENU)
            {
                gShowConfigMenu = false;
            }
        } else
        {
            // screen tapped so show menu
            gShowConfigMenu = true;
        }
    }
}

float cpu_time, gpu_time;
uint8_t skip_debounce;

static bool gfx_3ds_start_frame(void)
{
#ifndef DISABLE_N3DS_FRAMESKIP
    if (skip_debounce)
    {
        skip_debounce--;
        return true;
    }
    // skip if frame took longer than 1 / 30 = 33.3 ms
    if (cpu_time + gpu_time > 33.3f)
    {
        skip_debounce = 3; // skip a max of once every 4 frames
        cpu_time = 0, gpu_time = 0;
        return false;
    }
#endif
    return true;
}

static void gfx_3ds_swap_buffers_begin(void)
{
}

static void gfx_3ds_swap_buffers_end(void)
{
    cpu_time = C3D_GetProcessingTime();
    gpu_time = C3D_GetDrawingTime();
}

static double gfx_3ds_get_time(void)
{
    return 0.0;
}

struct GfxWindowManagerAPI gfx_3ds =
{
    gfx_3ds_init,
    gfx_set_keyboard_callbacks,
    gfx_set_fullscreen_changed_callback,
    gfx_set_fullscreen,
    gfx_3ds_main_loop,
    gfx_3ds_get_dimensions,
    gfx_3ds_handle_events,
    gfx_3ds_start_frame,
    gfx_3ds_swap_buffers_begin,
    gfx_3ds_swap_buffers_end,
    gfx_3ds_get_time
};

#endif
