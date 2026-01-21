#include "playbox/pb.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define PBR_MAGIC1 "PBR1"
#define PBR_MAGIC2 "PBR2"

struct pb_replay {
    FILE* f;
    int mode;
    uint32_t seed;
    uint32_t initial_w;
    uint32_t initial_h;
    int has_meta;
};

static int write_u32(FILE* f, uint32_t v) {
    uint8_t b[4];
    b[0] = (uint8_t)(v & 0xFFu);
    b[1] = (uint8_t)((v >> 8) & 0xFFu);
    b[2] = (uint8_t)((v >> 16) & 0xFFu);
    b[3] = (uint8_t)((v >> 24) & 0xFFu);
    return fwrite(b, 1, 4, f) == 4;
}

static int write_i32(FILE* f, int32_t v) {
    return write_u32(f, (uint32_t)v);
}

static int write_f64(FILE* f, double v) {
    uint64_t u = 0;
    memcpy(&u, &v, sizeof(u));
    uint8_t b[8];
    b[0] = (uint8_t)(u & 0xFFu);
    b[1] = (uint8_t)((u >> 8) & 0xFFu);
    b[2] = (uint8_t)((u >> 16) & 0xFFu);
    b[3] = (uint8_t)((u >> 24) & 0xFFu);
    b[4] = (uint8_t)((u >> 32) & 0xFFu);
    b[5] = (uint8_t)((u >> 40) & 0xFFu);
    b[6] = (uint8_t)((u >> 48) & 0xFFu);
    b[7] = (uint8_t)((u >> 56) & 0xFFu);
    return fwrite(b, 1, 8, f) == 8;
}

static int read_u32(FILE* f, uint32_t* out) {
    uint8_t b[4];
    if (fread(b, 1, 4, f) != 4) return 0;
    *out = (uint32_t)b[0]
    | ((uint32_t)b[1] << 8)
    | ((uint32_t)b[2] << 16)
    | ((uint32_t)b[3] << 24);
    return 1;
}

static int read_i32(FILE* f, int32_t* out) {
    uint32_t u = 0;
    if (!read_u32(f, &u)) return 0;
    *out = (int32_t)u;
    return 1;
}

static int read_f64(FILE* f, double* out) {
    uint8_t b[8];
    if (fread(b, 1, 8, f) != 8) return 0;
    uint64_t u = 0;
    u |= (uint64_t)b[0];
    u |= (uint64_t)b[1] << 8;
    u |= (uint64_t)b[2] << 16;
    u |= (uint64_t)b[3] << 24;
    u |= (uint64_t)b[4] << 32;
    u |= (uint64_t)b[5] << 40;
    u |= (uint64_t)b[6] << 48;
    u |= (uint64_t)b[7] << 56;
    memcpy(out, &u, sizeof(u));
    return 1;
}

static int write_event(FILE* f, const pb_event* ev) {
    if (!write_u32(f, (uint32_t)ev->type)) return 0;

    switch (ev->type) {
        case PB_EVENT_NONE:
        case PB_EVENT_QUIT:
            return 1;

        case PB_EVENT_TEXT:
            return write_u32(f, (uint32_t)ev->as.text);

        case PB_EVENT_KEY:
            if (!write_u32(f, (uint32_t)ev->as.key.key)) return 0;
            if (!write_u32(f, (uint32_t)ev->as.key.codepoint)) return 0;
            if (!write_u32(f, (uint32_t)ev->as.key.alt)) return 0;
            if (!write_u32(f, (uint32_t)ev->as.key.ctrl)) return 0;
            if (!write_u32(f, (uint32_t)ev->as.key.shift)) return 0;
            if (!write_u32(f, (uint32_t)ev->as.key.pressed)) return 0;
            return 1;

        case PB_EVENT_MOUSE:
            if (!write_i32(f, (int32_t)ev->as.mouse.x)) return 0;
            if (!write_i32(f, (int32_t)ev->as.mouse.y)) return 0;
            if (!write_u32(f, (uint32_t)ev->as.mouse.button)) return 0;
            if (!write_u32(f, (uint32_t)ev->as.mouse.pressed)) return 0;
            if (!write_i32(f, (int32_t)ev->as.mouse.wheel)) return 0;
            if (!write_u32(f, (uint32_t)ev->as.mouse.shift)) return 0;
            if (!write_u32(f, (uint32_t)ev->as.mouse.alt)) return 0;
            if (!write_u32(f, (uint32_t)ev->as.mouse.ctrl)) return 0;
            return 1;

        case PB_EVENT_RESIZE:
            if (!write_i32(f, (int32_t)ev->as.resize.width)) return 0;
            if (!write_i32(f, (int32_t)ev->as.resize.height)) return 0;
            return 1;

        default:
            return 0;
    }
}

