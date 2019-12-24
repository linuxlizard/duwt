#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <net/if.h>

// https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/
#include <unicode/utypes.h>
#include <unicode/ustring.h>
#include <unicode/utext.h>
#include <unicode/utf8.h>
#include <unicode/ustdio.h>

#include "core.h"
#include "iw.h"
#include "ie.h"
#include "list.h"
#include "bss.h"
#include "hdump.h"
#include "ie_print.h"

U_STRING_DECL(hidden, "<hidden>", 8);

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	ERR("%s\n", __func__);
	(void)nla;
	(void)err;
	(void)arg;

	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	DBG("%s\n", __func__);

	(void)msg;

	int *ret = (int *)arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	DBG("%s\n", __func__);
	(void)msg;

	int *ret = (int*)(arg);
	*ret = 0;
	return NL_STOP;
}


static int valid_handler(struct nl_msg *msg, void *arg)
{
	struct list_head* bss_list = (struct list_head*)arg;
	struct BSS* bss = NULL;

	INFO("%s\n", __func__);

	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(hdr);

	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	peek_nla_attr(tb_msg, NL80211_ATTR_MAX);

	int err=0;
	err = bss_from_nlattr(tb_msg, &bss);
	if (err) {
		goto fail;
	}

	list_add(&bss->node, bss_list);

	DBG("%s success\n", __func__);
	return NL_OK;
fail:
	return NL_SKIP;
}

static void print_supported_rates(struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_SUPPORTED_RATES);
	if (ie) {
		const struct IE_Supported_Rates *sie = IE_CAST(ie, struct IE_Supported_Rates);
		printf("\tSupported rates: ");
		for (size_t i=0 ; i<sie->count ; i++) {
			printf("%0.1f%s ", sie->rate[i], sie->basic[i]?"*":"");
		}
		printf("\n");
	}
}

static void print_dsss_param(struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_DSSS_PARAMETER_SET);
	if (ie) {
		printf("\tDS Parameter set: channel %d\n", ie->value);
	}
}

static void print_extended_capabilities(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_EXTENDED_CAPABILITIES);
	if (!ie) {
		return;
	}
	const struct IE_Extended_Capabilities* sie = IE_CAST(ie, const struct IE_Extended_Capabilities);
	printf("\tExtended capabilities:\n");

	// TODO rewrite this iterator style like RM_Enabled_Capabilities

	// using the macros+strings from iw scan.c print_capabilities() but
	// slightly modified for my IE_Extended_Capabilities structure
#define CAPA(field,str)\
	if (sie->field) printf("\t\t * %s\n", str)

	CAPA(bss_2040_coexist, "HT Information Exchange Supported");
	CAPA(ESS, "Extended Channel Switching");
	CAPA(wave_indication, "reserved (Wave Indication)");
	CAPA(psmp_capa, "PSMP Capability");
	CAPA(service_interval_granularity_flag, "reserved (Service Interval Granularity)");
	CAPA(spsmp_support, "S-PSMP Capability");
	CAPA(event, "Event");

	CAPA(diagnostics, "Diagnostics");
	CAPA(multicast_diagnostics, "Multicast Diagnostics");
	CAPA(location_tracking, "Location Tracking");
	CAPA(FMS, "FMS");
	CAPA(proxy_arp, "Proxy ARP Service");
	CAPA(collocated_interference_reporting, "Collocated Interference Reporting");
	CAPA(civic_location, "Civic Location");
	CAPA(geospatial_location, "Geospatial Location");

	CAPA(TFS, "TFS");
	CAPA(WNM_sleep_mode, "WNM-Sleep Mode");
	CAPA(TIM_broadcast, "TIM Broadcast");
	CAPA(BSS_transition, "BSS Transition");
	CAPA(QoS_traffic_capa, "QoS Traffic Capability");
	CAPA(AC_station_count, "AC Station Count");
	CAPA(multiple_BSSID, "Multiple BSSID");
	CAPA(timing_measurement, "Timing Measurement");

	CAPA(channel_usage, "Channel Usage");
	CAPA(SSID_list, "SSID List");
	CAPA(DMS, "DMS");
	CAPA(UTC_TSF_offset, "UTC TSF Offset");
	CAPA(TPU_buffer_STA_support, "TDLS Peer U-APSD Buffer STA Support");
	CAPA(TDLS_peer_PSM_support, "TDLS Peer PSM Support");
	CAPA(TDLS_channel_switch_prohibited, "TDLS channel switching");
	CAPA(internetworking, "Interworking");

	CAPA(QoS_map, "QoS Map");
	CAPA(EBR, "EBR");
	CAPA(SSPN_interface, "SSPN Interface");
	CAPA(MSGCF_capa, "MSGCF Capability");
	CAPA(TDLS_support, "TDLS Support");
	CAPA(TDLS_prohibited, "TDLS Prohibited");
	CAPA(TDLS_channel_switch_prohibited, "TDLS Channel Switching Prohibited");

	CAPA(reject_unadmitted_frame, "Reject Unadmitted Frame");
	CAPA(identifier_location, "Identifier Location");
	CAPA(UAPSD_coexist, "U-APSD Coexistence");
	CAPA(WNM_notification, "WNM-Notification");
	CAPA(QAB_capa, "QAB Capability");

	CAPA(UTF8_ssid, "UTF-8 SSID");
	CAPA(QMF_activated, "QMFActivated");
	CAPA(QMF_reconfig_activated, "QMFReconfigurationActivated");
	CAPA(robust_av_streaming, "Robust AV Streaming");
	CAPA(advanced_GCR, "Advanced GCR");
	CAPA(mesh_GCR, "Mesh GCR");
	CAPA(SCS, "SCS");
	CAPA(q_load_report, "QLoad Report");

	CAPA(alternate_EDCA, "Alternate EDCA");
	CAPA(unprot_TXOP_negotiation, "Unprotected TXOP Negotiation");
	CAPA(prot_TXOP_negotiation, "Protected TXOP egotiation");
	CAPA(prot_q_load_report, "Protected QLoad Report");
	CAPA(TDLS_wider_bandwidth, "TDLS Wider Bandwidth");
	CAPA(operating_mode_notification, "Operating Mode Notification");

	CAPA(channel_mgmt_sched, "Channel Schedule Management");
	CAPA(geo_db_inband, "Geodatabase Inband Enabling Signal");
	CAPA(network_channel_control, "Network Channel Control");
	CAPA(whitespace_map, "White Space Map");
	CAPA(channel_avail_query, "Channel Availability Query");
	CAPA(FTM_responder, "FTM Responder");
	CAPA(FTM_initiator, "FTM Initiator");

	CAPA(extended_spectrum_mgmt, "Extended Spectrum Management Capable");
#undef CAPA
	if (bss_is_vht(bss) ) {
		printf("\t\t * Max Number Of MSDUs In A-MSDU is %s\n", 
				ht_max_amsdu_str(sie->max_MSDU_in_AMSDU));
	}
}

static void print_vht_capabilities(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_VHT_CAPABILITIES);
	if (!ie) {
		return;
	}
	const struct IE_VHT_Capabilities* sie = IE_CAST(ie, const struct IE_VHT_Capabilities);
	printf("\tVHT capabilities:\n");
	printf("\t\tVHT Capabilities (0x%"PRIx32"):\n", sie->info);
	printf("\t\t\tMax MPDU length: %s\n", vht_max_mpdu_length_str(sie->max_mpdu_length));
	printf("\t\t\tSupported Channel Width: %s\n", vht_supported_channel_width_str(sie->supported_channel_width));
#define PRINT_VHT_CAPA(field, str) \
	do { \
		if (sie->field) \
			printf("\t\t\t" str "\n"); \
	} while (0)
	PRINT_VHT_CAPA(rx_ldpc, "RX LDPC");
	PRINT_VHT_CAPA(short_gi_80, "short GI (80 MHz)");
	PRINT_VHT_CAPA(short_gi_160_8080, "short GI (160/80+80 MHz)");
	PRINT_VHT_CAPA(tx_stbc, "TX STBC");
	PRINT_VHT_CAPA(su_beamformer, "SU Beamformer");
	PRINT_VHT_CAPA(su_beamformee, "SU Beamformee");
	PRINT_VHT_CAPA(mu_beamformer, "MU Beamformer");
	PRINT_VHT_CAPA(mu_beamformee, "MU Beamformee");
	PRINT_VHT_CAPA(vht_txop_ps, "VHT TXOP PS");
	PRINT_VHT_CAPA(htc_vht, "+HTC-VHT");
	PRINT_VHT_CAPA(rx_antenna_pattern_consistency, "RX antenna pattern consistency");
	PRINT_VHT_CAPA(tx_antenna_pattern_consistency, "TX antenna pattern consistency");

