#include "playbox/pb.h"
#include "playbox/pb_math.h"
#include "playbox/pb_3d.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ARENA_HALF   8.0f
#define MAX_CRYSTALS 12
#define MAX_ENEMIES  10
#define MAX_BULLETS  24
#define MAX_PARTS    48

typedef enum {
    ST_TITLE = 0,
    ST_PLAY,
    ST_DEAD
} game_state;

typedef struct {
    float x, z;
    float spin;
    int alive;
} crystal_t;

typedef struct {
    float x, z;
    float vx, vz;
    float wobble;
    int alive;
} enemy_t;

typedef struct {
    float x, y, z;
    float vx, vy, vz;
    float life;
    int alive;
} bullet_t;

typedef struct {
    float x, y, z;
    float vx, vy, vz;
    float life;
    pb_color color;
    int alive;
} part_t;

typedef struct {
    pb_3d* gfx;
    game_state state;

    float px, pz;
    float facing;       /* movement yaw */
    float cam_yaw;
    float cam_pitch;
    float cam_dist;
    float invuln;
    float shoot_cd;

    int lives;
    int score;
    int wave;
    int best;
    float wave_t;
    float title_t;
    float dead_t;

    crystal_t crystals[MAX_CRYSTALS];
    enemy_t enemies[MAX_ENEMIES];
    bullet_t bullets[MAX_BULLETS];
    part_t parts[MAX_PARTS];

    int drag;
    int last_mx, last_my;
    unsigned rng;
} arena_t;

static float clampf(float v, float lo, float hi){
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

static unsigned rng_next(arena_t* g){
    g->rng = g->rng * 1664525u + 1013904223u;
    return g->rng;
}

static float rng_f(arena_t* g){
    return (float)(rng_next(g) & 0xffffu) / 65535.0f;
}

static float rng_range(arena_t* g, float lo, float hi){
    return lo + rng_f(g) * (hi - lo);
}

static void emit_burst(arena_t* g, float x, float y, float z, pb_color c, int n){
    for(int i = 0; i < n; i++){
        int slot = -1;
        for(int j = 0; j < MAX_PARTS; j++){
            if(!g->parts[j].alive){ slot = j; break; }
        }
        if(slot < 0) break;
        part_t* p = &g->parts[slot];
        p->alive = 1;
        p->x = x; p->y = y; p->z = z;
        float a = rng_f(g) * 6.2831853f;
        float s = rng_range(g, 1.5f, 4.5f);
        p->vx = cosf(a) * s;
        p->vy = rng_range(g, 1.0f, 4.0f);
        p->vz = sinf(a) * s;
        p->life = rng_range(g, 0.35f, 0.8f);
        p->color = c;
    }
}

static void spawn_crystal(arena_t* g){
    for(int i = 0; i < MAX_CRYSTALS; i++){
        if(g->crystals[i].alive) continue;
        crystal_t* c = &g->crystals[i];
        c->alive = 1;
        c->spin = rng_f(g) * 6.2831853f;
        for(int tries = 0; tries < 20; tries++){
            c->x = rng_range(g, -ARENA_HALF + 1.2f, ARENA_HALF - 1.2f);
            c->z = rng_range(g, -ARENA_HALF + 1.2f, ARENA_HALF - 1.2f);
            float dx = c->x - g->px, dz = c->z - g->pz;
            if(dx * dx + dz * dz > 4.0f) break;
        }
        return;
    }
}

static void spawn_enemy(arena_t* g){
    for(int i = 0; i < MAX_ENEMIES; i++){
        if(g->enemies[i].alive) continue;
        enemy_t* e = &g->enemies[i];
        e->alive = 1;
        e->wobble = rng_f(g) * 6.2831853f;
        /* Spawn at edge */
        int edge = (int)(rng_next(g) % 4u);
        if(edge == 0){ e->x = -ARENA_HALF + 0.6f; e->z = rng_range(g, -ARENA_HALF, ARENA_HALF); }
        else if(edge == 1){ e->x = ARENA_HALF - 0.6f; e->z = rng_range(g, -ARENA_HALF, ARENA_HALF); }
        else if(edge == 2){ e->z = -ARENA_HALF + 0.6f; e->x = rng_range(g, -ARENA_HALF, ARENA_HALF); }
        else { e->z = ARENA_HALF - 0.6f; e->x = rng_range(g, -ARENA_HALF, ARENA_HALF); }
        e->vx = e->vz = 0;
        return;
    }
}

static void reset_run(arena_t* g){
    g->px = 0; g->pz = 0;
    g->facing = 0;
    g->cam_yaw = 0.7f;
    g->cam_pitch = 0.55f;
    g->cam_dist = 10.0f;
    g->invuln = 1.5f;
    g->shoot_cd = 0;
    g->lives = 3;
    g->score = 0;
    g->wave = 1;
    g->wave_t = 0;
    g->dead_t = 0;
    memset(g->crystals, 0, sizeof(g->crystals));
    memset(g->enemies, 0, sizeof(g->enemies));
    memset(g->bullets, 0, sizeof(g->bullets));
    memset(g->parts, 0, sizeof(g->parts));
    for(int i = 0; i < 5; i++) spawn_crystal(g);
    spawn_enemy(g);
}

static void start_game(arena_t* g){
    g->state = ST_PLAY;
    reset_run(g);
}

static void fire(arena_t* g){
    if(g->shoot_cd > 0) return;
    for(int i = 0; i < MAX_BULLETS; i++){
        if(g->bullets[i].alive) continue;
        bullet_t* b = &g->bullets[i];
        b->alive = 1;
        b->x = g->px;
        b->y = 0.55f;
        b->z = g->pz;
        float speed = 14.0f;
        b->vx = sinf(g->facing) * speed;
        b->vy = 0;
        b->vz = cosf(g->facing) * speed;
        b->life = 1.2f;
        g->shoot_cd = 0.18f;
        return;
    }
}

static void on_event(pb_app* app, void* user, const pb_event* ev){
    arena_t* g = (arena_t*)user;
    if(ev->type == PB_EVENT_QUIT){ pb_app_quit(app); return; }

    if(ev->type == PB_EVENT_KEY && ev->as.key.pressed){
        pb_key k = ev->as.key.key;
        uint32_t cp = ev->as.key.codepoint;

        if(g->state == ST_TITLE){
            if(k == PB_KEY_ENTER || cp == ' ' || cp == 's' || cp == 'S')
                start_game(g);
            if(k == PB_KEY_ESC || cp == 'q' || cp == 'Q')
                pb_app_quit(app);
            return;
        }
        if(g->state == ST_DEAD){
            if(k == PB_KEY_ENTER || cp == ' ' || cp == 'r' || cp == 'R')
                start_game(g);
            if(k == PB_KEY_ESC || cp == 'q' || cp == 'Q')
                pb_app_quit(app);
            return;
        }
        /* PLAY */
        if(k == PB_KEY_ESC || cp == 'q' || cp == 'Q'){ pb_app_quit(app); return; }
        if(cp == 'r' || cp == 'R'){ start_game(g); return; }
        if(cp == ' ' || k == PB_KEY_ENTER) fire(g);
    }

    if(ev->type == PB_EVENT_MOUSE && g->state == ST_PLAY){
        const pb_mouse_event* m = &ev->as.mouse;
        if(m->button == PB_MOUSE_RIGHT && m->pressed) fire(g);
        if(m->wheel != 0){
            g->cam_dist = clampf(g->cam_dist - (float)m->wheel * 0.5f, 7.0f, 18.0f);
        }
    }
}

static void update_parts(arena_t* g, float dt){
    for(int i = 0; i < MAX_PARTS; i++){
        part_t* p = &g->parts[i];
        if(!p->alive) continue;
        p->life -= dt;
        if(p->life <= 0){ p->alive = 0; continue; }
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        p->z += p->vz * dt;
        p->vy -= 9.0f * dt;
        if(p->y < 0){ p->y = 0; p->vy *= -0.35f; }
    }
}

static void on_update(pb_app* app, void* user, double dt0){
    arena_t* g = (arena_t*)user;
    float dt = (float)dt0;
    if(dt < 0) dt = 0;
    if(dt > 0.05f) dt = 0.05f;

    g->title_t += dt;

    if(g->state == ST_TITLE){
        g->cam_yaw += dt * 0.35f;
        return;
    }
    if(g->state == ST_DEAD){
        g->dead_t += dt;
        update_parts(g, dt);
        return;
    }

    /* Camera orbit (drag right → yaw right, drag up → look higher) */
    float turn = 1.6f * dt;
    if(pb_is_key_down(app, PB_KEY_LEFT) || pb_is_char_down(app, 'q')) g->cam_yaw += turn;
    if(pb_is_key_down(app, PB_KEY_RIGHT) || pb_is_char_down(app, 'e')) g->cam_yaw -= turn;
    if(pb_is_char_down(app, 'z')) g->cam_dist = clampf(g->cam_dist - 4.f * dt, 7.f, 18.f);
    if(pb_is_char_down(app, 'x')) g->cam_dist = clampf(g->cam_dist + 4.f * dt, 7.f, 18.f);

    /* LMB drag orbit — sample on press edge, then accumulate deltas while held.
     * (SGR drag motion also reports pressed=1; must not reset the origin then.) */
    if(pb_is_mouse_button_pressed(app, PB_MOUSE_LEFT)){
        g->drag = 1;
        g->last_mx = pb_get_mouse_x(app);
        g->last_my = pb_get_mouse_y(app);
    }
    if(!pb_is_mouse_button_down(app, PB_MOUSE_LEFT)) g->drag = 0;
    if(g->drag){
        int mx = pb_get_mouse_x(app);
        int my = pb_get_mouse_y(app);
        g->cam_yaw   -= (float)(mx - g->last_mx) * 0.02f;
        g->cam_pitch += (float)(my - g->last_my) * 0.02f;
        g->last_mx = mx;
        g->last_my = my;
    }
    g->cam_pitch = clampf(g->cam_pitch, 0.25f, 1.15f);

    /* Move in camera view space on XZ.
     * Camera sits at yaw around target: forward on ground = (-sin(yaw), -cos(yaw)). */
    float mx = 0, mz = 0;
    if(pb_is_char_down(app, 'w') || pb_is_key_down(app, PB_KEY_UP)) mz += 1;
    if(pb_is_char_down(app, 's') || pb_is_key_down(app, PB_KEY_DOWN)) mz -= 1;
    if(pb_is_char_down(app, 'a')) mx -= 1;
    if(pb_is_char_down(app, 'd')) mx += 1;
    if(mx != 0 || mz != 0){
        float len = sqrtf(mx * mx + mz * mz);
        mx /= len; mz /= len;
        float cy = cosf(g->cam_yaw), sy = sinf(g->cam_yaw);
        float fwd_x = -sy, fwd_z = -cy;
        float right_x = cy, right_z = -sy;
        float fx = fwd_x * mz + right_x * mx;
        float fz = fwd_z * mz + right_z * mx;
        float speed = 6.5f;
        g->px += fx * speed * dt;
        g->pz += fz * speed * dt;
        g->facing = atan2f(fx, fz);
    }

    g->px = clampf(g->px, -ARENA_HALF + 0.5f, ARENA_HALF - 0.5f);
    g->pz = clampf(g->pz, -ARENA_HALF + 0.5f, ARENA_HALF - 0.5f);

    if(g->shoot_cd > 0) g->shoot_cd -= dt;
    if(g->invuln > 0) g->invuln -= dt;
    if(pb_is_char_down(app, 'f') || pb_is_mouse_button_down(app, PB_MOUSE_RIGHT))
        fire(g);

    /* Crystals */
    for(int i = 0; i < MAX_CRYSTALS; i++){
        crystal_t* c = &g->crystals[i];
        if(!c->alive) continue;
        c->spin += dt * 2.8f;
        float dx = c->x - g->px, dz = c->z - g->pz;
        if(dx * dx + dz * dz < 0.85f * 0.85f){
            c->alive = 0;
            g->score += 10 + g->wave * 2;
            emit_burst(g, c->x, 0.6f, c->z, pb_rgb(120, 255, 200), 10);
            spawn_crystal(g);
        }
    }

    /* Bullets */
    for(int i = 0; i < MAX_BULLETS; i++){
        bullet_t* b = &g->bullets[i];
        if(!b->alive) continue;
        b->life -= dt;
        b->x += b->vx * dt;
        b->y += b->vy * dt;
        b->z += b->vz * dt;
        if(b->life <= 0 || fabsf(b->x) > ARENA_HALF + 1 || fabsf(b->z) > ARENA_HALF + 1){
            b->alive = 0;
            continue;
        }
        for(int j = 0; j < MAX_ENEMIES; j++){
            enemy_t* e = &g->enemies[j];
            if(!e->alive) continue;
            float dx = e->x - b->x, dz = e->z - b->z;
            if(dx * dx + dz * dz < 0.7f * 0.7f){
                e->alive = 0;
                b->alive = 0;
                g->score += 25;
                emit_burst(g, e->x, 0.5f, e->z, pb_rgb(255, 110, 130), 14);
                break;
            }
        }
    }

    /* Enemies chase */
    float esp = 2.2f + (float)g->wave * 0.35f;
    if(esp > 5.5f) esp = 5.5f;
    for(int i = 0; i < MAX_ENEMIES; i++){
        enemy_t* e = &g->enemies[i];
        if(!e->alive) continue;
        e->wobble += dt * 5.0f;
        float dx = g->px - e->x, dz = g->pz - e->z;
        float d = sqrtf(dx * dx + dz * dz);
        if(d > 0.01f){
            e->vx = dx / d * esp;
            e->vz = dz / d * esp;
        }
        e->x += e->vx * dt;
        e->z += e->vz * dt;
        e->x = clampf(e->x, -ARENA_HALF + 0.4f, ARENA_HALF - 0.4f);
        e->z = clampf(e->z, -ARENA_HALF + 0.4f, ARENA_HALF - 0.4f);

        if(g->invuln <= 0){
            float px = e->x - g->px, pz = e->z - g->pz;
            if(px * px + pz * pz < 0.75f * 0.75f){
                g->lives--;
                g->invuln = 1.6f;
                emit_burst(g, g->px, 0.5f, g->pz, pb_rgb(255, 200, 90), 16);
                if(g->lives <= 0){
                    g->state = ST_DEAD;
                    g->dead_t = 0;
                    if(g->score > g->best) g->best = g->score;
                    emit_burst(g, g->px, 0.8f, g->pz, pb_rgb(255, 80, 120), 28);
                }
            }
        }
    }

    /* Waves */
    g->wave_t += dt;
    int want = 1 + g->wave / 2;
    if(want > MAX_ENEMIES) want = MAX_ENEMIES;
    int alive_e = 0;
    for(int i = 0; i < MAX_ENEMIES; i++) if(g->enemies[i].alive) alive_e++;
    if(alive_e < want && g->wave_t > 1.8f){
        spawn_enemy(g);
        g->wave_t = 0;
    }
    if(g->score >= g->wave * 80){
        g->wave++;
        spawn_enemy(g);
        spawn_crystal(g);
    }

    update_parts(g, dt);
}

static void draw_hud(pb_fb* fb, arena_t* g){
    pb_color bg = pb_rgb(6, 8, 14);
    pb_color fg = pb_rgb(210, 220, 235);
    pb_color accent = pb_rgb(90, 220, 255);
    pb_color danger = pb_rgb(255, 110, 140);
    char line[96];

    snprintf(line, sizeof line, " SCORE %d   WAVE %d   LIVES %d   BEST %d ",
             g->score, g->wave, g->lives, g->best);
    pb_fb_text(fb, 1, 0, line, accent, bg, PB_STYLE_BOLD);

    pb_fb_text(fb, 1, fb->h - 1,
               " WASD move  Q/E orbit  LMB drag cam  F/RMB/Space shoot  R restart  Q quit ",
               pb_rgb(90, 105, 130), bg, 0);

    if(g->invuln > 0 && g->state == ST_PLAY && ((int)(g->invuln * 10) & 1))
        pb_fb_text(fb, fb->w / 2 - 4, 1, "HIT!", danger, bg, PB_STYLE_BOLD);

    (void)fg;
}

static void on_draw(pb_app* app, void* user, pb_fb* fb){
    arena_t* g = (arena_t*)user;
    (void)app;

    pb_color bg = pb_rgb(6, 8, 14);
    pb_fb_clear(fb, pb_cell_make(' ', pb_rgb(200, 210, 220), bg, 0));

    float look_x = g->px, look_z = g->pz;
    if(g->state == ST_TITLE){
        look_x = 0; look_z = 0;
        g->cam_dist = 11.0f;
        g->cam_pitch = 0.55f;
    }

    pb_camera3d cam = pb_camera3d_default();
    float cp = cosf(g->cam_pitch), sp = sinf(g->cam_pitch);
    float cy = cosf(g->cam_yaw), sy = sinf(g->cam_yaw);
    float dist = clampf(g->cam_dist, 7.0f, 18.0f);
    cam.position = pb_v3(
        look_x + dist * cp * sy,
        dist * sp + 1.5f,
        look_z + dist * cp * cy
    );
    cam.target = pb_v3(look_x, 0.5f, look_z);
    cam.fovy = 55.0f * PB_DEG2RAD;
    cam.znear = 0.35f;
    cam.zfar = 120.0f;

    /* Half-block pixels: denser, more solid fills than braille for gameplay. */
    if(!pb_3d_begin(g->gfx, fb, PB_3D_HALF, &cam)) return;
    pb_3d_set_light(g->gfx, pb_v3(0.5f, 1.0f, 0.4f), 0.38f);

    /* Tiled floor — small quads so near-plane clip never nukes the whole arena */
    {
        float fh = ARENA_HALF;
        float step = 2.0f;
        pb_color floor_a = pb_rgb(24, 34, 54);
        pb_color floor_b = pb_rgb(18, 26, 42);
        for(float z = -fh; z < fh - 0.01f; z += step){
            for(float x = -fh; x < fh - 0.01f; x += step){
                float x1 = x + step; if(x1 > fh) x1 = fh;
                float z1 = z + step; if(z1 > fh) z1 = fh;
                int checker = (((int)floorf((x + fh) / step) + (int)floorf((z + fh) / step)) & 1);
                pb_color fc = checker ? floor_a : floor_b;
                pb_3d_triangle(g->gfx, pb_v3(x, 0, z), pb_v3(x, 0, z1), pb_v3(x1, 0, z1), fc, 0);
                pb_3d_triangle(g->gfx, pb_v3(x, 0, z), pb_v3(x1, 0, z1), pb_v3(x1, 0, z), fc, 0);
            }
        }
    }
    pb_3d_grid(g->gfx, ARENA_HALF, 1.0f, pb_rgb(50, 75, 110));
    pb_color wall = pb_rgb(80, 120, 170);
    float h = ARENA_HALF;
    pb_3d_line(g->gfx, pb_v3(-h, 0, -h), pb_v3( h, 0, -h), wall);
    pb_3d_line(g->gfx, pb_v3( h, 0, -h), pb_v3( h, 0,  h), wall);
    pb_3d_line(g->gfx, pb_v3( h, 0,  h), pb_v3(-h, 0,  h), wall);
    pb_3d_line(g->gfx, pb_v3(-h, 0,  h), pb_v3(-h, 0, -h), wall);

    for(int i = 0; i < 4; i++){
        float sx = (i & 1) ? h - 0.4f : -h + 0.4f;
        float sz = (i & 2) ? h - 0.4f : -h + 0.4f;
        pb_3d_cube(g->gfx, pb_v3(sx, 0.8f, sz), pb_v3(0.45f, 1.6f, 0.45f),
                   pb_m4_identity(), pb_rgb(70, 110, 160), 0);
    }

    if(g->state == ST_TITLE){
        float t = g->title_t;
        pb_mat4 m = pb_m4_mul(pb_m4_rotate_y(t * 0.8f), pb_m4_rotate_x(t * 0.3f));
        pb_3d_cube(g->gfx, pb_v3(0, 1.2f, 0), pb_v3(1.8f, 1.8f, 1.8f), m,
                   pb_rgb(90, 210, 255), 0);
        pb_3d_cube(g->gfx, pb_v3(2.8f, 0.5f + 0.2f * sinf(t * 3), -1.5f),
                   pb_v3(0.7f, 0.7f, 0.7f), pb_m4_rotate_y(-t * 2),
                   pb_rgb(120, 255, 200), 0);
        pb_3d_cube(g->gfx, pb_v3(-2.5f, 0.5f, 2.0f),
                   pb_v3(0.8f, 0.8f, 0.8f), pb_m4_rotate_y(t),
                   pb_rgb(255, 110, 140), 0);
    } else {
        int blink = (g->invuln > 0 && ((int)(g->invuln * 12) & 1));
        if(!blink){
            pb_mat4 pm = pb_m4_rotate_y(g->facing);
            pb_3d_cube(g->gfx, pb_v3(g->px, 0.5f, g->pz), pb_v3(0.8f, 1.0f, 0.8f),
                       pm, pb_rgb(90, 210, 255), 0);
            float nx = g->px + sinf(g->facing) * 0.6f;
            float nz = g->pz + cosf(g->facing) * 0.6f;
            pb_3d_cube(g->gfx, pb_v3(nx, 0.55f, nz), pb_v3(0.28f, 0.28f, 0.28f),
                       pb_m4_identity(), pb_rgb(180, 240, 255), 0);
        }

        for(int i = 0; i < MAX_CRYSTALS; i++){
            crystal_t* c = &g->crystals[i];
            if(!c->alive) continue;
            float bob = 0.6f + 0.15f * sinf(c->spin * 1.5f);
            pb_mat4 m = pb_m4_mul(pb_m4_rotate_y(c->spin), pb_m4_rotate_x(0.6f));
            pb_3d_cube(g->gfx, pb_v3(c->x, bob, c->z), pb_v3(0.5f, 0.75f, 0.5f),
                       m, pb_rgb(100, 255, 190), 0);
        }

        for(int i = 0; i < MAX_ENEMIES; i++){
            enemy_t* e = &g->enemies[i];
            if(!e->alive) continue;
            float bob = 0.45f + 0.08f * sinf(e->wobble);
            pb_mat4 m = pb_m4_rotate_y(e->wobble * 0.4f);
            pb_3d_cube(g->gfx, pb_v3(e->x, bob, e->z), pb_v3(0.85f, 0.85f, 0.85f),
                       m, pb_rgb(255, 100, 130), 0);
        }

        for(int i = 0; i < MAX_BULLETS; i++){
            bullet_t* b = &g->bullets[i];
            if(!b->alive) continue;
            pb_3d_cube(g->gfx, pb_v3(b->x, b->y, b->z), pb_v3(0.28f, 0.28f, 0.28f),
                       pb_m4_identity(), pb_rgb(255, 230, 120), 0);
        }

        for(int i = 0; i < MAX_PARTS; i++){
            part_t* p = &g->parts[i];
            if(!p->alive) continue;
            float s = 0.15f + 0.12f * p->life;
            pb_3d_cube(g->gfx, pb_v3(p->x, p->y, p->z), pb_v3(s, s, s),
                       pb_m4_identity(), p->color, 0);
        }
    }

    pb_3d_end(g->gfx);

    if(g->state == ST_TITLE){
        pb_fb_text_centered(fb, 1, "ARENA 3D", pb_rgb(90, 220, 255), bg, PB_STYLE_BOLD);
        pb_fb_text_centered(fb, 3, "Collect crystals. Shoot red cubes. Survive.",
                            pb_rgb(180, 195, 215), bg, 0);
        pb_fb_text_centered(fb, fb->h - 3, "Press ENTER / SPACE to start",
                            pb_rgb(120, 255, 200), bg, PB_STYLE_BOLD);
        if(g->best > 0){
            char line[48];
            snprintf(line, sizeof line, "Best score: %d", g->best);
            pb_fb_text_centered(fb, 5, line, pb_rgb(255, 200, 90), bg, 0);
        }
    } else if(g->state == ST_DEAD){
        draw_hud(fb, g);
        pb_popup_desc pop;
        pb_popup_desc_init(&pop);
        pop.title = "Game Over";
        {
            static char body[96];
            snprintf(body, sizeof body, "Score %d   Wave %d\nBest %d",
                     g->score, g->wave, g->best);
            pop.body = body;
        }
        pop.hint = "ENTER / R restart · ESC quit";
        pb_popup_draw(fb, &pop);
    } else {
        draw_hud(fb, g);
    }
}

int main(void){
    arena_t g;
    memset(&g, 0, sizeof(g));
    g.rng = (unsigned)time(NULL) ^ 0xA5A5u;
    g.state = ST_TITLE;
    g.cam_yaw = 0.7f;
    g.cam_pitch = 0.55f;
    g.cam_dist = 10.0f;
    g.gfx = pb_3d_create();
    if(!g.gfx) return 1;

    pb_app_desc d;
    memset(&d, 0, sizeof(d));
    d.title = "Arena 3D";
    d.target_fps = 60;
    d.flags = PB_APP_FLAG_CUSTOM_CLEAR;
    d.clear = pb_cell_make(' ', pb_rgb(200, 210, 220), pb_rgb(6, 8, 14), 0);
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
