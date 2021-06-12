/*
 * mimetypes.h  pure C parse /etc/mime.types into glibc hash table
 *
 * (work in progress)
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef MIMETYPES_H
#define MIMETYPES_H

#include <search.h>

int mimetype_parse(const char* infilename, struct hsearch_data* htab );

#endif

