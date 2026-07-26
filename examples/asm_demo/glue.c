/* Playbox ASM demo — Pattern A: C trampoline + asm update.
 * Build:
 *   tools/build.sh build
 *   cc -O2 -Iinclude -c examples/asm_demo/glue.c -o build/obj/asm_glue.o
 *   cc -O2 -Iinclude -c examples/asm_demo/draw_helpers.c -o build/obj/asm_draw.o
 *   as --64 examples/asm_demo/game.S -o build/obj/asm_game.o
 *   cc build/obj/asm_glue.o build/obj/asm_draw.o build/obj/asm_game.o \
 *      -o build/bin/pb_asm_demo -Lbuild/lib -lplaybox -lm -Wl,-rpath,'$ORIGIN/../lib'
 */

#include "playbox/pb.h"
#include <string.h>

typedef struct {
    double t;
    double x, y;
    double vx, vy;
} asm_state;

void asm_on_update(asm_state* s, double dt);
void asm_draw_frame(pb_fb* fb, double x, double y, int fps);

static void on_update(pb_app* app, void* user, double dt){
    asm_state* s = (asm_state*)user;
    asm_on_update(s, dt);

    int w = pb_app_width(app);
    int h = pb_app_height(app);
    if(s->x < 2){ s->x = 2; s->vx = (s->vx < 0) ? -s->vx : s->vx; }
    if(s->y < 2){ s->y = 2; s->vy = (s->vy < 0) ? -s->vy : s->vy; }
    if(s->x > w - 3){ s->x = w - 3; s->vx = (s->vx > 0) ? -s->vx : s->vx; }
    if(s->y > h - 3){ s->y = h - 3; s->vy = (s->vy > 0) ? -s->vy : s->vy; }
}

static void on_draw(pb_app* app, void* user, pb_fb* fb){
    asm_state* s = (asm_state*)user;
    asm_draw_frame(fb, s->x, s->y, pb_get_fps(app));
}

static void on_event(pb_app* app, void* user, const pb_event* ev){
    (void)user;
    if(ev->type == PB_EVENT_KEY && ev->as.key.pressed){
        if(ev->as.key.key == PB_KEY_ESC || ev->as.key.codepoint == 'q'){
            pb_app_quit(app);
        }
    }
}

int main(void){
    asm_state s;
    memset(&s, 0, sizeof(s));
    s.x = 20;
    s.y = 10;
    s.vx = 18;
    s.vy = 11;

    pb_app_desc d;
    memset(&d, 0, sizeof(d));
    d.title = "Playbox ASM";
    d.target_fps = 60;
    d.on_update = on_update;
    d.on_draw = on_draw;
    d.on_event = on_event;

    pb_app* app = pb_app_create(&d, &s);
    if(!app) return 1;
    int ok = pb_app_run(app);
    pb_app_destroy(app);
    return ok ? 0 : 1;
}
