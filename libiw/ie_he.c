/* Uses chunks of Wireshark, specifically the strings from packet-ieee80211.c
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

	// I don't know officially how big this IE is (could be variable length
	// like Extended Capabilities) but let's assume it's a fixed size until I
	// know different
	if (ie->len < 26) {
		return -EINVAL;
	}

	CONSTRUCT(struct IE_HE_Capabilities)

	const uint8_t* ptr = ie->buf;
	// skip the Extension Id 
	ptr++;
	// point into the ie buf
	sie->mac_capa = ptr;
	ptr += 6; 
	sie->phy_capa = ptr;
	ptr += 9;
	sie->mcs_and_nss_set = ptr;
	ptr += 4;
	sie->ppe_threshold = ptr;

	sie->mac = (const struct IE_HE_MAC*)sie->mac_capa;
	sie->phy = (const struct IE_HE_PHY*)sie->phy_capa;

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

	if (ie->len != 10) {
		return -EINVAL;
	}

	CONSTRUCT(struct IE_HE_Operation)

	sie->params = ie->buf;
	sie->bss_color = ie->buf + 4;
	sie->mcs_and_nss_set = ie->buf + 5;

	sie->fields = (struct IE_HE_Operation_Fields*)ie->buf;

	return 0;
}

void ie_he_operation_free(struct IE* ie)
{
	INFO("%s\n", __func__);
	DESTRUCT(struct IE_HE_Operation);
}

// from packet-ieee80211.c function max_frag_msdus_base_custom()
int he_max_frag_msdus_base_to_str(uint8_t max_frag_msdus_value, char* s, size_t len)
{
  if (max_frag_msdus_value == 7)
    return snprintf(s, len, "No restriction");
  else
    return snprintf( s, len, "%u", 1 << max_frag_msdus_value);
}

static const char* he_mac_cap_str[] = {
	"+HTC HE Support", // htc_he_support
	"TWT Requester Support", // twt_req_support
	"TWT Responder Support", // twt_rsp_support
	"Fragmentation Support", // fragmentation_support
	NULL,
	"Maximum Number of Fragmented MSDUs", // max_frag_msdus
	NULL,
	NULL,

	"Minimum Fragment Size", // min_frag_size
	NULL,
	"Trigger Frame MAC Padding Duration", // trig_frm_mac_padding_dur
	NULL,
	"Multi-TID Aggregation Support", // multi_tid_agg_support
	NULL, 
	NULL,

	"HE Link Adaptation Support", // he_link_adaptation_support
	NULL,
	"All Ack Support", // all_ack_support
	"TRS Support", // trs_support
	"BSR Support", // bsr_support
	"Broadcast TWT Support", // broadcast_twt_support
	"32-bit BA Bitmap Support", // 32_bit_ba_bitmap_support
	"MU Cascading Support", // mu_cascading_support
	"Ack-Enabled Aggregation Support", // ack_enabled_agg_support

	"Reserved", // reserved_b24
	"OM Control Support", // om_control_support
	"OFDMA RA Support", // ofdma_ra_support
	"Maximum A-MPDU Length Exponent Extension", // max_a_mpdu_len_exp_ext
	NULL,
	"A-MSDU Fragmentation Support", // a_msdu_frag_support
	"Flexible TWT Schedule Support", // flexible_twt_sched_support
	"Rx Control Frame to MultiBSS", // rx_ctl_frm_multibss

	"BSRP BQRP A-MPDU Aggregation", // bsrp_bqrp_a_mpdu_agg
	"QTP Support", // qtp_support
	"BQR Support", // bqr_support
	"SRP Responder Role", // sr_responder
	"NDP Feedback Report Support", // ndp_feedback_report_support
	"OPS Support", // ops_support
	"A-MSDU in A-MPDU Support", // a_msdu_in_a_mpdu_support
	"Multi-TID Aggregation TX Support", // multi_tid_agg_support
	NULL,
	NULL,

	"HE Subchannel Selective Transmission Support", // subchannel_selective_xmit_support
	"UL 2x996-tone RU Support", // ul_2_996_tone_ru_support
	"OM Control UL MU Data Disable RX Support", // om_cntl_ul_mu_data_disable_rx_support
	"HE Dynamic SM Power Save", // he_dynamic_sm_power_save
	"Punctured Sounding Support", // he_punctured_sounding_support
	"HT And VHT Trigger Frame RX Support", // he_ht_and_vht_trigger_frame_rx_support
};

static const char* he_phy_cap_str[] = {
	// bits 0-7
	NULL,
	"40MHz in 2.4GHz band", // 40mhz_in_2_4ghz
	"40 & 80MHz in the 5GHz band", // 40_80_in_5ghz
	"160MHz in the 5GHz band", // 160_in_5ghz
	"160/80+80MHz in the 5GHz band", // 160_80_80_in_5ghz
	"242 tone RUs in the 2.4GHz band", // 242_tone_in_2_4ghz
	"242 tone RUs in the 5GHz band", // 242_tone_in_5ghz
	NULL,

	// bits 8-23
	"Punctured Preamble RX", // punc_preamble_rx 4-bits
	NULL, 
	NULL, 
	NULL, 
	"Device Class", // device_class
	"LDPC Coding In Payload", // ldpc_coding_in_payload
	"HE SU PPDU With 1x HE-LTF and 0.8us GI", // he_su_ppdu_with_1x_he_ltf_08us
	"Midamble Rx Max NSTS", // midamble_rx_max_nsts 2-bits
	NULL,
	"NDP With 4x HE-LTF and 3.2us GI", // 2us
	"STBC Tx <= 80 MHz", // stbc_tx_lt_80mhz
	"STBC Rx <= 80 MHz", // stbc_rx_lt_80mhz
	"Doppler Tx", // doppler_tx
	"Doppler Rx", // doppler_rx
	"Full Bandwidth UL MU-MIMO", // full_bw_ul_mu_mimo
	"Partial Bandwidth UL MU-MIMO", // partial_bw_ul_mu_mimo

	"DCM Max Constellation Tx", // dcm_max_const_tx 2-bits
	NULL,
	"DCM Max NSS Tx", // dcm_max_nss_tx
	"DCM Max Constellation Rx", // dcm_max_const_rx 2-bits
	NULL,
	"DCM Max NSS Rx", // dcm_max_nss_tx
	"Rx HE MU PPDU from Non-AP STA", // rx_he_mu_ppdu
	"SU Beamformer", // su_beamformer
	"SU Beamformee", // su_beamformee
	"MU Beamformer", // mu_beamformer
	"Beamformee STS <= 80 MHz", // beamformee_sts_lte_80mhz 3-bits
	NULL, 
	NULL,
	"Beamformee STS > 80 MHz", // beamformee_sts_gt_80mhz 3-bits
	NULL, 
	NULL,

	"Number Of Sounding Dimensions <= 80 MHz", // no_sounding_dims_lte_80 3-bits
	NULL, 
	NULL,
	"Number Of Sounding Dimensions > 80 MHz", // no_sounding_dims_gt_80 3-bits
	NULL, 
	NULL,
	"Ng = 16 SU Feedback", // ng_eq_16_su_fb
	"Ng = 16 MU Feedback", // ng_eq_16_mu_fb
	"Codebook Size SU Feedback", // codebook_size_su_fb
	"Codebook Size MU Feedback", // codebook_size_mu_fb
	"Triggered SU Beamforming Feedback", // trig_su_bf_fb
	"Triggered MU Beamforming Feedback", // trig_mu_bf_fb
	"Triggered CQI Feedback", // trig_cqi_fb
	"Partial Bandwidth Extended Range", // partial_bw_er
	"Partial Bandwidth DL MU-MIMO", // partial_bw_dl_mu_mimo
	"PPE Threshold Present", // ppe_thres_present

	"SRP-based SR Support", // srp_based_sr_sup
	"Power Boost Factor ar Support", // pwr_bst_factor_ar_sup
	"HE SU PPDU & HE MU PPDU w 4x HE-LTF & 0.8us GI", // he_su_ppdu_etc_gi
	"Max Nc", // max_nc 3-bits
	NULL, 
	NULL,
	"STBC Tx > 80 MHz", // stbc_tx_gt_80_mhz
	"STBC Rx > 80 MHz", // stbc_rx_gt_80_mhz
	"HE ER SU PPDU W 4x HE-LTF & 0.8us GI", // he_er_su_ppdu_4xxx_gi
	"20 MHz In 40 MHz HE PPDU In 2.4GHz Band", // 20_mhz_in_40_in_2_4ghz
	"20 MHz In 160/80+80 MHz HE PPDU", // 20_mhz_in_160_80p80_ppdu
	"80 MHz In 160/80+80 MHz HE PPDU", // 80_mhz_in_160_80p80_ppdu
	"HE ER SU PPDU W 1x HE-LTF & 0.8us GI", // he_er_su_ppdu_1xxx_gi
	"Midamble Rx 2x & 1x HE-LTF", // midamble_rx_2x_1x_he_ltf
	"DCM Max BW", // dcm_max_bw 2-bits
	NULL,

	"Longer Than 16 HE SIG-B OFDM Symbols Support", // longer_than_16_he_sigb_ofdm_sym_support
	"Non-Triggered CQI Feedback", // non_triggered_feedback
	"Tx 1024-QAM Support < 242-tone RU", // tx_1024_qam_support_lt_242_tone_ru
	"Rx 1024-QAM Support < 242-tone RU", // rx_1024_qam_support_lt_242_tone_ru
	"Rx Full BW SU Using HE MU PPDU With Compressed SIGB", // rx_full_bw_su_using_he_mu_ppdu_with_compressed_sigb
	"Rx Full BW SU Using HE MU PPDU With Non-Compressed SIGB", // rx_full_bw_su_using_he_mu_ppdu_with_non_compressed_sigb
	"Nominal Packet Padding", // nominal_packet_padding 2-bits
	NULL,
};

// packet-ieee80211.c array of same name
static const char* he_fragmentation_support_vals[] = {
  "No support for dynamic fragmentation",
  "Support for dynamic fragments in MPDUs or S-MPDUs",
  "Support for dynamic fragments in MPDUs and S-MPDUs and up to 1 dyn frag in MSDUs...",
  "Support for all types of dynamic fragments",
};

// from packet-ieee80211.c array of same name
static const char* he_link_adaptation_support_vals[] = {
  "No feedback if the STA does not provide HE MFB",
  "Reserved",
  "Unsolicited if the STA can receive and provide only unsolicited HE MFB",
  "Both",
};

// from packet-ieee80211.c array of same name
static const char* he_minimum_fragmentation_size_vals[] = {
  "No restriction on minimum payload size",
  "Minimum payload size of 128 bytes",
  "Minimum payload size of 256 bytes",
  "Minimum payload size of 512 bytes",
};

int he_mac_capa_to_str(const struct IE_HE_MAC* mac, unsigned int idx, char* s, size_t len)
{

	if (idx >= ARRAY_SIZE(he_mac_cap_str)){
		return -EINVAL;
	}

	if (!he_mac_cap_str[idx]) {
		return -ENOENT;
	}

	char tmpstr[32];
	size_t ret;

	switch (idx) {
		case 3: // 4
			// fragmentation support
			return snprintf(s, len, "%s: %s (%d)", 
					he_mac_cap_str[idx],
					he_fragmentation_support_vals[mac->fragmentation_support], 
					mac->fragmentation_support);

		case 5:  // 6 7
			// max frag msdu
			ret = he_max_frag_msdus_base_to_str(mac->max_number_fragmented_msdus, tmpstr, sizeof(tmpstr));
			XASSERT(ret<sizeof(tmpstr), ret);
			return snprintf(s, len, "%s: %s",
					he_mac_cap_str[idx], tmpstr);

		case 8: // 9
			return snprintf(s, len, "%s: %s (%d)", 
					he_mac_cap_str[idx],
					he_minimum_fragmentation_size_vals[mac->min_fragment_size],
					mac->min_fragment_size);

		case 10: // 11
			// min trigger frame mac
			return snprintf(s, len, "%s (%d)", 
					he_mac_cap_str[idx],
					mac->trigger_frame_mac_padding_dur);

		case 12: // 13 14
			// multi tid
			return snprintf(s, len, "%s: %d", 
					he_mac_cap_str[idx],
					mac->multi_tid_aggregation_support);

		case 15: // 16
			// he link adaptation
			return snprintf(s, len, "%s: %s (%d)",
					he_mac_cap_str[idx], 
					he_link_adaptation_support_vals[mac->he_link_adaptation_support],
					mac->he_link_adaptation_support);

		case 27: // 28
			// max ampdu len exponent exten
			return snprintf(s, len, "%s (%d)",
					he_mac_cap_str[idx], mac->max_a_mpdu_length_exponent_ext);

		case 39: // 40 41
			// multi-tid agg support
			return snprintf(s, len, "%s (%d)",
					he_mac_cap_str[idx], mac->multi_tid_aggregation_support);

		default:
			break;
	}
	return snprintf(s, len, "%s", he_mac_cap_str[idx]);
}

static const char* he_phy_device_class_vals[] = {
  "Class A Device",
  "Class B Device",
};

static const char* he_phy_midamble_rx_max_nsts_vals[] = {
  "1 Space-Time Stream",
  "2 Space-Time Streams",
  "3 Space-Time Streams",
  "4 Space-Time Streams",
};

static const char* he_phy_dcm_max_constellation_vals[] = {
  "DCM is not supported",
  "BPSK",
  "QPSK",
  "16-QAM",
};

static const char* he_phy_dcm_max_nss_vals[] = {
  "1 Space-Time Stream",
  "2 Space-Time Streams",
};

static const char* he_phy_nominal_packet_padding_vals[] = {
  "0 µs for all Constellations",
  "8 µs for all Constellations",
  "16 µs for all Constellations",
  "Reserved",
};

int he_phy_capa_to_str(const struct IE_HE_PHY* phy, unsigned int idx, char* s, size_t len)
{

	if (idx >= ARRAY_SIZE(he_phy_cap_str)){
		return -EINVAL;
	}

	if (!he_phy_cap_str[idx]) {
		return -ENOENT;
	}

	switch(idx)
	{
		case 8:
			return snprintf(s, len, "%s: %d",
					he_phy_cap_str[idx],
					phy->punctured_preamble_rx);

		case 12:
			return snprintf(s, len, "%s: %s (%d)",
					he_phy_cap_str[idx],
					he_phy_device_class_vals[phy->device_class],
					phy->device_class);

		case 15: // 16
			return snprintf(s, len, "%s: %s (%d)", 
					he_phy_cap_str[idx], 
					he_phy_midamble_rx_max_nsts_vals[phy->midamble_rx_max_nsts], 
					phy->midamble_rx_max_nsts);

		case 24: // 25
			return snprintf(s, len, "%s: %s (%d)", 
					he_phy_cap_str[idx], 
					he_phy_dcm_max_constellation_vals[phy->dcm_max_constellation_tx],
					phy->dcm_max_constellation_tx);

		case 26:
			return snprintf(s, len, "%s: %s (%d)", 
					he_phy_cap_str[idx], 
					he_phy_dcm_max_nss_vals[phy->dcm_max_nss_tx],
					phy->dcm_max_nss_tx);

		case 27: // 28
			return snprintf(s, len, "%s: %s (%d)", 
					he_phy_cap_str[idx], 
					he_phy_dcm_max_constellation_vals[phy->dcm_max_constellation_rx],
					phy->dcm_max_constellation_rx);

		case 29:
			return snprintf(s, len, "%s: %s (%d)", 
					he_phy_cap_str[idx], 
					he_phy_dcm_max_nss_vals[phy->dcm_max_nss_rx],
					phy->dcm_max_nss_rx);

		case 34: // 35 36
			return snprintf(s, len, "%s: %d",
					he_phy_cap_str[idx],
					phy->beamformer_sts_lte_80mhz);

		case 37: // 38 39
			return snprintf(s, len, "%s: %d",
					he_phy_cap_str[idx],
					phy->beamformer_sts_gt_80mhz);

		case 40: // 41 42
			return snprintf(s, len, "%s: %d",
					he_phy_cap_str[idx],
					phy->number_of_sounding_dims_lte_80);

		case 43: // 44 45
			return snprintf(s, len, "%s: %d",
					he_phy_cap_str[idx],
					phy->number_of_sounding_dims_gt_80);

		case 59: // 60 61
			return snprintf(s, len, "%s: %d",
					he_phy_cap_str[idx],
					phy->max_nc);

		case 70: // 71
			return snprintf(s, len, "%s: %d",
					he_phy_cap_str[idx],
					phy->dcm_max_bw);

		case 78: // 79
			return snprintf(s, len, "%s: %s (%d)", 
					he_phy_cap_str[idx], 
					he_phy_nominal_packet_padding_vals[phy->nominal_packet_padding],
					phy->nominal_packet_padding);

		default:
			break;
	}

	return snprintf(s, len, "%s", he_phy_cap_str[idx]);
}

static const char* he_operation_str[] = {
	"Default PE Duration",  // 3 bits
	NULL, NULL,
	"TWT Required",
	"TXOP Duration RTS Threshold", // 10 bits
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	"VHT Operation Information Present",
	"Co-located BSS",
	"ER SU Disable",
	// 7 bits reserved
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, 

	// bits 24-31
	"BSS Color", // 6 bits
	NULL, NULL, NULL, NULL, NULL, NULL, 
	"Partial BSS Color",
	"BSS Color Disabled",
};

int he_operation_to_str(const struct IE_HE_Operation* sie, unsigned int idx, char* s, size_t len)
{
	if (idx >= ARRAY_SIZE(he_operation_str)){
		return -EINVAL;
	}

	if (!he_operation_str[idx]) {
		return -ENOENT;
	}

	switch (idx) {
		// default PE duration
		case 0:
			return snprintf(s, len, "%s: %d", 
					he_operation_str[idx], 
					sie->fields->default_pe_duration);

		case 3:
			return snprintf(s, len, "%s: %s",
					he_operation_str[idx], 
					sie->fields->twt_required ? "Required" : "Not required" );

		case 4:
			return snprintf(s, len, "%s: %d", 
					he_operation_str[idx], 
					sie->fields->txop_duration_rts_thresh);

		case 24:
			// bss color
			return snprintf(s, len, "%s: 0x%02x", 
					he_operation_str[idx],
					sie->fields->bss_color);

		default:
			break;
	}

	return snprintf(s, len, "%s", he_operation_str[idx]);
}

