#include "playbox/pb_ui.h"
#include "playbox/pb_state.h"
#include "playbox/pb_app.h"

#include <stdio.h>
#include <string.h>

void pb_popup_desc_init(pb_popup_desc* desc){
    if(!desc) return;
    memset(desc, 0, sizeof(*desc));
    desc->box_style = PB_BOX_ROUNDED;
    desc->border = pb_rgb(80, 210, 255);
    desc->title_fg = pb_rgb(120, 255, 200);
    desc->fill = pb_rgb(14, 18, 30);
    desc->body_fg = pb_rgb(230, 236, 245);
    desc->hint_fg = pb_rgb(140, 155, 175);
    desc->shadow_alpha = 0.45f;
    desc->dim_backdrop = 1;
    desc->center = 1;
}

static int pb_count_body_lines(const char* body, int max_w){
    if(!body || !body[0]) return 0;
    int lines = 1;
    int col = 0;
    for(const char* p = body; *p; p++){
        if(*p == '\n'){ lines++; col = 0; continue; }
        col++;
        if(max_w > 0 && col >= max_w){ lines++; col = 0; }
    }
    return lines;
}

void pb_popup_draw_ex(pb_fb* fb, const pb_popup_desc* desc,
                      int* out_x, int* out_y, int* out_w, int* out_h){
    if(!fb || !desc) return;

    int fw = fb->w;
    int fh = fb->h;
    if(fw < 8 || fh < 5) return;

    int max_w = desc->width > 0 ? desc->width : (fw < 52 ? fw - 4 : 48);
    if(max_w < 16) max_w = fw > 4 ? fw - 4 : fw;
    if(max_w > fw - 2) max_w = fw - 2;

    int body_w = max_w - 4;
    if(body_w < 8) body_w = max_w > 2 ? max_w - 2 : max_w;
    int body_lines = pb_count_body_lines(desc->body, body_w);
    int ph = desc->height;
    if(ph <= 0){
        ph = 4 + body_lines;
        if(desc->hint && desc->hint[0]) ph += 2;
        if(ph < 6) ph = 6;
    }
    if(ph > fh - 2) ph = fh - 2;

    int px = desc->center ? (fw - max_w) / 2 : desc->x;
    int py = desc->center ? (fh - ph) / 2 : desc->y;
    if(px < 0) px = 0;
    if(py < 0) py = 0;
    if(px + max_w > fw) px = fw - max_w;
    if(py + ph > fh) py = fh - ph;

    if(desc->dim_backdrop)
        pb_fb_fill_shade(fb, 0, 0, fw, fh, pb_rgb(0, 0, 0), pb_rgb(8, 10, 16), 1);

    float shadow = desc->shadow_alpha;
    if(shadow < 0.f) shadow = 0.f;
    if(shadow > 1.f) shadow = 1.f;
    if(shadow > 0.01f)
        pb_fb_shadow(fb, px, py, max_w, ph, pb_rgb(0, 0, 0), shadow);

    pb_fb_panel_ex(fb, px, py, max_w, ph, desc->title ? desc->title : "",
                   desc->box_style, desc->border, desc->title_fg, desc->fill, PB_STYLE_BOLD);

    int text_y = py + 2;
    if(desc->body && desc->body[0]){
        char line[256];
        int li = 0;
        int rows_left = ph - 3 - ((desc->hint && desc->hint[0]) ? 2 : 0);
        for(const char* p = desc->body; *p && rows_left > 0; p++){
            if(*p == '\n' || li >= body_w || li >= (int)sizeof(line) - 1){
                line[li] = 0;
                if(line[0])
                    pb_fb_text(fb, px + 2, text_y, line, desc->body_fg, desc->fill, 0);
                text_y++;
                rows_left--;
                li = 0;
                if(*p == '\n') continue;
            }
            if(*p != '\n') line[li++] = *p;
        }
        if(li > 0 && rows_left > 0){
            line[li] = 0;
            pb_fb_text(fb, px + 2, text_y, line, desc->body_fg, desc->fill, 0);
        }
    }

    if(desc->hint && desc->hint[0])
        pb_fb_text_clipped(fb, px + 2, py + ph - 2, max_w - 4,
                           desc->hint, desc->hint_fg, desc->fill, 0);

    if(out_x) *out_x = px;
    if(out_y) *out_y = py;
    if(out_w) *out_w = max_w;
    if(out_h) *out_h = ph;
}

