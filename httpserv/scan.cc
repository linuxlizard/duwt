/*
 * scan.cc  nl80211 scanning thread using my libiw
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <iostream>
#include <unordered_map>
#include <cstdint>
#include <cinttypes>

#include <sys/epoll.h>

// netlink
#include <linux/if_ether.h>
#include <linux/nl80211.h>
#include <netlink/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>

#include "core.h"
#include "iw.h"
#include "bss.h"
#include "nlnames.h"
#include "scan.h"
#include "survey.h"

static int survd_debug = 0;

static const int MAX_EVENTS = 32;
static const size_t MAX_DEV_LIST = 16;

Survey survey;

#define BUNDLE_COOKIE 0x01448f44
struct netlink_socket_bundle 
{
	uint32_t cookie;
	const char* name;

	struct nl_cb* cb;
	struct nl_sock* nl_sock;
	int sock_fd;
	int nl80211_id;
	int cb_err;
	void* more_args;

	// if we're already reading a CMD_GET_SCAN result, don't push another
	// request for scan results.
	// https://github.com/linuxlizard/duwt/issues/3
	bool busy;
};

// netlink callback on the NL80211_CMD_GET_SCAN message
static int scan_survey_valid_handler(struct nl_msg *msg, void *arg)
{
	INFO("%s\n", __func__);

	if (survd_debug > 1) {
		nl_msg_dump(msg, stdout);
	}

	struct netlink_socket_bundle* bun = (struct netlink_socket_bundle*)arg;
	XASSERT(bun->cookie == BUNDLE_COOKIE, bun->cookie);

	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	INFO("%s len=%" PRIu32 " type=%u flags=%u seq=%" PRIu32 " pid=%" PRIu32 "\n", __func__,
			hdr->nlmsg_len, hdr->nlmsg_type, hdr->nlmsg_flags, 
			hdr->nlmsg_seq, hdr->nlmsg_pid);

	struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(hdr);

	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
	int err = nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);
	XASSERT(err==0, err);

	peek_nla_attr(tb_msg, NL80211_ATTR_MAX);

	int ifidx = -1;
	if (tb_msg[NL80211_ATTR_IFINDEX]) {
		ifidx = nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]);
		DBG("%s ifindex=%d\n", __func__, ifidx);
	}

	struct BSS* new_bss = NULL;
	err = bss_from_nlattr(tb_msg, &new_bss);
	if (err) {
		ERR("%s bss_from_nlattr failed err=%d\n", __func__, err);
		return NL_SKIP;
	}

	survey.store(new_bss);

	DBG("%s success\n", __func__);
	return NL_OK;
}

// libnl callback 
static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	(void)nla;

	// TODO better error decode
	ERR("%s seq=%" PRIu32 " error=%d: %s\n", __func__, err->msg.nlmsg_seq, err->error, strerror(err->error));

	struct netlink_socket_bundle* bun = (struct netlink_socket_bundle*)arg;
	XASSERT(bun->cookie == BUNDLE_COOKIE, bun->cookie);
	bun->busy = false;

	return NL_STOP;
}

// libnl callback
static int finish_handler(struct nl_msg *msg, void *arg)
{
	DBG("%s\n", __func__);

	if (survd_debug > 1) {
		nl_msg_dump(msg, stdout);
	}

	(void)msg;

	struct netlink_socket_bundle* bun = (struct netlink_socket_bundle*)arg;
	XASSERT(bun->cookie == BUNDLE_COOKIE, bun->cookie);
	bun->cb_err = 0;
	bun->busy = false;

	return NL_SKIP;
}

// libnl callback
static int ack_handler(struct nl_msg *msg, void *arg)
{
	DBG("%s\n", __func__);

	if (survd_debug > 1) {
		nl_msg_dump(msg, stdout);
	}

	(void)msg;

	int *ret = (int*)(arg);
	*ret = 0;
	return NL_STOP;
}

// libnl callback
static int no_seq_check(struct nl_msg *msg, void *arg)
{
	INFO("%s\n", __func__);

	if (survd_debug > 1) {
		nl_msg_dump(msg, stdout);
	}

	(void)msg;
	(void)arg;

	return NL_OK;
}

// libnl callback for the Scan Event received
static int on_scan_event_valid_handler(struct nl_msg *msg, void *arg)
{
	DBG("%s %p %p\n", __func__, (void *)msg, arg);

	struct netlink_socket_bundle* bun = (struct netlink_socket_bundle*)arg;
	XASSERT(bun->cookie == BUNDLE_COOKIE, bun->cookie);

//	hex_dump("msg", (const unsigned char *)msg, 128);

	if (survd_debug > 1) {
		nl_msg_dump(msg,stdout);
	}

	struct nlmsghdr *hdr = nlmsg_hdr(msg);

	int datalen = nlmsg_datalen(hdr);
	DBG("datalen=%d attrlen=%d\n", datalen, nlmsg_attrlen(hdr,0));

	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];

	struct genlmsghdr *gnlh = (struct genlmsghdr*)nlmsg_data(nlmsg_hdr(msg));
	int err = nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);
	if (err) {
		// TODO
		XASSERT(0,err);
	}


	DBG("%s cmd=%d %s\n", __func__, (int)(gnlh->cmd), to_string_nl80211_commands((enum nl80211_commands)gnlh->cmd));

	// debug dump
	for (int i=0 ; i<NL80211_ATTR_MAX ; i++ ) {
		if (tb_msg[i]) {
			const char *name = to_string_nl80211_attrs((enum nl80211_attrs)i);
			DBG("%s i=%d %s at %p type=%d len=%d\n", __func__, 
				i, name, (void *)tb_msg[i], nla_type(tb_msg[i]), nla_len(tb_msg[i]));
		}
	}

	int ifidx = -1;
	if (tb_msg[NL80211_ATTR_IFINDEX]) {
		ifidx = nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]);
		DBG("%s ifindex=%d\n", __func__, ifidx);
	}

	// if new scan results,
	// send the get scan results message
	if ( (gnlh->cmd == NL80211_CMD_NEW_SCAN_RESULTS || gnlh->cmd == NL80211_CMD_SCHED_SCAN_RESULTS) && ifidx != -1) {
		struct netlink_socket_bundle* scan_sock_bun = (struct netlink_socket_bundle*)bun->more_args;
		XASSERT(scan_sock_bun->cookie == BUNDLE_COOKIE, scan_sock_bun->cookie);

		// https://github.com/linuxlizard/duwt/issues/3  don't start another fetch while a fetch is already running
		if (scan_sock_bun->busy) {
			WARN("%s not starting a new NL80211_CMD_GET_SCAN while a get already running\n", __func__);
		}
		else {
			struct nl_msg* new_msg = nlmsg_alloc();
			XASSERT(new_msg,0);

			void* p = genlmsg_put(new_msg, NL_AUTO_PORT, NL_AUTO_SEQ, 
								scan_sock_bun->nl80211_id, 
								0, 
								NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
			XASSERT(p,0);
			nla_put_u32(new_msg, NL80211_ATTR_IFINDEX, ifidx);
			int ret = nl_send_auto(scan_sock_bun->nl_sock, new_msg);
			if (ret < 0) {
				// TODO
				XASSERT(0,ret);
			}
			if (survd_debug > 1) {
				nl_msg_dump(new_msg, stdout);
			}
			DBG("%s sent NL80211_CMD_GET_SCAN bytes=%d\n", __func__, ret);
			scan_sock_bun->busy = true;
		}
	}

	return NL_SKIP;
}

int setup_scan_netlink_socket(struct netlink_socket_bundle* bun)
{
	// this is the netlink socket we'll use to get scan results
	memset(bun, 0, sizeof(struct netlink_socket_bundle));
	bun->cookie = BUNDLE_COOKIE;
	bun->name = "scanresults";

	bun->cb = nl_cb_alloc(NL_CB_DEFAULT);

	int err;
	err = nl_cb_set(bun->cb, NL_CB_VALID, NL_CB_CUSTOM, scan_survey_valid_handler, (void*)bun);
	XASSERT(err>=0,err);
	err = nl_cb_err(bun->cb, NL_CB_CUSTOM, error_handler, (void*)bun);
	XASSERT(err>=0,err);
	err = nl_cb_set(bun->cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, (void*)bun);
	XASSERT(err>=0,err);
	err = nl_cb_set(bun->cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, (void*)&bun->cb_err);
	XASSERT(err>=0,err);

	bun->nl_sock = nl_socket_alloc_cb(bun->cb);
	XASSERT(bun->nl_sock, 0);

	err = genl_connect(bun->nl_sock);
	if (err) {
		// TODO
		XASSERT(0,err);
	}

	bun->nl80211_id = genl_ctrl_resolve(bun->nl_sock, NL80211_GENL_NAME);
	XASSERT(bun->nl80211_id >= 0, bun->nl80211_id);

	bun->sock_fd = nl_socket_get_fd(bun->nl_sock);
	XASSERT(bun->sock_fd >= 0, bun->sock_fd);

	return 0;
}

int setup_scan_event_sock(struct netlink_socket_bundle* bun)
{
	int err;

	memset(bun, 0, sizeof(struct netlink_socket_bundle));
	bun->cookie = BUNDLE_COOKIE;
	bun->name = "events";

	/* from iw event.c */
	/* no sequence checking for multicast messages */
	bun->cb = nl_cb_alloc(NL_CB_DEFAULT);
	err = nl_cb_set(bun->cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
	XASSERT(err>=0,err);
	err = nl_cb_set(bun->cb, NL_CB_VALID, NL_CB_CUSTOM, on_scan_event_valid_handler, (void*)bun);
	XASSERT(err>=0,err);

	bun->nl_sock = nl_socket_alloc_cb(bun->cb);
	err = genl_connect(bun->nl_sock);
	DBG("genl_connect err=%d\n", err);
	if (err) {
		// TODO
		XASSERT(0,err);
	}

	bun->nl80211_id = genl_ctrl_resolve(bun->nl_sock, NL80211_GENL_NAME);
	XASSERT(bun->nl80211_id >= 0, bun->nl80211_id);

	int mcid = genl_ctrl_resolve_grp(bun->nl_sock, "nl80211", "scan");
	DBG("mcid=%d\n", mcid);
	err = nl_socket_add_membership(bun->nl_sock, mcid);
	XASSERT(err >=0, err);

	bun->sock_fd = nl_socket_get_fd(bun->nl_sock);
	XASSERT(bun->sock_fd >= 0, bun->sock_fd);

	return 0;
}

static int netlink_read(struct netlink_socket_bundle* bun)
{
	DBG("%s on %s\n", __func__, bun->name);
	do {
		int err = nl_recvmsgs(bun->nl_sock, bun->cb);
		DBG("%s err=%d cb_err=%d\n", __func__, err, bun->cb_err);
//		if (err < 0) {
//			// TODO
//			ERR("%s on %s err=%d: %s\n", __func__, bun->name, err, nl_geterror(err));
//			break;
//		}
	} while (bun->cb_err > 0);

	return 0;
}

class ScanningThread::ScanningInternal
{
public:
	ScanningInternal() : counter(0), 
						stop_flag(false)
		{ std::cout << "hello from ScanningInternal\n"; };
	~ScanningInternal() =default;

	uint64_t get_counter(void) {
		return this->counter++;
	};

	int init();
	int run();

	void stop(void) {
		stop_flag = true;
	};

private:
	uint64_t counter;
	std::atomic<bool> stop_flag;
	struct netlink_socket_bundle scan_event_watcher;
	struct netlink_socket_bundle scan_results_watcher;
};


int ScanningThread::ScanningInternal::init() 
{
	log_set_level(LOG_LEVEL_DEBUG);

	int err = setup_scan_event_sock(&scan_event_watcher);
	if (err) {
		// TODO
		XASSERT(0,err);
	}

	err = setup_scan_netlink_socket(&scan_results_watcher);
	if (err) {
		// TODO
		XASSERT(0,err);
	}

	// attach the scan results watcher to the scan event watcher so we can call
	// GET_SCAN_RESULTS from the watcher's libnl callback
	scan_event_watcher.more_args = (void*)&scan_results_watcher;

	scan_results_watcher.more_args = nullptr;

	return 0;
}

