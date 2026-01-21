#include "playbox/pb_input.h"
#include "playbox/pb_term.h"
#include "playbox/pb_time.h"
#include "pb_utf8.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct pb_input {
    pb_term* term;
    uint8_t buf[4096];
    size_t len;
    int esc_pending;
    uint64_t esc_since;
};

pb_input* pb_input_create(void){
    pb_input* in = (pb_input*)calloc(1, sizeof(pb_input));
    if(!in) return NULL;
    in->term = NULL;
    in->len = 0;
    in->esc_pending = 0;
    in->esc_since = 0;
    return in;
}

void pb_input_destroy(pb_input* in){
    if(!in) return;
    free(in);
}

int pb_input_attach(pb_input* in, void* term_handle){
    if(!in) return 0;
    in->term = (pb_term*)term_handle;
    return in->term ? 1 : 0;
}

void pb_input_flush(pb_input* in){
    if(!in) return;
    in->len = 0;
    in->esc_pending = 0;
    in->esc_since = 0;
}

static void pb_buf_consume(pb_input* in, size_t n){
    if(n >= in->len){
        in->len = 0;
        return;
    }
    memmove(in->buf, in->buf + n, in->len - n);
    in->len -= n;
}

static int pb_parse_num(const uint8_t* s, size_t n, size_t* io_i, int* out){
    int v = 0;
    size_t i = *io_i;
    int any = 0;
    while(i < n && s[i] >= '0' && s[i] <= '9'){
        any = 1;
        v = v * 10 + (int)(s[i] - '0');
        i++;
    }
    if(!any) return 0;
    *io_i = i;
    *out = v;
    return 1;
}

static void pb_apply_mods(int mod, pb_key_event* ke){
    if(mod <= 1) return;
    int m = mod - 1;
    ke->shift = (m & 1) ? 1 : 0;
    ke->alt   = (m & 2) ? 1 : 0;
    ke->ctrl  = (m & 4) ? 1 : 0;
}

static int pb_try_mouse(pb_input* in, pb_event* out_ev){
    if(in->len < 6) return 0;
    const uint8_t* s = in->buf;
    if(s[0] != 0x1B || s[1] != '[' || s[2] != '<') return 0;

    size_t i = 3;
    int b=0, x=0, y=0;
    if(!pb_parse_num(s, in->len, &i, &b)) return 0;
    if(i >= in->len || s[i] != ';') return 0;
    i++;
    if(!pb_parse_num(s, in->len, &i, &x)) return 0;
    if(i >= in->len || s[i] != ';') return 0;
    i++;
    if(!pb_parse_num(s, in->len, &i, &y)) return 0;
    if(i >= in->len) return 0;

    uint8_t fin = s[i];
    if(fin != 'M' && fin != 'm') return 0;
    i++;

    pb_mouse_event me;
    memset(&me, 0, sizeof(me));
    me.x = x - 1;
    me.y = y - 1;

    int pressed = (fin == 'M') ? 1 : 0;
    me.pressed = (uint8_t)pressed;

    me.button = (uint8_t)(b & 3u);

    int wheel = 0;
    if((b & 64) != 0){
        wheel = (me.button == 0) ? 1 : -1;
        me.button = 0;
        me.pressed = 1;
    }
    me.wheel = wheel;

    me.shift = (b & 4) ? 1 : 0;
    me.alt   = (b & 8) ? 1 : 0;
    me.ctrl  = (b & 16) ? 1 : 0;

    out_ev->type = PB_EVENT_MOUSE;
    out_ev->as.mouse = me;
    pb_buf_consume(in, i);
    return 1;
}

