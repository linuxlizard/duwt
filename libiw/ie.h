#ifndef IE_H
#define IE_H

// From IEEE802.11-2016:
//
// This revision is based on IEEE Std 802.11-2012, into which the following amendments have been
// incorporated:
// — IEEE Std 802.11aeTM-2012: Prioritization of Management Frames (Amendment 1)
// — IEEE Std 802.11aaTM-2012: MAC Enhancements for Robust Audio Video Streaming (Amendment 2)
// — IEEE Std 802.11adTM-2012: Enhancements for Very High Throughput in the 60 GHz Band (Amendment 3)
// — IEEE Std 802.11acTM-2013: Enhancements for Very High Throughput for Operation in Bands below 6 GHz (Amendment 4)
// — IEEE Std 802.11afTM-2013: Television White Spaces (TVWS) Operation (Amendment 5)

// From IEEE802.11-2012:
//
// <quote>
// The original standard was published in 1999 and reaffirmed in 2003. A revision was published in 2007,
// which incorporated into the 1999 edition the following amendments: IEEE Std 802.11aTM-1999,
// IEEE Std 802.11bTM-1999, IEEE Std 802.11b-1999/Corrigendum 1-2001, IEEE Std 802.11dTM-2001, IEEE
// Std 802.11gTM-2003, IEEE Std 802.11hTM-2003, IEEE Std 802.11iTM-2004, IEEE Std 802.11jTM-2004 and
// IEEE Std 802.11eTM-2005.
//
// The current revision, IEEE Std 802.11-2012, incorporates the following amendments into the 2007 revision:
// — IEEE Std 802.11kTM-2008: Radio Resource Measurement of Wireless LANs (Amendment 1)
// — IEEE Std 802.11rTM-2008: Fast Basic Service Set (BSS) Transition (Amendment 2)
// — IEEE Std 802.11yTM-2008: 3650–3700 MHz Operation in USA (Amendment 3)
// — IEEE Std 802.11wTM-2009: Protected Management Frames (Amendment 4)
// — IEEE Std 802.11nTM-2009: Enhancements for Higher Throughput (Amendment 5)
// — IEEE Std 802.11pTM-2010: Wireless Access in Vehicular Environments (Amendment 6)
// — IEEE Std 802.11zTM-2010: Extensions to Direct-Link Setup (DLS) (Amendment 7)
// — IEEE Std 802.11vTM-2011: IEEE 802.11 Wireless Network Management (Amendment 8)
// — IEEE Std 802.11uTM-2011: Interworking with External Networks (Amendment 9)
// — IEEE Std 802.11sTM-2011: Mesh Networking (Amendment 10)
//
// As a result of publishing this revision, all of the previously published amendments and revisions are now
// retired.
// </quote>

#include <stdint.h>
#include <stdbool.h>

// https://unicode-org.github.io/icu-docs/apidoc/released/icu4c/
#include <unicode/utypes.h>
#include <unicode/utext.h>
#include <unicode/utf8.h>

typedef enum {
	IE_SSID = 0,
	IE_SUPPORTED_RATES = 1,
	IE_DSSS_PARAMETER_SET = 3,
	IE_TIM = 5, // there are those who call me...
	IE_COUNTRY = 7,
	IE_ERP = 42,
	IE_HT_CAPABILITIES = 45,
	IE_HT_OPERATION = 61,
	IE_EXTENDED_SUPPORTED_RATES = 50,
	IE_MESH_ID = 114,
	IE_EXTENDED_CAPABILITIES = 127,
	IE_VHT = 191,
	IE_VENDOR = 221,
		
} IE_ID;

extern const uint8_t ms_oui[3];
extern const uint8_t ieee80211_oui[3];
extern const uint8_t wfa_oui[3];

#define IE_COOKIE 0xdc4904e3
#define IE_SPECIFIC_COOKIE 0x65cb47f0
#define IE_LIST_COOKIE 0xcb72f5ff

// general "base class" of all IE
struct IE 
{
	uint32_t cookie;
	IE_ID id;
	size_t len;

	// malloc'd buffer
	// raw bytes of the IE; does not include id+len
	uint8_t* buf;

	// some IEs are just a single element small enough to fit here so let's not
	// bother allocating memory if we don't have to
	uint32_t value;

	// blob of specific IE info
	void* specific;
};


// base is a pointer to the struct IE that contains us
#define IE_SPECIFIC_FIELDS\
	uint32_t cookie;\
	struct IE* base;

// standard says 32 octets but Unicode seriously muddies the water
#define SSID_MAX_LEN 32

#define IE_CAST(ieptr, type)\
	((type*)ie->specific)

struct IE_SSID
{
	IE_SPECIFIC_FIELDS

	// http://userguide.icu-project.org/strings
	// http://userguide.icu-project.org/strings/utf-8
	UChar ssid[SSID_MAX_LEN*2];
	int32_t ssid_len;
};

