#include <sstream>
#include <iomanip>

#include "json/json.h"

#include "bss.h"
#include "nlnames.h"
#include "bss_json.h"

Json::Value ie_to_json(const struct IE* ie)
{
	// may the gods of RVO smile upon me
	Json::Value obj;

	obj["id"] =  static_cast<Json::Value::Int>(ie->id);
	obj["len"] = static_cast<Json::Value::Int>(ie->len);

	std::ostringstream ostr;
//	ostr.str(bytes);
	ostr.setf(std::ios_base::hex);
	for (size_t i=0 ; i<ie->len ; i++) {
		ostr << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ie->buf[i]);
	}
	obj["bytes"] = ostr.str();

	// TODO
#if 0
	switch (ie->id) {
		case IE_SUPPORTED_RATES:
			break;
		case IE_DSSS_PARAMETER_SET:
			break;
		case IE_TIM:
			break;
		case IE_COUNTRY:
			break;
		case IE_HT_CAPABILITIES:
			break;
		case IE_RSN:
			break;
		case IE_HT_OPERATION:
			break;
		case IE_VHT_CAPABILITIES:
			break;
		case IE_VHT_OPERATION:
			break;
		default:
			break;
	}
#endif

	return obj;
}

Json::Value bss_to_json(const struct BSS* bss)
{
	// may the gods of RVO smile upon me
	Json::Value obj(Json::objectValue);

	obj["bssid"] =  bss->bssid_str;

	// dangerous untrusted input!
	const struct IE_SSID* ie_ssid = bss_get_ssid(bss);
	if (ie_ssid) {
		std::string ssid_unsafe { ie_ssid->ssid, ie_ssid->ssid_len };
//		std::string ssid_esc = Poco::UTF8::escape(ssid_unsafe, true);
		// TODO validate this unsafe string into a safe string first!!!!!!!!!
		obj["ssid"]  = ssid_unsafe;
	}
	else {
		obj["ssid"] = "<hidden>";
	}

	obj["freq"] =  bss->frequency;
	obj["dbm"] = bss->signal_strength_mbm / 100.0;
	char str[32] {};
	bss_get_chan_width_str(bss, str, 32);
	obj["chwidth"] =  std::string(str);
	bss_get_mode_str(bss, str, 32);
	obj["mode"] = std::string(str);
	obj["TSF"] = static_cast<Json::Value::UInt64>(bss->tsf);

	Json::Value  ie_array(Json::arrayValue);
	const struct IE* ie = nullptr;
	ie_list_for_each_entry(ie, bss->ie_list) {
		ie_array.append(ie_to_json(ie));
	}

	obj["IE"] = ie_array;

	return obj;
}

