#include <iostream>
#include <memory>
#include <array>
#include <vector>
#include <cassert>
#include <sstream>

//#include <sys/socket.h>
//#include <linux/if_ether.h>
//#include <net/if.h>
//#include <netlink/genl/genl.h>
//#include <netlink/genl/family.h>
//#include <netlink/genl/ctrl.h>
//#include <netlink/msg.h>
//#include <netlink/attr.h>
//#include <linux/nl80211.h>

#include "mynetlink.h"
#include "fmt/format.h"
#include "json/json.h"
#include "logging.h"
#include "ie.h"
#include "oui.h"

// From IEEE802.11-2016:
//
// This revision is based on IEEE Std 802.11-2012, into which the following amendments have been
// incorporated:
// — IEEE Std 802.11aeTM-2012: Prioritization of Management Frames (Amendment 1)
// — IEEE Std 802.11aaTM-2012: MAC Enhancements for Robust Audio Video Streaming (Amendment 2)
// — IEEE Std 802.11adTM-2012: Enhancements for Very High Throughput in the 60 GHz Band (Amendment 3)
// — IEEE Std 802.11acTM-2013: Enhancements for Very High Throughput for Operation in Bands below 6 GHz (Amendment 4)
// — IEEE Std 802.11afTM-2013: Television White Spaces (TVWS) Operation (Amendment 5)

// From IEEE802.11-2012:
//
// <quote>
// The original standard was published in 1999 and reaffirmed in 2003. A revision was published in 2007,
// which incorporated into the 1999 edition the following amendments: IEEE Std 802.11aTM-1999,
// IEEE Std 802.11bTM-1999, IEEE Std 802.11b-1999/Corrigendum 1-2001, IEEE Std 802.11dTM-2001, IEEE
// Std 802.11gTM-2003, IEEE Std 802.11hTM-2003, IEEE Std 802.11iTM-2004, IEEE Std 802.11jTM-2004 and
// IEEE Std 802.11eTM-2005.
//
// The current revision, IEEE Std 802.11-2012, incorporates the following amendments into the 2007 revision:
// — IEEE Std 802.11kTM-2008: Radio Resource Measurement of Wireless LANs (Amendment 1)
// — IEEE Std 802.11rTM-2008: Fast Basic Service Set (BSS) Transition (Amendment 2)
// — IEEE Std 802.11yTM-2008: 3650–3700 MHz Operation in USA (Amendment 3)
// — IEEE Std 802.11wTM-2009: Protected Management Frames (Amendment 4)
// — IEEE Std 802.11nTM-2009: Enhancements for Higher Throughput (Amendment 5)
// — IEEE Std 802.11pTM-2010: Wireless Access in Vehicular Environments (Amendment 6)
// — IEEE Std 802.11zTM-2010: Extensions to Direct-Link Setup (DLS) (Amendment 7)
// — IEEE Std 802.11vTM-2011: IEEE 802.11 Wireless Network Management (Amendment 8)
// — IEEE Std 802.11uTM-2011: Interworking with External Networks (Amendment 9)
// — IEEE Std 802.11sTM-2011: Mesh Networking (Amendment 10)
//
// As a result of publishing this revision, all of the previously published amendments and revisions are now
// retired.
// </quote>

using namespace cfg80211;

using Blob = std::vector<uint8_t>;

namespace {
	unsigned char ms_oui[3]		= { 0x00, 0x50, 0xf2 };
	unsigned char ieee80211_oui[3]	= { 0x00, 0x0f, 0xac };
	unsigned char wfa_oui[3]		= { 0x50, 0x6f, 0x9a };
}

// iw scan.c print_supprates()
#define BSS_MEMBERSHIP_SELECTOR_VHT_PHY 126
#define BSS_MEMBERSHIP_SELECTOR_HT_PHY 127

// iw scan.c print_country()

#define IEEE80211_COUNTRY_EXTENSION_ID 201

union ieee80211_country_ie_triplet {
	struct {
		__u8 first_channel;
		__u8 num_channels;
		__s8 max_power;
	} __attribute__ ((packed)) chans;
	struct {
		__u8 reg_extension_id;
		__u8 reg_class;
		__u8 coverage_class;
	} __attribute__ ((packed)) ext;
} __attribute__ ((packed));

