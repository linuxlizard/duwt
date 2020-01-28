#include <stdio.h>
#include <string.h>
#include <linux/nl80211.h>

#include "core.h"
#include "nlnames.h"

int main(void)
{
	for( int i=0 ; ; i++ ) {
		const char* s = to_string_nl80211_attrs((enum nl80211_attrs)i);
		if (!s || strncmp(s, "unknown",7) == 0) {
			break;
		}
		printf("i=%d i=%#0x s=%s\n", i, i, s);
	}

	for( int i=0 ; ; i++ ) {
		const char* s = to_string_nl80211_bss((enum nl80211_bss)i);
		if (!s || strncmp(s, "unknown",7) == 0) {
			break;
		}
		printf("i=%d i=%#0x s=%s\n", i, i, s);
	}

	return 0;
}