#undef PRINT_VHT_CAPA

	// TODO
	printf("\t\tVHT RX MCS set:\n");
	printf("\t\tVHT RX highest supported: \n");
	printf("\t\tVHT TX MCS set:\n");
	printf("\t\tVHT TX highest supported:\n");
}

static void print_vht_operation(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_VHT_OPERATION);
	if (!ie) {
		return;
	}
	const struct IE_VHT_Operation* sie = IE_CAST(ie, const struct IE_VHT_Operation);
	printf("\tVHT operation:\n");
	printf("\t\t * channel width: %d (%s)\n", 
			sie->channel_width, vht_channel_width_str(sie->channel_width));
	printf("\t\t * center freq segment 1: %d\n", sie->channel_center_freq_segment_0);
	printf("\t\t * center freq segment 2: %d\n", sie->channel_center_freq_segment_1);
	printf("\t\t * VHT basic MCS set: 0x%"PRIx16"\n", sie->mcs_and_nss_set);

}


static void print_he_capabilities(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_ext_id(&bss->ie_list, IE_EXT_HE_CAPABILITIES);
	if (!ie) {
		return;
	}
	const struct IE_HE_Capabilities* sie = IE_CAST(ie, struct IE_HE_Capabilities);

	ie_print_he_capabilities(sie);
}

static void print_he_operation(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_ext_id(&bss->ie_list, IE_EXT_HE_OPERATION);
	if (!ie) {
		return;
	}
	printf("\tHE operation:\n");
}

static void print_rsn(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_RSN);
	if (!ie) {
		return;
	}
	char s[128];

	const struct IE_RSN* sie = IE_CAST(ie, const struct IE_RSN);
	printf("\tRSN:\t * Version: %d\n", sie->version);
	int ret = cipher_suite_to_str(sie->group_data, s, sizeof(s));
	XASSERT((size_t)ret<sizeof(s), ret);
	printf("\t\t * Group cipher: %s\n", s);

	printf("\t\t * Pairwise ciphers:");
	for (size_t i=0 ; i<sie->pairwise_cipher_count ; i++) {
		XASSERT((sie->pairwise) != NULL, i);
		ret = cipher_suite_to_str(&sie->pairwise[i], s, sizeof(s));
		XASSERT((size_t)ret<sizeof(s), ret);
		printf(" %s", s);
	}
	printf("\n");

	printf("\t\t * Authentication suites:");
	for (size_t i=0 ; i<sie->akm_suite_count; i++) {
		XASSERT((sie->akm_suite) != NULL, i);
		ret = auth_to_str(&sie->akm_suite[i], s, sizeof(s));
		XASSERT((size_t)ret<sizeof(s), ret);
		printf(" %s", s);
	}
	printf("\n");

	ret = rsn_capabilities_to_str(sie, s, sizeof(s));
	XASSERT((size_t)ret<sizeof(s), ret);
	printf("\t\t * Capabilities:%s", s);
	printf(" (%#06x)\n", sie->capabilities);
}

static void print_rm_enabled_capabilities(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_RM_ENABLED_CAPABILITIES);
	if (!ie) {
		return;
	}
	const struct IE_RM_Enabled_Capabilities* sie = IE_CAST(ie, struct IE_RM_Enabled_Capabilities);
	printf("\tRadio Measurement Capabilities:\n");
	char s[128];
	int ret;
	for (size_t i=0 ; ; i++) {
		// I feel like cheating and not looking at each individual structure field
		unsigned int byte = i/8;
		unsigned int bit = i%8;
		if (ie->buf[byte] & (1<<bit) || i==18 || i==21 || i==24) {
			ret = rm_enabled_capa_to_str(sie, i, s, sizeof(s));
			if (ret == -ENOENT) continue;  // no value for this bit
			if (ret == -EINVAL) break; // end of list
			XASSERT((size_t)ret < sizeof(s), ret);
			printf("\t\t * %s\n", s);
		}
	}

}

