#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <unicode/utypes.h>
#include <unicode/ustdio.h>

#define HAVE_MODULE_LOGLEVEL 1

#include "core.h"
#include "ie.h"

const uint8_t ms_oui[3] = { 0x00, 0x50, 0xf2 };
const uint8_t ieee80211_oui[3] = { 0x00, 0x0f, 0xac };
const uint8_t wfa_oui[3] = { 0x50, 0x6f, 0x9a };

#define IE_LIST_DEFAULT_MAX 32


#define INIT_SPEC(ie, spec_ie, free_fn)\
	spec_ie->cookie = IE_SPECIFIC_COOKIE;\
	ie->specific = spec_ie;\
	spec_ie->base = ie;\
	ie->free = free_fn

static int module_loglevel=LOG_LEVEL_INFO;

static void ie_ssid_free(struct IE* ie)
{
	struct IE_SSID* ie_ssid = (struct IE_SSID*)ie->specific;

	XASSERT(ie_ssid->cookie == IE_SPECIFIC_COOKIE, ie_ssid->cookie);

	PTR_FREE(ie->specific);
}

static int ie_ssid_new(struct IE* ie)
{
	struct IE_SSID* ie_ssid;

	ie_ssid = calloc(1,sizeof(struct IE_SSID));
	if (!ie_ssid) {
		return -ENOMEM;
	}

	INIT_SPEC(ie, ie_ssid, ie_ssid_free);

	UErrorCode status = U_ZERO_ERROR;
	u_strFromUTF8( ie_ssid->ssid, sizeof(ie_ssid->ssid), &ie_ssid->ssid_len, ie->buf, ie->len, &status);
	if ( !U_SUCCESS(status)) {
		PTR_FREE(ie_ssid);
		ERR("%s unicode parse fail status=%d\n", __func__, status);
		return -EINVAL;
	}

	if (ie_ssid->ssid_len > SSID_MAX_LEN) {
		WARN("ssid too long len=%d\n", ie_ssid->ssid_len);
	}

	return 0;
}

static void ie_extended_capa_free(struct IE* ie)
{
	(void)ie;
}

static int ie_extended_capa_new(struct IE* ie)
{
	(void)ie;
	return 0;
}


static void ie_vendor_free(struct IE* ie)
{
	struct IE_Vendor* vie = (struct IE_Vendor*)ie->specific;

	DBG("%s %p id=%d\n", __func__, (void *)ie, ie->id);

	XASSERT(ie->id==IE_VENDOR, ie->id);

	memset(vie, POISON, sizeof(struct IE_Vendor));
	PTR_FREE(ie->specific);
}

static int ie_vendor_new(struct IE* ie)
{
	struct IE_Vendor* newie;

	DBG("%s id=%d len=%zu oui=0x%02x%02x%02x\n", __func__, 
		ie->id, ie->len, ie->buf[0], ie->buf[1], ie->buf[2]);

	newie = calloc(1, sizeof(struct IE_Vendor));
	if (!newie) { 
		WARN("%s calloc failed\n", __func__);
		return -ENOMEM;
	}

	newie->cookie = IE_SPECIFIC_COOKIE;
	// point to self
	ie->specific = (void *)newie;
	newie->base = ie;
	ie->free = ie_vendor_free;

	memcpy(newie->oui, ie->buf, 3);

	return 0;
}


typedef int (*specific_ie_new)(struct IE *);

static const specific_ie_new constructors[256] = {
	[IE_SSID] = ie_ssid_new,
	[IE_EXTENDED_CAPABILITIES] = ie_extended_capa_new,
	[IE_VENDOR] = ie_vendor_new,
};

struct IE* ie_new(uint8_t id, uint8_t len, const uint8_t* buf)
{
	DBG("%s id=%d len=%d\n", __func__, id, len);

	struct IE* ie = (struct IE*)calloc(1, sizeof(struct IE));
	if (!ie) {
		return NULL;
	} 

	ie->cookie = IE_COOKIE;
	ie->id = id;
	ie->len = len;
	// keep in mind `buf` doesn't contain the id+len octets
	ie->buf = malloc(len);
	if (!ie->buf) {
		PTR_FREE(ie);
		return NULL;
	}
	memcpy(ie->buf, buf, ie->len);

