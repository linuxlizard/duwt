#ifndef SSID_H
#define SSID_H

// https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/
#include <unicode/utypes.h>
#include <unicode/ustring.h>
#include <unicode/utext.h>
#include <unicode/utf8.h>

#ifdef __cplusplus
extern "C" {
#endif

int ssid_to_unicode_str(const char* buf, size_t buf_len, UChar ssid[], size_t ssid_len );
const UChar* ssid_get_hidden(void);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif

