#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "ie.h"
#include "ie_print.h"

const uint8_t buf[] = {
 0xff, 0x1d,
 0x23, 0x0d, 0x01, 0x08, 0x1a, 0x40, 0x00, 0x04, 0x60, 0x4c, 0x89, 0x7f, 0xc1, 0x83, 0x9c, 0x01,
 0x08, 0x00, 0xfa, 0xff, 0xfa, 0xff, 0x79, 0x1c, 0xc7, 0x71, 0x1c, 0xc7, 0x71
};

int main(void)
{
	struct IE* ie = ie_new(buf[0], buf[1], buf+2);
	XASSERT(ie, 0);

	struct IE_HE_Capabilities* sie = IE_CAST(ie, struct IE_HE_Capabilities);
	XASSERT(sie->htc_he_support, sie->htc_he_support);
	XASSERT(!sie->twt_requester_support, sie->twt_requester_support);
	XASSERT(sie->twt_responder_support, sie->twt_responder_support);

	print_ie_he_capabilities(sie);

	return EXIT_SUCCESS;
}

