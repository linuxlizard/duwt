#include <string.h>

#include <jansson.h>

#include "core.h"
#include "iw.h"
#include "ie.h"
#include "list.h"
#include "bss.h"
#include "bss_json.h"

int bss_to_json(const struct BSS* bss)
{
	json_t* ssid;

	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_SSID);
	const struct IE_SSID* sie = IE_CAST(ie, struct IE_SSID);
	ssid = json_stringn(sie->ssid, sie->ssid_len);
	(void)ssid;

	// TODO 

	return 0;
}

int bss_list_to_json(struct dl_list* bss_list, json_t** p_jarray)
{
	int err = 0;
	json_t* jarray = NULL; // array of bss
	json_t* jbss = NULL; // object containing a single bss; keys "ssid" and "bssid"
	json_t* jssid = NULL;
	json_t* jbssid = NULL;

	jarray = json_array();
	if (!jarray) {
		err = -ENOMEM;
		goto fail;
	}

#define ERRCHECK(msg)\
		if (err) {\
			ERR("%s %s failed\n", __func__, msg);\
			err = -ENOMEM;\
			goto fail;\
		}

	struct BSS* bss;
	dl_list_for_each(bss, bss_list, struct BSS, node) {
		const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_SSID);
		if (ie) {
			const struct IE_SSID* sie = IE_CAST(ie, struct IE_SSID);
			jssid = json_stringn(sie->ssid, sie->ssid_len);
		}
		else {
			jssid = json_string("<hidden>");
		}


		jbssid = json_string(bss->bssid_str);

		jbss = json_object();

		err = json_object_set_new(jbss, "bssid", jbssid);
		ERRCHECK("new bssid");

		err = json_object_set_new(jbss, "ssid", jssid);
		ERRCHECK("new ssid");

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
	if (jssid) {
		json_decref(jssid);
	}
	if (jbssid) {
		json_decref(jbssid);
	}

	return err;
}
