/*
 * libiw/ssid.c  handle SSID
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <iconv.h>

#include "core.h"
#include "bss.h"
#include "ssid.h"

// https://github.com/zwegner/faster-utf8-validator/blob/master/z_validate.c
//
// https://stackoverflow.com/questions/115210/how-to-check-whether-a-file-is-valid-utf-8
//
int ssid_utf8_validate(const char* ssid, size_t ssid_len)
{
	if (ssid_len > SSID_MAX_LEN) {
		return -ENAMETOOLONG;
	}

	// convert an allegedly UTF-8 ssid into UTF-16 which should allow the
	// iconv() library to validate the UTF-8-ness of our SSID
	iconv_t cd = iconv_open("UTF16//", "UTF-8//");
	if (cd  == (iconv_t)-1) {
		// failed
		return -errno;
	}

	hex_dump(__func__, (unsigned char*)ssid, ssid_len);

	char out[ssid_len*4];
	size_t out_len = sizeof(out);
	char *inp = (char *)ssid;
	char *outp = out;
	errno = 0;
	size_t s = iconv(cd, &inp, &ssid_len, &outp, &out_len);
	if (s == (size_t)-1) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
		printf("%s s=%zu out_len=%zu %d %m\n", __func__, s, out_len, errno);
#pragma GCC diagnostic pop
		return -errno;
	}

	iconv_close(cd);
	return 0;
}

int ssid_print(const struct BSS* bss, FILE* outfile, size_t width, const char* extra_str)
{
	// default to 32-char display width
	if (width==0) {
		width = 32;
	}

	const struct IE_SSID* sie = bss_get_ssid(bss);
	if (!sie || sie->ssid_is_hidden) {
		// if no SSID found for this BSS so say hidden
		if (extra_str) {
			return(fprintf(outfile, "%*s%s", (int)width, HIDDEN_SSID, extra_str));
		}
		else {
			return(fprintf(outfile, "%*s", (int)width, HIDDEN_SSID ));
		}
	}

	if ( !sie->ssid_is_valid_utf8 ) {
		// TODO print the hex dump
		return sie->ssid_len;
	}

	// TODO check ssid_is_printable

	if (extra_str) {
		return(fprintf(outfile, "%*.*s%s", (int)width, (int)sie->ssid_len, sie->ssid, extra_str));
	}
	else {
		return(fprintf(outfile, "%*.*s", (int)width, (int)sie->ssid_len, sie->ssid));
	}

}