static int pb_try_escape(pb_input* in, pb_event* out_ev){
    if(in->len < 2) return 0;
    const uint8_t* s = in->buf;

    if(s[0] != 0x1B) return 0;

    if(s[1] == 'O'){
        if(in->len < 3) return 0;
        pb_key k = PB_KEY_NONE;
        switch(s[2]){
            case 'A': k = PB_KEY_UP; break;
            case 'B': k = PB_KEY_DOWN; break;
            case 'C': k = PB_KEY_RIGHT; break;
            case 'D': k = PB_KEY_LEFT; break;
            case 'P': k = PB_KEY_F1; break;
            case 'Q': k = PB_KEY_F2; break;
            case 'R': k = PB_KEY_F3; break;
            case 'S': k = PB_KEY_F4; break;
            case 'H': k = PB_KEY_HOME; break;
            case 'F': k = PB_KEY_END; break;
            default: break;
        }
        if(k == PB_KEY_NONE) return 0;

        pb_key_event ke;
        memset(&ke, 0, sizeof(ke));
        ke.key = k;
        ke.pressed = 1;
        out_ev->type = PB_EVENT_KEY;
        out_ev->as.key = ke;
        pb_buf_consume(in, 3);
        return 1;
    }

    if(s[1] != '['){
        uint32_t cp = 0;
        size_t adv = 0;
        if(!pb_utf8_decode(s + 1, in->len - 1, &cp, &adv)) return 0;

        pb_key_event ke;
        memset(&ke, 0, sizeof(ke));
        ke.key = PB_KEY_NONE;
        ke.codepoint = cp;
        ke.alt = 1;
        ke.pressed = 1;
        out_ev->type = PB_EVENT_KEY;
        out_ev->as.key = ke;
        pb_buf_consume(in, 1 + adv);
        return 1;
    }

    if(in->len >= 3 && (s[2] == 'A' || s[2] == 'B' || s[2] == 'C' || s[2] == 'D' || s[2] == 'H' || s[2] == 'F')){
        pb_key k = PB_KEY_NONE;
        if(s[2] == 'A') k = PB_KEY_UP;
        if(s[2] == 'B') k = PB_KEY_DOWN;
        if(s[2] == 'C') k = PB_KEY_RIGHT;
        if(s[2] == 'D') k = PB_KEY_LEFT;
        if(s[2] == 'H') k = PB_KEY_HOME;
        if(s[2] == 'F') k = PB_KEY_END;

        pb_key_event ke;
        memset(&ke, 0, sizeof(ke));
        ke.key = k;
        ke.pressed = 1;
        out_ev->type = PB_EVENT_KEY;
        out_ev->as.key = ke;
        pb_buf_consume(in, 3);
        return 1;
    }

    size_t i = 2;
    int p[4] = {0,0,0,0};
    int pc = 0;

    while(i < in->len && pc < 4){
        int v = 0;
        if(!pb_parse_num(s, in->len, &i, &v)) break;
        p[pc++] = v;
        if(i < in->len && s[i] == ';'){
            i++;
            continue;
        }
        break;
    }

    if(i >= in->len) return 0;

    uint8_t fin = s[i];

    if(fin == 'A' || fin == 'B' || fin == 'C' || fin == 'D' || fin == 'H' || fin == 'F'){
        pb_key k = PB_KEY_NONE;
        if(fin == 'A') k = PB_KEY_UP;
        if(fin == 'B') k = PB_KEY_DOWN;
        if(fin == 'C') k = PB_KEY_RIGHT;
        if(fin == 'D') k = PB_KEY_LEFT;
        if(fin == 'H') k = PB_KEY_HOME;
        if(fin == 'F') k = PB_KEY_END;

        pb_key_event ke;
        memset(&ke, 0, sizeof(ke));
        ke.key = k;
        ke.pressed = 1;

        int mod = 1;
        if(pc >= 2 && p[0] == 1) mod = p[1];
        pb_apply_mods(mod, &ke);

        out_ev->type = PB_EVENT_KEY;
        out_ev->as.key = ke;
        pb_buf_consume(in, i + 1);
        return 1;
    }

    if(fin == '~'){
        pb_key k = PB_KEY_NONE;
        int code = (pc >= 1) ? p[0] : 0;

        switch(code){
            case 1: case 7: k = PB_KEY_HOME; break;
            case 4: case 8: k = PB_KEY_END; break;
            case 2: k = PB_KEY_INS; break;
            case 3: k = PB_KEY_DEL; break;
            case 5: k = PB_KEY_PGUP; break;
            case 6: k = PB_KEY_PGDN; break;
            case 11: k = PB_KEY_F1; break;
            case 12: k = PB_KEY_F2; break;
            case 13: k = PB_KEY_F3; break;
            case 14: k = PB_KEY_F4; break;
            case 15: k = PB_KEY_F5; break;
            case 17: k = PB_KEY_F6; break;
            case 18: k = PB_KEY_F7; break;
            case 19: k = PB_KEY_F8; break;
            case 20: k = PB_KEY_F9; break;
            case 21: k = PB_KEY_F10; break;
            case 23: k = PB_KEY_F11; break;
            case 24: k = PB_KEY_F12; break;
            default: break;
        }

        if(k != PB_KEY_NONE){
            pb_key_event ke;
            memset(&ke, 0, sizeof(ke));
            ke.key = k;
            ke.pressed = 1;

            int mod = 1;
            if(pc >= 2) mod = p[1];
            pb_apply_mods(mod, &ke);

            out_ev->type = PB_EVENT_KEY;
            out_ev->as.key = ke;
            pb_buf_consume(in, i + 1);
            return 1;
        }
    }

    return 0;
}

