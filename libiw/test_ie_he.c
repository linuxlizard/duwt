#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "ie.h"
#include "ie_print.h"

// IE HE Capabilities hex dumps (including Extended ID + Length bytes)

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

static void test_mac_buf1(const struct IE_HE_MAC* mac)
{
	XASSERT(mac->htc_he_support == 1, mac->htc_he_support);
	XASSERT(mac->twt_requester_support == 0, mac->twt_requester_support);
	XASSERT(mac->twt_responder_support == 1, mac->twt_responder_support);
	XASSERT(mac->fragmentation_support == 1, mac->fragmentation_support);
	XASSERT(mac->max_number_fragmented_msdus == 0, mac->max_number_fragmented_msdus);
	XASSERT(mac->min_fragment_size == 1, mac->min_fragment_size);
	XASSERT(mac->trigger_frame_mac_padding_dur == 0, mac->trigger_frame_mac_padding_dur);
	XASSERT(mac->multi_tid_aggregation_support == 0, mac->multi_tid_aggregation_support);
	XASSERT(mac->he_link_adaptation_support == 0, mac->he_link_adaptation_support);
	XASSERT(mac->all_ack_support == 0, mac->all_ack_support);
	XASSERT(mac->trs_support == 0, mac->trs_support);
	XASSERT(mac->bsr_support == 1, mac->bsr_support);
	XASSERT(mac->broadcast_twt_support == 0, mac->broadcast_twt_support);
	XASSERT(mac->_32_bit_ba_bitmap_support == 0, mac->_32_bit_ba_bitmap_support);
	XASSERT(mac->mu_cascading_support == 0, mac->mu_cascading_support);
	XASSERT(mac->ack_enabled_aggregation_support == 0, mac->ack_enabled_aggregation_support);
	XASSERT(mac->reserved_b24 == 0, mac->reserved_b24);
	XASSERT(mac->om_control_support == 1, mac->om_control_support);
	XASSERT(mac->ofdma_ra_support == 0, mac->ofdma_ra_support);
	XASSERT(mac->max_a_mpdu_length_exponent_ext == 3, mac->max_a_mpdu_length_exponent_ext);
	XASSERT(mac->a_msdu_fragmentation_support == 0, mac->a_msdu_fragmentation_support);
	XASSERT(mac->flexible_twt_schedule_support == 0, mac->flexible_twt_schedule_support);
	XASSERT(mac->rx_control_frame_to_multibss == 0, mac->rx_control_frame_to_multibss);
	XASSERT(mac->bsrp_bqrp_a_mpdu_aggregation == 0, mac->bsrp_bqrp_a_mpdu_aggregation);
	XASSERT(mac->qtp_support == 0, mac->qtp_support);
	XASSERT(mac->bqr_support == 0, mac->bqr_support);
	XASSERT(mac->srp_responder == 0, mac->srp_responder);
	XASSERT(mac->ndp_feedback_report_support == 0, mac->ndp_feedback_report_support);
	XASSERT(mac->ops_support == 0, mac->ops_support);
	XASSERT(mac->a_msdu_in_a_mpdu_support == 1, mac->a_msdu_in_a_mpdu_support);
	XASSERT(mac->multi_tid_aggregation_tx_support == 0, mac->multi_tid_aggregation_tx_support);
	XASSERT(mac->subchannel_selective_trans_support == 0, mac->subchannel_selective_trans_support);
	XASSERT(mac->ul_2_996_tone_ru_support == 0, mac->ul_2_996_tone_ru_support);
	XASSERT(mac->om_control_ul_mu_data_disable_rx_support == 0, mac->om_control_ul_mu_data_disable_rx_support);
}

