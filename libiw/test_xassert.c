/*
 * libiw/test_xassert.c   test xassert functions
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "xassert.h"

int main(void)
{
	pid_t me = getpid();

	assert(me & 1);

	XASSERT( me & 1, me );
	XASSERT( !(me & 1), me );

	return EXIT_SUCCESS;
}


