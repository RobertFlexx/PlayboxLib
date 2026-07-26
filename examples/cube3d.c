#include "playbox/pb.h"
#include "playbox/pb_math.h"
#include "playbox/pb_3d.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    pb_3d* gfx;
    float yaw;
    float pitch;
    float dist;
    float spin;
    int wire;
    int show_help;
    int drag;
    int last_mx, last_my;
} cube_state;

static void on_event(pb_app* app, void* user, const pb_event* ev){
    cube_state* s = (cube_state*)user;
    if(ev->type == PB_EVENT_QUIT){
        pb_app_quit(app);
        return;
    }
    if(ev->type == PB_EVENT_KEY && ev->as.key.pressed){
        pb_key k = ev->as.key.key;
        uint32_t cp = ev->as.key.codepoint;
        if(k == PB_KEY_ESC || cp == 'q' || cp == 'Q'){ pb_app_quit(app); return; }
        if(cp == 'h' || cp == 'H') s->show_help = !s->show_help;
        if(cp == 'w' || cp == 'W') s->wire = !s->wire;
        if(cp == 'r' || cp == 'R'){ s->yaw = 0.6f; s->pitch = 0.35f; s->dist = 7.0f; }
    }
    if(ev->type == PB_EVENT_MOUSE){
        const pb_mouse_event* m = &ev->as.mouse;
        if(m->wheel != 0){
            s->dist -= (float)m->wheel * 0.4f;
            if(s->dist < 2.5f) s->dist = 2.5f;
            if(s->dist > 20.f) s->dist = 20.f;
        }
    }
}

static void on_update(pb_app* app, void* user, double dt){
    cube_state* s = (cube_state*)user;
    if(dt < 0) dt = 0;
    if(dt > 0.25) dt = 0.25;
    s->spin += (float)dt * 0.9f;

    float turn = 1.4f * (float)dt;
    if(pb_is_key_down(app, PB_KEY_LEFT) || pb_is_char_down(app, 'a')) s->yaw += turn;
    if(pb_is_key_down(app, PB_KEY_RIGHT) || pb_is_char_down(app, 'd')) s->yaw -= turn;
    if(pb_is_key_down(app, PB_KEY_UP) || pb_is_char_down(app, 'i')) s->pitch += turn;
    if(pb_is_key_down(app, PB_KEY_DOWN) || pb_is_char_down(app, 'k')) s->pitch -= turn;
    if(pb_is_char_down(app, 'z')) s->dist -= 3.0f * (float)dt;
    if(pb_is_char_down(app, 'x')) s->dist += 3.0f * (float)dt;

    if(pb_is_mouse_button_pressed(app, PB_MOUSE_LEFT)){
        s->drag = 1;
        s->last_mx = pb_get_mouse_x(app);
        s->last_my = pb_get_mouse_y(app);
    }
    if(!pb_is_mouse_button_down(app, PB_MOUSE_LEFT)) s->drag = 0;
    if(s->drag){
        int mx = pb_get_mouse_x(app);
        int my = pb_get_mouse_y(app);
        s->yaw   -= (float)(mx - s->last_mx) * 0.02f;
        s->pitch += (float)(my - s->last_my) * 0.02f;
        s->last_mx = mx;
        s->last_my = my;
    }

    if(s->pitch < -1.2f) s->pitch = -1.2f;
    if(s->pitch > 1.2f) s->pitch = 1.2f;
    if(s->dist < 2.5f) s->dist = 2.5f;
    if(s->dist > 20.f) s->dist = 20.f;
}

static void on_draw(pb_app* app, void* user, pb_fb* fb){
    cube_state* s = (cube_state*)user;
    (void)app;

    pb_color bg = pb_rgb(8, 10, 18);
    pb_fb_clear(fb, pb_cell_make(' ', pb_rgb(200,210,220), bg, 0));

    pb_camera3d cam = pb_camera3d_default();
    float cp = cosf(s->pitch), sp = sinf(s->pitch);
    float cy = cosf(s->yaw), sy = sinf(s->yaw);
    cam.position = pb_v3(s->dist * cp * sy, s->dist * sp + 0.5f, s->dist * cp * cy);
    cam.target = pb_v3(0, 0.5f, 0);
    cam.fovy = 55.0f * PB_DEG2RAD;
    cam.znear = 0.35f;

    if(!pb_3d_begin(s->gfx, fb, PB_3D_HALF, &cam)) return;
    pb_3d_set_light(s->gfx, pb_v3(0.55f, 1.0f, 0.35f), 0.35f);

    pb_color grid_c = pb_rgb(40, 55, 80);
    pb_3d_grid(s->gfx, 4.0f, 1.0f, grid_c);

    pb_mat4 model = pb_m4_mul(pb_m4_rotate_y(s->spin), pb_m4_rotate_x(s->spin * 0.4f));
    pb_3d_cube(s->gfx, pb_v3(0, 0.85f, 0), pb_v3(1.6f, 1.6f, 1.6f), model,
               pb_rgb(90, 200, 255), s->wire);

    pb_3d_cube(s->gfx, pb_v3(2.4f, 0.4f, -1.2f), pb_v3(0.7f, 0.7f, 0.7f),
               pb_m4_rotate_y(-s->spin * 1.5f), pb_rgb(255, 120, 180), s->wire);

    pb_3d_end(s->gfx);

    pb_fb_text(fb, 1, 0, "PlayboxLib 3D — orbit cube", pb_rgb(120, 255, 200), bg, PB_STYLE_BOLD);
    char line[96];
    snprintf(line, sizeof line, "yaw %.1f  pitch %.1f  dist %.1f  %s",
             s->yaw, s->pitch, s->dist, s->wire ? "wire" : "solid");
    pb_fb_text(fb, 1, 1, line, pb_rgb(140, 155, 175), bg, 0);
    pb_fb_text(fb, 1, fb->h - 1, "A/D orbit  arrows/I/K pitch  Z/X zoom  drag LMB  wheel  W wire  H help  Q quit",
               pb_rgb(90, 110, 140), bg, 0);

    if(s->show_help){
        pb_popup_desc pop;
        pb_popup_desc_init(&pop);
        pop.title = "cube3d";
        pop.body = "Soft 3D raster into braille pixels with a depth buffer.\n"
                   "Orbit the camera, toggle wireframe, watch flat lighting.";
        pop.hint = "H close";
        pb_popup_draw(fb, &pop);
    }
}

int main(void){
    cube_state s;
    memset(&s, 0, sizeof(s));
    s.yaw = 0.7f;
    s.pitch = 0.4f;
    s.dist = 7.0f;
    s.show_help = 0;
    s.gfx = pb_3d_create();
    if(!s.gfx) return 1;

    pb_app_desc d;
    memset(&d, 0, sizeof(d));
    d.title = "PlayboxLib cube3d";
    d.target_fps = 60;
    d.flags = PB_APP_FLAG_CUSTOM_CLEAR;
    d.clear = pb_cell_make(' ', pb_rgb(200,210,220), pb_rgb(8,10,18), 0);
    d.on_event = on_event;
    d.on_update = on_update;
    d.on_draw = on_draw;

    pb_app* app = pb_app_create(&d, &s);
    if(!app){ pb_3d_destroy(s.gfx); return 1; }
    int ok = pb_app_run(app);
    pb_app_destroy(app);
    pb_3d_destroy(s.gfx);
    return ok ? 0 : 1;
}
