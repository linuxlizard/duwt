#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <iostream>
#include <vector>

#include <linux/if_ether.h>
#include <linux/nl80211.h>

class BSS
{
public:
	uint8_t bssid[ETH_ALEN];
	enum nl80211_chan_width channel_width;
	uint32_t freq;
	uint32_t center_freq1;
	uint32_t center_freq2;
	int age;
};

// https://wireless.wiki.kernel.org/en/developers/Documentation/cfg80211
class Cfg80211
{
public:
	// https://en.cppreference.com/w/cpp/language/rule_of_three
	// https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#S-ctor
	Cfg80211();
	virtual ~Cfg80211();  // Destructor

	// definitely don't want copy constructors
	Cfg80211(const Cfg80211&) = delete; // Copy constructor
	Cfg80211& operator=(const Cfg80211&) = delete;  // Copy assignment operator

	// added move constructors to learn how to use move constructors
	Cfg80211(Cfg80211&&);  // Move constructor
	Cfg80211& operator=(Cfg80211&&); // Move assignment operator

	int get_scan(const char*iface, std::vector<BSS>& network_list);

private:
	struct nl_sock* nl_sock;
	int nl80211_id;
	struct nl_cb* s_cb;

};


// TODO re-read Stroustup to make sure I'm doing this right
std::ostream& operator<<(std::ostream& os, BSS& network);

#endif
