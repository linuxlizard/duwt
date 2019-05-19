#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <ostream>
#include <vector>
#include <memory>
#include <stdexcept>

#include <linux/if_ether.h>
#include <linux/nl80211.h>
#include <netlink/socket.h>

#include "fmt/format.h"
#include "spdlog/spdlog.h"

#include "ie.h"

namespace cfg80211 {

class NetlinkException : public std::runtime_error 
{
	public:
		NetlinkException() : std::runtime_error("NetlinkException") {}
		explicit NetlinkException(const char* what) : std::runtime_error(what) {}
		explicit NetlinkException(const std::string& what) : std::runtime_error(what) {}

		explicit NetlinkException(const char* what, int err)
			: std::runtime_error(fmt::format("{} err={}", what, err)) {}

		explicit NetlinkException(const std::string& what, int err)
			: std::runtime_error(fmt::format("{} err={}", what, err)) {}
};

// encapsulate a netlink socket struct so can wrap it in a unique_ptr
class NLSock
{
	public:
		NLSock() 
		{ 
			_logger = spdlog::get("basic_logger");
			if (_logger) {
				_logger->info("info");
				_logger->debug("debug");
			}
			my_sock = nl_socket_alloc(); 
//			std::cout << "class sock=" << my_sock << "\n";
			if (!my_sock) {
				throw NetlinkException("nl_socket_alloc failed");
			}
		}

		~NLSock() 
		{
//			std::cout << "~class sock\n";
			if (my_sock) {
//				std::cout << "class sock nl_socket_free " << my_sock << "\n";
				nl_socket_free(my_sock);
				my_sock = nullptr;
			}
        }

		// TODO can I move my_sock private?
		// http://www.nuonsoft.com/blog/2019/02/01/presentation-modern-c-memory-management/
		// Conversion operator to FILE*
		//   operator FILE*() const { return m_file; }
//		operator struct nl_sock* () const { return my_sock; }

		struct nl_sock* my_sock;

	private:
		std::shared_ptr<spdlog::logger> _logger;
};


class NLCallback
{
	public:
		NLCallback() 
		{ 
			my_cb = nl_cb_alloc(NL_CB_DEBUG);
			if (!my_cb) {
				throw NetlinkException("nl_cb_alloc failed");
			}
        }

		~NLCallback() 
		{
			if (my_cb) {
				nl_cb_put(my_cb);
				my_cb = nullptr;
			}
		}

		struct nl_cb* my_cb;
};

using nl_sock_ptr_class = std::unique_ptr<NLSock>;
using nl_cb_ptr_class = std::unique_ptr<NLCallback>;

struct ScanEvent
{
	ScanEvent() = default;

	ScanEvent(const ScanEvent&) = delete;
//	ScanEvent(const ScanEvent&) { std::cout << "A copy was made.\n"; }
	uint32_t phyidx;
	std::string phyname;
	uint32_t ifidx;
	std::string ifname;
	std::vector<uint32_t> frequencies;
};

// representation of a WiFi BSS
class BSS
{
public:
	BSS() = delete;
	BSS(uint8_t *bssid);
	~BSS() = default;

	// definitely don't want copy constructors
	BSS(const BSS&) = delete; // Copy constructor
	BSS& operator=(const BSS&) = delete;  // Copy assignment operator

	// added move constructors to learn how to use move constructors
	BSS(BSS&&) = default;  // Move constructor
	BSS& operator=(BSS&&) = default; // Move assignment operator
//	BSS(BSS&&);  // Move constructor
//	BSS& operator=(BSS&&); // Move assignment operator

	// TODO take some of this stuff private
	std::array<uint8_t,ETH_ALEN> bssid;
	enum nl80211_chan_width channel_width;
	uint32_t freq;
	uint32_t center_freq1;
	uint32_t center_freq2;
	int age;

	// FIXME this method is awful; need a better way
	// also std::string isn't UTF8 safe
	std::string get_ssid(void);

	std::string get_bssid(void) const { return bssid_str; };

	void add_ie(std::shared_ptr<IE> ie) { ie_list.push_back(ie); }
	size_t ie_count(void) { return ie_list.size(); }

	using IE_List = std::vector<std::shared_ptr<IE>>;
	using const_iterator = typename IE_List::const_iterator;

	const_iterator cbegin() const { return std::cbegin(ie_list); }
	const_iterator cend() const { return std::cend(ie_list); }

private:
//	std::vector<IE> ie_list;
	IE_List ie_list;

	std::string bssid_str;
};

// https://wireless.wiki.kernel.org/en/developers/Documentation/cfg80211
class Cfg80211
{
public:
	// https://en.cppreference.com/w/cpp/language/rule_of_three
	// https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#S-ctor
	Cfg80211();
	~Cfg80211();  // Destructor

	// definitely don't want copy constructors
	Cfg80211(const Cfg80211&) = delete; // Copy constructor
	Cfg80211& operator=(const Cfg80211&) = delete;  // Copy assignment operator

	// added move constructors to learn how to use move constructors
	Cfg80211(Cfg80211&&);  // Move constructor
	Cfg80211& operator=(Cfg80211&&); // Move assignment operator

	// get results of a scan
	int get_scan(const char*iface, std::vector<BSS>& network_list);

	// set the netlink socket to listen for scan events 
	int listen_scan_events(void);

	// fetch a scan event
	void fetch_scan_events(ScanEvent& sev);

	int get_fd(void)
	{
		return nl_socket_get_fd(nl_sock->my_sock);
	}

private:
	nl_sock_ptr_class nl_sock;
	nl_cb_ptr_class nl_cb;
	int nl80211_id;

	std::shared_ptr<spdlog::logger> logcfg;
};

}; // end namespace cfg80211

// TODO re-read Stroustrup to make sure I'm doing this right
std::ostream& operator<<(std::ostream& os, const cfg80211::BSS& bss);

#endif
