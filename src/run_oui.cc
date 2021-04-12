/*
 * run_oui.cc 
 *
 * read mac addresses from stdin ; look them up in the oui db
 *
 * Copyright (c) 2021 David Poole <davep@mbuf.com>
 */

#include <iostream>
#include <algorithm>

#include "oui.h"

int main(int argc, char *argv[])
{
	std::string path = "/usr/share/hwdata/oui.txt";
	ieeeoui::OUI_MA oui { path };

	std::string arg;
	while (true) {
//		std::string arg { argv[i] };
		std::getline(std::cin, arg);
		if (arg.length() == 0) {
			break;
		}

		// TODO error checking (shame!)
		std::string s = arg.substr(0,8);
		s.erase(2,1);
		s.erase(4,1);

		// https://stackoverflow.com/questions/7131858/stdtransform-and-toupper-no-matching-function
		std::string findme;
		std::transform(
		   s.begin(),
		   s.end(),
		   std::back_inserter(findme),
		   ::toupper                  // global scope
		);
		std::cout << "findme=" << findme << "\n";

		uint32_t num32 = ieeeoui::string_to_oui(findme);
		std::string org = oui.get_org_name(num32);
		std::cout << "org=" << org << "\n";
	}

	return 0;
}
