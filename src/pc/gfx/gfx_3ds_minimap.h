#ifndef GFX_3DS_MINIMAP_H
#define GFX_3DS_MINIMAP_H

#include "gfx_3ds_common.h"

#include "src/minimap/minimap.h"

/* mario head */
#include "src/minimap/textures/mario_t3x.h"
/* heading */
#include "src/minimap/textures/arrow_t3x.h"

static const float mario_sprite_size = 16.0f / 2.0f; // 16px but 8px left/right of 0
static const float arrow_sprite_size = 8.0f / 2.0f;  // 8px but 4px left/right of 0

static const vertex vertex_list_mario[] =
{
    { { -mario_sprite_size, -mario_sprite_size, 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  mario_sprite_size,  mario_sprite_size, 0.5f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  mario_sprite_size, -mario_sprite_size, 0.5f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },

    { { -mario_sprite_size, -mario_sprite_size, 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -mario_sprite_size,  mario_sprite_size, 0.5f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  mario_sprite_size,  mario_sprite_size, 0.5f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }
};

static const vertex vertex_list_arrow[] =
{
    { { -arrow_sprite_size, -arrow_sprite_size, 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  arrow_sprite_size,  arrow_sprite_size, 0.5f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  arrow_sprite_size, -arrow_sprite_size, 0.5f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },

    { { -arrow_sprite_size, -arrow_sprite_size, 0.5f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -arrow_sprite_size,  arrow_sprite_size, 0.5f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  arrow_sprite_size,  arrow_sprite_size, 0.5f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }
};

void gfx_3ds_init_minimap();
uint32_t gfx_3ds_draw_minimap(float *vertex_buffer, int vertex_offset);

#endif
