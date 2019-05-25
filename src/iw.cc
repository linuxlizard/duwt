/* this code based originally on 'iw' which has the following COPYING */
/* 
 * Copyright (c) 2007, 2008        Johannes Berg
 * Copyright (c) 2007              Andy Lutomirski
 * Copyright (c) 2007              Mike Kershaw
 * Copyright (c) 2008-2009         Luis R. Rodriguez
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>
#include <assert.h>

#include "mynetlink.h"
#include "logging.h"
#include "iw.h"

/* davep 20190505 ; TODO make a user configurable flag */
static const bool iw_debug = false;

/* iw-4.9 iw.h */
#define BIT(x) (1ULL<<(x))

/* iw-4.9 station.c */
enum plink_state {
	LISTEN,
	OPN_SNT,
	OPN_RCVD,
	CNF_RCVD,
	ESTAB,
	HOLDING,
	BLOCKED
};

void peek(struct nl_msg *msg)
{
	struct nlmsghdr * nlh;
	struct nlattr * attr;
	int i, rem;

	// curious about how to use nlmsg_for_each_xxx()
	// tests/check-attr.c from libnl-3.2.25
	printf( "%s\n", __func__);

	nlh = nlmsg_hdr(msg);
	i = 1;
//	int len = NLMSG_LENGTH(nlh);
	int len = 1024;
	printf("peek len=%d\n", len);

//	hex_dump("peek", (const unsigned char *)msg, 64);

	int hdrlen = 0;
	nlmsg_attrdata(nlh, hdrlen);
	nlmsg_attrlen(nlh, hdrlen);

	nlmsg_for_each_attr(attr, nlh, 0, rem) {
		printf("peek type=%d len=%d\n", nla_type(attr), nla_len(attr));
		i++;
	}
}

/* from iw 5.0.1 */
int nl80211_init(struct nl80211_state* state)
{
	int err;

	state->nl_sock = nl_socket_alloc();
	if (!state->nl_sock) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}

	if (genl_connect(state->nl_sock)) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		err = -ENOLINK;
		goto out_handle_destroy;
	}

	nl_socket_set_buffer_size(state->nl_sock, 8192, 8192);

	/* try to set NETLINK_EXT_ACK to 1, ignoring errors */
#ifdef NETLINK_EXT_ACK
	err = 1;
	setsockopt(nl_socket_get_fd(state->nl_sock), SOL_NETLINK,
		   NETLINK_EXT_ACK, &err, sizeof(err));
#endif

	state->nl80211_id = genl_ctrl_resolve(state->nl_sock, "nl80211");
	if (state->nl80211_id < 0) {
		fprintf(stderr, "nl80211 not found.\n");
		err = -ENOENT;
		goto out_handle_destroy;
	}

	return 0;

 out_handle_destroy:
	nl_socket_free(state->nl_sock);
	return err;
}

/* from iw 5.0.1 */
//void nl80211_cleanup(struct nl80211_state *state)
//{
//	nl_socket_free(state->nl_sock);
//}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	if (iw_debug) {
		printf("%s\n", __func__);
	}
	(void)nla;
	(void)err;
	(void)arg;

	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	if (iw_debug) {
		printf("%s\n", __func__);
	}
	(void)msg;

	int *ret = static_cast<int*>(arg);
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	if (iw_debug) {
		printf("%s\n", __func__);
	}
	(void)msg;

	int *ret = static_cast<int*>(arg);
	*ret = 0;
	return NL_STOP;
}


int valid_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr_list *attrs = static_cast<struct nlattr_list*>(arg);

//	printf("%s %p %p\n", __func__, (void *)msg, arg);

	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	struct genlmsghdr* gnlh = static_cast<struct genlmsghdr*>(nlmsg_data(hdr));

	if (iw_debug) {
		nl_msg_dump(msg,stdout);
		printf("%s nlmsg max size=%ld\n", __func__, nlmsg_get_max_size(msg));
		printf("%s msgsize=%d\n", __func__, nlmsg_datalen(hdr));
		printf("%s genlen=%d genattrlen=%d\n", __func__, genlmsg_len(gnlh), genlmsg_attrlen(gnlh, 0));
	}

	// copy the attrs blob to the caller
	size_t buflen = genlmsg_attrlen(gnlh, 0);
	struct nlattr *ptr = genlmsg_attrdata(gnlh, 0);
	// TODO get rid of malloc
	// XXX why was I memcpy-ing these out again?  Why can't I decode them here?
	// IIRC this file was originally meant to be pure C so I could do the NLA
	// decode in C++. But then I converted this file to C++ so now it's stupid
	// extra work to do this malloc+memcpy
	struct nlattr *buf = static_cast<struct nlattr *>(malloc(buflen));
	if (buf) {
		memcpy(buf, ptr, buflen);
		attrs->attr_list[attrs->counter] = static_cast<struct nlattr *>(buf);
		attrs->attr_len_list[attrs->counter] = buflen;
		attrs->counter++;
	}
	else {
		// TODO report malloc failure
		assert(0);
	}

	return NL_SKIP;

}

