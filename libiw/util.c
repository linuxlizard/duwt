#include <stdio.h>
#include <inttypes.h>
#include <linux/if_ether.h>
#include <linux/nl80211.h>

#include "core.h"
#include "iw.h"

//
// contains copied chunks from from iw util.c
//

// map between channel bandwidth and string
static const struct {
	const char *name;
	unsigned int val;
} bwmap[] = {
	{ .name = "5", .val = NL80211_CHAN_WIDTH_5, },
	{ .name = "10", .val = NL80211_CHAN_WIDTH_10, },
	{ .name = "20-noht", .val = NL80211_CHAN_WIDTH_20_NOHT, },
	{ .name = "20", .val = NL80211_CHAN_WIDTH_20, },
	{ .name = "40", .val = NL80211_CHAN_WIDTH_40, },
	{ .name = "80", .val = NL80211_CHAN_WIDTH_80, },
	{ .name = "80+80", .val = NL80211_CHAN_WIDTH_80P80, },
	{ .name = "160", .val = NL80211_CHAN_WIDTH_160, },
};

const char* bw_str(enum nl80211_chan_width w)
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(bwmap); i++) {
		if (w==bwmap[i].val) {
			return bwmap[i].name;
		}
	}
	return "???";
}

void mac_addr_n2a(char *mac_addr, const unsigned char *arg)
{
	int i, l;

	l = 0;
	for (i = 0; i < ETH_ALEN ; i++) {
		if (i == 0) {
			sprintf(mac_addr+l, "%02x", arg[i]);
			l += 2;
		} else {
			sprintf(mac_addr+l, ":%02x", arg[i]);
			l += 3;
		}
	}
}

int tsf_to_timestamp_str(uint64_t tsf, char* s, size_t len)
{
	// iw print_bss_handler() scan.c
//	return snprintf(s, len, "%llud, %.2lld:%.2llu:%.2llu",
	return snprintf(s, len, "%"PRIu64"d, %.2"PRId64":%.2"PRIu64":%.2"PRIu64"",
		tsf/1000/1000/60/60/24,  // days
		(tsf/1000/1000/60/60) % 24,  // hours
		(tsf/1000/1000/60) % 60, // minutes
		(tsf/1000/1000) % 60);  // seconds
}

// iw scan.c
const char *country_env_str(enum Environment environment)
{
	switch (environment) {
	case ENV_INDOOR_ONLY:
		return "Indoor only";
	case ENV_OUTDOOR_ONLY:
		return "Outdoor only";
	case ENV_INDOOR_OUTDOOR:
		return "Indoor/Outdoor";
	case ENV_INVALID:
	default:
		return "bogus";
	}
}

// iw util.c
int ieee80211_channel_to_frequency(int chan, enum nl80211_band band)
{
	/* see 802.11 17.3.8.3.2 and Annex J
	 * there are overlapping channel numbers in 5GHz and 2GHz bands */
	if (chan <= 0)
		return 0; /* not supported */
	switch (band) {
	case NL80211_BAND_2GHZ:
		if (chan == 14)
			return 2484;
		else if (chan < 14)
			return 2407 + chan * 5;
		break;
	case NL80211_BAND_5GHZ:
		if (chan >= 182 && chan <= 196)
			return 4000 + chan * 5;
		else
			return 5000 + chan * 5;
		break;
	case NL80211_BAND_60GHZ:
		if (chan < 5)
			return 56160 + chan * 2160;
		break;
#ifdef HAVE_NUM_NL80211_BANDS
	case NUM_NL80211_BANDS:
#endif
	default:
		;
	}
	return 0; /* not supported */
}

// iw util.c
int ieee80211_frequency_to_channel(int freq)
{
	/* see 802.11-2007 17.3.8.3.2 and Annex J */
	if (freq == 2484)
		return 14;
	else if (freq < 2484)
		return (freq - 2407) / 5;
	else if (freq >= 4910 && freq <= 4980)
		return (freq - 4000) / 5;
	else if (freq <= 45000) /* DMG band lower limit */
		return (freq - 5000) / 5;
	else if (freq >= 58320 && freq <= 64800)
		return (freq - 56160) / 2160;
	else
		return 0;
}

