#include "playbox/pb_3d.h"
#include "playbox/pb_gfx.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef PB_3D_W_EPS
#define PB_3D_W_EPS 1e-4f
#endif

struct pb_3d {
    pb_fb* fb;
    pb_3d_mode mode;
    pb_camera3d cam;
    pb_mat4 view;
    pb_mat4 vp;
    float* depth;
    int pw, ph;
    int depth_cap;
    pb_vec3 light_dir;
    float ambient;
    int active;
};

typedef struct {
    float x, y, z, w;
} pb_clipv;

typedef struct {
    float x, y;
    float z;
    float inv_w;
} pb_screenv;

pb_camera3d pb_camera3d_default(void){
    pb_camera3d c;
    c.position = pb_v3(0, 2.5f, 6.0f);
    c.target   = pb_v3(0, 0, 0);
    c.up       = pb_v3(0, 1, 0);
    c.fovy     = 50.0f * PB_DEG2RAD;
    c.znear    = 0.2f;
    c.zfar     = 200.0f;
    return c;
}

pb_3d* pb_3d_create(void){
    pb_3d* ctx = (pb_3d*)calloc(1, sizeof(pb_3d));
    if(!ctx) return NULL;
    ctx->light_dir = pb_v3_normalize(pb_v3(0.4f, 1.0f, 0.35f));
    ctx->ambient = 0.35f;
    return ctx;
}

void pb_3d_destroy(pb_3d* ctx){
    if(!ctx) return;
    free(ctx->depth);
    free(ctx);
}

static void pb_3d_mode_dims(const pb_fb* fb, pb_3d_mode mode, int* pw, int* ph){
    int w = fb ? fb->w : 0;
    int h = fb ? fb->h : 0;
    switch(mode){
        case PB_3D_BRAILLE: *pw = w * 2; *ph = h * 4; break;
        case PB_3D_HALF:    *pw = w;     *ph = h * 2; break;
        case PB_3D_QUAD:    *pw = w * 2; *ph = h * 2; break;
        case PB_3D_CELL:
        default:            *pw = w;     *ph = h; break;
    }
}

int pb_3d_begin(pb_3d* ctx, pb_fb* fb, pb_3d_mode mode, const pb_camera3d* cam){
    if(!ctx || !fb || !cam) return 0;
    ctx->fb = fb;
    ctx->mode = mode;
    ctx->cam = *cam;
    if(ctx->cam.znear < 0.05f) ctx->cam.znear = 0.05f;
    if(ctx->cam.zfar <= ctx->cam.znear + 1.f) ctx->cam.zfar = ctx->cam.znear + 100.f;
    pb_3d_mode_dims(fb, mode, &ctx->pw, &ctx->ph);
    if(ctx->pw < 1 || ctx->ph < 1) return 0;

    int need = ctx->pw * ctx->ph;
    if(need > ctx->depth_cap){
        float* nd = (float*)realloc(ctx->depth, (size_t)need * sizeof(float));
        if(!nd) return 0;
        ctx->depth = nd;
        ctx->depth_cap = need;
    }
    for(int i = 0; i < need; i++) ctx->depth[i] = 1.0f;

    float aspect = (float)ctx->pw / (float)ctx->ph;
    if(aspect < 0.05f) aspect = 0.05f;
    ctx->view = pb_m4_look_at(ctx->cam.position, ctx->cam.target, ctx->cam.up);
    pb_mat4 proj = pb_m4_perspective(ctx->cam.fovy, aspect, ctx->cam.znear, ctx->cam.zfar);
    ctx->vp = pb_m4_mul(proj, ctx->view);
    ctx->active = 1;
    return 1;
}

void pb_3d_end(pb_3d* ctx){
    if(!ctx) return;
    ctx->active = 0;
    ctx->fb = NULL;
}

void pb_3d_set_light(pb_3d* ctx, pb_vec3 direction, float ambient){
    if(!ctx) return;
    float len = pb_v3_len(direction);
    ctx->light_dir = len > 1e-6f ? pb_v3_normalize(direction) : pb_v3(0,1,0);
    ctx->ambient = pb_clampf(ambient, 0.f, 1.f);
}

void pb_3d_clear_color(pb_3d* ctx, pb_color bg){
    if(!ctx || !ctx->fb || !ctx->active) return;
    switch(ctx->mode){
        case PB_3D_BRAILLE:
            pb_fb_braille_clear(ctx->fb, bg);
            break;
        default:
            pb_fb_clear(ctx->fb, pb_cell_make(' ', pb_rgb(200,200,200), bg, 0));
            break;
    }
}

