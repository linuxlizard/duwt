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

const char* bw_to_str(enum nl80211_chan_width w)
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

int erp_to_str(const struct IE* ie, char* s, size_t len)
{
	const struct IE_ERP* sie = IE_CAST(ie, struct IE_ERP);

	if (!ie->buf[0]) {
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

