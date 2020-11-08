/*
 * bytebuf  blob of unit8_t with size information
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef BYTEBUF
#define BYTEBUF

#define BYTEBUF_COOKIE 0xa6e59014

// put a limit on bytebuf size to avoid accidental memory overuse
#define BYTEBUF_SANITY_MAX 10*1024*1024

// uninterpreted, unverified, unknown blob of bytes that may or may not be
// valid utf8
struct bytebuf
{
	uint32_t cookie;
	uint8_t *buf;
	size_t len;
};

int bytebuf_init(struct bytebuf* byteb, uint8_t *ptr, size_t len);
void bytebuf_free(struct bytebuf* byteb);
int bytebuf_init_from_file(struct bytebuf* byteb, const char* path);

#endif

