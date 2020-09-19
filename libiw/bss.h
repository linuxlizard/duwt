/*
 * libiw/bss.h   BSS data structure
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef BSS_H
#define BSS_H

#include <linux/if_ether.h>
#include <linux/netlink.h>
#include <linux/nl80211.h>

#include "list.h"
#include "ie.h"

#define BSS_COOKIE 0xc36194b7

typedef uint8_t macaddr[ETH_ALEN];

struct capability;

// this seems like a bad idea
#define BSSID_U64(buf) (*(uint64_t*)(buf) & ~(0xffffULL << 48))

struct BSS 
{
	uint32_t cookie;
	struct dl_list node;
	int ifindex;

	macaddr bssid;

	// bssid in printable string format
	char bssid_str[24];

	uint32_t frequency;

	// * @NL80211_BSS_TSF: TSF of the received probe response/beacon (u64)
	// *	(if @NL80211_BSS_PRESP_DATA is present then this is known to be
	// *	from a probe response, otherwise it may be from the same beacon
	// *	that the NL80211_BSS_BEACON_TSF will be from)
	uint64_t tsf;  // Timing Synchronization Function

 	// @NL80211_ATTR_BEACON_INTERVAL: beacon interval in TU
	uint16_t beacon_interval;

	// @NL80211_BSS_CAPABILITY: capability field (CPU order, u16)
	// IEEE 80211-2016.pdf 9.4.1.4 Capability Information field
	uint16_t capability;

	// "@NL80211_BSS_SIGNAL_MBM: signal strength of probe response/beacon
	//	in mBm (100 * dBm) (s32)"  (via nl80211.h)
	int32_t signal_strength_mbm;

	// "@NL80211_BSS_SIGNAL_UNSPEC: signal strength of the probe response/beacon
	//	in unspecified units, scaled to 0..100 (u8)"  (via nl80211.h)
	uint8_t signal_unspec;

 	// enum nl80211_band - Frequency band
	enum nl80211_band band;

	// enum nl80211_chan_width - channel width definitions
	// "These values are used with the %NL80211_ATTR_CHANNEL_WIDTH
	// attribute."  (via nl80211.h)
	enum nl80211_chan_width chan_width;
	uint32_t center_freq1, center_freq2;

	struct IE_List ie_list;
};

// iw scan.c bits in NL80211_BSS_CAPABILITY
#define WLAN_CAPABILITY_ESS		(1<<0)
#define WLAN_CAPABILITY_IBSS		(1<<1)
#define WLAN_CAPABILITY_CF_POLLABLE	(1<<2)
#define WLAN_CAPABILITY_CF_POLL_REQUEST	(1<<3)
#define WLAN_CAPABILITY_PRIVACY		(1<<4)
#define WLAN_CAPABILITY_SHORT_PREAMBLE	(1<<5)
#define WLAN_CAPABILITY_PBCC		(1<<6)
#define WLAN_CAPABILITY_CHANNEL_AGILITY	(1<<7)
#define WLAN_CAPABILITY_SPECTRUM_MGMT	(1<<8)
#define WLAN_CAPABILITY_QOS		(1<<9)
#define WLAN_CAPABILITY_SHORT_SLOT_TIME	(1<<10)
#define WLAN_CAPABILITY_APSD		(1<<11)
#define WLAN_CAPABILITY_RADIO_MEASURE	(1<<12)
#define WLAN_CAPABILITY_DSSS_OFDM	(1<<13)
#define WLAN_CAPABILITY_DEL_BACK	(1<<14)
#define WLAN_CAPABILITY_IMM_BACK	(1<<15)
/* DMG (60gHz) 802.11ad */
/* type - bits 0..1 */
#define WLAN_CAPABILITY_DMG_TYPE_MASK		(3<<0)

#define WLAN_CAPABILITY_DMG_TYPE_IBSS		(1<<0) /* Tx by: STA */
#define WLAN_CAPABILITY_DMG_TYPE_PBSS		(2<<0) /* Tx by: PCP */
#define WLAN_CAPABILITY_DMG_TYPE_AP		(3<<0) /* Tx by: AP */

#define WLAN_CAPABILITY_DMG_CBAP_ONLY		(1<<2)
#define WLAN_CAPABILITY_DMG_CBAP_SOURCE		(1<<3)
#define WLAN_CAPABILITY_DMG_PRIVACY		(1<<4)
#define WLAN_CAPABILITY_DMG_ECPAC		(1<<5)

#define WLAN_CAPABILITY_DMG_SPECTRUM_MGMT	(1<<8)
#define WLAN_CAPABILITY_DMG_RADIO_MEASURE	(1<<12)
// end iw scan.c capability bits

// decoded 16-bit Capability field NL80211_BSS_CAPABILITY
// (TODO DMG)
struct Capability
{
	uint8_t ESS : 1;
	uint8_t IBSS : 1;
	uint8_t CF_POLLABLE : 1;
	uint8_t CF_POLL_REQUEST : 1;
	uint8_t PRIVACY : 1;
	uint8_t SHORT_PREAMBLE : 1;
	uint8_t PBCC : 1;
	uint8_t CHANNEL_AGILITY : 1;
	uint8_t SPECTRUM_MGMT : 1;
	uint8_t QOS : 1;
	uint8_t SHORT_SLOT_TIME : 1;
	uint8_t APSD : 1;
	uint8_t RADIO_MEASURE : 1;
	uint8_t DSSS_OFDM : 1;
	uint8_t DEL_BACK : 1;
	uint8_t IMM_BACK : 1;
};

#ifdef __cplusplus
extern "C" {
#endif

struct BSS* bss_new(void);
void bss_free(struct BSS** pbss);

void bss_free_list(struct dl_list* list);

// parse NL80211_ATTR_xxx into a struct BSS
int bss_from_nlattr(struct nlattr* attr[], struct BSS** pbss);

const struct IE_SSID* bss_get_ssid(const struct BSS* bss);

int bss_guess_chan_width(struct BSS* bss);
bool bss_is_vht(const struct BSS* bss);
bool bss_is_ht(const struct BSS* bss);

int bss_get_chan_width_str(const struct BSS* bss, char* s, size_t len);
int bss_get_mode_str(const struct BSS* bss, char* s, size_t len);

int bss_str_to_bss(const char* bss_str, size_t bssid_str_len, macaddr bssid);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif

