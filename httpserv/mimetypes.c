/*
 * mimetypes.h  pure C parse /etc/mime.types into glibc hash table
 *
 * (work in progress)
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <search.h>

#include "mimetypes.h"

enum State {
	START = 1,
	TYPE,
	SEEK_EXTENSION,
	EXTENSION,
	ERROR
} ;

static const char LF = 0x0a;
static const char CR = 0x0d;
static const char SP = ' ';
static const char HT = '\t';

static enum State eat_eol(FILE* infile)
{
	char c = fgetc(infile);
	if ( feof(infile) || c != LF) {
		return ERROR;
	}
	return START;
}

static enum State eat_line(FILE* infile)
{
	// read chars until end of line
	while ( !feof(infile) ) {
		char c = fgetc(infile);

		if (c==LF) {
			break;
		}
		else if (c==CR) {
			// handle CRLF
			return eat_eol(infile);
		}
	}

	return START;
}


static int save(const char* type, const char* ext, struct hsearch_data* htab)
{
	int err;
	ENTRY entry, *found;

	printf("%s %s %s\n", __func__, type, ext);

	entry.key = strdup(ext);
	entry.data = strdup(type);

	err = hsearch_r(entry, ENTER, &found, htab);

	return err;
}

int mimetype_parse(const char* infilename, struct hsearch_data* htab )
{
	int err;
	FILE* infile;

	infile = fopen(infilename, "r");
	if (!infile) {
		return -errno;
	}

	enum State state = START;

	char type[128];
	char ext[128];
	int type_idx=0, ext_idx=0;

	memset(type,0,sizeof(type));
	memset(ext,0,sizeof(ext));

	while (1) {
#ifdef DEBUG
		dbg_cout_file(infile);
#endif
		char c = fgetc(infile);
		if (feof(infile)) {
			break;
		}

//		dbg_cout_file(infile);

		switch (state) {
			case START:
				if (c=='#') {
					state = eat_line(infile);
				}
				else if (c==CR) {
					state = eat_eol(infile);
				}
				else if (c==LF || c==SP || c==HT) {
				}
				else {
//					std::cout << "State::TYPE start\n";
					memset(type,0,sizeof(type));
					type_idx = 0;
					type[type_idx++] = c;
					state = TYPE;
				}
				break;

			case TYPE:
				if (c==CR) {
					// end of type
					state = eat_eol(infile);
				}
				else if (c==LF) {
					// end of type
					state = START;
				}
				else if (c==LF || c==SP || c==HT) {
					// end of type
					state = SEEK_EXTENSION;
				}
				else {
					memset(ext,0,sizeof(ext));
					ext_idx = 0;
					type[type_idx++] = c;
				}
				break;

			case SEEK_EXTENSION:
				if (c==CR) {
					// end of line
					state = eat_eol(infile);
				}
				else if (c==LF) {
					// end of line
					state = START;
				}
				else if (!(c==SP || c==HT)) {
					ext_idx = 0;
					memset(ext,0,sizeof(ext));
					ext[ext_idx++] = c;
					state = EXTENSION;
				}
				break;

			case EXTENSION:
				if (c==CR) {
					// end of line; save extension
//					mt[ext] = type;
					err = save(type, ext, htab);
					printf("%s=%s\n", ext, type);
					state = eat_eol(infile);
				}
				else if (c==LF) {
					// end of line; save extension
//					mt[ext] = type;
					err = save(type, ext, htab);
					printf("%s=%s\n", ext, type);
					state = START;
				}
				else if (c==SP || c==HT) {
					// end of this extension
//					mt[ext] = type;
					err = save(type, ext, htab);
					printf("%s=%s\n", ext, type);
					state = SEEK_EXTENSION;
				}
				else {
					ext[ext_idx++] = c;
				}
				break;

			case ERROR:
				break;
		}
	}

	fclose(infile);
	return 0;
}

#ifdef TEST_ME
int main(void)
{
	int ret;
	struct hsearch_data htab;

	memset(&htab, 0, sizeof(struct hsearch_data));

	ret = hcreate_r(1024, &htab);

	ret = mimetype_parse("/etc/mime.types", &htab);

	ENTRY entry, *found;
	entry.key = "html";
	entry.data = NULL;
	ret = hsearch_r(entry, FIND, &found, &htab);
	if (ret) {
		printf("%s=%s\n", entry.key, (char*)found->data);
	}

	hdestroy_r(&htab);

	return 0;
}
#endif

