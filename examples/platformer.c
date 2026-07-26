/*
 * PlayboxLib — Terminal 2D platformer (v3)
 *
 *   Move: A/D or arrows     Jump: Space / W / Up / J   (double-jump in air)
 *   Sprint: Z               Pause: P                   Frame stats: F
 *   Springs bounce you      Checkpoints save progress  Quit: Esc / Q
 *
 * Build:
 *   cc -O2 -Iinclude examples/platformer.c -Lbuild/lib -lplaybox -lm \
 *      -Wl,-rpath,'$ORIGIN/../lib' -o build/bin/pb_platformer
 */
#include "playbox/pb.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    TW = 120,
    TH = 28,
    MAX_COINS = 64,
    MAX_ENEMIES = 24,
    MAX_LIVES = 3
};

enum {
    T_EMPTY = 0,
    T_SOLID = 1,
    T_SPIKE = 2,
    T_GOAL = 3,
    T_PLATFORM = 4, /* one-way */
    T_SPRING = 5,
    T_CHECKPOINT = 6
};

enum {
    ST_TITLE = 0,
    ST_PLAY = 1,
    ST_DEAD = 2,
    ST_WIN = 3,
    ST_OVER = 4,
    ST_PAUSE = 5
};

typedef struct {
    float x, y;
    float vx;
    int alive;
    int facing;
    float anim;
} enemy;

typedef struct {
    float x, y;
    int taken;
    float phase;
} coin;

typedef struct {
    uint8_t kind;
    uint8_t flags;
} tile;

typedef struct {
    int left, right, up, down;
    int jump, jump_held;
    int sprint;
} input;

typedef struct {
    tile tiles[TH][TW];
    int spawn_x, spawn_y;
    int goal_x, goal_y;
    int ck_x, ck_y;
    int has_checkpoint;

    coin coins[MAX_COINS];
    int coin_count;
    int coins_left;
    int coins_total;

    enemy enemies[MAX_ENEMIES];
    int enemy_count;

    pb_particles fx;

    float px, py;
    float pvx, pvy;
    float pw, ph;
    int on_ground;
    int on_wall;
    int facing;
    int air_jumps;
    int lives;
    int score;
    int combo;
    float combo_t;
    int state;
    float state_t;
    float anim_t;
    float coyote;
    float jump_buf;
    float invuln;
    float land_dust;
    float wall_slide;
    float run_time;
    float toast_t;
    char toast_msg[64];
    pb_color toast_fg;

    float cam_x, cam_y;
    float shake;
    int paused_from;

    float accum;
    int was_ground;
    int fps_smooth;
    int show_stats;
} game;

static const float FIXED_DT = 1.0f / 120.0f;

/* Level: # solid  = one-way  ^ spike  * spring  C coin  E enemy
 *        S spawn  H checkpoint  F flag */
static const char* LEVEL[TH] = {
  "........................................................................................................................",
  "........................................................................................................................",
  "....C...............................................C......................C........................C...................",
  "...........====.................................====...................====......................====...................",
  "........................................................................................................................",
  "......................C................*...............................................C................................",
  ".................#########........============......................C.............#########.............................",
  "........................................................................................................................",
  ".........C......................C...............C...........E..........####.......................C.....................",
  ".......####...................####............####...............................H.............####.....................",
  ".......................................................E................................................................",
  "..............................E...................##########..............C............E................................",
  "S.............E............#######......................................####.........#####.............C................",
  "#############^^^........*...............................................................................................",
  "........................................................................................................................",
  "......................C................C.....................*......................C...................................",
  "....................####.............####..............============...............####..................................",
  "........................................................................................................................",
  "..............................................C...........E.............................................................",
  "............................................#####.......#####.............C.............................................",
  "........................................................................####............................................",
  "........................E..............*...............................................E................................",
  "......................#####........========.........................................#####..............C................",
  "...............................................................................................C......####..............",
  ".........................................C..............................................................................",
  "..............C........................####..........................C..........H..........E.................F..........",
  "........................................................................................................................",
  "########################################################################################################################"
};

static float clampf(float v, float lo, float hi){
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}
static float lerpf(float a, float b, float t){ return a + (b - a) * t; }
static int clampi(int v, int lo, int hi){
    if(v < lo) return lo;
    if(v > hi) return hi;
    return v;
}

static int tile_kind(const game* g, int tx, int ty){
    if(tx < 0 || ty < 0 || tx >= TW || ty >= TH) return T_SOLID;
    return g->tiles[ty][tx].kind;
}
static int solid_at(const game* g, int tx, int ty){
    int k = tile_kind(g, tx, ty);
    return k == T_SOLID || k == T_SPRING;
}

static void toast(game* g, const char* msg, pb_color fg){
    snprintf(g->toast_msg, sizeof g->toast_msg, "%s", msg);
    g->toast_fg = fg;
    g->toast_t = 1.6f;
}

static void fx_burst(game* g, float x, float y, pb_color c, int n, float speed){
    for(int i = 0; i < n; i++){
        float a = (float)i / (float)n * 6.2831853f + g->anim_t;
        float sp = speed * (0.55f + 0.45f * (float)((i * 17) % 10) * 0.1f);
        pb_particles_emit(&g->fx, x, y, cosf(a) * sp, sinf(a) * sp - speed * 0.25f,
                          0.28f + 0.04f * (float)(i % 5), c);
    }
}

static void bake_tile_flags(game* g){
    for(int y = 0; y < TH; y++){
        for(int x = 0; x < TW; x++){
            int k = g->tiles[y][x].kind;
            if(k != T_SOLID && k != T_SPRING){ g->tiles[y][x].flags = 0; continue; }
            uint8_t f = 0;
            if(y == 0 || (g->tiles[y - 1][x].kind != T_SOLID && g->tiles[y - 1][x].kind != T_SPRING)) f |= 1u;
            if(x == 0 || g->tiles[y][x - 1].kind != T_SOLID) f |= 2u;
            if(x == TW - 1 || g->tiles[y][x + 1].kind != T_SOLID) f |= 4u;
            g->tiles[y][x].flags = f;
        }
    }
}

