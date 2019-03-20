#include <iostream>
#include <array>
#include <cassert>

#include <sys/socket.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/nl80211.h>

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

class IE_Names
{
	public:
		IE_Names();

		// "IEEE Std 802.11-2012"
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

			"Reserved", // 30
			"Reserved", // 31
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
			"Reserved", // 128
			"Reserved", // 129

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
}

static IE_Names ie_names;

IE::IE(uint8_t id, uint8_t len, uint8_t *buf) 
	: id{id}, len{len}
{
	this->buf.assign(buf, buf+len);

	name = ie_names.names.at(id);
}

std::ostream& operator<<(std::ostream& os, const IE& ie)
{
	// The id and len are uint8_t which confuses ostream. So promote them to
	// official ints and ostream is happy.

//	std::cerr << "err id=" << static_cast<int>(ie.id) << " len=" << static_cast<int>(ie.len)  
//		<< " name=" << (ie.name?ie.name:"undefined") << std::endl;

	os << "id=" << static_cast<int>(ie.id) << " len=" << static_cast<int>(ie.len) 
		<< " name=" << (ie.name?ie.name:"undefined") ;
	return os;
}
