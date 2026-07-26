"""PlayboxLib Python bindings (ctypes).

Requires libplaybox.so on LD_LIBRARY_PATH or PLAYBOX_LIB_DIR.
Falls back to <repo>/build/lib when running from a source checkout.
"""

from __future__ import annotations

import ctypes
import os
from ctypes import (
    CFUNCTYPE,
    POINTER,
    Structure,
    Union,
    byref,
    c_char_p,
    c_double,
    c_float,
    c_int,
    c_uint,
    c_uint16,
    c_uint32,
    c_uint8,
    c_void_p,
)
from typing import Optional

__all__ = [
    "Color",
    "Cell",
    "Fb",
    "Event",
    "KeyEvent",
    "MouseEvent",
    "ResizeEvent",
    "FocusEvent",
    "Framebuffer",
    "Particles",
    "App",
    "rgb",
    "cell",
    "version",
    "version_tuple",
    "measure_text",
    "char_width",
    "PB_STYLE_NONE",
    "PB_STYLE_BOLD",
    "PB_STYLE_DIM",
    "PB_STYLE_UNDERLINE",
    "PB_STYLE_REVERSE",
    "PB_STYLE_ITALIC",
    "PB_STYLE_STRIKETHROUGH",
    "PB_KEY_NONE",
    "PB_KEY_ESC",
    "PB_KEY_ENTER",
    "PB_KEY_BACKSPACE",
    "PB_KEY_TAB",
    "PB_KEY_UP",
    "PB_KEY_DOWN",
    "PB_KEY_LEFT",
    "PB_KEY_RIGHT",
    "PB_KEY_HOME",
    "PB_KEY_END",
    "PB_KEY_PGUP",
    "PB_KEY_PGDN",
    "PB_KEY_INS",
    "PB_KEY_DEL",
    "PB_KEY_F1",
    "PB_KEY_F2",
    "PB_KEY_F3",
    "PB_KEY_F4",
    "PB_KEY_F5",
    "PB_KEY_F6",
    "PB_KEY_F7",
    "PB_KEY_F8",
    "PB_KEY_F9",
    "PB_KEY_F10",
    "PB_KEY_F11",
    "PB_KEY_F12",
    "PB_MOUSE_LEFT",
    "PB_MOUSE_MIDDLE",
    "PB_MOUSE_RIGHT",
    "PB_EVENT_NONE",
    "PB_EVENT_KEY",
    "PB_EVENT_TEXT",
    "PB_EVENT_MOUSE",
    "PB_EVENT_RESIZE",
    "PB_EVENT_QUIT",
    "PB_EVENT_FOCUS",
    "PB_BOX_SINGLE",
    "PB_BOX_DOUBLE",
    "PB_BOX_ROUNDED",
    "PB_BOX_HEAVY",
    "PB_BOX_ASCII",
    "PB_BOX_DASHED",
    "PB_BLEND_REPLACE",
    "PB_BLEND_ALPHA",
    "PB_BLEND_ADD",
    "PB_BLEND_MUL",
    "PB_APP_FLAG_NO_AUTO_CLEAR",
    "PB_APP_FLAG_NO_MOUSE",
    "PB_APP_FLAG_NO_FOCUS",
    "PB_APP_FLAG_CUSTOM_CLEAR",
]


# ---------------------------------------------------------------------------
# Library load
# ---------------------------------------------------------------------------

def _load():
    libdir = os.environ.get("PLAYBOX_LIB_DIR", "")
    names = ["libplaybox.so", "libplaybox.so.1", "libplaybox.so.1.1.0"]
    paths = []
    if libdir:
        paths += [os.path.join(libdir, n) for n in names]
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
    paths += [os.path.join(root, "build", "lib", n) for n in names]
    paths += names
    last = None
    for p in paths:
        try:
            return ctypes.CDLL(p)
        except OSError as e:
            last = e
    raise OSError(f"could not load libplaybox: {last}")


_lib = _load()


# ---------------------------------------------------------------------------
# C structures
# ---------------------------------------------------------------------------

class Color(Structure):
    _fields_ = [("r", c_uint8), ("g", c_uint8), ("b", c_uint8)]

    def __repr__(self):
        return f"Color({self.r}, {self.g}, {self.b})"


class Cell(Structure):
    _fields_ = [("ch", c_uint32), ("fg", Color), ("bg", Color), ("style", c_uint16)]


class Fb(Structure):
    _fields_ = [
        ("w", c_int),
        ("h", c_int),
        ("cells", c_void_p),
        ("cam_x", c_int),
        ("cam_y", c_int),
        ("clip_x0", c_int),
        ("clip_y0", c_int),
        ("clip_x1", c_int),
        ("clip_y1", c_int),
    ]


class KeyEvent(Structure):
    _fields_ = [
        ("key", c_int),
        ("codepoint", c_uint32),
        ("alt", c_uint8),
        ("ctrl", c_uint8),
        ("shift", c_uint8),
        ("pressed", c_uint8),
    ]


