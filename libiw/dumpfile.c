/*
 * libiw/dumpfile.c   write nl80211/netlink blobs to a file for later testing
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <errno.h>
#include <inttypes.h>

#include "core.h"
#include "bss.h"
#include "list.h"
#include "iw.h"
#include "dumpfile.h"

int dumpfile_create(const char* filename, FILE** p_outfile)
{
	*p_outfile = fopen(filename,"wb");
	if (!*p_outfile) {
		return errno;
	}
	else {
		// write a simple header so I can remember what file this silly file
		// format is.  Header terminated by \n
		fprintf(*p_outfile,"scan-dump\n");
	}

	return 0;
}

int dumpfile_parse(const char* dump_filename, struct dl_list* bss_list )
{
	FILE* infile;
	int err = 0;

	infile = fopen(dump_filename, "rb");
	if (!infile) {
		return errno;
	}

	// read until \n (the header)
	while (!feof(infile)) {
		uint8_t c;
		int ret;

		ret = fread(&c, sizeof(uint8_t), 1, infile);
		if (c=='\n') {
			break;
		}
	}
	if (feof(infile)) {
		return -EINVAL;
	}

	const int max_size = 4096;
	struct BSS* bss=NULL;
	while (true) { 
		int ret;
		uint32_t size, cookie;
		uint8_t buf[max_size];

		ret = fread((char *)&cookie, sizeof(uint32_t), 1, infile);
		if (ret != 1) {
			if (feof(infile)) {
				// normal end of file. no more data!
				break;
			}

			ERR("%s premature end of file reading cookie; read=%d expected=%d\n", __func__, ret, 1);
			err = -EINVAL;
			goto leave;
		}

		ret = fread((char *)&size, sizeof(uint32_t), 1, infile);
		if (ret != 1) {
			ERR("%s premature end of file reading size; read=%d expected=%d\n", __func__, ret, 1);
			err = -EINVAL;
			goto leave;
		}

		DBG("%1$s cookie=%2$#"PRIx32" size=%3$#"PRIx32" (%3$#"PRIx32")\n", __func__, cookie, size);

		if (cookie != SCAN_DUMP_COOKIE) {
			ERR("%s invalid cookie=%#"PRIx32"; not a valid scan-dump file\n", 
					dump_filename, cookie);
			err = -EINVAL;
			goto leave;
		}
		if (size > max_size) { 
			ERR("%1$s size=%2$"PRIu32" (%2$#"PRIx32") too big\n", __func__, size);
			err = -E2BIG;
			goto leave;
		}

		ret = fread((char *)buf, sizeof(uint8_t), size, infile);
		if (ret != size) {
			ERR("%s premature end of file reading buffer; read=%d expected=%d\n", __func__, ret, size);
			err = -EINVAL;
			goto leave;
		}

		int attrlen = (int) size;
		struct nlattr* attr = (struct nlattr*)buf;
		struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
		struct BSS* bss;

		// inject data into the normal netlink callback path
		err = nla_parse(tb_msg, NL80211_ATTR_MAX, attr, attrlen, NULL);
		if (err < 0) {
			return err;
		}

		peek_nla_attr(tb_msg, NL80211_ATTR_MAX);

		err = bss_from_nlattr(tb_msg, &bss);
		if (err) {
			goto leave;
		}

		dl_list_add_tail(bss_list, &bss->node);
	}

leave:
	fclose(infile);
	return err;
}

int dumpfile_write(FILE* outfile, struct nlattr* attrdata, int attrlen)
{
	// 4-byte magic 
	uint32_t magic = SCAN_DUMP_COOKIE;
	fwrite((void*)&magic, sizeof(uint32_t), 1, outfile);

	// 4-byte length
	uint32_t len32 = attrlen;
	fwrite((void*)&len32, sizeof(uint32_t), 1, outfile);

	// write the data
	fwrite((void*)attrdata, attrlen, 1, outfile);

	return 0;
}

