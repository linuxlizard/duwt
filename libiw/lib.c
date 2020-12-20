/*
 * libiw/lib.c  library-ish wrapper around some `iw` fuctionality
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
#include "lib.h"

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	ERR("%s\n", __func__);
	(void)nla;
	(void)err;

	struct nl80211_connect* conn = (struct nl80211_connect*)arg;
	conn->ret = err->error;

	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	DBG("%s\n", __func__);

	(void)msg;

	struct nl80211_connect* conn = (struct nl80211_connect*)arg;
	conn->ret = 0;

	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	DBG("%s\n", __func__);
	(void)msg;

	struct nl80211_connect* conn = (struct nl80211_connect*)arg;
	conn->ret = 0;

	return NL_STOP;
}

int nl80211_setup(struct nl80211_connect* conn)
{
	int err;
	struct nl_sock* nl_sock = NULL;
	struct nl_cb* cb = NULL;

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		err = -ENOMEM;
		goto fail;
	}

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, conn);
//	nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, conn);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, conn);

	// caller should fill this out with their own handler
//	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, conn);

	nl_sock = nl_socket_alloc_cb(cb);
	if (!nl_sock) {
		err = -ENOMEM;
		goto fail;
	}

	err = genl_connect(nl_sock);
	if (err < 0) {
		ERR("%s genl_connect failed err=%d %s\n", __func__, err, nl_geterror(err));
		goto fail;
	}

	int nl80211_id = genl_ctrl_resolve(nl_sock, NL80211_GENL_NAME);
	if (nl80211_id < 0) {
		ERR("%s genl_ctrl_resolve failed err=%d %s\n", __func__, err, nl_geterror(err));
		err = nl80211_id;
		goto fail;
	}

	conn->sock = nl_sock;
	conn->cb = cb;
	conn->id = nl80211_id;

	return 0;
fail:
	if (nl_sock) {
		nl_socket_free(nl_sock);
	}
	if (cb) {
		nl_cb_put(cb);
	}
	return err;
}

void nl80211_cleanup(struct nl80211_connect* conn)
{
	nl_cb_put(conn->cb);
	nl_socket_free(conn->sock);
}
