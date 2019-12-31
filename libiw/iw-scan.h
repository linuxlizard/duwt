#ifndef IW_SCAN_H
#define IW_SCAN_H

#include "bytebuf.h"

#ifdef __cplusplus
extern "C" {
#endif

int get_multicast_id(struct nl_sock* sock, const char* family, const char* group);

int print_sta_handler(struct nl_msg* msg, void *arg);

void decode_tid_stats(struct nlattr *tid_stats_attr);
void decode_bitrate(struct nlattr *bitrate_attr, char *buf, int buflen);
void decode_bss_param(struct nlattr *bss_param_attr);
void decode_attr_bss( struct nlattr* attr );
int decode_attr_scan_frequencies( struct nlattr *attr);
int decode_attr_scan_ssids( struct nlattr* attr, struct bytebuf_array* ssid_list  ) ;

#ifdef __cplusplus
} // end extern "C"
#endif


#endif
