#ifndef NETLINK_HPP
#define NETLINK_HPP

#include <iostream>
#include <vector>
#include <variant>
#include <memory>

#include <linux/if_ether.h>
#include <linux/nl80211.h>

// base class of 802.11 Information Element
class IE
{
	public:
		IE(uint8_t id, uint8_t len, uint8_t *buf); 
		~IE();

//		std::string repr(void);

		// definitely don't want copy constructors
		IE(const IE&) = delete; // Copy constructor
		IE& operator=(const IE&) = delete;  // Copy assignment operator

		// added move constructors to learn how to use move constructors
		// Move constructor
		IE(IE&& src) 
			: id(src.id), len(src.len), buf(src.buf)
		{
			src.id = -1;
			src.len = -1;
			src.buf = nullptr;
		};

		IE& operator=(IE&& src) // Move assignment operator
		{
			id = src.id;
			len = src.len;
			buf = src.buf;

			src.id = -1;
			src.len = -1;
			src.buf = nullptr;

			return *this;
		};


		friend std::ostream & operator << (std::ostream &, const IE&);

	private:
		uint8_t id;
		uint8_t len;
		uint8_t *buf;

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
};


// representation of a WiFi BSS
class BSS
{
public:
	BSS() = default;
	BSS(uint8_t *bssid);
	~BSS();

	// definitely don't want copy constructors
//	BSS(const BSS&) = delete; // Copy constructor
//	BSS& operator=(const BSS&) = delete;  // Copy assignment operator
	BSS(const BSS&) ; // Copy constructor
	BSS& operator=(const BSS&) ;  // Copy assignment operator

	// added move constructors to learn how to use move constructors
	BSS(BSS&&);  // Move constructor
	BSS& operator=(BSS&&); // Move assignment operator

	uint8_t bssid[ETH_ALEN];
	enum nl80211_chan_width channel_width;
	uint32_t freq;
	uint32_t center_freq1;
	uint32_t center_freq2;
	int age;

	std::vector<IE> ie_list;
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


// TODO re-read Stroustrup to make sure I'm doing this right
std::ostream& operator<<(std::ostream& os, const BSS& bss);

std::ostream& operator<<(std::ostream& os, const IE& ie);

#endif
