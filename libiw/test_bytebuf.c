#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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
	XASSERT(bb.buf == 0, (uintmax_t)bb.buf);

	struct bytebuf_array bba;
	err = bytebuf_array_init(&bba, 64);
	XASSERT(err==0, err);
	XASSERT(bba.cookie == 0xd80e50c6, bba.cookie);
	XASSERT(bba.max==64, bba.max);
	XASSERT(bba.len==0, bba.len);

	for (size_t i=0 ; i<64 ; i++) {
		err = bytebuf_array_emplace_back(&bba, hello, 13);
		XASSERT(err==0, err);
		XASSERT(bba.len==i+1, bba.len);
	}

	err = bytebuf_array_emplace_back(&bba, hello, 13);
	XASSERT(err==-ENOMEM, err);
	XASSERT(bba.len==64, bba.len);

	struct bytebuf* pbb = NULL;
	printf("list=%p\n", (void *)bba.list);
	bytebuf_array_for_each(bba, pbb) {
//		printf("pbb=%p\n", (void *)pbb);
		if (pbb->len) {
			XASSERT(pbb->len == 13, pbb->len);
		}
		else {
			XASSERT(pbb->buf == NULL, (uintmax_t)pbb->buf);
		}
	}
	bytebuf_array_free(&bba);
	XASSERT(bba.cookie == 0, bba.cookie);

	return EXIT_SUCCESS;
}

