#ifndef THREAD6_H
#define THREAD6_H

// GX is here since Wii has an option to enable rumble on all versions
// Code for this lives here: src/pc/controller/controller_rumble.c
#if defined(VERSION_SH) || defined(TARGET_GX)

extern s32 gRumblePakTimer;

void init_rumble_pak_scheduler_queue(void);
void block_until_rumble_pak_free(void);
void release_rumble_pak_control(void);
void queue_rumble_data(s16 a0, s16 a1);
void func_sh_8024C89C(s16 a0);
u8 is_rumble_finished_and_queue_empty(void);
void reset_rumble_timers(void);
void reset_rumble_timers_2(s32 a0);
void func_sh_8024CA04(void);
void cancel_rumble(void);
void create_thread_6(void);
void rumble_thread_update_vi(void);

#endif // VERSION_SH || TARGET_GX

#endif // THREAD6_H