static void load_level(game* g){
    memset(g->tiles, 0, sizeof(g->tiles));
    g->coin_count = g->enemy_count = g->coins_left = g->coins_total = 0;
    g->spawn_x = 2; g->spawn_y = 10;
    g->goal_x = TW - 3; g->goal_y = TH - 3;
    g->ck_x = g->spawn_x; g->ck_y = g->spawn_y;
    g->has_checkpoint = 0;

    for(int y = 0; y < TH; y++){
        const char* row = LEVEL[y];
        for(int x = 0; x < TW; x++){
            switch(row[x]){
                case '#': g->tiles[y][x].kind = T_SOLID; break;
                case '=': g->tiles[y][x].kind = T_PLATFORM; break;
                case '^': g->tiles[y][x].kind = T_SPIKE; break;
                case '*': g->tiles[y][x].kind = T_SPRING; break;
                case 'H': g->tiles[y][x].kind = T_CHECKPOINT; break;
                case 'F':
                    g->tiles[y][x].kind = T_GOAL;
                    g->goal_x = x; g->goal_y = y;
                    break;
                case 'S':
                    g->spawn_x = x; g->spawn_y = y;
                    g->ck_x = x; g->ck_y = y;
                    break;
                case 'C':
                    if(g->coin_count < MAX_COINS){
                        coin* c = &g->coins[g->coin_count++];
                        c->x = (float)x + 0.5f;
                        c->y = (float)y + 0.5f;
                        c->taken = 0;
                        c->phase = (float)x * 0.37f;
                        g->coins_left++;
                        g->coins_total++;
                    }
                    break;
                case 'E':
                    if(g->enemy_count < MAX_ENEMIES){
                        enemy* e = &g->enemies[g->enemy_count++];
                        e->x = (float)x; e->y = (float)y;
                        e->vx = (g->enemy_count & 1) ? 2.5f : -2.5f;
                        e->facing = e->vx > 0 ? 1 : -1;
                        e->alive = 1;
                        e->anim = (float)g->enemy_count;
                    }
                    break;
                default: break;
            }
        }
    }
    bake_tile_flags(g);
}

static void place_player(game* g){
    g->pw = 0.68f;
    g->ph = 1.40f;
    g->px = (float)g->ck_x + 0.15f;
    g->py = (float)g->ck_y - g->ph + 0.95f;
    g->pvx = g->pvy = 0;
    g->on_ground = 0;
    g->on_wall = 0;
    g->facing = 1;
    g->air_jumps = 1;
    g->coyote = 0;
    g->jump_buf = 0;
    g->invuln = 1.4f;
    g->land_dust = 0;
    g->wall_slide = 0;
    g->was_ground = 0;
    g->shake = 0;
    g->combo = 0;
    g->combo_t = 0;
    g->cam_x = clampf(g->px - 20.0f, 0, (float)TW);
    g->cam_y = clampf(g->py - 8.0f, 0, (float)TH);
}

static void reset_run(game* g){
    load_level(g);
    place_player(g);
    g->score = 0;
    g->lives = MAX_LIVES;
    g->state = ST_PLAY;
    g->state_t = 0;
    g->anim_t = 0;
    g->accum = 0;
    g->run_time = 0;
    g->toast_t = 0;
    pb_particles_free(&g->fx);
    pb_particles_init(&g->fx, 192);
    toast(g, "Go!", pb_rgb(120, 255, 200));
}

static void soft_respawn(game* g){
    place_player(g);
    g->state = ST_PLAY;
    g->state_t = 0;
    toast(g, g->has_checkpoint ? "Checkpoint!" : "Try again", pb_rgb(255, 200, 120));
}

static void sample_input(pb_app* app, input* in){
    memset(in, 0, sizeof *in);
    if(!pb_is_focused(app)) return;
    in->left = pb_is_key_down(app, PB_KEY_LEFT) || pb_is_char_down(app, 'a') || pb_is_char_down(app, 'A');
    in->right = pb_is_key_down(app, PB_KEY_RIGHT) || pb_is_char_down(app, 'd') || pb_is_char_down(app, 'D');
    in->up = pb_is_key_down(app, PB_KEY_UP) || pb_is_char_down(app, 'w') || pb_is_char_down(app, 'W');
    in->down = pb_is_key_down(app, PB_KEY_DOWN) || pb_is_char_down(app, 's') || pb_is_char_down(app, 'S');
    in->jump = pb_is_key_pressed(app, PB_KEY_UP) || pb_is_char_pressed(app, ' ') ||
               pb_is_char_pressed(app, 'w') || pb_is_char_pressed(app, 'W') ||
               pb_is_char_pressed(app, 'j') || pb_is_char_pressed(app, 'J');
    in->jump_held = pb_is_key_down(app, PB_KEY_UP) || pb_is_char_down(app, ' ') ||
                    pb_is_char_down(app, 'w') || pb_is_char_down(app, 'W') ||
                    pb_is_char_down(app, 'j') || pb_is_char_down(app, 'J');
    in->sprint = pb_is_char_down(app, 'z') || pb_is_char_down(app, 'Z');
}

