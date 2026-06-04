#ifdef TARGET_GX

#include <stdbool.h>

#include <ogc/pad.h>

#include <ultra64.h>

#include "controller_api.h"

#ifdef __gamecube__
#include "../configfile.h"
#endif

static int button_mapping[10][2];

static void set_button_mapping(int index, int mask_n64, int mask_gamecube)
{
    button_mapping[index][0] = mask_gamecube;
    button_mapping[index][1] = mask_n64;
}

static uint32_t controller_gamecube_get_held(const PADStatus *status)
{
    uint32_t res = 0;
    uint32_t kDown = status->button;

    for (size_t i = 0; i < sizeof(button_mapping) / sizeof(button_mapping[0]); i++)
    {
        if (button_mapping[i][0] & kDown) {
            res |= button_mapping[i][1];
        }
    }

    bool inverted_look = false;
    s8 deadzone = 10;
    s8 ssx = status->substickX;
    s8 ssy = status->substickY;
    if (ssx > deadzone)
        res |= inverted_look ? L_CBUTTONS : R_CBUTTONS;
    if (ssx < -deadzone)
        res |= inverted_look ? R_CBUTTONS : L_CBUTTONS;
    if (ssy > deadzone)
        res |= inverted_look ? D_CBUTTONS : U_CBUTTONS;
    if (ssy < -deadzone)
        res |= inverted_look ? U_CBUTTONS : D_CBUTTONS;

    return res;
}

static bool controller_gamecube_read_status(PADStatus *status)
{
    PADStatus pads[PAD_CHANMAX];

    PAD_Read(pads);

    if (pads[PAD_CHAN0].err != PAD_ERR_NONE) {
        return false;
    }

    *status = pads[PAD_CHAN0];
    return true;
}

static void controller_gamecube_init(void) {
    PAD_Init();

    uint8_t i = 0;
#ifdef __wii__ // wii uses config for wiimote
    set_button_mapping(i++, A_BUTTON,     PAD_BUTTON_A | PAD_BUTTON_Y); // n64 button => configured button
    set_button_mapping(i++, B_BUTTON,     PAD_BUTTON_B | PAD_BUTTON_X);
    set_button_mapping(i++, START_BUTTON, PAD_BUTTON_START);
    set_button_mapping(i++, L_TRIG,       PAD_TRIGGER_L);
    set_button_mapping(i++, R_TRIG,       PAD_TRIGGER_R);
    set_button_mapping(i++, Z_TRIG,       PAD_TRIGGER_Z);
    set_button_mapping(i++, U_CBUTTONS,   PAD_BUTTON_UP);
    set_button_mapping(i++, D_CBUTTONS,   PAD_BUTTON_DOWN);
    set_button_mapping(i++, L_CBUTTONS,   PAD_BUTTON_LEFT);
    set_button_mapping(i++, R_CBUTTONS,   PAD_BUTTON_RIGHT);
#else
    set_button_mapping(i++, A_BUTTON,     configKeyA);
    set_button_mapping(i++, B_BUTTON,     configKeyB);
    set_button_mapping(i++, START_BUTTON, configKeyStart);
    set_button_mapping(i++, L_TRIG,       configKeyL);
    set_button_mapping(i++, R_TRIG,       configKeyR);
    set_button_mapping(i++, Z_TRIG,       configKeyZ);
    set_button_mapping(i++, U_CBUTTONS,   configKeyCUp);
    set_button_mapping(i++, D_CBUTTONS,   configKeyCDown);
    set_button_mapping(i++, L_CBUTTONS,   configKeyCLeft);
    set_button_mapping(i++, R_CBUTTONS,   configKeyCRight);
#endif
}

static void controller_gamecube_read(OSContPad *pad)
{
    PADStatus status;

    if (!controller_gamecube_read_status(&status)) {
        return;
    }

    pad->button |= controller_gamecube_get_held(&status);

    if (status.stickX != 0 || status.stickY != 0) {
        pad->stick_x = status.stickX;
        pad->stick_y = status.stickY;
    }
}

struct ControllerAPI controller_gamecube = {
    controller_gamecube_init,
    controller_gamecube_read
};

#endif