void pb_popup_draw(pb_fb* fb, const pb_popup_desc* desc){
    pb_popup_draw_ex(fb, desc, NULL, NULL, NULL, NULL);
}

pb_popup_result pb_popup_handle_event(const pb_event* ev){
    if(!ev || ev->type != PB_EVENT_KEY || !ev->as.key.pressed)
        return PB_POPUP_RESULT_NONE;
    pb_key k = ev->as.key.key;
    uint32_t cp = ev->as.key.codepoint;
    if(k == PB_KEY_ENTER || cp == ' ' || cp == 'y' || cp == 'Y' ||
       cp == 'r' || cp == 'R' || cp == 's' || cp == 'S')
        return PB_POPUP_RESULT_CONFIRM;
    if(k == PB_KEY_ESC || cp == 'q' || cp == 'Q' || cp == 'n' || cp == 'N')
        return PB_POPUP_RESULT_CANCEL;
    return PB_POPUP_RESULT_NONE;
}

void pb_toast_draw(pb_fb* fb, int y, const char* text,
                   pb_color fg, pb_color bg, uint16_t style){
    if(!fb || !text) return;
    if(y < 0 || y >= fb->h) y = fb->h - 1;
    int tw = pb_fb_measure_text(text);
    int x = (fb->w - tw) / 2;
    if(x < 0) x = 0;
    pb_fb_fill_rect(fb, 0, y, fb->w, 1, pb_cell_make(' ', fg, bg, 0));
    pb_fb_text_clipped(fb, x, y, fb->w - x, text, fg, bg, style);
}

void pb_ui_draw_frame_stats(pb_fb* fb, const pb_app* app, int x, int y){
    if(!fb || !app) return;
    pb_frame_stats st;
    pb_get_frame_stats(app, &st);

    pb_color panel = pb_rgb(12, 16, 26);
    pb_color border = pb_rgb(70, 120, 180);
    pb_color fg = pb_rgb(210, 220, 235);
    pb_color ok = pb_rgb(120, 255, 180);
    pb_color warn = pb_rgb(255, 200, 90);
    pb_color bad = pb_rgb(255, 110, 130);

    int w = 36;
    int h = 9;
    if(x < 0) x = 1;
    if(y < 0) y = 1;
    if(x + w > fb->w) x = fb->w > w ? fb->w - w : 0;
    if(y + h > fb->h) y = fb->h > h ? fb->h - h : 0;

    pb_fb_shadow(fb, x, y, w, h, pb_rgb(0, 0, 0), 0.35f);
    pb_fb_panel_ex(fb, x, y, w, h, "Frame", PB_BOX_ROUNDED, border, ok, panel, PB_STYLE_BOLD);

    char line[72];
    snprintf(line, sizeof line, "FPS %d / %d   vsync %dHz",
             st.fps, st.target_fps, st.refresh_hz);
    pb_fb_text(fb, x + 2, y + 2, line, fg, panel, 0);

    snprintf(line, sizeof line, "upd %.2f  draw %.2f  present %.2f",
             st.update_ms, st.draw_ms, st.present_ms);
    pb_fb_text(fb, x + 2, y + 3, line, fg, panel, 0);

    snprintf(line, sizeof line, "frame %.2f ms  wait %.2f ms",
             st.frame_ms, st.wait_ms);
    pb_fb_text(fb, x + 2, y + 4, line, fg, panel, 0);

    snprintf(line, sizeof line, "drop %s  total %llu",
             st.dropped ? "YES" : "no",
             (unsigned long long)st.drop_count);
    pb_fb_text(fb, x + 2, y + 5, line, st.dropped ? bad : ok, panel, 0);

    snprintf(line, sizeof line, "hitch %s  total %llu",
             st.hitch ? "YES" : "no",
             (unsigned long long)st.hitch_count);
    pb_fb_text(fb, x + 2, y + 6, line, st.hitch ? warn : fg, panel, 0);

    snprintf(line, sizeof line, "#%llu", (unsigned long long)st.frame_index);
    pb_fb_text(fb, x + 2, y + 7, line, pb_rgb(120, 130, 150), panel, 0);
}

