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
#include "iw.h"
#include "bss.h"

static int decode(struct nlattr* attr, size_t attrlen, struct BSS** p_bss)
{
	struct BSS* bss = NULL;

	INFO("%s\n", __func__);
//	peek_nl_msg(msg);
//
//	INFO("%s max_size=%zu proto=%d\n", __func__,
//			nlmsg_get_max_size(msg), 
//			nlmsg_get_proto(msg));
//
//	struct nlmsghdr *hdr = nlmsg_hdr(msg);
//
//	struct genlmsghdr* gnlh = (struct genlmsghdr*)nlmsg_data(hdr);

	struct nlattr* tb_msg[NL80211_ATTR_MAX + 1];
	nla_parse(tb_msg, NL80211_ATTR_MAX, attr, attrlen, NULL);

	peek_nla_attr(tb_msg, NL80211_ATTR_MAX);

	int err=0;
	err = bss_from_nlattr(tb_msg, &bss);
	if (err) {
		goto fail;
	}

	PTR_ASSIGN(*p_bss, bss);

	DBG("%s success\n", __func__);
	return NL_OK;
fail:
	return NL_SKIP;
}

static int load_file(const char* filename, uint8_t** p_buf, size_t* p_size)
{
	struct stat stats;
	int err =  stat(filename, &stats);
	if (err<0) {
		fprintf(stderr, "stat %s failed err=%d %s\n", filename, errno, strerror(errno));
		return err;
	}

	uint8_t* buf = malloc(stats.st_size);
	if (!buf) {
		return -ENOMEM;
	}

	int fd = open(filename, O_RDONLY);
	if (fd<0) {
		PTR_FREE(buf);
		fprintf(stderr, "open %s failed err=%d %s\n", filename, errno, strerror(errno));
		return -errno;
	}

	ssize_t count = read(fd, buf, stats.st_size);
	if (count < 0) {
		PTR_FREE(buf);
		fprintf(stderr, "read %s failed err=%d %s\n", filename, errno, strerror(errno));
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

int main(int argc, char* argv[])
{
	int i;

	for (i=1 ; i<argc ; i++) {
		uint8_t* buf;
		size_t size;

		int err = load_file(argv[i], &buf, &size);
		if (err < 0) {
			fprintf(stderr, "failed to load %s; err=%d\n", argv[i], err);
			continue;
		}

		struct nlattr* attr = (struct nlattr*)buf;

		struct BSS* bss;

		decode(attr, size, &bss);

		PTR_FREE(buf);
	}

	return 0;
}