struct Supported_Rate
{
	float rate;
	bool basic : 1;
};

struct IE_Supported_Rates
{
	IE_SPECIFIC_FIELDS

	size_t count;
	float rate[8];
	bool basic[8];
};

// TIM == Traffic Indication Map
struct IE_TIM
{
	IE_SPECIFIC_FIELDS

	uint8_t dtim_count;
	uint8_t dtim_period;
	uint8_t control;
	uint8_t* bitmap; // POINTER into ie->buf
};

// iw scan.c print_country()
#define IEEE80211_COUNTRY_EXTENSION_ID 201
union ieee80211_country_ie_triplet {
	struct {
		uint8_t first_channel;
		uint8_t num_channels;
		int8_t max_power; // note signed integer!
	} __attribute__ ((packed)) chans;
	struct {
		uint8_t reg_extension_id;
		uint8_t reg_class;
		uint8_t coverage_class;
	} __attribute__ ((packed)) ext;
} __attribute__ ((packed));
// end iw

enum Environment {
	ENV_INVALID, ENV_INDOOR_ONLY, ENV_OUTDOOR_ONLY, ENV_INDOOR_OUTDOOR
};

#define IE_COUNTRY_TRIPLET_MAX 32

struct IE_Country
{
	IE_SPECIFIC_FIELDS

	char country[3];

	// save the original byte, too
	uint8_t environment_byte;
	enum Environment environment;

	union ieee80211_country_ie_triplet triplets[IE_COUNTRY_TRIPLET_MAX];
	size_t count;
};

struct IE_ERP
{
	IE_SPECIFIC_FIELDS

	// bitfields; using same name as the IEEE 80211-2016.pdf
	bool NonERP_Present : 1;
	bool Use_Protection : 1;
	bool Barker_Preamble_Mode : 1;
};

// RX STBC bits of HT Capabilities IE
enum IE_HT_RX_STBC {
	NO_RX_STBC = 0,
	RX_STBC_1_STREAM = 1,
	RX_STBC_2_STREAM = 2,
	RX_STBC_3_STREAM = 3
};

// MCS Set field
// XXX does this also apply to VHT MCS?
struct HT_MCS_Set
{
	// whole blob is 16 bytes
	// using same names as iw util.c print_ht_mcs()
	uint8_t mcs_bits[10];  // 77 bits + 3 reserved
	uint16_t max_rx_supp_data_rate;  // 10 bits + 6 reserved
	uint8_t tx_mcs_set_defined : 1;
	uint8_t tx_mcs_set_equal : 1;
	uint8_t tx_max_num_spatial_streams : 2;
	uint8_t tx_unequal_modulation : 1;
	// 27 reserved bits
};

struct IE_HT_Capabilities
{
	IE_SPECIFIC_FIELDS

	// capa decoded into individual bitfields in 
	// HT Capability Info Field below
	uint16_t capa;
	uint8_t ampdu_param;
	uint8_t* supported_mcs_ptr; // 16 bytes; points into ie->buf
	uint16_t extended_capa;
	uint32_t tx_beamforming_capa;
	uint8_t asel_capa;

	// HT Capability Info Field
	uint8_t LDPC_coding_capa : 1;
	uint8_t supported_channel_width : 1;
	uint8_t SM_power_save : 2;
	uint8_t greenfield : 1;
	uint8_t short_gi_20Mhz : 1;
	uint8_t short_gi_40Mhz : 1;
	uint8_t tx_stbc : 1;
	enum IE_HT_RX_STBC rx_stbc : 2;
	uint8_t delayed_block_ack : 1;
	uint8_t max_amsdu_len : 1;
	uint8_t dsss_cck_in_40Mhz : 1;
	uint8_t _40Mhz_intolerant : 1;
	uint8_t lsig_txop_prot : 1;

	// A-MPDU Parameters Field
	uint8_t max_ampdu_len : 2;
	uint8_t min_ampdu_spacing : 3;

	struct HT_MCS_Set mcs;
};

struct IE_HT_Operation
{
	IE_SPECIFIC_FIELDS

	uint8_t primary_channel;
	uint8_t* info_ptr; // point into ie->buf ; 5 bytes
	uint8_t* mcs_set_ptr; // point into ie->buf ; 16 bytes

	// HT Operation Information field bits
	uint8_t secondary_channel_offset : 2;
	uint8_t sta_channel_width : 1;
	uint8_t rifs : 1;
	// bit 4-7 reserved
	uint8_t ht_protection : 2;
	uint8_t non_gf_present : 1;
	// bit 11 reserved
	uint8_t obss_non_gf_present : 1;
	// iw doesn't report this --v
	uint16_t channel_center_frequency_2 : 11;
	// bit 21-29 reserved
	uint8_t dual_beacon : 1;
	uint8_t dual_cts_protection : 1;
	uint8_t stbc_beacon : 1;
	uint8_t lsig_txop_prot : 1;
	uint8_t pco_active : 1;
	uint8_t pco_phase : 1;
	// bits 36-39 reserved
};

