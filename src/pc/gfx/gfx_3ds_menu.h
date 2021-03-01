#ifndef GFX_3DS_MENU_H
#define GFX_3DS_MENU_H

#include <stdio.h>
#include <stdbool.h>

#include "src/minimap/textures/mode_400_t3x.h"
#include "src/minimap/textures/mode_800_t3x.h"
#include "src/minimap/textures/aa_on_t3x.h"
#include "src/minimap/textures/aa_off_t3x.h"
#include "src/minimap/textures/resume_t3x.h"
#include "src/minimap/textures/exit_t3x.h"
#include "src/minimap/textures/menu_cleft_t3x.h"
#include "src/minimap/textures/menu_cright_t3x.h"
#include "src/minimap/textures/menu_cdown_t3x.h"
#include "src/minimap/textures/menu_cup_t3x.h"

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
void gfx_3ds_menu_draw(float *vertex_buffer, int vertex_offset, bool enabled);
menu_action gfx_3ds_menu_on_touch(int x, int y);

typedef struct {
    float xyzw[4];
    float texcoord[2];
    float rgba[4];
} vertex;


static const vertex vertex_list_color[] =
{
    { {   0.0f,   0.0f, 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { 320.0f, 240.0f, 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { 320.0f,   0.0f, 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },

    { {   0.0f,   0.0f, 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {   0.0f, 240.0f, 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { 320.0f, 240.0f, 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }
};

static const vertex vertex_list_button[] =
{
    { {   0.0f,   0.0f, 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  64.0f,  64.0f, 0.5f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  64.0f,   0.0f, 0.5f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },

    { {   0.0f,   0.0f, 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {   0.0f,  64.0f, 0.5f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  64.0f,  64.0f, 0.5f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }
};

static const vertex vertex_list_button_thin[] =
{
    { {   0.0f,   0.0f, 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  64.0f,  32.0f, 0.5f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  64.0f,   0.0f, 0.5f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },

    { {   0.0f,   0.0f, 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {   0.0f,  32.0f, 0.5f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  64.0f,  32.0f, 0.5f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }
};

#endif
