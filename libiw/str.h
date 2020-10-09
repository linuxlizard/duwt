/*
 * libiw/str.h  simple useful string fns
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef STR_H
#define STR_h

#include <stdint.h>

int str_join_uint8(char* str, size_t len, const uint8_t num_array[], size_t num_len);
int str_escape(char* src, size_t src_len, char* dst_str, size_t dst_len);


#endif

