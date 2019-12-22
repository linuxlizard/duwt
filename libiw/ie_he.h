#ifndef IE_HE_H
#define IE_HE_H

#include "ie.h"

// I don't have the US$3000 to get the IEEE 80211.ax standard so I'm using
// Wireshark's decode code. I'm putting HE decode into its own file to
// carefully show the HE code from Wireshark.

struct IE_HE_Capabilities
{
	IE_SPECIFIC_FIELDS

	// pointers into ie->buf
	const uint8_t* mac_capa;  // 6 bytes
	const uint8_t* phy_capa;  // 9 bytes
	const uint8_t* mcs_and_nss_set; // 4  bytes
	const uint8_t* ppe_threshold; // 7 bytes

	// following taken from Wireshark epan/dissectors/packet-ieee80211.c
	// I changed to uint8_t and bitfield.
	//
	/* ************************************************************************* */
	/*                              802.11AX fields                              */
	/* ************************************************************************* */
	// HE Mac Capabilities
	uint8_t htc_he_support : 1;
	uint8_t twt_requester_support : 1;
	uint8_t twt_responder_support : 1;
	uint8_t fragmentation_support : 2;
	uint8_t max_number_fragmented_msdus : 3;

	uint8_t min_fragment_size : 2;
	uint8_t trigger_frame_mac_padding_dur : 2;
	uint8_t multi_tid_aggregation_support : 3;
	uint8_t he_link_adaptation_support : 2;

	uint8_t all_ack_support : 1;
	uint8_t trs_support : 1;
	uint8_t bsr_support : 1;
	uint8_t broadcast_twt_support : 1;
	uint8_t _32_bit_ba_bitmap_support : 1;
	uint8_t mu_cascading_support : 1;
	uint8_t ack_enabled_aggregation_support : 1;

	uint8_t om_control_support : 1;
	uint8_t ofdma_ra_support : 1;
	uint8_t max_a_mpdu_length_exponent_ext : 2;
	uint8_t a_msdu_fragmentation_support : 1;
	uint8_t flexible_twt_schedule_support : 1;
	uint8_t rx_control_frame_to_multibss : 1;

	uint8_t bsrp_bqrp_a_mpdu_aggregation : 1;
	uint8_t qtp_support : 1;
	uint8_t bqr_support : 1;
	uint8_t srp_responder : 1;
	uint8_t ndp_feedback_report_support : 1;
	uint8_t ops_support : 1;
	uint8_t a_msdu_in_a_mpdu_support : 1;
	uint8_t multi_tid_aggregation_tx_support : 3;

	uint8_t subchannel_selective_trans_support : 1;
	uint8_t ul_2_996_tone_ru_support : 1;
	uint8_t om_control_ul_mu_data_disable_rx_support : 1;

	// HE PHY Capabilities
	// Channel Width Set 8-bits
	uint8_t ch40mhz_channel_2_4ghz : 1;
	uint8_t ch40_and_80_mhz_5ghz : 1;
	uint8_t ch160_mhz_5ghz : 1;
	uint8_t ch160_80_plus_80_mhz_5ghz : 1;
	uint8_t ch242_tone_rus_in_2_4ghz : 1;
	uint8_t ch242_tone_rus_in_5ghz : 1;

	uint8_t phy_cap_punctured_preamble_rx : 4;
	uint8_t phy_cap_device_class : 1;
	uint8_t phy_cap_ldpc_coding_in_payload : 1;
	uint8_t phy_cap_he_su_ppdu_1x_he_ltf_08us : 1;
	uint8_t phy_cap_midamble_rx_max_nsts : 2;
	uint8_t phy_cap_ndp_with_4x_he_ltf_32us : 1;
	uint8_t phy_cap_stbc_tx_lt_80mhz : 1;
	uint8_t phy_cap_stbc_rx_lt_80mhz : 1;
	uint8_t phy_cap_doppler_tx : 1;
	uint8_t phy_cap_doppler_rx : 1;
	uint8_t phy_cap_full_bw_ul_mu_mimo : 1;
	uint8_t phy_cap_partial_bw_ul_mu_mimo : 1;

