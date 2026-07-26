#ifndef PLAYBOX_PB_3D_H
#define PLAYBOX_PB_3D_H

#include "pb_export.h"
#include "pb_fb.h"
#include "pb_math.h"
#include "pb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Soft 3D raster into terminal pixel modes (braille / half / quad / cell).
 * Depth-buffered; flat-shaded triangles with one directional light. */

typedef enum {
    PB_3D_BRAILLE = 0, /* 2x4 dots per cell */
    PB_3D_HALF    = 1, /* half-block: w x (h*2) */
    PB_3D_QUAD    = 2, /* 2x2 per cell */
    PB_3D_CELL    = 3  /* 1x1 cell "pixels" */
} pb_3d_mode;

typedef struct {
    pb_vec3 position;
    pb_vec3 target;
    pb_vec3 up;
    float   fovy;   /* radians */
    float   znear;
    float   zfar;
} pb_camera3d;

typedef struct {
    pb_vec3* positions; /* count verts */
    pb_color* colors;   /* optional per-vert; may be NULL */
    int* indices;       /* triangles: 3 * tri_count indices */
    int vert_count;
    int index_count;
} pb_mesh;

typedef struct pb_3d pb_3d;

PB_API pb_camera3d pb_camera3d_default(void);

PB_API pb_3d* pb_3d_create(void);
PB_API void   pb_3d_destroy(pb_3d* ctx);

/* Bind framebuffer + mode + camera for this frame. Clears depth buffer.
 * Call once per frame before drawing; end with pb_3d_end. */
PB_API int  pb_3d_begin(pb_3d* ctx, pb_fb* fb, pb_3d_mode mode, const pb_camera3d* cam);
PB_API void pb_3d_end(pb_3d* ctx);

PB_API void pb_3d_set_light(pb_3d* ctx, pb_vec3 direction, float ambient);
PB_API void pb_3d_clear_color(pb_3d* ctx, pb_color bg); /* optional clear of pixel layer */

PB_API int pb_3d_pixel_width(const pb_3d* ctx);
PB_API int pb_3d_pixel_height(const pb_3d* ctx);
PB_API pb_mat4 pb_3d_view_proj(const pb_3d* ctx);

PB_API void pb_3d_plot(pb_3d* ctx, int px, int py, float z, pb_color color);
PB_API void pb_3d_line(pb_3d* ctx, pb_vec3 a, pb_vec3 b, pb_color color);
PB_API void pb_3d_triangle(pb_3d* ctx, pb_vec3 a, pb_vec3 b, pb_vec3 c, pb_color color, int wire);
PB_API void pb_3d_cube(pb_3d* ctx, pb_vec3 center, pb_vec3 size, pb_mat4 model, pb_color color, int wire);
PB_API void pb_3d_grid(pb_3d* ctx, float half_extent, float step, pb_color color);
PB_API void pb_3d_mesh(pb_3d* ctx, const pb_mesh* mesh, pb_mat4 model, pb_color fallback, int wire);

#ifdef __cplusplus
}
#endif

#endif
