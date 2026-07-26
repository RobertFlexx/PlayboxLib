#include "playbox/pb_app.h"
#include "playbox/pb_state.h"
#include "playbox/pb_term.h"
#include "playbox/pb_input.h"
#include "playbox/pb_renderer.h"
#include "playbox/pb_time.h"
#include "playbox/pb_replay.h"
#include <stdlib.h>
#include <string.h>

/* Fallback when the terminal has no key-up (legacy mode). Keep this short so
 * movement stops almost immediately after the last key/repeat event. */
#define PB_HOLD_LEGACY_NS  100000000ull /* 100ms */
/* Safety net if a release event is dropped while using Kitty protocol. */
#define PB_HOLD_RELEASE_NS 2000000000ull /* 2s */

typedef struct {
    uint8_t down[PB_KEY_COUNT];
    uint8_t pressed[PB_KEY_COUNT];
    uint8_t released[PB_KEY_COUNT];
    uint64_t last_ns[PB_KEY_COUNT];

    uint8_t ascii_down[128];
    uint8_t ascii_pressed[128];
    uint8_t ascii_released[128];
    uint64_t ascii_last_ns[128];

    int mouse_x;
    int mouse_y;
    int mouse_wheel;
    uint8_t mouse_down[PB_MOUSE_BUTTON_COUNT];
    uint8_t mouse_pressed[PB_MOUSE_BUTTON_COUNT];
    uint8_t mouse_released[PB_MOUSE_BUTTON_COUNT];

    int focused;
    double dt;
    int fps;
    double fps_accum;
    int fps_frames;

    /* Frame detection */
    double update_ms;
    double draw_ms;
    double present_ms;
    double wait_ms;
    double frame_ms;
    int dropped;
    int hitch;
    uint64_t frame_index;
    uint64_t drop_count;
    uint64_t hitch_count;

    int have_key_release; /* 1 once we see a real key-up from the terminal */
} pb_input_state;

struct pb_app {
    pb_app_desc desc;
    void* user;
    pb_term* term;
    pb_input* input;
    pb_renderer* renderer;
    pb_fb fb;
    int running;
    int w;
    int h;
    pb_cell clear;
    char title[256];
    int target_fps;
    int vsync;
    int refresh_hz;

    pb_replay* rec;
    pb_replay* rep;

    pb_event* evbuf;
    int evlen;
    int evcap;

    uint32_t meta_seed;
    int meta_w;
    int meta_h;
    int flag_replay;
    int flag_record;

    pb_input_state state;
};

static void pb_set_title(pb_term* t, const char* s){
    if(!t || !s) return;
    char buf[512];
    size_t n = 0;
    buf[n++] = 0x1B; buf[n++] = ']'; buf[n++] = '0'; buf[n++] = ';';
    for(size_t i=0; s[i] && n < sizeof(buf)-2; i++) buf[n++] = s[i];
    buf[n++] = 0x07;
    pb_term_write(t, buf, (int)n);
}

static void pb_evbuf_clear(pb_app* app){
    app->evlen = 0;
}

static void pb_evbuf_push(pb_app* app, const pb_event* ev){
    if(!app || !ev) return;
    if(app->evlen + 1 > app->evcap){
        int nc = app->evcap ? app->evcap * 2 : 256;
        pb_event* nb = (pb_event*)realloc(app->evbuf, (size_t)nc * sizeof(pb_event));
        if(!nb) return;
        app->evbuf = nb;
        app->evcap = nc;
    }
    app->evbuf[app->evlen++] = *ev;
}

static uint32_t pb_env_u32(const char* name, uint32_t fallback){
    const char* s = getenv(name);
    if(!s || !s[0]) return fallback;
    char* end = NULL;
    unsigned long v = strtoul(s, &end, 0);
    if(end == s) return fallback;
    return (uint32_t)v;
}

static void pb_state_begin_frame(pb_input_state* st){
    memset(st->pressed, 0, sizeof(st->pressed));
    memset(st->released, 0, sizeof(st->released));
    memset(st->ascii_pressed, 0, sizeof(st->ascii_pressed));
    memset(st->ascii_released, 0, sizeof(st->ascii_released));
    memset(st->mouse_pressed, 0, sizeof(st->mouse_pressed));
    memset(st->mouse_released, 0, sizeof(st->mouse_released));
    st->mouse_wheel = 0;
}