// iw scan.c print_tim()
static std::string decode_tim(Blob bytes)
{
	std::string s = fmt::format(" DTIM Count {:d} DTIM Period {:d} Bitmap Control {:#x} Bitmap[0] {:#x}",
	       bytes.data()[0], bytes.data()[1], bytes.data()[2], bytes.data()[3]);

	if (bytes.size() - 4)
		s += fmt::format(" (+ {:d} octet{:s})", bytes.size() - 4, bytes.size() - 4 == 1 ? "" : "s");
	return s;
}

static std::string unknown_suite(const uint8_t* data)
{
	return fmt::format("{0:02x}-{1:02x}-{2:02x}:{3:d}",
		data[0], data[1] ,data[2], data[3]);
}

// iw scan.c print_cipher()
static std::string decode_cipher(const uint8_t *data)
{
	if (memcmp(data, ms_oui, 3) == 0) {
		switch (data[3]) {
		case 0:
			return "Use group cipher suite";
			break;
		case 1:
			return "WEP-40";
			break;
		case 2:
			return "TKIP";
			break;
		case 4:
			return "CCMP";
			break;
		case 5:
			return "WEP-104";
			break;
		default:
			return unknown_suite(data);
			break;
		}
	} else if (memcmp(data, ieee80211_oui, 3) == 0) {
		switch (data[3]) {
		case 0:
			return "Use group cipher suite";
			break;
		case 1:
			return "WEP-40";
			break;
		case 2:
			return "TKIP";
			break;
		case 4:
			return "CCMP";
			break;
		case 5:
			return "WEP-104";
			break;
		case 6:
			return "AES-128-CMAC";
			break;
		case 7:
			return "NO-GROUP";
			break;
		case 8:
			return "GCMP";
			break;
		default:
			return unknown_suite(data);
			break;
		}
	} else {
		return unknown_suite(data);
	}
}

// iw scan.c print_auth() 
static std::string decode_auth(const uint8_t *data)
{
	if (memcmp(data, ms_oui, 3) == 0) {
		switch (data[3]) {
		case 1:
			return "IEEE 802.1X";
			break;
		case 2:
			return "PSK";
			break;
		default:
			return unknown_suite(data);
			break;
		}
	} else if (memcmp(data, ieee80211_oui, 3) == 0) {
		switch (data[3]) {
		case 1:
			return "IEEE 802.1X";
			break;
		case 2:
			return "PSK";
			break;
		case 3:
			return "FT/IEEE 802.1X";
			break;
		case 4:
			return "FT/PSK";
			break;
		case 5:
			return "IEEE 802.1X/SHA-256";
			break;
		case 6:
			return "PSK/SHA-256";
			break;
		case 7:
			return "TDLS/TPK";
			break;
		case 8:
			return "SAE";
			break;
		case 9:
			return "FT/SAE";
			break;
		case 11:
			return "IEEE 802.1X/SUITE-B";
			break;
		case 12:
			return "IEEE 802.1X/SUITE-B-192";
			break;
		case 13:
			return "FT/IEEE 802.1X/SHA-384";
			break;
		case 14:
			return "FILS/SHA-256";
			break;
		case 15:
			return "FILS/SHA-384";
			break;
		case 16:
			return "FT/FILS/SHA-256";
			break;
		case 17:
			return "FT/FILS/SHA-384";
			break;
		case 18:
			return "OWE";
			break;
		default:
			return unknown_suite(data);
			break;
		}
	} else if (memcmp(data, wfa_oui, 3) == 0) {
		switch (data[3]) {
		case 1:
			return "OSEN";
			break;
		case 2:
			return "DPP";
			break;
		default:
			return unknown_suite(data);
			break;
		}
	} else {
		return unknown_suite(data);
	}
}

