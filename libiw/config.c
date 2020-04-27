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
	this should fail
	enum nl80211_bss = NL80211_BSS_CHAIN_SIGNAL;
#endif

	return 0;
}