class MouseEvent(Structure):
    _fields_ = [
        ("x", c_int),
        ("y", c_int),
        ("button", c_uint8),
        ("pressed", c_uint8),
        ("wheel", c_int),
        ("shift", c_uint8),
        ("alt", c_uint8),
        ("ctrl", c_uint8),
    ]


class ResizeEvent(Structure):
    _fields_ = [("width", c_int), ("height", c_int)]


class FocusEvent(Structure):
    _fields_ = [("focused", c_uint8)]


class EventData(Union):
    _fields_ = [
        ("key", KeyEvent),
        ("text", c_uint32),
        ("mouse", MouseEvent),
        ("resize", ResizeEvent),
        ("focus", FocusEvent),
    ]


class Event(Structure):
    _fields_ = [("type", c_int), ("as_", EventData)]


class Particle(Structure):
    _fields_ = [
        ("x", c_float),
        ("y", c_float),
        ("vx", c_float),
        ("vy", c_float),
        ("life", c_float),
        ("max_life", c_float),
        ("color", Color),
        ("alive", c_uint8),
    ]


class ParticlesRaw(Structure):
    _fields_ = [
        ("items", POINTER(Particle)),
        ("count", c_int),
        ("capacity", c_int),
    ]


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

PB_STYLE_NONE = 0
PB_STYLE_BOLD = 1 << 0
PB_STYLE_DIM = 1 << 1
PB_STYLE_UNDERLINE = 1 << 2
PB_STYLE_REVERSE = 1 << 3
PB_STYLE_ITALIC = 1 << 4
PB_STYLE_STRIKETHROUGH = 1 << 5

PB_KEY_NONE = 0
PB_KEY_ESC = 1
PB_KEY_ENTER = 2
PB_KEY_BACKSPACE = 3
PB_KEY_TAB = 4
PB_KEY_UP = 5
PB_KEY_DOWN = 6
PB_KEY_LEFT = 7
PB_KEY_RIGHT = 8
PB_KEY_HOME = 9
PB_KEY_END = 10
PB_KEY_PGUP = 11
PB_KEY_PGDN = 12
PB_KEY_INS = 13
PB_KEY_DEL = 14
PB_KEY_F1 = 15
PB_KEY_F2 = 16
PB_KEY_F3 = 17
PB_KEY_F4 = 18
PB_KEY_F5 = 19
PB_KEY_F6 = 20
PB_KEY_F7 = 21
PB_KEY_F8 = 22
PB_KEY_F9 = 23
PB_KEY_F10 = 24
PB_KEY_F11 = 25
PB_KEY_F12 = 26

PB_MOUSE_LEFT = 0
PB_MOUSE_MIDDLE = 1
PB_MOUSE_RIGHT = 2

PB_EVENT_NONE = 0
PB_EVENT_KEY = 1
PB_EVENT_TEXT = 2
PB_EVENT_MOUSE = 3
PB_EVENT_RESIZE = 4
PB_EVENT_QUIT = 5
PB_EVENT_FOCUS = 6

PB_BOX_SINGLE = 0
PB_BOX_DOUBLE = 1
PB_BOX_ROUNDED = 2
PB_BOX_HEAVY = 3
PB_BOX_ASCII = 4
PB_BOX_DASHED = 5

PB_BLEND_REPLACE = 0
PB_BLEND_ALPHA = 1
PB_BLEND_ADD = 2
PB_BLEND_MUL = 3

PB_APP_FLAG_NO_AUTO_CLEAR = 1 << 0
PB_APP_FLAG_NO_MOUSE = 1 << 1
PB_APP_FLAG_NO_FOCUS = 1 << 2
PB_APP_FLAG_CUSTOM_CLEAR = 1 << 3


# ---------------------------------------------------------------------------
# Callbacks / AppDesc
# ---------------------------------------------------------------------------

OnInit = CFUNCTYPE(None, c_void_p, c_void_p)
OnEvent = CFUNCTYPE(None, c_void_p, c_void_p, POINTER(Event))
OnUpdate = CFUNCTYPE(None, c_void_p, c_void_p, c_double)
OnDraw = CFUNCTYPE(None, c_void_p, c_void_p, POINTER(Fb))
OnShutdown = CFUNCTYPE(None, c_void_p, c_void_p)


class AppDesc(Structure):
    _fields_ = [
        ("title", c_char_p),
        ("target_fps", c_int),
        ("flags", c_uint),
        ("clear", Cell),
        ("on_init", OnInit),
        ("on_event", OnEvent),
        ("on_update", OnUpdate),
        ("on_draw", OnDraw),
        ("on_shutdown", OnShutdown),
    ]


# ---------------------------------------------------------------------------
# ctypes signatures
# ---------------------------------------------------------------------------

def _bind(name, argtypes, restype=None):
    fn = getattr(_lib, name)
    fn.argtypes = argtypes
    fn.restype = restype
    return fn


_pb_rgb_ex = _bind("pb_rgb_ex", [c_uint8, c_uint8, c_uint8], Color)
_pb_cell_ex = _bind("pb_cell_ex", [c_uint32, Color, Color, c_uint16], Cell)
_pb_version_string = _bind("pb_version_string", [], c_char_p)
_pb_version = _bind("pb_version", [POINTER(c_int), POINTER(c_int), POINTER(c_int)], None)

_pb_fb_create = _bind("pb_fb_create", [c_int, c_int], POINTER(Fb))
_pb_fb_destroy = _bind("pb_fb_destroy", [POINTER(Fb)], None)
_pb_fb_free = _bind("pb_fb_free", [POINTER(Fb)], None)
_pb_fb_clear = _bind("pb_fb_clear", [POINTER(Fb), Cell], None)
_pb_fb_put = _bind("pb_fb_put", [POINTER(Fb), c_int, c_int, Cell], None)
_pb_fb_get = _bind("pb_fb_get", [POINTER(Fb), c_int, c_int], Cell)
_pb_fb_put_screen = _bind("pb_fb_put_screen", [POINTER(Fb), c_int, c_int, Cell], None)
_pb_fb_get_screen = _bind("pb_fb_get_screen", [POINTER(Fb), c_int, c_int], Cell)
_pb_fb_text = _bind("pb_fb_text", [POINTER(Fb), c_int, c_int, c_char_p, Color, Color, c_uint16], None)
_pb_fb_text_centered = _bind(
    "pb_fb_text_centered", [POINTER(Fb), c_int, c_char_p, Color, Color, c_uint16], None
)
_pb_fb_text_wrap = _bind(
    "pb_fb_text_wrap",
    [POINTER(Fb), c_int, c_int, c_int, c_int, c_char_p, Color, Color, c_uint16],
    c_int,
)
_pb_fb_text_clipped = _bind(
    "pb_fb_text_clipped",
    [POINTER(Fb), c_int, c_int, c_int, c_char_p, Color, Color, c_uint16],
    None,
)
_pb_fb_measure_text = _bind("pb_fb_measure_text", [c_char_p], c_int)
_pb_char_width = _bind("pb_char_width", [c_uint32], c_int)
_pb_fb_fill_rect = _bind("pb_fb_fill_rect", [POINTER(Fb), c_int, c_int, c_int, c_int, Cell], None)
_pb_fb_box = _bind("pb_fb_box", [POINTER(Fb), c_int, c_int, c_int, c_int, Color, Color, c_uint16], None)
_pb_fb_box_double = _bind(
    "pb_fb_box_double", [POINTER(Fb), c_int, c_int, c_int, c_int, Color, Color, c_uint16], None
)
_pb_fb_box_ex = _bind(
    "pb_fb_box_ex",
    [POINTER(Fb), c_int, c_int, c_int, c_int, c_int, Color, Color, c_uint16],
    None,
)
_pb_fb_panel = _bind(
    "pb_fb_panel",
    [POINTER(Fb), c_int, c_int, c_int, c_int, c_char_p, Color, Color, Color, c_uint16],
    None,
)
_pb_fb_panel_ex = _bind(
    "pb_fb_panel_ex",
    [POINTER(Fb), c_int, c_int, c_int, c_int, c_char_p, c_int, Color, Color, Color, c_uint16],
    None,
)
_pb_fb_shadow = _bind(
    "pb_fb_shadow", [POINTER(Fb), c_int, c_int, c_int, c_int, Color, c_float], None
)
_pb_fb_hline = _bind("pb_fb_hline", [POINTER(Fb), c_int, c_int, c_int, Cell], None)
_pb_fb_vline = _bind("pb_fb_vline", [POINTER(Fb), c_int, c_int, c_int, Cell], None)
_pb_fb_line = _bind("pb_fb_line", [POINTER(Fb), c_int, c_int, c_int, c_int, Cell], None)
_pb_fb_circle = _bind("pb_fb_circle", [POINTER(Fb), c_int, c_int, c_int, Cell], None)
_pb_fb_fill_circle = _bind("pb_fb_fill_circle", [POINTER(Fb), c_int, c_int, c_int, Cell], None)
_pb_fb_fill_triangle = _bind(
    "pb_fb_fill_triangle",
    [POINTER(Fb), c_int, c_int, c_int, c_int, c_int, c_int, Cell],
    None,
)
_pb_fb_blit = _bind("pb_fb_blit", [POINTER(Fb), c_int, c_int, POINTER(Fb)], None)
_pb_fb_blit_region = _bind(
    "pb_fb_blit_region",
    [POINTER(Fb), c_int, c_int, POINTER(Fb), c_int, c_int, c_int, c_int],
    None,
)
_pb_fb_blit_masked = _bind(
    "pb_fb_blit_masked", [POINTER(Fb), c_int, c_int, POINTER(Fb), c_uint32], None
)
_pb_fb_blit_blend = _bind(
    "pb_fb_blit_blend", [POINTER(Fb), c_int, c_int, POINTER(Fb), c_float, c_int], None
)
_pb_fb_put_blend = _bind(
    "pb_fb_put_blend", [POINTER(Fb), c_int, c_int, Cell, c_float, c_int], None
)
_pb_fb_plot = _bind("pb_fb_plot", [POINTER(Fb), c_int, c_int, Color], None)
_pb_fb_plot_blend = _bind("pb_fb_plot_blend", [POINTER(Fb), c_int, c_int, Color, c_float], None)
_pb_fb_plot_line = _bind(
    "pb_fb_plot_line", [POINTER(Fb), c_int, c_int, c_int, c_int, Color], None
)
_pb_fb_plot_rect = _bind(
    "pb_fb_plot_rect", [POINTER(Fb), c_int, c_int, c_int, c_int, Color], None
)
_pb_fb_plot_fill_rect = _bind(
    "pb_fb_plot_fill_rect", [POINTER(Fb), c_int, c_int, c_int, c_int, Color], None
)
_pb_fb_plot_circle = _bind("pb_fb_plot_circle", [POINTER(Fb), c_int, c_int, c_int, Color], None)
_pb_fb_plot_fill_circle = _bind(
    "pb_fb_plot_fill_circle", [POINTER(Fb), c_int, c_int, c_int, Color], None
)
_pb_fb_braille_clear = _bind("pb_fb_braille_clear", [POINTER(Fb), Color], None)
_pb_fb_braille_plot = _bind("pb_fb_braille_plot", [POINTER(Fb), c_int, c_int, Color], None)
_pb_fb_braille_plot_blend = _bind(
    "pb_fb_braille_plot_blend", [POINTER(Fb), c_int, c_int, Color, c_float], None
)
_pb_fb_braille_line = _bind(
    "pb_fb_braille_line", [POINTER(Fb), c_int, c_int, c_int, c_int, Color], None
)
_pb_fb_braille_fill_rect = _bind(
    "pb_fb_braille_fill_rect", [POINTER(Fb), c_int, c_int, c_int, c_int, Color], None
)
_pb_fb_braille_fill_circle = _bind(
    "pb_fb_braille_fill_circle", [POINTER(Fb), c_int, c_int, c_int, Color], None
)
_pb_fb_braille_circle = _bind(
    "pb_fb_braille_circle", [POINTER(Fb), c_int, c_int, c_int, Color], None
)
_pb_fb_braille_fill_triangle = _bind(
    "pb_fb_braille_fill_triangle",
    [POINTER(Fb), c_int, c_int, c_int, c_int, c_int, c_int, Color],
    None,
)
_pb_fb_quad_plot = _bind("pb_fb_quad_plot", [POINTER(Fb), c_int, c_int, Color], None)
_pb_fb_quad_fill_rect = _bind(
    "pb_fb_quad_fill_rect", [POINTER(Fb), c_int, c_int, c_int, c_int, Color], None
)
_pb_fb_quad_fill_circle = _bind(
    "pb_fb_quad_fill_circle", [POINTER(Fb), c_int, c_int, c_int, Color], None
)
_pb_fb_pixel = _bind("pb_fb_pixel", [POINTER(Fb), c_int, c_int, Color], None)
_pb_fb_fill_shade = _bind(
    "pb_fb_fill_shade", [POINTER(Fb), c_int, c_int, c_int, c_int, Color, Color, c_int], None
)
_pb_fb_fill_gradient_v = _bind(
    "pb_fb_fill_gradient_v", [POINTER(Fb), c_int, c_int, c_int, c_int, Color, Color], None
)
_pb_fb_fill_gradient_h = _bind(
    "pb_fb_fill_gradient_h", [POINTER(Fb), c_int, c_int, c_int, c_int, Color, Color], None
)
_pb_fb_fill_dither = _bind(
    "pb_fb_fill_dither",
    [POINTER(Fb), c_int, c_int, c_int, c_int, Color, Color, c_int],
    None,
)
_pb_fb_set_camera = _bind("pb_fb_set_camera", [POINTER(Fb), c_int, c_int], None)
_pb_fb_get_camera = _bind(
    "pb_fb_get_camera", [POINTER(Fb), POINTER(c_int), POINTER(c_int)], None
)
_pb_fb_set_clip = _bind("pb_fb_set_clip", [POINTER(Fb), c_int, c_int, c_int, c_int], None)
_pb_fb_reset_clip = _bind("pb_fb_reset_clip", [POINTER(Fb)], None)

