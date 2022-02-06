/*
 * libiw/dev.c   fetch wlan devices like 'iw dev'
 *
 * Contains copied chunks from from iw iw.h See COPYING.
 *
 * Copyright (c) 2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <net/if.h>

#include <linux/netlink.h>
#include <linux/nl80211.h>

#include "core.h"
#include "nlnames.h"
#include "lib.h"
#include "iw.h"

struct iw_dev_list {
	struct iw_dev* list;
	size_t count;
	size_t max;
};

static int valid_handler(struct nl_msg *msg, void *arg)
{
	struct iw_dev_list* devlist = (struct iw_dev_list*)arg;
	struct iw_dev* dev;

	DBG("%s\n", __func__);

	if (devlist->count + 1 > devlist->max) {
		ERR("%s out of memory for iw_dev at count=%zu\n", __func__, devlist->count);
		return NL_SKIP;
	}

	dev = &devlist->list[devlist->count++];
	memset(dev, 0, sizeof(struct iw_dev));

//	nl_msg_dump(msg, stdout);

	struct nlmsghdr *hdr = nlmsg_hdr(msg);

	struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(hdr);

	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
	int err = nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh,0), genlmsg_attrlen(gnlh,0), NULL);
	if (err) {
		ERR("%s parse failed err=%d %s\n", __func__, err, nl_geterror(err));
		goto fail;
	}

	peek_nla_attr(tb_msg, NL80211_ATTR_MAX);

	if (tb_msg[NL80211_ATTR_WIPHY]) {
		dev->phy_id = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]);
	}

	if (tb_msg[NL80211_ATTR_IFINDEX]) {
		dev->ifindex = (int)nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]);
		if_indextoname(dev->ifindex, dev->ifname);
	}

	if (tb_msg[NL80211_ATTR_IFNAME]) {
		strncpy(dev->ifname, nla_get_string(tb_msg[NL80211_ATTR_IFNAME]), IF_NAMESIZE);
	}

	if (tb_msg[NL80211_ATTR_IFTYPE]) {
		dev->iftype = nla_get_u32(tb_msg[NL80211_ATTR_IFTYPE]);
	}

	if (tb_msg[NL80211_ATTR_MAC]) {
		memcpy(dev->addr, nla_data(tb_msg[NL80211_ATTR_MAC]), ETH_ALEN);
	}

 	/*	"%NL80211_ATTR_WIPHY_FREQ in positive KHz. Only valid when supplied with
	 *	an %NL80211_ATTR_WIPHY_FREQ_OFFSET."
	 */
	if (tb_msg[NL80211_ATTR_WIPHY_FREQ]) {
		dev->freq_khz = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_FREQ]);
	}

	// ATTR_WIPHY_CHANNEL_TYPE

 	/* "@NL80211_ATTR_GENERATION: Used to indicate consistent snapshots for
	 *	dumps. This number increases whenever the object list being
	*	dumped changes"
	 */
	if (tb_msg[NL80211_ATTR_GENERATION]) {
		dev->generation = nla_get_u32(tb_msg[NL80211_ATTR_GENERATION]);
	}

	// TODO
//	if (tb_msg[NL80211_ATTR_SSID]) {
//		memcpy(dev->ssid, nla_data(tb_msg[NL80211_ATTR_SSID]), 32);
//	}

	// ATTR_4ADDR
	// ATTR_WIPHY_TX_POWER_LEVEL

	if (tb_msg[NL80211_ATTR_WDEV]) {
		dev->wdev = nla_get_u64(tb_msg[NL80211_ATTR_WDEV]);
	}

	if (tb_msg[NL80211_ATTR_CHANNEL_WIDTH]) {
		dev->channel_width = nla_get_u32(tb_msg[NL80211_ATTR_CHANNEL_WIDTH]);
	}

	if (tb_msg[NL80211_ATTR_CENTER_FREQ1]) {
		dev->channel_center_freq1 = nla_get_u32(tb_msg[NL80211_ATTR_CENTER_FREQ1]);
	}

	if (tb_msg[NL80211_ATTR_CENTER_FREQ2]) {
		dev->channel_center_freq1 = nla_get_u32(tb_msg[NL80211_ATTR_CENTER_FREQ2]);
	}

	// ATTR_TXQ_SATS

	return NL_OK;
fail:
	return NL_SKIP;
}

/* get all the wlan devices 
 * 
 */

int iw_dev_list(struct iw_dev* dev_list, size_t* count)
{
	int ret;
	int err=0;

	struct iw_dev_list devlist = {
		.list = dev_list,
		.count = 0,
		.max = *count
	};
	*count = 0;

	struct nl80211_connect conn = {
		.sock = NULL,
		.cb = NULL,
		.id = 0
	};

	ret = nl80211_setup(&conn);
	if (ret < 0) {
		return ret;
	}

	nl_cb_set(conn.cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, &devlist);

	struct nl_msg* msg = nlmsg_alloc();
	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, conn.id, 0, 
						NLM_F_DUMP, NL80211_CMD_GET_INTERFACE, 0);
	ret = nl_send_auto_complete(conn.sock, msg);
	// TODO check error

	conn.ret = 1;
	while (conn.ret == 1) {
		ret = nl_recvmsgs_default(conn.sock);
		if (ret) {
			INFO("nl_recvmsgs ret=%d %s\n", ret, nl_geterror(ret));
		}
	}

	err = 0;
	nl80211_cleanup(&conn);

	if (msg) {
		nlmsg_free(msg);
	}
	*count = devlist.count;
	return err;
}

