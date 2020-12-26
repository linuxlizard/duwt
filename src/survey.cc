/*
 * survey.cc  a container to hold the wifi survey 
 *
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */
#include <iostream>

#include "iw.h"
#include "bss.h"
#include "survey.h"

struct BSS;

void Survey::store(struct BSS* bss)
{
	struct BSS* old_bss=nullptr;
	// if the entry exists, must remove+free the memory first
	try {
		old_bss = survey.at(bss->bssid_str);
	}
	catch (std::out_of_range& err) {
		// not found
	}

	if (old_bss) {
		// free the memory
		bss_free(&old_bss);
	}

	// replace
	survey[bss->bssid_str] = bss;
}

std::optional<const struct BSS*> Survey::find(std::string bssid) const
{
	const struct BSS* bss;
	try {
		return survey.at(bssid);
	}
	catch (std::out_of_range& err) {
		return {};
	}
}


