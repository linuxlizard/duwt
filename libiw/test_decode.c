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
U_STRING_DECL(u_maxan_anvol, "Maxan Anvol", 11);
U_STRING_DECL(u_e300, "E300-7e2-5g", 11);

static int decode(struct nlattr* attr, size_t attrlen, struct BSS** p_bss)
{
	struct BSS* bss = NULL;

	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
	nla_parse(tb_msg, NL80211_ATTR_MAX, attr, attrlen, NULL);

	peek_nla_attr(tb_msg, NL80211_ATTR_MAX);

	int err=0;
	err = bss_from_nlattr(tb_msg, &bss);
	if (err) {
		goto fail;
	}

	PTR_ASSIGN(*p_bss, bss);

	return 0;
fail:
	return -1;
}

static int load_file(const char* filename, uint8_t** p_buf, size_t* p_size)
{
	struct stat stats;
	int err =  stat(filename, &stats);
	if (err<0) {
		fprintf(stderr, "stat file \"%s\" failed err=%d %s\n", filename, errno, strerror(errno));
		return err;
	}

	uint8_t* buf = malloc(stats.st_size);
	if (!buf) {
		return -ENOMEM;
	}

	int fd = open(filename, O_RDONLY);
	if (fd<0) {
		PTR_FREE(buf);
		fprintf(stderr, "open file \"%s\" failed err=%d %s\n", filename, errno, strerror(errno));
		return -errno;
	}

	ssize_t count = read(fd, buf, stats.st_size);
	if (count < 0) {
		PTR_FREE(buf);
		fprintf(stderr, "read file \"%s\" failed err=%d %s\n", filename, errno, strerror(errno));
		return -errno;
	}
	close(fd);

	if (count != stats.st_size) {
		PTR_FREE(buf);
		return -EIO;
	}

	PTR_ASSIGN(*p_buf, buf);
	*p_size = stats.st_size;
	return 0;
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

	UChar u_ssid[SSID_MAX_LEN*2];
	int ret = ssid_to_unicode_str(ssid_ie->ssid, ssid_ie->ssid_len, u_ssid, sizeof(u_ssid));
	XASSERT(ret>=0, ret);

	if (u_strcmp(u_ssid, u_maxan_anvol) == 0) {
		verify_maxan_anvol(bss);
	}
	else if (u_strcmp(u_ssid, u_e300) == 0) {
		verify_e300(bss);
	}

	char s[64];
	ret = bss_get_chan_width_str(bss, s, sizeof(s));
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
//	hex_dump(__func__, (unsigned char*)attr, len);

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

	int err = args_parse(argc, argv, &args);

	if (args.debug > 0) {
		log_set_level(LOG_LEVEL_DEBUG);
	}

	DBG("%s this is a debug message\n", __func__);
	INFO("%s this is an info message\n", __func__);
	WARN("%s this is a warning message\n", __func__);
	ERR("%s this is an error message\n", __func__);

	U_STRING_INIT(u_maxan_anvol, "Maxan Anvol", 11);
	U_STRING_INIT(u_e300, "E300-7e2-5g", 11);

	for (i=0 ; i<args.argc ; i++) {
		uint8_t* buf;
		size_t size;

		err = load_file(args.argv[i], &buf, &size);
		if (err < 0) {
			fprintf(stderr, "failed to load file \"%s\"; err=%d\n", args.argv[i], err);
			continue;
		}

		// work in progress
		test_encode(buf, size);

		struct nlattr* attr = (struct nlattr*)buf;

		struct BSS* bss;

		err = decode(attr, size, &bss);
		XASSERT(err==0, err);

		verify_bss(bss);

		PTR_FREE(buf);

		json_t* jbss = NULL;
		err = bss_to_json_summary(bss, &jbss);
		XASSERT(err==0, err);
		char* s = json_dumps(jbss, JSON_INDENT(1));
		printf("%s\n", s);
		PTR_FREE(s);
		json_decref(jbss);

		bss_free(&bss);
	}

	return 0;
}