struct IE_Extended_Supported_Rates
{
	IE_SPECIFIC_FIELDS

	size_t count;
	struct Supported_Rate rates[];
};

struct IE_Extended_Capabilities
{
	IE_SPECIFIC_FIELDS

	// bitfields ahoy!

	// octet [0]
	bool bss_2040_coexist : 1;
	// bit 1 reserved
	bool ESS : 1;
	// bit 3 reserved but iw scan.c says "Wave Indication" and who am I to
	// disagree
	bool wave_indication : 1;
	bool psmp_capa : 1;
	// bit 5 reserved but iw scan.c says "Service Interval Granularity"
	bool service_interval_granularity_flag;
	bool spsmp_support : 1;
	bool event : 1;

	// octest [1]
	bool diagnostics : 1;
	bool multicast_diagnostics : 1;
	bool location_tracking : 1;  // you should be ashamed
	bool FMS : 1;
	bool proxy_arp : 1;
	bool collocated_interference_reporting : 1;
	bool civic_location : 1;
	bool geospatial_location : 1;

	// octet [2]
	bool TFS : 1;
	bool WNM_sleep_mode : 1;
	bool TIM_broadcast : 1;
	bool BSS_transition : 1;
	bool QoS_traffic_capa : 1;
	bool AC_station_count : 1;
	bool multiple_BSSID : 1;
	bool timing_measurement : 1;

	// octet [3]
	bool channel_usage : 1;
	bool SSID_list : 1;
	bool DMS : 1;
	bool UTC_TSF_offset : 1;
	bool TPU_buffer_STA_support : 1;
	bool TDLS_peer_PSM_support : 1;
	bool TDLS_channel_switching : 1;
	bool internetworking : 1;

	// octet[4]
	bool QoS_map : 1;
	bool EBR : 1;
	bool SSPN_interface : 1;
	// 35 reserved
	bool MSGCF_capa : 1;
	bool TDLS_support : 1;
	bool TDLS_prohibited : 1;
	bool TDLS_channel_switch_prohibited : 1;

	// octet [5]
	bool reject_unadmitted_frame : 1;
	// 3 bits
	unsigned int service_interval_granularity : 3;
	bool identifier_location : 1;
	bool UAPSD_coexist : 1;
	bool WNM_notification : 1;
	bool QAB_capa : 1;

	// octet [6]
	bool UTF8_ssid : 1; // ¯\_(ツ)_/¯
	bool QMF_activated : 1;
	bool QMF_reconfig_activated : 1;
	bool robust_av_streaming : 1;
	bool advanced_GCR : 1;
	bool mesh_GCR : 1;
	bool SCS : 1;
	bool q_load_report : 1;

	// octet[7];
	bool alternate_EDCA : 1;
	bool unprot_TXOP_negotiation : 1;
	bool prot_TXOP_negotiation : 1;
	// bit 59 reserved
	bool prot_q_load_report : 1;
	bool TDLS_wider_bandwidth : 1;
	bool operating_mode_notification : 1;

	// 2 bits broken across octet[7] and octet[8]
	// (thanks, 802.11 technical committee(s)!)
	unsigned int max_MSDU_in_AMSDU : 2;

	// rest of octet[8] starting with bit 65
	bool channel_mgmt_sched : 1;
	bool geo_db_inband : 1;
	bool network_channel_control : 1;
	bool whitespace_map : 1;
	bool channel_avail_query : 1;
	bool FTM_responder : 1;
	bool FTM_initiator : 1;

	// octet [9]
	// bit 72 reserved (skipped) in IEEE 80211-2016.pdf 
	bool extended_spectrum_mgmt : 1;
	bool future_channel_guidance : 1;

};

#define IE_VENDOR_OUI_LEN 5
struct IE_Vendor
{
	IE_SPECIFIC_FIELDS

	uint8_t oui[IE_VENDOR_OUI_LEN];
};

struct IE_List
{
	uint32_t cookie;
	// array of pointers
	struct IE** ieptrlist;
	size_t max;
	size_t count;
};

struct IE* ie_new(uint8_t id, uint8_t len, const uint8_t* buf);
void ie_delete(struct IE** pie);

#define IE_LIST_BLANK\
	{ .cookie = 0, .max = 0, .count = 0 }

int ie_list_init(struct IE_List* list);
void ie_list_release(struct IE_List* list);
int ie_list_move_back(struct IE_List* list, struct IE** pie);
void ie_list_peek(const char *label, struct IE_List* list);
const struct IE* ie_list_find_id(const struct IE_List* list, IE_ID id);

int decode_ie_buf( const uint8_t* buf, size_t len, struct IE_List* ielist);

#define ie_list_for_each_entry(pos, list)\
	for (size_t i=0 ; (pos=list.ieptrlist[i]) && i<list.count ; pos=list.ieptrlist[++i]) 

#endif