static void pb_state_expire_holds(pb_input_state* st, uint64_t now){
    uint64_t hold = st->have_key_release ? PB_HOLD_RELEASE_NS : PB_HOLD_LEGACY_NS;
    for(int i = 0; i < PB_KEY_COUNT; i++){
        if(st->down[i] && st->last_ns[i] && (now - st->last_ns[i]) > hold){
            st->down[i] = 0;
            st->released[i] = 1;
        }
    }
    for(int i = 0; i < 128; i++){
        if(st->ascii_down[i] && st->ascii_last_ns[i] && (now - st->ascii_last_ns[i]) > hold){
            st->ascii_down[i] = 0;
            st->ascii_released[i] = 1;
        }
    }
}

static void pb_state_note_key(pb_input_state* st, const pb_key_event* ke, uint64_t now){
    if(!ke) return;
    if(!ke->pressed) st->have_key_release = 1;
    if(ke->key > PB_KEY_NONE && ke->key < PB_KEY_COUNT){
        int k = (int)ke->key;
        if(ke->pressed){
            if(!st->down[k]) st->pressed[k] = 1;
            st->down[k] = 1;
            st->last_ns[k] = now;
        }else{
            if(st->down[k]) st->released[k] = 1;
            st->down[k] = 0;
        }
    }
    if(ke->codepoint > 0 && ke->codepoint < 128u){
        int c = (int)ke->codepoint;
        if(ke->pressed){
            if(!st->ascii_down[c]) st->ascii_pressed[c] = 1;
            st->ascii_down[c] = 1;
            st->ascii_last_ns[c] = now;
        }else{
            if(st->ascii_down[c]) st->ascii_released[c] = 1;
            st->ascii_down[c] = 0;
        }
        /* Mirror letter case so 'W' release also clears 'w' hold and vice versa */
        if(c >= 'A' && c <= 'Z'){
            int lo = c - 'A' + 'a';
            if(ke->pressed){
                st->ascii_down[lo] = 1;
                st->ascii_last_ns[lo] = now;
            }else{
                st->ascii_down[lo] = 0;
            }
        }else if(c >= 'a' && c <= 'z'){
            int up = c - 'a' + 'A';
            if(ke->pressed){
                st->ascii_down[up] = 1;
                st->ascii_last_ns[up] = now;
            }else{
                st->ascii_down[up] = 0;
            }
        }
    }
}

static void pb_state_note_mouse(pb_input_state* st, const pb_mouse_event* me){
    if(!me) return;
    st->mouse_x = me->x;
    st->mouse_y = me->y;
    if(me->wheel) st->mouse_wheel += me->wheel;
    if(me->button == 0xFFu || me->wheel != 0) return;
    if(me->button < PB_MOUSE_BUTTON_COUNT){
        int b = (int)me->button;
        if(me->pressed){
            if(!st->mouse_down[b]) st->mouse_pressed[b] = 1;
            st->mouse_down[b] = 1;
        }else{
            if(st->mouse_down[b]) st->mouse_released[b] = 1;
            st->mouse_down[b] = 0;
        }
    }
}

static void pb_dispatch_event(pb_app* app, const pb_event* ev, uint64_t now){
    if(!app || !ev) return;

    if(ev->type == PB_EVENT_KEY){
        pb_state_note_key(&app->state, &ev->as.key, now);
    }else if(ev->type == PB_EVENT_TEXT){
        if(ev->as.text > 0 && ev->as.text < 128u){
            pb_key_event ke;
            memset(&ke, 0, sizeof(ke));
            ke.codepoint = ev->as.text;
            ke.pressed = 1;
            pb_state_note_key(&app->state, &ke, now);
        }
    }else if(ev->type == PB_EVENT_MOUSE){
        pb_state_note_mouse(&app->state, &ev->as.mouse);
    }else if(ev->type == PB_EVENT_FOCUS){
        app->state.focused = ev->as.focus.focused ? 1 : 0;
    }

    if(app->desc.on_event) app->desc.on_event(app, app->user, ev);

    /* Mirror printable KEY as TEXT for handlers that want character input.
       Not recorded separately — replay reconstructs it from KEY. */
    if(ev->type == PB_EVENT_KEY && ev->as.key.pressed &&
       ev->as.key.codepoint >= 32u && !ev->as.key.ctrl && !ev->as.key.alt){
        pb_event te;
        memset(&te, 0, sizeof(te));
        te.type = PB_EVENT_TEXT;
        te.as.text = ev->as.key.codepoint;
        if(app->desc.on_event) app->desc.on_event(app, app->user, &te);
    }
}