static void print_vendor(const struct BSS* bss)
{
	const struct IE* ie_list[64];

	ssize_t ret = ie_list_get_all(&bss->ie_list, IE_VENDOR, ie_list, 64);
	XASSERT(ret > 0, ret);

	for( ssize_t i=0 ; i<ret ; i++ ) {

		const struct IE* ie = ie_list[i];

		const struct IE_Vendor* vie = IE_CAST(ie, struct IE_Vendor);

		uint8_t* ptr = ie->buf + 3;
		uint8_t* endptr = ie->buf + ie->len;

		printf("\tVendor specific: OUI %02x:%02x:%02x, data",
				vie->oui[0],
				vie->oui[1],
				vie->oui[2]);

		while (ptr < endptr) {
			printf(" %02x", *ptr++);
		}

		printf("\n");
	}

}

static void print_bss_load(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_BSS_LOAD);
	if (!ie) {
		return;
	}
	const struct IE_BSS_Load* sie = IE_CAST(ie, const struct IE_BSS_Load);
	printf("\tBSS Load:\n");
	printf("\t\t * station count: %d\n", sie->station_count);
	printf("\t\t * Channel utilization: %d/255\n", sie->channel_utilization);
	printf("\t\t * available admission capacity: %d [*32us]\n", sie->available_capacity);
}

// iw util.c print_ht_mcs_index()
static void print_ht_mcs_index(const struct HT_MCS_Set* mcs)
{
	// TODO
	hex_dump("mcs", mcs->mcs_bits, 10);
}

// iw util.c print_ht_mcs()
static void print_ht_mcs(const struct HT_MCS_Set* mcs)
{
	if (mcs->max_rx_supp_data_rate)
		printf("\t\tHT Max RX data rate: %d Mbps\n", mcs->max_rx_supp_data_rate);
	/* XXX: else see 9.6.0e.5.3 how to get this I think */

	if (mcs->tx_mcs_set_defined) {
		if (mcs->tx_mcs_set_equal) {
			printf("\t\tHT TX/RX MCS rate indexes supported:");
			print_ht_mcs_index(mcs);
		} else {
			printf("\t\tHT RX MCS rate indexes supported:");
			print_ht_mcs_index(mcs);

			if (mcs->tx_unequal_modulation)
				printf("\t\tTX unequal modulation supported\n");
			else
				printf("\t\tTX unequal modulation not supported\n");

			printf("\t\tHT TX Max spatial streams: %d\n",
				mcs->tx_max_num_spatial_streams);

			printf("\t\tHT TX MCS rate indexes supported may differ\n");
		}
	} else {
		printf("\t\tHT RX MCS rate indexes supported:");
		print_ht_mcs_index(mcs);
		printf("\t\tHT TX MCS rate indexes are undefined\n");
	}
}

static void print_power_constraint(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_POWER_CONSTRAINT);
	if (!ie) {
		return;
	}
	printf("\tPower constraint: %d dB\n", ie->value);
}

static void print_ht_capabilities(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_HT_CAPABILITIES);
	if (!ie) {
		return;
	}
	const struct IE_HT_Capabilities* sie = IE_CAST(ie, const struct IE_HT_Capabilities);
	printf("\tHT capabilities:\n");
	// iw util.c print_ht_capability