int pb_3d_pixel_width(const pb_3d* ctx){ return ctx ? ctx->pw : 0; }
int pb_3d_pixel_height(const pb_3d* ctx){ return ctx ? ctx->ph : 0; }
pb_mat4 pb_3d_view_proj(const pb_3d* ctx){ return ctx ? ctx->vp : pb_m4_identity(); }

static void pb_3d_write_pixel(pb_3d* ctx, int px, int py, pb_color color){
    switch(ctx->mode){
        case PB_3D_BRAILLE: pb_fb_braille_plot(ctx->fb, px, py, color); break;
        case PB_3D_HALF:    pb_fb_plot(ctx->fb, px, py, color); break;
        case PB_3D_QUAD:    pb_fb_quad_plot(ctx->fb, px, py, color); break;
        default:
            pb_fb_put(ctx->fb, px, py, pb_cell_make(0x2588, color, pb_rgb(0,0,0), 0));
            break;
    }
}

void pb_3d_plot(pb_3d* ctx, int px, int py, float z, pb_color color){
    if(!ctx || !ctx->active || !ctx->depth) return;
    if(px < 0 || py < 0 || px >= ctx->pw || py >= ctx->ph) return;
    if(z < 0.f) z = 0.f;
    if(z > 1.f) z = 1.f;
    int i = py * ctx->pw + px;
    if(z >= ctx->depth[i]) return;
    ctx->depth[i] = z;
    pb_3d_write_pixel(ctx, px, py, color);
}

static pb_clipv pb_3d_world_to_clip(pb_3d* ctx, pb_vec3 world){
    pb_vec4 c = pb_m4_mul_v4(ctx->vp, pb_v4(world.x, world.y, world.z, 1.0f));
    pb_clipv v = {c.x, c.y, c.z, c.w};
    return v;
}

static pb_clipv pb_clipv_lerp(pb_clipv a, pb_clipv b, float t){
    pb_clipv o;
    o.x = a.x + (b.x - a.x) * t;
    o.y = a.y + (b.y - a.y) * t;
    o.z = a.z + (b.z - a.z) * t;
    o.w = a.w + (b.w - a.w) * t;
    return o;
}

static int pb_3d_clip_w(const pb_clipv* in, int nin, pb_clipv* out, float eps){
    if(nin < 1) return 0;
    int nout = 0;
    pb_clipv prev = in[nin - 1];
    int prev_in = prev.w >= eps;
    for(int i = 0; i < nin; i++){
        pb_clipv cur = in[i];
        int cur_in = cur.w >= eps;
        if(cur_in != prev_in){
            float dw = cur.w - prev.w;
            float t = (fabsf(dw) > 1e-12f) ? (eps - prev.w) / dw : 0.f;
            t = pb_clampf(t, 0.f, 1.f);
            if(nout < 16) out[nout++] = pb_clipv_lerp(prev, cur, t);
        }
        if(cur_in && nout < 16) out[nout++] = cur;
        prev = cur;
        prev_in = cur_in;
    }
    return nout;
}

static int pb_3d_to_screen(pb_3d* ctx, pb_clipv c, pb_screenv* out){
    if(c.w < PB_3D_W_EPS) return 0;
    float iw = 1.0f / c.w;
    float ndc_x = c.x * iw;
    float ndc_y = c.y * iw;
    float ndc_z = c.z * iw;
    out->x = (ndc_x * 0.5f + 0.5f) * (float)ctx->pw;
    out->y = (1.0f - (ndc_y * 0.5f + 0.5f)) * (float)ctx->ph;
    out->z = pb_clampf(ndc_z * 0.5f + 0.5f, 0.f, 1.f);
    out->inv_w = iw;
    return 1;
}

static pb_color pb_3d_shade(pb_3d* ctx, pb_vec3 n, pb_color base){
    pb_vec3 nn = pb_v3_normalize(n);
    float ndl = pb_v3_dot(nn, ctx->light_dir);
    if(ndl < 0.f) ndl = 0.f;
    float lit = ctx->ambient + (1.0f - ctx->ambient) * ndl;
    lit = pb_clampf(lit, 0.2f, 1.0f);
    return pb_color_fade(base, lit);
}

static float pb_3d_edge(float ax, float ay, float bx, float by, float cx, float cy){
    return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
}

