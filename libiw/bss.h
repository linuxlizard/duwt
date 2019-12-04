#ifndef BSS_H
#define BSS_H

#include <linux/if_ether.h>
#include <linux/netlink.h>
#include <linux/nl80211.h>

#include "list.h"
#include "ie.h"

#define BSS_COOKIE 0xc36194b7

typedef uint8_t macaddr[ETH_ALEN];

struct BSS 
{
	uint32_t cookie;
	struct list_head node;
	int ifindex;

	macaddr bssid;
	// bssid in printable string format
	char bssid_str[24];


	// * @NL80211_BSS_TSF: TSF of the received probe response/beacon (u64)
	// *	(if @NL80211_BSS_PRESP_DATA is present then this is known to be
	// *	from a probe response, otherwise it may be from the same beacon
	// *	that the NL80211_BSS_BEACON_TSF will be from)
	uint64_t tsf;  // Timing Synchronization Function

 	// @NL80211_ATTR_BEACON_INTERVAL: beacon interval in TU
	uint16_t beacon_interval;

	// "@NL80211_BSS_SIGNAL_MBM: signal strength of probe response/beacon
	//	in mBm (100 * dBm) (s32)"  (via nl80211.h)
	int32_t signal_strength_mbm;

	// "@NL80211_BSS_SIGNAL_UNSPEC: signal strength of the probe response/beacon
	//	in unspecified units, scaled to 0..100 (u8)"  (via nl80211.h)
	uint8_t signal_unspec;

	uint32_t frequency;

	struct IE_List ie_list;
};

struct BSS* bss_new(void);
void bss_free(struct BSS** pbss);

void bss_free_list(struct list_head* list);

// parse NL80211_ATTR_xxx into a struct BSS
int bss_from_nlattr(struct nlattr* attr[], struct BSS** pbss);

#endif

