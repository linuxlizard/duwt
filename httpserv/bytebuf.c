/*
 * bytebuf  blob of unit8_t with size information
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "core.h"
#include "xassert.h"
#include "log.h"
#include "bytebuf.h"

int bytebuf_init(struct bytebuf* bb, uint8_t *ptr, size_t len)
{
	DBG("%s %p %zu\n", __func__, (void*)ptr, len);
	if (!bb || !ptr) {
		return -EINVAL;
	}

	memset(bb, 0, sizeof(struct bytebuf));
	bb->cookie = BYTEBUF_COOKIE;
	if (len) {
		bb->buf = malloc(len);
		if (!bb->buf) {
			return -ENOMEM;
		}
		memcpy(bb->buf, ptr, len);
	}
	bb->len = len;

	return 0;
}

int bytebuf_init_from_file(struct bytebuf* bb, const char* path)
{
	int ret;
	struct stat stbuf;

	memset(&stbuf, 0, sizeof(struct stat));

	ret = stat(path, &stbuf);
	if (ret != 0) {
		ret = -errno;
		ERR("%s path=%s stat failed err=%d %s\n", __func__,
				path, errno, strerror(errno));
		return ret;
	}

	// put an upper limit on file size to prevent stupid mistakes causing
	// memory thrashing
	if (stbuf.st_size > BYTEBUF_SANITY_MAX ) {
		return -E2BIG;
	}

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		ret = -errno;
		ERR("%s path=%s open failed err=%d %s\n", __func__,
				path, errno, strerror(errno));
		return ret;
	}

	memset(bb, 0, sizeof(struct bytebuf));
	bb->buf = malloc(stbuf.st_size);
	if (!bb->buf) {
		return -ENOMEM;
	}

	uint8_t* ptr = bb->buf;
	uint8_t* endptr = ptr + stbuf.st_size;
	size_t remain = stbuf.st_size;

	while (ptr < endptr) {
		ssize_t count = read(fd, ptr, remain);
		if (count < 0) {
			// something went wrong
			PTR_FREE(bb->buf);
		}
		ptr += count;
		remain -= count;
	}

	bb->cookie = BYTEBUF_COOKIE;
	bb->len = stbuf.st_size;

	return 0;
}

void bytebuf_free(struct bytebuf* bb)
{
	XASSERT(bb, 0);
	XASSERT(bb->cookie == BYTEBUF_COOKIE, bb->cookie);

	if (bb->buf) {
		/* poison the buffer */
		memset(bb->buf, 0xee, bb->len);
		PTR_FREE(bb->buf);
	}
	memset(bb, 0, sizeof(struct bytebuf));
}

