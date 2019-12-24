#ifndef IE_HE_H
#define IE_HE_H

#include "ie.h"

// I don't have the US$3000 to get the IEEE 80211.ax standard so I'm using
// Wireshark's decode code. I'm putting HE decode into its own file to
// carefully show the HE code from Wireshark.

struct __attribute__((__packed__)) IE_HE_MAC
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	// 0-7
	uint64_t htc_he_support : 1,
	twt_requester_support : 1,
	twt_responder_support : 1,
	fragmentation_support : 2,
	max_number_fragmented_msdus : 3,

	// 8-14
	min_fragment_size : 2,
	trigger_frame_mac_padding_dur : 2,
	multi_tid_aggregation_support : 3,

	// 15,16
	he_link_adaptation_support : 2,

	// 17-23
	all_ack_support : 1,
	trs_support : 1,
	bsr_support : 1,
	broadcast_twt_support : 1,
	_32_bit_ba_bitmap_support : 1,
	mu_cascading_support : 1,
	ack_enabled_aggregation_support : 1,

	// 24-31
	reserved_24 : 1,
	om_control_support : 1,
	ofdma_ra_support : 1,
	max_a_mpdu_length_exponent_ext : 2,
	a_msdu_fragmentation_support : 1,
	flexible_twt_schedule_support : 1,
	rx_control_frame_to_multibss : 1,

	// 32-38
	bsrp_bqrp_a_mpdu_aggregation : 1,
	qtp_support : 1,
	bqr_support : 1,
	srp_responder : 1,
	ndp_feedback_report_support : 1,
	ops_support : 1,
	a_msdu_in_a_mpdu_support : 1,

	// 39,40,41
	multi_tid_aggregation_tx_support : 3,

	// 42
	subchannel_selective_trans_support : 1,
	ul_2_996_tone_ru_support : 1,
	om_control_ul_mu_data_disable_rx_support : 1,
	reserved_45: 1,
	reserved_46: 1,
	reserved_47: 1;
#else
#error TODO big endian
#endif
} ;

struct __attribute__((__packed__)) IE_HE_PHY 
{
	// 0-7
	uint8_t reserved_0 : 1,
		ch40mhz_channel_2_4ghz : 1,
	ch40_and_80_mhz_5ghz : 1,
	ch160_mhz_5ghz : 1,
	ch160_80_plus_80_mhz_5ghz : 1,
	ch242_tone_rus_in_2_4ghz : 1,
	ch242_tone_rus_in_5ghz : 1,
	reserved_7 : 1;

#if __BYTE_ORDER == __LITTLE_ENDIAN
	// wireshark using uint16 for reasons I don't fully comprehend so when in
	// Rome...  
	// 8-23
	uint16_t punctured_preamble_rx : 4,
	 device_class : 1,
	 ldpc_coding_in_payload : 1,
	 he_su_ppdu_1x_he_ltf_08us : 1,
	 midamble_rx_max_nsts : 2,
	 ndp_with_4x_he_ltf_32us : 1,
	 stbc_tx_lt_80mhz : 1,
	 stbc_rx_lt_80mhz : 1,
	 doppler_tx : 1,
	 doppler_rx : 1,
	 full_bw_ul_mu_mimo : 1,
	 partial_bw_ul_mu_mimo : 1;

	// 24-39
	uint16_t dcm_max_constellation_tx : 2,
	 dcm_max_nss_tx : 1,
	 dcm_max_constellation_rx : 2,
	 dcm_max_nss_rx : 1,
	 rx_he_muppdu_from_non_ap : 1,
	 su_beamformer : 1,
	 su_beamformee : 1,
	 mu_beamformer : 1,
	 beamformer_sts_lte_80mhz : 3,
	 beamformer_sts_gt_80mhz : 3;

	// 40-55
	uint16_t number_of_sounding_dims_lte_80 : 3,
	 number_of_sounding_dims_gt_80 : 3,
	 ng_eq_16_su_fb : 1,
	 ng_eq_16_mu_fb : 1,
	 codebook_size_eq_4_2_fb : 1,
	 codebook_size_eq_7_5_fb : 1,
	 triggered_su_beamforming_fb : 1,
	 triggered_mu_beamforming_fb : 1,
	 triggered_cqi_fb : 1,
	 partial_bw_extended_range : 1,
	 partial_bw_dl_mu_mimo : 1,
	 ppe_threshold_present : 1;

	// 56-71
	uint16_t srp_based_sr_support : 1,
	 power_boost_factor_ar_support : 1,
	 he_su_ppdu_etc_gi : 1,
	 max_nc : 3,
	 stbc_tx_gt_80_mhz : 1,
	 stbc_rx_gt_80_mhz : 1,
	 he_er_su_ppdu_4xxx_gi : 1,
	 _20mhz_in_40mhz_24ghz_band : 1,
	 _20mhz_in_160_80p80_ppdu : 1,
	 _80mgz_in_160_80p80_ppdu : 1,
	 he_er_su_ppdu_1xxx_gi : 1,
	 midamble_rx_2x_xxx_ltf : 1,
	 dcm_max_bw : 2;

	// 72-87
	uint8_t longer_than_16_he_sigb_ofdm_symbol_support : 1,
	 non_triggered_cqi_feedback : 1,
	 tx_1024_qam_242_tone_ru_support : 1,
	 rx_1024_qam_242_tone_ru_support : 1,
	 rx_full_bw_su_using_he_muppdu_w_compressed_sigb : 1,
	 rx_full_bw_su_using_he_muppdu_w_non_compressed_sigb : 1,
	 nominal_packet_padding : 2,
	reserved_;
#else
# error TODO big endian
#endif
} ;

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
	const struct IE_HE_MAC* mac;
	const struct IE_HE_PHY* phy;

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

#ifdef __cplusplus
extern "C" {
#endif

int ie_he_capabilities_new(struct IE* ie);
void ie_he_capabilities_free(struct IE* ie);
int ie_he_operation_new(struct IE* ie);
void ie_he_operation_free(struct IE* ie);

const char* he_fragmentation_support_str(uint8_t val);
int he_max_frag_msdus_base_to_str(uint8_t max_frag_msdus_value, char* s, size_t len);
const char* he_min_fragment_size_str(uint8_t val);
const char* he_link_adapt_support_str(uint8_t w);
int he_mac_capa_to_str(const struct IE_HE_MAC* sie, unsigned int idx, char* s, size_t len);
int he_phy_capa_to_str(const struct IE_HE_PHY* sie, unsigned int idx, char* s, size_t len);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif

