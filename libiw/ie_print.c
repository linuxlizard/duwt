#include <stdio.h>

#include "core.h"
#include "ie.h"
#include "ie_print.h"

void ie_print_he_capabilities(const struct IE_HE_Capabilities* sie)
{
	printf("\tHE capabilities:\n");
	ie_print_he_capabilities_mac(sie->mac);
	ie_print_he_capabilities_phy(sie->phy);
}

void ie_print_he_capabilities_mac(const struct IE_HE_MAC* mac)
{
	char s[128];
	unsigned int bit;
	int ret;

#define STRING_FN he_mac_capa_to_str

#define sie mac

#define PRN(field, _idx)\
	do {\
		typeof (_idx) s_idx = (_idx);\
		ret = STRING_FN(sie, s_idx, s, sizeof(s));\
		XASSERT(ret > 0 && (size_t)ret<sizeof(s), ret);\
		printf("\t\t\t * %s\n", s);\
	} while(0);

#define PRNBOOL(field, _idx) \
	do {\
		typeof (_idx) s_idx = (_idx);\
		if(sie->field) {\
			ret = STRING_FN(sie, s_idx, s, sizeof(s));\
			XASSERT(ret > 0 && (size_t)ret<sizeof(s), ret);\
			printf("\t\t\t * %s\n", s);\
		}\
	} while(0);

	printf("\t\tHE MAC capabilities:\n");
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
}

void ie_print_he_capabilities_phy(const struct IE_HE_PHY* phy)
{
	char s[128];
	unsigned int bit;
	int ret;

#undef STRING_FN
#undef sie
#define sie phy
#define STRING_FN he_phy_capa_to_str

	printf("\t\tHE PHY capabilities:\n");
	// bit 0 reserved
	bit = 1;
	PRNBOOL(ch40mhz_channel_2_4ghz, bit++)
	PRNBOOL(ch40_and_80_mhz_5ghz , bit++)
	PRNBOOL(ch160_mhz_5ghz       , bit++)
	PRNBOOL(ch160_80_plus_80_mhz_5ghz , bit++)
	PRNBOOL(ch242_tone_rus_in_2_4ghz , bit++)
	PRNBOOL(ch242_tone_rus_in_5ghz, bit++)
	bit++; // bit 7 reserved
	XASSERT(bit==8, bit);

	// bits 8-23
	bit = 8;
	PRN(punctured_preamble_rx,bit); bit+=4; 
	PRN(device_class,bit++)
	PRNBOOL(ldpc_coding_in_payload, bit++);
	PRNBOOL(he_su_ppdu_1x_he_ltf_08us, bit++);
	PRN(midamble_rx_max_nsts, bit); bit+=2;
	PRNBOOL(ndp_with_4x_he_ltf_32us, bit++);
	PRNBOOL(stbc_tx_lt_80mhz, bit++);
	PRNBOOL(stbc_rx_lt_80mhz, bit++);
	PRNBOOL(doppler_tx, bit++);
	PRNBOOL(doppler_rx, bit++);
	PRNBOOL(full_bw_ul_mu_mimo, bit++);
	PRNBOOL(partial_bw_ul_mu_mimo, bit++);
	XASSERT(bit==24, bit);

	// 24-39
	PRN(dcm_max_constellation_tx, bit); bit+=2;
	PRNBOOL(dcm_max_nss_tx, bit++); // 1
	PRN(dcm_max_constellation_rx, bit); bit+=2; // 2
	PRNBOOL(dcm_max_nss_rx, bit++); // 1
	PRNBOOL(rx_he_muppdu_from_non_ap, bit++); // 1
	PRNBOOL(su_beamformer, bit++); // 1
	PRNBOOL(su_beamformee, bit++); // 1
	PRNBOOL(mu_beamformer, bit++); // 1
	PRN(beamformer_sts_lte_80mhz, bit); bit+=3; // 3
	PRN(beamformer_sts_gt_80mhz, bit); bit+=3;
	XASSERT(bit==40, bit);

	// 40-55
	PRN(number_of_sounding_dims_lte_80, bit); bit+=3;
	PRN(number_of_sounding_dims_gt_80, bit); bit+=3; // 3
	PRNBOOL(ng_eq_16_su_fb, bit++); // 1
	PRNBOOL(ng_eq_16_mu_fb, bit++); // 1
	PRNBOOL(codebook_size_eq_4_2_fb, bit++); // 1
	PRNBOOL(codebook_size_eq_7_5_fb, bit++); // 1
	PRNBOOL(triggered_su_beamforming_fb, bit++); // 1
	PRNBOOL(triggered_mu_beamforming_fb, bit++); // 1
	PRNBOOL(triggered_cqi_fb, bit++); // 1
	PRNBOOL(partial_bw_extended_range, bit++); // 1
	PRNBOOL(partial_bw_dl_mu_mimo, bit++); // 1
	PRNBOOL(ppe_threshold_present, bit++);
	XASSERT(bit==56, bit);

	// 56-71
	PRNBOOL(srp_based_sr_support, bit++);
	PRNBOOL(power_boost_factor_ar_support, bit++); // 1
	PRNBOOL(he_su_ppdu_etc_gi, bit++); // 1
	PRN(max_nc, bit); bit+=3; // 3
	PRNBOOL(stbc_tx_gt_80_mhz, bit++); // 1
	PRNBOOL(stbc_rx_gt_80_mhz, bit++); // 1
	PRNBOOL(he_er_su_ppdu_4xxx_gi, bit++); // 1
	PRNBOOL(_20mhz_in_40mhz_24ghz_band, bit++); // 1
	PRNBOOL(_20mhz_in_160_80p80_ppdu, bit++); // 1
	PRNBOOL(_80mgz_in_160_80p80_ppdu, bit++); // 1
	PRNBOOL(he_er_su_ppdu_1xxx_gi, bit++); // 1
	PRNBOOL(midamble_rx_2x_xxx_ltf, bit++); // 1
	PRN(dcm_max_bw, bit); bit += 2;
	XASSERT(bit==72, bit);

	// 72-87
	PRNBOOL(longer_than_16_he_sigb_ofdm_symbol_support, bit++);
	PRNBOOL(non_triggered_cqi_feedback, bit++); // 1
	PRNBOOL(tx_1024_qam_242_tone_ru_support, bit++); // 1
	PRNBOOL(rx_1024_qam_242_tone_ru_support, bit++); // 1
	PRNBOOL(rx_full_bw_su_using_he_muppdu_w_compressed_sigb, bit++); // 1
	PRNBOOL(rx_full_bw_su_using_he_muppdu_w_non_compressed_sigb, bit++); // 1
	PRN(nominal_packet_padding, bit ); bit++; // 2

#undef STRING_FN
#undef sie
#undef PRN
#undef PRNBOOL
}
