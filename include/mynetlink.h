#ifndef CPP_NETLINK_H
#define CPP_NETLINK_H

#include <asm/types.h>
#include <sys/socket.h>
#include <linux/socket.h>
#include <linux/netlink.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/nl80211.h>

		// C++ frowned upon naked casting void* return value of nla_data() to nlattr*
#define my_nla_for_each_nested(pos, nla, rem) \
	for (pos = static_cast<nlattr*>(nla_data(nla)), rem = nla_len(nla); \
	     nla_ok(pos, rem); \
	     pos = nla_next(pos, &(rem)))

#endif

