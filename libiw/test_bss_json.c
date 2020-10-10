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

static void dump_from_file(struct args* args)
{
	struct BSS* bss;
	int err = 0;

	DEFINE_DL_LIST(bss_list);

	err = dumpfile_parse(args->dump_filename, &bss_list);
	XASSERT(err==0, err);

	dl_list_for_each(bss, &bss_list, struct BSS, node) {
//		print_short(bss);
		json_t* jbss;
		err = bss_to_json_summary(bss, &jbss);
		char* s = json_dumps(jbss, JSON_INDENT(3));
		printf("%s\n", s);
		json_decref(jbss);
	}

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
		return 0;
	}

	return EXIT_SUCCESS;
}