int capability_to_str(uint16_t capa, char* s, size_t len)
{
	return snprintf(s, len, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", 
		(capa & WLAN_CAPABILITY_ESS ? "ESS " : ""),
		(capa & WLAN_CAPABILITY_IBSS ? "IBSS " : ""),
		(capa & WLAN_CAPABILITY_CF_POLLABLE ? "CfPollable " : ""),
		(capa & WLAN_CAPABILITY_CF_POLL_REQUEST ? "CfPollReq " : ""),
		(capa & WLAN_CAPABILITY_PRIVACY ? "Privacy " : ""),
		(capa & WLAN_CAPABILITY_SHORT_PREAMBLE ? "ShortPreamble " : ""),
		(capa & WLAN_CAPABILITY_PBCC ? "PBCC " : ""),
		(capa & WLAN_CAPABILITY_CHANNEL_AGILITY ? "ChannelAgility " : ""),
		(capa & WLAN_CAPABILITY_SPECTRUM_MGMT ? "SpectrumMgmt " : ""),
		(capa & WLAN_CAPABILITY_QOS ? "QoS " : ""),
		(capa & WLAN_CAPABILITY_SHORT_SLOT_TIME ? "ShortSlotTime " : ""),
		(capa & WLAN_CAPABILITY_APSD ? "APSD " : ""),
		(capa & WLAN_CAPABILITY_RADIO_MEASURE ? "RadioMeasure " : ""),
		(capa & WLAN_CAPABILITY_DSSS_OFDM ? "DSSS-OFDM " : ""),
		(capa & WLAN_CAPABILITY_DEL_BACK ? "DelayedBACK " : ""),
		(capa & WLAN_CAPABILITY_IMM_BACK ? "ImmediateBACK " : "")
	);
}

int erp_to_str(const struct IE_ERP* sie, char* s, size_t len)
{
	if (!sie->base->buf[0]) {
		return snprintf(s, len, " <no flags>");
	}

	return snprintf(s, len, "%s%s%s",
			sie->NonERP_Present ? " NonERP_Present" : "",
			sie->Use_Protection ? " Use_Protection" : "",
			sie->Barker_Preamble_Mode ? " Barker_Preamble_Mode" : ""
		);
}

// iw util.c
/*
 * There are only 4 possible values, we just use a case instead of computing it,
 * but technically this can also be computed through the formula:
 *
 * Max AMPDU length = (2 ^ (13 + exponent)) - 1 bytes
 */
static uint32_t compute_ampdu_length(uint8_t exponent)
{
	switch (exponent) {
	case 0: return 8191;  /* (2 ^(13 + 0)) -1 */
	case 1: return 16383; /* (2 ^(13 + 1)) -1 */
	case 2: return 32767; /* (2 ^(13 + 2)) -1 */
	case 3: return 65535; /* (2 ^(13 + 3)) -1 */
	default: return 0;
	}
}

int ht_ampdu_length_to_str(uint8_t exponent, char* s, size_t len)
{
	// iw util.c print_ampdu_length()
	uint32_t max_ampdu_length;

	max_ampdu_length = compute_ampdu_length(exponent);

	if (max_ampdu_length) {
		return snprintf(s, len, "Maximum RX AMPDU length %d bytes (exponent: 0x0%02x)",
		       max_ampdu_length, exponent);
	}
	return snprintf(s, len, "Maximum RX AMPDU length: unrecognized bytes "
		   "(exponent: %d)", exponent);
}

const char* ht_max_amsdu_str(uint8_t max_amsdu)
{
	// iw scan.c print_capabilities()
	static const char* const amsdu_str[] = {
		"unlimited", "32", "16", "8"
	};

	if (max_amsdu > 3) {
		WARN("%s invalid value=%u for max_amdu\n", __func__, max_amsdu);
		return "invalid value";
	}

	return amsdu_str[max_amsdu];
}

// iw util.c print_ampdu_spacing()
int ht_ampdu_spacing_to_str(uint8_t spacing, char* s, size_t len)
{
	// strings from iw util.c print_ampdu_space(0
	static const char* const space_str[] = {
		"No restriction",
		"1/4 usec",
		"1/2 usec",
		"1 usec",
		"2 usec",
		"4 usec",
		"8 usec",
		"16 usec",
	};

	if (spacing > 7) {
		return -EINVAL;
	}

	return snprintf(s, len, "Minimum RX AMPDU time spacing: %s (0x%02x)",
	       space_str[spacing], spacing);
}

const char* ht_secondary_offset_str(uint8_t off)
{
	// strings from iw scan.c ht_secondary_offset
	static const char* str[] = {
		"no secondary",
		"above",
		"[reserved!]",
		"below",
	};

	if (off > 4) {
		return "invalid!";
	}
	return str[off];
}

