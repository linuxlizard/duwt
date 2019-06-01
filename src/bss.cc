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

	Json::Value ie_dict_js {Json::objectValue};
	// 221 and 255 can have multiple values
	Json::Value vendor_specific_js {Json::arrayValue}; 
	Json::Value extension_js {Json::arrayValue};

	for (auto&& ie = this->cbegin() ; ie != this->cend() ; ++ie) {
		// iterator across a shared ptr so one * for iterator and another * for deference
		logger->info("ie={}", **ie);
		Json::Value v { (*ie)->make_json() };

		IE_ID id = (*ie)->get_id();

		// vendor specific and extension IEs can appear multiple times
		if (id == IE_ID::VENDOR) {
			vendor_specific_js.append(v);
		}
		else if (id == IE_ID::EXTENSION ) {
			extension_js.append(v);
		}
		else {
			ie_dict_js[(*ie)->get_id_str()] = v;
		}

		// brute force find a few useful fields and make a top level copy
		// because it's most often required
		if (id == IE_ID::SSID) {
			bss_json["SSID"] = v["SSID"];
		}
		else if (id == IE_ID::DSSS_PARAMETER_SET) {
			bss_json["channel"] = v["value"];
		}
	}

	if (!vendor_specific_js.empty()) {
		ie_dict_js["221"] = vendor_specific_js;
	}
	if (!extension_js.empty()) {
		ie_dict_js["255"] = extension_js;
	}

	// TODO add suport for BSS_SIGNAL_UNSPEC (unspecific)
	bss_json["signal_strength"] = Json::Value(Json::objectValue);
	bss_json["signal_strength"]["value"] = signal_strength_dbm;
	bss_json["signal_strength"]["units"] = "dBm";

	bss_json["last_seen_ms"] = last_seen_ms;
	bss_json["frequency"] = freq;
	bss_json["ie"] = ie_dict_js;

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
		if ((*ie).get_id() == IE_ID::SSID) {
//			return (*ie).str();
		}
	}

	return std::string("(SSID not found)");
}

void BSS::set_bss_signal_mbm(uint32_t value)
{
	signal_strength_dbm = static_cast<int>(value)/100.0;
}



