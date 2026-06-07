#include "engine/surface_collision.h"
#include "engine/surface_load.h"
#include "engine/math_util.h"

//#define EXT_BOUNDS

/**
 * Raycast functions
 */
s32 ray_surface_intersect(Vec3f orig, Vec3f dir, f32 dir_length, struct Surface *surface, Vec3f hit_pos, f32 *length)
{
    Vec3f v0, v1, v2, e1, e2, h, s, q;
    f32 a, f, u, v;
    Vec3f add_dir;

    // Get surface normal and some other stuff
    vec3s_to_vec3f(v0, surface->vertex1);
    vec3s_to_vec3f(v1, surface->vertex2);
    vec3s_to_vec3f(v2, surface->vertex3);

    vec3f_dif(e1, v1, v0);
    vec3f_dif(e2, v2, v0);

    vec3f_cross(h, dir, e2);

    // Check if we're perpendicular from the surface
    a = vec3f_dot(e1, h);
    if (a > -0.00001f && a < 0.00001f)
        return FALSE;

    // Check if we're making contact with the surface
    f = 1.0f / a;

    vec3f_dif(s, orig, v0);
    u = f * vec3f_dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return FALSE;

    vec3f_cross(q, s, e1);
    v = f * vec3f_dot(dir, q);
    if (v < 0.0f || u + v > 1.0f)
        return FALSE;

    // Get the length between our origin and the surface contact point
    *length = f * vec3f_dot(e2, q);
    if (*length <= 0.00001 || *length > dir_length)
        return FALSE;

    // Successful contact
    vec3f_copy(add_dir, dir);
    vec3f_mul(add_dir, *length);
    vec3f_sum(hit_pos, orig, add_dir);
    return TRUE;
}

void find_surface_on_ray_list(struct SurfaceNode *list, Vec3f orig, Vec3f dir, f32 dir_length, struct Surface **hit_surface, Vec3f hit_pos, f32 *max_length)
{
    s32 hit;
    f32 length;
    Vec3f chk_hit_pos;
    f32 top, bottom;

    // Get upper and lower bounds of ray
    if (dir[1] >= 0.0f)
    {
        top = orig[1] + dir[1] * dir_length;
        bottom = orig[1];
    }
    else
    {
        top = orig[1];
        bottom = orig[1] + dir[1] * dir_length;
    }

    // Iterate through every surface of the list
    for (; list != NULL; list = list->next)
    {
        // Reject surface if out of vertical bounds
        if (list->surface->lowerY > top || list->surface->upperY < bottom)
            continue;

        // Check intersection between the ray and this surface
        if ((hit = ray_surface_intersect(orig, dir, dir_length, list->surface, chk_hit_pos, &length)) != 0)
        {
            if (length <= *max_length)
            {
                *hit_surface = list->surface;
                vec3f_copy(hit_pos, chk_hit_pos);
                *max_length = length;
            }
        }
    }
}


void find_surface_on_ray_cell(s16 cellX, s16 cellZ, Vec3f orig, Vec3f normalized_dir, f32 dir_length, struct Surface **hit_surface, Vec3f hit_pos, f32 *max_length)
{
	// Skip if OOB
	if (cellX >= 0 && cellX <= 0xF && cellZ >= 0 && cellZ <= 0xF)
	{
		// Iterate through each surface in this partition
		if (normalized_dir[1] > -0.99f)
		{
			find_surface_on_ray_list(gStaticSurfacePartition[cellZ][cellX][SPATIAL_PARTITION_CEILS].next, orig, normalized_dir, dir_length, hit_surface, hit_pos, max_length);
			find_surface_on_ray_list(gDynamicSurfacePartition[cellZ][cellX][SPATIAL_PARTITION_CEILS].next, orig, normalized_dir, dir_length, hit_surface, hit_pos, max_length);
		}
		if (normalized_dir[1] < 0.99f)
		{
			find_surface_on_ray_list(gStaticSurfacePartition[cellZ][cellX][SPATIAL_PARTITION_FLOORS].next, orig, normalized_dir, dir_length, hit_surface, hit_pos, max_length);
			find_surface_on_ray_list(gDynamicSurfacePartition[cellZ][cellX][SPATIAL_PARTITION_FLOORS].next, orig, normalized_dir, dir_length, hit_surface, hit_pos, max_length);
		}
		find_surface_on_ray_list(gStaticSurfacePartition[cellZ][cellX][SPATIAL_PARTITION_WALLS].next, orig, normalized_dir, dir_length, hit_surface, hit_pos, max_length);
		find_surface_on_ray_list(gDynamicSurfacePartition[cellZ][cellX][SPATIAL_PARTITION_WALLS].next, orig, normalized_dir, dir_length, hit_surface, hit_pos, max_length);
	}
}

void find_surface_on_ray(Vec3f orig, Vec3f dir, struct Surface **hit_surface, Vec3f hit_pos)
{
    f32 max_length;
    s16 cellZ, cellX;
    f32 fCellZ, fCellX;
    f32 dir_length;
    Vec3f normalized_dir;
    f32 step, dx, dz;
    u32 i;

    #ifdef EXT_BOUNDS
    orig[0] /= MULTI;
    orig[1] /= MULTI;
    orig[2] /= MULTI;

    dir[0] /= MULTI;
    dir[1] /= MULTI;
    dir[2] /= MULTI;
    #endif // EXT_BOUNDS

    // Set that no surface has been hit
    *hit_surface = NULL;
    vec3f_sum(hit_pos, orig, dir);

    // Get normalized direction
    dir_length = vec3f_length(dir);
    max_length = dir_length;
    vec3f_copy(normalized_dir, dir);
    vec3f_normalize(normalized_dir);

    // Get our cell coordinate
    fCellX = (orig[0] + LEVEL_BOUNDARY_MAX) / CELL_SIZE;
    fCellZ = (orig[2] + LEVEL_BOUNDARY_MAX) / CELL_SIZE;
    cellX = (s16)fCellX;
    cellZ = (s16)fCellZ;

    // Don't do DDA if straight down
    if (normalized_dir[1] >= 0.99999f || normalized_dir[1] <= -0.99999f)
    {
		find_surface_on_ray_cell(cellX, cellZ, orig, normalized_dir, dir_length, hit_surface, hit_pos, &max_length);
		return;
	}

    // Get cells we cross using DDA
    if (ABS(dir[0]) >= ABS(dir[2]))
        step = ABS(dir[0]) / CELL_SIZE;
    else
        step = ABS(dir[2]) / CELL_SIZE;

    dx = dir[0] / step / CELL_SIZE;
    dz = dir[2] / step / CELL_SIZE;

    for (i = 0; i < step && *hit_surface == NULL; i++)
    {
		find_surface_on_ray_cell(cellX, cellZ, orig, normalized_dir, dir_length, hit_surface, hit_pos, &max_length);

        // Move cell coordinate
        fCellX += dx;
        fCellZ += dz;
        cellX = (s16)fCellX;
        cellZ = (s16)fCellZ;
    }

    #ifdef EXT_BOUNDS
    hit_pos[0] *= MULTI;
    hit_pos[1] *= MULTI;
    hit_pos[2] *= MULTI;
    #endif // EXT_BOUNDS
}
