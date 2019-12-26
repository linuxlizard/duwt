#include <stdio.h>
#include <stdarg.h>

#include "log.h"

void logmsg(int level, const char* fmt, ...)
{
	va_list arg;

	(void)level;  // TODO

	va_start(arg, fmt);
	if (level < LOG_LEVEL_INFO) {
		vfprintf(stderr, fmt, arg);
	}
	else { 
		vfprintf(stdout, fmt, arg);
	}

	va_end(arg);
}
