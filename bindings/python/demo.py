"""Tiny Python Playbox demo. Run from repo root:

  PYTHONPATH=bindings/python PLAYBOX_LIB_DIR=$PWD/build/lib \\
    python3 bindings/python/demo.py
"""

from playbox import (
    App,
    PB_BOX_ROUNDED,
    PB_EVENT_KEY,
    PB_EVENT_QUIT,
    PB_KEY_ESC,
    PB_STYLE_BOLD,
    rgb,
)


def main():
    app = App("Playbox Python", 60)

    def on_event(a, ev):
        if ev.type == PB_EVENT_QUIT:
            a.quit()
        elif ev.type == PB_EVENT_KEY and ev.as_.key.pressed:
            if ev.as_.key.key == PB_KEY_ESC:
                a.quit()

    def on_draw(a, fb):
        top, bot = rgb(8, 10, 16), rgb(24, 32, 56)
        neon = rgb(80, 220, 255)
        panel_bg = rgb(14, 18, 28)
        border = rgb(60, 140, 200)
        title_fg = rgb(200, 230, 255)
        shadow = rgb(0, 0, 0)
        text_bg = rgb(10, 12, 18)

        fb.fill_gradient_v(0, 0, fb.w, fb.h, top, bot)

        # Braille accent (pixel space = w*2 x h*4)
        cx, cy = fb.w, fb.h * 2
        fb.braille_fill_circle(cx, cy, 18, neon)
        fb.braille_circle(cx + 40, cy - 10, 10, rgb(255, 120, 180))

        # Rounded panel with drop shadow
        px, py, pw, ph = 4, 2, min(48, max(20, fb.w - 8)), min(10, max(6, fb.h - 4))
        fb.shadow(px + 1, py + 1, pw, ph, shadow, 0.5)
        fb.panel_ex(px, py, pw, ph, " Python ", PB_BOX_ROUNDED, border, title_fg, panel_bg, 0)
        fb.text(
            px + 2,
            py + 2,
            f"Playbox {a.fps()} FPS  mouse {a.mouse_x()},{a.mouse_y()}  (Esc quit)",
            neon,
            panel_bg,
            PB_STYLE_BOLD,
        )
        fb.text_clipped(px + 2, py + 4, pw - 4, "rounded panel + braille + shadow", title_fg, panel_bg)

        if a.focused():
            fb.text(2, fb.h - 1, "focused", rgb(120, 200, 140), text_bg)

    app.on_event = on_event
    app.on_draw = on_draw
    try:
        app.run()
    finally:
        app.destroy()


if __name__ == "__main__":
    main()
