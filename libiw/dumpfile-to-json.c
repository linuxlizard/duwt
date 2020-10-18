#/*
 * libiw/dumpfile_to_json.c   read a dumpfile, write it all to parsable json
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <getopt.h>

#include "core.h"
#include "iw.h"
#include "dumpfile.h"
#include "bss_json.h"


static json_t* mkjson(const char* label, struct dl_list* bss_list, unsigned int flags)
{
	json_t* jobj = json_object();
	XASSERT(jobj,0);

	json_t* jarray;
	int err = bss_list_to_json(bss_list, &jarray, flags);
	XASSERT(err==0, err);

	if (json_object_set_new_nocheck(jobj, "bss", jarray) < 0) {
		json_decref(jarray);
		goto fail;
	}

	json_t *jstr = json_string(label);
	if (!jstr) {
		goto fail;
	}

	if (json_object_set_nocheck(jobj, "filename", jstr)) {
		json_decref(jobj);
		goto fail;
	}

	return jobj;

fail:
	if (jobj) {
		json_decref(jobj);
	}
	return NULL;
}

static void dump_from_file(const char* dump_filename, unsigned int flags)
{
	int err = 0;

	DEFINE_DL_LIST(bss_list);

	err = dumpfile_parse(dump_filename, &bss_list);
	if (err != 0) {
		ERR("%s failed to parse \"%s\": %s\n", __func__, dump_filename, strerror(err));
		return;
	}

	json_t* jobj = mkjson(dump_filename, &bss_list, flags);
	if (jobj) {
		char* s = json_dumps(jobj, 0);
		printf("%s\n", s);
		PTR_FREE(s);
	}

	bss_free_list(&bss_list);
}

struct args {
	unsigned int flags;
	const char* filename_list[64];
	size_t filename_list_len;
};

static void show_help(void)
{
	printf("dumpfile-to-json ; convert scan-dump file(s) to JSON\n");
	printf("usage: dumpfile-to-json [-h][--short-ie] [DUMPFILE]...\n");
	printf("   --short-ie  ;  only display id,len,hexdump of each IE\n");
}

static int parse_args(int argc, char* argv[], struct args* args)
{
	memset(args, 0, sizeof(struct args));
	int ret;

	struct option long_options[] = {
		{"help", no_argument, 0, 0},
		{"short-ie", no_argument, 0, 0 },
		{0,0,0,0}
	};

	while(1) {
		int option_index = 0;

		ret = getopt_long(argc, argv, "h", long_options, &option_index);
		if (ret == -1) {
			break;
		}
		if (ret=='h' || option_index==0) {
			show_help();
		}
		else if (option_index == 1) {
			args->flags |= BSS_JSON_SHORT_IE_DECODE ;
		}
	}

	while (optind < argc) {
		args->filename_list[args->filename_list_len++] = argv[optind++];
	}

	return 0;
}

int main(int argc, char* argv[])
{
	struct args args;

	if (parse_args(argc, argv, &args) < 0) {
		return EXIT_FAILURE;
	}

	for (size_t i = 0 ; i<args.filename_list_len ; i++) {
		dump_from_file(args.filename_list[i], args.flags);
	}

	return EXIT_SUCCESS;
}

