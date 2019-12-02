#include "core.h"
#include "bss.h"

int main(void)
{
	struct BSS* bss;

	macaddr me = { 0x00, 0x40, 0x68, 0x12, 0x34, 0x56 };
	LIST_HEAD(bss_list);

	bss = bss_new(me);
	list_add(&bss->node, &bss_list);
	size_t off = offsetof(struct BSS, node);
	DBG("off=%zu\n", off);
	DBG("%p %p %p %p\n", (void *)bss, (void *)&bss->node, (void *)bss_list.next, (void *)bss_list.prev);
	bss = (struct BSS*)((unsigned char*)bss_list.next - offsetof(struct BSS, node));
	DBG("bss=%p\n", (void *)bss);
	XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);

	me[5]++;
	bss = bss_new(me);
	list_add(&bss->node, &bss_list);
	me[5]++;
	bss = bss_new(me);
	list_add(&bss->node, &bss_list);
	me[5]++;
	bss = bss_new(me);
	list_add(&bss->node, &bss_list);
	me[5]++;
	bss = bss_new(me);
	list_add(&bss->node, &bss_list);

	struct list_head* tmp;
	char mac_str[64];
	list_for_each(tmp, &bss_list) {
		bss = list_entry(tmp, struct BSS, node);
		XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);
		mac_addr_n2a(mac_str, bss->bssid);
		INFO("%s\n", mac_str);
	}

	bss = list_first_entry(&bss_list, typeof(*bss), node);
	DBG("%d %p\n", __LINE__, (void *)bss);
	XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);

	bss = list_next_entry(bss, node);
	DBG("%d %p\n", __LINE__, (void *)bss);
	XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);

	bss = list_last_entry(&bss_list, typeof(*bss), node);
	DBG("%d %p\n", __LINE__, (void *)bss);
	XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);

	list_for_each_entry(bss, &bss_list, node) {
		DBG("%p\n", (void *)bss);
		XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);
		mac_addr_n2a(mac_str, bss->bssid);
		INFO("%s\n", mac_str);
	}

	while( !list_empty(&bss_list)) {
		bss = list_first_entry(&bss_list, typeof(*bss), node);
		if (!bss) {
			break;
		}

		XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);
		list_del(&bss->node);
		XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);
		bss_free(&bss);
	}

	return EXIT_SUCCESS;
}

