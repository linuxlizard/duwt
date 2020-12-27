/*
 * survey.cc  a container to hold the wifi survey 
 *
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */
#include <iostream>

#include "jansson.h"

#include "iw.h"
#include "bss.h"
#include "bss_json.h"
#include "survey.h"

struct BSS;

void Survey::store(struct BSS* bss)
{
	// if the entry exists, must free the memory first
	// (this is a C-allocated struct so no destructor for you)
	try {
		struct BSS* old_bss = survey.at(bss->bssid_str);
		// free the memory
		bss_free(&old_bss);
	}
	catch (std::out_of_range& err) {
		// not found
	}

	// replace
	survey[bss->bssid_str] = bss;
}

std::optional<const struct BSS*> Survey::find(std::string bssid) const
{
	try {
		return survey.at(bssid);
	}
	catch (std::out_of_range& err) {
		return {};
	}
}


std::optional<std::reference_wrapper<const std::string>> Survey::get_json(std::string bssid) 
{

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

	// release jansson memory
	json_decref(j_bss);
	free(s);

	return std::ref(json.at(bssid));
}