	uint8_t phy_cap_dcm_max_constellation_tx : 2;
	uint8_t phy_cap_dcm_max_nss_tx : 1;
	uint8_t phy_cap_dcm_max_constellation_rx : 2;
	uint8_t phy_cap_dcm_max_nss_rx : 1;
	uint8_t phy_cap_rx_he_muppdu_from_non_ap : 1;
	uint8_t phy_cap_su_beamformer : 1;
	uint8_t phy_cap_su_beamformee : 1;
	uint8_t phy_cap_mu_beamformer : 1;
	uint8_t phy_cap_beamformer_sts_lte_80mhz : 3;
	uint8_t phy_cap_beamformer_sts_gt_80mhz : 3;

	uint8_t phy_cap_number_of_sounding_dims_lte_80 : 3;
	uint8_t phy_cap_number_of_sounding_dims_gt_80 : 3;
	uint8_t phy_cap_ng_eq_16_su_fb : 1;
	uint8_t phy_cap_ng_eq_16_mu_fb : 1;
	uint8_t phy_cap_codebook_size_eq_4_2_fb : 1;
	uint8_t phy_cap_codebook_size_eq_7_5_fb : 1;
	uint8_t phy_cap_triggered_su_beamforming_fb : 1;
	uint8_t phy_cap_triggered_mu_beamforming_fb : 1;
	uint8_t phy_cap_triggered_cqi_fb : 1;
	uint8_t phy_cap_partial_bw_extended_range : 1;
	uint8_t phy_cap_partial_bw_dl_mu_mimo : 1;
	uint8_t phy_cap_ppe_threshold_present : 1;

	uint8_t phy_cap_srp_based_sr_support : 1;
	uint8_t phy_cap_power_boost_factor_ar_support : 1;
	uint8_t phy_cap_he_su_ppdu_etc_gi : 1;
	uint8_t phy_cap_max_nc : 3;
	uint8_t phy_cap_stbc_tx_gt_80_mhz : 1;
	uint8_t phy_cap_stbc_rx_gt_80_mhz : 1;
	uint8_t phy_cap_he_er_su_ppdu_4xxx_gi : 1;
	uint8_t phy_cap_20mhz_in_40mhz_24ghz_band : 1;
	uint8_t phy_cap_20mhz_in_160_80p80_ppdu : 1;
	uint8_t phy_cap_80mgz_in_160_80p80_ppdu : 1;
	uint8_t phy_cap_he_er_su_ppdu_1xxx_gi : 1;
	uint8_t phy_cap_midamble_rx_2x_xxx_ltf : 1;
	uint8_t phy_cap_dcm_max_bw : 2;

	uint8_t phy_cap_longer_than_16_he_sigb_ofdm_symbol_support : 1;
	uint8_t phy_cap_non_triggered_cqi_feedback : 1;
	uint8_t phy_cap_tx_1024_qam_242_tone_ru_support : 1;
	uint8_t phy_cap_rx_1024_qam_242_tone_ru_support : 1;
	uint8_t phy_cap_rx_full_bw_su_using_he_muppdu_w_compressed_sigb : 1;
	uint8_t phy_cap_rx_full_bw_su_using_he_muppdu_w_non_compressed_sigb : 1;
	uint8_t phy_cap_nominal_packet_padding : 2;

	// HE MCS and NSS Set
	// TODO
};

struct IE_HE_Operation
{
	IE_SPECIFIC_FIELDS

	uint8_t params[3];
	uint8_t bss_color;
	uint8_t mcs_and_nss_set[2];
};

int ie_he_capabilities_new(struct IE* ie);
void ie_he_capabilities_free(struct IE* ie);
int ie_he_operation_new(struct IE* ie);
void ie_he_operation_free(struct IE* ie);

const char* he_fragmentation_support_str(uint8_t val);
int he_max_frag_msdus_base_to_str(uint8_t max_frag_msdus_value, char* s, size_t len);
const char* he_min_fragment_size_str(uint8_t val);
const char* he_link_adapt_support_str(uint8_t w);
int he_mac_capa_to_str(const struct IE_HE_Capabilities* sie, unsigned int idx, char* s, size_t len);
int he_phy_capa_to_str(const struct IE_HE_Capabilities* sie, unsigned int idx, char* s, size_t len);

#endif

