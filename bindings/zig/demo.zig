//! Zig Playbox demo — idiomatic App wrapper.
//!   zig build -C bindings/zig
//!   ./build/bin/pb_zig_demo

const std = @import("std");
const playbox = @import("playbox");

const State = struct {
    t: f64 = 0,
};

const MyApp = playbox.App(State);

fn onUpdate(app: *MyApp, dt: f64) void {
    app.user.t += dt;
    if (app.isKeyPressed(playbox.KEY_ESC) or app.isCharPressed('q')) {
        app.quit();
    }
}

fn onDraw(app: *MyApp, fb: playbox.Framebuffer) void {
    const bg = playbox.rgb(10, 12, 18);
    const neon = playbox.rgb(80, 220, 255);
    const mag = playbox.rgb(255, 120, 200);
    const panel = playbox.rgb(18, 22, 32);
    const fg = playbox.rgb(220, 224, 230);

    fb.fillGradientV(0, 0, fb.width(), fb.height(), playbox.rgb(8, 10, 16), playbox.rgb(24, 32, 56));
    fb.boxEx(0, 0, fb.width(), fb.height(), playbox.BOX_ROUNDED, playbox.rgb(70, 90, 120), bg, 0);

    const ang: f32 = @floatCast(app.user.t * 1.5);
    const cx = fb.width();
    const cy = fb.height() * 2;
    fb.brailleFillCircle(cx + @as(c_int, @intFromFloat(@cos(ang) * 24)), cy + @as(c_int, @intFromFloat(@sin(ang) * 16)), 10, neon);
    fb.quadFillCircle(fb.width() + @as(c_int, @intFromFloat(@cos(ang * 0.7) * 14)), fb.height() + @as(c_int, @intFromFloat(@sin(ang * 0.7) * 10)), 6, mag);

    var buf: [64]u8 = undefined;
    const msg = std.fmt.bufPrintZ(&buf, "Zig + Playbox  FPS {d}  (ESC/Q quit)", .{app.fps()}) catch "Zig + Playbox";
    fb.text(2, 0, msg.ptr, neon, bg, playbox.STYLE_BOLD);

    const hw = @min(fb.width() - 4, 48);
    const hh = @min(fb.height() - 4, 8);
    const hx = @divTrunc(fb.width() - hw, 2);
    const hy = @divTrunc(fb.height() - hh, 2);
    fb.shadow(hx, hy, hw, hh, playbox.rgb(0, 0, 0), 0.4);
    fb.panelEx(hx, hy, hw, hh, "Playbox Zig", playbox.BOX_ROUNDED, neon, fg, panel, 0);
    _ = fb.textWrap(hx + 2, hy + 2, hw - 4, hh - 3,
        "Idiomatic Zig bindings: App(T), Framebuffer, braille/quad, rounded panels.",
        fg, panel, 0);
}

pub fn main() !void {
    var app = try MyApp.init("Playbox Zig", 60, .{});
    defer app.deinit();
    app.onUpdate(onUpdate);
    app.onDraw(onDraw);
    _ = app.run();
}
