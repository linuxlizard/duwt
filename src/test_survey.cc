/*
 * test_survey.cc  test file for survey.cc
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

// if I'm not using the pre-compiled header catch.hpp, then pull it in now
#ifndef CATCH_CONFIG_MAIN
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#endif

//#include "core.h"
#include "iw.h"
#include "ie.h"
#include "bss.h"
#include "dumpfile.h"

#include "survey.h"

int load_dumpfile(std::string& dumpfile_name)
{
//	log_set_level(LOG_LEVEL_DEBUG);

	struct BSS* bss;

	DEFINE_DL_LIST(bss_list);
	int err = dumpfile_parse(dumpfile_name.c_str(), &bss_list);
//	XASSERT(err==0, err);


	return 0;
}


