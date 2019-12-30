// started from https://www.gnu.org/software/libmicrohttpd/tutorial.html
// davep 20191227
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// netlink
#include <linux/if_ether.h>
#include <linux/nl80211.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>

#include <microhttpd.h>

// http://software.schmorp.de/pkg/libev.html
// Blessed by Bishop Collins
#include <ev.h>

#include "core.h"
#include "iw.h"
#include "iw-scan.h"
#include "nlnames.h"
#include "hdump.h"
#include "ie.h"

#define PORT 8888

#define PARAM_COOKIE 0x47558bf3
struct parameters {
	uint32_t cookie;
	const char* key;
	const char* value;
};

#define BUNDLE_COOKIE 0x01448f44
struct netlink_socket_bundle 
{
	ev_io io;
	uint32_t cookie;
	struct nl_cb* cb;
	struct nl_sock* nl_sock;
	int nl80211_id;
	int cb_err;
};

#define NETLINK_EV_COOKIE 0xb3106234
struct netlink_ev
{
	ev_io io;
	uint32_t cookie;

	// scan evevent listener
	struct nl_cb* cb;
	struct nl_sock* nl_sock;

	// netlink socket stuff for getting scan results
	struct netlink_socket_bundle scan_sock;
};

static void on_scan_event_ev_cb(EV_P_ ev_io* w, int revents)
{
	// libev callback on the Scan Event netlink socket 
	(void)w;
	(void)revents;
	struct netlink_ev* n = (struct netlink_ev *)w;
	struct nl_sock* nl_sock = n->nl_sock;

	DBG("%s\n", __func__);
	int nl_err = nl_recvmsgs_default(nl_sock);
	DBG("%s recvmsgs=%d\n", __func__, nl_err);
}

static void on_scan_result_ev_cb(EV_P_ ev_io* w, int revents)
{
	// libev callback on the Get Scan netlink socket 
	(void)w;
	(void)revents;
	struct netlink_socket_bundle* bun = (struct netlink_socket_bundle*)w;
	XASSERT(bun->cookie == BUNDLE_COOKIE, bun->cookie);

	DBG("%s\n", __func__);
	bun->cb_err = 1;
	while (bun->cb_err > 0) {
		int err = nl_recvmsgs(bun->nl_sock, bun->cb);
		INFO("nl_recvmsgs err=%d\n", err);
		DBG("%s recvmsgs=%d\n", __func__, err);
	}
}

static int scan_survey_valid_handler(struct nl_msg *msg, void *arg)
{
	// netlink callback on the NL80211_CMD_GET_SCAN message
	INFO("%s\n", __func__);

//	struct list_head* bss_list = (struct list_head*)arg;
//	struct BSS* bss = NULL;
	struct netlink_socket_bundle* bund = (struct netlink_socket_bundle*)arg;

	XASSERT(bund->cookie == BUNDLE_COOKIE, bund->cookie);

	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(hdr);

	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	peek_nla_attr(tb_msg, NL80211_ATTR_MAX);

#if 0
	int err=0;
	err = bss_from_nlattr(tb_msg, &bss);
	if (err) {
		goto fail;
	}

	list_add(&bss->node, bss_list);
#endif

	DBG("%s success\n", __func__);
	return NL_OK;
//fail:
//	return NL_SKIP;
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	// libnl callback
	ERR("%s\n", __func__);
	(void)nla;
	(void)err;
	(void)arg;

	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	// libnl callback
	DBG("%s\n", __func__);

	(void)msg;

	int *ret = (int *)arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	// libnl callback
	DBG("%s\n", __func__);
	(void)msg;

	int *ret = (int*)(arg);
	*ret = 0;
	return NL_STOP;
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
	// libnl callback
	INFO("%s\n", __func__);
	(void)msg;
	(void)arg;

	return NL_OK;
}

static int on_scan_event_valid_handler(struct nl_msg *msg, void *arg)
{
	// libnl callback for the Scan Event received
	DBG("%s %p %p\n", __func__, (void *)msg, arg);
	
	struct netlink_ev* nlev = (struct netlink_ev*)arg;
	XASSERT(nlev->cookie == NETLINK_EV_COOKIE, nlev->cookie);

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

	int ifidx = -1;
	if (tb_msg[NL80211_ATTR_IFINDEX]) {
		counter--;
		ifidx = nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]);
		DBG("ifindex=%d\n", ifidx);
	}

	// if new scan results,
	// send the get scan results message
	if (gnlh->cmd == NL80211_CMD_NEW_SCAN_RESULTS ) {
		struct nl_msg* new_msg = nlmsg_alloc();

		genlmsg_put(new_msg, NL_AUTO_PORT, NL_AUTO_SEQ, 
							nlev->scan_sock.nl80211_id, 
							0, 
							NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
		nla_put_u32(new_msg, NL80211_ATTR_IFINDEX, ifidx);
		int err = nl_send_auto(nlev->scan_sock.nl_sock, new_msg);
		if (err) {
			// TODO
		}
	}

#if 0
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
	int err = ie_list_init(&ie_list);

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
#endif
	return NL_SKIP;
}

int capture_keys (void *cls, enum MHD_ValueKind kind, 
					const char *key, const char *value)
{
	// MHD callback
	printf ("%s %d %s=%s\n", __func__, kind, key, value);
	if (cls) {
		struct parameters* params = (struct parameters*)cls;
		printf("params cookie=%#x\n", params->cookie);
	}

	return MHD_YES;
}

int answer_to_connection (void *cls, 
						struct MHD_Connection *connection,
						const char *url,
						const char *method, 
						const char *version,
						const char *upload_data,
						size_t *upload_data_size, 
						void **con_cls)
{
	// MHD callback
	const char *page  = "<html><body>Hello, browser!</body></html>";
	struct MHD_Response *response;
	int ret;

	(void)cls;
	(void)upload_data;
	(void)upload_data_size;
	(void)con_cls;

	printf("%s\n", __func__);
	printf ("New %s request for %s using version %s\n", method, url, version);

	const union MHD_ConnectionInfo* conn_info = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
	char* s = inet_ntoa(((struct sockaddr_in*)conn_info->client_addr)->sin_addr);
	printf("client=%s\n", s);

	MHD_get_connection_values (connection, MHD_HEADER_KIND, &capture_keys, NULL);

	struct parameters params = { PARAM_COOKIE, NULL, NULL };

	MHD_get_connection_values (connection, MHD_GET_ARGUMENT_KIND, &capture_keys, &params);

	response = MHD_create_response_from_buffer (strlen (page),
								(void*) page, MHD_RESPMEM_PERSISTENT);

	ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
	MHD_destroy_response (response);

	return ret;
}


static int on_client_connect (void *cls,
								const struct sockaddr *addr,
								socklen_t addrlen)
{
	// MHD callback
	(void)cls;
	(void)addrlen;

	char* s = inet_ntoa(((struct sockaddr_in*)addr)->sin_addr);
	printf("%s client=%s\n", __func__, s);

	return MHD_YES;
}

int setup_scan_netlink_socket(struct netlink_socket_bundle* bun)
{
	// this is the netlink socket we'll use to get scan results

	memset(bun, 0, sizeof(struct netlink_socket_bundle));
	bun->cookie = BUNDLE_COOKIE;

	bun->cb = nl_cb_alloc(NL_CB_DEFAULT);

	nl_cb_set(bun->cb, NL_CB_VALID, NL_CB_CUSTOM, scan_survey_valid_handler, (void*)bun);
	nl_cb_err(bun->cb, NL_CB_CUSTOM, error_handler, (void*)bun);
	nl_cb_set(bun->cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, (void*)bun);
	nl_cb_set(bun->cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, (void*)bun);

	bun->nl_sock = nl_socket_alloc_cb(bun->cb);
	int err = genl_connect(bun->nl_sock);
	if (err) {
		// TODO
	}

	bun->nl80211_id = genl_ctrl_resolve(bun->nl_sock, NL80211_GENL_NAME);

	return 0;
}

int setup_scan_event_sock(struct netlink_ev* nlev)
{
	nlev->cookie = NETLINK_EV_COOKIE;

	/* from iw event.c */
	/* no sequence checking for multicast messages */
	nlev->cb = nl_cb_alloc(NL_CB_DEFAULT);
	nl_cb_set(nlev->cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
	nl_cb_set(nlev->cb, NL_CB_VALID, NL_CB_CUSTOM, on_scan_event_valid_handler, (void*)nlev);

	nlev->nl_sock = nl_socket_alloc_cb(nlev->cb);
	int err = genl_connect(nlev->nl_sock);
	DBG("genl_connect err=%d\n", err);
	if (err) {
		// TODO
	}

//	int nl80211_id = genl_ctrl_resolve(nl_sock, NL80211_GENL_NAME);
//	DBG("nl80211_id=%d\n", nl80211_id);
	int mcid = get_multicast_id(nlev->nl_sock, "nl80211", "scan");
	DBG("mcid=%d\n", mcid);
	nl_socket_add_membership(nlev->nl_sock, mcid);

	err = setup_scan_netlink_socket(&nlev->scan_sock);
	if (err) {
		// TODO
	}

	return 0;
}

int main (void)
{
	struct ev_loop* loop = EV_DEFAULT;
	struct MHD_Daemon *daemon;

	DBG("using libev %d %d\n", ev_version_major(), ev_version_minor());
	DBG("libev recommended=%#x\n", ev_recommended_backends());

//	daemon = MHD_start_daemon ( MHD_NO_FLAG, PORT, 
//			on_client_connect, NULL,
//			&answer_to_connection, NULL, 
//			MHD_OPTION_END);
//
//	fd_set read_fd_set, write_fd_set, except_fd_set;
//	MHD_socket max_fd;
//	FD_ZERO(read_fd_set);
//	FD_ZERO(write_fd_set);
//	FD_ZERO(except_fd_set);
//	int ret = MHD_get_fdset (daemon,
//               &read_fd_set,
//               &write_fd_set,
//               &except_fd_set,
//               &max_fd);
//	XASSERT(ret==MHD_YES, ret);

//	ret = MHD_run_from_select(daemon, const fd_set *read_fd_set,
//								const fd_set *write_fd_set,
//								const fd_set *except_fd_set);

	struct netlink_ev nl_watcher;
	int err = setup_scan_event_sock(&nl_watcher);
	if (err) {
		// TODO
	}

	int nl_sock_fd = nl_socket_get_fd(nl_watcher.nl_sock);
	ev_io_init(&nl_watcher.io, on_scan_event_ev_cb, nl_sock_fd, EV_READ);
	ev_io_start(loop, &nl_watcher.io);

	nl_sock_fd = nl_socket_get_fd(nl_watcher.scan_sock.nl_sock);
	ev_io_init(&nl_watcher.scan_sock.io, on_scan_result_ev_cb, nl_sock_fd, EV_READ);
	ev_io_start(loop, &nl_watcher.scan_sock.io);

	daemon = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD, PORT, 
			on_client_connect, NULL,
			&answer_to_connection, NULL, 
			MHD_OPTION_END);
	if (NULL == daemon) return 1;

	printf("daemon running...\n");
//	getchar ();

	ev_run(loop, 0);

	MHD_stop_daemon (daemon);
	return 0;
}