_pb_particles_init = _bind("pb_particles_init", [POINTER(ParticlesRaw), c_int], c_int)
_pb_particles_free = _bind("pb_particles_free", [POINTER(ParticlesRaw)], None)
_pb_particles_emit = _bind(
    "pb_particles_emit",
    [POINTER(ParticlesRaw), c_float, c_float, c_float, c_float, c_float, Color],
    None,
)
_pb_particles_update = _bind("pb_particles_update", [POINTER(ParticlesRaw), c_double], None)
_pb_particles_draw_braille = _bind(
    "pb_particles_draw_braille", [POINTER(Fb), POINTER(ParticlesRaw)], None
)
_pb_particles_draw_half = _bind(
    "pb_particles_draw_half", [POINTER(Fb), POINTER(ParticlesRaw)], None
)

_pb_app_create = _bind("pb_app_create", [POINTER(AppDesc), c_void_p], c_void_p)
_pb_app_destroy = _bind("pb_app_destroy", [c_void_p], None)
_pb_app_run = _bind("pb_app_run", [c_void_p], c_int)
_pb_app_quit = _bind("pb_app_quit", [c_void_p], None)
_pb_app_request_resize = _bind("pb_app_request_resize", [c_void_p], None)
_pb_app_width = _bind("pb_app_width", [c_void_p], c_int)
_pb_app_height = _bind("pb_app_height", [c_void_p], c_int)
_pb_app_set_title = _bind("pb_app_set_title", [c_void_p, c_char_p], None)
_pb_app_set_clear = _bind("pb_app_set_clear", [c_void_p, Cell], None)
_pb_app_set_target_fps = _bind("pb_app_set_target_fps", [c_void_p, c_int], None)
_pb_app_is_replay = _bind("pb_app_is_replay", [c_void_p], c_int)
_pb_app_is_recording = _bind("pb_app_is_recording", [c_void_p], c_int)