#define PRINT_HT_CAP(_cond, _str) \
	do { \
		if (_cond) \
			printf("\t\t\t" _str "\n"); \
	} while (0)

	printf("\t\tCapabilities: 0x%02x\n", sie->capa);

	PRINT_HT_CAP(sie->LDPC_coding_capa, "RX LDPC");
	PRINT_HT_CAP(sie->supported_channel_width, "HT20/HT40");
	PRINT_HT_CAP(!sie->supported_channel_width, "HT20");

	PRINT_HT_CAP(sie->SM_power_save == 0, "Static SM Power Save");
	PRINT_HT_CAP(sie->SM_power_save == 1, "Dynamic SM Power Save");
	PRINT_HT_CAP(sie->SM_power_save == 3, "SM Power Save disabled");

	PRINT_HT_CAP(sie->greenfield, "RX Greenfield");
	PRINT_HT_CAP(sie->short_gi_20Mhz, "RX HT20 SGI");
	PRINT_HT_CAP(sie->short_gi_40Mhz, "RX HT40 SGI");
	PRINT_HT_CAP(sie->tx_stbc, "TX STBC");

	PRINT_HT_CAP(sie->rx_stbc == 0, "No RX STBC");
	PRINT_HT_CAP(sie->rx_stbc == 1, "RX STBC 1-stream");
	PRINT_HT_CAP(sie->rx_stbc == 2, "RX STBC 2-streams");
	PRINT_HT_CAP(sie->rx_stbc == 3, "RX STBC 3-streams");

	PRINT_HT_CAP(sie->delayed_block_ack, "HT Delayed Block Ack");

	PRINT_HT_CAP(!(sie->max_amsdu_len), "Max AMSDU length: 3839 bytes");
	PRINT_HT_CAP(sie->max_amsdu_len, "Max AMSDU length: 7935 bytes");

	/*
	 * For beacons and probe response this would mean the BSS
	 * does or does not allow the usage of DSSS/CCK HT40.
	 * Otherwise it means the STA does or does not use
	 * DSSS/CCK HT40.
	 */
	PRINT_HT_CAP(sie->dsss_cck_in_40Mhz, "DSSS/CCK HT40");
	PRINT_HT_CAP(!sie->dsss_cck_in_40Mhz, "No DSSS/CCK HT40");
	/* BIT(13) is reserved */
	PRINT_HT_CAP(sie->_40Mhz_intolerant, "40 MHz Intolerant");
	PRINT_HT_CAP(sie->lsig_txop_prot, "L-SIG TXOP protection");
#undef PRINT_HT_CAP

	char s[128];
	int ret = ht_ampdu_length_to_str(sie->max_ampdu_len, s, sizeof(s));
	XASSERT((size_t)ret<sizeof(s), ret);
	printf("\t\t%s\n", s);

	ret = ht_ampdu_spacing_to_str(sie->min_ampdu_spacing, s, sizeof(s));
	XASSERT((size_t)ret<sizeof(s), ret);
	printf("\t\t%s\n", s);

	print_ht_mcs(&sie->mcs);
}

static void print_ht_operation(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_HT_OPERATION);
	if (!ie) {
		return;
	}
	const struct IE_HT_Operation* sie = IE_CAST(ie, const struct IE_HT_Operation);
	printf("\tHT operation:\n");
	printf("\t\t * primary channel: %d\n", sie->primary_channel);
	printf("\t\t * secondary channel offset: %s\n", 
			ht_secondary_offset_str(sie->secondary_channel_offset)); 
	printf("\t\t * STA channel width: %s\n", 
			ht_sta_channel_width_str(sie->sta_channel_width));
	printf("\t\t * RIFS: %d\n", sie->rifs);
	printf("\t\t * HT protection: %s\n", 
			ht_protection_str(sie->ht_protection)); 
	printf("\t\t * non-GF present: %d\n", sie->non_gf_present);
//	printf("\t\t * Channel Center Freq 2: %d\n", sie->channel_center_frequency_2);
	printf("\t\t * OBSS non-GF present: %d\n", sie->obss_non_gf_present);
	printf("\t\t * dual beacon: %d\n", sie->dual_beacon);
	printf("\t\t * dual CTS protection: %d\n", sie->dual_cts_protection);
	printf("\t\t * STBC beacon: %d\n", sie->stbc_beacon);
	printf("\t\t * L-SIG TXOP Prot: %d\n", sie->lsig_txop_prot);
	printf("\t\t * PCO active: %d\n", sie->pco_active);
	printf("\t\t * PCO phase: %d\n", sie->pco_phase);

}

static int ssid_to_unicode_str(const struct BSS* bss, UChar ssid[], size_t len )
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_SSID);

	if (!ie) {
		return -ENOENT;
	}

	// return 0 to indicate a hidden SSID
	if (ie->len == 0 || ie->buf[0] == 0) {
		return 0;
	}

	int32_t ssid_len = len;

	// http://userguide.icu-project.org/strings
	// http://userguide.icu-project.org/strings/utf-8
//	UChar ssid[SSID_MAX_LEN*2];
	UErrorCode status = U_ZERO_ERROR;
	u_strFromUTF8(ssid, len, &ssid_len, (const char*)ie->buf, ie->len, &status);
	if ( !U_SUCCESS(status)) {
		ERR("%s unicode parse fail status=%d\n", __func__, status);
		return -EINVAL;
	}

	// TODO check for weird-o SSIDs full of NULLs
	return ssid_len;
}

static void print_country(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_COUNTRY);
	if (!ie) {
		return;
	}

	XASSERT(ie->specific != NULL, ie->id);
	const struct IE_Country* cie = IE_CAST(ie, const struct IE_Country);

	printf("\tCountry: %s\tEnvironment: %s\n", cie->country, country_env_str(cie->environment));

	// loop inspired by / leveraged from iw scan.c
	for (size_t i=0 ; i<cie->count ; i++) {
		int end_channel;
		const union ieee80211_country_ie_triplet *triplet = &cie->triplets[i];

		if (triplet->ext.reg_extension_id >= IEEE80211_COUNTRY_EXTENSION_ID) {
			printf("\t\tExtension ID: %d Regulatory Class: %d Coverage class: %d (up to %dm)\n",
			       triplet->ext.reg_extension_id,
			       triplet->ext.reg_class,
			       triplet->ext.coverage_class,
			       triplet->ext.coverage_class * 450);
			continue;
		}

		/* 2 GHz */
		if (triplet->chans.first_channel <= 14)
			end_channel = triplet->chans.first_channel + (triplet->chans.num_channels - 1);
		else
			end_channel =  triplet->chans.first_channel + (4 * (triplet->chans.num_channels - 1));

		printf("\t\tChannels [%d - %d] @ %d dBm\n", triplet->chans.first_channel, end_channel, triplet->chans.max_power);
	}

}

static void print_tpc_report(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_TPC_REPORT);
	if (!ie) {
		return;
	}
	const struct IE_TPC_Report* sie = IE_CAST(ie, struct IE_TPC_Report);

	printf("\tTPC report: TX power: %d dBm\n", sie->tx_power);
}


static void print_erp(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_ERP);
	if (!ie) {
		return;
	}
	const struct IE_ERP* sie = IE_CAST(ie, struct IE_ERP);
	char s[128];
	int ret = erp_to_str(sie, s, sizeof(s));
	XASSERT((size_t)ret<sizeof(s), ret);
	printf("\tERP:%s\n", s);
}

static void print_extended_supported_rates(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_EXTENDED_SUPPORTED_RATES);
	if (!ie) {
		return;
	}
	const struct IE_Extended_Supported_Rates *sie = IE_CAST(ie, struct IE_Extended_Supported_Rates);
	printf("\tExtended supported rates: ");
	for (size_t i=0 ; i<sie->count ; i++) {
		printf("%0.1f%s ", sie->rates[i].rate, sie->rates[i].basic?"*":"");
	}
	printf("\n");
}

static void print_ssid(struct BSS* bss)
{
	UChar ssid[SSID_MAX_LEN*2];
	int ret = ssid_to_unicode_str(bss, ssid, sizeof(ssid));
	XASSERT(ret>=0, ret);

	if (ret==0) {
		u_printf("%32S ", hidden);
	}
	else {
		u_printf("%32S ", ssid);
	}
}

static void print_bss(struct BSS* bss)
{
	XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);

	printf("BSS %s\n", bss->bssid_str);

	char str[128];
	int err = tsf_to_timestamp_str(bss->tsf, str, sizeof(str));
	XASSERT(err < 0 || (size_t)err < sizeof(str), err);

	printf("\tTSF: %" PRIu64 " usec (%s)\n", bss->tsf, str);
	printf("\tfreq: %"PRIu32"\n", bss->frequency);
	printf("\tbeacon interval: %d TUs\n", bss->beacon_interval);

	err = capability_to_str(bss->capability, str, sizeof(str));
	XASSERT(err < 0 || (size_t)err < sizeof(str), err);
	printf("\tcapability: %s (%#06x)\n", str, bss->capability);

	printf("\tsignal: %0.2f dBm\n", bss->signal_strength_mbm/100.0);
	print_ssid(bss);
	print_supported_rates(bss);
	print_dsss_param(bss);
	print_country(bss);
	print_bss_load(bss);
	print_tpc_report(bss);
	print_erp(bss);
	print_extended_supported_rates(bss);
	print_power_constraint(bss);
	print_ht_capabilities(bss);
	print_ht_operation(bss);
	print_extended_capabilities(bss);
	print_vht_capabilities(bss);
	print_vht_operation(bss);
	print_he_capabilities(bss);
	print_he_operation(bss);
	print_rsn(bss);
	print_rm_enabled_capabilities(bss);
	print_vendor(bss);
}

static void print_bss_to_csv(struct BSS* bss, bool header)
{
	if (header) {
		printf("BSSID,frequency,signal_strength,SSID\n");
	}

	UChar ssid[SSID_MAX_LEN*2];
	int ret = ssid_to_unicode_str(bss, ssid, sizeof(ssid));
	XASSERT(ret>=0, ret);

	printf("%s, %d, %0.2f, ", bss->bssid_str, bss->frequency, bss->signal_strength_mbm/100.0);
	if (ret==0) {
		u_printf("%32S ", hidden);
	}
	else {
		u_printf("%32S ", ssid);
	}
	printf("\n");
}

static void print_short(const struct BSS* bss)
{
	UChar ssid[SSID_MAX_LEN*2];
	int ret = ssid_to_unicode_str(bss, ssid, sizeof(ssid));
	XASSERT(ret>=0, ret);

	//  SSID            BSSID              CHAN RATE  S:N   INT CAPS
	if (ret==0) {
		u_printf("%32S ", hidden);
	}
	else {
		u_printf("%32S ", ssid);
	}

	printf("%18s  %d  %d  %0.2f ", bss->bssid_str, bss->frequency, -1, bss->signal_strength_mbm/100.0);
//	for( size_t i=0 ; i<bss->ie_list.count ; i++) {
//		printf("%3d ", bss->ie_list.ieptrlist[i]->id);
//	}
//	printf("\n");

	const struct IE* ie= (void*)-1;
	ie_list_for_each_entry(ie, bss->ie_list) {
//		printf("%zu %p\n", i, (void*)ie);
//		printf("%zu %p\n", i, bss->ie_list.ieptrlist[i]);
		XASSERT(ie->cookie == IE_COOKIE, ie->cookie);
		printf("%3d ", ie->id);
	}
	printf("\n");

}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s ifname\n", argv[0]);
		exit(1);
	}

	const char* ifname = argv[1];

	LIST_HEAD(bss_list);
	struct nl_cb* cb = nl_cb_alloc(NL_CB_DEFAULT);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, (void*)&bss_list);
	int cb_err = 1;
	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &cb_err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &cb_err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &cb_err);

	struct nl_sock* nl_sock = nl_socket_alloc_cb(cb);
	int err = genl_connect(nl_sock);

	int nl80211_id = genl_ctrl_resolve(nl_sock, NL80211_GENL_NAME);

	int ifidx = if_nametoindex(ifname);

	struct nl_msg* msg = nlmsg_alloc();

	genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, nl80211_id, 0, 
						NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);

	nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifidx);
	err = nl_send_auto(nl_sock, msg);
	if (err < 0) {
		ERR("nl_send_auto failed err=%d\n", err);
		goto leave;
	}

	while (cb_err > 0) {
		err = nl_recvmsgs(nl_sock, cb);
		INFO("nl_recvmsgs err=%d\n", err);
	}

	U_STRING_INIT(hidden, "<hidden>", 8);
	struct BSS* bss;
	list_for_each_entry(bss, &bss_list, node) {
		print_bss(bss);
	}

//	bool first=true;
//	list_for_each_entry(bss, &bss_list, node) {
//		print_bss_to_csv(bss, first);
//		first = false;
//	}

	printf("\n\n");
	list_for_each_entry(bss, &bss_list, node) {
		print_short(bss);
	}

leave:
	bss_free_list(&bss_list);
	nl_cb_put(cb);
	nl_socket_free(nl_sock);
	nlmsg_free(msg);
	return EXIT_SUCCESS;
}

