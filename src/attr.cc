#include <cassert>

#include <netlink/attr.h>
#include <linux/nl80211.h>

#include "logging.h"
#include "attr.h"

// Notes:
// https://www.infradead.org/~tgr/libnl/doc/core.html#core_attr
// https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/nl80211.h#L2306

void decode_nl80211_attr(struct nlattr* tb_msg[], size_t counter)
{

	if (tb_msg[NL80211_ATTR_WIPHY]) {
		counter--;
		uint32_t phy_id = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]);
		printf("phy_id=%u\n", phy_id);
	}

	if (tb_msg[NL80211_ATTR_WIPHY_NAME]) {
		counter--;
		const char* p = nla_get_string(tb_msg[NL80211_ATTR_WIPHY_NAME]);
//		printf("phy_name=%s\n", p);
	}

	if (tb_msg[NL80211_ATTR_IFINDEX]) {
		counter--;
		uint32_t ifidx = nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]);
//		printf("ifindex=%d\n", nla_get_u32(tb_msg[NL80211_ATTR_IFINDEX]));
	}

	if (tb_msg[NL80211_ATTR_IFNAME]) {
		counter--;
		const char* ifname = nla_get_string(tb_msg[NL80211_ATTR_IFNAME]);
//		printf("ifname=%s\n", nla_get_string(tb_msg[NL80211_ATTR_IFNAME]));
	}

	if (tb_msg[NL80211_ATTR_IFTYPE]) {
		counter--;
		struct nlattr *attr = tb_msg[NL80211_ATTR_IFTYPE];
		enum nl80211_iftype* iftype = static_cast<enum nl80211_iftype*>(nla_data(attr));
		(void)iftype;
//		printf("iftype=%d\n", *iftype);
	}

	if (tb_msg[NL80211_ATTR_MAC]) {
		counter--;
		struct nlattr *attr = tb_msg[NL80211_ATTR_MAC];
//		printf("attr type=%d len=%d ok=%d\n", nla_type(attr), nla_len(attr), nla_ok(attr,0));
		uint8_t *mac = static_cast<uint8_t*>(nla_data(attr));
		(void)mac;
//		hex_dump("mac", mac, nla_len(attr));
	}

	if (tb_msg[NL80211_ATTR_KEY_DATA]) {
		counter--;
//		printf("keydata=?\n");
	}

	if (tb_msg[NL80211_ATTR_GENERATION]) {
		counter--;
		uint32_t attr_gen = nla_get_u32(tb_msg[NL80211_ATTR_GENERATION]);
		(void)attr_gen;
//		printf("attr generation=%#" PRIx32 "\n", attr_gen);
	}

	if (tb_msg[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]) {
		counter--;
		uint32_t tx_power = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]);
		(void)tx_power;
//		printf("tx_power_level=%" PRIu32 "\n", tx_power);
		// printf taken from iw-4.14 interface.c print_iface_handler()
//		printf("tx_power %d.%.2d dBm\n", tx_power / 100, tx_power % 100);
	}

	if (tb_msg[NL80211_ATTR_WDEV]) {
		counter--;
		uint64_t wdev = nla_get_u64(tb_msg[NL80211_ATTR_WDEV]);
		(void)wdev;
//		printf("wdev=%#" PRIx64 "\n", wdev);
	}

	if (tb_msg[NL80211_ATTR_CHANNEL_WIDTH]) {
		counter--;
		enum nl80211_chan_width w = static_cast<enum nl80211_chan_width>(nla_get_u32(tb_msg[NL80211_ATTR_CHANNEL_WIDTH]));
		(void)w;
//		printf("channel_width=%" PRIu32 "\n", w);
	}

	if (tb_msg[NL80211_ATTR_WIPHY_CHANNEL_TYPE]) {
		counter--;
//		printf("channel_type=?\n");
	}

	if (tb_msg[NL80211_ATTR_WIPHY_FREQ]) {
		counter--;
		uint32_t wiphy_freq = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_FREQ]);
		(void)wiphy_freq;
//		printf("wiphy_freq=%" PRIu32 "\n", wiphy_freq);
	}

	if (tb_msg[NL80211_ATTR_CENTER_FREQ1]) {
		counter--;
		uint32_t center_freq1 = nla_get_u32(tb_msg[NL80211_ATTR_CENTER_FREQ1]);
		(void)center_freq1;
//		printf("center_freq1=%" PRIu32 "\n", center_freq1);
	}

	if (tb_msg[NL80211_ATTR_CENTER_FREQ2]) {
		counter--;
		uint32_t center_freq2 = nla_get_u32(tb_msg[NL80211_ATTR_CENTER_FREQ2]);
		(void)center_freq2;
//		printf("center_freq2=%" PRIu32 "\n", center_freq2);
	}

	if (tb_msg[NL80211_ATTR_WIPHY_TXQ_PARAMS]) {
		counter--;
		printf("txq_params=?\n");
	}

	if (tb_msg[NL80211_ATTR_STA_INFO]) {
		counter--;
		/* @NL80211_ATTR_STA_INFO: information about a station, part of station info
		 *  given for %NL80211_CMD_GET_STATION, nested attribute containing
		 *  info as possible, see &enum nl80211_sta_info.
		 */
		/* davep 20190323 ; TODO */
		assert(0);
//		printf("sta info=?\n");
//		print_sta_handler(msg, arg);
	}

	if (tb_msg[NL80211_ATTR_BANDS]) {
		counter--;
		/* @NL80211_ATTR_BANDS: operating bands configuration.  This is a u32
		 *	bitmask of BIT(NL80211_BAND_*) as described in %enum
		 *	nl80211_band.  For instance, for NL80211_BAND_2GHZ, bit 0
		 *	would be set.  This attribute is used with
		 *	%NL80211_CMD_START_NAN and %NL80211_CMD_CHANGE_NAN_CONFIG, and
		 *	it is optional.  If no bands are set, it means don't-care and
		 *	the device will decide what to use.
		 */

		enum nl80211_band_attr band = static_cast<enum nl80211_band_attr>(nla_get_u32(tb_msg[NL80211_ATTR_BANDS]));
		(void)band;
		// results are kinda boring ... 
//		printf("attr_bands=%#" PRIx32 "\n", band);
	}

	if (tb_msg[NL80211_ATTR_BSS]) {
		counter--;
		assert(0); // TODO
//		printf("bss=?\n");
//		decode_attr_bss(tb_msg[NL80211_ATTR_BSS]);
	}

	if (tb_msg[NL80211_ATTR_SCAN_FREQUENCIES]) {
		counter--;
		printf("scan_frequencies\n");
		struct nlattr *nst;
		int rem_nst;
		nla_for_each_nested(nst, tb_msg[NL80211_ATTR_SCAN_FREQUENCIES], rem_nst)
			printf(" %d", nla_get_u32(nst));
		printf(",");
	}

	if (tb_msg[NL80211_ATTR_SCAN_SSIDS]) {
		counter--;
		printf("scan_ssids\n");
		struct nlattr *nst;
		int rem_nst;
		nla_for_each_nested(nst, tb_msg[NL80211_ATTR_SCAN_SSIDS], rem_nst) {
			printf(" \"");
//			print_ssid_escaped(nla_len(nst), nla_data(nst));
			printf("\"");
		}
	}

}
