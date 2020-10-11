/* UTF8 is fraught with complications. Created this test file so I could make
 * sure I understand how to handle UTF8 SSIDs.
 *
 * JSON RFC https://tools.ietf.org/rfc/rfc8259.txt
 * JSON Strings https://tools.ietf.org/html/rfc8259#section-8
 * https://www.json.org/json-en.html
 *
 * UTF8 RFC https://tools.ietf.org/html/rfc2279
 *
 * http://utf8everywhere.org/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "core.h"
#include "list.h"
#include "bss.h"
#include "ssid.h"
#include "dumpfile.h"


static void test_strings(void)
{
	char s[] = "Académie française";
	size_t len = sizeof(s);

	// this should succeed
	int retcode = ssid_utf8_validate(s, len);
	printf("%s %s retcode=%d\n", __func__, s, retcode);
	XASSERT(retcode==0, retcode);

	// this should fail;
	s[0] = (char)0xef;
	retcode = ssid_utf8_validate(s, len);
	printf("%s %s retcode=%d\n", __func__, s, retcode);
	XASSERT(retcode!=0, retcode);

	// this should fail;
	memset(s,0,len);
	retcode = ssid_utf8_validate(s, len);
	printf("%s %s retcode=%d\n", __func__, s, retcode);

	char s2[] = "你好";
	len = sizeof(s2);
	retcode = ssid_utf8_validate(s2, len);
	printf("%s %s retcode=%d\n", __func__, s2, retcode);
	XASSERT(retcode==0, retcode);

	// break something
	s2[1] = 0x19;
	retcode = ssid_utf8_validate(s2, len);
	printf("%s %s retcode=%d\n", __func__, s2, retcode);
	XASSERT(retcode!=0, retcode);

	// success
	char s3[] = "🤮";
	retcode = ssid_utf8_validate(s3,sizeof(s3));
	printf("%s %s retcode=%d\n", __func__, s3, retcode);

	// success (!?)
	char s4[] = "foo\0bar\0baz";
	retcode = ssid_utf8_validate(s4,sizeof(s4));
	printf("%s %s retcode=%d\n", __func__, s4, retcode);

	// RFC2279 "6.  Security Considerations"
	char s5[] = "\x2F\xC0\xAE\x2E\x2F";
	retcode = ssid_utf8_validate(s5,sizeof(s5));
	printf("%s %s retcode=%d\n", __func__, s5, retcode);
}

int main(int argc, char* argv[])
{
	log_set_level(LOG_LEVEL_DEBUG);

	for (int i=1 ; i<argc ; i++) {
		DEFINE_DL_LIST(bss_list);
		int err = dumpfile_parse(argv[i], &bss_list);
		if (err) {
			fprintf(stderr, "parse of %s failed err=%d %s\n", argv[i], err, strerror(err));
		}

		struct BSS* bss;
		dl_list_for_each(bss, &bss_list, struct BSS, node) {
//			UChar ssid_u[SSID_MAX_LEN*2];
//			int ret = ssid_get_utf8_from_bss(bss, ssid_u, sizeof(ssid_u));
//			if (ret < 0) {
//				ERR("invalid SSID\n");
//				continue;
//			}
//
//			u_printf("ssid=%S\n", ssid_u);

			const struct IE_SSID* sie = bss_get_ssid(bss);
			int ret = ssid_utf8_validate(sie->ssid, sie->ssid_len);

			if (ret == 0) {
				printf("valid ssid=%s\n", sie->ssid);
			}
			else { 
				printf("ssid invalid %s", strerror(ret));
			}
		}

		bss_free_list(&bss_list);
	}

	test_strings();

	return EXIT_SUCCESS;
}
