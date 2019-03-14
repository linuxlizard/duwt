#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <iostream>

#include <linux/if_ether.h>
#include <linux/nl80211.h>

class Netlink
{
public:
	// https://en.cppreference.com/w/cpp/language/rule_of_three
	// https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#S-ctor
	Netlink();
	virtual ~Netlink();  // Destructor

	// definitely don't want copy constructors
	Netlink(const Netlink&) = delete; // Copy constructor
	Netlink& operator=(const Netlink&) = delete;  // Copy assignment operator

	// added move constructors to learn how to use move constructors
	Netlink(Netlink&&);  // Move constructor
	Netlink& operator=(Netlink&&); // Move assignment operator

	int get_scan(const char *iface);

private:
	struct nl_sock* nl_sock;
	int nl80211_id;
	struct nl_cb* s_cb;

};

class Network
{
public:
	uint8_t bssid[ETH_ALEN];
	enum nl80211_chan_width channel_width;
	uint32_t freq;
	uint32_t center_freq1;
	uint32_t center_freq2;
	int age;
};


// TODO re-read Stroustup to make sure I'm doing this right
std::ostream& operator<<(std::ostream& os, Network& network);

#endif
