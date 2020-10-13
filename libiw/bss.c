/*
 * libiw/bss.c   BSS data structure
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <string.h>
#include <time.h>
#include <linux/nl80211.h>

#include "core.h"
#include "iw.h"
#include "bss.h"

struct BSS* bss_new(void)
{
	struct BSS* bss;

	bss = calloc(1, sizeof(struct BSS));
	if (!bss) {
		return NULL;
	}

	bss->cookie = BSS_COOKIE;

	int err = ie_list_init(&bss->ie_list);
	if (err) {
		PTR_FREE(bss);
		return NULL;
	}

	// enum nl80211_chan_width starts at 0 so we need a special value to
	// indicate "uninitialized"
	// center freq1, freq2 are fine at zero for uninitialized
	bss->chan_width = (enum nl80211_chan_width)-1;

	// ditto enum nl80211_band
	bss->band = (enum nl80211_band)-1;

	return bss;
}

void bss_free(struct BSS** pbss)
{
	struct BSS* bss;

	PTR_ASSIGN(bss, *pbss);
	XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);

	ie_list_release(&bss->ie_list);

	POISON(bss, sizeof(struct BSS));
	PTR_FREE(bss);
}

void bss_free_list(struct dl_list* bss_list)
{
	while( !dl_list_empty(bss_list)) {
		struct BSS* bss = dl_list_first(bss_list, typeof(*bss), node);
		if (!bss) {
			break;
		}

		XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);
		dl_list_del(&bss->node);
		XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);
		bss_free(&bss);
	}
}

int bss_from_nlattr(struct nlattr* attr_list[], struct BSS** pbss)
{
	struct nlattr* attr;
	struct BSS* bss;

	*pbss = NULL;

	if (!attr_list[NL80211_ATTR_BSS]) {
		return -EINVAL;
	}

	bss = bss_new();
	if (!bss) {
		return -ENOMEM;
	}

	int err = parse_nla_bss(attr_list[NL80211_ATTR_BSS], bss);
	if (err != 0) {
		goto fail;
	}

	if ((attr=attr_list[NL80211_ATTR_IFINDEX])) {
		bss->ifindex = nla_get_u32(attr);
	}

	bss->last_seen = time(NULL);

	PTR_ASSIGN(*pbss, bss);
	DBG("%s success\n", __func__);
	return 0;

fail:
	if (bss) {
		bss_free(&bss);
	}
	return err;
}

bool bss_is_vht(const struct BSS* bss)
{
	return ie_list_find_id(&bss->ie_list, IE_VHT_CAPABILITIES) != NULL;
}

bool bss_is_ht(const struct BSS* bss)
{
	return ie_list_find_id(&bss->ie_list, IE_HT_CAPABILITIES) != NULL;
}

int bss_guess_chan_width(struct BSS* bss)
{
	const struct IE* const ht_ie = ie_list_find_id(&bss->ie_list, IE_HT_OPERATION);
	const struct IE* const vht_ie = ie_list_find_id(&bss->ie_list, IE_VHT_OPERATION);
//	const struct IE* const he_ie = ie_list_find_ext_id(&bss->ie_list, IE_EXT_HE_CAPABILITIES );

	const struct IE_HT_Operation* ht_oper = NULL;
	const struct IE_VHT_Operation* vht_oper = NULL;

	// in order to decide the channel width, using the following:
	// 80211-2016.pdf Table 11-24 "VHT BSS bandwidth"

	// ¯\_(ツ)_/¯
	bss->chan_width = NL80211_CHAN_WIDTH_20;

	if (ht_ie) {
		ht_oper = IE_CAST(ht_ie, struct IE_HT_Operation);
		if (ht_oper->sta_channel_width != 0) {
			// 20+40 supported
			bss->chan_width = NL80211_CHAN_WIDTH_40;
		}
	}

	if (vht_ie) {
		vht_oper = IE_CAST(vht_ie, struct IE_VHT_Operation);
		bool ht_40 = ht_oper && ht_oper->sta_channel_width != 0;

		switch (vht_oper->channel_width) {
			case IE_VHT_OPER_CHANNEL_WIDTH_20_40:
				if (ht_40) {
					bss->chan_width = NL80211_CHAN_WIDTH_40;
				}
				break;

			case IE_VHT_OPER_CHANNEL_WIDTH_80_160_80P80:
				if (ht_40) {
					if (vht_oper->channel_center_freq_segment_1 == 0) {
						bss->chan_width = NL80211_CHAN_WIDTH_80;
					}
					else {
						int ccfs_delta = vht_oper->channel_center_freq_segment_0 - 
										vht_oper->channel_center_freq_segment_1;
						if (ccfs_delta < 0) {
							ccfs_delta = -ccfs_delta;
						}
						if (ccfs_delta == 8) {
							bss->chan_width = NL80211_CHAN_WIDTH_160;
						}
						else {
							bss->chan_width = NL80211_CHAN_WIDTH_80P80;
						}

					}
				}
				break;

			case IE_VHT_OPER_CHANNEL_WIDTH_160 :
				// deprecated according to 80211-2016.pdf
				bss->chan_width = NL80211_CHAN_WIDTH_160;
				break;

			case IE_VHT_OPER_CHANNEL_WIDTH_80P80 :
				// deprecated according to 80211-2016.pdf
				bss->chan_width = NL80211_CHAN_WIDTH_80P80;
				break;

			default:
				break;
		}
	}

#if 0
	// davep 20200714: TODO need to revisit HE
	if (he_ie) {
		const struct IE_HE_Capabilities* he_capa = IE_CAST(he_ie, struct IE_HE_Capabilities);
		const struct IE_HE_PHY* he_phy = he_capa->phy;

		if (bss->band == NL80211_BAND_2GHZ) {
			if (he_phy->ch40mhz_channel_2_4ghz) {
				bss->chan_width = NL80211_CHAN_WIDTH_40;
			}
		}
		else if (bss->band == NL80211_BAND_5GHZ) {
			if (he_phy->ch160_mhz_5ghz) {
				bss->chan_width = NL80211_CHAN_WIDTH_160;
			}
			else if (he_phy->ch160_80_plus_80_mhz_5ghz) {
				bss->chan_width = NL80211_CHAN_WIDTH_80P80;
			}
			else if (he_phy->ch40_and_80_mhz_5ghz) {
				bss->chan_width = NL80211_CHAN_WIDTH_80;
			}
		}
		else {
			WARN("%s %s unknown band=%d\n", __func__, bss->bssid_str, bss->band);
		}
	}
#endif
	return 0;
}

int bss_get_mode_str(const struct BSS* bss, char* s, size_t len)
{
	// determine this BSS mode
	// b b/g b/g/n b/g/n/ax 
	// a a/n a/n/ac a/n/ac/ax

	const struct IE* const ht_ie = ie_list_find_id(&bss->ie_list, IE_HT_CAPABILITIES);
	const struct IE* const vht_ie = ie_list_find_id(&bss->ie_list, IE_VHT_CAPABILITIES);
	const struct IE* const he_ie = ie_list_find_ext_id(&bss->ie_list, IE_EXT_HE_CAPABILITIES );

	// btw I'm using snprintf() so I can easily change the string to something
	// more interesting without breaking existing src

	DBG("%s band=%d\n", __func__, bss->band);

	if (bss->band == NL80211_BAND_2GHZ) {
		if (!ht_ie) {
			// no 'n' so b or b/g
			return snprintf(s, len, "b/g");
		}
		if (!he_ie) {
			// no 'ax' so b/n/n
			return snprintf(s, len, "b/g/n");
		}
		return snprintf(s, len, "b/g/n/ax");
	}
	else if (bss->band == NL80211_BAND_5GHZ) {
		if (!ht_ie) {
			// no 'n' so 'a' only 
			return snprintf(s, len, "a");
		}
		if (!vht_ie) {
			// no 'ac' so 'a/n' only
			return snprintf(s, len, "a/n");
		}
		if (!he_ie) {
			return snprintf(s, len, "a/n/ac");
		}
		return snprintf(s, len, "a/n/ac/ax");
	}
	else {
		// TODO
		return -ENOSYS;
	}

	return 0;
}

int bss_get_chan_width_str(const struct BSS* bss, char* s, size_t len)
{
	// determine available channel width
	// 20
	// 20/40
	// 20/48/80
	// 20/40/80/80+80
	// 20/40/80/80+80/160

	// TODO 
	// 40+  above
	// 40-  below

	if ((int)bss->chan_width == -1) {
		return -EINVAL;
	}

	static const char* width_str[] = {
		"20-NOHT",
		"20",
		"20/40",
		"20/40/80",
		"20/40/80/80+80",
		"20/40/80/160",   // note: 160 does not imply 80+80 
		"5",
		"10",
	};

	if (bss->chan_width > ARRAY_SIZE(width_str)) {
		return snprintf(s,len,"invalid=%d", bss->chan_width); 
	}

	return snprintf(s, len, "%s", width_str[(unsigned int)bss->chan_width]);
}

const struct IE_SSID* bss_get_ssid(const struct BSS* bss)
{
	// TODO maybe cache this IE in the struct BSS itself?
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_SSID);
	if (!ie) {
		return NULL;
	}

	return IE_CAST(ie, struct IE_SSID);
}

int bss_str_to_bss(const char* bss_str, size_t bssid_str_len, macaddr bssid)
{
	unsigned int in = 0;
	unsigned int out = 0;

#define HEX2BYTE(cin,cout)\
		if (cin >= '0' && cin <= '9') {\
			cout = cin-48;\
		}\
		else if (cin >= 'a' && cin <= 'f') {\
			cout = cin-87;\
		}\
		else if (cin >= 'A' && cin <= 'F') {\
			cout = cin-55;\
		}\
		else {\
			return -EINVAL;\
		}

	uint8_t hi,lo;
	while (out < ETH_ALEN) {
		HEX2BYTE(bss_str[in], hi); in++;
		HEX2BYTE(bss_str[in], lo); in++;
		bssid[out++] = (hi<<4) | lo;
		if (bss_str[in] == ':') {
			in++;
		}
	}
	
	if (in != bssid_str_len) {
		return -EINVAL;
	}

	return 0;
}

