
#ifndef PLAYBOX_PB_TYPES_H
#define PLAYBOX_PB_TYPES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { uint8_t r, g, b; } pb_color;

static inline pb_color pb_rgb(uint8_t r, uint8_t g, uint8_t b){
    pb_color c; c.r=r; c.g=g; c.b=b; return c;
}

typedef enum {
    PB_STYLE_NONE      = 0,
    PB_STYLE_BOLD      = 1u << 0,
    PB_STYLE_DIM       = 1u << 1,
    PB_STYLE_UNDERLINE = 1u << 2,
    PB_STYLE_REVERSE   = 1u << 3
} pb_style;

typedef struct {
    uint32_t ch;
    pb_color fg;
    pb_color bg;
    uint16_t style;
} pb_cell;

typedef enum {
    PB_KEY_NONE = 0,
    PB_KEY_ESC,
    PB_KEY_ENTER,
    PB_KEY_BACKSPACE,
    PB_KEY_TAB,
    PB_KEY_UP,
    PB_KEY_DOWN,
    PB_KEY_LEFT,
    PB_KEY_RIGHT,
    PB_KEY_HOME,
    PB_KEY_END,
    PB_KEY_PGUP,
    PB_KEY_PGDN,
    PB_KEY_INS,
    PB_KEY_DEL,
    PB_KEY_F1,
    PB_KEY_F2,
    PB_KEY_F3,
    PB_KEY_F4,
    PB_KEY_F5,
    PB_KEY_F6,
    PB_KEY_F7,
    PB_KEY_F8,
    PB_KEY_F9,
    PB_KEY_F10,
    PB_KEY_F11,
    PB_KEY_F12
} pb_key;

typedef enum {
    PB_EVENT_NONE = 0,
    PB_EVENT_KEY,
    PB_EVENT_TEXT,
    PB_EVENT_MOUSE,
    PB_EVENT_RESIZE,
    PB_EVENT_QUIT
} pb_event_type;

typedef struct {
    pb_key key;
    uint32_t codepoint;
    uint8_t alt;
    uint8_t ctrl;
    uint8_t shift;
    uint8_t pressed;
} pb_key_event;

typedef struct {
    int x;
    int y;
    uint8_t button;
    uint8_t pressed;
    int wheel;
    uint8_t shift;
    uint8_t alt;
    uint8_t ctrl;
} pb_mouse_event;

typedef struct {
    int width;
    int height;
} pb_resize_event;

typedef struct {
    pb_event_type type;
    union {
        pb_key_event key;
        uint32_t text;
        pb_mouse_event mouse;
        pb_resize_event resize;
    } as;
} pb_event;

#ifdef __cplusplus
}
#endif

#endif
