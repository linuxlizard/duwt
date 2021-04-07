/*
 * davep 20210218 ; tinkering with jansson's limits
 *
 * gcc -g -Wall -Wextra -o test_jansson test_jansson.c -ljansson
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include <jansson.h>

int main(void)
{
	uint8_t num8;
	uint16_t num16;
	uint32_t num32;
	uint64_t num64;

	// peek at jansson and 64-bit integers
	json_int_t n = -1;
	printf("%s n=%"JSON_INTEGER_FORMAT" size=%zu\n", __func__, n, sizeof(n));
#ifdef JSON_INTEGER_IS_LONG_LONG
	printf("%s JSON_INTEGER_IS_LONG_LONG is defined\n", __func__);
#else
	printf("%s JSON_INTEGER_IS_LONG_LONG is NOT defined\n", __func__);
#endif

	// jansson uses signed long long for integers. Will that cause problems?
	num8 = num16 = num32 = num64 = 0;

	json_t *jnum;
	char *s;
	char *si, *sI;

	for (size_t i=0 ; i<8 ; i++) {
		num8 = 1u<<i;
		jnum = json_integer((json_int_t)num8);
		assert(jnum);
		s = json_dumps(json_pack("{si}", "num8", num8), 0);
		assert(s);
		printf("%"PRIu8 " 0x%"PRIx8 " %d %s\n", num8, num8, (int)num8, s);
	}

	for (size_t i=0 ; i<16 ; i++) {
		num16 = 1u<<i;
		jnum = json_integer(num16);
		assert(jnum);
		s = json_dumps(json_pack("{si}", "num16", num16), 0);
		assert(s);
		printf("%"PRIu16 " 0x%"PRIx16 " %d %s\n", num16, num16, (int)num16, s);
	}

	for (size_t i=0 ; i<32 ; i++) {
		num32 = 1ul<<i;
		jnum = json_integer(num32);
		assert(jnum);
		si = json_dumps(json_pack("{si}", "num32", num32), 0);
		sI = json_dumps(json_pack("{sI}", "num32", num32), 0);
		assert(s);
		printf("%"PRIu32 " 0x%"PRIx32 " %d si=%s sI=%s\n", num32, num32, (int)num32, si, sI);
	}

	num64 = 0ull;
	for (size_t i=0 ; i<64 ; i++) {
		num64 = 1ull<<i;
		jnum = json_integer(num64);
		assert(jnum);
		s = json_dumps(json_pack("{sI}", "num64", num64), 0);
		assert(s);
		printf("%"PRIu64 " 0x%"PRIx64 " %d %s\n", num64, num64, (int)num64, s);
	}

	return 0;
}

