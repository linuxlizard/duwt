/*
 * libiw/hdump.c   useful hexdump I've hauled around for years
 *
 * Copyright (c) 199?-2020 David Poole <davep@mbuf.com>
 */

#ifndef HDUMP_H
#define HDUMP_H

void hex_dump( const char *label, const unsigned char *ptr, int size );

#endif

