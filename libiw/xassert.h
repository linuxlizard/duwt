#ifndef XASSERT_H
#define XASSERT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void xassert_fail(const char *expr, const char *file, int line, uintmax_t value);

#ifdef __cplusplus
} // end extern "C"
#endif

#define XASSERT(expr,value) ((expr) ? (void)value : xassert_fail(#expr, __FILE__, __LINE__, value))

#endif