static void pb_3d_fill_tri(pb_3d* ctx, pb_screenv a, pb_screenv b, pb_screenv c, pb_color color){
    float area = pb_3d_edge(a.x, a.y, b.x, b.y, c.x, c.y);
    if(fabsf(area) < 1e-4f) return;
    if(area < 0.f){
        pb_screenv tmp = b; b = c; c = tmp;
        area = -area;
    }
    float inv_area = 1.0f / area;

    int minx = (int)floorf(fminf(a.x, fminf(b.x, c.x)));
    int maxx = (int)ceilf (fmaxf(a.x, fmaxf(b.x, c.x)));
    int miny = (int)floorf(fminf(a.y, fminf(b.y, c.y)));
    int maxy = (int)ceilf (fmaxf(a.y, fmaxf(b.y, c.y)));
    if(minx < 0) minx = 0;
    if(miny < 0) miny = 0;
    if(maxx >= ctx->pw) maxx = ctx->pw - 1;
    if(maxy >= ctx->ph) maxy = ctx->ph - 1;
    if(minx > maxx || miny > maxy) return;

    for(int y = miny; y <= maxy; y++){
        float py = (float)y + 0.5f;
        for(int x = minx; x <= maxx; x++){
            float px = (float)x + 0.5f;
            float w0 = pb_3d_edge(b.x, b.y, c.x, c.y, px, py) * inv_area;
            float w1 = pb_3d_edge(c.x, c.y, a.x, a.y, px, py) * inv_area;
            float w2 = pb_3d_edge(a.x, a.y, b.x, b.y, px, py) * inv_area;
            if(w0 < 0.f || w1 < 0.f || w2 < 0.f) continue;
            float inv_w = w0 * a.inv_w + w1 * b.inv_w + w2 * c.inv_w;
            float z = (inv_w > 1e-8f)
                ? (w0 * (a.z * a.inv_w) + w1 * (b.z * b.inv_w) + w2 * (c.z * c.inv_w)) / inv_w
                : (w0 * a.z + w1 * b.z + w2 * c.z);
            pb_3d_plot(ctx, x, y, z, color);
        }
    }
}

static void pb_3d_draw_clipped_poly(pb_3d* ctx, pb_clipv* poly, int n, pb_color color){
    if(n < 3) return;
    pb_screenv s[16];
    int ns = 0;
    for(int i = 0; i < n && ns < 16; i++){
        if(pb_3d_to_screen(ctx, poly[i], &s[ns])) ns++;
    }
    if(ns < 3) return;
    for(int i = 1; i + 1 < ns; i++)
        pb_3d_fill_tri(ctx, s[0], s[i], s[i + 1], color);
}

