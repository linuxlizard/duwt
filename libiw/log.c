#include <stdio.h>
#include <stdarg.h>

#include "log.h"

void logmsg(int level, const char* fmt, ...)
{
	va_list arg;

	(void)level;  // TODO

	va_start(arg, fmt);
	vfprintf(stdout, fmt, arg);
	va_end(arg);
}