static void test_phy_buf1(const struct IE_HE_PHY* phy)
{
	XASSERT(phy->reserved_b0 == 0, phy->reserved_b0);
	XASSERT(phy->ch40mhz_channel_2_4ghz == 0, phy->ch40mhz_channel_2_4ghz);
	XASSERT(phy->ch40_and_80_mhz_5ghz == 1, phy->ch40_and_80_mhz_5ghz);
	XASSERT(phy->ch160_mhz_5ghz == 0, phy->ch160_mhz_5ghz);
	XASSERT(phy->ch160_80_plus_80_mhz_5ghz == 0, phy->ch160_80_plus_80_mhz_5ghz);
	XASSERT(phy->ch242_tone_rus_in_2_4ghz == 0, phy->ch242_tone_rus_in_2_4ghz);
	XASSERT(phy->ch242_tone_rus_in_5ghz == 0, phy->ch242_tone_rus_in_5ghz);
	XASSERT(phy->reserved_b7 == 0, phy->reserved_b7);
	XASSERT(phy->punctured_preamble_rx == 0, phy->punctured_preamble_rx);
	XASSERT(phy->device_class == 0, phy->device_class);
	XASSERT(phy->ldpc_coding_in_payload == 1, phy->ldpc_coding_in_payload);
	XASSERT(phy->he_su_ppdu_1x_he_ltf_08us == 1, phy->he_su_ppdu_1x_he_ltf_08us);
	XASSERT(phy->midamble_rx_max_nsts == 0, phy->midamble_rx_max_nsts);
	XASSERT(phy->ndp_with_4x_he_ltf_32us == 0, phy->ndp_with_4x_he_ltf_32us);
	XASSERT(phy->stbc_tx_lt_80mhz == 1, phy->stbc_tx_lt_80mhz);
	XASSERT(phy->stbc_rx_lt_80mhz == 1, phy->stbc_rx_lt_80mhz);
	XASSERT(phy->doppler_tx == 0, phy->doppler_tx);
	XASSERT(phy->doppler_rx == 0, phy->doppler_rx);
	XASSERT(phy->full_bw_ul_mu_mimo == 1, phy->full_bw_ul_mu_mimo);
	XASSERT(phy->partial_bw_ul_mu_mimo == 0, phy->partial_bw_ul_mu_mimo);
	XASSERT(phy->dcm_max_constellation_tx == 1, phy->dcm_max_constellation_tx);
	XASSERT(phy->dcm_max_nss_tx == 0, phy->dcm_max_nss_tx);
	XASSERT(phy->dcm_max_constellation_rx == 1, phy->dcm_max_constellation_rx);
	XASSERT(phy->dcm_max_nss_rx == 0, phy->dcm_max_nss_rx);
	XASSERT(phy->rx_he_muppdu_from_non_ap == 0, phy->rx_he_muppdu_from_non_ap);
	XASSERT(phy->su_beamformer == 1, phy->su_beamformer);
	XASSERT(phy->su_beamformee == 1, phy->su_beamformee);
	XASSERT(phy->mu_beamformer == 1, phy->mu_beamformer);
	XASSERT(phy->beamformer_sts_lte_80mhz == 7, phy->beamformer_sts_lte_80mhz);
	XASSERT(phy->beamformer_sts_gt_80mhz == 3, phy->beamformer_sts_gt_80mhz);
	XASSERT(phy->number_of_sounding_dims_lte_80 == 1, phy->number_of_sounding_dims_lte_80);
	XASSERT(phy->number_of_sounding_dims_gt_80 == 0, phy->number_of_sounding_dims_gt_80);
	XASSERT(phy->ng_eq_16_su_fb == 1, phy->ng_eq_16_su_fb);
	XASSERT(phy->ng_eq_16_mu_fb == 1, phy->ng_eq_16_mu_fb);
	XASSERT(phy->codebook_size_eq_4_2_fb == 1, phy->codebook_size_eq_4_2_fb);
	XASSERT(phy->codebook_size_eq_7_5_fb == 1, phy->codebook_size_eq_7_5_fb);
	XASSERT(phy->triggered_su_beamforming_fb == 0, phy->triggered_su_beamforming_fb);
	XASSERT(phy->triggered_mu_beamforming_fb == 0, phy->triggered_mu_beamforming_fb);
	XASSERT(phy->triggered_cqi_fb == 0, phy->triggered_cqi_fb);
	XASSERT(phy->partial_bw_extended_range == 0, phy->partial_bw_extended_range);
	XASSERT(phy->partial_bw_dl_mu_mimo == 0, phy->partial_bw_dl_mu_mimo);
	XASSERT(phy->ppe_threshold_present == 1, phy->ppe_threshold_present);
	XASSERT(phy->srp_based_sr_support == 0, phy->srp_based_sr_support);
	XASSERT(phy->power_boost_factor_ar_support == 0, phy->power_boost_factor_ar_support);
	XASSERT(phy->he_su_ppdu_etc_gi == 1, phy->he_su_ppdu_etc_gi);
	XASSERT(phy->max_nc == 3, phy->max_nc);
	XASSERT(phy->stbc_tx_gt_80_mhz == 0, phy->stbc_tx_gt_80_mhz);
	XASSERT(phy->stbc_rx_gt_80_mhz == 1, phy->stbc_rx_gt_80_mhz);
	XASSERT(phy->he_er_su_ppdu_4xxx_gi == 1, phy->he_er_su_ppdu_4xxx_gi);
	XASSERT(phy->_20mhz_in_40mhz_24ghz_band == 0, phy->_20mhz_in_40mhz_24ghz_band);
	XASSERT(phy->_20mhz_in_160_80p80_ppdu == 0, phy->_20mhz_in_160_80p80_ppdu);
	XASSERT(phy->_80mgz_in_160_80p80_ppdu == 0, phy->_80mgz_in_160_80p80_ppdu);
	XASSERT(phy->he_er_su_ppdu_1xxx_gi == 0, phy->he_er_su_ppdu_1xxx_gi);
	XASSERT(phy->midamble_rx_2x_xxx_ltf == 0, phy->midamble_rx_2x_xxx_ltf);
	XASSERT(phy->dcm_max_bw == 0, phy->dcm_max_bw);
	XASSERT(phy->longer_than_16_he_sigb_ofdm_symbol_support == 0, phy->longer_than_16_he_sigb_ofdm_symbol_support);
	XASSERT(phy->non_triggered_cqi_feedback == 0, phy->non_triggered_cqi_feedback);
	XASSERT(phy->tx_1024_qam_242_tone_ru_support == 0, phy->tx_1024_qam_242_tone_ru_support);
	XASSERT(phy->rx_1024_qam_242_tone_ru_support == 1, phy->rx_1024_qam_242_tone_ru_support);
	XASSERT(phy->rx_full_bw_su_using_he_muppdu_w_compressed_sigb == 0, phy->rx_full_bw_su_using_he_muppdu_w_compressed_sigb);
	XASSERT(phy->rx_full_bw_su_using_he_muppdu_w_non_compressed_sigb == 0, phy->rx_full_bw_su_using_he_muppdu_w_non_compressed_sigb);
	XASSERT(phy->nominal_packet_padding == 0, phy->nominal_packet_padding);
}

