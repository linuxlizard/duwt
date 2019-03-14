#include <iostream>
#include <vector>

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
#include "iw.h"
#include "util.h"

class NLA_Policy
{

};

class BSS_Policy : public NLA_Policy
{

public:
	BSS_Policy();

	struct nla_policy policy[NL80211_BSS_MAX+1];
};

BSS_Policy::BSS_Policy() : NLA_Policy()
{
	policy[NL80211_BSS_TSF].type = NLA_U64 ;
	policy[NL80211_BSS_FREQUENCY].type = NLA_U32 ;
	policy[NL80211_BSS_BSSID].minlen = ETH_ALEN;
	policy[NL80211_BSS_BSSID].maxlen = ETH_ALEN;
	policy[NL80211_BSS_BEACON_INTERVAL].type = NLA_U16 ;
	policy[NL80211_BSS_CAPABILITY].type = NLA_U16 ;
//	policy[NL80211_BSS_INFORMATION_ELEMENTS];
	policy[NL80211_BSS_SIGNAL_MBM].type = NLA_U32 ;
	policy[NL80211_BSS_SIGNAL_UNSPEC].type = NLA_U8 ;
	policy[NL80211_BSS_STATUS].type = NLA_U32 ;
	policy[NL80211_BSS_SEEN_MS_AGO].type = NLA_U32 ;
//	policy[NL80211_BSS_BEACON_IES];
}

String_IE::String_IE(uint8_t id, uint8_t len, uint8_t *buf) 
	: IE(id, len), s((char *)buf, len)
{
	// buf won't be null terminated
}

String_IE::String_IE(uint8_t id, uint8_t len, char *s) 
	: IE(id, len), s(s)
{
}

Cfg80211::Cfg80211() :
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

Cfg80211::~Cfg80211()
{
	if (nl_sock) {
		nl_socket_free(nl_sock);
	}
	if (s_cb) {
		nl_cb_put(s_cb);
	}
}

Cfg80211::Cfg80211(Cfg80211&& src)
	: nl_sock(src.nl_sock), nl80211_id(src.nl80211_id), s_cb(src.s_cb)
{
	// move constructor
	src.nl_sock = nullptr;
	src.nl80211_id = -1;
	src.s_cb = nullptr;
}

Cfg80211& Cfg80211::operator=(Cfg80211&& src)
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

int Cfg80211::get_scan(const char *iface, std::vector<BSS>& bss_list)
{
	struct nl80211_state state = {
		.nl_sock = nl_sock,
		.nl80211_id = nl80211_id
	};

	struct nlattr_list attrlist {};

	int retcode = iw_get_scan(&state, iface, &attrlist);

//	std::vector<BSS> bss_list;

//	BSS* network;
	BSS_Policy bss_policy;

	for (size_t i=0 ; i<attrlist.counter ; i++) {
		printf("%s %ld %p\n", __func__, i, attrlist.attr_list[i]);

//		hex_dump("attr", (unsigned char*)attrlist.attr_list[i], 
//				attrlist.attr_len_list[i]);

		struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
		retcode = nla_parse(tb_msg, NL80211_ATTR_MAX, 
					(struct nlattr*)attrlist.attr_list[i],
			  				attrlist.attr_len_list[i], NULL);
		printf("parse retcode=%d\n", retcode);

		for (int i=0 ; i<NL80211_ATTR_MAX ; i++ ) {
			if (tb_msg[i]) {
				printf("%s %d=%p type=%d len=%d\n", __func__, i, (void *)tb_msg[i], nla_type(tb_msg[i]), nla_len(tb_msg[i]));
			}
		}

		bss_list.emplace_back();

		struct nlattr *bss[NL80211_BSS_MAX + 1];

		if (tb_msg[NL80211_ATTR_BSS]) {
			if (nla_parse_nested(bss, NL80211_BSS_MAX,
						 tb_msg[NL80211_ATTR_BSS],
						 bss_policy.policy)) {
				std::cerr << "failed to parse nested attributes!\n";
				continue;
			}

			if (!bss[NL80211_BSS_BSSID]) {
				std::cerr << "bssid missing\n";
				continue;
			}

			struct nlattr *attr = bss[NL80211_BSS_BSSID];
			hex_dump("bss", (unsigned char *)nla_data(attr), nla_len(attr));
			memcpy( bss_list.back().bssid, (unsigned char *)nla_data(attr), nla_len(attr));
			if (bss[NL80211_BSS_FREQUENCY]) {
				bss_list.back().freq = nla_get_u32(bss[NL80211_BSS_FREQUENCY]);
			}
			if (bss[NL80211_BSS_SEEN_MS_AGO]) {
				bss_list.back().age = nla_get_u32(bss[NL80211_BSS_SEEN_MS_AGO]);
			}

			// following 'if' copied mostly from iw scan.c
			if (bss[NL80211_BSS_INFORMATION_ELEMENTS] ) {
				struct nlattr *ies = bss[NL80211_BSS_INFORMATION_ELEMENTS];
				struct nlattr *bcnies = bss[NL80211_BSS_BEACON_IES];

				if (bss[NL80211_BSS_PRESP_DATA] ||
					// TODO wtf does this test do?
					(bcnies && (nla_len(ies) != nla_len(bcnies) ||
						memcmp(nla_data(ies), nla_data(bcnies),
							   nla_len(ies))))) {
					std::cout << "found bytes=" << nla_len(ies) << " Information elements from Probe Response frame\n" ;
					uint8_t *ie = (uint8_t *)nla_data(ies);
					ssize_t ielen = (ssize_t)nla_len(ies);
					hex_dump("ie", ie, ielen);

					// quick and dirty get the SSID (Usually the first)
					if (ie[0] == 0) {
						String_IE ssid {ie[0], ie[1], ie+2};
						std::cout << "SSID=" << ssid << "\n";
					}
//				print_ies(nla_data(ies), nla_len(ies),
//					  params->unknown, params->type);
				}
			}
			// following 'if' copied from iw scan.c
			if (bss[NL80211_BSS_BEACON_IES] ) {
				struct nlattr *bcnies = bss[NL80211_BSS_BEACON_IES];
				std::cout << "found bytes="  << nla_len(bss[NL80211_BSS_BEACON_IES]) << " Information elements from Beacon frame\n";
				uint8_t *ie = (uint8_t *)nla_data(bcnies);
				ssize_t ielen = (ssize_t)nla_len(bcnies);
				hex_dump("bie", ie, ielen);
//				print_ies(nla_data(bss[NL80211_BSS_BEACON_IES]),
//					  nla_len(bss[NL80211_BSS_BEACON_IES]),
//					  params->unknown, params->type);
			}

		}
		// TODO what if I don't get a valid record (with no BSSID)?
	}

	for (size_t i=0 ; i<attrlist.counter ; i++) {
		free(attrlist.attr_list[i]);
	}

	return retcode;
}

std::ostream& operator<<(std::ostream& os, const BSS& bss)
{
	char mac_addr[20];
	mac_addr_n2a(mac_addr, (const unsigned char *)bss.bssid);
	os << mac_addr;
	return os;
}

std::ostream& operator<<(std::ostream& os, const String_IE& ie)
{
	os << ie.s;
	return os;
}