_pb_is_key_down = _bind("pb_is_key_down", [c_void_p, c_int], c_int)
_pb_is_key_pressed = _bind("pb_is_key_pressed", [c_void_p, c_int], c_int)
_pb_is_key_released = _bind("pb_is_key_released", [c_void_p, c_int], c_int)
_pb_is_char_down = _bind("pb_is_char_down", [c_void_p, c_uint32], c_int)
_pb_is_char_pressed = _bind("pb_is_char_pressed", [c_void_p, c_uint32], c_int)
_pb_get_mouse_x = _bind("pb_get_mouse_x", [c_void_p], c_int)
_pb_get_mouse_y = _bind("pb_get_mouse_y", [c_void_p], c_int)
_pb_is_mouse_button_down = _bind("pb_is_mouse_button_down", [c_void_p, c_int], c_int)
_pb_is_mouse_button_pressed = _bind("pb_is_mouse_button_pressed", [c_void_p, c_int], c_int)
_pb_is_mouse_button_released = _bind("pb_is_mouse_button_released", [c_void_p, c_int], c_int)
_pb_get_mouse_wheel = _bind("pb_get_mouse_wheel", [c_void_p], c_int)
_pb_get_frame_time = _bind("pb_get_frame_time", [c_void_p], c_double)
_pb_get_fps = _bind("pb_get_fps", [c_void_p], c_int)
_pb_is_focused = _bind("pb_is_focused", [c_void_p], c_int)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def rgb(r: int, g: int, b: int) -> Color:
    return _pb_rgb_ex(r & 0xFF, g & 0xFF, b & 0xFF)


def cell(ch, fg: Color, bg: Color, style: int = 0) -> Cell:
    if isinstance(ch, str):
        ch = ord(ch[0]) if ch else 32
    return _pb_cell_ex(int(ch), fg, bg, int(style))


def version() -> str:
    s = _pb_version_string()
    return s.decode() if s else ""


def version_tuple():
    maj = c_int()
    mn = c_int()
    pat = c_int()
    _pb_version(byref(maj), byref(mn), byref(pat))
    return maj.value, mn.value, pat.value


def measure_text(text: str) -> int:
    return int(_pb_fb_measure_text(text.encode("utf-8")))


def char_width(codepoint: int) -> int:
    return int(_pb_char_width(int(codepoint)))


def _enc(s: str) -> bytes:
    return s.encode("utf-8") if isinstance(s, str) else s


# ---------------------------------------------------------------------------
# Framebuffer — wraps POINTER(Fb); never copies the struct
# ---------------------------------------------------------------------------

