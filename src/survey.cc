/*
 * survey.cc  a container to hold the wifi survey 
 *
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */
//#define DEBUG

#ifdef DEBUG
#include <iostream>
#endif
#include <sstream>
#include <string>
#include <stdexcept>
#include <cassert>

#include "jansson.h"

#include "iw.h"
#include "bss.h"
#include "bss_json.h"
#include "survey.h"

struct BSS;

void Survey::store(struct BSS* bss)
{
	assert(bss);
	assert(bss->cookie == BSS_COOKIE);

	const std::lock_guard<std::mutex> local_lock(lock);

#ifdef DEBUG
	std::clog << "store " << bss->bssid_str << "\n";
#endif

	// if the entry exists, must free the memory first
	// (this is a C-allocated struct so no destructor for you)
	try {
		struct BSS* old_bss = survey.at(bss->bssid_str);

#ifdef DEBUG
		std::clog << "store remove " << bss->bssid_str << "\n";
#endif
		// remove from json, too
		json.erase(bss->bssid_str);

		// free the memory
		bss_free(&old_bss);

		counters.update++;
	}
	catch (std::out_of_range& err) {
		// not found
#ifdef DEBUG
		std::clog << "store new " << bss->bssid_str << "\n";
#endif
		counters.add++;
	}

	// replace
	survey[bss->bssid_str] = bss;
}

bool Survey::erase(std::string bssid)
{
	struct BSS* bss = nullptr;

	const std::lock_guard<std::mutex> local_lock(lock);

#ifdef DEBUG
	std::clog << "erase " << bssid << "\n";
#endif
	try {
		bss = survey.at(bssid);
	}
	catch (std::out_of_range& err) {
		return false;
	}

	// remove from map
	survey.erase(bssid);

	// release the C allocated memory
	bss_free(&bss);

	// remove from json, too
	json.erase(bssid);

	counters.erase++;
	return true;
}

std::optional<const struct BSS*> Survey::find(std::string bssid)
{
#ifdef DEBUG
	std::clog << "find " << bssid << "\n";
#endif
	const std::lock_guard<std::mutex> local_lock(lock);

	counters.find++;
	try {
		return survey.at(bssid);
	}
	catch (std::out_of_range& err) {
		counters.not_found++;
		return {};
	}
}

size_t Survey::size(void)
{
	const std::lock_guard<std::mutex> local_lock(lock);
	return survey.size();
}


std::optional<std::reference_wrapper<const std::string>> Survey::get_json(std::string bssid) 
{
	const std::lock_guard<std::mutex> local_lock(lock);

	// check the json cache
	try {
		return std::ref(json.at(bssid));
	}
	catch (std::out_of_range& err) {
		// no such entry
	}

	// at this point, we have to create it
	auto findme = find(bssid);
	if (!findme) {
		// no such BSS
		return {};
	}

	const struct BSS* bss = findme.value();

	json_t* j_bss;
	int ret = bss_to_json(bss, &j_bss, 0);
	if (ret != 0) {
		return {};
	}

	char* s = json_dumps(j_bss, JSON_INDENT(1));
	if (!s) {
		return {};
	}

//	std::string str_json { s };

//	json[bssid] = s;
	json.emplace( std::make_pair(bssid, std::string(s)) );
	counters.json_add++;

	// release jansson memory
	json_decref(j_bss);
	free(s);

	return std::ref(json.at(bssid));
}

std::string Survey::stats_get(void)
{
	const std::lock_guard<std::mutex> local_lock(lock);
	return counters.get();
}

std::string Counters::get(void)
{
	std::ostringstream stats;
	stats << "add=" << add 
		  << " update=" << update
		  << " erase=" << erase
		  << " find=" << find
		  << " not_found=" << not_found
		  << " json_add=" << json_add;
	return stats.str();
}

void Survey::stats_reset(void)
{
	const std::lock_guard<std::mutex> local_lock(lock);
	counters.reset();
}

void Counters::reset(void)
{
	add = 0;
	update = 0;
	erase = 0;
	find = 0;
	not_found = 0;
	json_add = 0;
}

