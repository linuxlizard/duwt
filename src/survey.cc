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
#include <thread>
#include <chrono>

#include "jansson.h"

#include "core.h"
#include "iw.h"
#include "bss.h"
#include "bss_json.h"
#include "survey.h"
#include "oui.h"

// add the OUI to the json encoded BSS
static void add_oui(ieeeoui::OUI_MA* oui, json_t* j_bss, const struct BSS* bss)
{
	uint8_t oui_bytes[3] { bss->bssid[0], bss->bssid[1], bss->bssid[2] };
	try {
		std::string oui_str = oui->get_org_name(oui_bytes);

		json_t* j_oui = json_string(oui_str.c_str());
		if (j_oui) {
			json_object_set_new(j_bss, "oui", j_oui);
			j_oui = nullptr; // belongs to j_bss now
		}
	} 
	catch (std::out_of_range& err) {
		// ignore 
	}
}

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

std::optional<const struct BSS*> Survey::_locked_find(std::string bssid)
{
	// do the search with the lock held
	counters.find++;
	try {
		return survey.at(bssid);
	}
	catch (std::out_of_range& err) {
		counters.not_found++;
		return {};
	}
}

std::optional<const struct BSS*> Survey::find(std::string bssid)
{
#ifdef DEBUG
	std::clog << "find " << bssid << "\n";
#endif
	const std::lock_guard<std::mutex> local_lock(lock);
	return _locked_find(bssid);
}

size_t Survey::size(void)
{
	const std::lock_guard<std::mutex> local_lock(lock);
	return survey.size();
}

std::optional<std::reference_wrapper<const std::string>> Survey::get_json_bssid(std::string bssid) 
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
	auto findme = _locked_find(bssid);
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

	// add OUI, if possible
	if (oui) {
		add_oui(oui, j_bss, bss);
	}

	char* s = json_dumps(j_bss, JSON_INDENT(1));
	if (!s) {
		json_decref(j_bss);
		return {};
	}

	// store the json encoded string back to our map
	json.emplace( std::make_pair(bssid, std::string(s)) );
	counters.json_add++;

	// release jansson memory
	json_decref(j_bss);
	PTR_FREE(s);

	return std::ref(json.at(bssid));
}

std::string Survey::get_json_survey(Decode decode)
//std::string Survey::get_json_survey(void)
{
	// get entire survey as json (expensive)

	const std::lock_guard<std::mutex> local_lock(lock);

	json_t* jarray = json_array();

	for (auto iter : survey) {
		struct BSS* bss = iter.second;
//		XASSERT(bss->cookie == BSS_COOKIE, bss->cookie);

		json_t* jbss;

		int err = bss_to_json(bss, &jbss, decode == Decode::short_ie ? BSS_JSON_SHORT_IE_DECODE : 0);
		// TODO error checking
		(void)err;

		// add OUI, if possible
		if (oui) {
			add_oui(oui, jbss, bss);
		}

		err = json_array_append_new(jarray, jbss);
		// TODO error checking
		(void)err;
	}

	char* s = json_dumps(jarray, 0);
	if (!s) {
//		ERR("%s json dump failed\n", __func__);
		json_decref(jarray);
		return "{}";
	}

	json_decref(jarray);

	std::string survey_str { s };
	PTR_FREE(s);
	return survey_str;
}

std::string Survey::stats_get(void)
{
	const std::lock_guard<std::mutex> local_lock(lock);
	return counters.get();
}

void Survey::add_oui_db(ieeeoui::OUI_MA* oui_db)
{
	const std::lock_guard<std::mutex> local_lock(lock);
	// for now, just support one table
	this->oui = oui_db;
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