class Framebuffer:
    """Thin wrapper around a live ``pb_fb*``.

    Always hold the pointer (not ``fb.contents``); copying the struct
    would leave draws writing into a detached cells buffer.
    """

    __slots__ = ("_ptr", "_owned")

    def __init__(self, ptr, owned: bool = False):
        if ptr is None:
            raise ValueError("Framebuffer pointer is None")
        self._ptr = ptr  # POINTER(Fb) or addressable ctypes pointer
        self._owned = owned

    @classmethod
    def create(cls, w: int, h: int) -> "Framebuffer":
        p = _pb_fb_create(int(w), int(h))
        if not p:
            raise RuntimeError("pb_fb_create failed")
        return cls(p, owned=True)

    def destroy(self):
        if self._owned and self._ptr:
            _pb_fb_destroy(self._ptr)
            self._ptr = None
            self._owned = False

    def __del__(self):
        try:
            self.destroy()
        except Exception:
            pass

    @property
    def ptr(self):
        return self._ptr

    @property
    def w(self) -> int:
        return int(self._ptr.contents.w) if self._ptr else 0

    @property
    def h(self) -> int:
        return int(self._ptr.contents.h) if self._ptr else 0

    def clear(self, fill: Cell):
        _pb_fb_clear(self._ptr, fill)

    def put(self, x: int, y: int, c: Cell):
        _pb_fb_put(self._ptr, x, y, c)

    def put_blend(self, x: int, y: int, c: Cell, alpha: float = 1.0, mode: int = PB_BLEND_ALPHA):
        _pb_fb_put_blend(self._ptr, x, y, c, float(alpha), int(mode))

    def get(self, x: int, y: int) -> Cell:
        return _pb_fb_get(self._ptr, x, y)

    def put_screen(self, x: int, y: int, c: Cell):
        _pb_fb_put_screen(self._ptr, x, y, c)

    def get_screen(self, x: int, y: int) -> Cell:
        return _pb_fb_get_screen(self._ptr, x, y)

    def text(self, x: int, y: int, s: str, fg: Color, bg: Color, style: int = 0):
        _pb_fb_text(self._ptr, x, y, _enc(s), fg, bg, int(style))

    def text_centered(self, y: int, s: str, fg: Color, bg: Color, style: int = 0):
        _pb_fb_text_centered(self._ptr, y, _enc(s), fg, bg, int(style))

    def text_wrap(
        self, x: int, y: int, max_w: int, max_h: int, s: str, fg: Color, bg: Color, style: int = 0
    ) -> int:
        return int(_pb_fb_text_wrap(self._ptr, x, y, max_w, max_h, _enc(s), fg, bg, int(style)))

    def text_clipped(
        self, x: int, y: int, max_w: int, s: str, fg: Color, bg: Color, style: int = 0
    ):
        _pb_fb_text_clipped(self._ptr, x, y, max_w, _enc(s), fg, bg, int(style))

    def fill_rect(self, x: int, y: int, w: int, h: int, c: Cell):
        _pb_fb_fill_rect(self._ptr, x, y, w, h, c)

    def box(self, x: int, y: int, w: int, h: int, fg: Color, bg: Color, style: int = 0):
        _pb_fb_box(self._ptr, x, y, w, h, fg, bg, int(style))

    def box_double(self, x: int, y: int, w: int, h: int, fg: Color, bg: Color, style: int = 0):
        _pb_fb_box_double(self._ptr, x, y, w, h, fg, bg, int(style))

    def box_ex(
        self,
        x: int,
        y: int,
        w: int,
        h: int,
        box_style: int,
        fg: Color,
        bg: Color,
        style: int = 0,
    ):
        _pb_fb_box_ex(self._ptr, x, y, w, h, int(box_style), fg, bg, int(style))

    def panel(
        self,
        x: int,
        y: int,
        w: int,
        h: int,
        title: str,
        border: Color,
        title_fg: Color,
        fill: Color,
        style: int = 0,
    ):
        _pb_fb_panel(self._ptr, x, y, w, h, _enc(title), border, title_fg, fill, int(style))

    def panel_ex(
        self,
        x: int,
        y: int,
        w: int,
        h: int,
        title: str,
        box_style: int,
        border: Color,
        title_fg: Color,
        fill: Color,
        style: int = 0,
    ):
        _pb_fb_panel_ex(
            self._ptr, x, y, w, h, _enc(title), int(box_style), border, title_fg, fill, int(style)
        )

    def shadow(self, x: int, y: int, w: int, h: int, shadow: Color, alpha: float = 0.45):
        _pb_fb_shadow(self._ptr, x, y, w, h, shadow, float(alpha))

    def hline(self, x: int, y: int, w: int, c: Cell):
        _pb_fb_hline(self._ptr, x, y, w, c)

    def vline(self, x: int, y: int, h: int, c: Cell):
        _pb_fb_vline(self._ptr, x, y, h, c)

    def line(self, x0: int, y0: int, x1: int, y1: int, c: Cell):
        _pb_fb_line(self._ptr, x0, y0, x1, y1, c)

    def circle(self, cx: int, cy: int, radius: int, c: Cell):
        _pb_fb_circle(self._ptr, cx, cy, radius, c)

    def fill_circle(self, cx: int, cy: int, radius: int, c: Cell):
        _pb_fb_fill_circle(self._ptr, cx, cy, radius, c)

    def fill_triangle(
        self, x0: int, y0: int, x1: int, y1: int, x2: int, y2: int, c: Cell
    ):
        _pb_fb_fill_triangle(self._ptr, x0, y0, x1, y1, x2, y2, c)

    def blit(self, dx: int, dy: int, src: "Framebuffer"):
        _pb_fb_blit(self._ptr, dx, dy, src._ptr)

    def blit_region(
        self, dx: int, dy: int, src: "Framebuffer", sx: int, sy: int, w: int, h: int
    ):
        _pb_fb_blit_region(self._ptr, dx, dy, src._ptr, sx, sy, w, h)

    def blit_masked(self, dx: int, dy: int, src: "Framebuffer", transparent_ch: int):
        _pb_fb_blit_masked(self._ptr, dx, dy, src._ptr, int(transparent_ch))

    def blit_blend(
        self,
        dx: int,
        dy: int,
        src: "Framebuffer",
        alpha: float = 1.0,
        mode: int = PB_BLEND_ALPHA,
    ):
        _pb_fb_blit_blend(self._ptr, dx, dy, src._ptr, float(alpha), int(mode))

    def plot(self, px: int, py: int, color: Color):
        _pb_fb_plot(self._ptr, px, py, color)

    def plot_blend(self, px: int, py: int, color: Color, alpha: float):
        _pb_fb_plot_blend(self._ptr, px, py, color, float(alpha))

    def plot_line(self, x0: int, y0: int, x1: int, y1: int, color: Color):
        _pb_fb_plot_line(self._ptr, x0, y0, x1, y1, color)

    def plot_rect(self, x: int, y: int, w: int, h: int, color: Color):
        _pb_fb_plot_rect(self._ptr, x, y, w, h, color)

    def plot_fill_rect(self, x: int, y: int, w: int, h: int, color: Color):
        _pb_fb_plot_fill_rect(self._ptr, x, y, w, h, color)

    def plot_circle(self, cx: int, cy: int, radius: int, color: Color):
        _pb_fb_plot_circle(self._ptr, cx, cy, radius, color)

    def plot_fill_circle(self, cx: int, cy: int, radius: int, color: Color):
        _pb_fb_plot_fill_circle(self._ptr, cx, cy, radius, color)

    def braille_clear(self, bg: Color):
        _pb_fb_braille_clear(self._ptr, bg)

    def braille_plot(self, px: int, py: int, color: Color):
        _pb_fb_braille_plot(self._ptr, px, py, color)

    def braille_plot_blend(self, px: int, py: int, color: Color, alpha: float):
        _pb_fb_braille_plot_blend(self._ptr, px, py, color, float(alpha))

    def braille_line(self, x0: int, y0: int, x1: int, y1: int, color: Color):
        _pb_fb_braille_line(self._ptr, x0, y0, x1, y1, color)

    def braille_fill_rect(self, x: int, y: int, w: int, h: int, color: Color):
        _pb_fb_braille_fill_rect(self._ptr, x, y, w, h, color)

    def braille_fill_circle(self, cx: int, cy: int, radius: int, color: Color):
        _pb_fb_braille_fill_circle(self._ptr, cx, cy, radius, color)

    def braille_circle(self, cx: int, cy: int, radius: int, color: Color):
        _pb_fb_braille_circle(self._ptr, cx, cy, radius, color)

    def braille_fill_triangle(
        self, x0: int, y0: int, x1: int, y1: int, x2: int, y2: int, color: Color
    ):
        _pb_fb_braille_fill_triangle(self._ptr, x0, y0, x1, y1, x2, y2, color)

    def quad_plot(self, px: int, py: int, color: Color):
        _pb_fb_quad_plot(self._ptr, px, py, color)

    def quad_fill_rect(self, x: int, y: int, w: int, h: int, color: Color):
        _pb_fb_quad_fill_rect(self._ptr, x, y, w, h, color)

    def quad_fill_circle(self, cx: int, cy: int, radius: int, color: Color):
        _pb_fb_quad_fill_circle(self._ptr, cx, cy, radius, color)

    def pixel(self, x: int, y: int, color: Color):
        _pb_fb_pixel(self._ptr, x, y, color)

    def fill_shade(
        self, x: int, y: int, w: int, h: int, fg: Color, bg: Color, level: int
    ):
        _pb_fb_fill_shade(self._ptr, x, y, w, h, fg, bg, int(level))

    def fill_gradient_v(self, x: int, y: int, w: int, h: int, top: Color, bottom: Color):
        _pb_fb_fill_gradient_v(self._ptr, x, y, w, h, top, bottom)

    def fill_gradient_h(self, x: int, y: int, w: int, h: int, left: Color, right: Color):
        _pb_fb_fill_gradient_h(self._ptr, x, y, w, h, left, right)

    def fill_dither(
        self, x: int, y: int, w: int, h: int, a: Color, b: Color, pattern: int = 0
    ):
        _pb_fb_fill_dither(self._ptr, x, y, w, h, a, b, int(pattern))

    def set_camera(self, cam_x: int, cam_y: int):
        _pb_fb_set_camera(self._ptr, cam_x, cam_y)

    def get_camera(self):
        x = c_int()
        y = c_int()
        _pb_fb_get_camera(self._ptr, byref(x), byref(y))
        return x.value, y.value

    def set_clip(self, x: int, y: int, w: int, h: int):
        _pb_fb_set_clip(self._ptr, x, y, w, h)

    def reset_clip(self):
        _pb_fb_reset_clip(self._ptr)


