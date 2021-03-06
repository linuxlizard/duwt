/*
 * libiw/core.h   all the things header file
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef CORE_H
#define CORE_H

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#include "config.h"
#include "xassert.h"
#include "hdump.h"
#include "log.h"

#define PTR_FREE(p) do { free(p); (p)=NULL; } while(0)
#define PTR_ASSIGN(dst,src) do { (dst)=(src); (src)=NULL; } while(0)

#define POISON(buf,len) memset(buf,0xee,len)

#define ARRAY_SIZE(ar) (sizeof(ar)/sizeof(ar[0]))

// e.g., printf("BSSID=" MAC_ADD_FMT "\n", MAC_ADD_PRN(bss->bssid));
#define MAC_ADD_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ADD_PRN(m) m[0], m[1], m[2], m[3], m[4], m[5]

// no C codebase is complete without rewriting these macros 
#define MAX(a,b)  ((a) > (b) ? (a) : (b))

#ifdef __cplusplus
extern "C" {
#endif

// from iw util.c
void mac_addr_n2a(char mac_addr[], const unsigned char *arg);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif

