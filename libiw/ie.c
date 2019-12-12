#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <unicode/utypes.h>
#include <unicode/ustdio.h>

#define HAVE_MODULE_LOGLEVEL 1

#include "core.h"
#include "ie.h"

const uint8_t ms_oui[3] = { 0x00, 0x50, 0xf2 };
const uint8_t ieee80211_oui[3] = { 0x00, 0x0f, 0xac };
const uint8_t wfa_oui[3] = { 0x50, 0x6f, 0x9a };

#define IE_LIST_DEFAULT_MAX 32

#define CONSTRUCT(type)\
	type* sie;\
	sie = calloc(1,sizeof(type));\
	if (!sie) {\
		return -ENOMEM;\
	}\
	sie->cookie = IE_SPECIFIC_COOKIE;\
	ie->specific = sie;\
	sie->base = ie;\

#define DESTRUCT(type)\
	type* sie = (type*)ie->specific;\
	XASSERT(sie->cookie == IE_SPECIFIC_COOKIE, sie->cookie);\
	memset(sie, 0, sizeof(*sie));\
	PTR_FREE(ie->specific);

#ifdef HAVE_MODULE_LOGLEVEL
static int module_loglevel=LOG_LEVEL_INFO;
//static int module_loglevel=LOG_LEVEL_DEBUG;
#endif

static int ie_ssid_new(struct IE* ie)
{
	CONSTRUCT(struct IE_SSID)

	UErrorCode status = U_ZERO_ERROR;
	u_strFromUTF8( sie->ssid, sizeof(sie->ssid), &sie->ssid_len, ie->buf, ie->len, &status);
	if ( !U_SUCCESS(status)) {
		PTR_FREE(sie);
		ERR("%s unicode parse fail status=%d\n", __func__, status);
		return -EINVAL;
	}

	if (sie->ssid_len > SSID_MAX_LEN) {
		WARN("ssid too long len=%d\n", sie->ssid_len);
	}

	return 0;
}

static void ie_ssid_free(struct IE* ie)
{
	struct IE_SSID* sie = (struct IE_SSID*)ie->specific;

	XASSERT(sie->cookie == IE_SPECIFIC_COOKIE, sie->cookie);

	memset(sie, POISON, sizeof(*sie));
	PTR_FREE(ie->specific);
}

static void decode_supported_rate(uint8_t byte, struct Supported_Rate *psr)
{
// iw scan.c print_supprates()
#define BSS_MEMBERSHIP_SELECTOR_VHT_PHY 126
#define BSS_MEMBERSHIP_SELECTOR_HT_PHY 127
	int r = byte & 0x7f;

	memset(psr, 0, sizeof(struct Supported_Rate));

	/* davep 20191201 ; TODO what the heck do I do with VHT_ HT_PHY!? */
	if (r == BSS_MEMBERSHIP_SELECTOR_VHT_PHY && byte & 0x80)
		(void)r;
//			s += "VHT";
	else if (r == BSS_MEMBERSHIP_SELECTOR_HT_PHY && byte & 0x80)
		(void)r;
//			s += "HT";
	else {
		psr->rate = r/2.0 + 5.0*(r&1);
		psr->basic =  true && (byte & 0x80) ;
	}
}

static int ie_supported_rates_new(struct IE* ie)
{
	CONSTRUCT(struct IE_Supported_Rates)

	// iw scan.c print_supprates()
	// "bits 6 to 0 are set to the data rate ... in units of 500kb/s"
	for (size_t i = 0; i < ie->len ; i++) {
		uint8_t byte = ie->buf[i];
		int r = byte & 0x7f;

		/* davep 20191201 ; TODO what the heck do I do with VHT_ HT_PHY!? */
		if (r == BSS_MEMBERSHIP_SELECTOR_VHT_PHY && byte & 0x80)
			(void)r;
//			s += "VHT";
		else if (r == BSS_MEMBERSHIP_SELECTOR_HT_PHY && byte & 0x80)
			(void)r;
//			s += "HT";
		else {
			sie->rate[sie->count] = r/2.0 + 5.0*(r&1);
			sie->basic[sie->count] =  true && (byte & 0x80) ;
			sie->count++;
		}
	}

	return 0;
}

static void ie_supported_rates_free(struct IE* ie)
{
	DESTRUCT(struct IE_Supported_Rates)
}

static int ie_tim_new(struct IE* ie)
{
	CONSTRUCT(struct IE_TIM)

	sie->dtim_count = ie->buf[0];
	sie->dtim_period = ie->buf[1];
	sie->control = ie->buf[2];
	// POINTER into already alloc'd buf
	sie->bitmap = &ie->buf[3];

	return 0;
}

static void ie_tim_free(struct IE* ie)
{
	DESTRUCT(struct IE_TIM)
}

static int ie_country_new(struct IE* ie)
{
	CONSTRUCT(struct IE_Country)

	int err = 0;
	uint8_t* ptr = ie->buf;
	uint8_t* endptr = ie->buf + ie->len;

//	hex_dump("country", ie->buf, ie->len);
	if (ie->len < 3) {
		err = -EINVAL;
		goto fail;
	}

	// iw scan.c print_country() 
	// http://www.ieee802.org/11/802.11mib.txt
	sie->country[0] = *ptr++;
	sie->country[1] = *ptr++;
	sie->country[2] = (char)0;

	sie->environment_byte = *ptr++;
	switch (sie->environment_byte) {
		case 'I':
			sie->environment = ENV_INDOOR_ONLY;
			break;
		case 'O':
			sie->environment = ENV_OUTDOOR_ONLY;
			break;
		case ' ':
			sie->environment = ENV_INDOOR_OUTDOOR;
			break;
		default:
			sie->environment = ENV_INVALID;
			break;
	}

	if (ptr >= endptr) {
		// no country codes
		return 0;
	}

	while (ptr < endptr-2) {
		if (sie->count+1 > IE_COUNTRY_TRIPLET_MAX) {
			err = -ENOMEM;
			goto fail;
		}

		sie->triplets[sie->count++] = *(union ieee80211_country_ie_triplet*)ptr;
//		DBG("%s %p %p %zu %d %d %d\n", __func__, (void*)ptr, (void*)endptr, sie->count,
//				sie->triplets[sie->count-1].chans.first_channel,
//				sie->triplets[sie->count-1].chans.num_channels,
//				sie->triplets[sie->count-1].chans.max_power);

		ptr += 3;
	}

	return 0;

fail:
	if (ie->specific) {
		PTR_FREE(ie->specific);
	}
	return err;
}

static void ie_country_free(struct IE* ie)
{
	DESTRUCT(struct IE_Country)
}

static int ie_erp_new(struct IE* ie)
{
	CONSTRUCT(struct IE_ERP)

	sie->NonERP_Present = !!(ie->buf[0] & 1);
	sie->Use_Protection = !!(ie->buf[0] & 2);
	sie->Barker_Preamble_Mode = !!(ie->buf[0] & 4);
	return 0;
}

static void ie_erp_free(struct IE* ie)
{
	DESTRUCT(struct IE_ERP)
}

// iw util.c print_ht_mcs()
static void ht_mcs_decode(uint8_t* buf, struct HT_MCS_Set* mcs)
{
	memset(mcs, 0, sizeof(struct HT_MCS_Set));
	memcpy(mcs->mcs_bits, buf, 10);

	mcs->max_rx_supp_data_rate = (buf[10] | ((buf[11] & 0x3) << 8));
	mcs->tx_mcs_set_defined = !!(buf[12] & (1 << 0));
	mcs->tx_mcs_set_equal = !(buf[12] & (1 << 1));
	mcs->tx_max_num_spatial_streams = ((buf[12] >> 2) & 3) + 1;
	mcs->tx_unequal_modulation = !!(buf[12] & (1 << 4));
}

static int ie_ht_capabilities_new(struct IE* ie)
{
	CONSTRUCT(struct IE_HT_Capabilities)

	uint8_t* ptr = ie->buf;

	// decode big blob into smaller blobs
	// Figure 9-331 HT Capabilities element format
	sie->capa = htole16(*(uint16_t*)ptr); 
	ptr += 2;
	sie->ampdu_param = *ptr;
	ptr += 1;
	// just point into the ie->buf (no need for more memory) 
	sie->supported_mcs_ptr = ptr;
	ptr += 16;
	sie->extended_capa = htole16(*(uint16_t*)ptr);
	ptr += 2;
	sie->tx_beamforming_capa = htole32(*(uint32_t*)ptr);
	ptr += 4;
	sie->asel_capa = *ptr;

	// decode bitfields

#define HTCAPA(field, bit)\
		sie->field = !!(sie->capa & (1<<bit))
	HTCAPA(LDPC_coding_capa, 0);
	HTCAPA(supported_channel_width, 1);
	sie->SM_power_save = (sie->capa >> 2) & 3;
	HTCAPA(greenfield, 4);
	HTCAPA(short_gi_20Mhz, 5);
	HTCAPA(short_gi_40Mhz, 6);
	HTCAPA(tx_stbc, 7);
	sie->rx_stbc = (sie->capa >> 8) & 3;
	HTCAPA(delayed_block_ack, 10);
	HTCAPA(max_amsdu_len, 11);
	HTCAPA(dsss_cck_in_40Mhz, 12);
	// bit 13 is reserved
	HTCAPA(_40Mhz_intolerant, 14);
	HTCAPA(lsig_txop_prot, 15);
#undef HTCAPA

	sie->max_ampdu_len = sie->ampdu_param & 3;
	sie->min_ampdu_spacing = (sie->ampdu_param >> 2) & 7;

	ht_mcs_decode(sie->supported_mcs_ptr, &sie->mcs);
	return 0;
}

static void ie_ht_capabilities_free(struct IE* ie)
{
	DESTRUCT(struct IE_HT_Capabilities)
}

static int ie_ht_operation_new(struct IE* ie)
{
	CONSTRUCT(struct IE_HT_Operation)

	uint8_t* ptr = ie->buf;
	hex_dump(__func__, ie->buf, ie->len);

	// iw scan.c print_ht_op()
	sie->primary_channel = *ptr++;
	sie->info_ptr = ptr; 
	ptr += 5;
	sie->mcs_set_ptr = ptr;

	// reset the ptr back to start of IE data so I can copy/paste the decode from 
	// iw scan.c print_ht_op()
	uint8_t* data = ie->buf;
	// primary_channel is data[0]
	sie->secondary_channel_offset = data[1] & 0x3;
	sie->sta_channel_width = (data[1] & 0x4)>>2;
	sie->rifs = (data[1] & 0x8)>>3;
	sie->ht_protection = data[2] & 0x3;
	sie->non_gf_present = (data[2] & 0x4) >> 2;
	sie->obss_non_gf_present = (data[2] & 0x10) >> 4;

	// docs say 11 bits but is actually 8 bits from bit-13 to bit-20
	// TODO
//	sie->channel_center_frequency_2 = (data[2]<<4) | (data[3]>>4);

	sie->dual_beacon = (data[4] & 0x40) >> 6;
	sie->dual_cts_protection = (data[4] & 0x80) >> 7;
	sie->stbc_beacon = data[5] & 0x1;
	sie->lsig_txop_prot = (data[5] & 0x4) >> 2;
	sie->pco_active = (data[5] & 0x4) >> 2;
	sie->pco_phase = (data[5] & 0x8) >> 3;

	return 0;
}

static void ie_ht_operation_free(struct IE* ie)
{
	DESTRUCT(struct IE_HT_Operation);
}

static int ie_extended_supported_rates_new(struct IE* ie)
{
	struct IE_Extended_Supported_Rates* sie;

	DBG("%s len=%zu\n", __func__, ie->len);
	sie = calloc(1, sizeof(struct IE_Extended_Supported_Rates) + (sizeof(struct Supported_Rate)*ie->len));
	if (!sie) {
		return -ENOMEM;
	}
	sie->cookie = IE_SPECIFIC_COOKIE;
	ie->specific = sie;
	sie->base = ie;

	for (size_t i=0 ; i<ie->len; i++ ) {
		decode_supported_rate(ie->buf[i], &sie->rates[i]);
	}
	sie->count = ie->len;

	return 0;
}

static void ie_extended_supported_rates_free(struct IE* ie)
{
	DESTRUCT(struct IE_Extended_Supported_Rates)
}

static int ie_dsss_parameter_set_new(struct IE* ie)
{
	// store the DSSS Param value in the IE itself, no
	// need to create a "child class"
	ie->value = (int)ie->buf[0];
	return 0;
}

static int ie_extended_capa_new(struct IE* ie)
{
	CONSTRUCT(struct IE_Extended_Capabilities)

//	hex_dump(__func__, ie->buf, ie->len);

	if (ie->len < 1) return 0;
	size_t idx = 0;
	uint8_t b = ie->buf[0];
#define NEXTBYTE\
	if (ie->len <= idx) return 0;\
	b = ie->buf[idx++];
#define ECBIT(field,bit) sie->field = !!(b & (1<<bit))

	NEXTBYTE
	ECBIT(bss_2040_coexist, 0);
	// bit 1 reserved
	ECBIT(ESS, 2);
	ECBIT(wave_indication, 3);
	ECBIT(psmp_capa, 4);
	ECBIT(service_interval_granularity_flag, 5);
	ECBIT(spsmp_support, 6);
	ECBIT(event, 7);

	NEXTBYTE
	ECBIT(diagnostics, 0);
	ECBIT(multicast_diagnostics, 1);
	ECBIT(location_tracking, 2);
	ECBIT(FMS, 3);
	ECBIT(proxy_arp, 4);
	ECBIT(collocated_interference_reporting, 5);
	ECBIT(civic_location, 6);
	ECBIT(geospatial_location, 7);

	NEXTBYTE
	ECBIT(TFS, 0);
	ECBIT(WNM_sleep_mode, 1);
	ECBIT(TIM_broadcast, 2);
	ECBIT(BSS_transition, 3);
	ECBIT(QoS_traffic_capa, 4);
	ECBIT(AC_station_count, 5);
	ECBIT(multiple_BSSID, 6);
	ECBIT(timing_measurement, 7);

	NEXTBYTE
	ECBIT(channel_usage, 0);
	ECBIT(SSID_list, 1);
	ECBIT(DMS, 2);
	ECBIT(UTC_TSF_offset, 3);
	ECBIT(TPU_buffer_STA_support, 4);
	ECBIT(TDLS_peer_PSM_support, 5);
	ECBIT(TDLS_channel_switching, 6);
	ECBIT(internetworking, 7);

	NEXTBYTE
	ECBIT(QoS_map, 0);
	ECBIT(EBR, 1);
	ECBIT(SSPN_interface, 2);
	// byte 4 bit 3 reserved
	ECBIT(MSGCF_capa, 4);
	ECBIT(TDLS_support, 5);
	ECBIT(TDLS_prohibited, 6);
	ECBIT(TDLS_channel_switch_prohibited, 7);

	NEXTBYTE
	ECBIT(reject_unadmitted_frame, 0);
	sie->service_interval_granularity = (b & 7)>>1;
	ECBIT(identifier_location, 4);
	ECBIT(UAPSD_coexist, 5);
	ECBIT(WNM_notification, 6);
	ECBIT(QAB_capa, 7);

	NEXTBYTE
	ECBIT(UTF8_ssid, 0);
	ECBIT(QMF_activated, 1);
	ECBIT(QMF_reconfig_activated, 2);
	ECBIT(robust_av_streaming, 3);
	ECBIT(advanced_GCR, 4);
	ECBIT(mesh_GCR, 5);
	ECBIT(SCS, 6);
	ECBIT(q_load_report, 7);

	NEXTBYTE
	ECBIT(alternate_EDCA, 0);
	ECBIT(unprot_TXOP_negotiation, 1);
	ECBIT(prot_TXOP_negotiation, 2);
	// byte 7 bit 3 reserved
	ECBIT(prot_q_load_report, 4);
	ECBIT(TDLS_wider_bandwidth, 5);
	ECBIT(operating_mode_notification, 6);

	sie->max_MSDU_in_AMSDU = ((ie->buf[7] & (1<<7))>>7) + (ie->buf[8] & 1);

	NEXTBYTE
	ECBIT(channel_mgmt_sched, 0);
	ECBIT(geo_db_inband, 0);
	ECBIT(network_channel_control, 0);
	ECBIT(whitespace_map, 0);
	ECBIT(channel_avail_query, 0);
	ECBIT(FTM_responder, 0);
	ECBIT(FTM_initiator, 0);

	NEXTBYTE
	// byte 9 bit 0 reserved
	ECBIT(extended_spectrum_mgmt, 1);
	ECBIT(future_channel_guidance, 2);

#undef ECBIT
#undef NEXTBYTE
	return 0;
}

static void ie_extended_capa_free(struct IE* ie)
{
	DESTRUCT(struct IE_Extended_Capabilities)
}

static int ie_vendor_new(struct IE* ie)
{
	CONSTRUCT(struct IE_Vendor)

	DBG("%s id=%d len=%zu oui=0x%02x%02x%02x\n", __func__, 
		ie->id, ie->len, ie->buf[0], ie->buf[1], ie->buf[2]);

	memcpy(sie->oui, ie->buf, 3);

	return 0;
}

static void ie_vendor_free(struct IE* ie)
{
	DESTRUCT(struct IE_Vendor)
	DBG("%s %p id=%d\n", __func__, (void *)ie, ie->id);
}


static const struct ie_class {
	int (*constructor)(struct IE *);
	void (*destructor)(struct IE*);
} ie_classes[256] = {
	[IE_SSID] = {
		ie_ssid_new,
		ie_ssid_free,
	},

	[IE_DSSS_PARAMETER_SET] = {
		ie_dsss_parameter_set_new,
		NULL,
	},

	[IE_SUPPORTED_RATES] = {
		ie_supported_rates_new,
		ie_supported_rates_free,
	},

	[IE_TIM] = { 
		ie_tim_new,
		ie_tim_free,
	},

	[IE_COUNTRY] = {
		ie_country_new,
		ie_country_free,
	},

	[IE_ERP] = {
		ie_erp_new,
		ie_erp_free,
	},

	[IE_HT_CAPABILITIES] = {
		ie_ht_capabilities_new,
		ie_ht_capabilities_free,
	},

	[IE_HT_OPERATION] = {
		ie_ht_operation_new,
		ie_ht_operation_free,
	},

	[IE_EXTENDED_SUPPORTED_RATES] = {
		ie_extended_supported_rates_new,
		ie_extended_supported_rates_free,
	},

	[IE_EXTENDED_CAPABILITIES] = {
		ie_extended_capa_new,
		ie_extended_capa_free,
	},

	[IE_VENDOR] = {
		ie_vendor_new,
		ie_vendor_free,
	},
};


struct IE* ie_new(uint8_t id, uint8_t len, const uint8_t* buf)
{
	DBG("%s id=%d len=%d\n", __func__, id, len);

	struct IE* ie = (struct IE*)calloc(1, sizeof(struct IE));
	if (!ie) {
		return NULL;
	} 

	ie->cookie = IE_COOKIE;
	ie->id = id;
	ie->len = len;
	// keep in mind `buf` doesn't contain the id+len octets
	ie->buf = malloc(len);
	if (!ie->buf) {
		PTR_FREE(ie);
		return NULL;
	}
	memcpy(ie->buf, buf, ie->len);

	if (ie_classes[id].constructor) {
		int err = ie_classes[id].constructor(ie);
		if (err) {
			ERR("%s id=%d failed err=%d\n", __func__, id, err);
			ie_delete(&ie);
			return NULL;
		}
	}
	else {
		WARN("%s unparsed IE=%d\n", __func__, id);
	}

	return ie;
}

void ie_delete(struct IE** pie)
{
	struct IE* ie;

	DBG("%s %p id=%d\n", __func__, (void *)(*pie), (*pie)->id);

	PTR_ASSIGN(ie, *pie);

	XASSERT( ie->cookie == IE_COOKIE, ie->cookie);

	// free my memory
	if (ie->buf) {
		memset(ie->buf, POISON, ie->len);
		PTR_FREE(ie->buf);
	}

	// now let the descendent free its memory
	// (ie->specific could be NULL if we have a partially initialized IE)
	if (ie_classes[ie->id].destructor && ie->specific) {
		ie_classes[ie->id].destructor(ie);
	}
	else {
		XASSERT(ie->specific == NULL, ie->id);
	}

	memset(ie, POISON, sizeof(struct IE));
	PTR_FREE(ie);
}

int decode_ie_buf( const uint8_t* ptr, size_t len, struct IE_List* ie_list)
{
	const uint8_t* endptr = ptr + len;
	uint8_t id, ielen;
	int err;

	// assuming caller called ie_list_init()
	XASSERT(ie_list->cookie == IE_LIST_COOKIE, ie_list->cookie);
	XASSERT(ie_list->ieptrlist != NULL, 0);
	XASSERT(ie_list->max != 0, 0);

	while (ptr < endptr) {
		id = *ptr++;
		ielen =  *ptr++;
		DBG("%s id=%u len=%u\n", __func__, id, ielen);

		struct IE* ie = ie_new(id, ielen, ptr);
		if (!ie) {
			ERR("%s failed to create ie\n", __func__);
			return -ENOMEM;
		}
		err = ie_list_move_back(ie_list, &ie);
		if (err) {
			ie_delete(&ie);
			return err;
		}
		// ie will be NULL at this point

		ptr += ielen;
	}

	return 0;
}

int ie_list_init(struct IE_List* list)
{
	memset(list, 0, sizeof(struct IE_List));
	list->cookie = IE_LIST_COOKIE;
	list->max = IE_LIST_DEFAULT_MAX;
	list->ieptrlist = (struct IE**)calloc(list->max, sizeof(struct IE*));
	if (!list->ieptrlist) {
		return -ENOMEM;
	}
	list->count = 0;

	return 0;
}

void ie_list_release(struct IE_List* list)
{
	DBG("%s\n", __func__);
	XASSERT(list->cookie == IE_LIST_COOKIE, list->cookie);
	XASSERT(list->max, 0);

	for (size_t i=0 ; i<list->count ; i++) {
		XASSERT(list->ieptrlist[i] != NULL, 0);
		ie_delete(&list->ieptrlist[i]);
	}

	memset(list->ieptrlist, POISON, sizeof(struct IE*)*list->max);
	PTR_FREE(list->ieptrlist);
	memset(list, POISON, sizeof(struct IE_List));
}

int ie_list_move_back(struct IE_List* list, struct IE** pie)
{
	if (list->count+1 > list->max) {
		struct IE** new_ieptrlist;
		INFO("%s resize list=%p from %zu to %zu\n", __func__,
				(void*)list, list->max, list->max*2);
		new_ieptrlist = reallocarray(list->ieptrlist, list->max*2, sizeof(struct IE*));
		if( !new_ieptrlist ) {
			WARN("%s realloc of %zu failed\n", __func__, sizeof(struct IE*)*(list->max*2));
			return -ENOMEM;
		}
		memset(new_ieptrlist+list->max, 0, list->max);
		list->ieptrlist = new_ieptrlist;
		list->max *= 2;
	}

	// dis mine now
	PTR_ASSIGN(list->ieptrlist[list->count++], *pie);

	return 0;
}

void ie_list_peek(const char *label, struct IE_List* list)
{
	for (size_t i=0 ; i<list->count ; i++) {
		struct IE* ie = list->ieptrlist[i];
		DBG("%s IE id=%d len=%zu\n", label, ie->id, ie->len);
	}

}

const struct IE* ie_list_find_id(const struct IE_List* list, IE_ID id)
{
	// search for the first instance of an id; note this will not work when
	// there are duplicates such as vendor id
	for (size_t i=0 ; i<list->count ; i++) {
		if (list->ieptrlist[i]->id == id) {
			return (const struct IE*)list->ieptrlist[i];
		}
	}

	return (const struct IE*)NULL;
}

