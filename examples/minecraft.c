#include "playbox/pb.h"
#include "playbox/pb_math.h"
#include "playbox/pb_3d.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---- World ---- */
#define WX 40
#define WY 28
#define WZ 40
#define RENDER_R 10
#define EYE_H 1.62f
#define PLAYER_W 0.3f
#define PLAYER_H 1.8f

enum {
    BLK_AIR = 0,
    BLK_GRASS,
    BLK_DIRT,
    BLK_STONE,
    BLK_SAND,
    BLK_WATER,
    BLK_LOG,
    BLK_LEAVES,
    BLK_COBBLE,
    BLK_PLANKS,
    BLK_BEDROCK,
    BLK_COUNT
};

static const char* BLK_NAME[BLK_COUNT] = {
    "Air", "Grass", "Dirt", "Stone", "Sand", "Water",
    "Log", "Leaves", "Cobble", "Planks", "Bedrock"
};

static pb_color blk_color(int id, int face){
    /* face: 0=+Y 1=-Y 2=side */
    pb_color c;
    switch(id){
        case BLK_GRASS:  c = (face==0) ? pb_rgb(90,170,70) : (face==1 ? pb_rgb(110,80,50) : pb_rgb(100,140,60)); break;
        case BLK_DIRT:   c = pb_rgb(120,85,55); break;
        case BLK_STONE:  c = pb_rgb(120,120,125); break;
        case BLK_SAND:   c = pb_rgb(210,200,140); break;
        case BLK_WATER:  c = pb_rgb(50,100,200); break;
        case BLK_LOG:    c = (face==0||face==1) ? pb_rgb(90,70,40) : pb_rgb(100,75,45); break;
        case BLK_LEAVES: c = pb_rgb(50,130,50); break;
        case BLK_COBBLE: c = pb_rgb(100,100,105); break;
        case BLK_PLANKS: c = pb_rgb(180,140,80); break;
        case BLK_BEDROCK:c = pb_rgb(40,40,45); break;
        default:         c = pb_rgb(255,0,255); break;
    }
    if(face == 2){
        c.r = (uint8_t)((int)c.r * 85 / 100);
        c.g = (uint8_t)((int)c.g * 85 / 100);
        c.b = (uint8_t)((int)c.b * 85 / 100);
    } else if(face == 1){
        c.r = (uint8_t)((int)c.r * 65 / 100);
        c.g = (uint8_t)((int)c.g * 65 / 100);
        c.b = (uint8_t)((int)c.b * 65 / 100);
    }
    return c;
}

static int blk_solid(int id){
    return id != BLK_AIR && id != BLK_WATER;
}

static int blk_collide(int id){
    return id != BLK_AIR && id != BLK_WATER && id != BLK_LEAVES;
}

/* ---- Game ---- */
enum { ST_PLAY = 0, ST_PAUSE };

typedef struct {
    pb_3d* gfx;
    uint8_t world[WY][WZ][WX];
    int state;

    float px, py, pz;
    float vx, vy, vz;
    float yaw, pitch;
    int on_ground;
    int flying;

    int hotbar[9];
    int slot;
    int breaking;
    float break_t;

    int look_bx, look_by, look_bz;
    int place_bx, place_by, place_bz;
    int has_look, has_place;

    unsigned seed;
    float bob;
    int pause_sel; /* 0 = resume, 1 = quit */
} mc_t;

