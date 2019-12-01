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
	macaddr bssid;

	// bssid in printable string format
	char bssid_str[24];

	uint32_t frequency;
	enum nl80211_chan_width channel_width;

	struct IE_List ie_list;
};

struct BSS* bss_new(void);
void bss_free(struct BSS** pbss);

void bss_free_list(struct list_head* list);

// parse NL80211_ATTR_xxx into a struct BSS
int bss_from_nlattr(struct nlattr* attr[], struct BSS** pbss);

#endif