static void move_axis(game* g, float* x, float* y, float* v, float w, float h, int axis_x, float dt){
    float delta = *v * dt;
    if(delta == 0.0f) return;
    float prev_y = *y;
    *x += axis_x ? delta : 0.0f;
    *y += axis_x ? 0.0f : delta;

    int x0 = (int)floorf(*x);
    int y0 = (int)floorf(*y);
    int x1 = (int)floorf(*x + w - 0.001f);
    int y1 = (int)floorf(*y + h - 0.001f);

    for(int ty = y0; ty <= y1; ty++){
        for(int tx = x0; tx <= x1; tx++){
            int kind = tile_kind(g, tx, ty);

            if(kind == T_PLATFORM){
                if(axis_x || delta <= 0) continue;
                float feet = *y + h;
                float prev_feet = prev_y + h;
                if(prev_feet <= (float)ty + 0.08f && feet >= (float)ty){
                    *y = (float)ty - h;
                    *v = 0;
                    g->on_ground = 1;
                }
                continue;
            }

            if(kind != T_SOLID && kind != T_SPRING) continue;

            if(axis_x){
                if(delta > 0){ *x = (float)tx - w; g->on_wall = 1; }
                else         { *x = (float)(tx + 1); g->on_wall = -1; }
                *v = 0;
            } else {
                if(delta > 0){
                    *y = (float)ty - h;
                    *v = 0;
                    g->on_ground = 1;
                    if(kind == T_SPRING){
                        *v = -16.5f;
                        g->on_ground = 0;
                        g->air_jumps = 1;
                        g->shake = fmaxf(g->shake, 0.2f);
                        fx_burst(g, *x + w * 0.5f, (float)ty, pb_rgb(120, 255, 180), 10, 8.0f);
                        toast(g, "Boing!", pb_rgb(140, 255, 190));
                    }
                } else {
                    *y = (float)(ty + 1);
                    *v = 0;
                }
            }
        }
    }
}

static int player_overlaps_tile(const game* g, int kind){
    int x0 = (int)floorf(g->px);
    int y0 = (int)floorf(g->py);
    int x1 = (int)floorf(g->px + g->pw - 0.001f);
    int y1 = (int)floorf(g->py + g->ph - 0.001f);
    for(int ty = y0; ty <= y1; ty++)
        for(int tx = x0; tx <= x1; tx++)
            if(tile_kind(g, tx, ty) == kind) return 1;
    return 0;
}

