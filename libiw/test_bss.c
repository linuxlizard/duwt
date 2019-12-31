#include <string.h>
#include "core.h"
#include "bss.h"

int main(void)
{
	struct BSS* bss;

	macaddr me = { 0x00, 0x40, 0x68, 0x12, 0x34, 0x56 };
	DEFINE_DL_LIST(bss_list);

	bss = bss_new();
	memcpy(bss->bssid, me, sizeof(me));
	dl_list_add(&bss_list, &bss->node);
	size_t off = offsetof(struct BSS, node);
	DBG("off=%zu\n", off);
	DBG("%p %p %p %p\n", (void *)bss, (void *)&bss->node, (void *)bss_list.next, (void *)bss_list.prev);
	bss = (struct BSS*)((unsigned char*)bss_list.next - offsetof(struct BSS, node));
	DBG("bss=%p\n", (void *)bss);
	XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);

	me[5]++;
	bss = bss_new();
	memcpy(bss->bssid, me, sizeof(me));
	dl_list_add(&bss_list, &bss->node);

	me[5]++;
	bss = bss_new();
	memcpy(bss->bssid, me, sizeof(me));
	dl_list_add(&bss_list, &bss->node);

	me[5]++;
	bss = bss_new();
	memcpy(bss->bssid, me, sizeof(me));
	dl_list_add(&bss_list, &bss->node);

	me[5]++;
	bss = bss_new();
	memcpy(bss->bssid, me, sizeof(me));
	dl_list_add(&bss_list, &bss->node);

	char mac_str[64];
	dl_list_for_each(bss, &bss_list, struct BSS, node) {
		XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);
		mac_addr_n2a(mac_str, bss->bssid);
		INFO("%s\n", mac_str);
	}

	bss = dl_list_first(&bss_list, typeof(*bss), node);
	DBG("%d %p\n", __LINE__, (void *)bss);
	XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);

	bss = dl_list_last(&bss_list, typeof(*bss), node);
	DBG("%d %p\n", __LINE__, (void *)bss);
	XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);

	dl_list_for_each(bss, &bss_list, struct BSS, node) {
		DBG("%p\n", (void *)bss);
		XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);
		mac_addr_n2a(mac_str, bss->bssid);
		INFO("%s\n", mac_str);
	}

	while( !dl_list_empty(&bss_list)) {
		bss = dl_list_first(&bss_list, typeof(*bss), node);
		if (!bss) {
			break;
		}

		XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);
		dl_list_del(&bss->node);
		XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);
		bss_free(&bss);
	}

	return EXIT_SUCCESS;
}

