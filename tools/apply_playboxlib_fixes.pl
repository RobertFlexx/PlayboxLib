use strict;
use warnings;
use File::Path qw(make_path);

sub write_file {
    my ($path, $content) = @_;
    my $dir = $path;
    $dir =~ s{/[^/]+$}{};
    make_path($dir) if $dir ne '' && !-d $dir;
    open my $fh, ">", $path or die "open($path): $!";
    binmode($fh);
    print $fh $content;
    close $fh or die "close($path): $!";
    print "wrote $path\n";
}

my $pb_input_c = <<'C';
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
    uint64_t esc_armed_ns;
};

pb_input* pb_input_create(void){
    pb_input* in = (pb_input*)calloc(1, sizeof(pb_input));
    if(!in) return NULL;
    in->term = NULL;
    in->len = 0;
    in->esc_armed_ns = 0;
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
    in->esc_armed_ns = 0;
}

static void pb_buf_consume(pb_input* in, size_t n){
    if(n >= in->len){
        in->len = 0;
        return;
    }
    memmove(in->buf, in->buf + n, in->len - n);
    in->len -= n;
}

static int pb_parse_csi_number(const uint8_t* s, size_t n, size_t* io_i, int* out_num){
    int v = 0;
    size_t i = *io_i;
    int any = 0;
    while(i < n && s[i] >= '0' && s[i] <= '9'){
        any = 1;
        v = v*10 + (int)(s[i] - '0');
        i++;
    }
    if(!any) return 0;
    *out_num = v;
    *io_i = i;
    return 1;
}

static int pb_try_mouse(pb_input* in, pb_event* out_ev){
    if(in->len < 6) return 0;
    const uint8_t* s = in->buf;
    if(s[0] != 0x1B || s[1] != '[' || s[2] != '<') return 0;

    size_t i = 3;
    int b=0, x=0, y=0;
    if(!pb_parse_csi_number(s, in->len, &i, &b)) return 0;
    if(i >= in->len || s[i] != ';') return 0;
    i++;
    if(!pb_parse_csi_number(s, in->len, &i, &x)) return 0;
    if(i >= in->len || s[i] != ';') return 0;
    i++;
    if(!pb_parse_csi_number(s, in->len, &i, &y)) return 0;
    if(i >= in->len) return 0;
    uint8_t fin = s[i];
    if(fin != 'M' && fin != 'm') return 0;
    i++;

    int pressed = (fin == 'M') ? 1 : 0;

    pb_mouse_event me;
    memset(&me, 0, sizeof(me));
    me.x = x - 1;
    me.y = y - 1;
    me.button = (uint8_t)(b & 0x3u);
    me.pressed = (uint8_t)pressed;

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
    in->esc_armed_ns = 0;
    return 1;
}

