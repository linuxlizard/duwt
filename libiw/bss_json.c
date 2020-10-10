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

	// channel width and mode are optional so extra work required
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
		json_decref(jssid);
		XASSERT(0,0);
		return -1;
	}

	PTR_ASSIGN(*p_jbss, jbss);

	return 0;
}

int bss_to_json(const struct BSS* bss, json_t** p_jbss)
{
	json_t* jbss;
	int err = bss_to_json_summary(bss, &jbss);
	if (err) {
		return err;
	}

	PTR_ASSIGN(*p_jbss, jbss);

	return 0;
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

