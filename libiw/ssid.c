
// https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/
#include <unicode/utypes.h>
#include <unicode/ustring.h>
#include <unicode/utext.h>
#include <unicode/utf8.h>
#include <unicode/ustdio.h>

#include "core.h"
#include "bss.h"
#include "ssid.h"

U_STRING_DECL(hidden, "<hidden>", 8);
static int hidden_len = 0;

int ssid_to_unicode_str(const char* buf, size_t buf_len, UChar ssid[], size_t ssid_len )
{
	if (hidden_len == 0) {
		U_STRING_INIT(hidden, "<hidden>", 8);
		hidden_len = u_strlen(hidden);
	}

	if (buf==NULL || buf_len==0 || buf[0] == 0) {
		u_strncpy(ssid, hidden, ssid_len);
		return hidden_len;
	}

	int32_t new_ssid_len = (int32_t)ssid_len;

	// http://userguide.icu-project.org/strings
	// http://userguide.icu-project.org/strings/utf-8
	UErrorCode status = U_ZERO_ERROR;
	u_strFromUTF8(ssid, ssid_len, &new_ssid_len, buf, buf_len, &status);
	if ( !U_SUCCESS(status)) {
		ERR("%s unicode parse fail status=%d\n", __func__, status);
		return -EINVAL;
	}

	return ssid_len;
}

const UChar* ssid_get_hidden(void)
{
	if (hidden_len == 0) {
		U_STRING_INIT(hidden, "<hidden>", 8);
		hidden_len = u_strlen(hidden);
	}
	return hidden;
}

int bss_get_utf8_ssid(const struct BSS* bss, UChar u_ssid[], size_t u_ssid_len )
{
	const struct IE_SSID* sie = bss_get_ssid(bss);
	if (!sie) {
		// returns a copy of hidden SSID u_string
		return ssid_to_unicode_str(NULL, 0, u_ssid, u_ssid_len);
	}

	return ssid_to_unicode_str(sie->ssid, sie->ssid_len, u_ssid, u_ssid_len);
}

