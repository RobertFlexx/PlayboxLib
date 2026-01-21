
#include "pb_utf8.h"

size_t pb_utf8_encode(uint32_t cp, char out[8]){
    if(cp <= 0x7Fu){
        out[0] = (char)cp;
        return 1;
    }
    if(cp <= 0x7FFu){
        out[0] = (char)(0xC0u | ((cp >> 6) & 0x1Fu));
        out[1] = (char)(0x80u | (cp & 0x3Fu));
        return 2;
    }
    if(cp <= 0xFFFFu){
        out[0] = (char)(0xE0u | ((cp >> 12) & 0x0Fu));
        out[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = (char)(0x80u | (cp & 0x3Fu));
        return 3;
    }
    if(cp <= 0x10FFFFu){
        out[0] = (char)(0xF0u | ((cp >> 18) & 0x07u));
        out[1] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
        out[2] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[3] = (char)(0x80u | (cp & 0x3Fu));
        return 4;
    }
    out[0] = '?';
    return 1;
}

int pb_utf8_decode(const uint8_t* s, size_t n, uint32_t* out_cp, size_t* out_len){
    if(!s || n == 0) return 0;
    uint8_t c0 = s[0];
    if(c0 < 0x80u){
        *out_cp = c0;
        *out_len = 1;
        return 1;
    }
    if((c0 & 0xE0u) == 0xC0u){
        if(n < 2) return 0;
        uint8_t c1 = s[1];
        if((c1 & 0xC0u) != 0x80u) return 0;
        uint32_t cp = ((uint32_t)(c0 & 0x1Fu) << 6) | (uint32_t)(c1 & 0x3Fu);
        if(cp < 0x80u) return 0;
        *out_cp = cp;
        *out_len = 2;
        return 1;
    }
    if((c0 & 0xF0u) == 0xE0u){
        if(n < 3) return 0;
        uint8_t c1 = s[1], c2 = s[2];
        if((c1 & 0xC0u) != 0x80u || (c2 & 0xC0u) != 0x80u) return 0;
        uint32_t cp = ((uint32_t)(c0 & 0x0Fu) << 12) | ((uint32_t)(c1 & 0x3Fu) << 6) | (uint32_t)(c2 & 0x3Fu);
        if(cp < 0x800u) return 0;
        if(cp >= 0xD800u && cp <= 0xDFFFu) return 0;
        *out_cp = cp;
        *out_len = 3;
        return 1;
    }
    if((c0 & 0xF8u) == 0xF0u){
        if(n < 4) return 0;
        uint8_t c1 = s[1], c2 = s[2], c3 = s[3];
        if((c1 & 0xC0u) != 0x80u || (c2 & 0xC0u) != 0x80u || (c3 & 0xC0u) != 0x80u) return 0;
        uint32_t cp = ((uint32_t)(c0 & 0x07u) << 18) | ((uint32_t)(c1 & 0x3Fu) << 12) | ((uint32_t)(c2 & 0x3Fu) << 6) | (uint32_t)(c3 & 0x3Fu);
        if(cp < 0x10000u || cp > 0x10FFFFu) return 0;
        *out_cp = cp;
        *out_len = 4;
        return 1;
    }
    return 0;
}
