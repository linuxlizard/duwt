#include <stdio.h>
#include <stdarg.h>

#include "log.h"
#include "bug.h"
#include "xassert.h"

bool CHECK_DATA_CORRUPTION(bool condition, const char* fmt, ...)
{
	va_list arg;

//	DBG("%s %d %s", __func__, condition, fmt);
//	XASSERT(condition,condition);
	va_start(arg, fmt);
	bool corruption = (condition);
	if (corruption) {
		vfprintf(stderr, fmt, arg);
		abort();
	}
	va_end(arg);
	return corruption;
}