static int pb_try_text(pb_input* in, pb_event* out_ev){
    if(in->len == 0) return 0;

    uint8_t c = in->buf[0];

    if(c == 0x1B) return 0;

    if(c == 0x7F || c == 0x08){
        pb_key_event ke;
        memset(&ke, 0, sizeof(ke));
        ke.key = PB_KEY_BACKSPACE;
        ke.pressed = 1;
        out_ev->type = PB_EVENT_KEY;
        out_ev->as.key = ke;
        pb_buf_consume(in, 1);
        return 1;
    }

    if(c == '\r' || c == '\n'){
        pb_key_event ke;
        memset(&ke, 0, sizeof(ke));
        ke.key = PB_KEY_ENTER;
        ke.pressed = 1;
        out_ev->type = PB_EVENT_KEY;
        out_ev->as.key = ke;
        pb_buf_consume(in, 1);
        return 1;
    }

    if(c == '\t'){
        pb_key_event ke;
        memset(&ke, 0, sizeof(ke));
        ke.key = PB_KEY_TAB;
        ke.pressed = 1;
        out_ev->type = PB_EVENT_KEY;
        out_ev->as.key = ke;
        pb_buf_consume(in, 1);
        return 1;
    }

    if(c >= 0x01u && c <= 0x1Au){
        uint32_t letter = (uint32_t)('a' + (c - 1));
        pb_key_event ke;
        memset(&ke, 0, sizeof(ke));
        ke.key = PB_KEY_NONE;
        ke.codepoint = letter;
        ke.ctrl = 1;
        ke.pressed = 1;
        out_ev->type = PB_EVENT_KEY;
        out_ev->as.key = ke;
        pb_buf_consume(in, 1);
        return 1;
    }

    uint32_t cp = 0;
    size_t adv = 0;
    if(!pb_utf8_decode(in->buf, in->len, &cp, &adv)) return 0;

    if(cp >= 32u){
        pb_key_event ke;
        memset(&ke, 0, sizeof(ke));
        ke.key = PB_KEY_NONE;
        ke.codepoint = cp;
        ke.pressed = 1;
        out_ev->type = PB_EVENT_KEY;
        out_ev->as.key = ke;
        pb_buf_consume(in, adv);
        return 1;
    }

    pb_buf_consume(in, adv);
    return 0;
}

int pb_input_poll(pb_input* in, pb_event* out_ev){
    if(!in || !out_ev || !in->term) return 0;

    if(pb_term_poll_resize(in->term)){
        out_ev->type = PB_EVENT_RESIZE;
        int w=0,h=0;
        pb_term_get_size(in->term, &w, &h);
        out_ev->as.resize.width = w;
        out_ev->as.resize.height = h;
        return 1;
    }

    uint8_t tmp[1024];
    int n = pb_term_read(in->term, tmp, (int)sizeof(tmp));
    if(n > 0){
        size_t room = sizeof(in->buf) - in->len;
        size_t take = (size_t)n;
        if(take > room){
            pb_input_flush(in);
            room = sizeof(in->buf);
            take = (size_t)n;
            if(take > room) take = room;
        }
        memcpy(in->buf + in->len, tmp, take);
        in->len += take;
    }

    out_ev->type = PB_EVENT_NONE;
    if(in->len == 0) return 0;

    uint64_t now = pb_time_ns();

    if(in->esc_pending){
        if(in->len == 1 && in->buf[0] == 0x1B){
            if(now - in->esc_since < 25000000ull) return 0;
            pb_key_event ke;
            memset(&ke, 0, sizeof(ke));
            ke.key = PB_KEY_ESC;
            ke.pressed = 1;
            out_ev->type = PB_EVENT_KEY;
            out_ev->as.key = ke;
            pb_buf_consume(in, 1);
            in->esc_pending = 0;
            return 1;
        }
        in->esc_pending = 0;
    }

    if(pb_try_mouse(in, out_ev)) return 1;
    if(pb_try_escape(in, out_ev)) return 1;

    if(in->len == 1 && in->buf[0] == 0x1B){
        in->esc_pending = 1;
        in->esc_since = now;
        return 0;
    }

    if(pb_try_text(in, out_ev)) return 1;

    return 0;
}
