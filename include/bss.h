#ifndef BSS_H
#define BSS_H

#include <memory>
#include <vector>
#include <array>
#include <linux/nl80211.h>

#include "fmt/format.h"
#include "json/json.h"

#include "logging.h"
#include "ie.h"

namespace cfg80211 {

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

	Json::Value make_json(void);

	// FIXME this method is awful; need a better way
	// also std::string isn't UTF8 safe
	std::string get_ssid(void);

	std::string get_bssid(void) const { return bssid_str; };

	void add_ie(std::shared_ptr<IE> ie) { ie_list.push_back(ie); }
	size_t ie_count(void) const { return ie_list.size(); }

	using IE_List = std::vector<std::shared_ptr<IE>>;
	using const_iterator = typename IE_List::const_iterator;

	const_iterator cbegin() const { return std::cbegin(ie_list); }
	const_iterator cend() const { return std::cend(ie_list); }

	// TODO take some of this stuff private
	std::array<uint8_t,ETH_ALEN> bssid;
	enum nl80211_chan_width channel_width;
	uint32_t freq;
	uint32_t center_freq1;
	uint32_t center_freq2;
	int age;

	// called for NL80211_BSS_SIGNAL_MBM
	void set_bss_signal_mbm(uint32_t value);

protected:
	IE_List ie_list;

	std::string bssid_str;
	std::shared_ptr<spdlog::logger> logger;

	// "@NL80211_BSS_SIGNAL_MBM: signal strength of probe response/beacon
	//	in mBm (100 * dBm) (s32)"  (via nl80211.h)
	//	Note I'm not storing float but as the original integer encoding of mBm.
	float signal_strength_dbm;

	// "@NL80211_BSS_SIGNAL_UNSPEC: signal strength of the probe response/beacon
	//	in unspecified units, scaled to 0..100 (u8)"  (via nl80211.h)
	unsigned int signal_unspec;

};

} // end namespace cfg80211

#endif

