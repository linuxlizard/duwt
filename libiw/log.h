/*
 * libiw/log.c   general logging functionality
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef LOG_H
#define LOG_H

#define LOG_LEVEL_ERR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_PLAID 99

#ifdef __cplusplus
extern "C" {
#endif

void log_set_level(int level);

void logmsg(int level, const char* fmt, ...) __attribute__((format (printf, 2,3)));

#ifdef __cplusplus
} // end extern "C"
#endif

#ifdef HAVE_MODULE_LOGLEVEL
// file level logging level so can change logging levels @ run-time
#define ERR(fmt, ...)\
	module_loglevel >= LOG_LEVEL_ERR ? logmsg(LOG_LEVEL_ERR, fmt, ##__VA_ARGS__) : (void)0
#define WARN(fmt, ...)\
	module_loglevel >= LOG_LEVEL_WARN ? logmsg(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__) : (void)0
#define INFO(fmt, ...)\
	module_loglevel >= LOG_LEVEL_INFO ? logmsg(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__) : (void)0
#define DBG(fmt, ...)\
	module_loglevel >= LOG_LEVEL_DEBUG ? logmsg(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__) : (void)0
#else
#define ERR(fmt, ...) logmsg(LOG_LEVEL_ERR, fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) logmsg(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define INFO(fmt, ...) logmsg(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define DBG(fmt, ...) logmsg(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#endif

#endif

