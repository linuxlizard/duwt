/*
 * survey.h  header file for survey.cc  the container to hold the wifi survey 
 *
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */
#ifndef SURVEY_H
#define SURVEY_H

#include <utility>
#include <unordered_map>
#include <optional>
#include <functional>
#include <mutex>

#include "oui.h"

struct BSS;

struct Counters 
{
	Counters() { reset(); };

	std::string get(void);
	void reset(void);

	unsigned long int add;
	unsigned long int update;
	unsigned long int erase;
	unsigned long int find;
	unsigned long int not_found;
	unsigned long int json_add;
};

// thread-safe container to hold survey results
class Survey
{
public:

	void store(struct BSS* bss);

	bool erase(std::string bssid);

	std::optional<const struct BSS*> find(std::string bssid);

	// return the json representation of the BSS
	// (note to future self: std::optional cannot contain references)
	// https://stackoverflow.com/questions/26858034/stdoptional-specialization-for-reference-types
//	std::optional<const std::string*> json_of(std::string bssid) const;
	std::optional<std::reference_wrapper<const std::string>> get_json_bssid(std::string bssid);

	enum class Decode { full, short_ie };

	// get survey as json
	std::string get_json_survey(Decode decode=Decode::short_ie);

	size_t size(void);

	void stats_reset(void);
	std::string stats_get(void);

	void add_oui_db(ieeeoui::OUI_MA* oui);

private:
	// key: bss->bssid_str
	// value: ptr to BSS
	std::unordered_map<std::string, struct BSS*> survey;

	// JSON encoded value of a BSSID is created on request then cached.
	//
	// key: bssid
	// value: json (built with jansson in libiw then converted to std::string)
	std::unordered_map<std::string, std::string> json;

	// protect the contents (scan survey runs in its own thread)
	std::mutex lock;

	// do the search with the lock held because we need to find from multiple
	// methods
	std::optional<const struct BSS*> _locked_find(std::string bssid);

	// track the survey cache behavior (for debugging)
	Counters counters;

	// for oui lookup
	ieeeoui::OUI_MA* oui;
};

#endif