static int pb_try_escape_key(pb_input* in, pb_event* out_ev){
    if(in->len < 2) return 0;
    const uint8_t* s = in->buf;
    if(s[0] != 0x1B) return 0;

    if(s[1] != '[' && s[1] != 'O'){
        pb_key_event ke;
        memset(&ke, 0, sizeof(ke));
        ke.key = PB_KEY_NONE;
        ke.codepoint = (uint32_t)s[1];
        ke.alt = 1;
        ke.pressed = 1;
        out_ev->type = PB_EVENT_KEY;
        out_ev->as.key = ke;
        pb_buf_consume(in, 2);
        in->esc_armed_ns = 0;
        return 1;
    }

    if(s[1] == 'O'){
        if(in->len < 3) return 0;
        pb_key k = PB_KEY_NONE;
        switch(s[2]){
            case 'P': k = PB_KEY_F1; break;
            case 'Q': k = PB_KEY_F2; break;
            case 'R': k = PB_KEY_F3; break;
            case 'S': k = PB_KEY_F4; break;
            case 'H': k = PB_KEY_HOME; break;
            case 'F': k = PB_KEY_END; break;
            default: break;
        }
        if(k != PB_KEY_NONE){
            pb_key_event ke;
            memset(&ke, 0, sizeof(ke));
            ke.key = k;
            ke.pressed = 1;
            out_ev->type = PB_EVENT_KEY;
            out_ev->as.key = ke;
            pb_buf_consume(in, 3);
            in->esc_armed_ns = 0;
            return 1;
        }
        return 0;
    }

    if(in->len < 3) return 0;

    if(s[2] >= 'A' && s[2] <= 'D'){
        pb_key k = PB_KEY_NONE;
        if(s[2] == 'A') k = PB_KEY_UP;
        if(s[2] == 'B') k = PB_KEY_DOWN;
        if(s[2] == 'C') k = PB_KEY_RIGHT;
        if(s[2] == 'D') k = PB_KEY_LEFT;

        pb_key_event ke;
        memset(&ke, 0, sizeof(ke));
        ke.key = k;
        ke.pressed = 1;
        out_ev->type = PB_EVENT_KEY;
        out_ev->as.key = ke;
        pb_buf_consume(in, 3);
        in->esc_armed_ns = 0;
        return 1;
    }

    if(s[2] == 'H' || s[2] == 'F'){
        pb_key k = (s[2] == 'H') ? PB_KEY_HOME : PB_KEY_END;
        pb_key_event ke;
        memset(&ke, 0, sizeof(ke));
        ke.key = k;
        ke.pressed = 1;
        out_ev->type = PB_EVENT_KEY;
        out_ev->as.key = ke;
        pb_buf_consume(in, 3);
        in->esc_armed_ns = 0;
        return 1;
    }

    size_t i = 2;
    int p1 = 0;
    if(!pb_parse_csi_number(s, in->len, &i, &p1)) return 0;
    if(i >= in->len) return 0;

    if(s[i] == '~'){
        pb_key k = PB_KEY_NONE;
        switch(p1){
            case 1: k = PB_KEY_HOME; break;
            case 2: k = PB_KEY_INS; break;
            case 3: k = PB_KEY_DEL; break;
            case 4: k = PB_KEY_END; break;
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
            out_ev->type = PB_EVENT_KEY;
            out_ev->as.key = ke;
            pb_buf_consume(in, i+1);
            in->esc_armed_ns = 0;
            return 1;
        }
    }

    return 0;
}

static int pb_try_text(pb_input* in, pb_event* out_ev){
    if(in->len == 0) return 0;
    uint8_t c = in->buf[0];
    if(c == 0x1B) return 0;

    if(c == 0x7F){
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
    if(c <= 0x1Au && c >= 0x01u){
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
        out_ev->type = PB_EVENT_TEXT;
        out_ev->as.text = cp;
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
        in->esc_armed_ns = 0;
        return 1;
    }

    uint8_t tmp[1024];
    int n = pb_term_read(in->term, tmp, (int)sizeof(tmp));
    if(n > 0){
        size_t room = sizeof(in->buf) - in->len;
        size_t take = (size_t)n;
        if(take > room) take = room;
        memcpy(in->buf + in->len, tmp, take);
        in->len += take;
    }

    out_ev->type = PB_EVENT_NONE;

    if(in->len == 0){
        in->esc_armed_ns = 0;
        return 0;
    }

    if(in->buf[0] != 0x1B) in->esc_armed_ns = 0;

    if(pb_try_mouse(in, out_ev)) return 1;
    if(pb_try_escape_key(in, out_ev)) return 1;

    if(in->buf[0] == 0x1B){
        uint64_t now = pb_time_ns();
        if(in->len == 1){
            if(in->esc_armed_ns == 0){
                in->esc_armed_ns = now;
                return 0;
            }
            if(now - in->esc_armed_ns < 30000000ull){
                return 0;
            }
        }
        pb_key_event ke;
        memset(&ke, 0, sizeof(ke));
        ke.key = PB_KEY_ESC;
        ke.pressed = 1;
        out_ev->type = PB_EVENT_KEY;
        out_ev->as.key = ke;
        pb_buf_consume(in, 1);
        in->esc_armed_ns = 0;
        return 1;
    }

    if(pb_try_text(in, out_ev)) return 1;

    return 0;
}
C

my $pb_renderer_c = <<'C';
#include "playbox/pb_renderer.h"
#include "playbox/pb_term.h"
#include "pb_utf8.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct pb_renderer {
    pb_term* term;
    pb_fb back;
    pb_cell clear;
    int full;
    pb_color cur_fg;
    pb_color cur_bg;
    uint16_t cur_style;
};

