#include <iostream>
#include <array>
#include <vector>
#include <cassert>
#include <boost/assert.hpp>

#include <sys/socket.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/nl80211.h>

#include "fmt/format.h"
#include "logging.h"
#include "ie.hh"

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

// iw scan.c print_ds()
static std::string decode_ds(Blob bytes)
{
	return std::to_string(static_cast<int>(bytes[0]));
}

// iw scan.c print_supprates()
#define BSS_MEMBERSHIP_SELECTOR_VHT_PHY 126
#define BSS_MEMBERSHIP_SELECTOR_HT_PHY 127
static std::string decode_supprates(Blob bytes)
{
	size_t i;
	std::string s;

	for (i = 0; i < bytes.size(); i++) {
		uint8_t byte = bytes.data()[i];
		int r = byte & 0x7f;

		if (r == BSS_MEMBERSHIP_SELECTOR_VHT_PHY && byte & 0x80)
			s += "VHT";
		else if (r == BSS_MEMBERSHIP_SELECTOR_HT_PHY && byte & 0x80)
			s += "HT";
		else
			s += fmt::format("{}.{}", r/2, 5*(r&1));
//			printf("%d.%d", r/2, 5*(r&1));

		s += (byte & 0x80) ? "* " : " ";
	}

	return s;
}

// iw scan.c print_tim()
static std::string decode_tim(Blob bytes)
{
	std::string s = fmt::format(" DTIM Count {:d} DTIM Period {:d} Bitmap Control {:#x} Bitmap[0] {:#x}",
	       bytes.data()[0], bytes.data()[1], bytes.data()[2], bytes.data()[3]);

	if (bytes.size() - 4)
		s += fmt::format(" (+ {:d} octet{:s})", bytes.size() - 4, bytes.size() - 4 == 1 ? "" : "s");
	return s;
}

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

static void decode_country(Blob bytes, std::vector<std::string>& decode)
{
	uint8_t *data = bytes.data();
	ssize_t len = bytes.size();

//	printf(" %.*s", 2, data);
	decode.emplace_back(std::string(reinterpret_cast<char *>(data), 2));

//	printf("\tEnvironment: %s\n", country_env_str(data[2]));

	data += 3;
	len -= 3;

	if (len < 3) {
		decode.push_back("No country IE triplets present");
		return;
	}

	while (len >= 3) {
		int end_channel;
		union ieee80211_country_ie_triplet *triplet = reinterpret_cast<union ieee80211_country_ie_triplet*>(data);

		if (triplet->ext.reg_extension_id >= IEEE80211_COUNTRY_EXTENSION_ID) {
			decode.push_back(fmt::format("Extension ID: {:d} Regulatory Class: {:d} Coverage class: {:d} (up to {:d}m)",
			       triplet->ext.reg_extension_id,
			       triplet->ext.reg_class,
			       triplet->ext.coverage_class,
			       triplet->ext.coverage_class * 450));

			data += 3;
			len -= 3;
			continue;
		}

		/* 2 GHz */
		if (triplet->chans.first_channel <= 14)
			end_channel = triplet->chans.first_channel + (triplet->chans.num_channels - 1);
		else
			end_channel =  triplet->chans.first_channel + (4 * (triplet->chans.num_channels - 1));

		decode.push_back(fmt::format("Channels [{:d} - {:d}] @ {:d} dBm", triplet->chans.first_channel, end_channel, triplet->chans.max_power));

		data += 3;
		len -= 3;
	}

}

#if 0
static void print_country(const uint8_t type, uint8_t len, const uint8_t *data,
			  const struct print_ies_data *ie_buffer)
{
	printf(" %.*s", 2, data);

	printf("\tEnvironment: %s\n", country_env_str(data[2]));

	data += 3;
	len -= 3;

	if (len < 3) {
		printf("\t\tNo country IE triplets present\n");
		return;
	}

	while (len >= 3) {
		int end_channel;
		union ieee80211_country_ie_triplet *triplet = (void *) data;

		if (triplet->ext.reg_extension_id >= IEEE80211_COUNTRY_EXTENSION_ID) {
			printf("\t\tExtension ID: %d Regulatory Class: %d Coverage class: %d (up to %dm)\n",
			       triplet->ext.reg_extension_id,
			       triplet->ext.reg_class,
			       triplet->ext.coverage_class,
			       triplet->ext.coverage_class * 450);

			data += 3;
			len -= 3;
			continue;
		}

		/* 2 GHz */
		if (triplet->chans.first_channel <= 14)
			end_channel = triplet->chans.first_channel + (triplet->chans.num_channels - 1);
		else
			end_channel =  triplet->chans.first_channel + (4 * (triplet->chans.num_channels - 1));

		printf("\t\tChannels [%d - %d] @ %d dBm\n", triplet->chans.first_channel, end_channel, triplet->chans.max_power);

		data += 3;
		len -= 3;
	}

	return;
}
#endif

// helper function to decode an Information Element blob
// going to pretty much copy iw's scan.c decode fns
static void decode_ie(int id, size_t len, Blob bytes, std::vector<std::string>& decode)
{
	(void)len;

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

		default:
			// TODO
			decode.emplace_back("(no decode)");
			break;
	}
	BOOST_ASSERT(decode.size() > 0);
}

class IE_Names
{
	public:
		IE_Names();

		// "IEEE Std 802.11-2012"
		// Table 8-54—Element IDs
		std::array<const char*, 256> names {
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
};

IE_Names::IE_Names()
{
	// fill in some holes
	names[221] = "Vendor specific";

	// from iw scan.c ieprinters[]
	names[191] = "VHT capabilities";
	names[192] = "VHT operation";
	names[107] = "802.11u Interworking";
	names[108] = "802.11u Advertisement",
	names[111] = "802.11u Roaming Consortium";

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

IE::IE(uint8_t id, uint8_t len, uint8_t *buf) 
	: id{id}, 
	  len{len},
	  bytes{},
	  name {nullptr},
	  decode {},
	  logie {nullptr}
{
	logie = spdlog::get("ie");
	if (!logie) {
		logie = spdlog::stdout_logger_mt("ie");
	}

	bytes.assign(buf, buf+len);

	name = ie_names.names.at(id);
	if (!name) {
		logie->error("failed to find id={}", id);
		logie->flush();
		assert(name);
	}

	int int_id = static_cast<int>(id);
	size_t int_len = static_cast<size_t>(len);

	decode_ie(int_id, int_len, bytes, decode);
	logie->debug("construct ie name=\"{}\" id={} len={} {}", 
			name, int_id, int_len, decode.at(0));
}

std::ostream& operator<<(std::ostream& os, const cfg80211::IE& ie)
{
	// The id and len are uint8_t which confuses ostream. So promote them to
	// official ints and ostream is happy.

//	std::cerr << "err id=" << static_cast<int>(ie.id) << " len=" << static_cast<int>(ie.len)  
//		<< " name=" << (ie.name?ie.name:"undefined") << std::endl;

	os << "id=" << static_cast<int>(ie.id) << " len=" << static_cast<int>(ie.len) 
		<< " name=" << (ie.name?ie.name:"undefined") 
		<< " \"" << ie.decode.at(0) << "\"";
	return os;
}

