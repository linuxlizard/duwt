/*
 * survey.h  header file for survey.cc  the container to hold the wifi survey 
 *
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */
#ifndef SURVEY_H
#define SURVEY_H

#include <unordered_map>

struct BSS;

// thread-safe container to hold survey results
class Survey
{
public:

	void store(struct BSS* bss);

	std::optional<const struct BSS*> find(std::string bssid) const;

private:
	// key: bss->bssid_str
	// value: ptr
	std::unordered_map<std::string,struct BSS*> survey;

};

#endif