int pb_is_key_down(const pb_app* app, pb_key key){
    if(!app || key <= PB_KEY_NONE || key >= PB_KEY_COUNT) return 0;
    return app->state.down[key] ? 1 : 0;
}

int pb_is_key_pressed(const pb_app* app, pb_key key){
    if(!app || key <= PB_KEY_NONE || key >= PB_KEY_COUNT) return 0;
    return app->state.pressed[key] ? 1 : 0;
}

int pb_is_key_released(const pb_app* app, pb_key key){
    if(!app || key <= PB_KEY_NONE || key >= PB_KEY_COUNT) return 0;
    return app->state.released[key] ? 1 : 0;
}

int pb_is_char_down(const pb_app* app, uint32_t codepoint){
    if(!app || codepoint >= 128u) return 0;
    return app->state.ascii_down[codepoint] ? 1 : 0;
}

int pb_is_char_pressed(const pb_app* app, uint32_t codepoint){
    if(!app || codepoint >= 128u) return 0;
    return app->state.ascii_pressed[codepoint] ? 1 : 0;
}

int pb_get_mouse_x(const pb_app* app){ return app ? app->state.mouse_x : 0; }
int pb_get_mouse_y(const pb_app* app){ return app ? app->state.mouse_y : 0; }

int pb_is_mouse_button_down(const pb_app* app, int button){
    if(!app || button < 0 || button >= PB_MOUSE_BUTTON_COUNT) return 0;
    return app->state.mouse_down[button] ? 1 : 0;
}

int pb_is_mouse_button_pressed(const pb_app* app, int button){
    if(!app || button < 0 || button >= PB_MOUSE_BUTTON_COUNT) return 0;
    return app->state.mouse_pressed[button] ? 1 : 0;
}

int pb_is_mouse_button_released(const pb_app* app, int button){
    if(!app || button < 0 || button >= PB_MOUSE_BUTTON_COUNT) return 0;
    return app->state.mouse_released[button] ? 1 : 0;
}

int pb_get_mouse_wheel(const pb_app* app){ return app ? app->state.mouse_wheel : 0; }

void pb_app_set_cursor_visible(pb_app* app, int visible){
    if(!app || !app->term) return;
    pb_term_set_cursor_visible(app->term, visible);
}

void pb_app_set_mouse_capture(pb_app* app, int capture){
    if(!app || !app->term) return;
    pb_term_set_mouse_capture(app->term, capture);
    /* Keep cursor visibility in sync with capture. */
    pb_term_set_cursor_visible(app->term, capture ? 0 : 1);
}

double pb_get_frame_time(const pb_app* app){ return app ? app->state.dt : 0.0; }
int pb_get_fps(const pb_app* app){ return app ? app->state.fps : 0; }
int pb_is_focused(const pb_app* app){ return app ? app->state.focused : 0; }

void pb_get_frame_stats(const pb_app* app, pb_frame_stats* out){
    if(!out) return;
    memset(out, 0, sizeof(*out));
    if(!app) return;
    out->dt = app->state.dt;
    out->update_ms = app->state.update_ms;
    out->draw_ms = app->state.draw_ms;
    out->present_ms = app->state.present_ms;
    out->wait_ms = app->state.wait_ms;
    out->frame_ms = app->state.frame_ms;
    out->fps = app->state.fps;
    out->target_fps = app->target_fps;
    out->refresh_hz = app->refresh_hz;
    out->dropped = app->state.dropped;
    out->hitch = app->state.hitch;
    out->frame_index = app->state.frame_index;
    out->drop_count = app->state.drop_count;
    out->hitch_count = app->state.hitch_count;
}

int pb_get_dropped_frames(const pb_app* app){
    return app ? (int)app->state.drop_count : 0;
}

int pb_was_frame_dropped(const pb_app* app){
    return app ? app->state.dropped : 0;
}

int pb_was_frame_hitch(const pb_app* app){
    return app ? app->state.hitch : 0;
}

pb_app* pb_app_create(const pb_app_desc* desc, void* user){
    if(!desc || !desc->on_draw) return NULL;

    pb_app* app = (pb_app*)calloc(1, sizeof(pb_app));
    if(!app) return NULL;

    app->desc = *desc;
    app->user = user;

    app->term = pb_term_create();
    app->input = pb_input_create();
    app->renderer = pb_renderer_create();
    app->fb = pb_fb_make(0,0);

    app->running = 1;
    app->w = 0;
    app->h = 0;
    app->target_fps = desc->target_fps > 0 ? desc->target_fps : 60;
    app->vsync = (desc->flags & PB_APP_FLAG_NO_VSYNC) ? 0 : 1;
    {
        int env_hz = (int)pb_env_u32("PLAYBOX_REFRESH", 0);
        app->refresh_hz = env_hz > 0 ? env_hz : app->target_fps;
    }
    app->state.focused = 1;

    if(desc->flags & PB_APP_FLAG_CUSTOM_CLEAR){
        app->clear = desc->clear;
    }else{
        app->clear = pb_cell_make(' ', pb_rgb(220,220,220), pb_rgb(0,0,0), 0);
    }
    pb_renderer_set_clear(app->renderer, app->clear);

    if(desc->title){
        strncpy(app->title, desc->title, sizeof(app->title)-1);
        app->title[sizeof(app->title)-1] = 0;
    }else{
        strcpy(app->title, "PlayboxLib");
    }

    return app;
}

void pb_app_destroy(pb_app* app){
    if(!app) return;
    pb_fb_free(&app->fb);
    pb_renderer_destroy(app->renderer);
    pb_input_destroy(app->input);
    pb_term_destroy(app->term);

    pb_replay_close(app->rec);
    pb_replay_close(app->rep);
    free(app->evbuf);

    free(app);
}

void pb_app_quit(pb_app* app){
    if(!app) return;
    app->running = 0;
}

void pb_app_request_resize(pb_app* app){
    if(!app) return;
    pb_renderer_force_full_redraw(app->renderer);
}

int pb_app_width(const pb_app* app){
    return app ? app->w : 0;
}

int pb_app_height(const pb_app* app){
    return app ? app->h : 0;
}

void pb_app_set_title(pb_app* app, const char* title){
    if(!app || !title) return;
    strncpy(app->title, title, sizeof(app->title)-1);
    app->title[sizeof(app->title)-1] = 0;
    pb_set_title(app->term, app->title);
}

void pb_app_set_clear(pb_app* app, pb_cell clear){
    if(!app) return;
    app->clear = clear;
    pb_renderer_set_clear(app->renderer, clear);
}

void pb_app_set_target_fps(pb_app* app, int fps){
    if(!app) return;
    app->target_fps = fps > 0 ? fps : 60;
    if(app->refresh_hz <= 0) app->refresh_hz = app->target_fps;
}

void pb_app_set_vsync(pb_app* app, int enabled){
    if(!app) return;
    app->vsync = enabled ? 1 : 0;
}

int pb_app_get_vsync(const pb_app* app){
    return app ? app->vsync : 0;
}

void pb_app_set_refresh_hz(pb_app* app, int hz){
    if(!app) return;
    app->refresh_hz = hz > 0 ? hz : app->target_fps;
}

int pb_app_get_refresh_hz(const pb_app* app){
    return app ? app->refresh_hz : 0;
}

int pb_app_is_replay(const pb_app* app){
    return app ? app->flag_replay : 0;
}

int pb_app_is_recording(const pb_app* app){
    return app ? app->flag_record : 0;
}

uint32_t pb_app_replay_seed(const pb_app* app){
    return app ? app->meta_seed : 0u;
}