const char* ht_sta_channel_width_str(uint8_t w)
{
	// strings from iw scan print_ht_op()
	static const char* width_str[] = {
		"20 MHz", "any",
	};

	if (w > 1) {
		return "invalid!";
	}
	return width_str[w];
}

const char* ht_protection_str(uint8_t prot)
{
	// strings from iw scan.c print_ht_op()
	static const char* prot_str[] = {
		"no",
		"nonmember",
		"20 Mhz",
		"non-HT mixed",
	};
	if (prot > 3) {
		return "invalid!";
	}
	return prot_str[prot];
}

int mcs_index_bitmask_to_str(const uint8_t* buf, char* s, size_t len)
{
	// TODO
	(void)buf;
	(void)s;
	(void)len;
	// assuming buf is array of 80 bytes
	return 0;
}

const char* vht_max_mpdu_length_str(uint8_t len)
{
	static const char* const len_str[] = {
		"3895", "7991", "11454", "(reserved)"
	};

	if (len > 4) {
		return "(invalid)";
	}
	return len_str[len];
}

const char* vht_supported_channel_width_str(uint8_t w)
{
	static const char* const width_str[] = {
		"neither 160 nor 80+80",
		"160 Mhz",
		"160 Mhz, 80+80 Mhz",
		"(reserved)"
	};

	if (w > 3) {
		return "(invalid)";
	}
	return width_str[w];
}

const char* vht_channel_width_str(uint8_t w)
{
	const char* const width_str[] = {
		"20 or 40 MHz",
		"80 MHz",
		"160 MHz",
		"80+80 MHz",
	};

	if (w > 3) {
		return "(invalid)";
	}
	return width_str[w];
}

static int unknown_suite(const struct RSN_Cipher_Suite* suite, char* s, size_t len)
{
	return snprintf(s, len, "%02x-%02x-%02x-%3d", 
			suite->oui[0],
			suite->oui[1],
			suite->oui[2],
			suite->type);
}

// iw scan.c print_cipher()
int cipher_suite_to_str(const struct RSN_Cipher_Suite* suite, char* s, size_t len)
{
	static const char* const ms_str[] = {
		"Use group cipher suite",
		"WEP-40",
		"TKIP",
		"(Reserved)", 
		"CCMP",
		"WEP-104",
	};

	static const char* const ieee80211_str[] = {
		"Use group cipher suite",
		"WEP-40",
		"TKIP",
		"(Reserved)", 
		"CCMP",
		"WEP-104",
		"AES-128-CMAC",
		"NO-GROUP",
		"GCMP",
	};

	if (memcmp(suite->oui, ms_oui, 3) == 0) {
		if (suite->type == 3 || suite->type > 4) {
			return unknown_suite(suite, s, len);
		}
		return snprintf(s, len, "%s", ms_str[suite->type]);
	} else if (memcmp(suite->oui, ieee80211_oui, 3) == 0) {
		if (suite->type == 3 || suite->type > 7) {
			return unknown_suite(suite, s, len);
		}
		return snprintf(s, len, "%s", ieee80211_str[suite->type]);
	} 
	return unknown_suite(suite, s, len);
}

// iw scan.c print_auth() 
int auth_to_str(const struct RSN_Cipher_Suite* suite, char* s, size_t len)
{
	static const char* const ms_str[] = {
		"", // reserved
		"IEEE 802.1X",
		"PSK",
	};
	static const char* const ieee80211_str[] = {
		"", // reserved 
		"IEEE 802.1X",
		"PSK",
		"FT/IEEE 802.1X",
		"FT/PSK",
		"IEEE 802.1X/SHA-256",
		"PSK/SHA-256",
		"TDLS/TPK",
		"SAE",
		"FT/SAE",
		"IEEE 802.1X/SUITE-B",
		"IEEE 802.1X/SUITE-B-192",
		"FT/IEEE 802.1X/SHA-384",
		"FILS/SHA-256",
		"FILS/SHA-384",
		"FT/FILS/SHA-256",
		"FT/FILS/SHA-384",
		"OWE",
	};

	static const char* const wfa_str[] = {
		"", // reserved
		"OSEN",
		"DPP",
	};

	if (memcmp(suite->oui, ms_oui, 3) == 0) {
		if (suite->type == 0 || suite->type > 3) {
			return unknown_suite(suite, s, len);
		}
		return snprintf(s, len, "%s", ms_str[suite->type]);

	} else if (memcmp(suite->oui, ieee80211_oui, 3) == 0) {
		if (suite->type == 0 || suite->type > 18) {
			return unknown_suite(suite, s, len);
		}
		return snprintf(s, len, "%s", ieee80211_str[suite->type]);

	} else if (memcmp(suite->oui, wfa_oui, 3) == 0) {
		if (suite->type == 0 || suite->type > 3) {
			return unknown_suite(suite, s, len);
		}
		return snprintf(s, len, "%s", wfa_str[suite->type]);
	} 
	return unknown_suite(suite, s, len);
}