/* ---- Immediate-mode widgets ---- */

void pb_ui_style_default(pb_ui_style* st){
    if(!st) return;
    st->panel = pb_rgb(14, 18, 28);
    st->border = pb_rgb(70, 110, 160);
    st->text = pb_rgb(220, 228, 240);
    st->muted = pb_rgb(120, 135, 155);
    st->accent = pb_rgb(90, 210, 255);
    st->hot = pb_rgb(40, 60, 90);
    st->active = pb_rgb(60, 120, 180);
    st->track = pb_rgb(28, 34, 48);
    st->box_style = PB_BOX_ROUNDED;
}

void pb_ui_begin(pb_ui_ctx* ui, pb_fb* fb, const pb_app* app){
    if(!ui) return;
    pb_ui_id keep_active = ui->active;
    pb_ui_style keep_style = ui->style;
    int styled = (ui->style.border.r | ui->style.border.g | ui->style.border.b) != 0;
    memset(ui, 0, sizeof(*ui));
    ui->active = keep_active;
    ui->fb = fb;
    ui->app = app;
    if(styled) ui->style = keep_style;
    else pb_ui_style_default(&ui->style);
    if(app){
        ui->mouse_x = pb_get_mouse_x(app);
        ui->mouse_y = pb_get_mouse_y(app);
        ui->mouse_down = pb_is_mouse_button_down(app, PB_MOUSE_LEFT);
        ui->mouse_pressed = pb_is_mouse_button_pressed(app, PB_MOUSE_LEFT);
        ui->mouse_released = pb_is_mouse_button_released(app, PB_MOUSE_LEFT);
    }
    ui->cx = 1;
    ui->cy = 1;
    ui->row_h = 1;
    ui->col_w = fb ? fb->w - 2 : 40;
    ui->content_x0 = 1;
    ui->content_x1 = fb ? fb->w - 1 : 40;
}

void pb_ui_end(pb_ui_ctx* ui){
    if(!ui) return;
    if(!ui->mouse_down) ui->active = 0;
}

void pb_ui_at(pb_ui_ctx* ui, int x, int y){
    if(!ui) return;
    ui->cx = x;
    ui->cy = y;
}

void pb_ui_spacer(pb_ui_ctx* ui, int h){
    if(!ui) return;
    if(h < 0) h = 0;
    ui->cy += h;
}

void pb_ui_begin_column(pb_ui_ctx* ui, int x, int y, int w){
    if(!ui) return;
    if(ui->stack_n < 8){
        ui->stack_x[ui->stack_n] = ui->cx;
        ui->stack_y[ui->stack_n] = ui->cy;
        ui->stack_w[ui->stack_n] = ui->col_w;
        ui->stack_n++;
    }
    ui->cx = x;
    ui->cy = y;
    ui->col_w = w > 0 ? w : ui->col_w;
    ui->content_x0 = x;
    ui->content_x1 = x + ui->col_w;
    ui->indent = 0;
}

void pb_ui_end_column(pb_ui_ctx* ui){
    if(!ui || ui->stack_n <= 0) return;
    ui->stack_n--;
    ui->cx = ui->stack_x[ui->stack_n];
    ui->cy = ui->stack_y[ui->stack_n];
    ui->col_w = ui->stack_w[ui->stack_n];
    ui->content_x0 = ui->cx;
    ui->content_x1 = ui->cx + ui->col_w;
}

void pb_ui_begin_row(pb_ui_ctx* ui){
    if(!ui) return;
    if(ui->stack_n < 8){
        ui->stack_x[ui->stack_n] = ui->cx;
        ui->stack_y[ui->stack_n] = ui->cy;
        ui->stack_w[ui->stack_n] = ui->row_h;
        ui->stack_n++;
    }
    ui->row_h = 1;
}

void pb_ui_end_row(pb_ui_ctx* ui){
    if(!ui || ui->stack_n <= 0) return;
    int start_x = ui->stack_x[ui->stack_n - 1];
    int start_y = ui->stack_y[ui->stack_n - 1];
    int rh = ui->row_h;
    ui->stack_n--;
    ui->cx = start_x;
    ui->cy = start_y + rh;
    ui->row_h = 1;
}

