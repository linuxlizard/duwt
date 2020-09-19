/*
 * libiw/test_decode.c   test dumpfile functions
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/netlink.h>
#include <netlink/attr.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>

#include "core.h"
#include "args.h"
#include "iw.h"
#include "bss.h"
#include "bss_json.h"
#include "ssid.h"

// known SSIDs from test dumps
//U_STRING_DECL(u_maxan_anvol, "Maxan Anvol", 11);
//U_STRING_DECL(u_e300, "E300-7e2-5g", 11);

static int decode(uint8_t* buf, ssize_t buflen, struct dl_list* bss_list)
{
	struct BSS* bss = NULL;
	struct nlattr* attr=NULL;
	int attrlen;

//	hex_dump(__func__, (unsigned char*)buf, 1024);

	uint8_t* endbuf = buf + buflen;
	while( buflen > 0) {
		// my silly little file format has a uint32_t in host endian order
		// stamped before each genlmsg 
		XASSERT (*(uint32_t*)buf == 0x64617665, *(uint32_t*)buf);
		buf += sizeof(uint32_t);

		// data length
		uint32_t len = *(uint32_t*)buf;
		hex_dump(__func__, buf, len);
		printf("blob len=%zu remain=%zu\n", len, buflen);
		buf += sizeof(uint32_t);
		XASSERT(buf < endbuf, (endbuf-buf));

		attrlen = (int)len;
		attr = (struct nlattr*)buf;
		buf += len;
		buflen -= len + 8;
		ERR("%s buf=%p end=%p buflen=%zd\n", __func__, buf, endbuf, buflen);

		struct nlattr* nla;
		int rem;
		nla_for_each_attr(nla, attr, attrlen, rem) {
			printf("type=%d len=%d rem=%d\n", nla_type(nla), nla_len(nla), rem);
		}

		struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
		int err = nla_parse(tb_msg, NL80211_ATTR_MAX, attr, attrlen, NULL);
		XASSERT(err==0, err);

		peek_nla_attr(tb_msg, NL80211_ATTR_MAX);

		err = bss_from_nlattr(tb_msg, &bss);
		if (err) {
			goto fail;
		}

		dl_list_add_tail(bss_list, &bss->node);
	}

	return 0;
fail:
	return -1;
}

static int load_scan_dump_file(const char* filename, uint8_t** p_buf, size_t* p_size)
{
	struct stat stats;
	uint8_t *buf = NULL;

	int err =  stat(filename, &stats);
	if (err<0) {
		// preserve errno
		err = errno;
		fprintf(stderr, "stat file \"%s\" failed err=%d %s\n", filename, errno, strerror(errno));
		return err;
	}

	int fd = open(filename, O_RDONLY);
	if (fd<0) {
		err = errno;
		fprintf(stderr, "open file \"%s\" failed err=%d %s\n", filename, errno, strerror(errno));
		return -err;
	}

	// skip my header which is free-form bytes up until a \n (0x0a)
	ssize_t count, header_size=0;
	unsigned char c;
	do {
		count = read(fd, &c, 1);
		if (count < 0) {
			err = errno;
			goto fail;
		}
		header_size++;
	} while (c != 0x0a);

	size_t data_size = stats.st_size - header_size;
	buf = malloc(data_size);
	if (!buf) {
		err = -ENOMEM;
		goto fail;
	}

	// read rest of file
	count = read(fd, buf, data_size);
	if (count < 0) {
		err = errno;
		fprintf(stderr, "read file \"%s\" failed err=%d %s\n", filename, errno, strerror(errno));
		goto fail;
	}

	if (count+header_size != stats.st_size) {
		err = EINVAL;
		goto fail;
	}

	PTR_ASSIGN(*p_buf, buf);
	*p_size = count;
	close(fd);
	DBG("header_size=%zu count=%zu\n", header_size, count);
	return 0;

fail:
	close(fd);
	if (buf) {
		PTR_FREE(buf);
	}
	return -err;
}

static void verify_maxan_anvol(const struct BSS* bss)
{
	INFO("%s\n", __func__);

	XASSERT(bss->band == NL80211_BAND_5GHZ, bss->band);
	XASSERT(bss->frequency ==5765, bss->frequency );
	XASSERT(bss->signal_strength_mbm == -3000, bss->signal_strength_mbm);

	const struct IE* const ht_ie = ie_list_find_id(&bss->ie_list, IE_HT_CAPABILITIES);
	const struct IE* const vht_ie = ie_list_find_id(&bss->ie_list, IE_VHT_CAPABILITIES);
	const struct IE* const he_ie = ie_list_find_ext_id(&bss->ie_list, IE_EXT_HE_CAPABILITIES );

	XASSERT(ht_ie, 0);
	XASSERT(vht_ie, 0);
	XASSERT(!he_ie, 0);

	char s[64];
	int ret = bss_get_mode_str(bss, s, sizeof(s));
	XASSERT(ret > 0, ret);
	XASSERT(strncmp(s, "a/n/ac", 9) == 0, 0);
}

static void verify_e300(const struct BSS* bss)
{
	INFO("%s\n", __func__);

	XASSERT(bss->band == NL80211_BAND_5GHZ, bss->band);
	XASSERT(bss->frequency == 5720, bss->frequency );
	XASSERT(bss->signal_strength_mbm == -3700, bss->signal_strength_mbm);

	const struct IE* const ht_ie = ie_list_find_id(&bss->ie_list, IE_HT_CAPABILITIES);
	const struct IE* const vht_ie = ie_list_find_id(&bss->ie_list, IE_VHT_CAPABILITIES);
	const struct IE* const he_ie_capa = ie_list_find_ext_id(&bss->ie_list, IE_EXT_HE_CAPABILITIES );
	const struct IE* const he_ie_op = ie_list_find_ext_id(&bss->ie_list, IE_EXT_HE_OPERATION);

	XASSERT(ht_ie, 0);
	XASSERT(vht_ie, 0);
	XASSERT(he_ie_capa, 0);
	XASSERT(he_ie_op, 0);

	char s[64];
	int ret = bss_get_mode_str(bss, s, sizeof(s));
	XASSERT(ret > 0, ret);
	XASSERT(strncmp(s, "a/n/ac/ax", 9) == 0, 0);
}

static void verify_bss(const struct BSS* bss)
{
	XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);
	INFO("%s %s\n", __func__, bss->bssid_str);

	const struct IE_SSID* ssid_ie = bss_get_ssid(bss);
	XASSERT(ssid_ie, 0);

//	UChar u_ssid[SSID_MAX_LEN*2];
//	int ret = ssid_to_unicode_str(ssid_ie->ssid, ssid_ie->ssid_len, u_ssid, sizeof(u_ssid));
//	XASSERT(ret>=0, ret);

#if 0
	if (u_strcmp(u_ssid, u_maxan_anvol) == 0) {
		verify_maxan_anvol(bss);
	}
	else if (u_strcmp(u_ssid, u_e300) == 0) {
		verify_e300(bss);
	}
#endif

	char s[64];
	int ret = bss_get_chan_width_str(bss, s, sizeof(s));
	INFO("%s %s width=%s\n", __func__, bss->bssid_str, s);
}

static void test_encode(uint8_t* buf, size_t buf_len)
{
	DBG("%s\n", __func__);

	// can I rebuild an nl_msg containing buf as a payload?
	struct nl_msg* msg = nlmsg_alloc();

	int nl80211_id = 0;

	void* p = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, 
						nl80211_id, 
						0, 
						NLM_F_DUMP, NL80211_CMD_NEW_SCAN_RESULTS, 0);
	(void)p;

	struct nlattr* attr = (struct nlattr*)buf;
	size_t attrlen = buf_len;
	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
	int err = nla_parse(tb_msg, NL80211_ATTR_MAX, attr, attrlen, NULL);
	XASSERT(err==0, err);

	for (size_t i=0 ; i<NL80211_ATTR_MAX ; i++ ) {
		if (tb_msg[i]) {
			err = nla_put(msg, 
							nla_type(tb_msg[i]), 
							nla_len(tb_msg[i]), 
							(void *)nla_data(tb_msg[i]));
			XASSERT(err==0, err);
		}
	}
//	int err = nla_put(msg, 0, buf_len, buf);
//	XASSERT(err==0, err);

	// now let's try taking it apart again
	struct nlmsghdr *hdr = nlmsg_hdr(msg);
	struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(hdr);

	size_t len = genlmsg_attrlen(gnlh,0);
	attr = genlmsg_attrdata(gnlh,0);

	DBG("%s buflen=%zu buf=%p len=%zu attr=%p\n", __func__, 
			buf_len, (void*)buf,
			len, (void*)attr);
	hex_dump(__func__, (unsigned char*)attr, len);

//	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
	err = nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);
	XASSERT(err==0, err);

	peek_nla_attr(tb_msg, NL80211_ATTR_MAX);
	XASSERT(tb_msg[NL80211_ATTR_BSS], NL80211_ATTR_BSS);
}


int main(int argc, char* argv[])
{
	int i;
	struct args args;

	int err=0;
//	int err = args_parse(argc, argv, &args);

	if (args.debug > 0) {
		log_set_level(LOG_LEVEL_DEBUG);
	}

	DBG("%s this is a debug message\n", __func__);
	INFO("%s this is an info message\n", __func__);
	WARN("%s this is a warning message\n", __func__);
	ERR("%s this is an error message\n", __func__);

//	U_STRING_INIT(u_maxan_anvol, "Maxan Anvol", 11);
//	U_STRING_INIT(u_e300, "E300-7e2-5g", 11);

	DEFINE_DL_LIST(bss_list);

	for (i=1 ; i<argc ; i++) {
		uint8_t* buf;
		size_t size;

		err = load_scan_dump_file(argv[i], &buf, &size);
		if (err < 0) {
			fprintf(stderr, "failed to load file \"%s\"; err=%d\n", args.argv[i], err);
			continue;
		}

		// work in progress
//		test_encode(buf, size);

		err = decode(buf, size, &bss_list);
		XASSERT(err==0, err);

		PTR_FREE(buf);

		struct BSS* bss;
		dl_list_for_each(bss, &bss_list, struct BSS, node) {
			verify_bss(bss);

			json_t* jbss = NULL;
			err = bss_to_json_summary(bss, &jbss);
			XASSERT(err==0, err);
			char* s = json_dumps(jbss, JSON_INDENT(1));
			printf("%s\n", s);
			PTR_FREE(s);
			json_decref(jbss);
		}

		bss_free_list(&bss_list);
	}

	return 0;
}

