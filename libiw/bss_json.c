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

struct json_key_value
{
	char* key;
	json_t* value;
};

static int transaction_add(json_t* jobj, struct json_key_value* kvp_list, size_t len)
{
	// fn to add all or nothing key/value pairs to an object .
	// If any add fails, then all the same keys are removed.

	size_t i;
	for (i=0 ; i<len ; i++) {
		// check for caller's memory fail
		if (!kvp_list[i].value) {
			break;
		}

		if (json_object_set_new(jobj, kvp_list[i].key, kvp_list[i].value)) {
			break;
		}
	}
	if (i != len) {
		// failure so clean up what we just added
		while(i>0) {
			i--;
			json_object_del(jobj, kvp_list[i].key);
		};
		return -ENOMEM;
	}
	return 0;
}

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

	jbss = json_pack_ex(&jerror, 0, "{sssosisfss*ss*sI}", 
				"bssid", bss->bssid_str,  // object key
				"ssid", jssid, 
				"freq", bss->frequency, 
				"dbm", bss->signal_strength_mbm / 100.0,
				"chwidth", channel_width_ptr,
				"mode", mode_ptr,
				"last_seen", (json_int_t)bss->last_seen
				);
	if (!jbss) {
		ERR("%s bss json encode failed msg=\"%s\"\n", __func__, jerror.text);
		json_decref(jssid);
		return -ENOMEM;
	}

	PTR_ASSIGN(*p_jbss, jbss);

	return 0;
}

static int ie_supported_rates_to_json(const struct IE* ie, json_t* jie)
{
	int err;
	const struct IE_Supported_Rates* sie = IE_CAST(ie, const struct IE_Supported_Rates);

	if (sie->count == 0) {
		return 0;
	}

	// "supported_rates" : [
	// 		{ "rate": NNN,
	// 		  "basic": boolean },
	// 		...
	// 		]

	json_t* jrate_list = json_array();
	if (!jrate_list) {
		return -ENOMEM;
	}

	for( size_t i=0 ; i < sie->count ; i++ ) {
		json_error_t jerror;

		json_t* jrate = json_pack_ex(&jerror, 0, "{sfsb}", 
							"rate", sie->rate[i], 
							"basic", (int)sie->basic[i]);
		if (!jrate) {
			ERR("%s bss json encode failed msg=\"%s\"\n", __func__, jerror.text);
			json_decref(jrate_list);
			return -ENOMEM;
		}

		err = json_array_append_new(jrate_list, jrate);
		if (err) {
			json_decref(jrate);
			json_decref(jrate_list);
			return -ENOMEM;
		}
	}

	err = json_object_set_new(jie, "supported_rates", jrate_list);
	if (err) {
		json_decref(jrate_list);
		return -ENOMEM;
	}

	return 0;
}

static int ie_tim_to_json(const struct IE* ie, json_t* jie)
{
	int err;
//	char s[128];
	const struct IE_TIM* sie = IE_CAST(ie, const struct IE_TIM);
	char bitmap_hex[512];

	int ret = str_hexdump(ie->buf, ie->len-3, bitmap_hex, sizeof(bitmap_hex));
	XASSERT(ret > 0, ret);

	struct json_key_value kvp_list[] = {
		{ "count", json_integer(sie->dtim_count) },
		{ "period", json_integer(sie->dtim_period) },
		{ "control", json_integer(sie->control) },
		{ "bitmap", json_string(bitmap_hex) }
	};

	err = transaction_add( jie, kvp_list, 4);
	if (err) {
		return err;
	}

	return 0;
}

static int jarray_append_if_true(bool cond, json_t* jlist, const char* name)
{
	// add a true/false value to an array if the condition is true
	// (created to build a string list of only enabled capabilities)
	if (cond) {
		json_t* jval;
		jval = json_string_nocheck(name);
		if (!jval) {
			return -ENOMEM;
		}
		if (json_array_append_new(jlist, jval) < 0) {
			json_decref(jval);
			return -ENOMEM;
		}
	}
	return 0;
}

