/*
 * libiw/str.h  simple useful string fns
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef STR_H
#define STR_h

#include <stdint.h>
#include <stdbool.h>

bool str_match(const char* s1, size_t s1_len, const char* s2, size_t s2_len);
int str_join_uint8(char* str, size_t len, const uint8_t num_array[], size_t num_len);
int str_escape(const char* src, size_t src_len, char* dst, size_t dst_len);
int str_hexdump(const unsigned char* src, size_t src_len, char* dst, size_t dst_len);

#endif

