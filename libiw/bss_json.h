/*
 * libiw/bss_json.h   encode a BSS as JSON using jansson
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef JSON_BSS_H
#define JSON_BSS_H

#include <jansson.h>

#ifdef __cplusplus
extern "C" {
#endif

// get a simple summary of a BSS
int bss_to_json_summary(const struct BSS* bss, json_t** p_jbss);
int bss_list_to_json_summary(struct dl_list* list, json_t** p_jlist);

// flags for passing to bss_to_json() to give different flavors of decode
#define BSS_JSON_SHORT_IE_DECODE (1<<1)  // only encode id, len,and hexdump of IE

// full meal deal dump of a BSS to json (big!)
int bss_to_json(const struct BSS* bss, json_t** p_jbss, unsigned int flags);
int bss_list_to_json(struct dl_list* list, json_t** p_jlist, unsigned int flags);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif
