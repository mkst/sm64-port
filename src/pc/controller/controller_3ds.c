#ifdef TARGET_N3DS

//hack for redefinition of types in libctru
#define u64 __u64
#define s64 __s64
#define u32 __u32
#define vu32 __vu32
#define vs32 __vs32
#define s32 __s32
#define u16 __u16
#define s16 __s16
#define u8 __u8
#define s8 __s8
#include <3ds/types.h>
#undef u64
#undef s64
#undef u32
#undef vu32
#undef vs32
#undef s32
#undef u16
#undef s16
#undef u8
#undef s8

#include <ultra64.h>

#include <3ds.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "controller_api.h"

#include "../configfile.h"

static int button_mapping[10][2];

static void set_button_mapping(int index, int mask_n64, int mask_3ds)
{
    button_mapping[index][0] = mask_3ds;
    button_mapping[index][1] = mask_n64;
}

// From gfx_3ds_menu
static bool is_inside_box(int pos_x, int pos_y, int x, int y, int width, int height)
{
    return pos_x >= x && pos_x <= (x+width) && pos_y >= y && pos_y <= (y+height);
}

static u32 controller_3ds_get_held(void)
{
    u32 res = 0;
    hidScanInput();
    u32 kDown = keysHeld();
    for (size_t i = 0; i < sizeof(button_mapping) / sizeof(button_mapping[0]); i++)
    {
        if (button_mapping[i][0] & kDown) {
            res |= button_mapping[i][1];
        }
    }

    touchPosition pos;
    hidTouchRead(&pos);

    if (is_inside_box(pos.px, pos.py, 170, 122, 64, 64))
        res |= L_CBUTTONS;
    if (is_inside_box(pos.px, pos.py, 245, 122, 64, 64))
        res |= R_CBUTTONS;
    if (is_inside_box(pos.px, pos.py, 207, 197, 64, 32))
        res |= D_CBUTTONS;
    if (is_inside_box(pos.px, pos.py, 207, 79, 64, 32))
        res |= U_CBUTTONS;

    return res;
}

static void controller_3ds_init(void)
{
    u32 i;
    set_button_mapping(i++, A_BUTTON,     configKeyA); // n64 button => configured button
    set_button_mapping(i++, B_BUTTON,     configKeyB);
    set_button_mapping(i++, START_BUTTON, configKeyStart);
    set_button_mapping(i++, L_TRIG,       configKeyL);
    set_button_mapping(i++, R_TRIG,       configKeyR);
    set_button_mapping(i++, Z_TRIG,       configKeyZ);
    set_button_mapping(i++, U_CBUTTONS,   configKeyCUp);
    set_button_mapping(i++, D_CBUTTONS,   configKeyCDown);
    set_button_mapping(i++, L_CBUTTONS,   configKeyCLeft);
    set_button_mapping(i++, R_CBUTTONS,   configKeyCRight);
}

static void controller_3ds_read(OSContPad *pad)
{
    pad->button = controller_3ds_get_held();

    circlePosition pos;
    hidCircleRead(&pos);
    pad->stick_x = pos.dx / 2;
    pad->stick_y = pos.dy / 2;
}

struct ControllerAPI controller_3ds = {
    controller_3ds_init,
    controller_3ds_read
};

#endif
