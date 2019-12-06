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

	// capture the BSSID, create a new BSS struct for this network
	uint8_t *ptr = nla_data(bss_attr[NL80211_BSS_BSSID]);
	memcpy(bss->bssid, ptr, ETH_ALEN);
	mac_addr_n2a(bss->bssid_str, bss->bssid);
	INFO("%s found bssid=%s\n", __func__, bss->bssid_str);

	/* 
	 * From this point on, use 'goto fail' on error so the BSS will be freed
	 * before return
	 */

	if ((attr = bss_attr[NL80211_BSS_CAPABILITY])) {
		bss->capability = nla_get_u16(attr);
	}

	if ((attr = bss_attr[NL80211_BSS_INFORMATION_ELEMENTS])) {
		// attr is actually an array here (nested nla)
		err = parse_nla_ies(attr, &bss->ie_list);
		if (err) {
			goto fail;
		}
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
	}

	if ((attr = bss_attr[NL80211_BSS_LAST_SEEN_BOOTTIME])) {
	}

	if ((attr = bss_attr[NL80211_BSS_FREQUENCY])) {
		bss->frequency = nla_get_u32(attr);
	}

	ie_list_peek(__func__, &bss->ie_list);

	DBG("%s success\n", __func__);
	return 0;

fail:
	return err;
}

