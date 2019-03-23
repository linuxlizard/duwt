#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <iostream>
#include <vector>
#include <variant>
#include <memory>

#include <linux/if_ether.h>
#include <linux/nl80211.h>
#include <netlink/socket.h>

#include "ie.hh"

namespace cfg80211 {

// encapsulate a netlink socket struct so can wrap it in a unique_ptr
class NLSock
{
	public:
		NLSock() 
		{ 
			my_sock = nl_socket_alloc(); 
			std::cout << "class sock=" << my_sock << "\n";
			// TODO throw something on alloc fail
		};

		~NLSock() 
		{
			std::cout << "~class sock\n";
			if (my_sock) {
				std::cout << "class sock nl_socket_free " << my_sock << "\n";
				nl_socket_free(my_sock);
				my_sock = nullptr;
			}
		};

		struct nl_sock* my_sock;
};


class NLCallback
{
	public:
		NLCallback() 
		{ 
			my_cb = nl_cb_alloc(NL_CB_DEBUG);
			// TODO throw something on alloc failure
		};

		~NLCallback() 
		{
			if (my_cb) {
				nl_cb_put(my_cb);
				my_cb = nullptr;
			}
		};

		struct nl_cb *my_cb;
};

using nl_sock_ptr_class = std::unique_ptr<NLSock>;
using nl_cb_ptr_class = std::unique_ptr<NLCallback>;

// representation of a WiFi BSS
class BSS
{
public:
	BSS() = default;
	BSS(uint8_t *bssid);
	~BSS() = default;

	// definitely don't want copy constructors
	BSS(const BSS&) = delete; // Copy constructor
	BSS& operator=(const BSS&) = delete;  // Copy assignment operator

	// added move constructors to learn how to use move constructors
	BSS(BSS&&);  // Move constructor
	BSS& operator=(BSS&&); // Move assignment operator

	std::array<uint8_t,ETH_ALEN>bssid;
	enum nl80211_chan_width channel_width;
	uint32_t freq;
	uint32_t center_freq1;
	uint32_t center_freq2;
	int age;

	std::vector<IE> ie_list;

	// FIXME this method is awful; need a better way
	// also std::string isn't UTF8 safe
	std::string get_ssid(void);
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

	int get_scan(const char*iface, std::vector<BSS>& network_list);

	int listen_scan_events(void);
	int fetch_scan_events(void);

	int get_fd(void)
	{
		return nl_socket_get_fd(nl_sock->my_sock);
	}

private:
	nl_sock_ptr_class nl_sock;
	nl_cb_ptr_class nl_cb;
	int nl80211_id;
};

}; // end namespace cfg80211

// TODO re-read Stroustrup to make sure I'm doing this right
std::ostream& operator<<(std::ostream& os, const cfg80211::BSS& bss);

#endif
