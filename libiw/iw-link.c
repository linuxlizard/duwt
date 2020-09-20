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

// using the snprintf() from iw parse_bitrate()-station.c
static char* bitrate_to_str(const struct bitrate* br, char* buf, int buflen)
{
	char *pos = buf;

	if (br->rate > 0)
		pos += snprintf(pos, buflen - (pos - buf),
				"%d.%d MBit/s", br->rate / 10, br->rate % 10);
	else
		pos += snprintf(pos, buflen - (pos - buf), "(unknown)");

	if (br->mcs) {
		pos += snprintf(pos, buflen - (pos - buf), " MCS %d", br->mcs);
	}
	if (br->vht_mcs) {
		pos += snprintf(pos, buflen - (pos - buf), " VHT-MCS %d", br->vht_mcs);
	}

	if (br->chan_width != -1) {
		pos += snprintf(pos, buflen - (pos - buf), " %sMHz", bw_str(br->chan_width));
	}

	if (br->short_gi) {
		pos += snprintf(pos, buflen - (pos - buf), " short GI");
	}
	if (br->vht_nss) {
		pos += snprintf(pos, buflen - (pos - buf), " VHT-NSS %d", br->vht_nss);
	}
	if (br->he_mcs) {
		pos += snprintf(pos, buflen - (pos - buf), " HE-MCS %d", br->he_mcs);
	}
	if (br->he_nss) {
		pos += snprintf(pos, buflen - (pos - buf), " HE-NSS %d", br->he_nss);
	}
	if (br->he_gi) {
		pos += snprintf(pos, buflen - (pos - buf), " HE-GI %d", br->he_gi);
	}
	if (br->he_dcm) {
		pos += snprintf(pos, buflen - (pos - buf), " HE-DCM %d", br->he_dcm);
	}
	if (br->he_ru_alloc) {
		pos += snprintf(pos, buflen - (pos - buf), " HE-RU-ALLOC %d", br->he_ru_alloc);
	}
	return buf;
}

// using the printf()s from iw print_link_sta() - link.c
static void print_link_state(const struct iw_link_state* ls)
{
	printf("\tRX: %u bytes (%u packets)\n", ls->rx_bytes, ls->rx_packets);
	printf("\tTX: %u bytes (%u packets)\n", ls->tx_bytes, ls->tx_packets);
	printf("\tsignal: %d dBm\n", ls->signal_dbm);

	char buf[100];
	printf("\trx bitrate: %s\n", bitrate_to_str(&ls->rx_bitrate, buf, sizeof(buf)));
	printf("\ttx bitrate: %s\n", bitrate_to_str(&ls->tx_bitrate, buf, sizeof(buf)));

	char *delim = "";
	printf("\n\tbss flags:\t");
	if (ls->bss_param & (1<<NL80211_STA_BSS_PARAM_CTS_PROT)) {
		printf("CTS-protection");
		delim = " ";
	}
	if (ls->bss_param & (1<<NL80211_STA_BSS_PARAM_SHORT_PREAMBLE)) {
		printf("%sshort-preamble", delim);
		delim = " ";
	}
	if (ls->bss_param & (1<<NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME)) {
		printf("%sshort-slot-time", delim);
		delim = " ";
	}
	printf("\n\tdtim period:\t%d", ls->dtim_period);
	printf("\n\tbeacon int:\t%d", ls->beacon_int);
	printf("\n");
}

