#ifndef GFX_3DS_MENU_H
#define GFX_3DS_MENU_H

#include <stdio.h>
#include <stdbool.h>

#include "gfx_3ds_common.h"

#include "src/minimap/textures/mode_400_t3x.h"
#include "src/minimap/textures/mode_800_t3x.h"
#include "src/minimap/textures/aa_on_t3x.h"
#include "src/minimap/textures/aa_off_t3x.h"
#include "src/minimap/textures/resume_t3x.h"
#include "src/minimap/textures/exit_t3x.h"

struct gfx_configuration
{
   bool useAA;
   bool useWide;
};

extern struct gfx_configuration gfx_config;

typedef enum {
    DO_NOTHING,
    CONFIG_CHANGED,
    EXIT_MENU
} menu_action;

void gfx_3ds_menu_init();
uint32_t gfx_3ds_menu_draw(float *vertex_buffer, int vertex_offset, bool enabled);
menu_action gfx_3ds_menu_on_touch(int x, int y);



#endif