static void sb_append(char** buf, size_t* len, size_t* cap, const char* s, size_t n){
    if(n == 0) return;
    if(*len + n + 1 > *cap){
        size_t nc = (*cap == 0) ? 8192 : *cap;
        while(nc < *len + n + 1) nc *= 2;
        char* nb = (char*)realloc(*buf, nc);
        if(!nb) return;
        *buf = nb;
        *cap = nc;
    }
    memcpy(*buf + *len, s, n);
    *len += n;
    (*buf)[*len] = '\0';
}

static void sb_append_cstr(char** buf, size_t* len, size_t* cap, const char* s){
    sb_append(buf, len, cap, s, strlen(s));
}

static void sb_append_int(char** buf, size_t* len, size_t* cap, int v){
    char tmp[64];
    int n = snprintf(tmp, sizeof(tmp), "%d", v);
    if(n > 0) sb_append(buf, len, cap, tmp, (size_t)n);
}

static int color_eq(pb_color a, pb_color b){
    return a.r==b.r && a.g==b.g && a.b==b.b;
}

static void emit_reset(char** buf, size_t* len, size_t* cap){
    sb_append_cstr(buf, len, cap, "\x1b[0m");
}

static void emit_style(char** buf, size_t* len, size_t* cap, uint16_t style){
    emit_reset(buf, len, cap);
    if(style & PB_STYLE_BOLD) sb_append_cstr(buf, len, cap, "\x1b[1m");
    if(style & PB_STYLE_DIM) sb_append_cstr(buf, len, cap, "\x1b[2m");
    if(style & PB_STYLE_UNDERLINE) sb_append_cstr(buf, len, cap, "\x1b[4m");
    if(style & PB_STYLE_REVERSE) sb_append_cstr(buf, len, cap, "\x1b[7m");
}

static void emit_fg(char** buf, size_t* len, size_t* cap, pb_color c){
    sb_append_cstr(buf, len, cap, "\x1b[38;2;");
    sb_append_int(buf, len, cap, (int)c.r);
    sb_append_cstr(buf, len, cap, ";");
    sb_append_int(buf, len, cap, (int)c.g);
    sb_append_cstr(buf, len, cap, ";");
    sb_append_int(buf, len, cap, (int)c.b);
    sb_append_cstr(buf, len, cap, "m");
}

static void emit_bg(char** buf, size_t* len, size_t* cap, pb_color c){
    sb_append_cstr(buf, len, cap, "\x1b[48;2;");
    sb_append_int(buf, len, cap, (int)c.r);
    sb_append_cstr(buf, len, cap, ";");
    sb_append_int(buf, len, cap, (int)c.g);
    sb_append_cstr(buf, len, cap, ";");
    sb_append_int(buf, len, cap, (int)c.b);
    sb_append_cstr(buf, len, cap, "m");
}

static void emit_move(char** buf, size_t* len, size_t* cap, int x, int y){
    sb_append_cstr(buf, len, cap, "\x1b[");
    sb_append_int(buf, len, cap, y);
    sb_append_cstr(buf, len, cap, ";");
    sb_append_int(buf, len, cap, x);
    sb_append_cstr(buf, len, cap, "H");
}

static int cell_eq(pb_cell a, pb_cell b){
    return a.ch == b.ch && a.style == b.style && color_eq(a.fg,b.fg) && color_eq(a.bg,b.bg);
}

pb_renderer* pb_renderer_create(void){
    pb_renderer* r = (pb_renderer*)calloc(1, sizeof(pb_renderer));
    if(!r) return NULL;
    r->term = NULL;
    r->back = pb_fb_make(0,0);
    r->clear = pb_cell_make(' ', pb_rgb(255,255,255), pb_rgb(0,0,0), 0);
    r->full = 1;
    r->cur_fg = pb_rgb(255,255,255);
    r->cur_bg = pb_rgb(0,0,0);
    r->cur_style = 0xFFFFu;
    return r;
}

void pb_renderer_destroy(pb_renderer* r){
    if(!r) return;
    pb_fb_free(&r->back);
    free(r);
}

int pb_renderer_bind(pb_renderer* r, void* term_handle){
    if(!r) return 0;
    r->term = (pb_term*)term_handle;
    r->full = 1;
    r->cur_style = 0xFFFFu;
    return r->term ? 1 : 0;
}