int iw_get_scan(struct nl80211_state* state, const char *ifname, struct nlattr_list *scan_attrs)
{
	/* SCAN -- THE BIG KAHOONA! */

	// FIXME use state's callback ptr
	// Currently don't need it here because GET_SCAN is stateless.

	unsigned int ifidx = if_nametoindex(ifname);
	if (ifidx <= 0) {
		return errno;
	}
	if (iw_debug) {
		printf("%s ifidx=%u\n", __func__, ifidx);
	}

	struct nl_cb* cb;
	struct nl_cb* s_cb;

	cb = nl_cb_alloc(iw_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
	s_cb = nl_cb_alloc(iw_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);


	/* davep 20190228 ; copied from iw.c */
	int err=1;
	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
//	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, NULL);

	nl_socket_set_cb(state->nl_sock, s_cb);

	struct nl_msg* msg = nlmsg_alloc();
	// TODO check for failure

	void* p = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, state->nl80211_id, 0, 
						NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
	if (!p) {
		/* "Returns Pointer to user header or NULL if an error occurred." */
		fprintf(stderr, "genlmsg_put failed somehow\n");
//		goto leave;
		assert(0);
	}

	nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifidx);

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, (void *)scan_attrs);

	int retcode = nl_send_auto(state->nl_sock, msg);
//	printf("%s nl_send_auto retcode=%d\n", __func__, retcode);

	assert(err==1);
	while (err > 0) {
//		printf("%s GET_SCAN calling nl_recvmsgs_default...\n", __func__);
		retcode = nl_recvmsgs(state->nl_sock, cb);
//		printf("%s GET_SCAN retcode=%d err=%d counter=%ld\n", 
//				__func__, retcode, err, scan_attrs->counter);
	}

	nlmsg_free(msg);
	msg = NULL;

	nl_cb_put(cb);
	nl_cb_put(s_cb);

//	for (size_t i=0 ; i<scan_attrs->counter ; i++) {
//		struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
//		retcode = nla_parse(tb_msg, NL80211_ATTR_MAX, 
//					scan_attrs->attr_list[i],
//			  				scan_attrs->attr_len_list[i], NULL);
//		// TODO add a correct failure check
//		assert(retcode==0);
//
//		decode_attr_bss(tb_msg[NL80211_ATTR_BSS]);
//	}

	return 0;
}

/* from iw event.c */
static int scan_event_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr_list *attrs = (struct nlattr_list*)arg;

//	printf("%s %p %p\n", __func__, (void *)msg, arg);

	struct nlmsghdr *hdr = nlmsg_hdr(msg);

	int datalen = nlmsg_datalen(hdr);
	printf("datalen=%d attrlen=%d\n", datalen, nlmsg_attrlen(hdr,0));

//	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];

	struct genlmsghdr *gnlh = static_cast<struct genlmsghdr*>(nlmsg_data(nlmsg_hdr(msg)));
#if 0
	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	printf("%s cmd=%d\n", __func__, (int)(gnlh->cmd));

	// report attrs not handled in my crappy code below
	ssize_t counter=0;

	for (int i=0 ; i<NL80211_ATTR_MAX ; i++ ) {
		if (tb_msg[i]) {
			printf("%s %d=%p type=%d len=%d\n", __func__, i, (void *)tb_msg[i], nla_type(tb_msg[i]), nla_len(tb_msg[i]));
			counter++;
		}
	}
#endif
	// copy the nlattr blob to the caller
	size_t buflen = genlmsg_attrlen(gnlh, 0);
	struct nlattr *ptr = genlmsg_attrdata(gnlh, 0);
	struct nlattr *buf = static_cast<struct nlattr *>(malloc(buflen));
	if (buf) {
		memcpy(buf, ptr, buflen);
		attrs->attr_list[attrs->counter] = static_cast<struct nlattr *>(buf);
		attrs->attr_len_list[attrs->counter] = buflen;
		attrs->counter++;
	}
	else {
		// TODO report malloc failure
		assert(0);
	}

	// TODO NL_SKIP vs NL_OK ?????  I really would like to understand this
	return NL_SKIP;
}

