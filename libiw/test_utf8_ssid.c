/* UTF8 is fraught with complications. Created this test file so I could make
 * sure I understand how to handle UTF8 SSIDs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/
#include <unicode/utypes.h>
#include <unicode/utf8.h>
#include <unicode/ustdio.h>

#include "core.h"
#include "list.h"
#include "bss.h"
#include "dumpfile.h"

int main(int argc, char* argv[])
{
	log_set_level(LOG_LEVEL_INFO);

	for (int i=1 ; i<argc ; i++) {
		DEFINE_DL_LIST(bss_list);
		int err = dumpfile_parse(argv[i], &bss_list);
		if (err) {
			fprintf(stderr, "parse of %s failed err=%d %s\n", argv[i], err, strerror(err));
		}

		struct BSS* bss;
		dl_list_for_each(bss, &bss_list, struct BSS, node) {
			UChar ssid_u[SSID_MAX_LEN*2];
			int ret = bss_get_utf8_ssid(bss, ssid_u, sizeof(ssid_u));

			u_printf("ssid=%S\n", ssid_u);
		}

		bss_free_list(&bss_list);
	}

	return EXIT_SUCCESS;
}
