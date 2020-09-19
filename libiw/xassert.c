/*
 * libiw/xassert.h   embraced and extended assert 
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "xassert.h"
#include "log.h"

void xassert_fail(const char *expr, const char *file, int line, uintmax_t value)
{
	ERR("XASSERT fail: %s:%d \"%s\" value=%#" PRIxMAX "\n", 
			file, line, expr, value);
	fflush(stdout);
	fflush(stderr);
	abort();
}

