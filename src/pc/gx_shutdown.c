#if defined(TARGET_GX) && defined(__wii__)

#include <stddef.h>

#include <ogc/system.h>
#include <wiiuse/wpad.h>

#include "wii_shutdown.h"

enum shutdown_kind {
    SHUTDOWN_NONE,
    SHUTDOWN_POWEROFF,
    SHUTDOWN_RESET,
};

static volatile enum shutdown_kind requested = SHUTDOWN_NONE;
static void (*save_config_hook)(void);

static void on_power_button(void) {
    requested = SHUTDOWN_POWEROFF;
}

static void on_wiimote_power_button(s32 chan) {
    (void) chan;
    requested = SHUTDOWN_POWEROFF;
}

static void on_reset_button(u32 irq, void *ctx) {
    (void) irq;
    (void) ctx;
    requested = SHUTDOWN_RESET;
}

void wii_shutdown_init(void (*save_config)(void)) {
    save_config_hook = save_config;

    SYS_SetPowerCallback(on_power_button);
    SYS_SetResetCallback(on_reset_button);
    WPAD_SetPowerButtonCallback(on_wiimote_power_button);
}

void wii_shutdown_poll(void) {
    const enum shutdown_kind kind = requested;
    if (kind == SHUTDOWN_NONE) {
        return;
    }

    // Save options before doing any other shutdown work
    if (save_config_hook != NULL) {
        save_config_hook();
    }

    if (kind == SHUTDOWN_POWEROFF) {
        SYS_ResetSystem(SYS_POWEROFF, 0, 0);
    } else {
        SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
    }
}

#endif