# ---------------------------------------------------------------------------
# Particles
# ---------------------------------------------------------------------------

class Particles:
    def __init__(self, capacity: int = 256):
        self._raw = ParticlesRaw()
        if _pb_particles_init(byref(self._raw), int(capacity)) == 0:
            raise RuntimeError("pb_particles_init failed")
        self._alive = True

    def destroy(self):
        if self._alive:
            _pb_particles_free(byref(self._raw))
            self._alive = False

    def __del__(self):
        try:
            self.destroy()
        except Exception:
            pass

    def emit(self, x: float, y: float, vx: float, vy: float, life: float, color: Color):
        _pb_particles_emit(
            byref(self._raw), float(x), float(y), float(vx), float(vy), float(life), color
        )

    def update(self, dt: float):
        _pb_particles_update(byref(self._raw), float(dt))

    def draw_braille(self, fb: Framebuffer):
        _pb_particles_draw_braille(fb._ptr, byref(self._raw))

    def draw_half(self, fb: Framebuffer):
        _pb_particles_draw_half(fb._ptr, byref(self._raw))


# ---------------------------------------------------------------------------
# App
# ---------------------------------------------------------------------------

class App:
    def __init__(self, title: str, fps: int = 60, flags: int = 0, clear: Optional[Cell] = None):
        self._title = title.encode("utf-8")
        self.on_init = None
        self.on_event = None
        self.on_update = None
        self.on_draw = None
        self.on_shutdown = None

        # Keep callback objects alive for the lifetime of the App.
        self._cb_init = OnInit(self._init)
        self._cb_event = OnEvent(self._event)
        self._cb_update = OnUpdate(self._update)
        self._cb_draw = OnDraw(self._draw)
        self._cb_shutdown = OnShutdown(self._shutdown)

        desc = AppDesc()
        desc.title = self._title
        desc.target_fps = int(fps)
        desc.flags = int(flags)
        if clear is not None:
            desc.clear = clear
            desc.flags |= PB_APP_FLAG_CUSTOM_CLEAR
        desc.on_init = self._cb_init
        desc.on_event = self._cb_event
        desc.on_update = self._cb_update
        desc.on_draw = self._cb_draw
        desc.on_shutdown = self._cb_shutdown

        self._app = _pb_app_create(byref(desc), None)
        if not self._app:
            raise RuntimeError("pb_app_create failed")

    def _init(self, app, user):
        if self.on_init:
            self.on_init(self)

    def _event(self, app, user, ev):
        if self.on_event:
            # Events are value-sized; copying the Event struct is fine.
            self.on_event(self, ev.contents)

    def _update(self, app, user, dt):
        if self.on_update:
            self.on_update(self, float(dt))

    def _draw(self, app, user, fb):
        # CRITICAL: pass the POINTER(Fb), never fb.contents (struct copy).
        if self.on_draw:
            self.on_draw(self, Framebuffer(fb, owned=False))

    def _shutdown(self, app, user):
        if self.on_shutdown:
            self.on_shutdown(self)

    def run(self) -> int:
        return int(_pb_app_run(self._app))

    def quit(self):
        _pb_app_quit(self._app)

    def destroy(self):
        if self._app:
            _pb_app_destroy(self._app)
            self._app = None

    def __del__(self):
        try:
            self.destroy()
        except Exception:
            pass

    def request_resize(self):
        _pb_app_request_resize(self._app)

    def width(self) -> int:
        return int(_pb_app_width(self._app))

    def height(self) -> int:
        return int(_pb_app_height(self._app))

    def set_title(self, title: str):
        self._title = title.encode("utf-8")
        _pb_app_set_title(self._app, self._title)

    def set_clear(self, c: Cell):
        _pb_app_set_clear(self._app, c)

    def set_target_fps(self, fps: int):
        _pb_app_set_target_fps(self._app, int(fps))

    def fps(self) -> int:
        return int(_pb_get_fps(self._app))

    def frame_time(self) -> float:
        return float(_pb_get_frame_time(self._app))

    def focused(self) -> bool:
        return bool(_pb_is_focused(self._app))

    def is_key_down(self, key: int) -> bool:
        return bool(_pb_is_key_down(self._app, int(key)))

    def is_key_pressed(self, key: int) -> bool:
        return bool(_pb_is_key_pressed(self._app, int(key)))

    def is_key_released(self, key: int) -> bool:
        return bool(_pb_is_key_released(self._app, int(key)))

    def is_char_down(self, codepoint: int) -> bool:
        return bool(_pb_is_char_down(self._app, int(codepoint)))

    def is_char_pressed(self, codepoint: int) -> bool:
        return bool(_pb_is_char_pressed(self._app, int(codepoint)))

    def mouse_x(self) -> int:
        return int(_pb_get_mouse_x(self._app))

    def mouse_y(self) -> int:
        return int(_pb_get_mouse_y(self._app))

    def mouse(self):
        return self.mouse_x(), self.mouse_y()

    def is_mouse_down(self, button: int = PB_MOUSE_LEFT) -> bool:
        return bool(_pb_is_mouse_button_down(self._app, int(button)))

    def is_mouse_pressed(self, button: int = PB_MOUSE_LEFT) -> bool:
        return bool(_pb_is_mouse_button_pressed(self._app, int(button)))

    def is_mouse_released(self, button: int = PB_MOUSE_LEFT) -> bool:
        return bool(_pb_is_mouse_button_released(self._app, int(button)))

    def mouse_wheel(self) -> int:
        return int(_pb_get_mouse_wheel(self._app))

    def is_replay(self) -> bool:
        return bool(_pb_app_is_replay(self._app))

    def is_recording(self) -> bool:
        return bool(_pb_app_is_recording(self._app))
