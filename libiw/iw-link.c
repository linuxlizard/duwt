#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <netlink/attr.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>

#include "iw.h"
#include "log.h"
#include "hdump.h"

static int get_station(struct nl_msg *msg, void *arg)
{
	DBG("%s\n", __func__);

	nl_msg_dump(msg, stdout);

	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(hdr);

	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
	int err = nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh,0), genlmsg_attrlen(gnlh,0), NULL);

	peek_nla_attr(tb_msg, NL80211_ATTR_MAX);

	if (tb_msg[NL80211_ATTR_MAC]) {
		// BSSID we're attached to
		uint8_t *ptr = nla_get_string(tb_msg[NL80211_ATTR_MAC]);
		hex_dump("mac", (unsigned char*)ptr, nla_len(tb_msg[NL80211_ATTR_MAC]));
	}

	if (tb_msg[NL80211_ATTR_STA_INFO]) {
		printf("%s is_nested=%d\n", __func__, nla_is_nested(tb_msg[NL80211_ATTR_STA_INFO]));
		struct nlattr* sta_info[NL80211_STA_INFO_MAX+1];
		err = nla_parse_nested(sta_info, NL80211_STA_INFO_MAX, 
									tb_msg[NL80211_ATTR_STA_INFO], NULL);
		for (size_t i=0 ; i<NL80211_STA_INFO_MAX ; i++) {
			if (sta_info[i]) {
				DBG("%s i=%zu %s type=%d len=%u\n", __func__, 
					i, "sta_info", nla_type(sta_info[i]), nla_len(sta_info[i]));
			}

		}

	}

	if (tb_msg[NL80211_ATTR_GENERATION]) {

	}



	return NL_OK;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	DBG("%s\n", __func__);

	(void)msg;

	int *ret = (int *)arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	DBG("%s\n", __func__);
	(void)msg;

	int *ret = (int*)(arg);
	*ret = 0;
	return NL_STOP;
}

int main(int argc, char* argv[])
{
	int err;

	log_set_level(LOG_LEVEL_DEBUG);

	if (argc != 2) {
		return EXIT_FAILURE;
	}

	const char* ifname = argv[1];

	int ifidx = if_nametoindex(ifname);
	if (ifidx <=0 ) {
		perror("if_nametoindex");
		return EXIT_FAILURE;
	}

	struct nl_sock* nl_sock = NULL;
	struct nl_msg* msg = NULL;

	struct nl_cb* cb = nl_cb_alloc(NL_CB_VERBOSE);
	if (!cb) {
		ERR("%s nl_cb_alloc failed\n", __func__);
		goto leave;
	}

	int cb_err = 1;
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, get_station, NULL);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &cb_err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &cb_err);

	nl_sock = nl_socket_alloc_cb(cb);
	if (!nl_sock) {
		ERR("%s nl_socket_alloc_cb failed\n", __func__);
		goto leave;
	}

	err = genl_connect(nl_sock);
	if (err) {
		ERR("%s genl_connect failed: %s\n", __func__, nl_geterror(err));
		goto leave;
	}

	int nl80211_id = genl_ctrl_resolve(nl_sock, NL80211_GENL_NAME);

	msg = nlmsg_alloc();
	if (!msg) {
		ERR("%s nlmsg_alloc failed\n", __func__);
		goto leave;
	}

	if (!genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, nl80211_id, 0, 
						NLM_F_DUMP, NL80211_CMD_GET_STATION, 0)) {
		ERR("%s genlmsg_put failed\n", __func__);
		goto leave;
	}

	nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifidx);
	err = nl_send_auto(nl_sock, msg);
	if (err < 0) {
		ERR("nl_send_auto failed err=%d %s\n", err, nl_geterror(err));
		goto leave;
	}

	err = 0;
	while (err == 0 && cb_err > 0) {
		err = nl_recvmsgs(nl_sock, cb);
		INFO("nl_recvmsgs err=%d %s\n", err, nl_geterror(err));
	}

leave:
	if (cb) {
		nl_cb_put(cb);
	}
	if (nl_sock) {
		nl_socket_free(nl_sock);
	}
	if (msg) {
		nlmsg_free(msg);
	}
	return EXIT_SUCCESS;
}