// iw scan.c _print_rsn_ie()
// is_osen seems to be a HotSpot thing
static void _decode_rsn_ie(const Blob& bytes, const char *defcipher, const char *defauth, int is_osen, std::vector<std::string>& decode)
{
	__u16 count, capa;
	int i;
	std::string s;
	const uint8_t* data = bytes.data();
	ssize_t len = bytes.size();

	if (!is_osen) {
		__u16 version;
		version = data[0] + (data[1] << 8);
		decode.push_back(fmt::format("Version: {:d}", version));

		data += 2;
		len -= 2;
	}

	if (len < 4) {
		decode.push_back(fmt::format("Group cipher: {:s}", defcipher));
		decode.push_back(fmt::format("Pairwise ciphers: {:s}", defcipher));
		return;
	}

	s = "Group cipher: " + decode_cipher(data);
	decode.push_back(s);

	data += 4;
	len -= 4;

	if (len < 2) {
		decode.push_back(fmt::format("Pairwise ciphers: {:s}", defcipher));
		return;
	}

	count = data[0] | (data[1] << 8);
	if (2 + (count * 4) > len)
		goto invalid;

	s = "Pairwise ciphers:";
	for (i = 0; i < count; i++) {
		s += " " + decode_cipher(data + 2 + (i * 4));
	}
	decode.push_back(s);

	data += 2 + (count * 4);
	len -= 2 + (count * 4);

	if (len < 2) {
		decode.push_back(fmt::format("Authentication suites: {:s}", defauth));
		return;
	}

	count = data[0] | (data[1] << 8);
	if (2 + (count * 4) > len)
		goto invalid;

	s = "Authentication suites:";
	for (i = 0; i < count; i++) {
		s += " " + decode_auth(data + 2 + (i * 4));
	}
	decode.push_back(s);

	data += 2 + (count * 4);
	len -= 2 + (count * 4);

	if (len >= 2) {
		capa = data[0] | (data[1] << 8);
		s = "Capabilities:";
		if (capa & 0x0001)
			s += " PreAuth";
		if (capa & 0x0002)
			s += " NoPairwise";
		switch ((capa & 0x000c) >> 2) {
		case 0:
			s += " 1-PTKSA-RC";
			break;
		case 1:
			s += " 2-PTKSA-RC";
			break;
		case 2:
			s += " 4-PTKSA-RC";
			break;
		case 3:
			s += " 16-PTKSA-RC";
			break;
		}
		switch ((capa & 0x0030) >> 4) {
		case 0:
			s += " 1-GTKSA-RC";
			break;
		case 1:
			s += " 2-GTKSA-RC";
			break;
		case 2:
			s += " 4-GTKSA-RC";
			break;
		case 3:
			s += " 16-GTKSA-RC";
			break;
		}
		if (capa & 0x0040)
			s += " MFP-required";
		if (capa & 0x0080)
			s += " MFP-capable";
		if (capa & 0x0200)
			s += " Peerkey-enabled";
		if (capa & 0x0400)
			s += " SPP-AMSDU-capable";
		if (capa & 0x0800)
			s += " SPP-AMSDU-required";
		s += fmt::format(" ({:#4x})", capa);
		data += 2;
		len -= 2;
		decode.push_back(s);
	}

	if (len >= 2) {
		int pmkid_count = data[0] | (data[1] << 8);

		if (len >= 2 + 16 * pmkid_count) {
			decode.push_back(fmt::format("{:d} PMKIDs", pmkid_count));
			/* not printing PMKID values */
			data += 2 + 16 * pmkid_count;
			len -= 2 + 16 * pmkid_count;
		} else
			goto invalid;
	}

	if (len >= 4) {
		s = "Group mgmt cipher suite: ";
		s += decode_cipher(data);
		decode.push_back(s);
		data += 4;
		len -= 4;
	}

 invalid:
	if (len != 0) {
		s = fmt::format("bogus tail data ({:d}):", len);
		while (len) {
			printf(" %.2x", *data);
			data++;
			len--;
		}
	}
}

// iw scan.c print_rsn()
static void decode_rsn(const Blob& bytes, std::vector<std::string>& decode)
{
	const char *defcipher = "CCMP";
	const char *defauth = "IEEE 802.1X";

	_decode_rsn_ie(bytes, defcipher, defauth, 0, decode);
}

// helper function to decode an Information Element blob
// going to pretty much copy iw's scan.c decode fns
#if 0
void decode_ie(int id, size_t len, Blob bytes, std::vector<std::string>& decode)
{
	(void)len;

	// TODO use a lut of some sort
	// https://stackoverflow.com/questions/55588440/return-a-function-from-a-class

	switch (id) {
		case 0:
			// FIXME UTF-8 (somehow)
			decode.emplace_back(std::string(reinterpret_cast<char*>(bytes.data()), bytes.size()));
			break;

		case 1:
			// Supported Rates
			decode.push_back(decode_supprates(bytes));
			break;

		case 3:
			decode.emplace_back(std::move(decode_ds(bytes)));
//			decode.push_back(decode_ds(bytes));
			break;

		case 5:
			decode.push_back(decode_tim(bytes));
			break;

		case 7:
			decode_country(bytes, decode);
			break;

		case 48:
			decode_rsn(bytes, decode);
			break;

		default:
			// TODO
			decode.emplace_back("(no decode)");
			break;
	}
}
#endif

class IE_Names
{
	public:
		IE_Names();

