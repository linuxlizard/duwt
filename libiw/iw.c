#include <linux/netlink.h>
#include <linux/nl80211.h>

#include "core.h"
#include "nlnames.h"
#include "iw.h"

// from iw scan.c
static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
	[NL80211_BSS_TSF] = { .type = NLA_U64 },
	[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
	[NL80211_BSS_BSSID] = { .type = NLA_UNSPEC },
	[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
	[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
	[NL80211_BSS_INFORMATION_ELEMENTS] = { .type = NLA_UNSPEC },
	[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
	[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
	[NL80211_BSS_STATUS] = { .type = NLA_U32 },
	[NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
	[NL80211_BSS_BEACON_IES] = { .type = NLA_UNSPEC },
};

void peek_nla_attr( struct nlattr* tb_msg[], size_t count)
{
	for (size_t i=0 ; i<count; i++ ) {
		if (tb_msg[i]) {
			const char *name = to_string_nl80211_attrs(i);
			DBG("%s i=%zu %s type=%d len=%u\n", __func__, 
				i, name, nla_type(tb_msg[i]), nla_len(tb_msg[i]));
		}
	}
}

static void peek_nla_bss(struct nlattr* bss_msg[static NL80211_BSS_MAX], size_t count)
{
	for (size_t i=0 ; i<count; i++ ) {
		if (bss_msg[i]) {
			const char *name = to_string_nl80211_bss(i);
			DBG("%s i=%zu %s type=%d len=%u\n", __func__, 
				i, name, nla_type(bss_msg[i]), nla_len(bss_msg[i]));
		}
	}
}

int parse_nla_ies(struct nlattr* ies, struct IE_List* ie_list)
{
	DBG("%s\n", __func__);
	int err = decode_ie_buf( nla_data(ies), nla_len(ies), ie_list);
	if (err) {
		ERR("%s IE buffer decode failed err=%d\n", __func__, err);
		return err;
	}

	return 0;
}

int parse_nla_bss(struct nlattr* attr_list, struct BSS* bss)
{
	struct nlattr* bss_attr[NL80211_BSS_MAX + 1];
	struct nlattr* attr;
	int err;

	DBG("%s\n", __func__);

	if (nla_parse_nested(bss_attr, NL80211_BSS_MAX,
			     attr_list,
			     bss_policy)) {
		ERR("%s failed to parse nested attributes!\n", __func__);
		return -EINVAL;
	}

	peek_nla_bss(bss_attr, NL80211_BSS_MAX);

	if (!bss_attr[NL80211_BSS_BSSID]) {
		ERR("%s invalid network found; mssing BSSID\n", __func__);
		return -EINVAL;
	}

	// capture the BSSID, fill a new BSS struct for this network
	uint8_t *ptr = nla_data(bss_attr[NL80211_BSS_BSSID]);
	memcpy(bss->bssid, ptr, ETH_ALEN);
	mac_addr_n2a(bss->bssid_str, bss->bssid);
	DBG("%s found bssid=%s\n", __func__, bss->bssid_str);

	if ((attr = bss_attr[NL80211_BSS_CAPABILITY])) {
		bss->capability = nla_get_u16(attr);
	}

	if ((attr = bss_attr[NL80211_BSS_TSF])) {
		bss->tsf = nla_get_u64(attr);
	}

	if ((attr = bss_attr[NL80211_BSS_BEACON_INTERVAL])) {
		bss->beacon_interval = (uint16_t)nla_get_u16(attr);
	}

	if ((attr = bss_attr[NL80211_BSS_SIGNAL_MBM])) {
		bss->signal_strength_mbm = (int32_t)nla_get_u32(attr);
	}

	if ((attr = bss_attr[NL80211_BSS_SIGNAL_UNSPEC])) {
		bss->signal_unspec = (uint8_t)nla_get_u32(attr);
	}

	if ((attr = bss_attr[NL80211_BSS_SEEN_MS_AGO])) {
		// TODO
	}

	if ((attr = bss_attr[NL80211_BSS_LAST_SEEN_BOOTTIME])) {
		// TODO
	}

	if ((attr = bss_attr[NL80211_BSS_FREQUENCY])) {
		bss->frequency = nla_get_u32(attr);

		if (bss->frequency < 3000) {
			bss->band = NL80211_BAND_2GHZ;
		}
		else if (bss->frequency < 5900) {
			bss->band = NL80211_BAND_5GHZ;
		}
	//	else if (bss->frequency < 8000) {
	//		bss->band = NL80211_BAND_6GHZ;
	//	}

	}

	// need IEs before BSS_CHAN_WIDTH because we have to guess chan_width from
	// the IE contents
	if ((attr = bss_attr[NL80211_BSS_INFORMATION_ELEMENTS])) {
		// attr is actually an array here (nested nla)
		err = parse_nla_ies(attr, &bss->ie_list);
		if (err) {
			goto fail;
		}
	}

	if ((attr = bss_attr[NL80211_BSS_CHAN_WIDTH])) {
		// I'm finding this coming back as zero so it's not actually useful.
		bss->chan_width = nla_get_u32(attr);
		// so lets try to gess it based on IE contents
		if (bss->chan_width == 0) {
			bss_guess_chan_width(bss);
		}
	}

	ie_list_peek(__func__, &bss->ie_list);

	DBG("%s success\n", __func__);
	return 0;

fail:
	return err;
}

