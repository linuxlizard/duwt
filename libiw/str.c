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

#include "str.h"

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

int str_escape(char* src, size_t src_len, char* dst_str, size_t dst_len)
{
	int i;
	char* ptr = dst_str;
	char* endptr = dst_str + dst_len;
	static char hex_ascii[] = "0123456789abcdef";

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
	return ptr - dst_str;
}

