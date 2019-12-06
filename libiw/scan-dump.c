#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <net/if.h>
#include <unicode/ustring.h>
#include <unicode/ustdio.h>

#include "core.h"
#include "iw.h"
#include "list.h"
#include "bss.h"
#include "hdump.h"

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
	// using the macros+strings from iw scan.c print_capabilities() but
	// slightly modified for my IE_Extended_Capabilities structure
#define CAPA(field,str)\
	if (sie->field) printf("\t\t* %s\n", str)

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

	if (bss_is_vht(bss) ) {
		printf("\t\t* Max Number Of MSDUs In A-MSDU is %d\n", sie->max_MSDU_in_AMSDU);
	}
}

static const UChar* get_ssid(const struct BSS* bss)
{
	const struct IE* ie = ie_list_find_id(&bss->ie_list, IE_SSID);
	const UChar* ssid = NULL;

	if (ie) {
		const struct IE_SSID *ie_ssid = IE_CAST(ie, const struct IE_SSID);
		if (ie_ssid->ssid_len) {
			ssid = ie_ssid->ssid;
//				hex_dump("ssid", ie->buf, ie->len);
		}
	}
	else {
		ERR("BSS %s has no SSID\n", bss->bssid_str);
	}

	// TODO check for weird-o SSIDs full of NULLs
	return ssid ? ssid : hidden;
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
	u_printf("\tSSID: %S\n", get_ssid(bss));
	print_supported_rates(bss);
	print_dsss_param(bss);
	print_country(bss);
	print_extended_capabilities(bss);
}

static void print_bss_to_csv(struct BSS* bss, bool header)
{
	if (header) {
		printf("BSSID,frequency,signal_strength,SSID\n");
	}

	printf("%s, %d, %0.2f, ", bss->bssid_str, bss->frequency, bss->signal_strength_mbm/100.0);
	u_printf("\"%S\"", get_ssid(bss));
	printf("\n");
}

static void print_short(const struct BSS* bss)
{
	//  SSID            BSSID              CHAN RATE  S:N   INT CAPS
	u_printf("%32S ", get_ssid(bss));
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

	bool first=true;
	list_for_each_entry(bss, &bss_list, node) {
//		print_bss_to_csv(bss, first);
		first = false;
	}

	printf("\n\n");
	list_for_each_entry(bss, &bss_list, node) {
		print_short(bss);
		first = false;
	}

leave:
	bss_free_list(&bss_list);
	nl_cb_put(cb);
	nl_socket_free(nl_sock);
	nlmsg_free(msg);
	return EXIT_SUCCESS;
}

