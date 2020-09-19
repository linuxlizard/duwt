/*
 * libiw/args.c   command line parser
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core.h"
#include "args.h"

const char *version = "1.0.0";

static void usage(void)
{
	fprintf(stderr, "scan-dump %s  Dump wifi survey using cfg80211.\n",version);
	fprintf(stderr, "Usage: scan-dump -[hd] [-s dumpfile] interface\n");
	fprintf(stderr, " -h  this help\n");
	fprintf(stderr, " -d  debug; use multiple times to increase verbosity\n");
	fprintf(stderr, " -o filename  write nl80211 capture to file for later examination\n");
	fprintf(stderr, " -i filename  read saved nl80211 capture file instead of wifi interface\n");
	fprintf(stderr, " interface   wifi device to query for scan results\n");
	fprintf(stderr, "Example:\n");
	fprintf(stderr, " scan-dump -d -s /tmp/wlan0-dump.dat wlan0\n");

}

int args_parse(int argc, char* argv[], struct args* args)
{
	memset(args, 0, sizeof(struct args));

	if (argc < 2) {
		usage();
		exit(1);
	}

	while (1) {

		int ret = getopt(argc, argv, "dhi:o:");
		if (ret == -1) {
			break;
		}

		switch (ret) {
			case 'd':
				args->debug++;
				break;

			case 'h':
				usage();
				exit(1);

			case 'o':
				args->save_dump_file = true;
				strncpy(args->dump_filename, optarg, FILENAME_MAX);
				break;

			case 'i':
				args->load_dump_file = true;
				strncpy(args->dump_filename, optarg, FILENAME_MAX);
				break;

			default:
				fprintf(stderr, "unhandled option '%c'\n", (char)ret);
				return -EINVAL;
		}
	}

	if (args->save_dump_file && args->load_dump_file) {
		// this doesn't make any sense
		fprintf(stderr, "error: cannot specify both load-dump-file and save-dump-file\n");
		fprintf(stderr, "see -h for help\n");
		return -EINVAL;
	}

	// save remaining arguments
	for (int i=optind ; i<argc ; i++) {
		args->argv[args->argc++] = argv[i];
		if (args->debug) {
			printf("%s i=%d argv[i]=%s\n", __func__, i, argv[i]);
		}
	}

	// must have one interface  unless reading from a dump
	if (args->argc != 1 && !args->load_dump_file) {
		fprintf(stderr, "error: need a wifi interface to read\n");
		fprintf(stderr, "see -h for help\n");
		exit(1);
	}

	return 0;
}

