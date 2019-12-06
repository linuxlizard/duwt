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

#define ARRAY_SIZE(ar) (sizeof(ar)/sizeof(ar[0]))

void peek_nla_attr( struct nlattr* tb_msg[], size_t count);
//void peek_nla_bss(struct nlattr* bss_msg[], size_t count);
//void peek_nla_bss(const struct nlattr* bss_msg[const static NL80211_BSS_MAX], size_t count);

int parse_nla_ies(struct nlattr* ies, struct IE_List* ie_list);
int parse_nla_bss(struct nlattr* attr, struct BSS* bss);

const char* bw_to_str(enum nl80211_chan_width w);
const char *country_env_str(enum Environment environment);

int tsf_to_timestamp_str(uint64_t tsf, char* s, size_t len);
int capability_to_str(uint16_t capa, char* s, size_t len);

#endif

