
#ifndef PLAYBOX_PB_UTF8_H
#define PLAYBOX_PB_UTF8_H

#include <stdint.h>
#include <stddef.h>

size_t pb_utf8_encode(uint32_t cp, char out[8]);
int pb_utf8_decode(const uint8_t* s, size_t n, uint32_t* out_cp, size_t* out_len);

#endif
