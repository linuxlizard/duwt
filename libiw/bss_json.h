#ifndef JSON_BSS_H
#define JSON_BSS_H

#ifdef __cplusplus
extern "C" {
#endif

int bss_to_json(const struct BSS* bss);
int bss_list_to_json(struct dl_list* list, json_t** p_jlist);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif
