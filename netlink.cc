#include <sys/socket.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/nl80211.h>

#include "netlink.hh"

Netlink::Netlink() :
	nl_sock(nullptr), nl80211_id(-1)
{
	/* starting from iw.c nl80211_init() */
	nl_sock = nl_socket_alloc();
    printf("nl_sock=%p\n", static_cast<void *>(nl_sock));

	int retcode = genl_connect(nl_sock);
	printf("genl_connect retcode=%d\n", retcode);

	nl80211_id = genl_ctrl_resolve(nl_sock, NL80211_GENL_NAME);
	if (nl80211_id < 0) {
		nl_socket_free(nl_sock);
//		throw ;
	}

	printf("nl80211_id=%d\n", nl80211_id);

//	struct nl_cb *cb = nl_cb_alloc(NL_CB_DEBUG);
	s_cb = nl_cb_alloc(NL_CB_DEBUG);
}

Netlink::~Netlink()
{
//	if (msg) {
//		nlmsg_free(msg);
//        msg = nullptr;
//	}

	if (nl_sock) {
		nl_socket_free(nl_sock);
	}
	if (s_cb) {
		nl_cb_put(s_cb);
	}
}

Netlink::Netlink(Netlink&& src)
	: nl_sock(src.nl_sock), nl80211_id(src.nl80211_id), s_cb(src.s_cb)
{
	// move constructor
	src.nl_sock = nullptr;
	src.nl80211_id = -1;
	src.s_cb = nullptr;
}

Netlink& Netlink::operator=(Netlink&& src)
{
	// move assignment
	nl_sock = src.nl_sock;
	src.nl_sock = nullptr;

	nl80211_id = src.nl80211_id;
	src.nl80211_id = -1;

	s_cb = src.s_cb;
	src.s_cb = nullptr;

	return *this;
}

int Netlink::get_scan_results(void)
{
	return -1;
}

