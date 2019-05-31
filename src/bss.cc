#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>
#include <linux/if_ether.h>

#include "fmt/format.h"
#include "fmt/ostream.h"

#include "logging.h"
#include "bss.h"
#include "cfg80211.h"
#include "iw.h"
#include "ie.h"

using namespace cfg80211;

BSS::BSS(uint8_t *bssid)
{
	logger = spdlog::get("bss");
	if (!logger) {
		logger = spdlog::stdout_logger_mt("bss");
	}

	memcpy(this->bssid.data(), bssid, ETH_ALEN);

	bssid_str = fmt::format("{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
					bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
	logger->info("create bss={}", bssid_str);
}

Json::Value BSS::make_json(void)
{
	Json::Value bss_json { Json::objectValue};

	bss_json["bssid"] = get_bssid();

	Json::Value ie_list_js { Json::arrayValue};

	for (auto&& ie = this->cbegin() ; ie != this->cend() ; ++ie) {
		// iterator across a shared ptr so one * for iterator and another * for deference
		logger->info("ie={}", **ie);
		Json::Value v { (*ie)->make_json() };

		ie_list_js.append(v);

		// brute force find the SSID and make a top level copy because it's
		// most often required
		if ((*ie)->get_id() == 0) {
			bss_json["SSID"] = v["SSID"];
		}
		if ((*ie)->get_id() == 3) {
			bss_json["channel"] = v["value"];
		}
	}

	bss_json["signal_strength"] = signal_strength_dbm;
	bss_json["last_seen_ms"] = last_seen_ms;
	bss_json["frequency"] = freq;
	bss_json["ie_list"] = ie_list_js;

	return bss_json;
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
//			return (*ie).str();
		}
	}

	return std::string("(SSID not found)");
}

void BSS::set_bss_signal_mbm(uint32_t value)
{
	signal_strength_dbm = static_cast<int>(value)/100.0;
}