	if (constructors[id]) {
		int err = constructors[id](ie);
		if (err) {
			ERR("%s id=%d failed err=%d\n", __func__, id, err);
			ie_delete(&ie);
			return NULL;
		}
	}
	else {
		WARN("%s unparsed IE=%d\n", __func__, id);
	}

	return ie;
}

void ie_delete(struct IE** pie)
{
	struct IE* ie;

	DBG("%s %p id=%d\n", __func__, (void *)(*pie), (*pie)->id);

	PTR_ASSIGN(ie, *pie);

	XASSERT( ie->cookie == IE_COOKIE, ie->cookie);

	// free my memory
	if (ie->buf) {
		memset(ie->buf, POISON, ie->len);
		PTR_FREE(ie->buf);
	}

	// now let the descendent free its memory
	if (ie->free) {
		ie->free(ie);
	}

	memset(ie, POISON, sizeof(struct IE));
	PTR_FREE(ie);
}

int decode_ie_buf( const uint8_t* ptr, size_t len, struct IE_List* ie_list)
{
	const uint8_t* endptr = ptr + len;
	uint8_t id, ielen;
	int err;

	// assuming caller called ie_list_init()
	XASSERT(ie_list->cookie == IE_LIST_COOKIE, ie_list->cookie);
	XASSERT(ie_list->ieptrlist != NULL, 0);
	XASSERT(ie_list->max != 0, 0);

	while (ptr < endptr) {
		id = *ptr++;
		ielen =  *ptr++;
		DBG("%s id=%u len=%u\n", __func__, id, ielen);

		struct IE* ie = ie_new(id, ielen, ptr);
		if (!ie) {
			ERR("%s failed to create ie\n", __func__);
			return -ENOMEM;
		}
		err = ie_list_move_back(ie_list, &ie);
		if (err) {
			ie_delete(&ie);
			return err;
		}
		// ie will be NULL at this point

		ptr += ielen;
	}

	return 0;
}

int ie_list_init(struct IE_List* list)
{
	memset(list, 0, sizeof(struct IE_List));
	list->cookie = IE_LIST_COOKIE;
	list->max = IE_LIST_DEFAULT_MAX;
	list->ieptrlist = (struct IE**)calloc(list->max, sizeof(struct IE*));
	if (!list->ieptrlist) {
		return -ENOMEM;
	}
	list->count = 0;

	return 0;
}

void ie_list_release(struct IE_List* list)
{
	DBG("%s\n", __func__);
	XASSERT(list->cookie == IE_LIST_COOKIE, list->cookie);
	XASSERT(list->max, 0);

	for (size_t i=0 ; i<list->count ; i++) {
		XASSERT(list->ieptrlist[i] != NULL, 0);
		ie_delete(&list->ieptrlist[i]);
	}

	memset(list->ieptrlist, POISON, sizeof(struct IE*)*list->max);
	PTR_FREE(list->ieptrlist);
	memset(list, POISON, sizeof(struct IE_List));
}

int ie_list_move_back(struct IE_List* list, struct IE** pie)
{
	if (list->count+1 > list->max) {
		struct IE** new_ieptrlist;
		INFO("%s resize list=%p from %zu to %zu\n", __func__,
				(void*)list, list->max, list->max*2);
		new_ieptrlist = reallocarray(list->ieptrlist, list->max*2, sizeof(struct IE*));
		if( !new_ieptrlist ) {
			WARN("%s realloc of %zu failed\n", __func__, sizeof(struct IE*)*(list->max*2));
			return -ENOMEM;
		}
		memset(new_ieptrlist+list->max, 0, list->max);
		list->ieptrlist = new_ieptrlist;
		list->max *= 2;
	}

	// dis mine now
	PTR_ASSIGN(list->ieptrlist[list->count++], *pie);

	return 0;
}

void ie_list_peek(const char *label, struct IE_List* list)
{
	for (size_t i=0 ; i<list->count ; i++) {
		struct IE* ie = list->ieptrlist[i];
		DBG("%s IE id=%d len=%zu\n", label, ie->id, ie->len);
	}

}

const struct IE* ie_list_find_id(struct IE_List* list, IE_ID id)
{
	// search for the first instance of an id; note this will not work when
	// there are duplicates such as vendor id
	for (size_t i=0 ; i<list->count ; i++) {
		if (list->ieptrlist[i]->id == id) {
			return (const struct IE*)list->ieptrlist[i];
		}
	}

	return (const struct IE*)NULL;
}

