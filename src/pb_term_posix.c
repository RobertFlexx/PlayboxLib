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
#include <time.h>

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
    tio.c_iflag |= ICRNL;
    tio.c_oflag |= OPOST;
    if(tcsetattr(t->in_fd, TCSAFLUSH, &tio) != 0) return 0;
    t->raw = 1;
    return 1;
}

static void pb_restore(pb_term* t){
    if(!t || !t->raw) return;
    tcsetattr(t->in_fd, TCSAFLUSH, &t->orig);
    t->raw = 0;
}

static int pb_write_all(int fd, const char* s, int n){
    if(n <= 0) return 0;
    const char* p = s;
    int left = n;
    while(left > 0){
        ssize_t w = write(fd, p, (size_t)left);
        if(w > 0){
            p += w;
            left -= (int)w;
            continue;
        }
        if(w < 0 && errno == EINTR) continue;
        if(w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)){
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 1000000L;
            nanosleep(&ts, NULL);
            continue;
        }
        return 0;
    }
    return 1;
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
    pb_write_all(t->out_fd, seq, (int)strlen(seq));
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
    pb_write_all(t->out_fd, seq, (int)strlen(seq));

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
    ssize_t n = read(t->in_fd, buf, (size_t)cap);
    if(n <= 0) return 0;
    return (int)n;
}

int pb_term_write(pb_term* t, const char* s, int n){
    if(!t || !s || n <= 0) return 0;
    return pb_write_all(t->out_fd, s, n) ? n : 0;
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
