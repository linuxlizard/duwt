#ifndef ARGS_H
#define ARGS_H

#define ARGS_MAX  64   // "64 args should be enough for anyone." --billg

struct args 
{
	int debug;

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