static int read_event(FILE* f, pb_event* ev) {
    uint32_t t = 0;
    if (!read_u32(f, &t)) return 0;
    ev->type = (pb_event_type)t;

    uint32_t u = 0;
    int32_t i = 0;

    switch (ev->type) {
        case PB_EVENT_NONE:
        case PB_EVENT_QUIT:
            return 1;

        case PB_EVENT_TEXT:
            if (!read_u32(f, &u)) return 0;
            ev->as.text = u;
        return 1;

        case PB_EVENT_KEY:
            if (!read_u32(f, &u)) return 0;
            ev->as.key.key = (pb_key)u;

        if (!read_u32(f, &u)) return 0;
        ev->as.key.codepoint = u;

        if (!read_u32(f, &u)) return 0;
        ev->as.key.alt = (uint8_t)u;

        if (!read_u32(f, &u)) return 0;
        ev->as.key.ctrl = (uint8_t)u;

        if (!read_u32(f, &u)) return 0;
        ev->as.key.shift = (uint8_t)u;

        if (!read_u32(f, &u)) return 0;
        ev->as.key.pressed = (uint8_t)u;

        return 1;

        case PB_EVENT_MOUSE:
            if (!read_i32(f, &i)) return 0;
            ev->as.mouse.x = (int)i;

        if (!read_i32(f, &i)) return 0;
        ev->as.mouse.y = (int)i;

        if (!read_u32(f, &u)) return 0;
        ev->as.mouse.button = (uint8_t)u;

        if (!read_u32(f, &u)) return 0;
        ev->as.mouse.pressed = (uint8_t)u;

        if (!read_i32(f, &i)) return 0;
        ev->as.mouse.wheel = (int)i;

        if (!read_u32(f, &u)) return 0;
        ev->as.mouse.shift = (uint8_t)u;

        if (!read_u32(f, &u)) return 0;
        ev->as.mouse.alt = (uint8_t)u;

        if (!read_u32(f, &u)) return 0;
        ev->as.mouse.ctrl = (uint8_t)u;

        return 1;

        case PB_EVENT_RESIZE:
            if (!read_i32(f, &i)) return 0;
            ev->as.resize.width = (int)i;

        if (!read_i32(f, &i)) return 0;
        ev->as.resize.height = (int)i;

        return 1;

        default:
            return 0;
    }
}

pb_replay* pb_replay_open_record(const char* path) {
    if (!path || !path[0]) return NULL;

    FILE* f = fopen(path, "wb");
    if (!f) return NULL;

    if (fwrite(PBR_MAGIC2, 1, 4, f) != 4) {
        fclose(f);
        return NULL;
    }

    pb_replay* r = (pb_replay*)malloc(sizeof(pb_replay));
    if (!r) {
        fclose(f);
        return NULL;
    }

    r->f = f;
    r->mode = 1;
    r->seed = 0;
    r->initial_w = 0;
    r->initial_h = 0;
    r->has_meta = 0;

    if (!write_u32(f, 0)) { free(r); fclose(f); return NULL; }
    if (!write_u32(f, 0)) { free(r); fclose(f); return NULL; }
    if (!write_u32(f, 0)) { free(r); fclose(f); return NULL; }

    return r;
}

