#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

#define ARGS_MAX  64   // "64 args should be enough for anyone." --billg

struct args 
{
	int debug;

	// save nl80211 to dump file for later inspection
	bool save_dump_file;

	// load a dump file instead of reading interface
	bool load_dump_file;

	char dump_filename[FILENAME_MAX+1];

	// remaining non-option arguments
	int argc;
	char* argv[ARGS_MAX];
};

#ifdef __cplusplus
extern "C" {
#endif

int args_parse(int argc, char* argv[], struct args* args);

#ifdef __cplusplus
} // end extern "C"
#endif


#endif

