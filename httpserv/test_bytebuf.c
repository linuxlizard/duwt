/*
 * test_bytebuf.c  test the bytebuf code
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <openssl/sha.h>

#include "xassert.h"
#include "bytebuf.h"
#include "hdump.h"

int main(void)
{
	struct bytebuf bb;

	uint8_t hello[] = "hello, world";

	int err = bytebuf_init(&bb, hello, 13);
	XASSERT(err==0, err);
	XASSERT(bb.len == 13, bb.len);
	hex_dump("hello", bb.buf, bb.len);
	XASSERT( !memcmp(bb.buf, hello, bb.len), 0);

	bytebuf_free(&bb);
	XASSERT(bb.cookie == 0, bb.cookie);
	XASSERT(bb.len == 0, bb.len);
	XASSERT(bb.buf == 0, 0);

	char testfile[FILENAME_MAX+1] = "/etc/mime.types";
	err = bytebuf_init_from_file(&bb, testfile);
	XASSERT(err==0, err);

	struct stat stbuf;
	memset(&stbuf, 0, sizeof(struct stat));
	err = stat(testfile, &stbuf);
	XASSERT(err==0, errno);
	XASSERT(bb.len == (size_t)stbuf.st_size, bb.len);
	// TODO verify the data
	// because I'm curious about how to use openssl libs 
	char hash[SHA512_DIGEST_LENGTH];
	SHA512(bb.buf, bb.len, hash);

	return EXIT_SUCCESS;
}
