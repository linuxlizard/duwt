#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core.h"
#include "args.h"

int args_parse(int argc, char* argv[], struct args* args)
{
	memset(args, 0, sizeof(struct args));

	while (1) {

		int ret = getopt(argc, argv, "dh");
		if (ret == -1) {
			break;
		}

		switch (ret) {
			case 'd':
				args->debug++;
				break;

			case 'h':
				break;

			default:
				fprintf(stderr, "unhandled option '%c'\n", (char)ret);
				return -EINVAL;
		}

		for (int i=optind ; i<argc ; i++) {
			args->argv[args->argc++] = argv[i];
			printf("%s i=%d argv[i]=%s\n", __func__, i, argv[i]);
		}


	}

	return 0;
}

