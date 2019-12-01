#include <string.h>
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

	return bss;
}

void bss_free(struct BSS** pbss)
{
	struct BSS* bss;

	PTR_ASSIGN(bss, *pbss);
	XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);

	ie_list_release(&bss->ie_list);

	memset(bss, POISON, sizeof(struct BSS));
	PTR_FREE(bss);
}

void bss_free_list(struct list_head* bss_list)
{
	while( !list_empty(bss_list)) {
		struct BSS* bss = list_first_entry(bss_list, typeof(*bss), node);
		if (!bss) {
			break;
		}

		XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);
		list_del(&bss->node);
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

	if ((attr=attr_list[NL80211_ATTR_CHANNEL_WIDTH])) {
		bss->channel_width = nla_get_u32(attr);
	}

	int err = parse_nla_bss(attr_list[NL80211_ATTR_BSS], bss);
	if (err != 0) {
		goto fail;
	}

	PTR_ASSIGN(*pbss, bss);
	DBG("%s success\n", __func__);
	return 0;

fail:
	if (bss) {
		bss_free(&bss);
	}
	return err;
}

