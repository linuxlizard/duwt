#include <string.h>

// https://jansson.readthedocs.io/en/2.12/
#include <jansson.h>

#include "core.h"
#include "iw.h"
#include "ie.h"
#include "list.h"
#include "bss.h"
#include "bss_json.h"

#define ERRCHECK(msg)\
		if (err) {\
			ERR("%s %s failed\n", __func__, msg);\
			err = -EINVAL;\
			goto fail;\
		}

int bss_to_json_summary(const struct BSS* bss, json_t** p_jbss)
{
	int err = 0;
	json_t* jbss = NULL; // object containing a single bss; keys: ssid bssid dBm freq
	json_t* jssid = NULL;
	json_t* jbssid = NULL;
	json_t* jfreq = NULL;
	json_t* jdbm = NULL;
	json_t* jwidth = NULL; // channel width
	json_t* jmode = NULL; // b/g/n vs a/n/ac etc.

	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_SSID);
	if (ie) {
		const struct IE_SSID* sie = IE_CAST(ie, struct IE_SSID);
		jssid = json_stringn(sie->ssid, sie->ssid_len);
	}
	else {
		jssid = json_string("<hidden>");
	}

	jbssid = json_string(bss->bssid_str);
	if (!jbssid) {
		ERR("%s bssid failed\n", __func__);
		err = -ENOMEM;
		goto fail;
	}

	jfreq = json_integer(bss->frequency);
	if (!jfreq) {
		ERR("%s freq failed\n", __func__);
		err = -ENOMEM;
		goto fail;
	}

	jdbm = json_real(bss->signal_strength_mbm / 100.0);
	if (!jdbm) {
		ERR("%s dbm failed\n", __func__);
		err = -ENOMEM;
		goto fail;
	}

	// channel width might not be provided so extra checks are required
	char s[64];
	int ret = bss_get_chan_width_str(bss, s, sizeof(s));
	if (ret > 0) {
		jwidth = json_string(s);
		if (!jwidth) {
			ERR("%s chan width failed\n", __func__);
			err = -ENOMEM;
			goto fail;
		}
	}

	ret = bss_get_mode_str(bss, s, sizeof(s));
	if (ret > 0) {
		jmode = json_string(s);
		if (!jmode) {
			ERR("%s get_mode failed\n", __func__);
			err = -ENOMEM;
			goto fail;
		}
	}

	jbss = json_object();
	if (!jbss) {
		ERR("%s json_object failed\n", __func__);
		err = -ENOMEM;
		goto fail;
	}

	err = json_object_set_new(jbss, "bssid", jbssid);
	ERRCHECK("new bssid");

	err = json_object_set_new(jbss, "ssid", jssid);
	ERRCHECK("new ssid");

	err = json_object_set_new(jbss, "freq", jfreq);
	ERRCHECK("new freq");

	err = json_object_set_new(jbss, "dbm", jdbm);
	ERRCHECK("new dbm");

	if (jwidth) {
		err = json_object_set_new(jbss, "chwidth", jwidth);
		ERRCHECK("new dbm");
	}

	if (jmode) {
		err = json_object_set_new(jbss, "mode", jmode);
		ERRCHECK("new mode");
	}


	PTR_ASSIGN(*p_jbss, jbss);

	return 0;

fail:
	if (jbss) {
		json_decref(jbss);
	}
	if (jssid) {
		json_decref(jssid);
	}
	if (jbssid) {
		json_decref(jbssid);
	}
	if (jfreq) {
		json_decref(jfreq);
	}
	if (jdbm) {
		json_decref(jdbm);
	}
	if (jwidth) {
		json_decref(jwidth);
	}
	if (jmode) {
		json_decref(jmode);
	}

	return err;
}

int bss_to_json(const struct BSS* bss, json_t** p_jbss)
{
	int err = 0;

	err = bss_to_json_summary(bss, p_jbss);

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
		ERRCHECK("bss_to_json_summary")

		err = json_array_append_new(jarray, jbss);
		ERRCHECK("array append");
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

