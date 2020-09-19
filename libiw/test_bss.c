/*
 * libiw/test_bss_convert.c   test bss.[ch] functions
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <string.h>
#include "core.h"
#include "bss.h"

void test_bss_convert(void)
{
	const char* bssid_str1 = "00:30:44:2c:59:28";
	const char* bssid_str2 = "0030442c5928";
	const char* bssid_str3 = "012345ABCDEF";
	const char* bssid_str4 = "012345abcdef";

	macaddr bssid;

	int ret = bss_str_to_bss(bssid_str1, strlen(bssid_str1), bssid);
	XASSERT(ret==0, ret);
	XASSERT(bssid[0] == 0x00, bssid[0]);
	XASSERT(bssid[1] == 0x30, bssid[1]);
	XASSERT(bssid[2] == 0x44, bssid[2]);
	XASSERT(bssid[3] == 0x2c, bssid[3]);
	XASSERT(bssid[4] == 0x59, bssid[4]);
	XASSERT(bssid[5] == 0x28, bssid[5]);

	ret = bss_str_to_bss(bssid_str2, strlen(bssid_str2), bssid);
	XASSERT(ret==0, ret);
	XASSERT(bssid[0] == 0x00, bssid[0]);
	XASSERT(bssid[1] == 0x30, bssid[1]);
	XASSERT(bssid[2] == 0x44, bssid[2]);
	XASSERT(bssid[3] == 0x2c, bssid[3]);
	XASSERT(bssid[4] == 0x59, bssid[4]);
	XASSERT(bssid[5] == 0x28, bssid[5]);

	ret = bss_str_to_bss("dave", 4, bssid);
	XASSERT(ret==-EINVAL,ret);

	ret = bss_str_to_bss("dave", 8, bssid);
	XASSERT(ret==-EINVAL,ret);

	ret = bss_str_to_bss(bssid_str2, 32, bssid);
	XASSERT(ret==-EINVAL,ret);

	ret = bss_str_to_bss(bssid_str3, strlen(bssid_str3), bssid);
	XASSERT(ret==0, ret);
	XASSERT(bssid[0] == 0x01, bssid[0]);
	XASSERT(bssid[1] == 0x23, bssid[1]);
	XASSERT(bssid[2] == 0x45, bssid[2]);
	XASSERT(bssid[3] == 0xab, bssid[3]);
	XASSERT(bssid[4] == 0xcd, bssid[4]);

	ret = bss_str_to_bss(bssid_str4, strlen(bssid_str4), bssid);
	XASSERT(ret==0, ret);
	XASSERT(bssid[0] == 0x01, bssid[0]);
	XASSERT(bssid[1] == 0x23, bssid[1]);
	XASSERT(bssid[2] == 0x45, bssid[2]);
	XASSERT(bssid[3] == 0xab, bssid[3]);
	XASSERT(bssid[4] == 0xcd, bssid[4]);
	XASSERT(bssid[5] == 0xef, bssid[5]);
	XASSERT(bssid[5] == 0xef, bssid[5]);
}

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

	test_bss_convert();

	return EXIT_SUCCESS;
}