void pb_3d_line(pb_3d* ctx, pb_vec3 a, pb_vec3 b, pb_color color){
    if(!ctx || !ctx->active) return;
    pb_clipv in[2] = { pb_3d_world_to_clip(ctx, a), pb_3d_world_to_clip(ctx, b) };
    pb_clipv out[4];
    int n = pb_3d_clip_w(in, 2, out, PB_3D_W_EPS);
    if(n < 2) return;

    pb_screenv sa, sb;
    if(!pb_3d_to_screen(ctx, out[0], &sa)) return;
    if(!pb_3d_to_screen(ctx, out[n - 1], &sb)) return;

    int x0 = (int)floorf(sa.x), y0 = (int)floorf(sa.y);
    int x1 = (int)floorf(sb.x), y1 = (int)floorf(sb.y);
    float z0 = sa.z, z1 = sb.z;
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int steps = dx > -dy ? dx : -dy;
    if(steps < 1) steps = 1;
    int i = 0;
    for(;;){
        float t = (float)i / (float)steps;
        pb_3d_plot(ctx, x0, y0, z0 + (z1 - z0) * t, color);
        if(x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if(e2 >= dy){ err += dy; x0 += sx; }
        if(e2 <= dx){ err += dx; y0 += sy; }
        i++;
        if(i > steps + 2) break;
    }
}

void pb_3d_triangle(pb_3d* ctx, pb_vec3 a, pb_vec3 b, pb_vec3 c, pb_color color, int wire){
    if(!ctx || !ctx->active) return;
    if(wire){
        pb_3d_line(ctx, a, b, color);
        pb_3d_line(ctx, b, c, color);
        pb_3d_line(ctx, c, a, color);
        return;
    }

    pb_vec3 n = pb_v3_cross(pb_v3_sub(b, a), pb_v3_sub(c, a));
    if(pb_v3_dot(n, n) < 1e-12f) return;

    /* Outward-normal back-face cull */
    if(pb_v3_dot(n, pb_v3_sub(ctx->cam.position, a)) <= 0.f) return;

    pb_clipv in[3] = {
        pb_3d_world_to_clip(ctx, a),
        pb_3d_world_to_clip(ctx, b),
        pb_3d_world_to_clip(ctx, c)
    };
    pb_clipv clipped[16];
    int nclip = pb_3d_clip_w(in, 3, clipped, PB_3D_W_EPS);
    if(nclip < 3) return;

    pb_3d_draw_clipped_poly(ctx, clipped, nclip, pb_3d_shade(ctx, n, color));
}

void pb_3d_cube(pb_3d* ctx, pb_vec3 center, pb_vec3 size, pb_mat4 model, pb_color color, int wire){
    if(!ctx || !ctx->active) return;
    float hx = size.x * 0.5f, hy = size.y * 0.5f, hz = size.z * 0.5f;
    pb_vec3 local[8] = {
        pb_v3(-hx,-hy,-hz), pb_v3( hx,-hy,-hz), pb_v3( hx, hy,-hz), pb_v3(-hx, hy,-hz),
        pb_v3(-hx,-hy, hz), pb_v3( hx,-hy, hz), pb_v3( hx, hy, hz), pb_v3(-hx, hy, hz)
    };
    pb_mat4 xform = pb_m4_mul(pb_m4_translate(center), model);

    pb_vec3 w[8];
    int any_front = 0;
    for(int i = 0; i < 8; i++){
        w[i] = pb_m4_mul_v3(xform, local[i], 1.f);
        if(pb_3d_world_to_clip(ctx, w[i]).w >= PB_3D_W_EPS) any_front = 1;
    }
    if(!any_front) return;

    static const int faces[6][4] = {
        {0, 3, 2, 1}, {4, 5, 6, 7}, {0, 4, 7, 3},
        {1, 2, 6, 5}, {3, 7, 6, 2}, {0, 1, 5, 4}
    };

    if(wire){
        static const int edges[12][2] = {
            {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}
        };
        for(int e = 0; e < 12; e++)
            pb_3d_line(ctx, w[edges[e][0]], w[edges[e][1]], color);
        return;
    }

    for(int f = 0; f < 6; f++){
        pb_3d_triangle(ctx, w[faces[f][0]], w[faces[f][1]], w[faces[f][2]], color, 0);
        pb_3d_triangle(ctx, w[faces[f][0]], w[faces[f][2]], w[faces[f][3]], color, 0);
    }
}

void pb_3d_grid(pb_3d* ctx, float half_extent, float step, pb_color color){
    if(!ctx || !ctx->active || step <= 0.f) return;
    for(float x = -half_extent; x <= half_extent + 1e-4f; x += step){
        pb_3d_line(ctx, pb_v3(x, 0, -half_extent), pb_v3(x, 0, half_extent), color);
        pb_3d_line(ctx, pb_v3(-half_extent, 0, x), pb_v3(half_extent, 0, x), color);
    }
}

void pb_3d_mesh(pb_3d* ctx, const pb_mesh* mesh, pb_mat4 model, pb_color fallback, int wire){
    if(!ctx || !ctx->active || !mesh || !mesh->positions || !mesh->indices) return;
    for(int i = 0; i + 2 < mesh->index_count; i += 3){
        int i0 = mesh->indices[i], i1 = mesh->indices[i+1], i2 = mesh->indices[i+2];
        if(i0 < 0 || i1 < 0 || i2 < 0) continue;
        if(i0 >= mesh->vert_count || i1 >= mesh->vert_count || i2 >= mesh->vert_count) continue;
        pb_vec3 a = pb_m4_mul_v3(model, mesh->positions[i0], 1.f);
        pb_vec3 b = pb_m4_mul_v3(model, mesh->positions[i1], 1.f);
        pb_vec3 c = pb_m4_mul_v3(model, mesh->positions[i2], 1.f);
        pb_color col = fallback;
        if(mesh->colors){
            pb_color c0 = mesh->colors[i0], c1 = mesh->colors[i1], c2 = mesh->colors[i2];
            col = pb_rgb((uint8_t)((c0.r+c1.r+c2.r)/3),
                         (uint8_t)((c0.g+c1.g+c2.g)/3),
                         (uint8_t)((c0.b+c1.b+c2.b)/3));
        }
        pb_3d_triangle(ctx, a, b, c, col, wire);
    }
}
