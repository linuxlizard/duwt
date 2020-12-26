/*
 * test_survey.cc  test file for survey.cc
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <iostream>
#include <string>

#include <jansson.h>

#include "catch.hpp"

#include "list.h"
#include "iw.h"
#include "ie.h"
#include "bss.h"
#include "dumpfile.h"
#include "bss_json.h"

#include "survey.h"


TEST_CASE("Load file", "[dumpfile]"){
	const std::string dump_filename { "soup.dat" };
	DEFINE_DL_LIST(bss_list);
	int err = dumpfile_parse(dump_filename.c_str(), &bss_list);
	REQUIRE(err==0);

	SECTION("sanity check loaded file") {
		json_t* jlist=NULL;

		int err = bss_list_to_json(&bss_list, &jlist, 0);
		REQUIRE(err==0);

		char* s = json_dumps(jlist, JSON_INDENT(1));
//		printf("%s\n", s);
		json_array_clear(jlist);
		json_decref(jlist);
		free(s);

	}

	SECTION("load into survey") {
		struct BSS* bss;

		Survey survey;
		dl_list_for_each(bss, &bss_list, struct BSS, node) {
			survey.store(bss);
		}

		// look for the last BSS in the Survey
		auto maybe_bss_ptr = survey.find(bss->bssid_str);
		if (maybe_bss_ptr) {
			const struct BSS* found_bss = maybe_bss_ptr.value();
			std::string s1 { found_bss->bssid_str };
			std::string s2 { bss->bssid_str };
			REQUIRE(s1==s2);
		}
	}

	// can I put code here? 
	// apparently I can
	std::cout << "bye!\n";
	bss_free_list(&bss_list);
}

