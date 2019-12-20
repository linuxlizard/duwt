/* Uses chunks of Wireshark, specifically the
 *
 * So this file needs to be carefully aligned with Wireshark's license.
 *
 *
 */ 

#include <stdio.h>
#include <string.h>

#include "core.h"
#include "ie.h"
#include "ie_he.h"


int ie_he_capabilities_new(struct IE* ie)
{
	INFO("%s\n", __func__);
	CONSTRUCT(struct IE_HE_Capabilities)

	// I don't know officially how big this IE is (could be variable length
	// like Extended Capabilities) but let's assume it's a fixed size until I
	// know different
	if (ie->len < 26) {
		return -EINVAL;
	}

	sie->mac_capa = ie->buf;
	sie->phy_capa = ie->buf + 6;
	sie->mcs_and_nss_set = ie->buf + 15;
	sie->ppe_threshold = ie->buf + 19;

	const uint8_t* ptr = ie->buf;
	// skip the Extension Id 
	ptr++;

#define BIT(field,bit) sie->field = !!(*ptr & (1<<bit))

	// bits 0-7
	BIT(htc_he_support, 0);
	BIT(twt_requester_support, 1);
	BIT(twt_responder_support, 2);

	sie->fragmentation_support = (*ptr >> 3) & 3; // 2 bits
	sie->max_number_fragmented_msdus = (*ptr >> 5) & 7; // 3 bits

	// bits 8-14
	ptr++; 
	sie->min_fragment_size = *ptr & 3; // 2 bits
	sie->trigger_frame_mac_padding_dur = (*ptr >> 2) & 3; // 2 bits
	sie->multi_tid_aggregation_support = (*ptr >> 4) & 7; // 3 bits

	// bits 15,16
	// this two bit field is split across bytes
	sie->he_link_adaptation_support = ((*ptr >> 7) & 1) | (((*ptr+1) & 1) << 1);

	// bits 17-23
	ptr++;
	BIT(all_ack_support, 1);
	BIT(trs_support, 2);
	BIT(bsr_support, 3);
	BIT(broadcast_twt_support, 4);
	BIT(_32_bit_ba_bitmap_support, 5);
	BIT(mu_cascading_support, 6);
	BIT(ack_enabled_aggregation_support, 7);

	// bits 24-31
	ptr++;
	// bit 0 reserved
	BIT(om_control_support, 1);
	BIT(ofdma_ra_support, 2);
	sie->max_a_mpdu_length_exponent_ext = (*ptr >> 3) & 3; // 2 bits
	BIT(a_msdu_fragmentation_support, 5);
	BIT(flexible_twt_schedule_support, 6);
	BIT(rx_control_frame_to_multibss, 7);

	// bits 32-38
	ptr++;
	BIT(bsrp_bqrp_a_mpdu_aggregation, 0);
	BIT(qtp_support, 1);
	BIT(bqr_support, 2);
	BIT(srp_responder, 3);
	BIT(ndp_feedback_report_support, 4);
	BIT(ops_support, 5);
	BIT(a_msdu_in_a_mpdu_support, 6);

	// 3-bit field spread across 2 bytes
	sie->multi_tid_aggregation_tx_support = (((*ptr+1) & 3) << 2) | ((*ptr >> 7) & 1); // 3 bits

	// bits 42-47
	BIT(subchannel_selective_trans_support, 2);
	BIT(ul_2_996_tone_ru_support, 3);
	BIT(om_control_ul_mu_data_disable_rx_support, 4);
	// last 3 bits reserved

	return 0;
}

void ie_he_capabilities_free(struct IE* ie)
{
	INFO("%s\n", __func__);
	DESTRUCT(struct IE_HE_Capabilities)
}

int ie_he_operation_new(struct IE* ie)
{
	INFO("%s\n", __func__);
	CONSTRUCT(struct IE_HE_Operation)

	return 0;
}

void ie_he_operation_free(struct IE* ie)
{
	INFO("%s\n", __func__);
	DESTRUCT(struct IE_HE_Operation);
}

const char* he_fragmentation_support_str(uint8_t val)
{
	// packet-ieee80211.c array of same name
	static const char* he_fragmentation_support_vals[] = {
	  "No support for dynamic fragmentation",
	  "Support for dynamic fragments in MPDUs or S-MPDUs",
	  "Support for dynamic fragments in MPDUs and S-MPDUs and up to 1 dyn frag in MSDUs...",
	  "Support for all types of dynamic fragments",
	};

	if (val > ARRAY_SIZE(he_fragmentation_support_vals)) {
		return "(invalid)";
	}
	return he_fragmentation_support_vals[val];
}

// from packet-ieee80211.c function max_frag_msdus_base_custom()
int he_max_frag_msdus_base_to_str(uint8_t max_frag_msdus_value, char* s, size_t len)
{
  if (max_frag_msdus_value == 7)
    return snprintf(s, len, "No restriction");
  else
    return snprintf( s, len, "%u", 1 << max_frag_msdus_value);
}

const char* he_min_fragment_size_str(uint8_t val)
{
	// from packet-ieee80211.c array of same name
	static const char* he_minimum_fragmentation_size_vals[] = {
	  "No restriction on minimum payload size",
	  "Minimum payload size of 128 bytes",
	  "Minimum payload size of 256 bytes",
	  "Minimum payload size of 512 bytes",
	};
	
	if (val > ARRAY_SIZE(he_minimum_fragmentation_size_vals)) {
		return "(invalid)";
	}
	return he_minimum_fragmentation_size_vals[val];
}


const char* he_link_adapt_support_str(uint8_t val)
{
	// from packet-ieee80211.c array of same name
	static const char* he_link_adaptation_support_vals[] = {
	  "No feedback if the STA does not provide HE MFB",
	  "Reserved",
	  "Unsolicited if the STA can receive and provide only unsolicited HE MFB",
	  "Both",
	};

	if (val > ARRAY_SIZE(he_link_adaptation_support_vals)) {
		return "(invalid)";
	}
	return he_link_adaptation_support_vals[val];
}

