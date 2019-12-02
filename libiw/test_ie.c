#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "ie.h"

static const uint8_t vendor_ie[] = {
	"\x7f\x08\x04\x00\x00\x00\x00\x00\x00\x40\xdd\x69\x00\x50\xf2\x04"\
	"\x10\x4a\x00\x01\x10\x10\x3a\x00\x01\x00\x10\x08\x00\x02\x31\x08"\
	"\x10\x47\x00\x10\x14\x0b\x30\x65\xe6\x68\x5b\x96\x88\xe8\x7d\x9f"\
	"\x66\x18\x7c\xf3\x10\x54\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00"\
	"\x10\x3c\x00\x01\x03\x10\x02\x00\x02\x00\x00\x10\x09\x00\x02\x00"\
	"\x00\x10\x12\x00\x02\x00\x00\x10\x21\x00\x01\x20\x10\x23\x00\x01"\
	"\x20\x10\x24\x00\x01\x20\x10\x11\x00\x01\x20\x10\x49\x00\x06\x00"\
	"\x37\x2a\x00\x01\x20\xdd\x11\x50\x6f\x9a\x09\x02\x02\x00\x25\x00"\
	"\x06\x05\x00\x58\x58\x04\x51\x06\x72\x00"
};

static void ie_validate(struct IE* ie, uint8_t id, size_t len)
{
	XASSERT( ie->cookie == IE_COOKIE, ie->cookie);
	XASSERT( ie->id == id, ie->id);
	XASSERT( ie->len == len, ie->len);
	if (len) {
		XASSERT( ie->buf, id);
	}
}

static void test_ie_ssid(void)
{
	struct IE* ie;

	ie = ie_new(IE_SSID, 13, (const uint8_t*)"hello, world");
	XASSERT(ie, 0);

	ie_delete(&ie);
}

int main(void)
{
	struct IE_List ie_list;

	int err = ie_list_init(&ie_list);
	XASSERT(err==0, err);
	XASSERT(ie_list.cookie == IE_LIST_COOKIE, ie_list.cookie);
	XASSERT(ie_list.count == 0, ie_list.count);
	XASSERT(ie_list.max > 0, ie_list.max);

	// -1 because I'm using string constant and the compiler is giving me an
	// extra NULL
	err = decode_ie_buf(vendor_ie, sizeof(vendor_ie)-1, &ie_list);
	XASSERT(err==0, err);
	XASSERT(ie_list.count == 4, ie_list.count);

	// verify counts
	size_t i, count=0;
	for (i=0 ; i<ie_list.max ; i++) {
		if (ie_list.ieptrlist[i]) {
			count++;
		}
	}
	XASSERT(count==4, count);

	// verify contents
	struct IE** pie = ie_list.ieptrlist;
	struct IE* ie = *pie;
	ie_validate(ie, IE_EXTENDED_CAPABILITIES, 8);

	pie++; ie = *pie;
	struct IE_Vendor* vie = (struct IE_Vendor*)ie->specific;
	XASSERT(vie->cookie == IE_SPECIFIC_COOKIE, vie->cookie);
	XASSERT(vie->base == ie, 0);
	err = memcmp(vie->oui, ms_oui, 3);
	XASSERT(err==0, err);
	ie_validate(ie, IE_VENDOR, 0x69);

	pie++; ie = *pie;
	vie = (struct IE_Vendor*)ie->specific;
	XASSERT(vie->cookie == IE_SPECIFIC_COOKIE, vie->cookie);
	XASSERT(vie->base == ie, 0);
	ie_validate(ie, IE_VENDOR, 0x11);

	pie++; ie = *pie;
	ie_validate(ie, IE_MESH_ID, 0);

	// search
	const struct IE* cie = ie_list_find_id(&ie_list, IE_EXTENDED_CAPABILITIES);
	XASSERT(cie!=NULL, 0);

	// verify list can grow
	for (i=0 ; i<1024 ; i++) {
		ie = ie_new(vendor_ie[10], vendor_ie[11], (const uint8_t*)&vendor_ie[12]);
		XASSERT(ie, 0);
		XASSERT(ie->buf, 0);
		ie_validate(ie, IE_VENDOR, vendor_ie[11]);
		err = memcmp(ie->buf, (const void *)&vendor_ie[12], ie->len);
		XASSERT(err==0, err);

		err = ie_list_move_back(&ie_list, &ie);
		XASSERT(err==0, err);
		XASSERT(ie==NULL, 0);
	}

	ie_list_release(&ie_list);

	test_ie_ssid();

	return EXIT_SUCCESS;
}

