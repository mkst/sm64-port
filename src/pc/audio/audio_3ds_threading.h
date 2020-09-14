#ifdef TARGET_N3DS

#ifndef GFX_3DS_AUDIO_THREADING_H
#define GFX_3DS_AUDIO_THREADING_H

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

#include <3ds.h>

extern LightEvent s_event_audio;
extern LightEvent s_event_main;

#endif
#endif
