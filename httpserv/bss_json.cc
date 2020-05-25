#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <iomanip>

#include <linux/netlink.h>
#include <netlink/attr.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>

#include "Poco/Logger.h"
#include "Poco/ConsoleChannel.h"
#include "Poco/StreamChannel.h"
#include "Poco/JSON/Object.h"
#include "Poco/JSON/JSONException.h"
#include "Poco/UTF8String.h"

#include "bss.h"
#include "nlnames.h"

using Poco::Logger;
using Poco::Message;

Logger& logger = Logger::root();

// newer versions of Poco have this
#ifndef poco_information_f
#define poco_information_f(logger, fmt, ...) \
	if ((logger).information()) (logger).information(Poco::format((fmt), __VA_ARGS__), __FILE__, __LINE__); else (void) 0
#endif

static const uint32_t SCAN_DUMP_COOKIE = 0x64617665;

class BSSPointer
{
public:
	BSSPointer(struct BSS* p) { bss=p; };
	~BSSPointer() { bss_free(&bss); bss=nullptr; };

	const struct BSS* get(void) { return bss; };

private:
	struct BSS* bss;
};

void parse(std::istream& infile, std::vector<std::unique_ptr<BSSPointer>>& bss_list)
{
	logger.debug("parse ");

	if (infile.fail()) {
		throw std::runtime_error("file open failed");
	}

	char c;
	do {
		c = infile.get();
		if (infile.fail()) {
			throw std::runtime_error("file fail bit set");
		}
	} while(c != 0x0a);

	while ( true ) { 
		uint32_t size, cookie;
		infile.read((char *)&cookie, sizeof(uint32_t));

		if (infile.eof() ) {
			break;
		}

		infile.read((char *)&size, sizeof(uint32_t));
		if (infile.fail()) {
			throw std::runtime_error("premature end of file");
		}
		if (cookie != SCAN_DUMP_COOKIE) {
			throw std::runtime_error("file format error; missing magic");
		}
		if (size > 4095) {
			throw std::runtime_error("file format error; invalid size");
		}

		logger.debug("size=%u cookie=0x%x", size, cookie);
		uint8_t buf[4096];
		infile.read((char *)buf, size);
		if (infile.fail()) {
			throw std::runtime_error("not enough data for size");
		}

		int attrlen = (int) size;
		struct nlattr* attr = (struct nlattr*)buf;
		struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
		int err = nla_parse(tb_msg, NL80211_ATTR_MAX, attr, attrlen, NULL);

	//	peek_nla_attr(tb_msg, NL80211_ATTR_MAX);

		struct BSS* bss = NULL;
		err = bss_from_nlattr(tb_msg, &bss);

		printf("bss=%p\n", bss);

		bss_list.push_back(std::make_unique<BSSPointer>(bss));
	}
}

Poco::JSON::Object ie_to_json(const struct IE* ie)
{
	// may the gods of RVO smile upon me
	Poco::JSON::Object obj;

	obj.set("id", static_cast<int>(ie->id));
	obj.set("len", ie->len);

	std::ostringstream ostr;
//	ostr.str(bytes);
	ostr.setf(std::ios_base::hex);
	for (int i=0 ; i<ie->len ; i++) {
		ostr << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ie->buf[i]);
	}
	logger.debug("ostr=%s", ostr.str());
	obj.set("bytes", ostr.str());

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

Poco::JSON::Object bss_to_json(const struct BSS* bss)
{
	// may the gods of RVO smile upon me
	Poco::JSON::Object obj;

	obj.set("bssid", bss->bssid_str);

	// dangerous untrusted input!
	const struct IE_SSID* ie_ssid = bss_get_ssid(bss);
	if (ie_ssid) {
		std::string ssid_unsafe { ie_ssid->ssid, ie_ssid->ssid_len };
		std::string ssid_esc = Poco::UTF8::escape(ssid_unsafe, true);
		obj.set("ssid", ssid_esc);
		logger.information("found ssid=%s", ssid_esc);
	}
	else {
		obj.set("ssid", "<hidden>");
	}

	obj.set("frequency", bss->frequency);
	obj.set("dBm", bss->signal_strength_mbm / 100.0);
	char str[32] {};
	bss_get_chan_width_str(bss, str, 32);
	obj.set("channel_width", std::string(str));
	obj.set("TSF", bss->tsf);

	Poco::JSON::Array ie_array;
	const struct IE* ie = nullptr;
	ie_list_for_each_entry(ie, bss->ie_list) {
//		ie_array.add(static_cast<int>(ie_ptr->id));
		ie_array.add(ie_to_json(ie));
	}

	obj.set("IE", ie_array);

	return obj;
}

int main(void)
{
	std::cout << "hello, world\n";

	logger.setChannel(new Poco::StreamChannel(std::cout));
	logger.setLevel(Message::PRIO_DEBUG);
	std::cout << "level=" << logger.getLevel() << "\n";
	logger.debug("This is a Debug message");
	logger.information("This is an Information message");
	logger.warning("This is a Warning message");
	logger.error("This is an Error message");

	poco_warning(logger, "Another warning");
	poco_information(logger, Poco::format("information with args: %d %d", 42, 54));
	poco_information_f(logger, "information with args: %d %d", 42, 54);

	Logger::get("ConsoleLogger").error("Another error message");

//	Logger& logger = Logger::create("iw", 

	std::ifstream infile;
	infile.open("scan-dump.dat", std::ios_base::binary|std::ios_base::in);

	std::vector<std::unique_ptr<BSSPointer>> bss_list;

	parse(infile, bss_list);
	infile.close();

	for (auto& bss_ptr: bss_list) {
		const struct BSS* bss = bss_ptr.get()->get();

		Poco::JSON::Object obj = bss_to_json(bss);
		std::ostringstream str;
		obj.stringify(str);
		std::cout << str.str() << "\n";
//		obj.stringify(std::cout, 5);
	}

	logger.information("found %z bss", bss_list.size());

	return 0;
}

