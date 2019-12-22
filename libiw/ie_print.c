#include <stdio.h>

#include "core.h"
#include "ie.h"
#include "ie_print.h"

void print_ie_he_capabilities(const struct IE_HE_Capabilities* sie)
{
	printf("\tHE capabilities:\n");
	char s[128];

	hex_dump(__func__, sie->base->buf, sie->base->len);

	unsigned int bit;
	int ret;

#define PRN(field, _idx)\
	do {\
		typeof (_idx) s_idx = (_idx);\
		ret = he_mac_capa_to_str(sie, s_idx, s, sizeof(s));\
		XASSERT(ret > 0 && (size_t)ret<sizeof(s), ret);\
		printf("%d \t\t * %s\n", s_idx, s);\
	} while(0);

#define PRNBOOL(field, _idx) \
	do {\
		typeof (_idx) s_idx = (_idx);\
		if(sie->field) {\
			ret = he_mac_capa_to_str(sie, s_idx, s, sizeof(s));\
			XASSERT(ret > 0 && (size_t)ret<sizeof(s), ret);\
			printf("%d\t\t * %s\n", s_idx, s);\
		}\
	} while(0);

	// 0-7
	bit = 0;
	PRNBOOL(htc_he_support, bit++)
	PRNBOOL(twt_requester_support,bit++)
	PRNBOOL(twt_responder_support,bit++)
	PRN(fragmentation_support,bit); bit+=2; // 2 bits
	PRN(max_number_fragmented_msdus, bit); bit+=3; // 3 bits
	XASSERT(bit==8, bit);

	// 8-14
	bit = 8;
	PRN(min_fragment_size, bit); bit+=2; // 2 bits
	PRN(trigger_frame_mac_padding_dur, bit); bit+=2;
	PRN(multi_tid_aggregation_support, bit); bit+=3;
	// bits 15,16
	PRN(he_link_adaptation_support, bit); bit+=2;
	XASSERT(bit==17, bit);

	// bits 16-23
	bit = 17; // first field is at bit1
	PRNBOOL(all_ack_support,bit++)
	PRNBOOL(trs_support,bit++)
	PRNBOOL(bsr_support,bit++)
	PRNBOOL(broadcast_twt_support,bit++)
	PRNBOOL(_32_bit_ba_bitmap_support,bit++)
	PRNBOOL(mu_cascading_support,bit++)
	PRNBOOL(ack_enabled_aggregation_support,bit++)
	XASSERT(bit==24, bit);

	// bits 24-31
	bit = 25; // bit 24 reserved
	PRNBOOL(om_control_support,bit++)
	PRNBOOL(ofdma_ra_support,bit++)
	PRN(max_a_mpdu_length_exponent_ext,bit ); bit += 2; // 2 bits
	PRNBOOL(a_msdu_fragmentation_support,bit++)
	PRNBOOL(flexible_twt_schedule_support,bit++)
	PRNBOOL(rx_control_frame_to_multibss,bit++)
	XASSERT(bit==32, bit);

	// bits 32-39
	bit = 32;
	PRNBOOL(bsrp_bqrp_a_mpdu_aggregation,bit++)
	PRNBOOL(qtp_support,bit++)
	PRNBOOL(bqr_support,bit++)
	PRNBOOL(srp_responder,bit++)
	PRNBOOL(ndp_feedback_report_support,bit++)
	PRNBOOL(ops_support,bit++)
	PRNBOOL(a_msdu_in_a_mpdu_support,bit++)
	XASSERT(bit==39, bit);

	// bits 39-41
	PRN(multi_tid_aggregation_support, bit); bit+= 3;
	XASSERT(bit==42, bit);

	bit = 42;
	PRNBOOL(subchannel_selective_trans_support,bit++)
	PRNBOOL(ul_2_996_tone_ru_support,bit++)
	PRNBOOL(om_control_ul_mu_data_disable_rx_support,bit++)

#if 0
	PRN(htc_he_support,"+HTC HE Support")
	PRN(twt_requester_support,"TWT Requester Support")
	PRN(twt_responder_support,"TWT Responder Support")
	printf("\t\t * Fragmentation Support: %s (%d)\n", 
			he_fragmentation_support_str(sie->fragmentation_support),
			sie->fragmentation_support);
	ret = he_max_frag_msdus_base_to_str(sie->max_number_fragmented_msdus, s, sizeof(s));
	XASSERT((size_t)ret < sizeof(s), ret);
	printf("\t\t * Maximum Number of Fragmented MSDUs: %s\n", s);
	printf("\t\t * Minimum Fragment Size: %s (%d)\n", 
			he_min_fragment_size_str(sie->min_fragment_size),
			sie->min_fragment_size);
	printf("\t\t * Trigger Frame MAC Padding Duration: %d\n", sie->trigger_frame_mac_padding_dur);
	printf("\t\t * Multi-TID Aggregation Support: %d\n", sie->multi_tid_aggregation_support);
	printf("\t\t * HE Link Adaptation Support: %s (%d)\n", 
			he_link_adapt_support_str(sie->he_link_adaptation_support),
			sie->he_link_adaptation_support);
	PRN(all_ack_support,"All Ack Support")
	PRN(trs_support,"TRS Support")
	PRN(bsr_support,"BSR Support")
	PRN(broadcast_twt_support,"Broadcast TWT Support")
	PRN(_32_bit_ba_bitmap_support,"32-bit BA Bitmap Support")
	PRN(mu_cascading_support,"MU Cascading Support")
	PRN(ack_enabled_aggregation_support,"Ack-Enabled Aggregation Support")
	PRN(om_control_support,"OM Control Support")
	PRN(ofdma_ra_support,"OFDMA RA Support")
	printf("\t\t * Maximum A-MPDU Length Exponent Extension: %d\n", sie->max_a_mpdu_length_exponent_ext );
	PRN(a_msdu_fragmentation_support,"A-MSDU Fragmentation Support")
	PRN(flexible_twt_schedule_support,"Flexible TWT Schedule Support")
	PRN(rx_control_frame_to_multibss,"Rx Control Frame to MultiBSS")
	PRN(bsrp_bqrp_a_mpdu_aggregation,"BSRP BQRP A-MPDU Aggregation")
	PRN(qtp_support,"QTP Support")
	PRN(bqr_support,"BQR Support")
	PRN(srp_responder,"SRP Responder Role")
	PRN(ndp_feedback_report_support,"NDP Feedback Report Support")
	PRN(ops_support,"OPS Support")
	PRN(a_msdu_in_a_mpdu_support,"A-MSDU in A-MPDU Support")
	printf("\t\t * Multi-TID Aggregation TX Support: %d\n", sie->multi_tid_aggregation_support);
	PRN(subchannel_selective_trans_support,"HE Subchannel Selective Transmission Support")
	PRN(ul_2_996_tone_ru_support,"UL 2x996-tone RU Support")
	PRN(om_control_ul_mu_data_disable_rx_support,"OM Control UL MU Data Disable RX Support")
#endif
#undef PRN
}

