// http://software.schmorp.de/pkg/libev.html
// Blessed by Bishop Collins
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#include <linux/if_ether.h>
#include <linux/nl80211.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <ev.h>

#include "core.h"
#include "iw-scan.h"
#include "nlnames.h"
#include "hdump.h"
#include "ie.h"

struct netlink_event
{
	ev_io io;
	struct nl_sock* nl_sock;
};

static void on_netlink_cb(EV_P_ ev_io* w, int revents)
{
	(void)w;
	(void)revents;
	struct netlink_event* n = (struct netlink_event *)w;
	struct nl_sock* nl_sock = n->nl_sock;

	DBG("%s\n", __func__);
	int nl_err = nl_recvmsgs_default(nl_sock);
	DBG("%s recvmsgs=%d\n", __func__, nl_err);
}


//static void on_timer_event(evutil_socket_t sock, short val, void* arg)
//{
//	(void)sock;
//	(void)val;
//	(void)arg;
//
//	DBG("%s\n", __func__);
//}

static int no_seq_check(struct nl_msg *msg, void *arg)
{
	(void)msg;
	(void)arg;

	return NL_OK;
}

static int valid_handler(struct nl_msg *msg, void *arg)
{
	int err;

	DBG("%s %p %p\n", __func__, (void *)msg, arg);

//	hex_dump("msg", (const unsigned char *)msg, 128);

//	nl_msg_dump(msg,stdout);

	struct nlmsghdr *hdr = nlmsg_hdr(msg);

	int datalen = nlmsg_datalen(hdr);
	DBG("datalen=%d attrlen=%d\n", datalen, nlmsg_attrlen(hdr,0));

	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];

	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	DBG("%s cmd=%d %s\n", __func__, (int)(gnlh->cmd), to_string_nl80211_commands(gnlh->cmd));

	// report attrs not handled in my crappy code below
	ssize_t counter=0;

	for (int i=0 ; i<NL80211_ATTR_MAX ; i++ ) {
		if (tb_msg[i]) {
			const char *name = to_string_nl80211_attrs(i);
			DBG("%s i=%d %s at %p type=%d len=%d\n", __func__, 
				i, name, (void *)tb_msg[i], nla_type(tb_msg[i]), nla_len(tb_msg[i]));
			counter++;
		}
	}

	if (tb_msg[NL80211_ATTR_WIPHY]) {
		counter--;
		uint32_t phy_id = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]);
		DBG("phy_id=%u\n", phy_id);
	}

	if (tb_msg[NL80211_ATTR_WIPHY_NAME]) {
		counter--;
		const char *p = nla_get_string(tb_msg[NL80211_ATTR_WIPHY_NAME]);
		DBG("phy_name=%s\n", p);
	}

	if (tb_msg[NL80211_ATTR_IFINDEX]) {
		counter--;
		DBG("ifindex=%d\n", nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]));
	}

	if (tb_msg[NL80211_ATTR_IFNAME]) {
		counter--;
		DBG("ifname=%s\n", nla_get_string(tb_msg[NL80211_ATTR_IFNAME]));
	}

	if (tb_msg[NL80211_ATTR_IFTYPE]) {
		counter--;
		struct nlattr *attr = tb_msg[NL80211_ATTR_IFTYPE];
		enum nl80211_iftype * iftype = nla_data(attr);
		DBG("iftype=%d\n", *iftype);
	}

	if (tb_msg[NL80211_ATTR_MAC]) {
		counter--;
		struct nlattr *attr = tb_msg[NL80211_ATTR_MAC];
		DBG("attr type=%d len=%d ok=%d\n", nla_type(attr), nla_len(attr), nla_ok(attr,0));
		uint8_t *mac = nla_data(attr);
		hex_dump("mac", mac, nla_len(attr));
	}

	if (tb_msg[NL80211_ATTR_KEY_DATA]) {
		counter--;
		DBG("keydata=?\n");
	}

	if (tb_msg[NL80211_ATTR_GENERATION]) {
		counter--;
		uint32_t attr_gen = nla_get_u32(tb_msg[NL80211_ATTR_GENERATION]);
		DBG("attr generation=%#" PRIx32 "\n", attr_gen);
	}

	if (tb_msg[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]) {
		counter--;
		uint32_t tx_power = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]);
		DBG("tx_power_level=%" PRIu32 "\n", tx_power);
		// DBG taken from iw-4.14 interface.c print_iface_handler()
		DBG("tx_power %d.%.2d dBm\n", tx_power / 100, tx_power % 100);
	}

	if (tb_msg[NL80211_ATTR_WDEV]) {
		counter--;
		uint64_t wdev = nla_get_u64(tb_msg[NL80211_ATTR_WDEV]);
		DBG("wdev=%#" PRIx64 "\n", wdev);
	}

	if (tb_msg[NL80211_ATTR_CHANNEL_WIDTH]) {
		counter--;
		enum nl80211_chan_width w = nla_get_u32(tb_msg[NL80211_ATTR_CHANNEL_WIDTH]);
		DBG("channel_width=%" PRIu32 "\n", w);
	}

	if (tb_msg[NL80211_ATTR_WIPHY_CHANNEL_TYPE]) {
		counter--;
		DBG("channel_type=?\n");
	}

	if (tb_msg[NL80211_ATTR_WIPHY_FREQ]) {
		counter--;
		uint32_t wiphy_freq = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_FREQ]);
		DBG("wiphy_freq=%" PRIu32 "\n", wiphy_freq);
	}

	if (tb_msg[NL80211_ATTR_CENTER_FREQ1]) {
		counter--;
		uint32_t center_freq1 = nla_get_u32(tb_msg[NL80211_ATTR_CENTER_FREQ1]);
		DBG("center_freq1=%" PRIu32 "\n", center_freq1);
	}

	if (tb_msg[NL80211_ATTR_CENTER_FREQ2]) {
		counter--;
		uint32_t center_freq2 = nla_get_u32(tb_msg[NL80211_ATTR_CENTER_FREQ2]);
		DBG("center_freq2=%" PRIu32 "\n", center_freq2);
	}

	if (tb_msg[NL80211_ATTR_WIPHY_TXQ_PARAMS]) {
		counter--;
		DBG("txq_params=?\n");
	}

	if (tb_msg[NL80211_ATTR_STA_INFO]) {
		counter--;
		/* @NL80211_ATTR_STA_INFO: information about a station, part of station info
		 *  given for %NL80211_CMD_GET_STATION, nested attribute containing
		 *  info as possible, see &enum nl80211_sta_info.
		 */
		DBG("sta info=?\n");
		print_sta_handler(msg, arg);
	}

	if (tb_msg[NL80211_ATTR_BANDS]) {
		counter--;
		/* @NL80211_ATTR_BANDS: operating bands configuration.  This is a u32
		 *	bitmask of BIT(NL80211_BAND_*) as described in %enum
		 *	nl80211_band.  For instance, for NL80211_BAND_2GHZ, bit 0
		 *	would be set.  This attribute is used with
		 *	%NL80211_CMD_START_NAN and %NL80211_CMD_CHANGE_NAN_CONFIG, and
		 *	it is optional.  If no bands are set, it means don't-care and
		 *	the device will decide what to use.
		 */

		enum nl80211_band_attr band = nla_get_u32(tb_msg[NL80211_ATTR_BANDS]);
		// results are kinda boring ... 
		DBG("attr_bands=%#" PRIx32 "\n", band);
	}

	if (tb_msg[NL80211_ATTR_BSS]) {
		counter--;
		DBG("bss=?\n");
		decode_attr_bss(tb_msg[NL80211_ATTR_BSS]);
	}

	struct IE_List ie_list;
	err = ie_list_init(&ie_list);

	if (tb_msg[NL80211_ATTR_IE]) {
		counter--;
		struct nlattr* nlie = tb_msg[NL80211_ATTR_IE];
		DBG("ATTR_IE len=%"PRIu16"\n", nla_len(nlie));
		hex_dump("attr_ie", nla_data(nlie), nla_len(nlie));
		err = decode_ie_buf(nla_data(nlie), nla_len(nlie), &ie_list);
		if (!err) {
			const struct IE* ie= (void*)-1;
			ie_list_for_each_entry(ie, ie_list) {
				XASSERT(ie->cookie == IE_COOKIE, ie->cookie);
				printf("%3d ", ie->id);
			}
			printf("\n");
		}
		
	}

	if (tb_msg[NL80211_ATTR_SCAN_FREQUENCIES]) {
		counter--;
		err = decode_attr_scan_frequencies(tb_msg[NL80211_ATTR_SCAN_FREQUENCIES]);
		if (err) {
			goto fail;
		}
	}

	struct bytebuf_array ssid_list;
	err = bytebuf_array_init(&ssid_list, 64);
	if (err) {
		goto fail;
	}

	if (tb_msg[NL80211_ATTR_SCAN_SSIDS]) {
		counter--;
		err = decode_attr_scan_ssids(tb_msg[NL80211_ATTR_SCAN_SSIDS], &ssid_list);
		if (err) {
			XASSERT(0, err);
			goto fail;
		}

		for (size_t i=0 ; i<ssid_list.len ; i++) {
			hex_dump("ssid", ssid_list.list[i].buf, ssid_list.list[i].len);
		}
	}

	DBG("%s counter=%zd unhandled attributes\n", __func__, counter);

//	return NL_SKIP;
	return NL_OK;
fail:
	// if it's been initialized, free it
	if (ssid_list.max) {
		bytebuf_array_free(&ssid_list);
	}

	return NL_SKIP;
}


int main(void)
{
	int err;
	struct ev_loop* loop = EV_DEFAULT;

	DBG("using libev %d %d\n", ev_version_major(), ev_version_minor());

	/* from iw event.c */
	/* no sequence checking for multicast messages */
	struct nl_cb* cb = nl_cb_alloc(NL_CB_DEFAULT);
	nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, NULL);

	struct nl_sock* nl_sock = nl_socket_alloc_cb(cb);
	err = genl_connect(nl_sock);
	DBG("genl_connect err=%d\n", err);

	int nl80211_id = genl_ctrl_resolve(nl_sock, NL80211_GENL_NAME);
	DBG("nl80211_id=%d\n", nl80211_id);
	int mcid = get_multicast_id(nl_sock, "nl80211", "scan");
	DBG("mcid=%d\n", mcid);
	nl_socket_add_membership(nl_sock, mcid);

	int nl_sock_fd = nl_socket_get_fd(nl_sock);

//	ev_io nl_watcher;
	struct netlink_event nl_watcher = { .nl_sock = nl_sock };
	ev_io_init(&nl_watcher.io, on_netlink_cb, nl_sock_fd, EV_READ);
	ev_io_start(loop, &nl_watcher.io);

	ev_run(loop, 0);

	return EXIT_SUCCESS;
}

