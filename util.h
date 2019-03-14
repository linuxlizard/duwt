/* this code based originally on 'iw' iw.h which has the following COPYING */
/* 
 * Copyright (c) 2007, 2008        Johannes Berg
 * Copyright (c) 2007              Andy Lutomirski
 * Copyright (c) 2007              Mike Kershaw
 * Copyright (c) 2008-2009         Luis R. Rodriguez
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef UTIL_H
#define UTIL_H

/* from iw 5.0.1 iw.h */
#define BIT(x) (1ULL<<(x))

struct chanmode {
	const char *name;
	unsigned int width;
	int freq1_diff;
	int chantype; /* for older kernel */
};


struct chandef {
	enum nl80211_chan_width width;

	unsigned int control_freq;
	unsigned int center_freq1;
	unsigned int center_freq2;
};

#define DIV_ROUND_UP(x, y) (((x) + (y - 1)) / (y))
#define ARRAY_SIZE(ar) (sizeof(ar)/sizeof(ar[0]))

#ifdef __cplusplus
extern "C" {
#endif

int mac_addr_a2n(unsigned char *mac_addr, char *arg);
void mac_addr_n2a(char *mac_addr, const unsigned char *arg);
int parse_hex_mask(char *hexmask, unsigned char **result, size_t *result_len,
		   unsigned char **mask);
unsigned char *parse_hex(char *hex, size_t *outlen);

void iw_hexdump(const char *prefix, const __u8 *buf, size_t size);

int get_cf1(const struct chanmode *chanmode, unsigned long freq);

void hex_dump( const char *title, unsigned char *ptr, int size );

#ifdef __cplusplus
}
#endif
#endif

