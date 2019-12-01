// sudo dnf install libevent-devel
// gcc -g -Wall -Wpedantic -Wextra event.c -o event -levent $(pkg-config --cflags --libs libnl-3.0 libnl-genl-3.0)
//
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
#include <event2/event.h>

#include "iw-scan.h"
#include "nlnames.h"
#include "hdump.h"

static void on_netlink_event(evutil_socket_t sock, short val, void* arg)
{
	(void)sock;
	(void)val;
	struct nl_sock* nl_sock = (struct nl_sock*)arg;

	printf("%s\n", __func__);
	int nl_err = nl_recvmsgs_default(nl_sock);
	printf("%s recvmsgs=%d\n", __func__, nl_err);
}


static void on_timer_event(evutil_socket_t sock, short val, void* arg)
{
	(void)sock;
	(void)val;
	(void)arg;

	printf("%s\n", __func__);
}

static int no_seq_check(struct nl_msg *msg, void *arg)
{
	(void)msg;
	(void)arg;

	return NL_OK;
}

static int valid_handler(struct nl_msg *msg, void *arg)
{
	printf("%s %p %p\n", __func__, (void *)msg, arg);

//	hex_dump("msg", (const unsigned char *)msg, 128);

//	nl_msg_dump(msg,stdout);

	struct nlmsghdr *hdr = nlmsg_hdr(msg);

	int datalen = nlmsg_datalen(hdr);
	printf("datalen=%d attrlen=%d\n", datalen, nlmsg_attrlen(hdr,0));

	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];

	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	printf("%s cmd=%d\n", __func__, (int)(gnlh->cmd));

	// report attrs not handled in my crappy code below
	ssize_t counter=0;

	for (int i=0 ; i<NL80211_ATTR_MAX ; i++ ) {
		if (tb_msg[i]) {
			const char *name = to_string_nl80211_attrs(i);
			printf("%s i=%d %s at %p type=%d len=%d\n", __func__, 
				i, name, (void *)tb_msg[i], nla_type(tb_msg[i]), nla_len(tb_msg[i]));
			counter++;
		}
	}

	if (tb_msg[NL80211_ATTR_WIPHY]) {
		counter--;
		uint32_t phy_id = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]);
		printf("phy_id=%u\n", phy_id);
	}

	if (tb_msg[NL80211_ATTR_WIPHY_NAME]) {
		counter--;
		const char *p = nla_get_string(tb_msg[NL80211_ATTR_WIPHY_NAME]);
		printf("phy_name=%s\n", p);
	}

	if (tb_msg[NL80211_ATTR_IFINDEX]) {
		counter--;
		printf("ifindex=%d\n", nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]));
	}

	if (tb_msg[NL80211_ATTR_IFNAME]) {
		counter--;
		printf("ifname=%s\n", nla_get_string(tb_msg[NL80211_ATTR_IFNAME]));
	}

	if (tb_msg[NL80211_ATTR_IFTYPE]) {
		counter--;
		struct nlattr *attr = tb_msg[NL80211_ATTR_IFTYPE];
		enum nl80211_iftype * iftype = nla_data(attr);
		printf("iftype=%d\n", *iftype);
	}

	if (tb_msg[NL80211_ATTR_MAC]) {
		counter--;
		struct nlattr *attr = tb_msg[NL80211_ATTR_MAC];
		printf("attr type=%d len=%d ok=%d\n", nla_type(attr), nla_len(attr), nla_ok(attr,0));
		uint8_t *mac = nla_data(attr);
		hex_dump("mac", mac, nla_len(attr));
	}

	if (tb_msg[NL80211_ATTR_KEY_DATA]) {
		counter--;
		printf("keydata=?\n");
	}

	if (tb_msg[NL80211_ATTR_GENERATION]) {
		counter--;
		uint32_t attr_gen = nla_get_u32(tb_msg[NL80211_ATTR_GENERATION]);
		printf("attr generation=%#" PRIx32 "\n", attr_gen);
	}

	if (tb_msg[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]) {
		counter--;
		uint32_t tx_power = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]);
		printf("tx_power_level=%" PRIu32 "\n", tx_power);
		// printf taken from iw-4.14 interface.c print_iface_handler()
		printf("tx_power %d.%.2d dBm\n", tx_power / 100, tx_power % 100);
	}

	if (tb_msg[NL80211_ATTR_WDEV]) {
		counter--;
		uint64_t wdev = nla_get_u64(tb_msg[NL80211_ATTR_WDEV]);
		printf("wdev=%#" PRIx64 "\n", wdev);
	}

	if (tb_msg[NL80211_ATTR_CHANNEL_WIDTH]) {
		counter--;
		enum nl80211_chan_width w = nla_get_u32(tb_msg[NL80211_ATTR_CHANNEL_WIDTH]);
		printf("channel_width=%" PRIu32 "\n", w);
	}

	if (tb_msg[NL80211_ATTR_WIPHY_CHANNEL_TYPE]) {
		counter--;
		printf("channel_type=?\n");
	}

	if (tb_msg[NL80211_ATTR_WIPHY_FREQ]) {
		counter--;
		uint32_t wiphy_freq = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_FREQ]);
		printf("wiphy_freq=%" PRIu32 "\n", wiphy_freq);
	}

	if (tb_msg[NL80211_ATTR_CENTER_FREQ1]) {
		counter--;
		uint32_t center_freq1 = nla_get_u32(tb_msg[NL80211_ATTR_CENTER_FREQ1]);
		printf("center_freq1=%" PRIu32 "\n", center_freq1);
	}

	if (tb_msg[NL80211_ATTR_CENTER_FREQ2]) {
		counter--;
		uint32_t center_freq2 = nla_get_u32(tb_msg[NL80211_ATTR_CENTER_FREQ2]);
		printf("center_freq2=%" PRIu32 "\n", center_freq2);
	}

	if (tb_msg[NL80211_ATTR_WIPHY_TXQ_PARAMS]) {
		counter--;
		printf("txq_params=?\n");
	}

	if (tb_msg[NL80211_ATTR_STA_INFO]) {
		counter--;
		/* @NL80211_ATTR_STA_INFO: information about a station, part of station info
		 *  given for %NL80211_CMD_GET_STATION, nested attribute containing
		 *  info as possible, see &enum nl80211_sta_info.
		 */
		printf("sta info=?\n");
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
		printf("attr_bands=%#" PRIx32 "\n", band);
	}

	if (tb_msg[NL80211_ATTR_BSS]) {
		counter--;
		printf("bss=?\n");
		decode_attr_bss(tb_msg[NL80211_ATTR_BSS]);
	}

	if (tb_msg[NL80211_ATTR_SCAN_FREQUENCIES]) {
		counter--;
		printf("scan_frequencies\n");
		struct nlattr *nst;
		int rem_nst;
		nla_for_each_nested(nst, tb_msg[NL80211_ATTR_SCAN_FREQUENCIES], rem_nst)
			printf(" %d", nla_get_u32(nst));
		printf(",");
	}

	if (tb_msg[NL80211_ATTR_SCAN_SSIDS]) {
		counter--;
		printf("scan_ssids\n");
	}

	printf("%s counter=%zd unhandled attributes\n", __func__, counter);

//	return NL_SKIP;
	return NL_OK;
}


int main(void)
{
	// http://www.wangafu.net/~nickm/libevent-book/Ref2_eventbase.html
	const char *v = event_get_version();
	printf("libevent version=%s\n", v);

	int i;
	const char** methods = event_get_supported_methods();
	printf("Starting Libevent %s.  Available methods are:\n",
			event_get_version());
	for (i=0; methods[i] != NULL; ++i) {
		printf("    %s\n", methods[i]);
	}

	struct event_base* base = event_base_new();
	printf("Using Libevent with backend method %s.",
				event_base_get_method(base));
	enum event_method_feature f = event_base_get_features(base);
	if ((f & EV_FEATURE_ET))
		printf("  Edge-triggered events are supported.");
	if ((f & EV_FEATURE_O1))
		printf("  O(1) event notification is supported.");
	if ((f & EV_FEATURE_FDS))
		printf("  All FD types are supported.");
	puts("");

	struct timeval ten_sec;

	ten_sec.tv_sec = 10;
	ten_sec.tv_usec = 0;

	// http://www.wangafu.net/~nickm/libevent-book/Ref4_event.html
	struct event* evt = event_new(base, -1, EV_TIMEOUT|EV_READ|EV_PERSIST, on_timer_event, NULL);
	struct timeval five_seconds = {5,0};
	int retcode = event_add(evt, &five_seconds);

	/* from iw event.c */
	/* no sequence checking for multicast messages */
	struct nl_cb* cb = nl_cb_alloc(NL_CB_DEFAULT);
	nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, NULL);

	struct nl_sock* nl_sock = nl_socket_alloc_cb(cb);
	retcode = genl_connect(nl_sock);
	int nl80211_id = genl_ctrl_resolve(nl_sock, NL80211_GENL_NAME);
	printf("nl80211_id=%d\n", nl80211_id);
	int mcid = get_multicast_id(nl_sock, "nl80211", "scan");
	printf("mcid=%d\n", mcid);
	nl_socket_add_membership(nl_sock, mcid);

	int nl_sock_fd = nl_socket_get_fd(nl_sock);
	struct event* nl_evt = event_new(base, nl_sock_fd, EV_READ|EV_PERSIST, on_netlink_event, (void *)nl_sock);
	retcode = event_add(nl_evt, NULL);
	printf("evet_add retcode=%d\n", retcode);

	/* Now we run the event_base for a series of 10-second intervals, printing
	"Tick" after each.  For a much better way to implement a 10-second
	timer, see the section below about persistent timer events. */
	while (1) {
		/* This schedules an exit ten seconds from now. */
		event_base_loopexit(base, &ten_sec);

		event_base_dispatch(base);
		puts("Tick");
	}

	return EXIT_SUCCESS;
}

