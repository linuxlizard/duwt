/*
 * libiw/lib.h   library-ish wrapper around some `iw` fuctionality
 *
 * Contains copied chunks from from iw iw.h See COPYING.
 *
 * Copyright (c) 2020 David Poole <davep@mbuf.com>
 */

#include <linux/netlink.h>
#include <linux/nl80211.h>

#include "core.h"
#include "nlnames.h"
#include "iw.h"

#ifndef IW_LIB_H
#define IW_LIB_H

// convenience wrapper around all the things for making a simple libnl call
struct nl80211_connect 
{
	struct nl_sock* sock;
	struct nl_cb* cb;
	int id;
	int ret;
};

#ifdef __cplusplus
extern "C" {
#endif

int nl80211_setup(struct nl80211_connect* conn);
void nl80211_cleanup(struct nl80211_connect* conn);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif

