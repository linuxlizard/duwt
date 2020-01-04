// TODO move IE printfs from scan-dump here. Want to make this file responsible
// for "stringify"-ing the IE contents.
//
#include <stdio.h>

#include "core.h"
#include "iw.h"
#include "ie.h"
#include "ie_print.h"

#define INDENT "\t\t\t"

#define PRN(field, _idx)\
	do {\
		typeof (_idx) s_idx = (_idx);\
		ret = STRING_FN(_struct, s_idx, s, sizeof(s));\
		XASSERT(ret > 0 && (size_t)ret<sizeof(s), ret);\
		printf(INDENT " * %s\n", s);\
	} while(0);

#define PRNBOOL(field, _idx) \
	do {\
		typeof (_idx) s_idx = (_idx);\
		if(_struct->field) {\
			ret = STRING_FN(_struct, s_idx, s, sizeof(s));\
			XASSERT(ret > 0 && (size_t)ret<sizeof(s), ret);\
			printf(INDENT " * %s\n", s);\
		}\
	} while(0);

void ie_print_rm_enabled_capabilities(const struct IE_RM_Enabled_Capabilities* sie)
{
	char s[128];
	int ret;

	printf("\tRadio Measurement Capabilities:\n");
#define STRING_FN rm_enabled_capa_to_str
#define _struct sie
	int bit = 0;
	PRNBOOL(link, bit++);
	PRNBOOL(neighbor_report, bit++);
	PRNBOOL(parallel, bit++);
	PRNBOOL(repeated, bit++);
	PRNBOOL(beacon_passive , bit++);
	PRNBOOL(beacon_active , bit++);
	PRNBOOL(beacon_table , bit++);
	PRNBOOL(beacon_measurement_conditions, bit++);
	XASSERT(bit==8, bit);

	PRNBOOL(frame , bit++);
	PRNBOOL(channel_load, bit++);
	PRNBOOL(noise_histogram , bit++);
	PRNBOOL(statistics , bit++);
	PRNBOOL(lci , bit++);
	PRNBOOL(lci_azimuth, bit++);
	PRNBOOL(tx_stream , bit++);
	PRNBOOL(triggered_tx_stream , bit++);
	XASSERT(bit==16, bit);

	PRNBOOL(ap_channel , bit++);
	PRNBOOL(rm_mib , bit++);
	PRN(operating_channel_max, bit); bit+=3;
	PRN(nonoperating_channel_max, bit); bit+=3;

	PRN(measurement_pilot_capa, bit); bit+=3;
	PRNBOOL(measurement_pilot_tx_info_capa , bit++);
	PRNBOOL(neighbor_report_tsf_offset_capa , bit++);
	PRNBOOL(rcpi , bit++);
	PRNBOOL(rsni , bit++);
	PRNBOOL(bss_avg_access_delay, bit++);

	PRNBOOL(bss_avail_admission_capacity, bit++);
	PRNBOOL(antenna, bit++);
	PRNBOOL(ftm_range_report, bit++);
	PRNBOOL(civic_location, bit++);
#undef STRING_FN
#undef _struct
}

void ie_print_mobility_domain(const struct IE_Mobility_Domain* sie)
{
	char s[128];
	unsigned int bit;
	int ret;

	printf("\tMobility domain: \n");
	printf("\t\tMDID: 0x%02x\n", sie->mdid);

#define STRING_FN mobility_domain_to_str
#define _struct sie
	bit = 0;
	PRNBOOL(fast_bss_transition_over_ds, bit++);
	PRNBOOL(resource_req_proto, bit++);

#undef STRING_FN
#undef _struct
}

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
#define _struct mac

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
#undef _struct
#define _struct phy
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
#undef _struct
}

void ie_print_he_operation(const struct IE_HE_Operation* sie)
{
	char s[128];
	unsigned int bit;
	int ret;

#define STRING_FN he_operation_to_str
#define _struct sie

	printf("\tHE operation:\n");
	printf("\t\tHE Operation Parameters:\n");
	bit = 0;
	PRN(default_pe_duration, bit);  bit += 3; 
	PRN(twt_required, bit++);
	PRN(txop_duration_rts_thresh, bit); bit += 10;
	PRNBOOL(fields->vht_op_info_present, bit++);
	PRNBOOL(fields->co_located_bss, bit++);
	PRNBOOL(fields->er_su_disable, bit++);
	bit += 7; // skip reserved bits
	XASSERT(bit==24, bit);

	printf("\t\tHE BSS Color Information\n");
	// bits 24-31
	PRN(bss_color, bit); bit+= 6;
	PRNBOOL(fields->partial_bss_color, bit++);
	PRNBOOL(fields->bss_color_disabled, bit++);

#undef STRING_FN
#undef _struct
}

