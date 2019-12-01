#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "xassert.h"
#include "log.h"

void xassert_fail(const char *expr, const char *file, int line, uintmax_t value)
{
	ERR("XASSET fail: %s:%d \"%s\" value=%#" PRIxMAX "\n", 
			file, line, expr, value);
	abort();
}

