/*
 * libiw/iw.h   library-ish wrapper around some `iw` fuctionality
 *
 * Contains copied chunks from from iw iw.h See COPYING.
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef IW_H
#define IW_H

#include <stddef.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <netlink/attr.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>

#include "ie.h"
#include "bss.h"

//
// contains copied chunks from from iw iw.h
//

#ifdef __cplusplus
extern "C" {
#endif

// nl80211.h enum nl80211_rate_info
struct bitrate 
{
	// avoid floats so using what iw uses (rate*10):
	// "%d.%d MBit/s", rate / 10, rate % 10);
	uint32_t rate;

	uint8_t mcs;

	// will be set to -1 if no value (0 is a valid value)
	enum nl80211_chan_width chan_width;

	bool short_gi;

	uint8_t vht_mcs;
	uint8_t vht_nss;

	uint8_t he_mcs;
	uint8_t he_nss;
	uint8_t he_gi;
	uint8_t he_dcm;
	uint8_t he_ru_alloc;
};

struct iw_link_state 
{
	int ifindex;

	// bssid we're connected to
	uint8_t connected[ETH_ALEN];

	uint32_t rx_bytes, rx_packets;
	uint32_t tx_bytes, tx_packets;

	// note signed integer
	int8_t signal_dbm;

	// enum nl80211_sta_bss_param encoded as (1<<enum)
	uint32_t bss_param;

	uint8_t dtim_period;
	uint16_t beacon_int;

	struct bitrate rx_bitrate, tx_bitrate;
};

// a wlan device ; sort of like 'iw dev'
struct iw_dev
{
	int ifindex;
	uint8_t addr[ETH_ALEN];
	char ifname[IF_NAMESIZE];
	uint32_t phy_id;
	uint64_t wdev;
	enum nl80211_iftype iftype;

	uint32_t freq_khz;
	enum nl80211_chan_width channel_width;
	uint32_t channel_center_freq1;
	uint32_t channel_center_freq2;

	uint32_t generation;
};

void peek_nla_attr( struct nlattr* tb_msg[], size_t count);
//void peek_nla_bss(struct nlattr* bss_msg[], size_t count);
//void peek_nla_bss(const struct nlattr* bss_msg[const static NL80211_BSS_MAX], size_t count);

int parse_nla_ies(struct nlattr* ies, struct IE_List* ie_list);
int parse_nla_bss(struct nlattr* attr, struct BSS* bss);

const char* bw_str(enum nl80211_chan_width w);
const char* country_env_str(enum Environment environment);
const char* ht_max_amsdu_str(uint8_t max_amsdu);
const char* ht_secondary_offset_str(uint8_t off);
const char* ht_sta_channel_width_str(uint8_t w);
const char* ht_protection_str(uint8_t prot);
const char* vht_max_mpdu_length_str(uint8_t len);
const char* vht_supported_channel_width_str(uint8_t w);
const char* vht_channel_width_str(uint8_t w);

int tsf_to_timestamp_str(uint64_t tsf, char* s, size_t len);
int capability_to_str(uint16_t capa, char* s, size_t len);
int erp_to_str(const struct IE_ERP* sie, char* s, size_t len);
uint32_t ht_ampdu_compute_length(uint8_t exponent);
int ht_ampdu_length_to_str(uint8_t exponent, char* s, size_t len);
int ht_ampdu_spacing_to_ns(uint8_t spacing);
int ht_ampdu_spacing_to_str(uint8_t spacing, char* s, size_t len);
int mcs_index_bitmask_to_str(const uint8_t* buf, char* s, size_t len);
int cipher_suite_to_str(const struct RSN_Cipher_Suite* suite, char* s, size_t len);
int auth_to_str(const struct RSN_Cipher_Suite* suite, char* s, size_t len);
int rsn_capabilities_to_str(const struct IE_RSN* ie, char* s, size_t len);
int rm_enabled_capa_to_str(const struct IE_RM_Enabled_Capabilities* sie, unsigned int idx, char* s, size_t len);
int rsn_capabilities_to_str_list(const struct IE_RSN* sie, char s_list[][64], size_t len, size_t s_list_len);
int mobility_domain_to_str(const struct IE_Mobility_Domain* sie, unsigned int bit, char* s, size_t len);
int ieee80211_channel_to_frequency(int chan, enum nl80211_band band);
int ieee80211_frequency_to_channel(int freq);

void mac_addr_n2a(char *mac_addr, const unsigned char *arg);
int parse_bitrate(struct nlattr *bitrate_attr, struct bitrate* br); 

int iw_dev_list(struct iw_dev* dev_list, size_t* count);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif

