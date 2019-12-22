#ifndef IW_H
#define IW_H

#include <stddef.h>
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
int ht_ampdu_length_to_str(uint8_t exponent, char* s, size_t len);
int ht_ampdu_spacing_to_str(uint8_t spacing, char* s, size_t len);
int mcs_index_bitmask_to_str(const uint8_t* buf, char* s, size_t len);
int cipher_suite_to_str(const struct RSN_Cipher_Suite* suite, char* s, size_t len);
int auth_to_str(const struct RSN_Cipher_Suite* suite, char* s, size_t len);
int rsn_capabilities_to_str(const struct IE_RSN* ie, char* s, size_t len);
int rm_enabled_capa_to_str(const struct IE_RM_Enabled_Capabilities* sie, unsigned int idx, char* s, size_t len);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif

