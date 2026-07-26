// Tiny Go Playbox demo. From repo root:
//
//	cd bindings/go && go run ./demo
package main

import (
	"fmt"

	"github.com/playboxlib/playbox/bindings/go/playbox"
)

func main() {
	app := playbox.NewApp("Playbox Go", 60)
	defer app.Destroy()

	app.OnEvent = func(a *playbox.App, ev playbox.Event) {
		switch ev.Type {
		case playbox.EventQuit:
			a.Quit()
		case playbox.EventKey:
			k := ev.Key()
			if k.Pressed && k.Key == playbox.KeyEsc {
				a.Quit()
			}
		}
	}

	app.OnDraw = func(a *playbox.App, fb *playbox.Framebuffer) {
		top := playbox.RGB(8, 10, 16)
		bot := playbox.RGB(24, 32, 56)
		neon := playbox.RGB(80, 220, 255)
		panelBg := playbox.RGB(14, 18, 28)
		border := playbox.RGB(60, 140, 200)
		titleFg := playbox.RGB(200, 230, 255)
		shadow := playbox.RGB(0, 0, 0)

		fb.FillGradientV(0, 0, fb.W(), fb.H(), top, bot)
		fb.BrailleFillCircle(fb.W(), fb.H()*2, 16, neon)

		pw := fb.W() - 8
		if pw > 48 {
			pw = 48
		}
		if pw < 20 {
			pw = 20
		}
		ph := 8
		if fb.H()-4 < ph {
			ph = fb.H() - 4
		}
		if ph < 5 {
			ph = 5
		}
		px, py := 4, 2
		fb.Shadow(px+1, py+1, pw, ph, shadow, 0.5)
		fb.PanelEx(px, py, pw, ph, " Go ", playbox.BoxRounded, border, titleFg, panelBg, 0)
		fb.Text(px+2, py+2,
			fmt.Sprintf("Playbox %s  FPS %d  (Esc quit)", playbox.Version(), a.FPS()),
			neon, panelBg, playbox.StyleBold)
	}

	app.Run()
}