static int aabb(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh){
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static void kill_player(game* g){
    if(g->invuln > 0 || g->state != ST_PLAY) return;
    fx_burst(g, g->px + g->pw * 0.5f, g->py + g->ph * 0.4f, pb_rgb(255, 110, 140), 20, 9.0f);
    g->shake = 0.6f;
    g->lives--;
    g->combo = 0;
    g->state_t = 0;
    g->state = (g->lives <= 0) ? ST_OVER : ST_DEAD;
}

static void try_jump(game* g){
    if(g->on_ground || g->coyote > 0){
        g->pvy = -12.4f;
        g->on_ground = 0;
        g->coyote = 0;
        g->jump_buf = 0;
        g->on_wall = 0;
        g->air_jumps = 1;
        fx_burst(g, g->px + g->pw * 0.5f, g->py + g->ph, pb_rgb(190, 230, 255), 7, 5.0f);
        return;
    }
    if(g->on_wall){
        g->pvy = -11.2f;
        g->pvx = (float)(-g->on_wall) * 8.8f;
        g->facing = -g->on_wall;
        g->on_wall = 0;
        g->jump_buf = 0;
        g->coyote = 0;
        g->air_jumps = 1;
        fx_burst(g, g->px + g->pw * 0.5f, g->py + g->ph * 0.5f, pb_rgb(160, 255, 210), 8, 6.0f);
        return;
    }
    if(g->air_jumps > 0){
        g->air_jumps--;
        g->pvy = -10.2f;
        g->jump_buf = 0;
        fx_burst(g, g->px + g->pw * 0.5f, g->py + g->ph * 0.7f, pb_rgb(200, 180, 255), 8, 6.0f);
        return;
    }
    g->jump_buf = 0.14f;
}

static void update_enemies(game* g, float dt){
    for(int i = 0; i < g->enemy_count; i++){
        enemy* e = &g->enemies[i];
        if(!e->alive) continue;
        e->anim += dt;
        float next = e->x + e->vx * dt;
        int foot_x = (int)floorf(next + (e->vx > 0 ? 0.9f : 0.1f));
        int foot_y = (int)floorf(e->y + 1.05f);
        int side_x = (int)floorf(next + (e->vx > 0 ? 0.95f : 0.05f));
        int body_y = (int)floorf(e->y + 0.5f);
        if(!solid_at(g, foot_x, foot_y) || solid_at(g, side_x, body_y) ||
           tile_kind(g, foot_x, foot_y) == T_SPIKE){
            e->vx = -e->vx;
            e->facing = e->vx > 0 ? 1 : -1;
        } else e->x = next;

        if(g->state == ST_PLAY && g->invuln <= 0){
            if(aabb(g->px, g->py, g->pw, g->ph, e->x + 0.05f, e->y + 0.1f, 0.95f, 0.9f)){
                if(g->pvy > 0.55f && (g->py + g->ph) - e->y < 0.65f){
                    e->alive = 0;
                    g->pvy = -9.2f;
                    g->air_jumps = 1;
                    int pts = 200 + g->combo * 50;
                    g->score += pts;
                    g->combo++;
                    g->combo_t = 2.0f;
                    g->shake = 0.2f;
                    fx_burst(g, e->x + 0.5f, e->y + 0.4f, pb_rgb(255, 170, 90), 14, 8.0f);
                    char msg[48];
                    snprintf(msg, sizeof msg, "Stomp +%d", pts);
                    toast(g, msg, pb_rgb(255, 180, 100));
                } else kill_player(g);
            }
        }
    }
}

static void update_coins(game* g){
    for(int i = 0; i < g->coin_count; i++){
        coin* c = &g->coins[i];
        if(c->taken) continue;
        if(aabb(g->px, g->py, g->pw, g->ph, c->x - 0.4f, c->y - 0.4f, 0.8f, 0.8f)){
            c->taken = 1;
            g->coins_left--;
            g->combo++;
            g->combo_t = 2.2f;
            int pts = 100 + (g->combo - 1) * 25;
            g->score += pts;
            fx_burst(g, c->x, c->y, pb_rgb(255, 220, 80), 14, 7.0f);
            char msg[48];
            if(g->combo >= 3) snprintf(msg, sizeof msg, "Combo x%d  +%d", g->combo, pts);
            else snprintf(msg, sizeof msg, "+%d", pts);
            toast(g, msg, pb_rgb(255, 220, 90));
        }
    }
}

static void update_checkpoints(game* g){
    if(!player_overlaps_tile(g, T_CHECKPOINT)) return;
    int x0 = (int)floorf(g->px);
    int y0 = (int)floorf(g->py);
    int x1 = (int)floorf(g->px + g->pw - 0.001f);
    int y1 = (int)floorf(g->py + g->ph - 0.001f);
    for(int ty = y0; ty <= y1; ty++){
        for(int tx = x0; tx <= x1; tx++){
            if(tile_kind(g, tx, ty) != T_CHECKPOINT) continue;
            if(g->ck_x == tx && g->ck_y == ty && g->has_checkpoint) return;
            g->ck_x = tx;
            g->ck_y = ty;
            g->has_checkpoint = 1;
            g->score += 50;
            fx_burst(g, (float)tx + 0.5f, (float)ty + 0.5f, pb_rgb(120, 220, 255), 12, 5.0f);
            toast(g, "Checkpoint saved", pb_rgb(120, 220, 255));
            return;
        }
    }
}

static void update_camera(pb_app* app, game* g, float dt){
    int aw = pb_app_width(app);
    int ah = pb_app_height(app);
    float look = (float)g->facing * (2.8f + fabsf(g->pvx) * 0.4f);
    float tx = g->px + g->pw * 0.5f - (float)aw * 0.5f + look;
    float ty = g->py + g->ph * 0.3f - (float)ah * 0.55f;
    float max_x = (float)(TW > aw ? TW - aw : 0);
    float max_y = (float)(TH > ah ? TH - ah : 0);
    tx = clampf(tx, 0, max_x);
    ty = clampf(ty, 0, max_y);
    float k = 1.0f - expf(-11.0f * dt);
    g->cam_x = lerpf(g->cam_x, tx, k);
    g->cam_y = lerpf(g->cam_y, ty, k);
    if(g->shake > 0){
        g->shake = fmaxf(0, g->shake - dt);
        float mag = g->shake * 2.4f;
        g->cam_x = clampf(g->cam_x + sinf(g->anim_t * 70.0f) * mag * 0.15f, 0, max_x);
        g->cam_y = clampf(g->cam_y + cosf(g->anim_t * 55.0f) * mag * 0.12f, 0, max_y);
    }
}

static void physics_step(game* g, const input* in, float dt){
    const float gravity = 31.0f;
    const float max_fall = 20.0f;
    float max_run = in->sprint ? 10.8f : 7.4f;
    float accel = g->on_ground ? (in->sprint ? 72.0f : 60.0f) : 40.0f;
    float friction = in->sprint ? 26.0f : 50.0f;

    int drop = 0;
    if(in->down && in->jump && g->on_ground){
        int fy = (int)floorf(g->py + g->ph + 0.05f);
        int fx0 = (int)floorf(g->px + 0.1f);
        int fx1 = (int)floorf(g->px + g->pw - 0.1f);
        for(int tx = fx0; tx <= fx1; tx++){
            if(tile_kind(g, tx, fy) == T_PLATFORM){ drop = 1; break; }
        }
        if(drop){
            g->py += 0.25f;
            g->on_ground = 0;
            g->coyote = 0;
            g->pvy = 1.5f;
        }
    }
    if(in->jump && !drop) try_jump(g);

    int move = 0;
    if(in->left && !in->right) move = -1;
    if(in->right && !in->left) move = 1;

    if(move == 0){
        if(g->on_ground){
            float f = friction * dt;
            if(g->pvx > 0) g->pvx = fmaxf(0, g->pvx - f);
            if(g->pvx < 0) g->pvx = fminf(0, g->pvx + f);
        } else g->pvx *= (1.0f - 1.15f * dt);
    } else {
        g->pvx += (float)move * accel * dt;
        g->pvx = clampf(g->pvx, -max_run, max_run);
        g->facing = move;
    }

    g->was_ground = g->on_ground;
    g->on_ground = 0;
    g->on_wall = 0;
    move_axis(g, &g->px, &g->py, &g->pvx, g->pw, g->ph, 1, dt);

    if(!g->on_ground){
        int lx = (int)floorf(g->px - 0.05f);
        int rx = (int)floorf(g->px + g->pw + 0.05f);
        int midy = (int)floorf(g->py + g->ph * 0.5f);
        if(in->left && solid_at(g, lx, midy)) g->on_wall = -1;
        if(in->right && solid_at(g, rx, midy)) g->on_wall = 1;
    }

    g->pvy += gravity * dt;
    if(g->on_wall && g->pvy > 0){
        g->pvy = fminf(g->pvy, 3.2f);
        g->wall_slide += dt;
        if(((int)(g->wall_slide * 18.0f) % 3) == 0)
            pb_particles_emit(&g->fx, g->px + (g->on_wall > 0 ? g->pw : 0),
                              g->py + g->ph * 0.3f, (float)(-g->on_wall) * 1.5f, 1.0f,
                              0.2f, pb_rgb(180, 210, 230));
    } else g->wall_slide = 0;
    if(g->pvy > max_fall) g->pvy = max_fall;
    move_axis(g, &g->px, &g->py, &g->pvy, g->pw, g->ph, 0, dt);

    if(g->on_ground){ g->coyote = 0.11f; g->air_jumps = 1; }
    else g->coyote = fmaxf(0, g->coyote - dt);

    if(g->jump_buf > 0){
        g->jump_buf -= dt;
        if(g->on_ground || g->coyote > 0 || g->on_wall || g->air_jumps > 0) try_jump(g);
    }

    if(!in->jump_held && g->pvy < -3.5f) g->pvy *= 0.5f;

    if(g->on_ground && !g->was_ground){
        g->land_dust = 0.15f;
        fx_burst(g, g->px + g->pw * 0.5f, g->py + g->ph, pb_rgb(160, 140, 110), 5, 3.5f);
    }
    if(g->on_ground && fabsf(g->pvx) > 3.2f){
        g->land_dust += dt;
        if(g->land_dust > 0.045f){
            g->land_dust = 0;
            pb_particles_emit(&g->fx, g->px + g->pw * 0.5f, g->py + g->ph,
                              -g->pvx * 0.12f, -1.0f, 0.22f, pb_rgb(150, 130, 100));
        }
    }

    if(g->invuln > 0) g->invuln -= dt;
    if(g->combo_t > 0){ g->combo_t -= dt; if(g->combo_t <= 0) g->combo = 0; }
    if(g->toast_t > 0) g->toast_t -= dt;

    update_coins(g);
    update_enemies(g, dt);
    update_checkpoints(g);

    if(player_overlaps_tile(g, T_SPIKE)) kill_player(g);
    if(g->py > (float)TH + 2.0f) kill_player(g);

    if(player_overlaps_tile(g, T_GOAL)){
        if(g->coins_left == 0){
            int time_bonus = (int)fmaxf(0, 180.0f - g->run_time) * 10;
            g->state = ST_WIN;
            g->state_t = 0;
            g->score += 1000 + g->lives * 500 + time_bonus;
            g->shake = 0.45f;
            fx_burst(g, g->px + g->pw * 0.5f, g->py, pb_rgb(120, 255, 180), 28, 11.0f);
        }
    }
}

static void on_event(pb_app* app, void* user, const pb_event* ev){
    game* g = (game*)user;
    if(ev->type == PB_EVENT_QUIT){ pb_app_quit(app); return; }
    if(ev->type != PB_EVENT_KEY || !ev->as.key.pressed) return;

    pb_key k = ev->as.key.key;
    uint32_t cp = ev->as.key.codepoint;
    int esc = (k == PB_KEY_ESC || cp == 'q' || cp == 'Q');
    int ok = (k == PB_KEY_ENTER || cp == ' ' || cp == 'r' || cp == 'R' || cp == 's' || cp == 'S');

    if(cp == 'f' || cp == 'F' || k == PB_KEY_F3){
        g->show_stats = !g->show_stats;
        return;
    }

    if(g->state == ST_TITLE){
        if(esc) pb_app_quit(app);
        else if(ok) reset_run(g);
        return;
    }
    if(g->state == ST_PAUSE){
        if(esc) pb_app_quit(app);
        else if(cp == 'p' || cp == 'P' || ok){ g->state = g->paused_from; g->state_t = 0; }
        return;
    }
    if(g->state == ST_PLAY){
        if(esc) pb_app_quit(app);
        else if(cp == 'p' || cp == 'P'){
            g->paused_from = ST_PLAY;
            g->state = ST_PAUSE;
            g->state_t = 0;
        }
        return;
    }
    if(g->state == ST_DEAD || g->state == ST_WIN || g->state == ST_OVER){
        if(esc){ g->state = ST_TITLE; g->state_t = 0; }
        else if(ok){
            if(g->state == ST_DEAD) soft_respawn(g);
            else reset_run(g);
        }
    }
}

static void on_update(pb_app* app, void* user, double dt){
    game* g = (game*)user;
    if(dt < 0) dt = 0;
    if(dt > 0.1) dt = 0.1;
    float fdt = (float)dt;
    g->anim_t += fdt;
    g->state_t += fdt;
    g->fps_smooth = pb_get_fps(app);
    pb_particles_update(&g->fx, dt);

    if(g->state == ST_DEAD && g->state_t > 1.0f) soft_respawn(g);
    if(g->state != ST_PLAY){ update_camera(app, g, fdt); return; }

    g->run_time += fdt;
    input in;
    sample_input(app, &in);
    g->accum += fdt;
    if(g->accum > 0.25f) g->accum = 0.25f;
    int steps = 0;
    while(g->accum >= FIXED_DT && steps < 8){
        physics_step(g, &in, FIXED_DT);
        g->accum -= FIXED_DT;
        steps++;
        if(g->state != ST_PLAY) break;
        in.jump = 0;
    }
    update_camera(app, g, fdt);
}

static void draw_sky(pb_fb* fb, game* g){
    int cx = (int)floorf(g->cam_x);
    int cy = (int)floorf(g->cam_y);
    pb_fb_fill_gradient_v(fb, cx, cy, fb->w, fb->h, pb_rgb(12, 22, 46), pb_rgb(60, 120, 170));
    pb_fb_fill_dither(fb, cx, cy + fb->h - 7, fb->w, 4, pb_rgb(40, 75, 105), pb_rgb(28, 55, 85), 1);
    for(int i = 0; i < 7; i++){
        float drift = g->anim_t * (3.5f + (float)(i % 3)) + (float)i * 19.0f;
        float span = (float)(fb->w + 24);
        int bx = cx + (int)(drift - span * floorf(drift / span)) - 12;
        int by = cy + 2 + (i * 2) % 6;
        pb_color cloud = pb_rgb(210, 225, 245);
        pb_fb_braille_fill_circle(fb, bx * 2, by * 4, 5, cloud);
        pb_fb_braille_fill_circle(fb, bx * 2 + 7, by * 4 + 1, 4, cloud);
    }
}

static void draw_tiles(pb_fb* fb, game* g){
    int cx = (int)floorf(g->cam_x), cy = (int)floorf(g->cam_y);
    int x0 = clampi(cx - 1, 0, TW - 1);
    int y0 = clampi(cy - 1, 0, TH - 1);
    int x1 = clampi(cx + fb->w + 1, 0, TW);
    int y1 = clampi(cy + fb->h + 1, 0, TH);

    pb_color grass = pb_rgb(72, 190, 100);
    pb_color grass_hi = pb_rgb(130, 235, 150);
    pb_color dirt = pb_rgb(120, 82, 52);
    pb_color dirt_dk = pb_rgb(90, 60, 40);
    pb_color rock = pb_rgb(70, 78, 88);
    pb_color dark = pb_rgb(16, 20, 28);
    pb_color plat = pb_rgb(180, 140, 90);
    pb_color plat_hi = pb_rgb(230, 200, 130);
    pb_color spring = pb_rgb(90, 255, 160);
    pb_color ck = pb_rgb(100, 210, 255);

    for(int y = y0; y < y1; y++){
        for(int x = x0; x < x1; x++){
            tile t = g->tiles[y][x];
            if(t.kind == T_SOLID){
                if(t.flags & 1u){
                    pb_fb_put(fb, x, y, pb_cell_make(0x2588, grass, dark, 0));
                    pb_fb_put_blend(fb, x, y, pb_cell_make(0x2580, grass_hi, grass, 0), 0.8f, PB_BLEND_ALPHA);
                    if(((x * 3 + y * 7) & 7) == 0)
                        pb_fb_put(fb, x, y, pb_cell_make(0x273F, pb_rgb(255, 160, 200), grass, 0));
                } else {
                    uint32_t ch = ((x + y) & 1) ? 0x2593u : 0x2592u;
                    pb_fb_put(fb, x, y, pb_cell_make(ch, (t.flags & 1u) ? dirt : dirt_dk, rock, 0));
                }
            } else if(t.kind == T_PLATFORM){
                pb_fb_put(fb, x, y, pb_cell_make(0x2550, plat_hi, dark, PB_STYLE_BOLD));
                pb_fb_put_blend(fb, x, y, pb_cell_make(0x2501, plat, dark, 0), 0.5f, PB_BLEND_ALPHA);
            } else if(t.kind == T_SPRING){
                float pulse = 0.7f + 0.3f * sinf(g->anim_t * 8.0f + x);
                pb_fb_put(fb, x, y, pb_cell_make(0x25B2, pb_color_fade(spring, pulse), dark, PB_STYLE_BOLD));
                pb_fb_put(fb, x, y, pb_cell_make(0x2666, spring, dark, 0));
            } else if(t.kind == T_SPIKE){
                pb_fb_put(fb, x, y, pb_cell_make(0x25B2, pb_rgb(240, 70, 90), dark, PB_STYLE_BOLD));
            } else if(t.kind == T_CHECKPOINT){
                int active = (g->has_checkpoint && g->ck_x == x && g->ck_y == y);
                pb_color c = active ? ck : pb_rgb(70, 110, 140);
                pb_fb_put(fb, x, y, pb_cell_make(0x2691, c, dark, active ? PB_STYLE_BOLD : 0));
                if(active)
                    pb_fb_braille_fill_circle(fb, x * 2 + 1, y * 4 + 1, 2, pb_color_fade(ck, 0.5f));
            } else if(t.kind == T_GOAL){
                float pulse = 0.55f + 0.45f * sinf(g->anim_t * 5.0f);
                pb_color flag = pb_rgb(90, 255, 170);
                pb_fb_put(fb, x, y + 1, pb_cell_make(0x2502, pb_rgb(210, 220, 230), dark, 0));
                pb_fb_put(fb, x, y, pb_cell_make(0x2691, pb_color_fade(flag, pulse), dark, PB_STYLE_BOLD));
                pb_fb_braille_fill_triangle(fb, x * 2 + 1, y * 4, x * 2 + 6, y * 4 + 2, x * 2 + 1, y * 4 + 4,
                                            pb_color_fade(flag, pulse));
                pb_fb_put(fb, x, y - 1, pb_cell_make(g->coins_left ? '!' : 0x2726,
                          g->coins_left ? pb_rgb(255, 210, 70) : pb_rgb(180, 255, 200), dark,
                          PB_STYLE_BOLD));
            }
        }
    }
}

static void draw_coin(pb_fb* fb, coin* c, float t){
    float bob = sinf(t * 5.0f + c->phase) * 0.22f;
    int cx = (int)(c->x * 2.0f);
    int cy = (int)((c->y + bob) * 4.0f);
    pb_color gold = pb_rgb(255, 215, 60);
    pb_fb_braille_fill_circle(fb, cx, cy, 2, gold);
    if(sinf(t * 6.0f + c->phase) > 0)
        pb_fb_braille_plot(fb, cx, cy - 1, pb_rgb(255, 245, 160));
    pb_fb_put_blend(fb, (int)floorf(c->x), (int)floorf(c->y + bob),
                    pb_cell_make(0x25CF, gold, pb_rgb(20, 24, 36), PB_STYLE_BOLD),
                    0.5f, PB_BLEND_ALPHA);
}

static void draw_enemy(pb_fb* fb, enemy* e){
    float squash = 1.0f + 0.1f * sinf(e->anim * 10.0f);
    int hx = (int)floorf(e->x);
    int hy = (int)(e->y * 2.0f);
    int hh = (int)(1.55f * squash);
    pb_color body = pb_rgb(255, 115, 70);
    pb_fb_plot_fill_rect(fb, hx, hy + (2 - hh), 1, hh, body);
    pb_fb_plot(fb, hx, hy + (2 - hh), pb_rgb(255, 255, 255));
    pb_fb_quad_plot(fb, hx * 2 + (e->facing > 0 ? 1 : 0), (int)(e->y * 2.0f), pb_rgb(20, 20, 30));
    pb_fb_put_blend(fb, hx, (int)floorf(e->y),
                    pb_cell_make(e->facing > 0 ? 0x25E4 : 0x25E5, pb_rgb(120, 40, 30), body, 0),
                    0.65f, PB_BLEND_ALPHA);
}

static void draw_player(pb_fb* fb, game* g){
    if(g->invuln > 0 && ((int)(g->anim_t * 16) & 1)) return;
    int hx = (int)floorf(g->px);
    int hy = (int)floorf(g->py * 2.0f);
    int hh = (int)ceilf(g->ph * 2.0f);
    pb_color body = pb_rgb(80, 210, 255);
    pb_color suit = pb_rgb(40, 120, 200);
    pb_color skin = pb_rgb(255, 220, 180);
    pb_color scarf = pb_rgb(255, 90, 120);
    pb_fb_plot_fill_rect(fb, hx, hy, 1, hh, body);
    pb_fb_plot_fill_rect(fb, hx, hy + hh - 2, 1, 2, suit);
    pb_fb_plot(fb, hx, hy, skin);
    pb_fb_quad_plot(fb, hx * 2 + (g->facing > 0 ? 1 : 0), (int)(g->py * 2.0f), pb_rgb(20, 30, 50));
    float flutter = sinf(g->anim_t * 14.0f + g->px) * 0.5f;
    pb_fb_braille_line(fb,
        (int)((g->px + (g->facing > 0 ? 0.1f : 0.9f)) * 2.0f), (int)((g->py + 0.55f) * 4.0f),
        (int)((g->px + (g->facing > 0 ? -0.45f : 1.45f) + flutter * 0.2f) * 2.0f), (int)((g->py + 0.72f) * 4.0f),
        scarf);
    if(!g->on_ground && g->air_jumps == 0){
        /* double-jump spent trail */
        pb_fb_braille_plot_blend(fb, (int)((g->px + 0.3f) * 2), (int)((g->py + g->ph) * 4),
                                 pb_rgb(180, 160, 255), 0.5f);
    }
    if(g->on_wall) pb_fb_plot(fb, hx, hy + 1, pb_rgb(255, 240, 200));
}

static void draw_hud(pb_fb* fb, game* g){
    int w = fb->w;
    pb_color bar = pb_rgb(10, 14, 22);
    pb_color fg = pb_rgb(230, 236, 245);
    pb_color gold = pb_rgb(255, 210, 70);
    pb_color heart = pb_rgb(255, 80, 120);
    pb_color dim = pb_rgb(90, 100, 120);
    pb_fb_fill_rect(fb, 0, 0, w, 1, pb_cell_make(0x2588, bar, bar, 0));

    char line[64];
    pb_fb_put(fb, 1, 0, pb_cell_make(0x2605, gold, bar, 0));
    snprintf(line, sizeof line, "%d", g->score);
    pb_fb_text(fb, 3, 0, line, fg, bar, PB_STYLE_BOLD);

    pb_fb_put(fb, 12, 0, pb_cell_make(0x25CF, gold, bar, PB_STYLE_BOLD));
    snprintf(line, sizeof line, "%d/%d", g->coins_total - g->coins_left, g->coins_total);
    pb_fb_text(fb, 14, 0, line, gold, bar, 0);

    int sec = (int)g->run_time;
    snprintf(line, sizeof line, "%d:%02d", sec / 60, sec % 60);
    pb_fb_text(fb, w / 2 - 2, 0, line, dim, bar, 0);

    if(g->combo >= 2){
        snprintf(line, sizeof line, "x%d", g->combo);
        pb_fb_text(fb, w / 2 + 5, 0, line, pb_rgb(255, 180, 90), bar, PB_STYLE_BOLD);
    }

    for(int i = 0; i < MAX_LIVES; i++){
        pb_color c = (i < g->lives) ? heart : dim;
        pb_fb_put(fb, w - 2 - (MAX_LIVES - 1 - i) * 2, 0,
                  pb_cell_make(0x2665, c, bar, i < g->lives ? PB_STYLE_BOLD : PB_STYLE_DIM));
    }

    /* progress toward goal */
    float prog = clampf(g->px / (float)(g->goal_x > 1 ? g->goal_x : TW - 1), 0, 1);
    int pw = (int)((w - 4) * prog);
    if(pw > 0) pb_fb_fill_rect(fb, 2, 1, pw, 1, pb_cell_make(0x2501, pb_rgb(80, 200, 255), bar, 0));
}

static void draw_overlay(pb_fb* fb, game* g){
    pb_popup_desc pop;
    pb_popup_desc_init(&pop);

    if(g->state == ST_TITLE){
        pop.title = "Playbox Platformer";
        pop.body =
            "Collect every coin, hit checkpoints, reach the flag.\n"
            "\n"
            "A/D arrows     move          Z sprint\n"
            "Space/W/J      jump / double / wall\n"
            "Down+Jump      drop through platforms\n"
            "* springs bounce   = one-way ledges\n"
            "F frame stats      P pause";
        pop.hint = "Enter / Space  start          Esc quit";
        pop.width = 52;
        pb_popup_draw(fb, &pop);
        return;
    }
    if(g->state == ST_PAUSE){
        pop.title = "Paused";
        pop.body = "World frozen.\nYour checkpoint is safe.";
        pop.hint = "P / Enter resume    Esc quit";
        pop.width = 36;
        pb_popup_draw(fb, &pop);
        return;
    }
    if(g->state == ST_DEAD){
        pb_toast_draw(fb, fb->h / 2, "Ouch!", pb_rgb(255, 120, 150), pb_rgb(8, 12, 20), PB_STYLE_BOLD);
        return;
    }
    if(g->state == ST_WIN){
        char body[128];
        int sec = (int)g->run_time;
        snprintf(body, sizeof body, "Score  %d\nTime   %d:%02d\nCoins  %d/%d",
                 g->score, sec / 60, sec % 60, g->coins_total, g->coins_total);
        pop.title = "Stage clear!";
        pop.body = body;
        pop.hint = "R / Enter again    Esc title";
        pop.border = pb_rgb(120, 255, 180);
        pop.width = 40;
        pb_popup_draw(fb, &pop);
        return;
    }
    if(g->state == ST_OVER){
        char body[96];
        snprintf(body, sizeof body, "Score  %d\nTime   %d:%02d",
                 g->score, (int)g->run_time / 60, (int)g->run_time % 60);
        pop.title = "Game over";
        pop.body = body;
        pop.hint = "R / Enter retry    Esc title";
        pop.border = pb_rgb(255, 100, 130);
        pop.title_fg = pb_rgb(255, 170, 180);
        pop.width = 36;
        pb_popup_draw(fb, &pop);
    }
}

static void draw_world(pb_fb* fb, game* g){
    draw_sky(fb, g);
    draw_tiles(fb, g);
    for(int i = 0; i < g->coin_count; i++)
        if(!g->coins[i].taken) draw_coin(fb, &g->coins[i], g->anim_t);
    for(int i = 0; i < g->enemy_count; i++)
        if(g->enemies[i].alive) draw_enemy(fb, &g->enemies[i]);
    if(g->state == ST_PLAY || g->state == ST_PAUSE || g->state == ST_DEAD)
        draw_player(fb, g);
    for(int i = 0; i < g->fx.count; i++){
        pb_particle* p = &g->fx.items[i];
        if(!p->alive) continue;
        pb_fb_plot_blend(fb, (int)p->x, (int)(p->y * 2.0f), p->color, p->life / p->max_life);
    }
}

static void on_draw(pb_app* app, void* user, pb_fb* fb){
    game* g = (game*)user;
    if(g->state == ST_TITLE){
        pb_fb_set_camera(fb, 0, 0);
        pb_fb_fill_gradient_v(fb, 0, 0, fb->w, fb->h, pb_rgb(8, 14, 28), pb_rgb(30, 70, 120));
        for(int i = 0; i < 10; i++){
            float a = g->anim_t * 1.1f + (float)i * 0.7f;
            int cx = fb->w / 2 + (int)(cosf(a) * (fb->w * 0.32f));
            int cy = fb->h / 2 + (int)(sinf(a * 0.8f) * (fb->h * 0.28f));
            pb_fb_braille_fill_circle(fb, cx * 2, cy * 4, 2 + (i & 1), pb_rgb(255, 210, 80));
        }
        pb_fb_plot_fill_rect(fb, fb->w / 2, fb->h - 6, 1, 3, pb_rgb(80, 210, 255));
        draw_overlay(fb, g);
        if(g->show_stats) pb_ui_draw_frame_stats(fb, app, 1, 2);
        return;
    }

    pb_fb_set_camera(fb, (int)floorf(g->cam_x), (int)floorf(g->cam_y));
    draw_world(fb, g);
    pb_fb_set_camera(fb, 0, 0);
    draw_hud(fb, g);

    if(g->toast_t > 0 && g->state == ST_PLAY)
        pb_toast_draw(fb, 2, g->toast_msg, g->toast_fg, pb_rgb(8, 12, 20), PB_STYLE_BOLD);

    if(g->state == ST_PLAY && g->coins_left > 0 &&
       aabb(g->px, g->py, g->pw, g->ph, (float)g->goal_x - 1.5f, (float)g->goal_y - 1.5f, 4, 4))
        pb_fb_text_centered(fb, fb->h - 1, "Collect all coins first!",
                            pb_rgb(255, 210, 90), pb_rgb(8, 12, 20), PB_STYLE_BOLD);
    if(g->state == ST_PLAY && g->on_wall && !g->on_ground)
        pb_fb_text(fb, 1, fb->h - 1, "wall-jump!", pb_rgb(160, 255, 210), pb_rgb(8, 12, 20), 0);

    draw_overlay(fb, g);
    if(g->show_stats) pb_ui_draw_frame_stats(fb, app, 1, 2);
}

static void on_shutdown(pb_app* app, void* user){
    (void)app;
    pb_particles_free(&((game*)user)->fx);
}

int main(void){
    game g;
    memset(&g, 0, sizeof g);
    pb_particles_init(&g.fx, 192);
    load_level(&g);
    g.state = ST_TITLE;
    g.lives = MAX_LIVES;

    pb_app_desc desc;
    memset(&desc, 0, sizeof desc);
    desc.title = "Playbox Platformer";
    desc.target_fps = 120;
    desc.clear = pb_cell_make(' ', pb_rgb(220, 224, 230), pb_rgb(10, 14, 24), 0);
    desc.on_event = on_event;
    desc.on_update = on_update;
    desc.on_draw = on_draw;
    desc.on_shutdown = on_shutdown;

    pb_app* app = pb_app_create(&desc, &g);
    if(!app){
        fprintf(stderr, "pb_app_create failed\n");
        pb_particles_free(&g.fx);
        return 1;
    }
    pb_app_set_vsync(app, 1);
    pb_app_set_refresh_hz(app, 120);
    int rc = pb_app_run(app);
    pb_app_destroy(app);
    return rc;
}
