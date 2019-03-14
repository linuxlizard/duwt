/* this code based originally on 'iw' which has the following COPYING */
/* 
 * Copyright (c) 2007, 2008        Johannes Berg
 * Copyright (c) 2007              Andy Lutomirski
 * Copyright (c) 2007              Mike Kershaw
 * Copyright (c) 2008-2009         Luis R. Rodriguez
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef NETLINK_H
#define NETLINK_H

struct nl_sock;
struct nl_cb;
struct nl_attr;

/* from iw 5.0.1 */
struct nl80211_state {
	struct nl_sock *nl_sock;
	int nl80211_id;
};

struct nlattr_list {
	size_t counter;

	/* array of malloc'd copies of the struct nlattr blob from the genl
	 * messages' payload 
	 */
	struct nl_attr *attr_list[1024];

	size_t attr_len_list[1024];
};

#ifdef __cplusplus
extern "C" {
#endif

/* from iw 5.0.1 */
int nl80211_init(struct nl80211_state *state);
void nl80211_cleanup(struct nl80211_state *state);

int iw_get_scan(struct nl80211_state* state, const char *ifname, struct nlattr_list *scan_attrs);

#ifdef __cplusplus
}
#endif

#endif
