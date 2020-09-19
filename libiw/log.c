/*
 * libiw/log.c   general logging functionality
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <stdarg.h>

#include "log.h"

static int log_level = LOG_LEVEL_INFO;

void log_set_level(int level)
{
	log_level = level;
}

void logmsg(int level, const char* fmt, ...)
{
	va_list arg;

	if (level > log_level) {
		return;
	}

	va_start(arg, fmt);
	if (level < LOG_LEVEL_INFO) {
		vfprintf(stderr, fmt, arg);
	}
	else { 
		vfprintf(stdout, fmt, arg);
	}

	va_end(arg);
}