static int ie_ht_capa_to_json(const struct IE* ie, json_t* jie)
{
	const struct IE_HT_Capabilities* sie = IE_CAST(ie, const struct IE_HT_Capabilities);

	json_t* jlist = json_array();
	if (!jlist) {
		return -ENOMEM;
	}

	if (jarray_append_if_true(sie->LDPC_coding_capa, jlist, "RX LDPC") < 0) goto fail;
	if (jarray_append_if_true(sie->supported_channel_width, jlist, "HT20/HT40") < 0) goto fail;
	if (jarray_append_if_true(!sie->supported_channel_width, jlist, "HT20") < 0) goto fail;

	if (jarray_append_if_true(sie->greenfield, jlist, "RX Greenfield") < 0) goto fail;
	if (jarray_append_if_true(sie->short_gi_20Mhz, jlist, "RX HT20 SGI") < 0) goto fail;
	if (jarray_append_if_true(sie->short_gi_40Mhz, jlist, "RX HT40 SGI") < 0) goto fail;
	if (jarray_append_if_true(sie->tx_stbc, jlist, "TX STBC") < 0) goto fail;

	if (jarray_append_if_true(sie->rx_stbc == 0, jlist, "No RX STBC") < 0) goto fail;
	if (jarray_append_if_true(sie->rx_stbc == 1, jlist, "RX STBC 1-stream") < 0) goto fail;
	if (jarray_append_if_true(sie->rx_stbc == 2, jlist, "RX STBC 2-streams") < 0) goto fail;
	if (jarray_append_if_true(sie->rx_stbc == 3, jlist, "RX STBC 3-streams") < 0) goto fail;

	if (jarray_append_if_true(sie->delayed_block_ack, jlist, "HT Delayed Block Ack") < 0) goto fail;

	if (jarray_append_if_true(!sie->max_amsdu_len, jlist, "Max AMSDU length: 3839 bytes") < 0) goto fail;
	if (jarray_append_if_true(sie->max_amsdu_len, jlist, "Max AMSDU length: 7935 bytes") < 0) goto fail;

	if (json_object_set_new(jie, "capabilities", jlist) < 0) goto fail;


	return 0;
fail:
	if (jlist) {
		json_decref(jlist);
	}
	return -ENOMEM;
}

static int ie_rsn_to_json(const struct IE* ie, json_t* jie)
{
	int err, ret;
	char s[128];

	const struct IE_RSN* sie = IE_CAST(ie, const struct IE_RSN);

	json_t* jversion = json_integer(sie->version);
	json_t* jpairwise_cipher_list = json_array();
	json_t* jakm_suite_list = json_array();
	json_t* jcapa_list = json_array();

	if (!jversion || !jpairwise_cipher_list || !jakm_suite_list || !jcapa_list) {
		err = -ENOMEM;
		goto fail;
	}

	// Group Cipher (single string)
	ret = cipher_suite_to_str(sie->group_data, s, sizeof(s));
	XASSERT((size_t)ret<sizeof(s), ret);
	json_t* jgroup_cipher = json_string(s);
	if (!jgroup_cipher) {
		err = -ENOMEM;
		goto fail;
	}

	// Pairwise Ciphers (Array) 
	for (size_t i=0 ; i<sie->pairwise_cipher_count ; i++) {
		ret = cipher_suite_to_str(&sie->pairwise[i], s, sizeof(s));
		XASSERT((size_t)ret<sizeof(s), ret);

		json_t* jstr = json_string(s);
		if (!jstr) {
			err = -ENOMEM;
			goto fail;
		}

		err = json_array_append_new(jpairwise_cipher_list, jstr);
		if (err) {
			json_decref(jstr);
			err = -ENOMEM;
			goto fail;
		}
	}

	// Authentication Suites (Array)
	for (size_t i=0 ; i<sie->akm_suite_count; i++) {
		XASSERT((sie->akm_suite) != NULL, i);
		ret = auth_to_str(&sie->akm_suite[i], s, sizeof(s));
		XASSERT((size_t)ret<sizeof(s), ret);

		json_t* jstr = json_string(s);
		if (!jstr ) {
			err = -ENOMEM;
			goto fail;
		}

		err = json_array_append_new(jakm_suite_list, jstr);
		if (err) {
			json_decref(jstr);
			err = -ENOMEM;
			goto fail;
		}
	}

	// Capabilities
	char capabilities[16][64];
	ret = rsn_capabilities_to_str_list(sie, capabilities, 64, 16);
	XASSERT(ret > 0, ret);

	for (int i=0 ; i<ret ; i++ ) {
		json_t* jstr = json_string(capabilities[i]);
		if (!jstr) {
			err = -ENOMEM;
			goto fail;
		}

		err = json_array_append_new(jcapa_list, jstr);
		if (err) {
			json_decref(jstr);
			err = -ENOMEM;
			goto fail;
		}
	}

	// store all our calculated stuff in the object
	struct json_key_value kvp_list[] = {
		{ "version", jversion },
		{ "group", jgroup_cipher},
		{ "pairwise", jpairwise_cipher_list},
		{ "auth", jakm_suite_list},
		{ "capabilities", jcapa_list},
	};

	err = transaction_add( jie, kvp_list, 5);
	if (err) {
		goto fail;
	}

	return 0;
fail:
	if (jversion) {
		json_decref(jversion);
	}
	if (jpairwise_cipher_list) {
		json_decref(jpairwise_cipher_list);
	}
	if (jgroup_cipher) {
		json_decref(jgroup_cipher);
	}
	if (jakm_suite_list) {
		json_decref(jakm_suite_list);
	}
	if (jcapa_list) {
		json_decref(jcapa_list);
	}
	return err;
}

