/* Draw helpers used from ASM (keeps asm free of awkward struct ABI). */
#include "playbox/pb.h"
#include <stdio.h>

void asm_draw_frame(pb_fb* fb, double x, double y, int fps){
    pb_color top = pb_rgb(8, 10, 16);
    pb_color bot = pb_rgb(24, 32, 56);
    pb_color neon = pb_rgb(80, 220, 255);
    pb_color fg = pb_rgb(220, 224, 230);
    pb_color bg = pb_rgb(10, 12, 18);

    pb_fb_fill_gradient_v(fb, 0, 0, fb->w, fb->h, top, bot);

    int cx = (int)x * 2;
    int cy = (int)y * 4;
    pb_fb_braille_fill_circle(fb, cx, cy, 14, neon);
    pb_fb_braille_circle(fb, fb->w, fb->h * 2, 22, pb_rgb(255, 120, 200));

    char buf[96];
    snprintf(buf, sizeof(buf), " ASM + Playbox  FPS %d  (ESC quit) ", fps);
    pb_fb_text(fb, 2, 0, buf, neon, bg, PB_STYLE_BOLD);
    pb_fb_text(fb, 2, fb->h - 1, "update() in game.S  |  draw helpers in C", fg, bg, 0);
}
