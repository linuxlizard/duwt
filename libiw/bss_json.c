/*
 * libiw/bss_json.c   encode a BSS as JSON using jansson
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <string.h>

// https://jansson.readthedocs.io/en/2.12/
#include <jansson.h>

#include "core.h"
#include "iw.h"
#include "ie.h"
#include "list.h"
#include "str.h"
#include "ssid.h"
#include "bss.h"
#include "bss_json.h"

int bss_to_json_summary(const struct BSS* bss, json_t** p_jbss)
{
	json_t* jbss, *jssid;
	json_error_t jerror;
	char escaped[128];
	int retcode=0;

	// ssid standalone so can add the "<hidden>" 
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_SSID);
	if (ie) {
		const struct IE_SSID* sie = IE_CAST(ie, struct IE_SSID);
		jssid = json_stringn(sie->ssid, sie->ssid_len);
		if (!jssid) {
			// invalid UTF8 string
			memset(escaped,0,sizeof(escaped));
			retcode = str_escape(sie->ssid, sie->ssid_len, escaped, sizeof(escaped)-1);
			if (retcode < 0) {
				// give up
				return -EINVAL;
			}
			jssid = json_string(escaped);
		}
	}
	else {
		jssid = json_string(HIDDEN_SSID);
	}
	if (!jssid) {
		return -ENOMEM;
	}

	// channel width and mode are optional so extra work required:
	// if the ptr is NULL, json_pack_ex() won't include in the object
	char channel_width[32], *channel_width_ptr = NULL;
	char mode[32], *mode_ptr = NULL;

	retcode = bss_get_chan_width_str(bss, channel_width, sizeof(channel_width));
	if (retcode > 0) {
		channel_width_ptr = channel_width;
	}

	retcode = bss_get_mode_str(bss, mode, sizeof(mode));
	if(retcode > 0) {
		mode_ptr = mode;
	}

	// "the reference to the value passed to o is stolen by the container."
	// so no need to decref jssid

	jbss = json_pack_ex(&jerror, 0, "{sssosisfss*ss*}", 
				"bssid", bss->bssid_str,  // object key
				"ssid", jssid, 
				"freq", bss->frequency, 
				"dbm", bss->signal_strength_mbm / 100.0,
				"chwidth", channel_width_ptr,
				"mode", mode_ptr);
	if (!jbss) {
		ERR("%s bss json encode failed msg=%s\n", __func__, jerror.text);
		json_decref(jssid);
		return -ENOMEM;
	}

	PTR_ASSIGN(*p_jbss, jbss);

	return 0;
}

static int ie_to_json(const struct IE* ie, json_t** p_jie)
{
	// each byte becomes two characters plus one for null
	char* hexdump = NULL;
	size_t dumplen = 0;

	// ie->len can be zero for a hidden SSID
	if (ie->len) {
		dumplen = ie->len*2+1;
		char* hexdump = (char*)malloc(dumplen);
		if (!hexdump) {
			return -ENOMEM;
		}

		int ret = str_hexdump(ie->buf, ie->len, hexdump, dumplen);
		XASSERT(ret > 0, ret);
	}

	json_t* jie;
	json_error_t jerror;
	jie = json_pack_ex(&jerror, 0, "{sisiss}",
			"id", ie->id, 
			"len", ie->len,
			"bytes", hexdump
			);

	// appears jansson makes a copy
	if (hexdump) {
		POISON(hexdump, dumplen);
		PTR_FREE(hexdump);
	}

	if (!jie) {
		ERR("%s json ie=%d len=%zu encode failed msg=%s\n", 
				__func__, ie->id, ie->len, jerror.text);
		return -ENOMEM;
	}

	PTR_ASSIGN(*p_jie, jie);
	return 0;
}

int bss_to_json(const struct BSS* bss, json_t** p_jbss)
{
	json_t* jbss;
	int err = bss_to_json_summary(bss, &jbss);
	if (err) {
		return err;
	}

	// build the array of encoded IEs
	json_t* jielist = json_array();

	const struct IE* ie;
	ie_list_for_each_entry(ie, bss->ie_list) {
		json_t* jie;
		err = ie_to_json(ie, &jie);
		if (err) {
			// skip this and continue
			WARN("%s ie=%d encode failed (continuing)\n", __func__, ie->id);
			continue;
		}

		err = json_array_append_new(jielist, jie);
		if (err) {
			// memory failure so we're kinda hosed
			json_decref(jie);
			err = -ENOMEM;
			goto fail;
		}

	}

	// ie list now belongs to the jbss
	err = json_object_set_new(jbss, "IE", jielist);
	if (err) {
		// memory failure so we're kinda hosed
		err = -ENOMEM;
		goto fail;
	}

	PTR_ASSIGN(*p_jbss, jbss);

	return 0;

fail:
	if (jielist) {
		json_decref(jielist);
	}

	return err;
}

int bss_list_to_json(struct dl_list* bss_list, json_t** p_jarray)
{
	int err = 0;
	json_t* jarray = NULL; // array of bss
	json_t* jbss = NULL; // object containing a single bss; keys "ssid" and "bssid"

	jarray = json_array();
	if (!jarray) {
		err = -ENOMEM;
		goto fail;
	}

	struct BSS* bss;
	dl_list_for_each(bss, bss_list, struct BSS, node) {
		err = bss_to_json_summary(bss, &jbss);
		if (err) {
			ERR("%s failed at bss_to_json_summary", __func__);
			goto fail;
		}

		err = json_array_append_new(jarray, jbss);
		if (err) {
			ERR("%s failed at json_array_append_new", __func__);
			goto fail;
		}
	}

	PTR_ASSIGN(*p_jarray, jarray);
	return 0;

fail:
	if (jarray) {
		json_decref(jarray);
	}
	if (jbss) {
		json_decref(jbss);
	}

	return err;
}