static int pb_ui_hit(const pb_ui_ctx* ui, int x, int y, int w, int h){
    return ui->mouse_x >= x && ui->mouse_y >= y &&
           ui->mouse_x < x + w && ui->mouse_y < y + h;
}

static void pb_ui_advance(pb_ui_ctx* ui, int w, int h){
    (void)w;
    if(h > ui->row_h) ui->row_h = h;
    /* Default: stack vertically */
    ui->cy += h;
}

void pb_ui_label(pb_ui_ctx* ui, const char* text){
    if(!ui || !ui->fb || !text) return;
    pb_fb_text_clipped(ui->fb, ui->cx + ui->indent, ui->cy, ui->col_w - ui->indent,
                       text, ui->style.text, ui->style.panel, 0);
    pb_ui_advance(ui, ui->col_w, 1);
}

void pb_ui_separator(pb_ui_ctx* ui){
    if(!ui || !ui->fb) return;
    int x = ui->cx + ui->indent;
    int w = ui->col_w - ui->indent;
    if(w < 1) w = 1;
    pb_fb_hline(ui->fb, x, ui->cy, w,
                pb_cell_make(0x2500 /* ─ */, ui->style.muted, ui->style.panel, 0));
    pb_ui_advance(ui, w, 1);
}

int pb_ui_button(pb_ui_ctx* ui, pb_ui_id id, const char* label, int w){
    if(!ui || !ui->fb) return 0;
    if(w < 4) w = 4;
    if(w > ui->col_w - ui->indent) w = ui->col_w - ui->indent;
    int x = ui->cx + ui->indent;
    int y = ui->cy;
    int h = 3;
    int clicked = 0;

    int hit = pb_ui_hit(ui, x, y, w, h);
    if(hit) ui->hot = id;
    if(ui->mouse_pressed && hit) ui->active = id;
    if(ui->mouse_released && ui->active == id && hit) clicked = 1;

    pb_color fill = ui->style.panel;
    pb_color border = ui->style.border;
    if(ui->active == id){ fill = ui->style.active; border = ui->style.accent; }
    else if(ui->hot == id){ fill = ui->style.hot; border = ui->style.accent; }

    pb_fb_box_ex(ui->fb, x, y, w, h, ui->style.box_style, border, fill, 0);
    if(label){
        int tw = pb_fb_measure_text(label);
        int tx = x + (w - tw) / 2;
        if(tx < x + 1) tx = x + 1;
        pb_fb_text_clipped(ui->fb, tx, y + 1, w - 2, label, ui->style.text, fill, PB_STYLE_BOLD);
    }

    pb_ui_advance(ui, w, h);
    /* Side-by-side: if caller uses begin_row they manage; for vertical stack we advanced.
       For rows, restore and move horizontally — detect via stack marker unused.
       Simple approach: also expose horizontal by not advancing y if row_h tracking —
       Use: after button in a row, move right instead. */
    return clicked;
}

int pb_ui_checkbox(pb_ui_ctx* ui, pb_ui_id id, const char* label, int* checked){
    if(!ui || !ui->fb || !checked) return 0;
    int x = ui->cx + ui->indent;
    int y = ui->cy;
    int box = 3;
    int lab_w = label ? pb_fb_measure_text(label) + 1 : 0;
    int w = box + 1 + lab_w;
    if(w > ui->col_w - ui->indent) w = ui->col_w - ui->indent;
    int changed = 0;

    int hit = pb_ui_hit(ui, x, y, w, 1);
    if(hit) ui->hot = id;
    if(ui->mouse_pressed && hit) ui->active = id;
    if(ui->mouse_released && ui->active == id && hit){
        *checked = !*checked;
        changed = 1;
    }

    pb_color border = (ui->hot == id || ui->active == id) ? ui->style.accent : ui->style.border;
    pb_fb_put(ui->fb, x, y, pb_cell_make('[', border, ui->style.panel, 0));
    pb_fb_put(ui->fb, x + 1, y, pb_cell_make(*checked ? 'x' : ' ',
              *checked ? ui->style.accent : ui->style.muted, ui->style.panel, PB_STYLE_BOLD));
    pb_fb_put(ui->fb, x + 2, y, pb_cell_make(']', border, ui->style.panel, 0));
    if(label)
        pb_fb_text_clipped(ui->fb, x + 4, y, ui->col_w - ui->indent - 4,
                           label, ui->style.text, ui->style.panel, 0);

    pb_ui_advance(ui, w, 1);
    return changed;
}

