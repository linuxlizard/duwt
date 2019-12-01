#ifndef BYTEBUF
#define BYTEBUF

#define BYTEBUF_COOKIE 0xa6e59014
#define BYTEBUF_ARRAY_COOKIE 0xd80e50c6

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

// array of bytebuf
struct bytebuf_array
{
	uint32_t cookie;
	struct bytebuf* list;
	size_t len;
	size_t max;
};

int bytebuf_array_init(struct bytebuf_array* bba, size_t len);
void bytebuf_array_free(struct bytebuf_array* list);
void bytebuf_array_verify(struct bytebuf_array* bba);
int bytebuf_array_emplace_back(struct bytebuf_array* bba, uint8_t* ptr, size_t len);

#define bytebuf_array_for_each(bba, pbb) \
	for(pbb=bba.list ; pbb<&bba.list[bba.len] ; pbb++)

#endif