		// "IEEE Std 802.11-2012"
		// Table 8-54—Element IDs
		std::array<const char*, 256> names; 
};

IE_Names::IE_Names()
{
	names = 
		{
			"SSID",
			"Supported rates",
			"FH parameter set",
			"DSSS parameter set",
			"CF parameter set",
			"TIM",
			"IBSS parameter set",
			"Country",
			"Hopping pattern parameters",
			"Hopping pattern table",

			"Request", // 10
			"BSS load",
			"EDCA paramter set",
			"TSPEC",
			"TCLAS",
			"Schedule",
			"Challenge text",
			"Reserved", // 17
			"Reserved",
			"Reserved",

			"Reserved", // 20
			"Reserved",
			"Reserved",
			"Reserved",
			"Reserved",
			"Reserved",
			"Reserved",
			"Reserved",
			"Reserved",
			"Reserved",

			"Reserved", // 30
			"Reserved",
			"Power constraint",
			"Power capability",
			"TPC request",
			"TPC report",
			"Supported channels",
			"Channel switch announcement",
			"Measurement request", 
			"Measurement report",

			"Quiet", // 40
			"IBSS DFS", 
			"ERP",
			"TS Delay",
			"TCLAS processing",
			"HT capabilities",
			"QoS capability",
			"Reserved", // 47
			"RSN",
			"Reserved",

			"Extended supported rates",	// 50
			"AP channel report",
			"Neighbor report",
			"RCPI",
			"Mobility domain (MDE)", 
			"FAST BSS transition (FTE)",
			"Timeout interval",
			"RIC data (RDE)",
			"DSE registered location",
			"Supported operating classes",

			"Extended channel switch announcement",	// 60
			"HT operation",
			"Secondary channel offset",
			"BSS average access delay",
			"Antenna",
			"RSNI",
			"Measurement pilot transmission",
			"BSS available administration capacity",
			"BSS AC access delay",
			"Time advertisement",

			"RM enabled capabilities", // 70
			"Multiple BSSID",
			"20/40 BSS coexistence",
			"20/40 BSS intolerance channel report",
			"Overlapping BSS scan parameters",
			"RIC descriptor",
			"Management MIC",
			"Reserved",
			"Event request",
			"Event report",

			"Diagnostic request", // 80
			"Diagnostic report",
			"Location parameters",
			"Nontransmitted BSSID capability",
			"SSID list",
			"Multiple BSSID index",
			"FMS descriptor",
			"FMS request",
			"FMS response",
			"QoS traffic capability",

			"BSS max idle period", // 90
			"TFS request",
			"TFS response",
			"WNM sleep mode",
			"TIM broadcast request",
			"TIM broadcast response",
			"Collacated interference report",
			"Channel usage",
			"Time zone",
			"DMS request",

			"DMS response", // 100
			"Link identifier", 
			"Wakeup schedule",
			"Reserved",
			"Channel switch timing",
			"PTI control",
			"TPU buffer status",
			"Internetworking",
			"Advertisement protocol",
			"Expedited bandwidth request",

			"QoS map set", // 110
			"Roaming consortium",
			"Emergency alert identifier",
			"Mesh configuration",
			"Mesh ID",
			"Mesh link metric report",
			"Congestion notification",
			"Mesh peering management",
			"Mesh channel switch parameters",
			"Mesh awake window",

			"Beacon timing", // 120
			"MCCAOP setup request", 
			"MCCAOP setup reply", 
			"MCCAOP advertisement", 
			"MCCAOP teardown", 
			"GANN",
			"RANN",
			"Extended capabilities",
			"Reserved",
			"Reserved",

			"PREQ", // 130
			"PREP", 
			"PERR",
			"Reserved", // 133
			"Reserved", // 134
			"Reserved", // 135
			"Reserved", // 136
			"PXU", 
			"PXUC",
			"Authenticated mesh peering exchange",

			"MIC", // 140
			"Destination URI", 
			"U-APSD coexistence", 
			"Reserved", // 143
		};
	// fill in some holes
	names[221] = "Vendor specific";

	// from iw scan.c ieprinters[]
	names[107] = "802.11u Interworking";
	names[108] = "802.11u Advertisement",
	names[111] = "802.11u Roaming Consortium";

	names[191] = "VHT capabilities";
	names[192] = "VHT operation";
	names[193] = "Extended BSS Load";
	names[194] = "Wide bandwidth channel switch";
	names[195] = "Transmit power envelope";
	names[196] = "Channel switch wrapper";
	names[197] = "AID";
	names[198] = "Quiet channel";
	names[199] = "Operating mode notification";
	names[200] = "UPSIM";

	names[255] = "Reserved";

	// TODO fill out VHT fields; anything new in HE (802.11x)?

	// fill out any empty fields with "Reserved"
	for (size_t idx=0 ; idx<256 ; idx++) {
		if (!names.at(idx)) {
			names[idx] = "Reserved";
		}
	}
}

static IE_Names ie_names;

std::string hex_dump_bytes(std::vector<uint8_t> input)
{
	static const char hex_ascii[] ="0123456789abcdef";

	std::stringstream ss;
	std::ostreambuf_iterator<char> out_ss(ss);

	std::for_each(input.cbegin(), input.cend(), 
			[&out_ss](char c) { 
				*out_ss++ = hex_ascii[(c>>4)&0x0f]; 
				*out_ss++ = hex_ascii[c&0x0f]; 
			});

	return ss.str();
}

IE::IE(uint8_t id, uint8_t len, uint8_t *buf) 
	: id{id}, 
	  len{len},
	  bytes{},
	  name {nullptr},
	  logger {nullptr}
{
	logger = spdlog::get("ie");
	if (!logger) {
		logger = spdlog::stdout_logger_mt("ie");
	}

	if (len && buf) {
		bytes.assign(buf, buf+len);
	}

	name = ie_names.names.at(id);
	if (!name) {
		// TODO throw something (don't use assert)
		logger->error("failed to find id={}", id);
		logger->flush();
		assert(name);
	}
	hexdump = hex_dump_bytes(bytes);

	int int_id = static_cast<int>(id);
	size_t int_len = static_cast<size_t>(len);

	logger->debug("construct ie name=\"{}\" id={} len={}", 
			name, int_id, int_len);
}

Json::Value IE::make_json(void)
{
	Json::Value v;

	v["id"] = id;
	v["len"] = len;
	v["name"] = std::string{name};
	v["hex"] = hexdump;
	return v;
}

IE_SSID::IE_SSID(uint8_t id_, uint8_t len_, uint8_t* buf)
	: IE(id_, len_, buf)
{
	// TODO need to very very carefully validate the bytes in the SSID.
	// Could be maliciously encoded UTF8 or invalid UTF8 or nulls or 
	// generally could be garbage.

	if (len_ > 32) {
		// TODO throw something?
		ssid = fmt::format("ERROR: SSID invalid len={}", len_);
		return;
	}

	ssid.assign(reinterpret_cast<char *>(buf), static_cast<size_t>(len_));
}

Json::Value IE_SSID::make_json(void)
{
	Json::Value v { IE::make_json() };
	v["SSID"] = ssid;

	return v;
}

IE_SupportedRates::IE_SupportedRates(uint8_t id_, uint8_t len_, uint8_t* buf)
	: IE(id_, len_, buf)
{
	// decode supported rates 
	// iw scan.c print_supprates()
	size_t i;
	std::string s;

	// "bits 6 to 0 are set to the data rate ... in units of 500kb/s"
	for (i = 0; i < bytes.size(); i++) {
		uint8_t byte = bytes.data()[i];
		int r = byte & 0x7f;

		logger->debug("{} idx={} byte={:#x} r={}", __func__, i, byte, r);

		if (r == BSS_MEMBERSHIP_SELECTOR_VHT_PHY && byte & 0x80)
			s += "VHT";
		else if (r == BSS_MEMBERSHIP_SELECTOR_HT_PHY && byte & 0x80)
			s += "HT";
		else {
			s += fmt::format("{}.{}", r/2, 5*(r&1));
			rates_list.emplace_back( r/2, true && (byte & 0x80) );
//			printf("%d.%d", r/2, 5*(r&1));
		}

		s += (byte & 0x80) ? "* " : " ";
	}
}

Json::Value IE_SupportedRates::make_json(void)
{
//	std::cout << "SupportedRates make_json id=" << static_cast<int>(id) << "\n";
	Json::Value v { IE::make_json() };

	v["rates"] = Json::Value{Json::arrayValue};

	for (auto& rate : rates_list) {
		v["rates"].append(rate.make_json());
	}

	return v;
}

IE_Integer::IE_Integer(uint8_t id_, uint8_t len_, uint8_t* buf)
	: IE(id_, len_, buf)
{
	value = static_cast<int>(bytes[0]);
}

