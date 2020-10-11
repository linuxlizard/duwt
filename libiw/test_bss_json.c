/*
 * libiw/test_bss_json.c   test BSS enconding as JSON using jansson
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "core.h"
#include "iw.h"
#include "args.h"
#include "dumpfile.h"
#include "bss_json.h"

static void json_summary(struct dl_list* bss_list)
{
	struct BSS* bss;
	dl_list_for_each(bss, bss_list, struct BSS, node) {
		json_t* jbss;
		int err = bss_to_json_summary(bss, &jbss);
		XASSERT(err==0, err);
		char* s = json_dumps(jbss, JSON_INDENT(3));
		printf("%s\n", s);
		PTR_FREE(s);
		json_decref(jbss);
	}
}

static void json_summary_list(struct dl_list* bss_list)
{
	json_t* jarray;
	int err = bss_list_to_json(bss_list, &jarray, bss_json_summary);
	XASSERT(err==0, err);
	char* s = json_dumps(jarray, 0);
	printf("%s\n", s);
	PTR_FREE(s);
	json_decref(jarray);
}

static void json_full_list(struct dl_list* bss_list)
{
	json_t* jarray;
	int err = bss_list_to_json(bss_list, &jarray, bss_json_summary);
	XASSERT(err==0, err);
	char* s = json_dumps(jarray, 0);
	printf("%s\n", s);
	PTR_FREE(s);
	json_decref(jarray);
}

static void json_full(struct dl_list* bss_list)
{
	struct BSS* bss;

	dl_list_for_each(bss, bss_list, struct BSS, node) {
		json_t* jbss;
		int err = bss_to_json(bss, &jbss);
		XASSERT(err==0, err);
		char* s = json_dumps(jbss, 0);
		printf("%s\n", s);
		PTR_FREE(s);
		json_decref(jbss);
	}
}

static void dump_from_file(struct args* args)
{
	int err = 0;

	DEFINE_DL_LIST(bss_list);

	err = dumpfile_parse(args->dump_filename, &bss_list);
	XASSERT(err==0, err);

	json_summary(&bss_list);
	json_summary_list(&bss_list);

	json_full(&bss_list);
	json_full_list(&bss_list);

	bss_free_list(&bss_list);
}

int main(int argc, char* argv[])
{
	struct args args;

	memset(&args,0,sizeof(struct args));
	int err = args_parse(argc, argv, &args);

	if (err) {
		exit(EXIT_FAILURE);
	}

	if (args.debug > 0) {
		log_set_level(LOG_LEVEL_DEBUG);
	}

	if (args.load_dump_file) {
		dump_from_file(&args);
	}

	return EXIT_SUCCESS;
}

