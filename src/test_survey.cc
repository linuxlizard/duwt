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


extern "C" void log_set_level(int level);

TEST_CASE("Load file", "[dumpfile]"){
	// can't include my libiw log.h because it collides with catch.hpp symbols
	log_set_level(1);  // LOG_LEVEL_ERR
//	log_set_level(2);  // LOG_LEVEL_WARN

	const std::string dump_filename { "soup.dat" };
	DEFINE_DL_LIST(bss_list);
	int err = dumpfile_parse(dump_filename.c_str(), &bss_list);
	REQUIRE(err==0);
	std::cout << "load\n";

	SECTION("sanity check loaded file") {
		json_t* jlist=NULL;

		err = bss_list_to_json(&bss_list, &jlist, 0);
		REQUIRE(err==0);

		char* s = json_dumps(jlist, JSON_INDENT(1));
		REQUIRE(s);
//		printf("%s\n", s);
		json_array_clear(jlist);
		json_decref(jlist);
		free(s);
	}

	SECTION("load into Survey") {
		struct BSS* bss;

		Survey survey;
		dl_list_for_each(bss, &bss_list, struct BSS, node) {
			survey.store(bss);
		}

		// look for the last BSS in the Survey
		struct BSS* findme = dl_list_last(&bss_list, struct BSS, node);
		auto maybe_bss_ptr = survey.find(findme->bssid_str);
		REQUIRE(maybe_bss_ptr.has_value());

		// verify we found what we were looking for
		const struct BSS* found = maybe_bss_ptr.value();
		REQUIRE(findme==found);
		std::string s1 { found->bssid_str };
		std::string s2 { findme->bssid_str };
		REQUIRE(s1==s2);
		
		// search for something that isn't there
		std::string zero { "00:00:00:00:00:00" };
		maybe_bss_ptr = survey.find(zero);
		REQUIRE(!maybe_bss_ptr.has_value());
	}

	SECTION("capture JSON") {
		struct BSS* bss;

		Survey survey;
		dl_list_for_each(bss, &bss_list, struct BSS, node) {
			survey.store(bss);
		}

		struct BSS* findme = dl_list_first(&bss_list, struct BSS, node);
		auto maybe_json = survey.get_json(findme->bssid_str);
		REQUIRE(maybe_json.has_value());

		std::cout << maybe_json.value().get() << "\n";
	}

	// can I put code here? 
	// apparently I can. Each SECTION runs all this outer code first/last.
	std::cout << "bye!\n";
	bss_free_list(&bss_list);
}

