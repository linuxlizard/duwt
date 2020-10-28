/*
 * libiw/iw-dev.c   duplicate 'iw dev' command
 *
 * XXX Work in progress. Not complete.
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <net/if.h>

//#include <jansson.h>

#include "core.h"
#include "iw.h"
#include "ie.h"
#include "list.h"
#include "bss.h"
#include "hdump.h"
#include "nlnames.h"
#include "str.h"

struct nl80211_connect {
	struct nl_sock* sock;
	struct nl_cb* cb;
	int id;
	int ret;
};

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

static int valid_handler(struct nl_msg *msg, void *arg)
{
	struct nl80211_connect* conn = (struct nl80211_connect*)arg;

	nl_msg_dump(msg, stdout);

	struct nlmsghdr *hdr = nlmsg_hdr(msg);

	struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(hdr);

	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
	int err = nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh,0), genlmsg_attrlen(gnlh,0), NULL);
	if (err) {
		ERR("%s parse failed err=%d %s\n", __func__, err, nl_geterror(err));
		goto fail;
	}

	printf("%s cmd=%d %s\n", __func__, (int)(gnlh->cmd), to_string_nl80211_commands(gnlh->cmd));
	peek_nla_attr(tb_msg, NL80211_ATTR_MAX);

	uint32_t phy_id;
	int ifindex;
	char ifname[IF_NAMESIZE+1];
	if (tb_msg[NL80211_ATTR_WIPHY]) {
		phy_id = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]);
		printf("phy_id=%u\n", phy_id);
	}

	if (tb_msg[NL80211_ATTR_IFINDEX]) {
		ifindex = (int)nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]);
		printf("ifindex=%d %s\n", ifindex, if_indextoname(ifindex, ifname));
	}
	return NL_OK;
fail:
	return NL_SKIP;
}

static int nl80211_setup(struct nl80211_connect* conn)
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
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, conn);

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

int main(void)
{
	int ret;

	log_set_level(LOG_LEVEL_PLAID);

	struct nl80211_connect conn = {
		.sock = NULL,
		.cb = NULL,
		.id = 0
	};

	ret = nl80211_setup(&conn);
	if (ret < 0) {
		return EXIT_FAILURE;
	}

	struct nl_msg* msg = nlmsg_alloc();
	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, conn.id, 0, 
						NLM_F_DUMP, NL80211_CMD_GET_INTERFACE, 0);
	ret = nl_send_auto_complete(conn.sock, msg);

	conn.ret = 1;
	while (conn.ret == 1) {
		ret = nl_recvmsgs_default(conn.sock);
		if (ret) {
			INFO("nl_recvmsgs ret=%d %s\n", ret, nl_geterror(ret));
		}
	}

	ret = EXIT_SUCCESS;
//leave:
	// clean up all the things
	DBG("cleanup!\n");
	nl_cb_put(conn.cb);
	nl_socket_free(conn.sock);

	return ret;
}