int ScanningThread::ScanningInternal::run() 
{
	int ret, err;
	int epoll_fd;

	epoll_fd = epoll_create1(0);
	if (epoll_fd < 0) {
		err = -errno;
		ERR("%s epoll_create1 failed errno=%d %s\n", __func__, errno, strerror(errno));
		return err;
	}

	struct epoll_event ev;

	memset(&ev, 0, sizeof(struct epoll_event));
	ev.events = EPOLLIN;
	ev.data.ptr = (void *)&scan_event_watcher;
	ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, scan_event_watcher.sock_fd, &ev);
	if (ret < 0) {
		err = -errno;
		ERR("%s epoll ctl scan event failed errno=%d %s\n", __func__, errno, strerror(errno));
		return err;
	}

	memset(&ev, 0, sizeof(struct epoll_event));
	ev.events = EPOLLIN;
	ev.data.ptr = (void *)&scan_results_watcher;
	ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, scan_results_watcher.sock_fd, &ev);
	if (ret < 0) {
		err = -errno;
		ERR("%s epoll ctl scan results failed errno=%d %s\n", __func__, errno, strerror(errno));
		return err;
	}

	struct epoll_event events[MAX_EVENTS];

	int final_err = 0;
	while( !stop_flag ) {
//		std::cout << "hello " << get_counter() << " from thread\n" ;
//		std::this_thread::sleep_for(std::chrono::seconds(1));

		DBG("%s epoll wait...\n", __func__);
		int num_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		DBG("%s epoll_wait returned num_fds=%d\n", __func__, num_fds);

		if (num_fds < 0) {
			// error or signal
			if (errno == EINTR) {
				// your system call cannot be completed at this time; please
				// try your call again
				continue;
			}
			else {
				// something went wrong
				final_err = -errno;
				ERR("%s epoll wait failed errno=%d %s\n", __func__, errno, strerror(errno));
				break;
			}
		}

		for ( int i=0 ; i<num_fds ; i++) {
			ret = netlink_read((struct netlink_socket_bundle*)events[i].data.ptr);

		}
	}

	return final_err;
}


ScanningThread::ScanningThread() : 
	thd(run, this),
	worker{std::make_unique<ScanningInternal>()}
{ 
}

ScanningThread::~ScanningThread()  { }

int ScanningThread::init_worker(void)
{
	int ret;
	struct iw_dev dev_list[MAX_DEV_LIST];
	size_t count = MAX_DEV_LIST;

	ret = iw_dev_list(dev_list, &count);
	if (ret != 0) {
		return ret;
	}

	worker->init();
	return 0;
}

int ScanningThread::run_worker(void)
{
	// will run forever or until stop is called
	return worker->run();
}

void ScanningThread::stop(void)
{
	worker->stop();
}