Json::Value IE_Integer::make_json(void)
{
	Json::Value v { IE::make_json() };
	v["value"] = value;
	return v;
}

IE_Country::Triplet::Triplet(unsigned int n1, unsigned int n2, unsigned int n3) 
{
	auto logger = spdlog::get("ie");
	logger->debug("Country Triplet values={},{},{}", n1, n2, n3);

	if (n1 >= IEEE80211_COUNTRY_EXTENSION_ID) {
		chans = {0,0,0,0};
		ext = {n1,n2,n3};
	}
	else {
		chans = {n1,n2,n3,0};
		ext = {0,0,0};

		// iw scan.c print_country()
		if (chans.first_channel <= 14)
			chans.end_channel = chans.first_channel + (chans.num_channels - 1);
		else
			chans.end_channel =  chans.first_channel + (4 * (chans.num_channels - 1));
	}
}

Json::Value IE_Country::Triplet::make_json(void)
{
	Json::Value v { Json::objectValue };

	// iw scan.c print_country() 
	if (ext.reg_extension_id >= IEEE80211_COUNTRY_EXTENSION_ID) {
		v["extension_id"] = ext.reg_extension_id;
		v["regulator_class"] = ext.reg_class;
		v["coverage_class"] = ext.coverage_class;
		v["m"] = ext.coverage_class * 450;
	}
	else {
		v["first_channel"] = chans.first_channel;
		v["num_channels"] = chans.num_channels;
		v["dBm"] = chans.max_power;
		v["end_channel"] = chans.end_channel;
	}

	return v;
}

IE_Country::IE_Country(uint8_t id_, uint8_t len_, uint8_t* buf)
	: IE(id_, len_, buf)
{
	char* data = reinterpret_cast<char*>(bytes.data());
	ssize_t len = bytes.size();

	/* iw scan.c print_country() */
	// http://www.ieee802.org/11/802.11mib.txt
	country.assign(data, 2);

	environment_byte = data[2];
	switch (data[2]) {
		case 'I':
			environment = Environment::INDOOR_ONLY;
			break;
		case 'O':
			environment = Environment::OUTDOOR_ONLY;
			break;
		case ' ':
			environment = Environment::INDOOR_OUTDOOR;
			break;
		default:
			environment = Environment::INVALID;
			break;
	}

	data += 3;
	len -= 3;

	if (len < 3) {
		// no country codes
		return;
	}

	while (len >= 3) {
		union ieee80211_country_ie_triplet *triplet = reinterpret_cast<union ieee80211_country_ie_triplet*>(data);

		if (triplet->ext.reg_extension_id >= IEEE80211_COUNTRY_EXTENSION_ID) {
			triplets.emplace_back(
				triplet->ext.reg_extension_id,
				triplet->ext.reg_class,
				triplet->ext.coverage_class);
			data += 3;
			len -= 3;
			continue;
		}

		// TODO I could probably just use the same values from the 3 bytes in data.. 
		triplets.emplace_back(triplet->chans.first_channel, triplet->chans.num_channels, triplet->chans.max_power);

		data += 3;
		len -= 3;
	}
}

Json::Value IE_Country::make_json(void)
{
	Json::Value v { IE::make_json() };
	v["country"] = country;

	switch (environment) {
		case Environment::INDOOR_ONLY:
			v["environment"] = std::string("Indoor only");
			break;
		case Environment::OUTDOOR_ONLY:
			v["environment"] = std::string("Outdoor only");
			break;
		case Environment::INDOOR_OUTDOOR:
			v["environment"] = std::string("Indoor/Outdoor");
			break;
		default:
			v["environment"] = std::string("Invalid");
			break;
	}

	v["triplets"] = Json::Value{Json::arrayValue};
	for (auto& triplet : triplets) {
		v["triplets"].append(triplet.make_json());
	}


	return v;
}

IE_RSN::IE_RSN(uint8_t id_, uint8_t len_, uint8_t* buf)
	: IE(id_, len_, buf)
{
	// iw scan.c print_rsn()
	const char *defcipher = "CCMP";
	const char *defauth = "IEEE 802.1X";

	decode_rsn_ie(defcipher, defauth, 0);
}

Json::Value IE_RSN::make_json(void)
{
	Json::Value v { IE::make_json() };
	v["version"] = version;
	v["group_cipher"] = Json::Value { Json::stringValue };
	v["pairwise_ciphers"] = Json::Value { Json::arrayValue };
//	v["pairwise_ciphers"].append(pairwise_cipher);
	return v;
}

