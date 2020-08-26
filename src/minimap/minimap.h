#ifndef GFX_MINIMAP_H
#define GFX_MINIMAP_H

#include <stdbool.h>

bool minimap_has_level_or_area_changed();
bool minimap_load_level_and_area();
void minimap_get_current_texture(uint8_t **texture, size_t *texture_size, uint32_t *color);
bool minimap_get_mario_position(float *mario_x, float *mario_y, float *mario_direction);

#endif
