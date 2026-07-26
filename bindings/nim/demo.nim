## Playbox Nim demo
##   nim c -d:release -o:build/bin/pb_nim_demo bindings/nim/demo.nim
##   ./build/bin/pb_nim_demo

import std/[math, strformat]
import playbox

proc main() =
  let app = newApp("Playbox Nim", 60)
  defer: app.destroy()
  var t = 0.0

  app.onUpdate = proc(a: App; dt: float) =
    t += dt
    if a.isKeyPressed(PbKeyEsc) or a.isCharPressed(uint32('q')):
      a.quit()

  app.onDraw = proc(a: App; fb: Framebuffer) =
    let
      bg = rgb(10, 12, 18)
      neon = rgb(80, 220, 255)
      mag = rgb(255, 120, 200)
      panel = rgb(18, 22, 32)
      fg = rgb(220, 224, 230)
      ang = t * 1.5

    fb.fillGradientV(0, 0, fb.width, fb.height, rgb(8, 10, 16), rgb(24, 32, 56))
    fb.boxEx(0, 0, fb.width, fb.height, PbBoxRounded, rgb(70, 90, 120), bg)

    fb.brailleFillCircle(
      fb.width + int(cos(ang) * 24),
      fb.height * 2 + int(sin(ang) * 16),
      10, neon)
    fb.quadFillCircle(
      fb.width + int(cos(ang * 0.7) * 14),
      fb.height + int(sin(ang * 0.7) * 10),
      6, mag)

    fb.text(2, 0, &"Nim + Playbox  FPS {a.fps}  (ESC/Q quit)", neon, bg, PbStyleBold)

    let
      hw = min(fb.width - 4, 48)
      hh = min(fb.height - 4, 8)
      hx = (fb.width - hw) div 2
      hy = (fb.height - hh) div 2
    fb.shadow(hx, hy, hw, hh, rgb(0, 0, 0), 0.4)
    fb.panelEx(hx, hy, hw, hh, "Playbox Nim", PbBoxRounded, neon, fg, panel)
    discard fb.textWrap(hx + 2, hy + 2, hw - 4, hh - 3,
      "Idiomatic Nim bindings: App, Framebuffer, braille/quad, rounded panels.",
      fg, panel)

  discard app.run()

when isMainModule:
  main()