int pb_app_replay_initial_size(const pb_app* app, int* out_w, int* out_h){
    if(!app) return 0;
    if(out_w) *out_w = app->meta_w;
    if(out_h) *out_h = app->meta_h;
    return (app->meta_w > 0 && app->meta_h > 0) ? 1 : 0;
}

static void pb_apply_resize(pb_app* app, int w, int h){
    if(w < 1) w = 1;
    if(h < 1) h = 1;
    if(w == app->w && h == app->h) return;

    pb_fb_free(&app->fb);
    app->fb = pb_fb_make(w, h);
    app->w = w;
    app->h = h;
    pb_fb_clear(&app->fb, app->clear);
    pb_renderer_force_full_redraw(app->renderer);
}

int pb_app_run(pb_app* app){
    if(!app) return 0;
    if(!pb_term_enter(app->term)) return 0;
    pb_set_title(app->term, app->title);

    pb_input_attach(app->input, app->term);
    pb_renderer_bind(app->renderer, app->term);

    if(!(app->desc.flags & PB_APP_FLAG_NO_MOUSE)){
        pb_term_enable_mouse(app->term, 1);
    }
    if(!(app->desc.flags & PB_APP_FLAG_NO_FOCUS)){
        pb_term_request_focus(app->term);
    }

    const char* rep_path = getenv("PLAYBOX_REPLAY");
    const char* rec_path = getenv("PLAYBOX_RECORD");

    if(rep_path && rep_path[0]){
        app->rep = pb_replay_open_replay(rep_path);
        app->flag_replay = app->rep ? 1 : 0;
        app->flag_record = 0;
    }else if(rec_path && rec_path[0]){
        app->rec = pb_replay_open_record(rec_path);
        app->flag_record = app->rec ? 1 : 0;
        app->flag_replay = 0;
    }

    int w=0,h=0;
    pb_term_get_size(app->term, &w, &h);
    pb_apply_resize(app, w, h);

    if(app->flag_replay && app->rep){
        uint32_t iw=0, ih=0;
        app->meta_seed = pb_replay_get_seed(app->rep);
        pb_replay_get_initial_size(app->rep, &iw, &ih);
        app->meta_w = (int)iw;
        app->meta_h = (int)ih;
        if(app->meta_w > 0 && app->meta_h > 0){
            pb_apply_resize(app, app->meta_w, app->meta_h);
        }
    }

    if(app->flag_record && app->rec){
        uint32_t seed = pb_env_u32("PLAYBOX_SEED", (uint32_t)pb_time_ns());
        app->meta_seed = seed;
        app->meta_w = app->w;
        app->meta_h = app->h;
        pb_replay_record_set_meta(app->rec, seed, (uint32_t)app->meta_w, (uint32_t)app->meta_h);
    }

    if(app->desc.on_init) app->desc.on_init(app, app->user);

    uint64_t last = pb_time_ns();
    int refresh = app->refresh_hz > 0 ? app->refresh_hz : app->target_fps;
    if(refresh < 1) refresh = 60;
    uint64_t frame_budget_ns = 1000000000ull / (uint64_t)refresh;
    uint64_t next_deadline = last + frame_budget_ns;

    while(app->running){
        double rep_dt = 0.0;
        int have_rep_dt = 0;
        uint64_t frame_start = pb_time_ns();
        uint64_t t0, t1, t2, t3, t4;

        pb_evbuf_clear(app);
        pb_state_begin_frame(&app->state);
        app->state.dropped = 0;
        app->state.hitch = 0;

        if(app->rep){
            int cnt = 0;
            if(!pb_replay_read_frame(app->rep, &rep_dt, &cnt)){
                pb_event q;
                memset(&q, 0, sizeof(q));
                q.type = PB_EVENT_QUIT;
                pb_dispatch_event(app, &q, frame_start);
                pb_app_quit(app);
            }else{
                have_rep_dt = 1;
                pb_event ev;
                for(int i=0; i<cnt; i++){
                    if(!pb_replay_read_event(app->rep, &ev)) break;

                    if(ev.type == PB_EVENT_RESIZE){
                        pb_apply_resize(app, ev.as.resize.width, ev.as.resize.height);
                    }
                    pb_dispatch_event(app, &ev, frame_start);
                }
            }
        }else{
            pb_event ev;
            while(pb_input_poll(app->input, &ev)){
                if(ev.type == PB_EVENT_RESIZE){
                    pb_apply_resize(app, ev.as.resize.width, ev.as.resize.height);
                }

                if(ev.type == PB_EVENT_KEY && ev.as.key.ctrl &&
                   (ev.as.key.codepoint == 'c' || ev.as.key.codepoint == 'C')){
                    pb_event q;
                    memset(&q, 0, sizeof(q));
                    q.type = PB_EVENT_QUIT;
                    if(app->rec) pb_evbuf_push(app, &q);
                    pb_dispatch_event(app, &q, frame_start);
                    pb_app_quit(app);
                    break;
                }

                if(app->rec) pb_evbuf_push(app, &ev);
                pb_dispatch_event(app, &ev, frame_start);
            }
        }

        pb_state_expire_holds(&app->state, pb_time_ns());

        double dt = (double)(frame_start - last) / 1000000000.0;
        if(dt > 0.25) dt = 0.25;
        if(dt < 0.0) dt = 0.0;
        last = frame_start;

        if(have_rep_dt){
            dt = rep_dt;
            if(dt > 0.25) dt = 0.25;
            if(dt < 0.0) dt = 0.0;
        }

        app->state.dt = dt;
        app->state.fps_accum += dt;
        app->state.fps_frames++;
        if(app->state.fps_accum >= 0.5){
            app->state.fps = (int)((double)app->state.fps_frames / app->state.fps_accum + 0.5);
            app->state.fps_accum = 0.0;
            app->state.fps_frames = 0;
        }

        t0 = pb_time_ns();
        if(app->desc.on_update) app->desc.on_update(app, app->user, dt);
        t1 = pb_time_ns();

        if(app->rec){
            pb_replay_write_frame(app->rec, dt, app->evlen);
            for(int i=0; i<app->evlen; i++){
                pb_replay_write_event(app->rec, &app->evbuf[i]);
            }
        }

        if(!(app->desc.flags & PB_APP_FLAG_NO_AUTO_CLEAR)){
            pb_fb_clear(&app->fb, app->clear);
        }
        app->desc.on_draw(app, app->user, &app->fb);
        t2 = pb_time_ns();

        pb_renderer_present(app->renderer, &app->fb);
        t3 = pb_time_ns();

        app->state.update_ms = (double)(t1 - t0) / 1e6;
        app->state.draw_ms = (double)(t2 - t1) / 1e6;
        app->state.present_ms = (double)(t3 - t2) / 1e6;
        app->state.frame_ms = (double)(t3 - frame_start) / 1e6;
        app->state.frame_index++;

        refresh = app->refresh_hz > 0 ? app->refresh_hz : app->target_fps;
        if(refresh < 1) refresh = 60;
        frame_budget_ns = 1000000000ull / (uint64_t)refresh;
        double budget_ms = (double)frame_budget_ns / 1e6;

        if(app->state.frame_ms > budget_ms * 2.0){
            app->state.hitch = 1;
            app->state.hitch_count++;
        }

        app->state.wait_ms = 0.0;
        if(app->vsync && !have_rep_dt){
            if(t3 > next_deadline + frame_budget_ns / 2){
                /* Missed vsync — frame drop detected, resync cadence */
                app->state.dropped = 1;
                app->state.drop_count++;
                next_deadline = t3 + frame_budget_ns;
            } else {
                if(t3 < next_deadline){
                    pb_sleep_until_ns(next_deadline);
                }
                t4 = pb_time_ns();
                app->state.wait_ms = (double)(t4 - t3) / 1e6;
                next_deadline += frame_budget_ns;
                /* Prevent deadline from drifting too far ahead after light frames */
                if(next_deadline < t4) next_deadline = t4 + frame_budget_ns;
            }
        } else {
            next_deadline = t3 + frame_budget_ns;
        }
    }

    if(app->desc.on_shutdown) app->desc.on_shutdown(app, app->user);

    pb_term_leave(app->term);
    return 1;
}
