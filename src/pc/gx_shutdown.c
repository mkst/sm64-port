#ifdef TARGET_GX

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <asndlib.h>
#include <ogc/gx.h>
#include <ogc/system.h>
#include <ogc/video.h>
#ifdef __wii__
#include <wiiuse/wpad.h>
#endif

#include "gx_shutdown.h"

enum shutdown_kind {
    SHUTDOWN_NONE,
    SHUTDOWN_POWEROFF,
    SHUTDOWN_RESET,
    SHUTDOWN_EXIT,
};

static volatile enum shutdown_kind requested = SHUTDOWN_NONE;
static void (*save_config_hook)(void);

#ifdef __wii__
static void on_power_button(void) {
    requested = SHUTDOWN_POWEROFF;
}

static void on_wiimote_power_button(s32 chan) {
    (void) chan;
    requested = SHUTDOWN_POWEROFF;
}
#endif

static void on_reset_button(u32 irq, void *ctx) {
    (void) irq;
    (void) ctx;
    requested = SHUTDOWN_RESET;
}

void gx_shutdown_init(void (*save_config)(void)) {
    save_config_hook = save_config;

    SYS_SetResetCallback(on_reset_button);
#ifdef __wii__
    SYS_SetPowerCallback(on_power_button);
    WPAD_SetPowerButtonCallback(on_wiimote_power_button);
#endif
}

void gx_shutdown_request_exit(void) {
    requested = SHUTDOWN_EXIT;
}

// Stop everything that talks to hardware behind our back. The reset paths run with
// interrupts disabled and tear the DSP and GX down themselves, so leaving a voice
// playing or a frame in flight is enough to hang the console on the way out.
static void quiesce_hardware(void) {
    ASND_Pause(1);
    ASND_End();

    GX_AbortFrame();
    GX_Flush();

    VIDEO_SetBlack(true);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

void gx_shutdown_poll(void) {
    const enum shutdown_kind kind = requested;
    if (kind == SHUTDOWN_NONE) {
        return;
    }
    requested = SHUTDOWN_NONE;

    // Save options before doing any other shutdown work
    if (save_config_hook != NULL) {
        save_config_hook();
    }

    quiesce_hardware();

#ifdef __wii__
    switch (kind) {
        case SHUTDOWN_POWEROFF:
            SYS_ResetSystem(SYS_POWEROFF, 0, 0);
            break;
        case SHUTDOWN_RESET:
            SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
            break;
        default:
            // Back to the Homebrew Channel, so the options that need a restart can be
            // picked up by relaunching
            exit(0);
            break;
    }
#else
    // A GameCube has no system menu to go back to. exit() hands control to the loader
    // that started us when it left a return stub behind, and otherwise resets the
    // console, which is as close to a restart as we can get.
    (void) kind;
    exit(0);
#endif
}

#endif
