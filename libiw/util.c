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
