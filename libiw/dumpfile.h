#ifndef DUMPFILE_H
#define DUMPFILE_H

#define SCAN_DUMP_COOKIE 0x64617665

int dumpfile_parse(const char* dump_filename, struct dl_list* bss_list);

int dumpfile_write(FILE* outfile, struct nlattr* attrdata, int attrlen);

#endif

