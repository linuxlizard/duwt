/*
 * libiw/config.c   nl80211 configuration detection tester
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <linux/nl80211.h>

int main(void)
{
#ifdef HAVE_NL80211_STA_INFO_RX_DURATION
	enum nl80211_sta_info sta_info = NL80211_STA_INFO_RX_DURATION;
	printf("sta_info=%d\n", sta_info);
#endif

#ifdef HAVE_NL80211_BSS_PAD
	enum nl80211_bss bss_attr = NL80211_BSS_PAD;
#endif

#ifdef HAVE_NL80211_BAND_6GHZ
	enum nl80211_band band = NL80211_BAND_6GHZ;
#endif

#ifdef HAVE_NL80211_BSS_CHAIN_SIGNAL
	enum nl80211_bss chain_signal = NL80211_BSS_CHAIN_SIGNAL;
#endif

#ifdef HAVE_NL80211_CMD_EXTERNAL_AUTH
	enum nl80211_commands external_auth = NL80211_CMD_EXTERNAL_AUTH;
#endif

#ifdef HAVE_NL80211_CMD_ABORT_SCAN
	enum nl80211_commands abort_scan = NL80211_CMD_ABORT_SCAN;
#endif

#ifdef HAVE_NL80211_RATE_INFO_HE_MCS
	enum nl80211_rate_info info = NL80211_RATE_INFO_HE_MCS;
#endif

#ifdef HAVE_NL80211_ATTR_AUTH_DATA
	enum nl80211_attrs auth_data = NL80211_ATTR_AUTH_DATA;
#endif

#ifdef HAVE_NL80211_ATTR_EXTERNAL_AUTH_ACTION
	enum nl80211_attrs external_auth_action = NL80211_ATTR_EXTERNAL_AUTH_ACTION;
#endif

#ifdef HAVE_NL80211_ATTR_HE_OBSS_PD 
	enum nl80211_attrs he_obss_pd = NL80211_ATTR_HE_OBSS_PD;
#endif
	return 0;
}