int rsn_capabilities_to_str(const struct IE_RSN* sie, char* s, size_t len)
{
	static const char* ptksa_str[] = {
		" 1-PTKSA-RC",
		" 2-PTKSA-RC",
		" 4-PTKSA-RC",
		" 16-PTKSA-RC",
	};
	static const char* gtksa_str[] = {
		" 1-GTKSA-RC",
		" 2-GTKSA-RC",
		" 4-GTKSA-RC",
		" 16-GTKSA-RC",
	};

	// strings from iw scan.c _print_rsn_ie()
	return snprintf(s, len, "%s%s%s%s%s%s%s%s%s%s%s%s",
			sie->preauth ? " PreAuth" : "",
			sie->no_pairwise ? " NoPairwise" : "",
			ptksa_str[sie->ptksa_rc],
			gtksa_str[sie->gtksa_rc],
			sie->mfp_required ? " MFP-required" : "",
			sie->mfp_capable ? " MFP-capable" : "",
			sie->multiband_rsna ? " JointMulti-bandRSNA": "",
			sie->peerkey_enabled ? " Peerkey-enabled" : "",
			sie->spp_amsdu_capable ? " SPP-AMSDU-capable" : "",
			sie->spp_amsdu_required ? " SPP-AMSDU-required": "",
			sie->pbac ? " PBAC" : "",
			sie->extkey_id ? " extKeyID" : ""
			);

}

int rm_enabled_capa_to_str(const struct IE_RM_Enabled_Capabilities* sie, unsigned int idx, char* s, size_t len)
{
	static const char* rm_str[] = {
		// bit 0
		"Link Measurement",
		"Neighbor Report",
		"Parallel Measurements",
		"Repeated Measurements",
		"Beacon Passive Measurement",
		"Beacon Active Measurement",
		"Beacon Table Measurement",
		"Beacon Measurement Reporting Conditions",

		// bit 8
		"Frame Measurement",
		"Channel Load Measurement",
		"Noise Histogram Measurement",
		"Statistics Measurement",
		"LCI Measurement",
		"LCI Azimuth",
		"Transmit Stream/Category Measurement",
		"Triggered Transmit Stream/Category Measurement",

		// bit 16
		"AP Channel Report",
		"RM MIB",
		"Operating Channel Max Measurement Duration", // 3 bits
		NULL, 
		NULL, // placeholders
		"Nonoperating Channel Max Measurement Duration", // 3 bits
		NULL, 
		NULL, // placeholders

		// bit 24
		"Measurement Pilot", // 3 bits
		NULL, 
		NULL, // placeholders
		"Measurement Pilot Transmission Information",
		"Neighbor Report TSF Offset",
		"RCPI Measurement",
		"RSNI Measurement",
		"BSS Average Access Delay",

		// bit 32
		"BSS Available Admission Capacity",
		"Antenna",
		"FTM Range Report",
		"Civic Location Measurement",
	};

//	DBG("%s idx=%d\n", __func__, idx);

	// Trying something new. With this many strings, maybe can do an
	// iterator-style look-up.
	if (idx >= ARRAY_SIZE(rm_str)) {
		return -EINVAL;
	}

	if (!rm_str[idx]) {
		return -ENOENT;
	}

	int ret;
	switch (idx) {
		case 18:
			ret = snprintf(s, len, "%s %d", rm_str[idx],
					sie->operating_channel_max);
			break;

		case 21:
			ret = snprintf(s, len, "%s %d", rm_str[idx],
					sie->nonoperating_channel_max);
			break;

		case 24:
			ret = snprintf(s, len, "%s %d", rm_str[idx],
					sie->measurement_pilot_capa);
			break;

		default:
			ret = snprintf(s, len, "%s", rm_str[idx]);
			break;
	}

	return ret;
}

int mobility_domain_to_str(const struct IE_Mobility_Domain* sie, unsigned int bit, char* s, size_t len)
{
	static const char* md_str[] = {
		"Fast BSS Transition over DSS",
		"Resource Request Protocol Capability",
	};

	(void)sie;

	if (bit > 2) {
		return -EINVAL;
	}

	return snprintf(s, len, "%s", md_str[bit]);
}

