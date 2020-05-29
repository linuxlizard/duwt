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