//IE_RSN::CipherSuite::CipherSuite()
//{
//	// default cipher
//	// iw scan.c print_rsn()
////	const char *defcipher = "CCMP";
//
//	uint8_t data[4] = "\x00\x0f\0xac\x04";
//	CipherSuite::CipherSuite(data);
//}

IE_RSN::CipherSuite::CipherSuite(std::array<uint8_t,4> data)
{
	CipherSuite(data.data());
}


// iw scan.c print_cipher()
IE_RSN::CipherSuite::CipherSuite(const uint8_t *data)
{
	memcpy(oui.data(), data, 3);
	cipher = data[3];

	oui_hex = fmt::format("{0:02x}-{1:02x}-{2:02x}",
		data[0], data[1] ,data[2]);
#if 0
	// TODO this is a transcription of scan.c print_cipher() but should I make
	// it a LUT?
	if (memcmp(data, ms_oui, 3) == 0) {
		switch (data[3]) {
		case 0:
			return CipherSuite::USE_GROUP_CIPHER;
			break;
		case 1:
			return CipherSuite::WEP_40;
			break;
		case 2:
			return CipherSuite::TKIP;
			break;
		case 4:
			return CipherSuite::CCMP;
			break;
		case 5:
			return CipherSuite::WEP_104;
			break;
		default:
			return unknown_suite(data);
			break;
		}
	} else if (memcmp(data, ieee80211_oui, 3) == 0) {
		switch (data[3]) {
		case 0:
			return "Use group cipher suite";
			break;
		case 1:
			return "WEP-40";
			break;
		case 2:
			return "TKIP";
			break;
		case 4:
			return "CCMP";
			break;
		case 5:
			return "WEP-104";
			break;
		case 6:
			return "AES-128-CMAC";
			break;
		case 7:
			return "NO-GROUP";
			break;
		case 8:
			return "GCMP";
			break;
		default:
			return unknown_suite(data);
			break;
		}
	} else {
		return unknown_suite(data);
	}
#endif
}

// iw scan.c _print_rsn_ie()
// is_osen seems to be a HotSpot thing
void IE_RSN::decode_rsn_ie(const char *defcipher, const char *defauth, int is_osen)
{
	__u16 count, capa;
	int i;
	std::string s;
	const uint8_t* data = bytes.data();
	ssize_t len = bytes.size();

	(void)defcipher;
	(void)defauth;

	if (!is_osen) {
		version = data[0] + (data[1] << 8);
//		decode.push_back(fmt::format("Version: {:d}", version));

		data += 2;
		len -= 2;
	}

	if (len < 4) {
//		decode.push_back(fmt::format("Group cipher: {:s}", defcipher));
//		decode.push_back(fmt::format("Pairwise ciphers: {:s}", defcipher));
//		group_cipher.assign(defcipher);
//		pairwise_cipher.assign(defcipher);
		return;
	}

	auto cipher = IE_RSN::CipherSuite(data);
	cipher = IE_RSN::CipherSuite();
//	s = "Group cipher: " + decode_cipher(data);
//	decode.push_back(s);

	data += 4;
	len -= 4;

	if (len < 2) {
//		decode.push_back(fmt::format("Pairwise ciphers: {:s}", defcipher));
		return;
	}

	count = data[0] | (data[1] << 8);
	if (2 + (count * 4) > len)
		goto invalid;

	s = "Pairwise ciphers:";
	for (i = 0; i < count; i++) {
		s += " " + decode_cipher(data + 2 + (i * 4));
	}
//	decode.push_back(s);

	data += 2 + (count * 4);
	len -= 2 + (count * 4);

	if (len < 2) {
//		decode.push_back(fmt::format("Authentication suites: {:s}", defauth));
		return;
	}

	count = data[0] | (data[1] << 8);
	if (2 + (count * 4) > len)
		goto invalid;

	s = "Authentication suites:";
	for (i = 0; i < count; i++) {
		s += " " + decode_auth(data + 2 + (i * 4));
	}
//	decode.push_back(s);

	data += 2 + (count * 4);
	len -= 2 + (count * 4);

	if (len >= 2) {
		capa = data[0] | (data[1] << 8);
		s = "Capabilities:";
		if (capa & 0x0001)
			s += " PreAuth";
		if (capa & 0x0002)
			s += " NoPairwise";
		switch ((capa & 0x000c) >> 2) {
		case 0:
			s += " 1-PTKSA-RC";
			break;
		case 1:
			s += " 2-PTKSA-RC";
			break;
		case 2:
			s += " 4-PTKSA-RC";
			break;
		case 3:
			s += " 16-PTKSA-RC";
			break;
		}
		switch ((capa & 0x0030) >> 4) {
		case 0:
			s += " 1-GTKSA-RC";
			break;
		case 1:
			s += " 2-GTKSA-RC";
			break;
		case 2:
			s += " 4-GTKSA-RC";
			break;
		case 3:
			s += " 16-GTKSA-RC";
			break;
		}
		if (capa & 0x0040)
			s += " MFP-required";
		if (capa & 0x0080)
			s += " MFP-capable";
		if (capa & 0x0200)
			s += " Peerkey-enabled";
		if (capa & 0x0400)
			s += " SPP-AMSDU-capable";
		if (capa & 0x0800)
			s += " SPP-AMSDU-required";
		s += fmt::format(" ({:#4x})", capa);
		data += 2;
		len -= 2;
//		decode.push_back(s);
	}

	if (len >= 2) {
		int pmkid_count = data[0] | (data[1] << 8);

		if (len >= 2 + 16 * pmkid_count) {
//			decode.push_back(fmt::format("{:d} PMKIDs", pmkid_count));
			/* not printing PMKID values */
			data += 2 + 16 * pmkid_count;
			len -= 2 + 16 * pmkid_count;
		} else
			goto invalid;
	}

	if (len >= 4) {
		s = "Group mgmt cipher suite: ";
		s += decode_cipher(data);
//		decode.push_back(s);
		data += 4;
		len -= 4;
	}

 invalid:
	if (len != 0) {
		// FIXME
		assert(0);
//		std::vector<uint8_t> tail;
//		tail.assign(data, len);
//		logger->error("bogus tail data ({:d}):{}", len, hex_dump_bytes(tail));
		
//		while (len) {
//			printf(" %.2x", *data);
//			data++;
//			len--;
//		}
	}
}

