#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <net/if.h>
#include <unicode/ustring.h>
#include <unicode/ustdio.h>

#include "core.h"
#include "iw.h"
#include "list.h"
#include "bss.h"
#include "hdump.h"

U_STRING_DECL(hidden, "<hidden>", 8);

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	ERR("%s\n", __func__);
	(void)nla;
	(void)err;
	(void)arg;

	return NL_STOP;
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


static int valid_handler(struct nl_msg *msg, void *arg)
{
	struct list_head* bss_list = (struct list_head*)arg;
	struct BSS* bss = NULL;

	INFO("%s\n", __func__);

	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(hdr);

	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	peek_nla_attr(tb_msg, NL80211_ATTR_MAX);

	int err=0;
	err = bss_from_nlattr(tb_msg, &bss);
	if (err) {
		goto fail;
	}

	list_add(&bss->node, bss_list);

	DBG("%s success\n", __func__);
	return NL_OK;
fail:
	return NL_SKIP;
}

static void print_supported_rates(struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_SUPPORTED_RATES);
	if (ie) {
		const struct IE_Supported_Rates *sie = IE_CAST(ie, struct IE_Supported_Rates);
		printf("\tSupported rates:");
		for (size_t i=0 ; i<sie->count ; i++) {
			printf("%0.1f%s ", sie->rate[i], sie->basic[i]?"*":"");
		}
		printf("\n");
	}

}

static void print_dsss_param(struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_DSSS_PARAMETER_SET);
	if (ie) {
		const struct IE_DSSS_Parameter_Set *sie = IE_CAST(ie, struct IE_DSSS_Parameter_Set);
		printf("\tDS Parameter set: channel %d\n", sie->current_channel);
	}
}

static const UChar* get_ssid(struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_SSID);
	const UChar* ssid = NULL;

	if (ie) {
		const struct IE_SSID *ie_ssid = IE_CAST(ie, const struct IE_SSID);
		if (ie_ssid->ssid_len) {
			ssid = ie_ssid->ssid;
//				hex_dump("ssid", ie->buf, ie->len);
		}
	}
	else {
		ERR("BSS %s has no SSID\n", bss->bssid_str);
	}

	// TODO check for weird-o SSIDs full of NULLs
	return ssid ? ssid : hidden;
}

static void print_bss(struct BSS* bss)
{
	XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);

	printf("BSS %s\n", bss->bssid_str);
	char tsf_str[64];
	int err = tsf_to_timestamp_str(bss->tsf, tsf_str, 64);
	XASSERT(err<64, err);
	printf("\tTSF: %" PRIu64 " usec (%s)\n", bss->tsf, tsf_str);
	printf("\tfreq: %"PRIu32"\n", bss->frequency);
	printf("\tbeacon interval: %d TUs\n", bss->beacon_interval);
	printf("\tsignal: %0.2f dBm\n", bss->signal_strength_mbm/100.0);
	u_printf("\tSSID: %S\n", get_ssid(bss));
	print_supported_rates(bss);
	print_dsss_param(bss);
}

static void print_bss_to_csv(struct BSS* bss, bool header)
{
	if (header) {
		printf("BSSID,frequency,signal_strength,SSID\n");
	}

	printf("%s, %d, %0.2f, ", bss->bssid_str, bss->frequency, bss->signal_strength_mbm/100.0);
	u_printf("\"%S\"", get_ssid(bss));
	printf("\n");
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s ifname\n", argv[0]);
		exit(1);
	}

	const char* ifname = argv[1];

	LIST_HEAD(bss_list);
	struct nl_cb* cb = nl_cb_alloc(NL_CB_DEFAULT);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, (void*)&bss_list);
	int cb_err = 1;
	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &cb_err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &cb_err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &cb_err);

	struct nl_sock* nl_sock = nl_socket_alloc_cb(cb);
	int err = genl_connect(nl_sock);

	int nl80211_id = genl_ctrl_resolve(nl_sock, NL80211_GENL_NAME);

	int ifidx = if_nametoindex(ifname);

	struct nl_msg* msg = nlmsg_alloc();

	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, nl80211_id, 0, 
						NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);

	nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifidx);
	err = nl_send_auto(nl_sock, msg);
	if (err < 0) {
		ERR("nl_send_auto failed err=%d\n", err);
		goto leave;
	}

	while (cb_err > 0) {
		err = nl_recvmsgs(nl_sock, cb);
		INFO("nl_recvmsgs err=%d\n", err);
	}

	U_STRING_INIT(hidden, "<hidden>", 8);
	struct BSS* bss;
	list_for_each_entry(bss, &bss_list, node) {
		print_bss(bss);
	}

	bool first=true;
	list_for_each_entry(bss, &bss_list, node) {
		print_bss_to_csv(bss, first);
		first = false;
	}

leave:
	bss_free_list(&bss_list);
	nl_cb_put(cb);
	nl_socket_free(nl_sock);
	nlmsg_free(msg);
	return EXIT_SUCCESS;
}

