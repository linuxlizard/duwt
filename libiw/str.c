/*
 * libiw/str.h  simple useful string fns
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>

#include "str.h"
#include "xassert.h"

static const char hex_ascii[] = "0123456789abcdef";

int str_join_uint8(char* str, size_t len, const uint8_t num_array[], size_t num_len)
{
	char* ptr;
	ssize_t remain = (ssize_t)len;
	int ret, total;

	memset(str, 0, len);
	total = 0;
	ptr = str;

	for (size_t i=0 ; i<num_len ; i++) {
		ret = snprintf(ptr, remain, "%d ", num_array[i] );
		if (ret < 0 ) {
			return -EIO;
		}

		ptr += ret;
		total += ret;
		remain -= ret;
		if (remain <= 0) {
			return -ENOMEM;
		}
	}

	return total;
}

int str_escape(const char* src, size_t src_len, char* dst, size_t dst_len)
{
	size_t i;
	char* ptr = dst;
	char* endptr = dst + dst_len;

	for (i=0 ; i<src_len ; i++) {
		// if not the escape symbol
		// and if printable
		if (src[i] != '\\' && (src[i] >= ' ' && src[i] <= '~')) {
			if (ptr+1 >= endptr) {
				return -ENOMEM;
			}

			*ptr++ = src[i];
		}
		else {
			// store escaped
			if (ptr+4 >= endptr) {
				return -ENOMEM;
			}
			*ptr++ = '\\';
			*ptr++ = 'x';
			*ptr++ = hex_ascii[ (uint8_t)src[i] >> 4 ];
			*ptr++ = hex_ascii[ (uint8_t)src[i] & 0x0f ];
		}

	}

	// return finished length
	return ptr - dst;
}

// write a hexdump of bytes in 'src' into 'dst'
// Return the length written.
// Will always null terminate.  Will return -ENOMEM if not enough space for destination.
int str_hexdump(const unsigned char* src, size_t src_len, char* dst, size_t dst_len)
{
	size_t i;
	char* ptr = dst;
	char* endptr = dst + dst_len;
	uint8_t* srcptr = (uint8_t*)src;

	for (i=0 ; i<src_len ; i++) {
		// checks for equality so we have enough space left for terminating NULL
		if (ptr+2 >= endptr) {
			return -ENOMEM;
		}
		*ptr++ = hex_ascii[*srcptr >> 4];
		*ptr++ = hex_ascii[*srcptr & 0x0f];
		srcptr++;
	}
	// lastly we null terminate (note we don't advance the ptr)
	*ptr = (char)0;

	return ptr - dst;
}

bool str_match(const char* s1, size_t s1_len, const char* s2, size_t s2_len)
{
	if (!s1 && !s2) {
		return true;
	}
	if (!s1 || !s2) {
		return false;
	}
	if (s1_len != s2_len) {
		return false;
	}
	return memcmp(s1,s2,s1_len) == 0;
}

enum splitstate {
	SPLITTING = 1,
	CONSUMING = 2
};

// split a string based on all chars in 'spliton'
// start of each string put into ptrlist[]
// NULL written src where any chars in spliton exist.
int str_split(char* src, size_t src_len, const char* spliton, char* ptrlist[], size_t* ptrlist_len)
{
	enum splitstate state = SPLITTING;
	size_t idx = 0;
	char* end_str = src + src_len;

	while (src < end_str && idx < *ptrlist_len) {
		if (state == SPLITTING) {
			if (strchr(spliton, *src)) {
				// found a split char
				*src = 0;
			}
			else {
				// save the new string we found
				ptrlist[idx++] = src;
				state = CONSUMING;
			}

		}
		else {
			XASSERT(state == CONSUMING, state);
			if (strchr(spliton, *src)) {
				// found a new split point
				*src = 0;
				state = SPLITTING;
			}
		}
		src++;
	}

	if (idx==*ptrlist_len) {
		// ran out of room
		return -ENOMEM;
	}

	// set number of strings found
	*ptrlist_len = idx;

	return 0;
}