void pb_renderer_set_clear(pb_renderer* r, pb_cell clear_cell){
    if(!r) return;
    r->clear = clear_cell;
}

void pb_renderer_force_full_redraw(pb_renderer* r){
    if(!r) return;
    r->full = 1;
}

int pb_renderer_present(pb_renderer* r, const pb_fb* fb){
    if(!r || !r->term || !fb || !fb->cells) return 0;

    if(r->back.w != fb->w || r->back.h != fb->h){
        pb_fb_free(&r->back);
        r->back = pb_fb_make(fb->w, fb->h);
        pb_fb_clear(&r->back, r->clear);
        r->full = 1;
    }

    char* out = NULL;
    size_t olen = 0, ocap = 0;

    if(r->full){
        emit_reset(&out, &olen, &ocap);
        emit_bg(&out, &olen, &ocap, r->clear.bg);
        sb_append_cstr(&out, &olen, &ocap, "\x1b[2J\x1b[H");
        r->cur_style = 0xFFFFu;
        r->cur_fg = pb_rgb(255,255,255);
        r->cur_bg = pb_rgb(0,0,0);
        pb_fb_clear(&r->back, r->clear);
    }

    for(int y=0; y<fb->h; y++){
        int x = 0;
        while(x < fb->w){
            size_t idx = (size_t)y * (size_t)fb->w + (size_t)x;
            pb_cell c = fb->cells[idx];
            pb_cell b = r->back.cells[idx];
            if(!r->full && cell_eq(c,b)){
                x++;
                continue;
            }

            int run_start = x;
            int run_end = x + 1;
            pb_cell first = c;

            while(run_end < fb->w){
                size_t j = (size_t)y * (size_t)fb->w + (size_t)run_end;
                pb_cell cj = fb->cells[j];
                pb_cell bj = r->back.cells[j];
                if(!r->full && cell_eq(cj, bj)) break;
                if(cj.style != first.style || !color_eq(cj.fg, first.fg) || !color_eq(cj.bg, first.bg)) break;
                run_end++;
            }

            emit_move(&out, &olen, &ocap, run_start + 1, y + 1);

            if(r->cur_style != first.style){
                emit_style(&out, &olen, &ocap, first.style);
                r->cur_style = first.style;
                r->cur_fg = pb_rgb(255,255,255);
                r->cur_bg = pb_rgb(0,0,0);
            }

            if(!color_eq(r->cur_fg, first.fg)){
                emit_fg(&out, &olen, &ocap, first.fg);
                r->cur_fg = first.fg;
            }
            if(!color_eq(r->cur_bg, first.bg)){
                emit_bg(&out, &olen, &ocap, first.bg);
                r->cur_bg = first.bg;
            }

            for(int xx=run_start; xx<run_end; xx++){
                size_t k = (size_t)y * (size_t)fb->w + (size_t)xx;
                pb_cell ck = fb->cells[k];
                char utf8[8];
                size_t wn = pb_utf8_encode(ck.ch ? ck.ch : (uint32_t)' ', utf8);
                sb_append(&out, &olen, &ocap, utf8, wn);
                r->back.cells[k] = ck;
            }

            x = run_end;
        }
    }

    emit_reset(&out, &olen, &ocap);
    emit_bg(&out, &olen, &ocap, r->clear.bg);
    emit_fg(&out, &olen, &ocap, r->clear.fg);

    int wrote = pb_term_write(r->term, out ? out : "", (int)olen);
    free(out);

    r->full = (wrote > 0) ? 0 : 1;
    return wrote > 0 ? 1 : 0;
}
C

my $pb_term_c = <<'C';
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "playbox/pb_term.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>

struct pb_term {
    int in_fd;
    int out_fd;
    struct termios orig;
    int raw;
    int nonblocking;
    volatile sig_atomic_t resized;
};

static struct pb_term* g_term = NULL;

static void pb_winch_handler(int sig){
    (void)sig;
    if(g_term) g_term->resized = 1;
}

pb_term* pb_term_create(void){
    pb_term* t = (pb_term*)calloc(1, sizeof(pb_term));
    if(!t) return NULL;
    t->in_fd = 0;
    t->out_fd = 1;
    t->raw = 0;
    t->nonblocking = 0;
    t->resized = 0;
    return t;
}

void pb_term_destroy(pb_term* t){
    if(!t) return;
    free(t);
}

static int pb_set_raw(pb_term* t){
    if(!t) return 0;
    struct termios tio;
    if(tcgetattr(t->in_fd, &t->orig) != 0) return 0;
    tio = t->orig;
    cfmakeraw(&tio);
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;
    if(tcsetattr(t->in_fd, TCSAFLUSH, &tio) != 0) return 0;
    t->raw = 1;
    return 1;
}

static void pb_restore(pb_term* t){
    if(!t || !t->raw) return;
    tcsetattr(t->in_fd, TCSAFLUSH, &t->orig);
    t->raw = 0;
}

int pb_term_enter(pb_term* t){
    if(!t) return 0;
    g_term = t;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = pb_winch_handler;
    sigaction(SIGWINCH, &sa, NULL);

    if(!pb_set_raw(t)) return 0;
    pb_term_set_nonblocking(t, 1);

    const char* seq =
        "\x1b[?1049h"
        "\x1b[?25l"
        "\x1b[?7l"
        "\x1b[0m"
        "\x1b[2J"
        "\x1b[H";
    pb_term_write(t, seq, (int)strlen(seq));
    return 1;
}

void pb_term_leave(pb_term* t){
    if(!t) return;
    pb_term_enable_mouse(t, 0);
    const char* seq =
        "\x1b[0m"
        "\x1b[?7h"
        "\x1b[?25h"
        "\x1b[?1049l";
    pb_term_write(t, seq, (int)strlen(seq));
    pb_restore(t);
    pb_term_set_nonblocking(t, 0);
    if(g_term == t) g_term = NULL;
}

int pb_term_get_size(pb_term* t, int* out_w, int* out_h){
    if(!t) return 0;
    struct winsize ws;
    if(ioctl(t->out_fd, TIOCGWINSZ, &ws) != 0) return 0;
    if(out_w) *out_w = (int)ws.ws_col;
    if(out_h) *out_h = (int)ws.ws_row;
    return 1;
}

int pb_term_read(pb_term* t, uint8_t* buf, int cap){
    if(!t || !buf || cap <= 0) return 0;
    int n = (int)read(t->in_fd, buf, (size_t)cap);
    if(n > 0) return n;
    if(n < 0){
        if(errno == EAGAIN || errno == EWOULDBLOCK) return 0;
    }
    return 0;
}

int pb_term_write(pb_term* t, const char* s, int n){
    if(!t || !s || n <= 0) return 0;
    int w = (int)write(t->out_fd, s, (size_t)n);
    if(w < 0) return 0;
    return w;
}

void pb_term_set_nonblocking(pb_term* t, int enabled){
    if(!t) return;
    int fl = fcntl(t->in_fd, F_GETFL, 0);
    if(fl < 0) return;
    if(enabled) fl |= O_NONBLOCK;
    else fl &= ~O_NONBLOCK;
    fcntl(t->in_fd, F_SETFL, fl);
    t->nonblocking = enabled ? 1 : 0;
}

void pb_term_request_focus(pb_term* t){
    if(!t) return;
    const char* seq = "\x1b[?1004h";
    pb_term_write(t, seq, (int)strlen(seq));
}

void pb_term_enable_mouse(pb_term* t, int enabled){
    if(!t) return;
    const char* on =
        "\x1b[?1000h"
        "\x1b[?1002h"
        "\x1b[?1006h";
    const char* off =
        "\x1b[?1000l"
        "\x1b[?1002l"
        "\x1b[?1006l";
    const char* seq = enabled ? on : off;
    pb_term_write(t, seq, (int)strlen(seq));
}

int pb_term_poll_resize(pb_term* t){
    if(!t) return 0;
    if(t->resized){
        t->resized = 0;
        return 1;
    }
    return 0;
}
C

my @targets = (
    ["src/pb_input.c",    $pb_input_c],
    ["src/pb_renderer.c", $pb_renderer_c],
    ["src/pb_term.c",     $pb_term_c],
);

for my $t (@targets){
    write_file($t->[0], $t->[1]);
}

print "done\n";
