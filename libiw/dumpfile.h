/*
 * libiw/dumpfile.c   write nl80211/netlink blobs to a file for later testing
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef DUMPFILE_H
#define DUMPFILE_H

#define SCAN_DUMP_COOKIE 0x64617665

#ifdef __cplusplus
extern "C" {
#endif

int dumpfile_parse(const char* dump_filename, struct dl_list* bss_list);

int dumpfile_create(const char* filename, FILE** p_outfile);
int dumpfile_write(FILE* outfile, struct nlattr* attrdata, int attrlen);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif

