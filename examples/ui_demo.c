#include "playbox/pb.h"

#include <stdio.h>
#include <string.h>

enum {
    UI_ID_BTN_PING = 1,
    UI_ID_BTN_MODAL,
    UI_ID_CHK_STATS,
    UI_ID_CHK_TOAST,
    UI_ID_SLIDER_VOL,
    UI_ID_SLIDER_SPEED
};

typedef struct {
    pb_ui_ctx ui;
    float volume;
    float speed;
    int show_stats;
    int show_toast;
    int clicks;
    int modal;
    pb_event last_ev;
    float progress;
    char toast[64];
    float toast_t;
} ui_state;

static void on_event(pb_app* app, void* user, const pb_event* ev){
    ui_state* s = (ui_state*)user;
    s->last_ev = *ev;
    if(ev->type == PB_EVENT_QUIT){ pb_app_quit(app); return; }
    if(ev->type == PB_EVENT_KEY && ev->as.key.pressed){
        uint32_t cp = ev->as.key.codepoint;
        pb_key k = ev->as.key.key;
        if(!s->modal && (k == PB_KEY_ESC || cp == 'q' || cp == 'Q'))
            pb_app_quit(app);
    }
}

static void on_update(pb_app* app, void* user, double dt){
    ui_state* s = (ui_state*)user;
    (void)app;
    if(dt < 0) dt = 0;
    if(dt > 0.25) dt = 0.25;
    s->progress += (float)dt * s->speed * 0.25f;
    if(s->progress > 1.f) s->progress -= 1.f;
    if(s->toast_t > 0.f) s->toast_t -= (float)dt;
}

static void on_draw(pb_app* app, void* user, pb_fb* fb){
    ui_state* s = (ui_state*)user;

    pb_fb_fill_gradient_v(fb, 0, 0, fb->w, fb->h, pb_rgb(8, 10, 16), pb_rgb(18, 24, 40));

    pb_ui_begin(&s->ui, fb, app);

    if(pb_ui_begin_panel(&s->ui, 2, 1, 34, 18, "Controls")){
        pb_ui_label(&s->ui, "Immediate-mode widgets");
        pb_ui_separator(&s->ui);
        pb_ui_spacer(&s->ui, 1);

        if(pb_ui_button(&s->ui, UI_ID_BTN_PING, "Ping", 12)){
            s->clicks++;
            snprintf(s->toast, sizeof s->toast, "Ping #%d", s->clicks);
            s->toast_t = 2.0f;
            s->show_toast = 1;
        }

        if(pb_ui_button(&s->ui, UI_ID_BTN_MODAL, "Open modal", 14))
            s->modal = 1;

        pb_ui_spacer(&s->ui, 1);
        pb_ui_checkbox(&s->ui, UI_ID_CHK_STATS, "Show frame stats", &s->show_stats);
        pb_ui_checkbox(&s->ui, UI_ID_CHK_TOAST, "Enable toasts", &s->show_toast);

        pb_ui_spacer(&s->ui, 1);
        pb_ui_label(&s->ui, "Volume");
        pb_ui_slider(&s->ui, UI_ID_SLIDER_VOL, &s->volume, 0.f, 1.f, 28);
        char line[48];
        snprintf(line, sizeof line, "  %.0f%%", s->volume * 100.f);
        pb_ui_label(&s->ui, line);

        pb_ui_label(&s->ui, "Speed");
        pb_ui_slider(&s->ui, UI_ID_SLIDER_SPEED, &s->speed, 0.2f, 3.f, 28);
        snprintf(line, sizeof line, "  %.2fx", s->speed);
        pb_ui_label(&s->ui, line);

        pb_ui_spacer(&s->ui, 1);
        pb_ui_label(&s->ui, "Progress");
        pb_ui_progress(&s->ui, s->progress, 28, NULL);

        pb_ui_end_panel(&s->ui);
    }

    if(pb_ui_begin_panel(&s->ui, 38, 1, 28, 12, "Status")){
        char line[64];
        snprintf(line, sizeof line, "clicks: %d", s->clicks);
        pb_ui_label(&s->ui, line);
        snprintf(line, sizeof line, "mouse: %d,%d", s->ui.mouse_x, s->ui.mouse_y);
        pb_ui_label(&s->ui, line);
        pb_ui_separator(&s->ui);
        pb_ui_label(&s->ui, "Q quit  click widgets");
        pb_ui_label(&s->ui, "with the mouse.");
        pb_ui_end_panel(&s->ui);
    }

    if(s->show_stats)
        pb_ui_draw_frame_stats(fb, app, fb->w - 38, fb->h - 11);

    if(s->show_toast && s->toast_t > 0.f && s->toast[0])
        pb_toast_draw(fb, fb->h - 2, s->toast, pb_rgb(120, 255, 200), pb_rgb(20, 28, 40), PB_STYLE_BOLD);

    if(s->modal){
        pb_popup_desc pop;
        pb_popup_desc_init(&pop);
        pop.title = "Modal";
        pop.body = "Confirm or cancel.\nEnter / Space confirm, Esc cancel.";
        pop.hint = "Enter confirm · Esc cancel";
        pb_popup_result r = pb_ui_modal(&s->ui, &pop, &s->last_ev);
        if(r == PB_POPUP_RESULT_CONFIRM){
            s->modal = 0;
            snprintf(s->toast, sizeof s->toast, "Confirmed");
            s->toast_t = 1.5f;
        } else if(r == PB_POPUP_RESULT_CANCEL){
            s->modal = 0;
            snprintf(s->toast, sizeof s->toast, "Cancelled");
            s->toast_t = 1.5f;
        }
        memset(&s->last_ev, 0, sizeof(s->last_ev));
    }

    pb_ui_end(&s->ui);
}

int main(void){
    ui_state s;
    memset(&s, 0, sizeof(s));
    s.volume = 0.65f;
    s.speed = 1.0f;
    s.show_toast = 1;

    pb_app_desc d;
    memset(&d, 0, sizeof(d));
    d.title = "PlayboxLib UI demo";
    d.target_fps = 60;
    d.flags = PB_APP_FLAG_CUSTOM_CLEAR;
    d.clear = pb_cell_make(' ', pb_rgb(220,224,230), pb_rgb(10,12,20), 0);
    d.on_event = on_event;
    d.on_update = on_update;
    d.on_draw = on_draw;

    pb_app* app = pb_app_create(&d, &s);
    if(!app) return 1;
    int ok = pb_app_run(app);
    pb_app_destroy(app);
    return ok ? 0 : 1;
}