static int no_seq_check(struct nl_msg *msg, void *arg)
{
	(void)msg;
	(void)arg;

	return NL_OK;
}

/* from iw genl.c */
struct handler_args {
	const char *group;
	int id;
};

/* from iw genl.c */
static int family_handler(struct nl_msg *msg, void *arg)
{
	struct handler_args *grp = static_cast<struct handler_args*>(arg);
	struct nlattr *tb[CTRL_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = static_cast<struct genlmsghdr*>(nlmsg_data(nlmsg_hdr(msg)));
	struct nlattr *mcgrp;
	int rem_mcgrp;

	printf("%s\n", __func__);
	nla_parse(tb, CTRL_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[CTRL_ATTR_MCAST_GROUPS])
		return NL_SKIP;

	my_nla_for_each_nested(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], rem_mcgrp) {
		struct nlattr *tb_mcgrp[CTRL_ATTR_MCAST_GRP_MAX + 1];

		nla_parse(tb_mcgrp, CTRL_ATTR_MCAST_GRP_MAX,
			  static_cast<struct nlattr*>(nla_data(mcgrp)), nla_len(mcgrp), NULL);

		if (!tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME] ||
		    !tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID])
			continue;
		if (strncmp(static_cast<char*>(nla_data(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME])),
			    grp->group, nla_len(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME])))
			continue;
		grp->id = nla_get_u32(tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID]);
		break;
	}

	return NL_SKIP;
}

/* from iw genl.c but I renamed from nl_get_multicast_id() because it was
 * confusing me thinking it was part of the libnl. 
 */
int iw_get_multicast_id(struct nl_sock *sock, const char *family, const char *group)
{
	struct nl_msg *msg;
	struct nl_cb *cb;
	int ret, ctrlid;
	struct handler_args grp = {
//		.group = group,
//		.id = -ENOENT,
	};

	grp.group = group;
	grp.id = -ENOENT;

	msg = nlmsg_alloc();
	if (!msg)
		return -ENOMEM;

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		ret = -ENOMEM;
		goto out_fail_cb;
	}

	ctrlid = genl_ctrl_resolve(sock, "nlctrl");

	genlmsg_put(msg, 0, 0, ctrlid, 0,
		    0, CTRL_CMD_GETFAMILY, 0);

	ret = -ENOBUFS;
	NLA_PUT_STRING(msg, CTRL_ATTR_FAMILY_NAME, family);

	ret = nl_send_auto_complete(sock, msg);
	if (ret < 0)
		goto out;

	ret = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &ret);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &ret);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, family_handler, &grp);

	while (ret > 0)
		nl_recvmsgs(sock, cb);

	if (ret == 0)
		ret = grp.id;
 nla_put_failure:
 out:
	nl_cb_put(cb);
 out_fail_cb:
	nlmsg_free(msg);
	return ret;
}

int iw_listen_scan_events(struct nl80211_state* state)
{
	printf("%s\n", __func__);

	// set up to listen for scan events
	int mcid = iw_get_multicast_id(state->nl_sock, "nl80211", "scan");
	if (mcid < 0) {
		fprintf(stderr, "iw_get_multicast_id errno=%d \"%s\"\n", mcid, strerror(mcid));
		// TODO create an err.h for useful error numbers
		return -1;
	}

	int err = nl_socket_add_membership(state->nl_sock, mcid);
	if (err < 0) {
		fprintf(stderr, "nl_socket_add_membership failed err=%d\n", err);
		// TODO err.h for useful error numbers
		return -1;
	}

	/* from iw event.c (TODO not sure why it's used)*/
	/* no sequence checking for multicast messages */
	err = nl_cb_set(state->cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
	if (err < 0) {
		// TODO complain loudly
		return err;
	}

	return 0;
}

int iw_fetch_scan_events(struct nl80211_state* state, struct nlattr_list* evt_attrs)
{
	int err;

	printf("%s\n", __func__);

	err = nl_cb_set(state->cb, NL_CB_VALID, NL_CB_CUSTOM, scan_event_handler, evt_attrs);
	if (err < 0) {
		// TODO complain loudly
		return err;
	}

	printf("%s calling nl_recvmsgs...\n", __func__);
	err = nl_recvmsgs(state->nl_sock, state->cb);
	if (err < 0) {
		fprintf(stderr, "%s nl_recvmsgs err=%d\n", __func__, err);
		return err;
	}
	return 0;
}