pb_replay* pb_replay_open_replay(const char* path) {
    if (!path || !path[0]) return NULL;

    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    char magic[4];
    if (fread(magic, 1, 4, f) != 4) {
        fclose(f);
        return NULL;
    }

    pb_replay* r = (pb_replay*)malloc(sizeof(pb_replay));
    if (!r) {
        fclose(f);
        return NULL;
    }

    r->f = f;
    r->mode = 2;
    r->seed = 0;
    r->initial_w = 0;
    r->initial_h = 0;
    r->has_meta = 0;

    if (memcmp(magic, PBR_MAGIC2, 4) == 0) {
        uint32_t seed = 0, w = 0, h = 0;
        if (!read_u32(f, &seed) || !read_u32(f, &w) || !read_u32(f, &h)) {
            free(r);
            fclose(f);
            return NULL;
        }
        r->seed = seed;
        r->initial_w = w;
        r->initial_h = h;
        r->has_meta = 1;
        return r;
    }

    if (memcmp(magic, PBR_MAGIC1, 4) == 0) {
        r->has_meta = 0;
        return r;
    }

    free(r);
    fclose(f);
    return NULL;
}

void pb_replay_close(pb_replay* r) {
    if (!r) return;
    if (r->f) fclose(r->f);
    free(r);
}

uint32_t pb_replay_get_seed(const pb_replay* r) {
    return r ? r->seed : 0u;
}

void pb_replay_get_initial_size(const pb_replay* r, uint32_t* out_w, uint32_t* out_h) {
    if (out_w) *out_w = r ? r->initial_w : 0u;
    if (out_h) *out_h = r ? r->initial_h : 0u;
}

int pb_replay_record_set_meta(pb_replay* r, uint32_t seed, uint32_t initial_w, uint32_t initial_h) {
    if (!r || !r->f || r->mode != 1) return 0;

    r->seed = seed;
    r->initial_w = initial_w;
    r->initial_h = initial_h;
    r->has_meta = 1;

    long pos = ftell(r->f);
    if (pos < 0) return 0;

    if (fseek(r->f, 4, SEEK_SET) != 0) return 0;
    if (!write_u32(r->f, r->seed)) return 0;
    if (!write_u32(r->f, r->initial_w)) return 0;
    if (!write_u32(r->f, r->initial_h)) return 0;

    if (fseek(r->f, pos, SEEK_SET) != 0) return 0;
    return 1;
}

int pb_replay_write_frame(pb_replay* r, double dt, int event_count) {
    if (!r || !r->f || r->mode != 1) return 0;
    if (event_count < 0) return 0;

    if (!write_f64(r->f, dt)) return 0;
    if (!write_u32(r->f, (uint32_t)event_count)) return 0;
    return 1;
}

int pb_replay_write_event(pb_replay* r, const pb_event* ev) {
    if (!r || !r->f || r->mode != 1) return 0;
    if (!ev) return 0;
    return write_event(r->f, ev);
}

int pb_replay_read_frame(pb_replay* r, double* out_dt, int* out_event_count) {
    if (!r || !r->f || r->mode != 2) return 0;
    if (!out_dt || !out_event_count) return 0;

    double dt = 0.0;
    uint32_t n = 0;

    if (!read_f64(r->f, &dt)) return 0;
    if (!read_u32(r->f, &n)) return 0;

    *out_dt = dt;
    *out_event_count = (int)n;
    return 1;
}

int pb_replay_read_event(pb_replay* r, pb_event* out_ev) {
    if (!r || !r->f || r->mode != 2) return 0;
    if (!out_ev) return 0;
    return read_event(r->f, out_ev);
}
