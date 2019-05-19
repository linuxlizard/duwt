#include <stdio.h>
#include <cassert>

#include <iostream>
#include <iterator>
#include <sstream>
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

#include "fmt/format.h"
#include "logging.h"
#include "cfg80211.h"
#include "iw.h"
#include "ie.h"
#include "attr.h"

using namespace cfg80211;

class NLA_Policy
{

};

class BSS_Policy : public NLA_Policy
{

public:
	BSS_Policy();
	// C++ doesn't support designated initializers so we'll build a class
	// instead
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
	policy[NL80211_BSS_INFORMATION_ELEMENTS].type = NLA_UNSPEC;
	policy[NL80211_BSS_SIGNAL_MBM].type = NLA_U32 ;
	policy[NL80211_BSS_SIGNAL_UNSPEC].type = NLA_U8 ;
	policy[NL80211_BSS_STATUS].type = NLA_U32 ;
//	policy[NL80211_BSS_SEEN_MS_AGO].type = NLA_U32 ;
	policy[NL80211_BSS_BEACON_IES].type = NLA_UNSPEC;
}

BSS::BSS(uint8_t *bssid)
{
	memcpy(this->bssid.data(), bssid, ETH_ALEN);

	bssid_str = fmt::format("{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
					bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
}

std::string BSS::get_ssid(void)
{
	// FIXME this method is stupid and needs to be fixed.
	// Dangerous to assume the SSID isn't UTF8.
	// Linear search is stupid.
	//
	// https://en.cppreference.com/w/cpp/language/range-for
	for (const auto& ie : ie_list) {
		if ((*ie).get_id() == 0) {
			return (*ie).str();
		}
	}

	return std::string("(SSID)not found)");
}


Cfg80211::Cfg80211() :
	nl_sock(std::make_unique<NLSock>()),
	nl_cb(std::make_unique<NLCallback>()),
	nl80211_id(-1)
{
	logger = spdlog::get("cfg80211");
	if (!logger) {
		logger = spdlog::stdout_logger_mt("cfg80211");
	}

	logger->info("hello world");

	if (nl_sock == nullptr) {
		logger->error("nl_socket_alloc() failed");
		// TODO throw something
	}

	if (nl_cb == nullptr) {
		logger->error("nl_cb_alloc() failed");
		// TODO throw something
	}

	int retcode = genl_connect(nl_sock->my_sock);
	if (retcode < 0) {
		logger->error("genl_connect failed err={}", retcode);
		throw NetlinkException("genl_connect failed", retcode);
	}

	nl80211_id = genl_ctrl_resolve(nl_sock->my_sock, NL80211_GENL_NAME);
	if (nl80211_id < 0) {
		logger->error("genl_ctrl_resolve of {} failed err={}", NL80211_GENL_NAME, nl80211_id );
		throw NetlinkException(fmt::format("genl_ctrl_resolve name={} failed err={}", NL80211_GENL_NAME, nl80211_id));
	}
}

Cfg80211::~Cfg80211()
{
	// anything?
}

Cfg80211::Cfg80211(Cfg80211&& src)
	: nl_sock(std::move(src.nl_sock)), 
	nl_cb(std::move(src.nl_cb)), 
	nl80211_id(src.nl80211_id),
	logger(std::move(src.logger))
{
	// move constructor
	src.nl80211_id = -1;
}

Cfg80211& Cfg80211::operator=(Cfg80211&& src)
{
	// move assignment
	nl_sock = std::move(src.nl_sock);
	nl_cb = std::move(src.nl_cb);

	nl80211_id = src.nl80211_id;
	src.nl80211_id = -1;

	logger = std::move(src.logger);

	return *this;
}

int Cfg80211::get_scan(const char *iface, std::vector<BSS>& bss_list)
{
	struct nl80211_state state = {
		nl_sock->my_sock,
		// TODO use the callback in GET_SCAN ???
		nullptr,
		nl80211_id
	};
	struct nlattr_list attrlist {};

	logger->info("get_scan iface={}", iface);

	int retcode = iw_get_scan(&state, iface, &attrlist);
	if (retcode < 0) {
		logger->error("get_scan iface={} iw_get_scan failed err={}", iface, retcode);
		assert(0);
		// TODO throw something
	}

	BSS_Policy bss_policy;

	for (size_t i=0 ; i<attrlist.counter ; i++) {
		logger->debug("get_scan attr i={} len={}", i, attrlist.attr_len_list[i]);

//		hex_dump("attr", (unsigned char*)attrlist.attr_list[i], 
//				attrlist.attr_len_list[i]);

		struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
		retcode = nla_parse(tb_msg, NL80211_ATTR_MAX, 
					attrlist.attr_list[i],
			  				attrlist.attr_len_list[i], NULL);
		assert(retcode==0);
		logger->debug("get_scan parse retcode={}", retcode);

		for (int msgidx=0 ; msgidx<NL80211_ATTR_MAX ; msgidx++ ) {
			if (tb_msg[msgidx]) {
				logger->debug("{} {}={} type={} len={}\n", __func__, 
						msgidx, (void *)tb_msg[msgidx], nla_type(tb_msg[msgidx]), nla_len(tb_msg[msgidx]));
			}
		}

		struct nlattr *bss[NL80211_BSS_MAX + 1] { };

		// TODO capture other fields in the tb_msg[]

		if (tb_msg[NL80211_ATTR_BSS]) {
//			decode_attr_bss(tb_msg[NL80211_ATTR_BSS]);

			if (nla_parse_nested(bss, NL80211_BSS_MAX,
						 tb_msg[NL80211_ATTR_BSS],
						 bss_policy.policy)) {
				// TODO throw something
				std::cerr << "failed to parse nested attributes!\n";
				continue;
			}

			if (!bss[NL80211_BSS_BSSID]) {
				std::cerr << "bssid missing\n";
				// TODO throw something (?)
				continue;
			}

			BSS& new_bss = bss_list.emplace_back((uint8_t*)nla_data(bss[NL80211_BSS_BSSID]));
			auto ssid = new_bss.get_ssid();
			auto bssid = new_bss.get_bssid();
			logger->debug("found bssid={}", new_bss.get_bssid());

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
					logger->debug("found bytes={} IEs in Probe Response frame", nla_len(ies));
					uint8_t *ie = (uint8_t *)nla_data(ies);
					ssize_t ielen = (ssize_t)nla_len(ies);
					uint8_t *ie_end = ie + ielen;
					size_t counter=0;
					while (ie < ie_end) {
						// XXX temp debug
//						IE new_ie = IE(ie[0], ie[1], ie+2);

						new_bss.add_ie(cfg80211::make_ie(ie[0], ie[1], ie+2));
//						new_bss.ie_list.emplace_back(ie[0], ie[1], ie+2);

//						if (ie[0] == 0) {
//							std::cout << "P BSS=" << new_bss << " SSID:" << new_bss.ie_list.back().str() << std::endl;
//						}
						// XXX temp debug
//						std::cout << __func__ << " ie_list last=" << new_bss.ie_list.back() << "\n";

						ie += ie[1] + 2;
						counter++;
					}
					logger->debug("found count={} IEs in Probe Response frame", counter);
				}
			}
			// following 'if' copied from iw scan.c
			if (bss[NL80211_BSS_BEACON_IES] ) {
				struct nlattr *bcnies = bss[NL80211_BSS_BEACON_IES];
				logger->debug("found bytes={} IEs in Beacon frame", nla_len(bss[NL80211_BSS_BEACON_IES]));
				uint8_t *ie = (uint8_t *)nla_data(bcnies);
				ssize_t ielen = (ssize_t)nla_len(bcnies);
				uint8_t *ie_end = ie + ielen;
				size_t counter = 0;
				while (ie < ie_end) {
					new_bss.add_ie(cfg80211::make_ie(ie[0], ie[1], ie+2));
//					new_bss.ie_list.emplace_back(ie[0], ie[1], ie+2);

//					if (ie[0] == 0) {
//						std::cout << "B BSS=" << new_bss << " SSID:" << new_bss.ie_list.back().str() << std::endl;
//					}

					ie += ie[1] + 2;
					counter++;
				}
				logger->debug("found count={} IEs in Beacon frame", counter);
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

int Cfg80211::listen_scan_events(void)
{
	struct nl80211_state state = {
		nl_sock->my_sock,
		nl_cb->my_cb,
		nl80211_id
	};

	std::cout << __func__ << "\n";
	int err = iw_listen_scan_events(&state);
	// TODO check error
	(void)err;

	return 0;
};

void Cfg80211::fetch_scan_events(ScanEvent& sev)
{
	struct nl80211_state state = {
		nl_sock->my_sock,
		nl_cb->my_cb,
		nl80211_id
	};

	struct nlattr_list attrlist {};

	std::cout << __func__ << "\n";
	int err = iw_fetch_scan_events(&state, &attrlist);
	if (err) {
		throw NetlinkException("iw_fetch_scan_events failed", err);
	}

	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1] {};

	err = nla_parse(tb_msg, NL80211_ATTR_MAX, 
				attrlist.attr_list[0],
						attrlist.attr_len_list[0], NULL);
	// TODO check error
	assert(err==0);

	for (int i=0 ; i<NL80211_ATTR_MAX ; i++ ) {
		if (tb_msg[i]) {
			printf("%s %d=%p type=%d len=%d\n", __func__, i, (void *)tb_msg[i], nla_type(tb_msg[i]), nla_len(tb_msg[i]));
		}
	}

//	decode_nl80211_attr(tb_msg, NL80211_ATTR_MAX);

	// TODO copy guts of event.c print_event() 

	if (tb_msg[NL80211_ATTR_WIPHY]) {
		sev.phyidx = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]);
	}

	if (tb_msg[NL80211_ATTR_WIPHY_NAME]) {
		sev.phyname.append(nla_get_string(tb_msg[NL80211_ATTR_WIPHY_NAME]));
	}

	if (tb_msg[NL80211_ATTR_IFINDEX]) {
		sev.ifidx = nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]);
	}

	if (tb_msg[NL80211_ATTR_IFNAME]) {
		sev.ifname = nla_get_string(tb_msg[NL80211_ATTR_IFNAME]);
	}

	sev.frequencies.clear();
	if (tb_msg[NL80211_ATTR_SCAN_FREQUENCIES]) {
		struct nlattr *nst;
		int rem_nst;
		size_t len = nla_len(tb_msg[NL80211_ATTR_SCAN_FREQUENCIES]);
		std::cout << "len=" << len << "\n";
		nla_for_each_nested(nst, tb_msg[NL80211_ATTR_SCAN_FREQUENCIES], rem_nst) {
			sev.frequencies.push_back(nla_get_u32(nst));
		}
	}

#if 0
	std::cout << "scan event on ifname=" << ifname << " ifidx=" << ifidx << "\n";
	std::cout << "scan event on phyname=" << phyname << " phyidx=" << phyidx << "\n";

	std::cout << "found count=" << frequencies.size() << " freq\n";
//	for (auto f : frequencies) {
//		std::cout << "f=" << f << "\n";
//	}
	// https://en.cppreference.com/w/cpp/algorithm/copy
	std::copy(frequencies.cbegin(), frequencies.cend(), 
				std::ostream_iterator<uint32_t>(std::cout, " "));
	std::cout << "\n";

	std::ostringstream s;
	std::copy(frequencies.cbegin(), frequencies.cend(), 
				std::ostream_iterator<uint32_t>(s, ","));
	std::string joined = s.str();
	joined.pop_back();
	std::cout << joined << "*\n";
#endif
};

std::ostream& operator<<(std::ostream& os, const BSS& bss)
{
//	char mac_addr[20];
//	mac_addr_n2a(mac_addr, (const unsigned char *)bss.bssid.data());
//	os << mac_addr;
	os << bss.get_bssid();
	return os;
}

