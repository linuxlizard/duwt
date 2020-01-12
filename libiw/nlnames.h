#ifndef NLNAMES_H
#define NLNAMES_H

#ifdef __cplusplus
extern "C" {
#endif

const char* to_string_nl80211_commands(enum nl80211_commands val);
const char* to_string_nl80211_attrs(enum nl80211_attrs val);
const char* to_string_nl80211_bss(enum nl80211_bss val);
const char* to_string_nl80211_chan_width(enum nl80211_chan_width val);

#ifdef __cplusplus
}  // end extern "C"
#endif

#endif

