# How to implement a PlayboxLib game

PlayboxLib is a small terminal game framework with a raylib-like loop:
`event → update → draw`.

## Minimal app

```c
#include "playbox/pb.h"

typedef struct { double x, y; } game_state;

static void on_update(pb_app* app, void* user, double dt){
    game_state* g = user;
    if(pb_is_key_down(app, PB_KEY_RIGHT)) g->x += 20.0 * dt;
    if(pb_is_key_down(app, PB_KEY_LEFT))  g->x -= 20.0 * dt;
}

static void on_draw(pb_app* app, void* user, pb_fb* fb){
    (void)app;
    game_state* g = user;
    pb_color fg = pb_rgb(220,220,220);
    pb_color bg = pb_rgb(12,14,18);
    pb_fb_fill_rect(fb, 0, 0, fb->w, fb->h, pb_cell_make(' ', fg, bg, 0));
    pb_fb_put(fb, (int)g->x, (int)g->y, pb_cell_make('@', pb_rgb(255,120,200), bg, PB_STYLE_BOLD));
    pb_fb_textf(fb, 1, 0, fg, bg, 0, "FPS %d", pb_get_fps(app));
}

int main(void){
    game_state g = { .x = 10, .y = 5 };
    pb_app_desc d = {0};
    d.title = "My Game";
    d.target_fps = 60;
    d.on_update = on_update;
    d.on_draw = on_draw;

    pb_app* app = pb_app_create(&d, &g);
    if(!app) return 1;
    pb_app_run(app);
    pb_app_destroy(app);
    return 0;
}
```

## Drawing

Cell framebuffer primitives:

| Function | Purpose |
|----------|---------|
| `pb_fb_put` / `get` | Single cell |
| `pb_fb_text` / `textf` / `text_centered` | UTF-8 text |
| `pb_fb_fill_rect` / `hline` / `vline` | Fills |
| `pb_fb_box` / `box_double` / `panel` | Frames |
| `pb_fb_line` / `circle` / `fill_circle` | Shapes |
| `pb_fb_blit` / `blit_masked` | Sprites / atlases |
| `pb_fb_plot*` | Half-block pixels (2× vertical resolution) |

## Input

You can still handle `on_event`, or poll like raylib:

* `pb_is_key_down` / `pressed` / `released`
* `pb_is_char_down` / `pressed`
* `pb_get_mouse_x` / `y`, `pb_is_mouse_button_down`
* `pb_get_mouse_wheel`, `pb_is_focused`
* `pb_get_fps`, `pb_get_frame_time`

Printable keys also arrive as `PB_EVENT_TEXT` (mirrored from `PB_EVENT_KEY`).

## Tips

* Draw everything each frame into the framebuffer; the renderer diffs and only writes changed cells.
* Seed RNG from `pb_app_replay_seed(app)` when recording/replaying.
* Use `pb_app_set_clear` / `PB_APP_FLAG_CUSTOM_CLEAR` for a default background.
* Link with `-lplaybox -lm` on Linux.
