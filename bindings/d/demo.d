#!/usr/bin/env dub
/+ dub.sdl:
name "pb_d_demo"
dependency "playbox" path="."
libs "playbox" "m"
dflags "-fPIC"
lflags "-L../../build/lib" "-rpath=../../build/lib"
+/
import playbox;
import std.math : cos, sin;
import std.string : format;

void main() {
    auto app = new App("Playbox D", 60);
    double t = 0;

    app.onUpdate = (dt) {
        t += dt;
        if (app.isKeyPressed(PB_KEY_ESC) || app.isCharPressed('q')) app.quit();
    };

    app.onDraw = (fb) {
        auto bg = rgb(10, 12, 18);
        auto neon = rgb(80, 220, 255);
        auto mag = rgb(255, 120, 200);
        auto panel = rgb(18, 22, 32);
        auto fg = rgb(220, 224, 230);

        fb.fillGradientV(0, 0, fb.width, fb.height, rgb(8, 10, 16), rgb(24, 32, 56));
        fb.boxEx(0, 0, fb.width, fb.height, PB_BOX_ROUNDED, rgb(70, 90, 120), bg);

        float ang = cast(float)(t * 1.5);
        fb.brailleFillCircle(fb.width + cast(int)(cos(ang) * 24),
                             fb.height * 2 + cast(int)(sin(ang) * 16), 10, neon);
        fb.quadFillCircle(fb.width + cast(int)(cos(ang * 0.7) * 14),
                          fb.height + cast(int)(sin(ang * 0.7) * 10), 6, mag);

        fb.text(2, 0, format("D + Playbox  FPS %s  (ESC/Q quit)", app.fps), neon, bg, PB_STYLE_BOLD);

        int hw = fb.width < 52 ? fb.width - 4 : 48;
        int hh = fb.height < 12 ? fb.height - 4 : 8;
        int hx = (fb.width - hw) / 2;
        int hy = (fb.height - hh) / 2;
        fb.shadow(hx, hy, hw, hh, rgb(0, 0, 0), 0.4f);
        fb.panelEx(hx, hy, hw, hh, "Playbox D", PB_BOX_ROUNDED, neon, fg, panel);
        fb.textWrap(hx + 2, hy + 2, hw - 4, hh - 3,
            "Idiomatic D bindings: App class, Framebuffer, braille/quad, rounded panels.",
            fg, panel);
    };

    app.run();
}
