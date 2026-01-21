#include "playbox/pb_app.h"
#include "playbox/pb_term.h"
#include "playbox/pb_input.h"
#include "playbox/pb_renderer.h"
#include "playbox/pb_time.h"
#include "playbox/pb_replay.h"
#include <stdlib.h>
#include <string.h>

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

    app->clear = pb_cell_make(' ', pb_rgb(220,220,220), pb_rgb(0,0,0), 0);
    pb_renderer_set_clear(app->renderer, app->clear);

    if(desc->title){
        strncpy(app->title, desc->title, sizeof(app->title)-1);
        app->title[sizeof(app->title)-1] = 0;
    }else{
        strcpy(app->title, "PlayboxLib");
    }

    app->rec = NULL;
    app->rep = NULL;
    app->evbuf = NULL;
    app->evlen = 0;
    app->evcap = 0;

    app->meta_seed = 0;
    app->meta_w = 0;
    app->meta_h = 0;
    app->flag_replay = 0;
    app->flag_record = 0;

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

    pb_term_enable_mouse(app->term, 1);

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

    int target_fps = app->desc.target_fps > 0 ? app->desc.target_fps : 60;
    double target_dt = 1.0 / (double)target_fps;

    while(app->running){
        double rep_dt = 0.0;
        int have_rep_dt = 0;

        pb_evbuf_clear(app);

        if(app->rep){
            int cnt = 0;
            if(!pb_replay_read_frame(app->rep, &rep_dt, &cnt)){
                pb_app_quit(app);
            }else{
                have_rep_dt = 1;
                pb_event ev;
                for(int i=0; i<cnt; i++){
                    if(!pb_replay_read_event(app->rep, &ev)) break;

                    if(ev.type == PB_EVENT_RESIZE){
                        pb_apply_resize(app, ev.as.resize.width, ev.as.resize.height);
                    }
                    if(app->desc.on_event) app->desc.on_event(app, app->user, &ev);
                }
            }
        }else{
            pb_event ev;
            while(pb_input_poll(app->input, &ev)){
                if(ev.type == PB_EVENT_RESIZE){
                    pb_apply_resize(app, ev.as.resize.width, ev.as.resize.height);
                }
                if(app->desc.on_event) app->desc.on_event(app, app->user, &ev);

                if(app->rec){
                    pb_evbuf_push(app, &ev);
                }

                if(ev.type == PB_EVENT_KEY && ev.as.key.ctrl &&
                    (ev.as.key.codepoint == 'c' || ev.as.key.codepoint == 'C')){
                    pb_app_quit(app);
                    }
            }
        }

        uint64_t now = pb_time_ns();
        double dt = (double)(now - last) / 1000000000.0;
        if(dt > 0.25) dt = 0.25;
        last = now;

        if(have_rep_dt){
            dt = rep_dt;
            if(dt > 0.25) dt = 0.25;
            if(dt < 0.0) dt = 0.0;
        }

        if(app->desc.on_update) app->desc.on_update(app, app->user, dt);

        if(app->rec){
            pb_replay_write_frame(app->rec, dt, app->evlen);
            for(int i=0; i<app->evlen; i++){
                pb_replay_write_event(app->rec, &app->evbuf[i]);
            }
        }

        pb_fb_clear(&app->fb, app->clear);
        app->desc.on_draw(app, app->user, &app->fb);
        pb_renderer_present(app->renderer, &app->fb);

        uint64_t end = pb_time_ns();
        double frame = (double)(end - now) / 1000000000.0;
        double sleep_s = target_dt - frame;
        if(sleep_s > 0.0){
            int ms = (int)(sleep_s * 1000.0);
            if(ms > 0) pb_sleep_ms(ms);
        }
    }

    if(app->desc.on_shutdown) app->desc.on_shutdown(app, app->user);

    pb_term_leave(app->term);
    return 1;
}
