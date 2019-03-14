#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <iostream>
#include <vector>

#include <linux/if_ether.h>
#include <linux/nl80211.h>

// representation of a WiFi BSS
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

// base class of 802.11 Information Element
class IE
{
	public:
		IE(uint8_t id, uint8_t len) { this->id = id; this->len = len; }

	private:
		uint8_t id;
		uint8_t len;
};

// IE with a string (e.g., SSID)
class String_IE : public IE
{
	public:
		String_IE(uint8_t id, uint8_t len, uint8_t *buf);
		String_IE(uint8_t id, uint8_t len, char *s);

	friend std::ostream & operator << (std::ostream &, const String_IE&);

	private:
		/* quick notes while I'm thinking of it: SSID could be utf8 
		 * https://www.gnu.org/software/libc/manual/html_node/Extended-Char-Intro.html
		 *
		 * https://stackoverflow.com/questions/55641/unicode-processing-in-c
		 * http://site.icu-project.org/ 
		 * dnf info libicu
		 * dnf info libicu-devel
		 *
		 * https://withblue.ink/2019/03/11/why-you-need-to-normalize-unicode-strings.html
		 */

		// for now, just store the chars
		std::string s;
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
std::ostream& operator<<(std::ostream& os, const BSS& bss);

std::ostream& operator<<(std::ostream& os, const String_IE& ie);

#endif
