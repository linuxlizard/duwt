/*
 * libiw/iw-dev.c   duplicate 'iw dev' command
 *
 * XXX Work in progress. Not complete.
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "core.h"
#include "iw.h"

int main(void)
{
	int ret=0;

	log_set_level(LOG_LEVEL_PLAID);

	size_t count = 10;
	struct iw_dev wlan_dev_list[count];

	ret = iw_dev_list(wlan_dev_list, &count);

	printf("found %zu interfaces\n", count);

	struct iw_dev* dev = &wlan_dev_list[0];
	for (size_t i=0 ; i<count ; i++, dev++ ) {
		if (dev->ifindex==0) {
			printf("Unnamed/non-netdev interface\n");
			printf("\twdev=%#" PRIx64 "\n", dev->wdev);
			printf("\taddr=" MAC_ADD_FMT "\n", MAC_ADD_PRN(dev->addr));
			printf("\ttype=%" PRIu32 "\n", dev->iftype);
		}
		else {
			printf("Interface %s\n", dev->ifname);
			printf("\tifindex=%d\n", dev->ifindex);
			printf("\twdev=%#" PRIx64 "\n", dev->wdev);
			printf("\taddr=" MAC_ADD_FMT "\n", MAC_ADD_PRN(dev->addr));
			printf("\ttype=%" PRIu32 "\n", dev->iftype);
			printf("\tchannel %d (%d MHz), width: %s MHz, center1: %d MHz\n",
					ieee80211_frequency_to_channel(dev->freq_khz), 
					dev->freq_khz, bw_str(dev->channel_width), dev->channel_center_freq1);
			printf("\tgeneration=%" PRIu32 "\n", dev->generation);
		}
	}
	return ret;
}