// from iw print_link_sta()-link.c modifed to write into my struct iw_link_state instead of printf
static int decode_link_sta(struct nl_msg *msg, void *arg)
{
	struct iw_link_state* link = (struct iw_link_state*)arg;
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
	struct nlattr *binfo[NL80211_STA_BSS_PARAM_MAX + 1];
	static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1] = {
		[NL80211_STA_INFO_INACTIVE_TIME] = { .type = NLA_U32 },
		[NL80211_STA_INFO_RX_BYTES] = { .type = NLA_U32 },
		[NL80211_STA_INFO_TX_BYTES] = { .type = NLA_U32 },
		[NL80211_STA_INFO_RX_PACKETS] = { .type = NLA_U32 },
		[NL80211_STA_INFO_TX_PACKETS] = { .type = NLA_U32 },
		[NL80211_STA_INFO_SIGNAL] = { .type = NLA_U8 },
		[NL80211_STA_INFO_RX_BITRATE] = { .type = NLA_NESTED },
		[NL80211_STA_INFO_TX_BITRATE] = { .type = NLA_NESTED },
		[NL80211_STA_INFO_LLID] = { .type = NLA_U16 },
		[NL80211_STA_INFO_PLID] = { .type = NLA_U16 },
		[NL80211_STA_INFO_PLINK_STATE] = { .type = NLA_U8 },
	};
	static struct nla_policy bss_policy[NL80211_STA_BSS_PARAM_MAX + 1] = {
		[NL80211_STA_BSS_PARAM_CTS_PROT] = { .type = NLA_FLAG },
		[NL80211_STA_BSS_PARAM_SHORT_PREAMBLE] = { .type = NLA_FLAG },
		[NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME] = { .type = NLA_FLAG },
		[NL80211_STA_BSS_PARAM_DTIM_PERIOD] = { .type = NLA_U8 },
		[NL80211_STA_BSS_PARAM_BEACON_INTERVAL] = { .type = NLA_U16 },
	};

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	peek_nla_attr(tb, NL80211_ATTR_MAX);

	if (!tb[NL80211_ATTR_STA_INFO]) {
		fprintf(stderr, "sta stats missing!\n");
		return NL_SKIP;
	}
	if (nla_parse_nested(sinfo, NL80211_STA_INFO_MAX,
			     tb[NL80211_ATTR_STA_INFO],
			     stats_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}

	if (sinfo[NL80211_STA_INFO_RX_BYTES] && sinfo[NL80211_STA_INFO_RX_PACKETS]) {
		link->rx_bytes = nla_get_u32(sinfo[NL80211_STA_INFO_RX_BYTES]);
		link->rx_packets = nla_get_u32(sinfo[NL80211_STA_INFO_RX_PACKETS]);
	}
	if (sinfo[NL80211_STA_INFO_TX_BYTES] && sinfo[NL80211_STA_INFO_TX_PACKETS]) {
		link->tx_bytes = nla_get_u32(sinfo[NL80211_STA_INFO_TX_BYTES]);
		link->tx_packets = nla_get_u32(sinfo[NL80211_STA_INFO_TX_PACKETS]);
	}
	if (sinfo[NL80211_STA_INFO_SIGNAL])
		link->signal_dbm = (int8_t)nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL]);

	if (sinfo[NL80211_STA_INFO_RX_BITRATE]) {
		parse_bitrate(sinfo[NL80211_STA_INFO_RX_BITRATE], &link->rx_bitrate);
	}
	if (sinfo[NL80211_STA_INFO_TX_BITRATE]) {
		parse_bitrate(sinfo[NL80211_STA_INFO_TX_BITRATE], &link->tx_bitrate);
	}

	if (sinfo[NL80211_STA_INFO_BSS_PARAM]) {
		if (nla_parse_nested(binfo, NL80211_STA_BSS_PARAM_MAX,
				     sinfo[NL80211_STA_INFO_BSS_PARAM],
				     bss_policy)) {
			fprintf(stderr, "failed to parse nested bss parameters!\n");
		} else {
			if (binfo[NL80211_STA_BSS_PARAM_CTS_PROT]) {
				link->bss_param |= (1<<NL80211_STA_BSS_PARAM_CTS_PROT);
			}
			if (binfo[NL80211_STA_BSS_PARAM_SHORT_PREAMBLE]) {
				link->bss_param |= (1<<NL80211_STA_BSS_PARAM_SHORT_PREAMBLE);
			}
			if (binfo[NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME]) {
				link->bss_param |= (1<<NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME);
			}
			link->dtim_period = nla_get_u8(binfo[NL80211_STA_BSS_PARAM_DTIM_PERIOD]);
			link->beacon_int = nla_get_u16(binfo[NL80211_STA_BSS_PARAM_BEACON_INTERVAL]);
		}
	}

	return NL_SKIP;
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

	struct iw_link_state iw_link;
	memset(&iw_link, 0, sizeof(struct iw_link_state));

	int cb_err = 1;
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, decode_link_sta, &iw_link);
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

	print_link_state(&iw_link);
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

