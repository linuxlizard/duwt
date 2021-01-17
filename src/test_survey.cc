/*
 * test_survey.cc  test file for survey.cc
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#include <iostream>
#include <string>
#include <thread>

// my libiw uses jansson to encode a BSS into json
#include <jansson.h>

#include "catch.hpp"

#include "list.h"
#include "iw.h"
#include "ie.h"
#include "bss.h"
#include "dumpfile.h"
#include "bss_json.h"

#include "survey.h"

// XXX can't include my libiw log.h because it collides with catch.hpp symbols
// (no idea how to fix)
extern "C" void log_set_level(int level);

TEST_CASE("Load file", "[dumpfile]"){
	// can't include my libiw log.h because it collides with catch.hpp symbols
//	log_set_level(1);  // LOG_LEVEL_ERR
	log_set_level(2);  // LOG_LEVEL_WARN

	// TODO add my own cli args to catch
	const std::string dump_filename { "soup.dat" };
	DEFINE_DL_LIST(bss_list);
	int err = dumpfile_parse(dump_filename.c_str(), &bss_list);
	REQUIRE(err==0);
	std::cout << "dumpfile " << dump_filename << " loaded\n";

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

		size_t counter=0;
		Survey survey;
		dl_list_for_each(bss, &bss_list, struct BSS, node) {
			survey.store(bss);

			// find what we just stored
			auto findme = survey.find(bss->bssid_str);
			REQUIRE(findme);

			counter++;
			REQUIRE(counter==survey.size());
		}
		REQUIRE(dl_list_len(&bss_list) == survey.size());

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

	SECTION("erase") {
		// test removal from the survey cache
		struct BSS* bss;

		Survey survey;
		dl_list_for_each(bss, &bss_list, struct BSS, node) {
			survey.store(bss);
		}

		bss = dl_list_first(&bss_list, struct BSS, node);

		// remove from the bss_list because survey.erase() will free the memory
		// and we need to keep the bss_list consistent
		dl_list_del(&bss->node);

		// save the bssid_str
		std::string save_bssid_str { bss->bssid_str };

		REQUIRE(survey.erase(save_bssid_str));

		// !! do NOT use bss ptr after this point! will be released memory !!

		auto findme = survey.find(save_bssid_str);
		REQUIRE(!findme);

		REQUIRE(!survey.erase("00:00:00:00:00:00"));
	}


	SECTION("capture JSON") {
		struct BSS* bss;

		Survey survey;
		dl_list_for_each(bss, &bss_list, struct BSS, node) {
			survey.store(bss);
		}

		struct BSS* findme = dl_list_first(&bss_list, struct BSS, node);
		auto maybe_json = survey.get_json_bssid(findme->bssid_str);
		REQUIRE(maybe_json.has_value());

		std::cout << maybe_json.value().get() << "\n";
	}

	SECTION("get_survey") {
		struct BSS* bss;
		Survey survey;

		dl_list_for_each(bss, &bss_list, struct BSS, node) {
			survey.store(bss);
		}
		std::string survey_json = survey.get_json_survey();
		std::cout << survey_json << "\n";
	}

	SECTION("survey update") {
		// when a new sample of a BSS arrives, replace the old version
		struct BSS* bss;

		Survey survey;

		// load the same dumpfile into a separate list
		DEFINE_DL_LIST(new_bss_list);
		err = dumpfile_parse(dump_filename.c_str(), &new_bss_list);
		REQUIRE(err==0);

		// note: I'm repeatedly calling dl_list_len() instead of storing its
		// size_t because dl_list_len() also serves as a check for data
		// corruption (found problems with my original test that way)

		// store the 2nd dumpfile load into the survey
		dl_list_for_each(bss, &new_bss_list, struct BSS, node) {
			survey.store(bss);
		}
		REQUIRE(dl_list_len(&new_bss_list) == survey.size());

		// Now replace the survey elements with the outer copy of the bss_list.
		// The elements of new_bss_list will be freed when the new value
		// overwrites.
		dl_list_for_each(bss, &bss_list, struct BSS, node) {
			auto findme = survey.find(bss->bssid_str);
			REQUIRE(findme);

			survey.store(bss);
			// we're updating, not adding a new element, so we should have the same size
			REQUIRE(dl_list_len(&bss_list) == survey.size());
		}

		// no need to bss_free_list(&new_bss_list) because all the elements
		// should be individually freed (which breaks the list)
	}

	SECTION("json evict") {
		// when a new sample of a BSS arrives, must evict the old JSON
		// representation from the cache.
		// when a new sample of a BSS arrives, replace the old version
		struct BSS* bss;

		Survey survey;
		dl_list_for_each(bss, &bss_list, struct BSS, node) {
			survey.store(bss);
		}

		// get some json
		struct BSS* first = dl_list_first(&bss_list, struct BSS, node);
		auto maybe_json = survey.get_json_bssid(first->bssid_str);
		REQUIRE(maybe_json.has_value());

		// we will remove the BSS from the survey so must remove from the
		// bss_list (survey.erase() will free() the BSS memory which would
		// corrupt the dl_list)
		dl_list_del(&first->node);

		// make an empty bss
		bss = bss_new();
		REQUIRE(bss != nullptr);

		// give it the same identity as the element we'll remove
		memcpy(bss->bssid, first->bssid, sizeof(bss->bssid));
		memcpy(bss->bssid_str, first->bssid_str, sizeof(bss->bssid_str));

		// replace 'first' with the empty-ish bss
		survey.store(bss);
		first = nullptr;  // points into freed memory so make safe

		// at this point store() should have flushed the BSS from the json
		// cache so a new json string would have to be generated
		auto maybe_json_after_store = survey.get_json_bssid(bss->bssid_str);
		REQUIRE(maybe_json_after_store.has_value());

		// the new json should be teeny tiny
		std::clog << maybe_json_after_store.value().get() << "\n";

		// the original json should be huge and the new json should be small
		REQUIRE(maybe_json.value().get() != maybe_json_after_store.value().get());

		// save the bssid so can search for it after erase()
		std::string new_bssid_str { bss->bssid_str };

		// remove the from the survey
		survey.erase(bss->bssid_str);
		bss = nullptr;

		// at this point erase() should have flushed the BSS from the json
		// cache 
		auto maybe_json_after_del = survey.get_json_bssid(new_bssid_str);
		REQUIRE(!maybe_json_after_del.has_value());
	}

	SECTION("threads") {
		// My webserver is using a thread for cfg80211 listening and another
		// thread(s) under the poco-httpd server. So my survey data structure
		// must be threadsafe.

		struct BSS* bss;

		Survey survey;

		bool stop { false };
		std::thread query ( [&]() {
			int counter = 0;
			while(!stop) {
				std::clog << "thread " << counter++ << "\n";
				survey.find("00:00:00:00:00:00");
			}
		} );

		dl_list_for_each(bss, &bss_list, struct BSS, node) {
			survey.store(bss);
		}
		stop = true;
		query.join();
		std::clog << "stats " << survey.stats_get() << "\n";
	}

	// can I put code here? 
	// apparently I can. Each SECTION runs all this outer code first/last.
	std::cout << "bye!\n";
	bss_free_list(&bss_list);
}

