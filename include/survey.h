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

struct BSS;

// thread-safe container to hold survey results
class Survey
{
public:

	void store(struct BSS* bss);

	std::optional<const struct BSS*> find(std::string bssid) const;

	// return the json representation of the BSS
	// (note to future self: std::optional cannot contain references)
	// https://stackoverflow.com/questions/26858034/stdoptional-specialization-for-reference-types
//	std::optional<const std::string*> json_of(std::string bssid) const;
	std::optional<std::reference_wrapper<const std::string>> get_json(std::string bssid);

private:
	// key: bss->bssid_str
	// value: ptr
	std::unordered_map<std::string, struct BSS*> survey;

	// JSON encoded value of a BSSID is created on request then cached.
	//
	// key: bssid
	// value: json (built with jansson in libiw then converted to std::string)
	std::unordered_map<std::string, std::string> json;
};

#endif

