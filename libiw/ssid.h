/*
 * libiw/ssid.h  handle SSID
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef SSID_H
#define SSID_H

#include <stdio.h>

// Standard says 32 octets but Unicode seriously muddies the water.
// 32 characters? 32-bytes? 32 octets implies 32 bytes, not 32 human language
// characters.
#define SSID_MAX_LEN 32

#ifdef __cplusplus
extern "C" {
#endif

struct BSS;

// this string used in output to indicate a hidden ssid
#define HIDDEN_SSID "<hidden>"

int ssid_utf8_validate(const char* ssid, size_t ssid_len);

int ssid_print(const struct BSS* bss, FILE* outfile, size_t width, const char* extra_str);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif

