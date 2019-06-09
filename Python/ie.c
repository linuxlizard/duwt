#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "ie.h"

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

static char* names[256] = 
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

static void finish_init(void)
{
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
		if (!names[idx]) {
			names[idx] = "Reserved";
		}
	}
}

const char* ie_get_name(uint8_t id)
{
	static bool init=false;

	if (!init) {
		finish_init();
		init = true;
	}

	return (const char*)names[id];
}

int ie_decode(uint8_t id, uint8_t len, uint8_t* buf, PyObject* dest_dict)
{
	printf("%s %d %d %p\n", __func__, id, len, buf);

	return 0;
}

