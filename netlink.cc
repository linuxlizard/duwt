#include <stdio.h>
#include <cassert>

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
#include "ie.hh"

class NLA_Policy
{

};

class BSS_Policy : public NLA_Policy
{

public:
	BSS_Policy();
#if 0
	static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
		[NL80211_BSS_TSF] = { .type = NLA_U64 },
		[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_BSS_BSSID] = { },
		[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
		[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
		[NL80211_BSS_INFORMATION_ELEMENTS] = { },
		[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
		[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
		[NL80211_BSS_STATUS] = { .type = NLA_U32 },
	};
#endif
	struct nla_policy policy[NL80211_BSS_MAX+1];
};

BSS_Policy::BSS_Policy() : NLA_Policy()
{
	memset(policy, 0, sizeof(policy));
	policy[NL80211_BSS_TSF].type = NLA_U64 ;
	policy[NL80211_BSS_FREQUENCY].type = NLA_U32 ;
//	policy[NL80211_BSS_BSSID].minlen = ETH_ALEN;
//	policy[NL80211_BSS_BSSID].maxlen = ETH_ALEN;
	policy[NL80211_BSS_BEACON_INTERVAL].type = NLA_U16 ;
	policy[NL80211_BSS_CAPABILITY].type = NLA_U16 ;
//	policy[NL80211_BSS_INFORMATION_ELEMENTS];
	policy[NL80211_BSS_SIGNAL_MBM].type = NLA_U32 ;
	policy[NL80211_BSS_SIGNAL_UNSPEC].type = NLA_U8 ;
	policy[NL80211_BSS_STATUS].type = NLA_U32 ;
//	policy[NL80211_BSS_SEEN_MS_AGO].type = NLA_U32 ;
//	policy[NL80211_BSS_BEACON_IES];
}

BSS::BSS(uint8_t *bssid)
{
	memcpy(this->bssid.data(), bssid, ETH_ALEN);
}

std::string BSS::get_ssid(void)
{
	for (auto&& ie : ie_list) {
		if (ie.get_id() == 0) {
			return ie.str();
		}
	}

	return std::string("not found");
}

BSS::BSS(BSS&& src)
	: bssid(std::move(src.bssid)),
	  channel_width(src.channel_width),
	  freq(src.freq),
	  center_freq1(src.center_freq1),
	  center_freq2(src.center_freq2),
	  age(src.age),
	  ie_list(std::move(src.ie_list))
{
	// Move constructor
	std::cout << "BSS move constructor\n";
}

BSS& BSS::operator=(BSS&& bss)
{
	// Move assignment operator
	std::cout << "BSS move assignment\n";
	// TODO
	return bss;
}

Cfg80211::Cfg80211() :
	nl_sock(std::make_unique<NLSock>()),
	nl80211_id(-1)
{
	if (nl_sock == nullptr) {
		// nl_socket_alloc() failed
		// TODO throw something
	}

	int retcode = genl_connect(nl_sock->my_sock);
	if (retcode < 0) {
		// TODO throw something
	}

	nl80211_id = genl_ctrl_resolve(nl_sock->my_sock, NL80211_GENL_NAME);
	if (nl80211_id < 0) {
		// TODO throw something
	}

}

Cfg80211::~Cfg80211()
{
	// anything?
}

Cfg80211::Cfg80211(Cfg80211&& src)
	: nl_sock(std::move(src.nl_sock)), 
	nl80211_id(src.nl80211_id)
{
	// move constructor
	src.nl80211_id = -1;
}

Cfg80211& Cfg80211::operator=(Cfg80211&& src)
{
	// move assignment
	nl_sock = std::move(src.nl_sock);

	nl80211_id = src.nl80211_id;
	src.nl80211_id = -1;

	return *this;
}

int Cfg80211::get_scan(const char *iface, std::vector<BSS>& bss_list)
{
	struct nl80211_state state = {
		.nl_sock = nl_sock->my_sock,
		.nl80211_id = nl80211_id
	};

	struct nlattr_list attrlist {};

	int retcode = iw_get_scan(&state, iface, &attrlist);

	BSS_Policy bss_policy;

	for (size_t i=0 ; i<attrlist.counter ; i++) {
		printf("%s %ld %p\n", __func__, i, attrlist.attr_list[i]);

//		hex_dump("attr", (unsigned char*)attrlist.attr_list[i], 
//				attrlist.attr_len_list[i]);

		struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
		retcode = nla_parse(tb_msg, NL80211_ATTR_MAX, 
					attrlist.attr_list[i],
			  				attrlist.attr_len_list[i], NULL);
		assert(retcode==0);
		printf("parse retcode=%d\n", retcode);

		for (int i=0 ; i<NL80211_ATTR_MAX ; i++ ) {
			if (tb_msg[i]) {
				printf("%s %d=%p type=%d len=%d\n", __func__, i, (void *)tb_msg[i], nla_type(tb_msg[i]), nla_len(tb_msg[i]));
			}
		}

		struct nlattr *bss[NL80211_BSS_MAX + 1] { };

		if (tb_msg[NL80211_ATTR_BSS]) {
			decode_attr_bss(tb_msg[NL80211_ATTR_BSS]);

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

			BSS& new_bss = bss_list.emplace_back((uint8_t*)nla_data(bss[NL80211_BSS_BSSID]));

			if (bss[NL80211_BSS_FREQUENCY]) {
				new_bss.freq = nla_get_u32(bss[NL80211_BSS_FREQUENCY]);
			}
			if (bss[NL80211_BSS_SEEN_MS_AGO]) {
				new_bss.age = nla_get_u32(bss[NL80211_BSS_SEEN_MS_AGO]);
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
					uint8_t *ie_end = ie + ielen;
					size_t counter=0;
					while (ie < ie_end) {
//						IE new_ie = IE(ie[0], ie[1], ie+2);
						new_bss.ie_list.emplace_back(ie[0], ie[1], ie+2);
						if (ie[0] == 0) {
							std::cout << "P BSS=" << new_bss << " SSID:" << new_bss.ie_list.back().str() << std::endl;
						}
						ie += ie[1] + 2;
						counter++;
					}
					std::cout << "found " << counter << " ie\n";
				}
			}
			// following 'if' copied from iw scan.c
			if (bss[NL80211_BSS_BEACON_IES] ) {
				struct nlattr *bcnies = bss[NL80211_BSS_BEACON_IES];
				std::cout << "found bytes="  << nla_len(bss[NL80211_BSS_BEACON_IES]) << " Information elements from Beacon frame\n";
				uint8_t *ie = (uint8_t *)nla_data(bcnies);
				ssize_t ielen = (ssize_t)nla_len(bcnies);
				uint8_t *ie_end = ie + ielen;
				size_t counter = 0;
				while (ie < ie_end) {
					new_bss.ie_list.emplace_back(ie[0], ie[1], ie+2);
					if (ie[0] == 0) {
						std::cout << "B BSS=" << new_bss << " SSID:" << new_bss.ie_list.back().str() << std::endl;
					}

					ie += ie[1] + 2;
					counter++;
				}
				std::cout << "found " << counter << " ie\n";
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
	mac_addr_n2a(mac_addr, (const unsigned char *)bss.bssid.data());
	os << mac_addr;
	return os;
}
