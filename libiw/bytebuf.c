#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

void bytebuf_array_verify(struct bytebuf_array* bba)
{
	XASSERT(bba != NULL, 0);
	XASSERT( bba->cookie == BYTEBUF_ARRAY_COOKIE, bba->cookie);
}

int bytebuf_array_init(struct bytebuf_array* bba, size_t max)
{
	memset(bba, 0, sizeof(struct bytebuf_array));

	bba->list = (struct bytebuf *)calloc(max, sizeof(struct bytebuf));
	if (!bba->list) {
		return -ENOMEM;
	}
	bba->cookie = BYTEBUF_ARRAY_COOKIE;
	bba->max = max;
	bba->len = 0;

	return 0;
}

void bytebuf_array_free(struct bytebuf_array* bba)
{
	XASSERT( bba->cookie == BYTEBUF_ARRAY_COOKIE, bba->cookie);

	for (size_t i=0 ; i<bba->max; i++) {
		if (bba->list[i].cookie ) {
			bytebuf_free(&bba->list[i]);
		}
	}

	PTR_FREE(bba->list);
	memset(bba, 0, sizeof(struct bytebuf_array));
}

int bytebuf_array_emplace_back(struct bytebuf_array* bba, uint8_t* ptr, size_t len)
{
	XASSERT( bba->cookie == BYTEBUF_ARRAY_COOKIE, bba->cookie);

	if (bba->len+1 > bba->max) {
		return -ENOMEM;
	}

	int err = bytebuf_init(&bba->list[bba->len], ptr, len);
	if (err) {
		return err;
	}
	bba->len++;

	return 0;
}

