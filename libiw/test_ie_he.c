#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "ie.h"
#include "ie_print.h"

// QCOM 807x
const uint8_t buf1[] = {
	0xff, 0x1d,
	0x23, 0x0d, 0x01, 0x08, 0x1a, 0x40, 0x00, 0x04, 0x60, 0x4c, 0x89, 0x7f,
	0xc1, 0x83, 0x9c, 0x01, 0x08, 0x00, 0xfa, 0xff, 0xfa, 0xff, 0x79, 0x1c,
	0xc7, 0x71, 0x1c, 0xc7, 0x71
};

// ASUS RT_AX88U (BRCM)
const uint8_t buf2[] = {
	0xff, 0x1d,

	0x23, 0x0d, 0x00, 0x08, 0x12, 0x00, 0x10, 0x0c, 0x20, 0x02, 0xc0, 0x6f,
	0x5b, 0x83, 0x18, 0x00, 0x0c, 0x00, 0xaa, 0xff, 0xaa, 0xff, 0xaa, 0xff,
	0xaa, 0xff, 0x3b, 0x1c, 0xc7, 0x71, 0x1c, 0xc7, 0x71, 0x1c, 0xc7, 0x71 
};


static void decode_and_print(const uint8_t* buf)
{
	struct IE* ie = ie_new(buf[0], buf[1], buf+2);
	XASSERT(ie, 0);

	struct IE_HE_Capabilities* sie = IE_CAST(ie, struct IE_HE_Capabilities);
	hex_dump("IE HE MAC", sie->mac_capa, 6);
	hex_dump("IE HE PHY", sie->phy_capa, 9);

	XASSERT(sie->mac->htc_he_support, sie->mac->htc_he_support);
	XASSERT(!sie->mac->twt_requester_support, sie->mac->twt_requester_support);
	XASSERT(sie->mac->twt_responder_support, sie->mac->twt_responder_support);

	ie_print_he_capabilities(sie);
}

int main(void)
{
	decode_and_print(buf1);
//	decode_and_print(buf2);

	const struct IE_HE_MAC* mac = (const struct IE_HE_MAC*)(buf1 + 3);
	hex_dump("mac", (const unsigned char *)mac, sizeof(struct IE_HE_MAC));

	const struct IE_HE_PHY* phy = (const struct IE_HE_PHY*)(buf1 + 3 + 6);
	hex_dump("phy", (const unsigned char *)phy, sizeof(struct IE_HE_PHY));
	printf("%#02x %#02x %#02x %#02x %#02x %#02x %#02x %#02x %#02x %#02x %#02x %#02x\n", 
			phy->punctured_preamble_rx, 
			phy->device_class, 
			phy->ldpc_coding_in_payload,
			phy->he_su_ppdu_1x_he_ltf_08us,
			phy->midamble_rx_max_nsts,
			phy->ndp_with_4x_he_ltf_32us,
			phy->stbc_tx_lt_80mhz,
			phy->stbc_rx_lt_80mhz,
			phy->doppler_tx,
			phy->doppler_rx,
			phy->full_bw_ul_mu_mimo,
			phy->partial_bw_ul_mu_mimo
	);

	ie_print_he_capabilities_mac(mac);
	ie_print_he_capabilities_phy(phy);

	return EXIT_SUCCESS;
}