static int ie_body_to_json(const struct IE* ie, json_t* jie)
{
	struct {
		IE_ID id;
		int (*fn)(const struct IE* ie, json_t* jie);
	} encode [] = {
		{ IE_SUPPORTED_RATES, ie_supported_rates_to_json },
		{ IE_TIM, ie_tim_to_json },
		{ IE_HT_CAPABILITIES, ie_ht_capa_to_json },
		{ IE_RSN, ie_rsn_to_json },
	};

	for (size_t i=0 ; i<ARRAY_SIZE(encode) ; i++ ) {
		if (encode[i].id == ie->id) {
			return encode[i].fn(ie, jie);
		}
	}
	return -ENOENT;

#if 0
	switch (ie->id) {
		case IE_SUPPORTED_RATES:
			ie_supported_rates_to_json(ie, jie);
			break;

		case IE_TIM:
			ie_tim_to_json(ie, jie);
			break;

		case IE_HT_CAPABILITIES:
//			ie_ht_capa_to_json(ie, jie);
			break;

		case IE_RSN:
			ie_rsn_to_json(ie, jie);
			break;

//		case IE_EXTENDED_CAPABILITIES:
//			ie_extended_capabilities_to_json(ie, jie);
//			break;

		default:
			break;
	}

	return 0;
#endif
}

static int ie_to_json(const struct IE* ie, json_t** p_jie, unsigned int flags)
{
	// each byte becomes two characters plus one for null
	char* hexdump = NULL;
	size_t dumplen = 0;

	// ie->len can be zero for a hidden SSID
	if (ie->len) {
		dumplen = ie->len*2+1;
		hexdump = (char*)malloc(dumplen);
		if (!hexdump) {
			return -ENOMEM;
		}

		int ret = str_hexdump(ie->buf, ie->len, hexdump, dumplen);
		XASSERT(ret > 0, ret);
	}

	json_t* jie;
	json_error_t jerror;
	jie = json_pack_ex(&jerror, 0, "{sisiss?}",
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
		ERR("%s json ie=%d len=%zu encode failed msg=\"%s\"\n", 
				__func__, ie->id, ie->len, jerror.text);
		return -ENOMEM;
	}

	if (!(flags & BSS_JSON_SHORT_IE_DECODE)) {
		ie_body_to_json(ie, jie);
	}

	PTR_ASSIGN(*p_jie, jie);
	return 0;
}

int bss_to_json(const struct BSS* bss, json_t** p_jbss, unsigned int flags)
{
	*p_jbss = NULL;

	json_t* jbss;
	int err = bss_to_json_summary(bss, &jbss);
	if (err) {
		return err;
	}

	// build the array of encoded IEs
	json_t* jielist = json_array();
	if (!jielist) {
		json_decref(jbss);
		return -ENOMEM;
	}

	const struct IE* ie;
	ie_list_for_each_entry(ie, bss->ie_list) {
		json_t* jie;
		err = ie_to_json(ie, &jie, flags);
		if (err) {
			// skip this and continue
			WARN("%s bss=%s ie=%d encode failed (continuing)\n", 
					__func__, bss->bssid_str, ie->id);
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

static int _bss_list_to_json(struct dl_list* bss_list, json_t** p_jarray, bool summary, unsigned int flags) 
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
		if (summary) {
			err = bss_to_json_summary(bss, &jbss);
		} 
		else {
			// all the things!
			err = bss_to_json(bss, &jbss, flags);
		}
		if (err) {
			ERR("%s failed get bss json err=%d", __func__, err);
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

int bss_list_to_json_summary(struct dl_list* bss_list, json_t** p_jarray)
{
	return _bss_list_to_json(bss_list, p_jarray, true, 0);
}

int bss_list_to_json(struct dl_list* bss_list, json_t** p_jarray, unsigned int flags)
{
	return _bss_list_to_json(bss_list, p_jarray, false, flags);
}

