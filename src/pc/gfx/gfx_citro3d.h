#ifndef GFX_CITRO3D_H
#define GFX_CITRO3D_H

#include "gfx_rendering_api.h"

extern struct GfxRenderingAPI gfx_citro3d_api;

#endif

#ifdef ENABLE_N3DS_3D_MODE
#define iodCannon       0x00
#define iodFileSelect   0x01
#define iodStarSelect   0x02
#define iodNormal       0x03
void iodSet(s16 iod);
#endif