static void test_buf2(void)
{
	const struct IE_HE_MAC* mac = (const struct IE_HE_MAC*)(buf2 + 3);
	const struct IE_HE_PHY* phy = (const struct IE_HE_PHY*)(buf2 + 3 + 6);
	XASSERT(mac->htc_he_support == 1, mac->htc_he_support);
	XASSERT(mac->twt_requester_support == 0, mac->twt_requester_support);
	XASSERT(mac->twt_responder_support == 1, mac->twt_responder_support);
	XASSERT(mac->fragmentation_support == 1, mac->fragmentation_support);
	XASSERT(mac->max_number_fragmented_msdus == 0, mac->max_number_fragmented_msdus);
	XASSERT(mac->min_fragment_size == 0, mac->min_fragment_size);
	XASSERT(mac->trigger_frame_mac_padding_dur == 0, mac->trigger_frame_mac_padding_dur);
	XASSERT(mac->multi_tid_aggregation_support == 0, mac->multi_tid_aggregation_support);
	XASSERT(mac->he_link_adaptation_support == 0, mac->he_link_adaptation_support);
	XASSERT(mac->all_ack_support == 0, mac->all_ack_support);
	XASSERT(mac->trs_support == 0, mac->trs_support);
	XASSERT(mac->bsr_support == 1, mac->bsr_support);
	XASSERT(mac->broadcast_twt_support == 0, mac->broadcast_twt_support);
	XASSERT(mac->_32_bit_ba_bitmap_support == 0, mac->_32_bit_ba_bitmap_support);
	XASSERT(mac->mu_cascading_support == 0, mac->mu_cascading_support);
	XASSERT(mac->ack_enabled_aggregation_support == 0, mac->ack_enabled_aggregation_support);
	XASSERT(mac->reserved_b24 == 0, mac->reserved_b24);
	XASSERT(mac->om_control_support == 1, mac->om_control_support);
	XASSERT(mac->ofdma_ra_support == 0, mac->ofdma_ra_support);
	XASSERT(mac->max_a_mpdu_length_exponent_ext == 2, mac->max_a_mpdu_length_exponent_ext);
	XASSERT(mac->a_msdu_fragmentation_support == 0, mac->a_msdu_fragmentation_support);
	XASSERT(mac->flexible_twt_schedule_support == 0, mac->flexible_twt_schedule_support);
	XASSERT(mac->rx_control_frame_to_multibss == 0, mac->rx_control_frame_to_multibss);
	XASSERT(mac->bsrp_bqrp_a_mpdu_aggregation == 0, mac->bsrp_bqrp_a_mpdu_aggregation);
	XASSERT(mac->qtp_support == 0, mac->qtp_support);
	XASSERT(mac->bqr_support == 0, mac->bqr_support);
	XASSERT(mac->srp_responder == 0, mac->srp_responder);
	XASSERT(mac->ndp_feedback_report_support == 0, mac->ndp_feedback_report_support);
	XASSERT(mac->ops_support == 0, mac->ops_support);
	XASSERT(mac->a_msdu_in_a_mpdu_support == 0, mac->a_msdu_in_a_mpdu_support);
	XASSERT(mac->multi_tid_aggregation_tx_support == 0, mac->multi_tid_aggregation_tx_support);
	XASSERT(mac->subchannel_selective_trans_support == 0, mac->subchannel_selective_trans_support);
	XASSERT(mac->ul_2_996_tone_ru_support == 0, mac->ul_2_996_tone_ru_support);
	XASSERT(mac->om_control_ul_mu_data_disable_rx_support == 1, mac->om_control_ul_mu_data_disable_rx_support);
	XASSERT(phy->reserved_b0 == 0, phy->reserved_b0);
	XASSERT(phy->ch40mhz_channel_2_4ghz == 0, phy->ch40mhz_channel_2_4ghz);
	XASSERT(phy->ch40_and_80_mhz_5ghz == 1, phy->ch40_and_80_mhz_5ghz);
	XASSERT(phy->ch160_mhz_5ghz == 1, phy->ch160_mhz_5ghz);
	XASSERT(phy->ch160_80_plus_80_mhz_5ghz == 0, phy->ch160_80_plus_80_mhz_5ghz);
	XASSERT(phy->ch242_tone_rus_in_2_4ghz == 0, phy->ch242_tone_rus_in_2_4ghz);
	XASSERT(phy->ch242_tone_rus_in_5ghz == 0, phy->ch242_tone_rus_in_5ghz);
	XASSERT(phy->reserved_b7 == 0, phy->reserved_b7);
	XASSERT(phy->punctured_preamble_rx == 0, phy->punctured_preamble_rx);
	XASSERT(phy->device_class == 0, phy->device_class);
	XASSERT(phy->ldpc_coding_in_payload == 1, phy->ldpc_coding_in_payload);
	XASSERT(phy->he_su_ppdu_1x_he_ltf_08us == 0, phy->he_su_ppdu_1x_he_ltf_08us);
	XASSERT(phy->midamble_rx_max_nsts == 0, phy->midamble_rx_max_nsts);
	XASSERT(phy->ndp_with_4x_he_ltf_32us == 1, phy->ndp_with_4x_he_ltf_32us);
	XASSERT(phy->stbc_tx_lt_80mhz == 0, phy->stbc_tx_lt_80mhz);
	XASSERT(phy->stbc_rx_lt_80mhz == 0, phy->stbc_rx_lt_80mhz);
	XASSERT(phy->doppler_tx == 0, phy->doppler_tx);
	XASSERT(phy->doppler_rx == 0, phy->doppler_rx);
	XASSERT(phy->full_bw_ul_mu_mimo == 0, phy->full_bw_ul_mu_mimo);
	XASSERT(phy->partial_bw_ul_mu_mimo == 0, phy->partial_bw_ul_mu_mimo);
	XASSERT(phy->dcm_max_constellation_tx == 0, phy->dcm_max_constellation_tx);
	XASSERT(phy->dcm_max_nss_tx == 0, phy->dcm_max_nss_tx);
	XASSERT(phy->dcm_max_constellation_rx == 0, phy->dcm_max_constellation_rx);
	XASSERT(phy->dcm_max_nss_rx == 0, phy->dcm_max_nss_rx);
	XASSERT(phy->rx_he_muppdu_from_non_ap == 1, phy->rx_he_muppdu_from_non_ap);
	XASSERT(phy->su_beamformer == 1, phy->su_beamformer);
	XASSERT(phy->su_beamformee == 1, phy->su_beamformee);
	XASSERT(phy->mu_beamformer == 1, phy->mu_beamformer);
	XASSERT(phy->beamformer_sts_lte_80mhz == 3, phy->beamformer_sts_lte_80mhz);
	XASSERT(phy->beamformer_sts_gt_80mhz == 3, phy->beamformer_sts_gt_80mhz);
	XASSERT(phy->number_of_sounding_dims_lte_80 == 3, phy->number_of_sounding_dims_lte_80);
	XASSERT(phy->number_of_sounding_dims_gt_80 == 3, phy->number_of_sounding_dims_gt_80);
	XASSERT(phy->ng_eq_16_su_fb == 1, phy->ng_eq_16_su_fb);
	XASSERT(phy->ng_eq_16_mu_fb == 0, phy->ng_eq_16_mu_fb);
	XASSERT(phy->codebook_size_eq_4_2_fb == 1, phy->codebook_size_eq_4_2_fb);
	XASSERT(phy->codebook_size_eq_7_5_fb == 1, phy->codebook_size_eq_7_5_fb);
	XASSERT(phy->triggered_su_beamforming_fb == 0, phy->triggered_su_beamforming_fb);
	XASSERT(phy->triggered_mu_beamforming_fb == 0, phy->triggered_mu_beamforming_fb);
	XASSERT(phy->triggered_cqi_fb == 0, phy->triggered_cqi_fb);
	XASSERT(phy->partial_bw_extended_range == 0, phy->partial_bw_extended_range);
	XASSERT(phy->partial_bw_dl_mu_mimo == 0, phy->partial_bw_dl_mu_mimo);
	XASSERT(phy->ppe_threshold_present == 1, phy->ppe_threshold_present);
	XASSERT(phy->srp_based_sr_support == 0, phy->srp_based_sr_support);
	XASSERT(phy->power_boost_factor_ar_support == 0, phy->power_boost_factor_ar_support);
	XASSERT(phy->he_su_ppdu_etc_gi == 0, phy->he_su_ppdu_etc_gi);
	XASSERT(phy->max_nc == 3, phy->max_nc);
	XASSERT(phy->stbc_tx_gt_80_mhz == 0, phy->stbc_tx_gt_80_mhz);
	XASSERT(phy->stbc_rx_gt_80_mhz == 0, phy->stbc_rx_gt_80_mhz);
	XASSERT(phy->he_er_su_ppdu_4xxx_gi == 0, phy->he_er_su_ppdu_4xxx_gi);
	XASSERT(phy->_20mhz_in_40mhz_24ghz_band == 0, phy->_20mhz_in_40mhz_24ghz_band);
	XASSERT(phy->_20mhz_in_160_80p80_ppdu == 0, phy->_20mhz_in_160_80p80_ppdu);
	XASSERT(phy->_80mgz_in_160_80p80_ppdu == 0, phy->_80mgz_in_160_80p80_ppdu);
	XASSERT(phy->he_er_su_ppdu_1xxx_gi == 0, phy->he_er_su_ppdu_1xxx_gi);
	XASSERT(phy->midamble_rx_2x_xxx_ltf == 0, phy->midamble_rx_2x_xxx_ltf);
	XASSERT(phy->dcm_max_bw == 0, phy->dcm_max_bw);
	XASSERT(phy->longer_than_16_he_sigb_ofdm_symbol_support == 0, phy->longer_than_16_he_sigb_ofdm_symbol_support);
	XASSERT(phy->non_triggered_cqi_feedback == 0, phy->non_triggered_cqi_feedback);
	XASSERT(phy->tx_1024_qam_242_tone_ru_support == 1, phy->tx_1024_qam_242_tone_ru_support);
	XASSERT(phy->rx_1024_qam_242_tone_ru_support == 1, phy->rx_1024_qam_242_tone_ru_support);
	XASSERT(phy->rx_full_bw_su_using_he_muppdu_w_compressed_sigb == 0, phy->rx_full_bw_su_using_he_muppdu_w_compressed_sigb);
	XASSERT(phy->rx_full_bw_su_using_he_muppdu_w_non_compressed_sigb == 0, phy->rx_full_bw_su_using_he_muppdu_w_non_compressed_sigb);
	XASSERT(phy->nominal_packet_padding == 0, phy->nominal_packet_padding);
}

int main(void)
{
	decode_and_print(buf1);
	decode_and_print(buf2);

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

	test_mac_buf1(mac);
	test_phy_buf1(phy);
	test_buf2();
	return EXIT_SUCCESS;
}