IE_Vendor::IE_Vendor(uint8_t id_, uint8_t len_, uint8_t* buf)
	: IE(id_, len_, buf)
{
	// TODO add vendor name lookup
//	ieeeoui::OUI oui { "oui.csv" };
//	orgname = oui.get_org_name(buf);
	this->oui = hexdump.substr(0,6);
}

Json::Value IE_Vendor::make_json(void)
{
	Json::Value v { IE::make_json() };
	// http://standards-oui.ieee.org/oui/oui.txt
	// http://standards-oui.ieee.org/oui/oui.csv
	v["oui"] = oui;
	v["org"] = orgname;
	return v;
}

std::shared_ptr<IE> cfg80211::make_ie(uint8_t id, uint8_t len, uint8_t* buf)
{
	// TODO is there a way to make this whole function a LUT ?
	// something with lambdas?
//	auto make_fn = [](uint8_t id, uint8_t len, uint8_t* buf) -> std::shared_ptr<IE> {
//		return std::make_shared<IE_SSID>(id,len,buf);
//	};
//	make_fn(id,len,buf);

	auto logger = spdlog::get("ie");
	logger->debug("{} id={} len={}", __func__, id, len);

	switch (id) {
		case 0:
			return std::make_shared<IE_SSID>(id,len,buf);

		case 1:
			return std::make_shared<IE_SupportedRates>(id,len,buf);

		case 3: // DSS Paramter Set
			return std::make_shared<IE_Integer>(id,len,buf);

		case 7:
			return std::make_shared<IE_Country>(id,len,buf);

		case 48:
			return std::make_shared<IE_RSN>(id,len,buf);

		case 221:
			return std::make_shared<IE_Vendor>(id,len,buf);

		default:
			return std::make_shared<IE>(id,len,buf);

	}

//	make_fn = [](uint8_t id, uint8_t len, uint8_t* buf) -> std::shared_ptr<IE> {
//		return std::make_shared<IE_Integer>(id,len,buf);
//	};
}

std::ostream& operator<<(std::ostream& os, const cfg80211::IE& ie)
{
	// The id and len are uint8_t which confuses ostream. So promote them to
	// official ints and ostream is happy.

//	std::cerr << "err id=" << static_cast<int>(ie.id) << " len=" << static_cast<int>(ie.len)  
//		<< " name=" << (ie.name?ie.name:"undefined") << std::endl;

	os << "id=" << static_cast<int>(ie.id) << " len=" << static_cast<int>(ie.len) 
		<< " name=" << (ie.name?ie.name:"undefined") ;
	return os;
}