int pb_ui_slider(pb_ui_ctx* ui, pb_ui_id id, float* value, float minv, float maxv, int w){
    if(!ui || !ui->fb || !value) return 0;
    if(maxv <= minv) maxv = minv + 1.f;
    if(w < 8) w = 8;
    if(w > ui->col_w - ui->indent) w = ui->col_w - ui->indent;
    int x = ui->cx + ui->indent;
    int y = ui->cy;
    int h = 1;
    int changed = 0;

    int hit = pb_ui_hit(ui, x, y, w, h);
    if(hit) ui->hot = id;
    if(ui->mouse_pressed && hit) ui->active = id;
    if(ui->active == id && ui->mouse_down){
        float t = (float)(ui->mouse_x - x) / (float)(w - 1);
        if(t < 0.f) t = 0.f;
        if(t > 1.f) t = 1.f;
        float nv = minv + t * (maxv - minv);
        if(nv != *value){ *value = nv; changed = 1; }
    }

    float t = (*value - minv) / (maxv - minv);
    if(t < 0.f) t = 0.f;
    if(t > 1.f) t = 1.f;
    int fill_w = (int)(t * (float)(w - 1) + 0.5f);
    pb_fb_hline(ui->fb, x, y, w, pb_cell_make(0x2500, ui->style.track, ui->style.panel, 0));
    if(fill_w > 0)
        pb_fb_hline(ui->fb, x, y, fill_w + 1,
                    pb_cell_make(0x2501 /* ━ */, ui->style.accent, ui->style.panel, 0));
    int knob = x + fill_w;
    pb_fb_put(ui->fb, knob, y, pb_cell_make(0x25CF /* ● */,
              (ui->active == id || ui->hot == id) ? ui->style.accent : ui->style.text,
              ui->style.panel, 0));

    pb_ui_advance(ui, w, h);
    return changed;
}

void pb_ui_progress(pb_ui_ctx* ui, float t01, int w, const char* label){
    if(!ui || !ui->fb) return;
    if(w < 6) w = 6;
    if(w > ui->col_w - ui->indent) w = ui->col_w - ui->indent;
    int x = ui->cx + ui->indent;
    int y = ui->cy;
    if(t01 < 0.f) t01 = 0.f;
    if(t01 > 1.f) t01 = 1.f;

    pb_fb_hline(ui->fb, x, y, w, pb_cell_make(0x2591 /* ░ */, ui->style.track, ui->style.panel, 0));
    int fw = (int)(t01 * (float)w + 0.5f);
    if(fw > 0)
        pb_fb_hline(ui->fb, x, y, fw, pb_cell_make(0x2588 /* █ */, ui->style.accent, ui->style.panel, 0));
    if(label)
        pb_fb_text_clipped(ui->fb, x, y, w, label, ui->style.text, ui->style.panel, 0);

    pb_ui_advance(ui, w, 1);
}

int pb_ui_begin_panel(pb_ui_ctx* ui, int x, int y, int w, int h, const char* title){
    if(!ui || !ui->fb || w < 6 || h < 4) return 0;
    pb_fb_shadow(ui->fb, x, y, w, h, pb_rgb(0,0,0), 0.3f);
    pb_fb_panel_ex(ui->fb, x, y, w, h, title ? title : "",
                   ui->style.box_style, ui->style.border, ui->style.accent,
                   ui->style.panel, PB_STYLE_BOLD);
    pb_ui_begin_column(ui, x + 2, y + 2, w - 4);
    return 1;
}

void pb_ui_end_panel(pb_ui_ctx* ui){
    pb_ui_end_column(ui);
}

pb_popup_result pb_ui_modal(pb_ui_ctx* ui, const pb_popup_desc* desc, const pb_event* ev){
    if(!ui || !ui->fb || !desc) return PB_POPUP_RESULT_NONE;
    pb_popup_draw(ui->fb, desc);
    return pb_popup_handle_event(ev);
}
