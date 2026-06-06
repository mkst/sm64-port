#include "controller_rumble.h"

#ifdef TARGET_GX

#include <stdbool.h>

#include <PR/ultratypes.h>

#ifdef __wii__
#include <wiiuse/wpad.h>
#endif
#include <ogc/pad.h>

#include "game/thread6.h"
#include "../configfile.h"

extern u32 gGlobalTimer;

// Replication of RumbleData / struct StructSH8031D9B0 from src/game/main.h
struct RumbleData {
    u8  unk00;
    u8  unk01;
    s16 unk02;
    s16 unk04;
};
struct RumbleSettings {
    s16 unk00;
    s16 unk02;
    s16 unk04;
    s16 unk06;
    s16 unk08;
    s16 unk0A;
    s16 unk0C;
    s16 unk0E;
};

static struct RumbleData sRumbleDataQueue[3];
static struct RumbleSettings sCurrRumbleSettings;
s32 gRumblePakTimer;

// Motor
static bool sMotorOn = false;

static void set_motor(bool on) {
    if (on == sMotorOn) {
        return;
    }
    sMotorOn = on;
#ifdef __wii__
    WPAD_Rumble(WPAD_CHAN_0, on);
#endif
    PAD_ControlMotor(PAD_CHAN0, on ? PAD_MOTOR_RUMBLE : PAD_MOTOR_STOP);
}

static void start_rumble(void) {
    set_motor(configRumble);
}

static void stop_rumble(void) {
    set_motor(false);
}

// Shindou rumble model ("borrowed" from thread6.c)
static void update_rumble_pak(void) {
    if (sCurrRumbleSettings.unk08 > 0) {
        sCurrRumbleSettings.unk08--;
        start_rumble();
    } else if (sCurrRumbleSettings.unk04 > 0) {
        sCurrRumbleSettings.unk04--;

        sCurrRumbleSettings.unk02 -= sCurrRumbleSettings.unk0E;
        if (sCurrRumbleSettings.unk02 < 0) {
            sCurrRumbleSettings.unk02 = 0;
        }

        if (sCurrRumbleSettings.unk00 == 1) {
            start_rumble();
        } else if (sCurrRumbleSettings.unk06 >= 0x100) {
            sCurrRumbleSettings.unk06 -= 0x100;
            start_rumble();
        } else {
            sCurrRumbleSettings.unk06 +=
                ((sCurrRumbleSettings.unk02 * sCurrRumbleSettings.unk02 * sCurrRumbleSettings.unk02) / (1 << 9)) + 4;
            stop_rumble();
        }
    } else {
        sCurrRumbleSettings.unk04 = 0;

        if (sCurrRumbleSettings.unk0A >= 5) {
            start_rumble();
        } else if ((sCurrRumbleSettings.unk0A >= 2) && (gGlobalTimer % sCurrRumbleSettings.unk0C == 0)) {
            start_rumble();
        } else {
            stop_rumble();
        }
    }

    if (sCurrRumbleSettings.unk0A > 0) {
        sCurrRumbleSettings.unk0A--;
    }
}

static void update_rumble_data_queue(void) {
    if (sRumbleDataQueue[0].unk00) {
        sCurrRumbleSettings.unk06 = 0;
        sCurrRumbleSettings.unk08 = 4;
        sCurrRumbleSettings.unk00 = sRumbleDataQueue[0].unk00;
        sCurrRumbleSettings.unk04 = sRumbleDataQueue[0].unk02;
        sCurrRumbleSettings.unk02 = sRumbleDataQueue[0].unk01;
        sCurrRumbleSettings.unk0E = sRumbleDataQueue[0].unk04;
    }

    sRumbleDataQueue[0] = sRumbleDataQueue[1];
    sRumbleDataQueue[1] = sRumbleDataQueue[2];
    sRumbleDataQueue[2].unk00 = 0;
}

void queue_rumble_data(s16 a0, s16 a1) {
    if (!configRumble) {
        return;
    }

    if (a1 > 70) {
        sRumbleDataQueue[2].unk00 = 1;
    } else {
        sRumbleDataQueue[2].unk00 = 2;
    }

    sRumbleDataQueue[2].unk01 = a1;
    sRumbleDataQueue[2].unk02 = a0;
    sRumbleDataQueue[2].unk04 = 0;
}

void func_sh_8024C89C(s16 a0) {
    sRumbleDataQueue[2].unk04 = a0;
}

u8 is_rumble_finished_and_queue_empty(void) {
    if (sCurrRumbleSettings.unk08 + sCurrRumbleSettings.unk04 >= 4) {
        return FALSE;
    }
    if (sRumbleDataQueue[0].unk00 != 0) {
        return FALSE;
    }
    if (sRumbleDataQueue[1].unk00 != 0) {
        return FALSE;
    }
    if (sRumbleDataQueue[2].unk00 != 0) {
        return FALSE;
    }
    return TRUE;
}

void reset_rumble_timers(void) {
    if (!configRumble) {
        return;
    }

    if (sCurrRumbleSettings.unk0A == 0) {
        sCurrRumbleSettings.unk0A = 7;
    }
    if (sCurrRumbleSettings.unk0A < 4) {
        sCurrRumbleSettings.unk0A = 4;
    }
    sCurrRumbleSettings.unk0C = 7;
}

void reset_rumble_timers_2(s32 a0) {
    if (!configRumble) {
        return;
    }

    if (sCurrRumbleSettings.unk0A == 0) {
        sCurrRumbleSettings.unk0A = 7;
    }
    if (sCurrRumbleSettings.unk0A < 4) {
        sCurrRumbleSettings.unk0A = 4;
    }

    if (a0 == 4) sCurrRumbleSettings.unk0C = 1;
    if (a0 == 3) sCurrRumbleSettings.unk0C = 2;
    if (a0 == 2) sCurrRumbleSettings.unk0C = 3;
    if (a0 == 1) sCurrRumbleSettings.unk0C = 4;
    if (a0 == 0) sCurrRumbleSettings.unk0C = 5;
}

void func_sh_8024CA04(void) {
    if (!configRumble) {
        return;
    }

    sCurrRumbleSettings.unk0A = 4;
    sCurrRumbleSettings.unk0C = 4;
}

void cancel_rumble(void) {
    stop_rumble();

    sRumbleDataQueue[0].unk00 = 0;
    sRumbleDataQueue[1].unk00 = 0;
    sRumbleDataQueue[2].unk00 = 0;

    sCurrRumbleSettings.unk04 = 0;
    sCurrRumbleSettings.unk0A = 0;

    gRumblePakTimer = 0;
}

//Rumble is based on VI not fps
static void rumble_step(void) {
    update_rumble_data_queue();
    update_rumble_pak();

    if (gRumblePakTimer > 0) {
        gRumblePakTimer--;
    }
}

void rumble_update(void) {
    rumble_step();
    rumble_step();
}

#else // !TARGET_GX

void rumble_update(void) {}

#endif
