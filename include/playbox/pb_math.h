#ifndef PLAYBOX_PB_MATH_H
#define PLAYBOX_PB_MATH_H

#include "pb_export.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PB_PI
#define PB_PI 3.14159265358979323846f
#endif
#ifndef PB_DEG2RAD
#define PB_DEG2RAD (PB_PI / 180.0f)
#endif
#ifndef PB_RAD2DEG
#define PB_RAD2DEG (180.0f / PB_PI)
#endif

typedef struct { float x, y; } pb_vec2;
typedef struct { float x, y, z; } pb_vec3;
typedef struct { float x, y, z, w; } pb_vec4;

/* Column-major 4x4, m[col*4 + row] */
typedef struct { float m[16]; } pb_mat4;

/* ---- Scalar ---- */
static inline float pb_clampf(float v, float lo, float hi){
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}
static inline float pb_lerpf(float a, float b, float t){ return a + (b - a) * t; }
static inline float pb_absf(float v){ return v < 0 ? -v : v; }

/* ---- Vec2 ---- */
static inline pb_vec2 pb_v2(float x, float y){ pb_vec2 v = {x,y}; return v; }
static inline pb_vec2 pb_v2_add(pb_vec2 a, pb_vec2 b){ return pb_v2(a.x+b.x, a.y+b.y); }
static inline pb_vec2 pb_v2_sub(pb_vec2 a, pb_vec2 b){ return pb_v2(a.x-b.x, a.y-b.y); }
static inline pb_vec2 pb_v2_scale(pb_vec2 a, float s){ return pb_v2(a.x*s, a.y*s); }
static inline float   pb_v2_dot(pb_vec2 a, pb_vec2 b){ return a.x*b.x + a.y*b.y; }
static inline float   pb_v2_len(pb_vec2 a){ return sqrtf(pb_v2_dot(a,a)); }
static inline pb_vec2 pb_v2_normalize(pb_vec2 a){
    float l = pb_v2_len(a);
    return l > 1e-8f ? pb_v2_scale(a, 1.0f/l) : pb_v2(0,0);
}
static inline pb_vec2 pb_v2_lerp(pb_vec2 a, pb_vec2 b, float t){
    return pb_v2(pb_lerpf(a.x,b.x,t), pb_lerpf(a.y,b.y,t));
}

/* ---- Vec3 ---- */
static inline pb_vec3 pb_v3(float x, float y, float z){ pb_vec3 v = {x,y,z}; return v; }
static inline pb_vec3 pb_v3_add(pb_vec3 a, pb_vec3 b){ return pb_v3(a.x+b.x, a.y+b.y, a.z+b.z); }
static inline pb_vec3 pb_v3_sub(pb_vec3 a, pb_vec3 b){ return pb_v3(a.x-b.x, a.y-b.y, a.z-b.z); }
static inline pb_vec3 pb_v3_scale(pb_vec3 a, float s){ return pb_v3(a.x*s, a.y*s, a.z*s); }
static inline float   pb_v3_dot(pb_vec3 a, pb_vec3 b){ return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline pb_vec3 pb_v3_cross(pb_vec3 a, pb_vec3 b){
    return pb_v3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}
static inline float   pb_v3_len(pb_vec3 a){ return sqrtf(pb_v3_dot(a,a)); }
static inline pb_vec3 pb_v3_normalize(pb_vec3 a){
    float l = pb_v3_len(a);
    return l > 1e-8f ? pb_v3_scale(a, 1.0f/l) : pb_v3(0,0,0);
}
static inline pb_vec3 pb_v3_lerp(pb_vec3 a, pb_vec3 b, float t){
    return pb_v3(pb_lerpf(a.x,b.x,t), pb_lerpf(a.y,b.y,t), pb_lerpf(a.z,b.z,t));
}

/* ---- Vec4 ---- */
static inline pb_vec4 pb_v4(float x, float y, float z, float w){ pb_vec4 v = {x,y,z,w}; return v; }

/* ---- Mat4 (non-inline heavier ops) ---- */
PB_API pb_mat4 pb_m4_identity(void);
PB_API pb_mat4 pb_m4_translate(pb_vec3 t);
PB_API pb_mat4 pb_m4_scale(pb_vec3 s);
PB_API pb_mat4 pb_m4_rotate_x(float rad);
PB_API pb_mat4 pb_m4_rotate_y(float rad);
PB_API pb_mat4 pb_m4_rotate_z(float rad);
PB_API pb_mat4 pb_m4_mul(pb_mat4 a, pb_mat4 b);
PB_API pb_vec3 pb_m4_mul_v3(pb_mat4 m, pb_vec3 v, float w);
PB_API pb_vec4 pb_m4_mul_v4(pb_mat4 m, pb_vec4 v);
PB_API pb_mat4 pb_m4_look_at(pb_vec3 eye, pb_vec3 target, pb_vec3 up);
PB_API pb_mat4 pb_m4_perspective(float fovy_rad, float aspect, float znear, float zfar);
PB_API pb_mat4 pb_m4_ortho(float left, float right, float bottom, float top, float znear, float zfar);

/* Project world point through VP matrix into pixel coords.
 * out_px/out_py are in the chosen pixel space; out_z is NDC depth (0..1 after perspective divide).
 * Returns 0 if behind camera (w <= 0). */
PB_API int pb_math_project(pb_mat4 vp, pb_vec3 world,
                           int pixel_w, int pixel_h,
                           int* out_px, int* out_py, float* out_z);

#ifdef __cplusplus
}
#endif

#endif
