#ifndef JSON_BSS_H
#define JSON_BSS_H

#include <jansson.h>

#ifdef __cplusplus
extern "C" {
#endif

// get a simple summary of a BSS
int bss_to_json_summary(const struct BSS* bss, json_t** p_jbss);

// get full BSS info as json
int bss_to_json(const struct BSS* bss, json_t** p_jbss);

int bss_list_to_json(struct dl_list* list, json_t** p_jlist);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif
