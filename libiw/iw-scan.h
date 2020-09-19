/* davep 20200919 ; adding copyright COPYING from 'iw' from which this springs */

//Copyright (c) 2007, 2008	Johannes Berg
//Copyright (c) 2007		Andy Lutomirski
//Copyright (c) 2007		Mike Kershaw
//Copyright (c) 2008-2009		Luis R. Rodriguez
//
//Permission to use, copy, modify, and/or distribute this software for any
//purpose with or without fee is hereby granted, provided that the above
//copyright notice and this permission notice appear in all copies.
//
//THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef IW_SCAN_H
#define IW_SCAN_H

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

#ifdef __cplusplus
} // end extern "C"
#endif


#endif