static float clampf(float v, float lo, float hi){
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

static float hash2(int x, int z, unsigned seed){
    unsigned n = (unsigned)(x * 374761393 + z * 668265263) ^ seed;
    n = (n ^ (n >> 13)) * 1274126177u;
    return (float)(n & 0xffffu) / 65535.0f;
}

static float smooth_noise(float x, float z, unsigned seed){
    int x0 = (int)floorf(x), z0 = (int)floorf(z);
    float fx = x - (float)x0, fz = z - (float)z0;
    float a = hash2(x0, z0, seed);
    float b = hash2(x0+1, z0, seed);
    float c = hash2(x0, z0+1, seed);
    float d = hash2(x0+1, z0+1, seed);
    float ux = fx * fx * (3.f - 2.f * fx);
    float uz = fz * fz * (3.f - 2.f * fz);
    return pb_lerpf(pb_lerpf(a, b, ux), pb_lerpf(c, d, ux), uz);
}

static float fbm(float x, float z, unsigned seed){
    float v = 0, a = 1, f = 1, n = 0;
    for(int i = 0; i < 4; i++){
        v += smooth_noise(x * f, z * f, seed + (unsigned)i * 1013u) * a;
        n += a; a *= 0.5f; f *= 2.f;
    }
    return v / n;
}

static int in_world(int x, int y, int z){
    return x >= 0 && y >= 0 && z >= 0 && x < WX && y < WY && z < WZ;
}

static int get_block(const mc_t* g, int x, int y, int z){
    if(!in_world(x,y,z)) return BLK_AIR;
    return g->world[y][z][x];
}

static void set_block(mc_t* g, int x, int y, int z, int id){
    if(!in_world(x,y,z)) return;
    if(get_block(g, x,y,z) == BLK_BEDROCK && id == BLK_AIR) return;
    g->world[y][z][x] = (uint8_t)id;
}

static void plant_tree(mc_t* g, int x, int y, int z){
    int h = 4 + (int)(hash2(x, z, g->seed) * 3.f);
    for(int i = 0; i < h; i++)
        if(in_world(x, y+i, z)) set_block(g, x, y+i, z, BLK_LOG);
    int top = y + h;
    for(int dy = -2; dy <= 2; dy++)
        for(int dz = -2; dz <= 2; dz++)
            for(int dx = -2; dx <= 2; dx++){
                if(abs(dx)+abs(dy)+abs(dz) > 4) continue;
                int bx = x+dx, by = top+dy, bz = z+dz;
                if(!in_world(bx,by,bz)) continue;
                if(get_block(g, bx,by,bz) == BLK_AIR)
                    set_block(g, bx, by, bz, BLK_LEAVES);
            }
}

static void gen_world(mc_t* g){
    memset(g->world, 0, sizeof(g->world));
    int water = 8;
    for(int z = 0; z < WZ; z++){
        for(int x = 0; x < WX; x++){
            float n = fbm((float)x * 0.08f, (float)z * 0.08f, g->seed);
            int h = 6 + (int)(n * 12.f);
            if(h < 3) h = 3;
            if(h >= WY - 2) h = WY - 3;
            for(int y = 0; y < WY; y++){
                int id = BLK_AIR;
                if(y == 0) id = BLK_BEDROCK;
                else if(y < h - 3) id = BLK_STONE;
                else if(y < h) id = BLK_DIRT;
                else if(y == h){
                    if(h <= water + 1) id = BLK_SAND;
                    else id = BLK_GRASS;
                } else if(y <= water) id = BLK_WATER;
                g->world[y][z][x] = (uint8_t)id;
            }
            if(h > water + 2 && hash2(x, z, g->seed ^ 0xbeefu) > 0.97f)
                plant_tree(g, x, h + 1, z);
        }
    }
    /* Spawn on highest grass near center */
    int sx = WX / 2, sz = WZ / 2;
    int sy = 1;
    for(int y = WY - 2; y > 0; y--){
        if(blk_solid(get_block(g, sx, y, sz))){ sy = y + 1; break; }
    }
    g->px = (float)sx + 0.5f;
    g->pz = (float)sz + 0.5f;
    g->py = (float)sy + 0.1f;
    g->yaw = 0.f;
    g->pitch = 0.f;
    g->vx = g->vy = g->vz = 0;
}

static void pause_game(pb_app* app, mc_t* g){
    (void)app;
    g->state = ST_PAUSE;
    g->vx = g->vy = g->vz = 0;
    g->pause_sel = 0;
}

static void resume_game(pb_app* app, mc_t* g){
    (void)app;
    g->state = ST_PLAY;
}

/* DDA raycast */
static int raycast(const mc_t* g, float ox, float oy, float oz,
                   float dx, float dy, float dz, float maxd,
                   int* hx, int* hy, int* hz,
                   int* px, int* py, int* pz){
    int x = (int)floorf(ox), y = (int)floorf(oy), z = (int)floorf(oz);
    int step_x = dx > 0 ? 1 : (dx < 0 ? -1 : 0);
    int step_y = dy > 0 ? 1 : (dy < 0 ? -1 : 0);
    int step_z = dz > 0 ? 1 : (dz < 0 ? -1 : 0);

    float t_max_x, t_max_y, t_max_z;
    float t_delta_x = (step_x != 0) ? fabsf(1.f / dx) : 1e30f;
    float t_delta_y = (step_y != 0) ? fabsf(1.f / dy) : 1e30f;
    float t_delta_z = (step_z != 0) ? fabsf(1.f / dz) : 1e30f;

    t_max_x = (step_x > 0) ? ((floorf(ox)+1.f - ox) * t_delta_x) :
              (step_x < 0) ? ((ox - floorf(ox)) * t_delta_x) : 1e30f;
    t_max_y = (step_y > 0) ? ((floorf(oy)+1.f - oy) * t_delta_y) :
              (step_y < 0) ? ((oy - floorf(oy)) * t_delta_y) : 1e30f;
    t_max_z = (step_z > 0) ? ((floorf(oz)+1.f - oz) * t_delta_z) :
              (step_z < 0) ? ((oz - floorf(oz)) * t_delta_z) : 1e30f;

    float t = 0.f;
    int px0 = x, py0 = y, pz0 = z;
    for(int i = 0; i < 64 && t <= maxd; i++){
        if(in_world(x,y,z) && blk_solid(get_block(g, x,y,z))){
            if(hx) *hx = x;
            if(hy) *hy = y;
            if(hz) *hz = z;
            if(px) *px = px0;
            if(py) *py = py0;
            if(pz) *pz = pz0;
            return 1;
        }
        px0 = x; py0 = y; pz0 = z;
        if(t_max_x < t_max_y){
            if(t_max_x < t_max_z){ t = t_max_x; t_max_x += t_delta_x; x += step_x; }
            else { t = t_max_z; t_max_z += t_delta_z; z += step_z; }
        } else {
            if(t_max_y < t_max_z){ t = t_max_y; t_max_y += t_delta_y; y += step_y; }
            else { t = t_max_z; t_max_z += t_delta_z; z += step_z; }
        }
    }
    return 0;
}

static int aabb_hits(const mc_t* g, float x, float y, float z){
    int x0 = (int)floorf(x - PLAYER_W), x1 = (int)floorf(x + PLAYER_W);
    int y0 = (int)floorf(y), y1 = (int)floorf(y + PLAYER_H - 0.01f);
    int z0 = (int)floorf(z - PLAYER_W), z1 = (int)floorf(z + PLAYER_W);
    for(int by = y0; by <= y1; by++)
        for(int bz = z0; bz <= z1; bz++)
            for(int bx = x0; bx <= x1; bx++)
                if(blk_collide(get_block(g, bx, by, bz))) return 1;
    return 0;
}

static void unstick(mc_t* g){
    if(!aabb_hits(g, g->px, g->py, g->pz)) return;
    for(float dy = 0.05f; dy <= 2.5f; dy += 0.05f){
        if(!aabb_hits(g, g->px, g->py + dy, g->pz)){
            g->py += dy;
            return;
        }
    }
    /* Nudge horizontally out of soft embeds */
    static const float ox[4] = {0.25f, -0.25f, 0.f, 0.f};
    static const float oz[4] = {0.f, 0.f, 0.25f, -0.25f};
    for(int i = 0; i < 4; i++){
        if(!aabb_hits(g, g->px + ox[i], g->py, g->pz + oz[i])){
            g->px += ox[i];
            g->pz += oz[i];
            return;
        }
    }
}

static void move_axis(mc_t* g, float* pos, float delta, int axis){
    if(fabsf(delta) < 1e-8f) return;
    float nx = g->px, ny = g->py, nz = g->pz;
    if(axis == 0) nx += delta;
    else if(axis == 1) ny += delta;
    else nz += delta;
    if(!aabb_hits(g, nx, ny, nz)){
        *pos += delta;
        return;
    }
    if(axis == 1){
        g->vy = 0;
        if(delta < 0) g->on_ground = 1;
        return;
    }
    /* Minecraft-like auto step-up onto low blocks */
    const float step = 0.6f;
    float sy = g->py + step;
    float sx = g->px, sz = g->pz;
    if(axis == 0) sx += delta; else sz += delta;
    if(!aabb_hits(g, g->px, sy, g->pz) && !aabb_hits(g, sx, sy, sz)){
        g->py = sy;
        *pos += delta;
    }
}

static void draw_face(pb_3d* gfx, int x, int y, int z, int face, pb_color c){
    float x0 = (float)x, y0 = (float)y, z0 = (float)z;
    float x1 = x0 + 1.f, y1 = y0 + 1.f, z1 = z0 + 1.f;
    switch(face){
        case 0: /* +Y */
            pb_3d_triangle(gfx, pb_v3(x0,y1,z0), pb_v3(x0,y1,z1), pb_v3(x1,y1,z1), c, 0);
            pb_3d_triangle(gfx, pb_v3(x0,y1,z0), pb_v3(x1,y1,z1), pb_v3(x1,y1,z0), c, 0);
            break;
        case 1: /* -Y */
            pb_3d_triangle(gfx, pb_v3(x0,y0,z0), pb_v3(x1,y0,z0), pb_v3(x1,y0,z1), c, 0);
            pb_3d_triangle(gfx, pb_v3(x0,y0,z0), pb_v3(x1,y0,z1), pb_v3(x0,y0,z1), c, 0);
            break;
        case 2: /* +X */
            pb_3d_triangle(gfx, pb_v3(x1,y0,z0), pb_v3(x1,y1,z0), pb_v3(x1,y1,z1), c, 0);
            pb_3d_triangle(gfx, pb_v3(x1,y0,z0), pb_v3(x1,y1,z1), pb_v3(x1,y0,z1), c, 0);
            break;
        case 3: /* -X */
            pb_3d_triangle(gfx, pb_v3(x0,y0,z0), pb_v3(x0,y0,z1), pb_v3(x0,y1,z1), c, 0);
            pb_3d_triangle(gfx, pb_v3(x0,y0,z0), pb_v3(x0,y1,z1), pb_v3(x0,y1,z0), c, 0);
            break;
        case 4: /* +Z */
            pb_3d_triangle(gfx, pb_v3(x0,y0,z1), pb_v3(x1,y0,z1), pb_v3(x1,y1,z1), c, 0);
            pb_3d_triangle(gfx, pb_v3(x0,y0,z1), pb_v3(x1,y1,z1), pb_v3(x0,y1,z1), c, 0);
            break;
        case 5: /* -Z */
            pb_3d_triangle(gfx, pb_v3(x0,y0,z0), pb_v3(x0,y1,z0), pb_v3(x1,y1,z0), c, 0);
            pb_3d_triangle(gfx, pb_v3(x0,y0,z0), pb_v3(x1,y1,z0), pb_v3(x1,y0,z0), c, 0);
            break;
    }
}

static int face_exposed(const mc_t* g, int x, int y, int z, int face){
    static const int ox[6] = {0,0,1,-1,0,0};
    static const int oy[6] = {1,-1,0,0,0,0};
    static const int oz[6] = {0,0,0,0,1,-1};
    int nx = x+ox[face], ny = y+oy[face], nz = z+oz[face];
    int n = get_block(g, nx, ny, nz);
    if(n == BLK_AIR) return 1;
    if(n == BLK_WATER && get_block(g,x,y,z) != BLK_WATER) return 1;
    if(n == BLK_LEAVES && get_block(g,x,y,z) != BLK_LEAVES) return 1;
    return 0;
}

static void draw_world(mc_t* g){
    int cx = (int)floorf(g->px);
    int cy = (int)floorf(g->py + EYE_H);
    int cz = (int)floorf(g->pz);
    int r = RENDER_R;
    for(int y = cy - r; y <= cy + r; y++){
        if(y < 0 || y >= WY) continue;
        for(int z = cz - r; z <= cz + r; z++){
            if(z < 0 || z >= WZ) continue;
            for(int x = cx - r; x <= cx + r; x++){
                if(x < 0 || x >= WX) continue;
                int id = g->world[y][z][x];
                if(id == BLK_AIR) continue;
                int dx = x - cx, dy = y - cy, dz = z - cz;
                if(dx*dx + dy*dy + dz*dz > r*r) continue;
                for(int f = 0; f < 6; f++){
                    if(!face_exposed(g, x, y, z, f)) continue;
                    int shade = (f == 0) ? 0 : (f == 1) ? 1 : 2;
                    draw_face(g->gfx, x, y, z, f, blk_color(id, shade));
                }
            }
        }
    }
    /* Highlight looked-at block wireframe */
    if(g->has_look){
        float x = (float)g->look_bx, y = (float)g->look_by, z = (float)g->look_bz;
        pb_color w = pb_rgb(0, 0, 0);
        pb_3d_line(g->gfx, pb_v3(x,y,z), pb_v3(x+1,y,z), w);
        pb_3d_line(g->gfx, pb_v3(x+1,y,z), pb_v3(x+1,y,z+1), w);
        pb_3d_line(g->gfx, pb_v3(x+1,y,z+1), pb_v3(x,y,z+1), w);
        pb_3d_line(g->gfx, pb_v3(x,y,z+1), pb_v3(x,y,z), w);
        pb_3d_line(g->gfx, pb_v3(x,y+1,z), pb_v3(x+1,y+1,z), w);
        pb_3d_line(g->gfx, pb_v3(x+1,y+1,z), pb_v3(x+1,y+1,z+1), w);
        pb_3d_line(g->gfx, pb_v3(x+1,y+1,z+1), pb_v3(x,y+1,z+1), w);
        pb_3d_line(g->gfx, pb_v3(x,y+1,z+1), pb_v3(x,y+1,z), w);
        pb_3d_line(g->gfx, pb_v3(x,y,z), pb_v3(x,y+1,z), w);
        pb_3d_line(g->gfx, pb_v3(x+1,y,z), pb_v3(x+1,y+1,z), w);
        pb_3d_line(g->gfx, pb_v3(x+1,y,z+1), pb_v3(x+1,y+1,z+1), w);
        pb_3d_line(g->gfx, pb_v3(x,y,z+1), pb_v3(x,y+1,z+1), w);
    }
}

static void draw_hotbar(pb_fb* fb, mc_t* g){
    int slots = 9;
    int sw = 5, sh = 3;
    int total = slots * (sw + 1) - 1;
    int x0 = (fb->w - total) / 2;
    int y0 = fb->h - sh - 1;
    for(int i = 0; i < slots; i++){
        int x = x0 + i * (sw + 1);
        pb_color border = (i == g->slot) ? pb_rgb(255,255,255) : pb_rgb(80,80,90);
        pb_color fill = pb_rgb(20, 20, 28);
        pb_fb_box(fb, x, y0, sw, sh, border, fill, 0);
        int id = g->hotbar[i];
        if(id > BLK_AIR && id < BLK_COUNT){
            pb_color c = blk_color(id, 0);
            pb_fb_fill_rect(fb, x+1, y0+1, sw-2, sh-2, pb_cell_make(' ', c, c, 0));
        }
    }
    if(g->hotbar[g->slot] > 0 && g->hotbar[g->slot] < BLK_COUNT)
        pb_fb_text_centered(fb, y0 - 1, BLK_NAME[g->hotbar[g->slot]],
                            pb_rgb(230,230,230), pb_rgb(6,8,14), 0);
}

static void draw_crosshair(pb_fb* fb){
    int cx = fb->w / 2, cy = fb->h / 2;
    pb_color c = pb_rgb(255,255,255);
    pb_fb_put(fb, cx, cy, pb_cell_make('+', c, pb_rgb(0,0,0), PB_STYLE_BOLD));
}

static void draw_pause(pb_fb* fb, const mc_t* g){
    pb_popup_desc d;
    pb_popup_desc_init(&d);
    d.title = "Game Menu";
    d.body = g->pause_sel == 0
        ? "> Resume\n  Quit to terminal"
        : "  Resume\n> Quit to terminal";
    d.hint = "Arrows select  Enter confirm  Esc resume";
    d.width = 34;
    d.height = 0; /* auto-size + clamp to terminal */
    pb_popup_draw(fb, &d);
}

static int key_wasd(const pb_app* app, char lo){
    char up = (char)(lo - 32);
    return pb_is_char_down(app, (uint32_t)lo) || pb_is_char_down(app, (uint32_t)up);
}

static void on_event(pb_app* app, void* user, const pb_event* ev){
    mc_t* g = (mc_t*)user;
    if(ev->type == PB_EVENT_QUIT){ pb_app_quit(app); return; }

    if(ev->type == PB_EVENT_KEY && ev->as.key.pressed){
        pb_key k = ev->as.key.key;
        uint32_t cp = ev->as.key.codepoint;

        if(g->state == ST_PAUSE){
            if(k == PB_KEY_ESC || cp == 'r' || cp == 'R'){
                resume_game(app, g);
                return;
            }
            if(k == PB_KEY_UP || k == PB_KEY_DOWN){
                g->pause_sel = 1 - g->pause_sel;
                return;
            }
            if(k == PB_KEY_ENTER || cp == ' '){
                if(g->pause_sel == 0) resume_game(app, g);
                else pb_app_quit(app);
                return;
            }
            if(cp == 'q' || cp == 'Q'){
                pb_app_quit(app);
                return;
            }
            return;
        }

        if(k == PB_KEY_ESC){
            pause_game(app, g);
            return;
        }

        if(cp >= '1' && cp <= '9') g->slot = (int)(cp - '1');
        if(cp == 'f' || cp == 'F') g->flying = !g->flying;
    }

    if(ev->type == PB_EVENT_MOUSE && g->state == ST_PLAY){
        const pb_mouse_event* m = &ev->as.mouse;
        if(m->wheel != 0){
            g->slot -= m->wheel;
            if(g->slot < 0) g->slot = 8;
            if(g->slot > 8) g->slot = 0;
        }
    }
}

static void on_update(pb_app* app, void* user, double dt0){
    mc_t* g = (mc_t*)user;
    float dt = (float)dt0;
    if(dt < 0) dt = 0;
    if(dt > 0.1f) dt = 0.1f;

    if(g->state == ST_PAUSE) return;

    /* Arrow-key look (no cursor lock in terminals) */
    const float look_spd = 2.4f;
    if(pb_is_key_down(app, PB_KEY_LEFT))  g->yaw   -= look_spd * dt;
    if(pb_is_key_down(app, PB_KEY_RIGHT)) g->yaw   += look_spd * dt;
    if(pb_is_key_down(app, PB_KEY_UP))    g->pitch += look_spd * dt;
    if(pb_is_key_down(app, PB_KEY_DOWN))  g->pitch -= look_spd * dt;
    g->pitch = clampf(g->pitch, -1.45f, 1.45f);

    float cy = cosf(g->yaw), sy = sinf(g->yaw);
    float cp = cosf(g->pitch), sp = sinf(g->pitch);
    /* View ray uses pitch; movement is yaw-only like Minecraft */
    float look_x = sy * cp, look_y = sp, look_z = -cy * cp;
    float fwd_x = sy, fwd_z = -cy;
    float right_x = cy, right_z = sy;

    float speed = g->flying ? 8.f : 4.3f;
    if(pb_is_char_pressed(app, ' ') && !g->flying && g->on_ground){
        g->vy = 8.2f;
        g->on_ground = 0;
    }

    float mxm = 0, mzm = 0;
    if(key_wasd(app, 'w')){ mxm += fwd_x; mzm += fwd_z; }
    if(key_wasd(app, 's')){ mxm -= fwd_x; mzm -= fwd_z; }
    if(key_wasd(app, 'a')){ mxm -= right_x; mzm -= right_z; }
    if(key_wasd(app, 'd')){ mxm += right_x; mzm += right_z; }
    float len = sqrtf(mxm*mxm + mzm*mzm);
    if(len > 1e-4f){ mxm /= len; mzm /= len; }

    if(g->flying){
        g->vx = mxm * speed;
        g->vz = mzm * speed;
        g->vy = 0;
        if(pb_is_char_down(app, ' ')) g->vy = speed;
        if(key_wasd(app, 'c') || pb_is_char_down(app, 'q') || pb_is_char_down(app, 'Q'))
            g->vy = -speed;
    } else {
        /* Direct velocity from keys — no coasting after release */
        g->vx = mxm * speed;
        g->vz = mzm * speed;
        g->vy -= 28.f * dt;
        if(g->vy < -40.f) g->vy = -40.f;
    }

    unstick(g);
    g->on_ground = 0;
    move_axis(g, &g->px, g->vx * dt, 0);
    move_axis(g, &g->pz, g->vz * dt, 2);
    move_axis(g, &g->py, g->vy * dt, 1);
    g->px = clampf(g->px, 1.f, (float)WX - 1.f);
    g->pz = clampf(g->pz, 1.f, (float)WZ - 1.f);
    if(g->py < 1.f){ g->py = 1.f; g->vy = 0; }
    if(g->py > (float)WY - 2.f){ g->py = (float)WY - 2.f; g->vy = 0; }

    g->bob += dt * (len > 0.1f && g->on_ground ? 10.f : 2.f);

    float ex = g->px, ey = g->py + EYE_H, ez = g->pz;
    g->has_look = raycast(g, ex, ey, ez, look_x, look_y, look_z, 5.5f,
                          &g->look_bx, &g->look_by, &g->look_bz,
                          &g->place_bx, &g->place_by, &g->place_bz);
    g->has_place = g->has_look;

    if(pb_is_mouse_button_pressed(app, PB_MOUSE_LEFT) && g->has_look){
        int id = get_block(g, g->look_bx, g->look_by, g->look_bz);
        if(id != BLK_BEDROCK) set_block(g, g->look_bx, g->look_by, g->look_bz, BLK_AIR);
    }
    if(pb_is_mouse_button_pressed(app, PB_MOUSE_RIGHT) && g->has_place){
        int id = g->hotbar[g->slot];
        if(id > BLK_AIR){
            float bx = (float)g->place_bx + 0.5f;
            float by = (float)g->place_by;
            float bz = (float)g->place_bz + 0.5f;
            int overlap = fabsf(bx - g->px) < 0.8f && fabsf(bz - g->pz) < 0.8f &&
                          by < g->py + PLAYER_H && by + 1.f > g->py;
            if(!overlap && get_block(g, g->place_bx, g->place_by, g->place_bz) == BLK_AIR)
                set_block(g, g->place_bx, g->place_by, g->place_bz, id);
        }
    }
}

static void on_draw(pb_app* app, void* user, pb_fb* fb){
    mc_t* g = (mc_t*)user;
    pb_color sky_top = pb_rgb(110, 170, 255);
    pb_color sky_bot = pb_rgb(180, 210, 255);
    pb_fb_fill_gradient_v(fb, 0, 0, fb->w, fb->h, sky_top, sky_bot);

    float cy = cosf(g->yaw), sy = sinf(g->yaw);
    float cp = cosf(g->pitch), sp = sinf(g->pitch);
    float ex = g->px, ey = g->py + EYE_H + (g->on_ground ? sinf(g->bob)*0.03f : 0.f), ez = g->pz;
    float fx = sy * cp, fy = sp, fz = -cy * cp;

    pb_camera3d cam = pb_camera3d_default();
    cam.position = pb_v3(ex, ey, ez);
    cam.target = pb_v3(ex + fx, ey + fy, ez + fz);
    cam.up = pb_v3(0, 1, 0);
    cam.fovy = 70.f * PB_DEG2RAD;
    cam.znear = 0.08f;
    cam.zfar = 48.f;

    if(pb_3d_begin(g->gfx, fb, PB_3D_HALF, &cam)){
        pb_3d_set_light(g->gfx, pb_v3(0.35f, 1.0f, 0.25f), 0.45f);
        draw_world(g);
        pb_3d_end(g->gfx);
    }

    if(g->state == ST_PLAY){
        draw_crosshair(fb);
        draw_hotbar(fb, g);
        char line[64];
        snprintf(line, sizeof line, "FPS %d  %s", pb_get_fps(app),
                 g->flying ? "FLY" : "");
        pb_fb_text(fb, 1, 0, line, pb_rgb(255,255,255), pb_rgb(40,60,100), 0);
        pb_fb_text(fb, 1, fb->h - 1,
                   "WASD move  Arrows look  Space jump  C/Q fly-down  F fly  LMB/RMB  Esc pause",
                   pb_rgb(200,210,230), pb_rgb(30,40,60), 0);
    } else {
        draw_pause(fb, g);
    }
}

static void on_init(pb_app* app, void* user){
    mc_t* g = (mc_t*)user;
    g->state = ST_PLAY;
    pb_app_set_cursor_visible(app, 1);
    pb_app_set_mouse_capture(app, 0);
}

int main(void){
    mc_t g;
    memset(&g, 0, sizeof(g));
    g.seed = (unsigned)time(NULL);
    g.gfx = pb_3d_create();
    if(!g.gfx) return 1;

    g.hotbar[0] = BLK_GRASS;
    g.hotbar[1] = BLK_DIRT;
    g.hotbar[2] = BLK_STONE;
    g.hotbar[3] = BLK_COBBLE;
    g.hotbar[4] = BLK_PLANKS;
    g.hotbar[5] = BLK_LOG;
    g.hotbar[6] = BLK_LEAVES;
    g.hotbar[7] = BLK_SAND;
    g.hotbar[8] = BLK_WATER;
    g.slot = 0;

    gen_world(&g);

    pb_app_desc d;
    memset(&d, 0, sizeof(d));
    d.title = "PlayboxCraft";
    d.target_fps = 60;
    d.flags = PB_APP_FLAG_CUSTOM_CLEAR;
    d.clear = pb_cell_make(' ', pb_rgb(255,255,255), pb_rgb(110,170,255), 0);
    d.on_init = on_init;
    d.on_event = on_event;
    d.on_update = on_update;
    d.on_draw = on_draw;

    pb_app* app = pb_app_create(&d, &g);
    if(!app){ pb_3d_destroy(g.gfx); return 1; }
    int ok = pb_app_run(app);
    pb_app_destroy(app);
    pb_3d_destroy(g.gfx);
    return ok ? 0 : 1;
}
